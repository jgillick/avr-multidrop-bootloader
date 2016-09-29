
#include <avr/io.h>
#include <util/crc16.h>

#include "serial.h"

////////////////////////////////////////////
/// Macros
////////////////////////////////////////////

#define	serialReceiveReady() (UCSR0A & (1<<RXC0))

#define UART_BAUD_SELECT(baudRate)  (((F_CPU) + 8UL * (baudRate)) / (16UL * (baudRate)) -1UL)

#define SOM_BYTE 0xFF 

#define MSG_SOM1 0
#define MSG_SOM2 1
#define MSG_TYPE 2
#define MSG_ADDR 3
#define MSG_LEN  4
#define MSG_DATA 5
#define MSG_CRC1 6
#define MSG_CRC2 6

#define TYPE_START 0xF1
#define TYPE_PAGE  0xF2
#define TYPE_END   0xF3

////////////////////////////////////////////
/// Globals Variables
////////////////////////////////////////////
uint8_t pageReady = 0;
uint8_t pageNum = 0;
uint8_t pageDataLen = 0;
uint8_t programDone = 0;
uint8_t pageData[ SPM_PAGESIZE * 2];

////////////////////////////////////////////
/// Local Variables
////////////////////////////////////////////

static uint8_t msgPos = 0;
static uint8_t msgType = 0;
static uint8_t msgAddr = 0;
static uint8_t msgLen = 0;
static uint8_t msgReady = 0;

static uint16_t msgCRC;
static uint16_t lastPage;

////////////////////////////////////////////
/// Local Prototypes
////////////////////////////////////////////

void error();
void reset();
void parse();
void processMessage();
static inline uint8_t serialReceive();

////////////////////////////////////////////
/// Methods
////////////////////////////////////////////

void serialSetup(uint32_t baud) {
  // Endable RX
  UCSR0B = (1<<RXEN0);

  // Frame format (8-bit, 1 stop bit)
  UCSR0C = 1<<UCSZ01 | 1<<UCSZ00;

  // Set baud
  UBRR0 =  (unsigned char)UART_BAUD_SELECT(baud);
}

// Receive a byte of data
static inline uint8_t serialReceive() {
  while(!serialReceiveReady());
  return UDR0;
}

static inline uint8_t serialReceiveWithCRC() {
  uint8_t b = serialReceive();
  msgCRC = _crc16_update(msgCRC, b);
  return b;
}

// Inform master an error occured while reading the message
void error() {
  
}

// Reset the message
void reset() {
  msgPos = 0;
  msgCRC = ~0;
  pageDataLen = 0;
  msgReady = 0;
}

// Watch the serial line for the next message
void watchSerial(uint8_t lastPageLoaded) {
  lastPage = lastPageLoaded;
  reset();

  while (1) {
    parse();

    if (msgReady) {
      return;
    }
  }
}

void parse() {
  uint8_t b = serialReceive();

  // Start of message
  if (msgPos == MSG_SOM1 && b == SOM_BYTE) {
    msgPos++;
  }
  else if (msgPos == MSG_SOM2 && b == SOM_BYTE) {
    msgPos++;

    // Header
    msgType = serialReceiveWithCRC();
    msgAddr = serialReceiveWithCRC();
    msgLen = serialReceiveWithCRC();

    // Data
    while (pageDataLen < msgLen) {
      pageData[pageDataLen++] = serialReceiveWithCRC();
    }

    // CRC
    serialReceiveWithCRC();
    serialReceiveWithCRC();

    // Validate CRC
    if (msgCRC == 0) {
      processMessage();
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
  if (TYPE_PAGE == msgType) {
    // Not the page we were expecting
    if (msgAddr != lastPage + 1) {
      reset();
    }
    pageReady = 1;
  }
  else if (TYPE_END == msgType) {
    programDone = 1;
  }
}