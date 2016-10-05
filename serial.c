
#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/crc16.h>

#include "config.h"
#include "common.h"
#include "serial.h"

////////////////////////////////////////////
/// Macros
////////////////////////////////////////////

#define SOM_BYTE 0xFF

#define POS_SOM1 0
#define POS_SOM2 1

#define TYPE_PROG_START 0xF1
#define TYPE_PAGE_NUM   0xF2
#define TYPE_PAGE_DATA  0xF3
#define TYPE_PROG_END   0xF4

////////////////////////////////////////////
/// Globals Variables
////////////////////////////////////////////
uint8_t pageData[SPM_PAGESIZE];

////////////////////////////////////////////
/// Local Variables
////////////////////////////////////////////

static uint8_t readyForPages = 0;

static uint8_t msgPos = 0;
static uint8_t msgType = 0;
static uint8_t msgLen = 0;

static uint8_t upcomingPage = 0;

static uint16_t msgCRC;
static uint16_t nextPageNumber;


////////////////////////////////////////////
/// Local Prototypes
////////////////////////////////////////////


void error();
void reset();
uint8_t readAndParse();
uint8_t processMessage();


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
  if (readyForPages) {
    signalEnable();
    upcomingPage = 0;
  }
}

// Reset the message
void reset() {
  msgPos = 0;
  msgCRC = ~0;
  msgLen = 0;
}

// Watch the serial line for the next message
uint8_t watchSerial(uint8_t pagesRead) {
  nextPageNumber = pagesRead;
  reset();

  while (1) {
    uint8_t status = readAndParse();
    if (status != STATUS_NONE) {
      break;
    }
  }
}

// Read from the serial line and parse the bytes as they come in
uint8_t readAndParse() {
  uint8_t b = serialReceive();

  // Start of message
  if (msgPos == POS_SOM1 && b == SOM_BYTE) {
    msgPos++;
  }
  else if (msgPos == POS_SOM2 && b == SOM_BYTE) {
    msgPos++;

    // Header
    serialReceiveWithCRC(); // flags (ignored)
    serialReceiveWithCRC(); // addr (ignored)
    msgType = serialReceiveWithCRC();
    serialReceiveWithCRC(); // len per section (ignored)
    msgLen = serialReceiveWithCRC();

    // Data
    if (msgLen) {
      uint8_t dataReceived = 0;
      while (dataReceived < msgLen) {
        pageData[dataReceived++] = serialReceiveWithCRC();
      }
    }

    // CRC Validation
    uint8_t crc1 = serialReceive();
    uint8_t crc2 = serialReceive();
    uint16_t fullCrc = (crc1 << 8 ) | (crc2 & 0xff);
    if (fullCrc != msgCRC){
      error();
      reset();
      return STATUS_NONE;
    }

    // Message received
    uint8_t status = processMessage();
    reset();
    return status;
  }
  else {
    error();
    reset();
  }

  return STATUS_NONE;
}

// Do something with the received message
uint8_t processMessage() {

#ifdef DEBUG
  serialWrite(0x00);
  serialWrite(msgType);
#endif

  // Let master node know we're ready by disabling signal
  if (msgType == TYPE_PROG_START) {
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

  // We're done programming
  else if (msgType == TYPE_PROG_END) {
#ifdef DEBUG
  serialWrite(0x85);
#endif
    readyForPages = 0;
    return STATUS_DONE;
  }

  else if (readyForPages == 1) {

    // Set upcoming page number
    if (msgType == TYPE_PAGE_NUM) {
      upcomingPage = pageData[0];
    }

    // Load the next page
    else if (msgType == TYPE_PAGE_DATA) {

      // Length and page size do not match up
      // or this is not the page we were expecting
      if (msgLen > SPM_PAGESIZE || upcomingPage != nextPageNumber) {

#ifdef DEBUG
  serialWrite(0x69);
#endif
        error();
      }
      // Valid page
      else {

        // Fill in rest of data with 0xFF
        if (msgLen < SPM_PAGESIZE) {
          for (uint8_t i = msgLen - 1; i < SPM_PAGESIZE; i++) {
            pageData[i] = 0xFF;
          }
        }

#ifdef DEBUG
  serialWrite(0x10);
#endif
        signalDisable();
        return STATUS_PAGE_READY;
      }
    }
  }

  return STATUS_NONE;
}