
#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/crc16.h>

#include "config.h"
#include "macros.h"
#include "serial.h"

////////////////////////////////////////////
/// Macros
////////////////////////////////////////////

#define SOM_BYTE 0xFF 

#define POS_SOM1 0
#define POS_SOM2 1

#define TYPE_START 0xF1
#define TYPE_PAGE  0xF2
#define TYPE_END   0xF3

////////////////////////////////////////////
/// Globals Variables
////////////////////////////////////////////
uint8_t pageReady = 0;
uint8_t programDone = 0;
uint8_t pageData[SPM_PAGESIZE];

////////////////////////////////////////////
/// Local Variables
////////////////////////////////////////////

static uint8_t readyForPages = 0;

static uint8_t msgPos = 0;
static uint8_t msgType = 0;
static uint8_t msgPageNum = 0;
static uint8_t msgLen = 0;

static uint16_t msgCRC;
static uint16_t nextPageNumber;

////////////////////////////////////////////
/// Local Prototypes
////////////////////////////////////////////

void error();
void reset();
void readAndParse();
void processMessage();

////////////////////////////////////////////
/// Methods
////////////////////////////////////////////


static inline uint8_t serialReceiveWithCRC() {
  uint8_t b = serialReceive();
  msgCRC = _crc16_update(msgCRC, b);
  return b;
}

// Inform master an error occured while reading the message
void error() {
  signalEnable();
}

// Reset the message
void reset() {
  msgPos = 0;
  msgCRC = ~0;
  msgLen = 0;
}

// Watch the serial line for the next message
void watchSerial(uint8_t pagesRead) {
  nextPageNumber = pagesRead;
  reset();

  while (1) {
    readAndParse();

    if (pageReady || programDone) {
      return;
    }
  }
}

// Read from the serial line and parse the bytes as they come in
void readAndParse() {
  uint8_t b = serialReceive();

  // Start of message
  if (msgPos == POS_SOM1 && b == SOM_BYTE) {
    msgPos++;
  }
  else if (msgPos == POS_SOM2 && b == SOM_BYTE) {
    msgPos++;

    // Header
    msgType = serialReceiveWithCRC();
    msgPageNum = serialReceiveWithCRC();
    msgLen = serialReceiveWithCRC();

    // Data
    uint8_t pageReceived = 0;
    while (pageReceived < msgLen) {
      pageData[pageReceived++] = serialReceiveWithCRC();
    }

    // CRC
    serialReceiveWithCRC();
    serialReceiveWithCRC();

    // Validate CRC
    if (msgCRC == 0) {
      processMessage();
      reset();
    } else {
      error();
      reset();
      return;
    }
  }
  else {
    error();
    reset();
  }
}

// Do something with the received message
void processMessage() {

  // Let master node know we're ready by disabling signal
  if (TYPE_START == msgType) {
    signalDisable();

    // Check version number
    if (msgLen == 2 
        && pageData[0] == eeprom_read_byte(EEPROM_VER_MAJ) 
        && pageData[1] == eeprom_read_byte(EEPROM_VER_MIN) ) {
      readyForPages = 0;
    }
    else {
      readyForPages = 1;
    }
    
  }

  // Load the next page
  else if (TYPE_PAGE == msgType && readyForPages == 1) { 
    if (msgPageNum == nextPageNumber) {

      // Fill in rest of data with 0xFF
      if (msgLen < SPM_PAGESIZE) {
        for (uint8_t i = msgLen - 1; i < SPM_PAGESIZE; i++) {
          pageData[i] = 0xFF;
        }
      }

      pageReady = 1;
      signalDisable();
    }
    else {
      error();
      reset();
    }
  }

  // We're done programming
  else if (TYPE_END == msgType) {
    programDone = 1;
  }
}