
#include "DiscobusSlave.h"
#include <util/crc16.h>
#include <util/delay.h>

#define MAX_ADDR_ERRORS 5

#define SOM 0xFF

DiscobusSlave::DiscobusSlave(DiscobusData *_serial) : Discobus(_serial) {
  flags = 0;
  myAddress = 0;
  responseHandler = 0;
  parseState = NO_MESSAGE;
}

void DiscobusSlave::resetNode() {
  lastAddr = 0xFF;
  address = 0;
  myAddress = 0;
  setNextDaisyValue(0);
}

uint8_t DiscobusSlave::hasNewMessage() {
  return parseState == MESSAGE_READY;
}

uint8_t DiscobusSlave::isAddressedToMe() {
  return hasNewMessage() && (address == myAddress || address == BROADCAST_ADDRESS);
}

uint8_t DiscobusSlave::inBatchMode() {
  return flags & BATCH_FLAG;
}

uint8_t DiscobusSlave::isResponseMessage() {
  return flags & RESPONSE_MESSAGE_FLAG;
}

uint8_t* DiscobusSlave::getData() {
  return dataBuffer;
}

uint8_t DiscobusSlave::getDataLen() {
  return dataIndex;
}

uint8_t DiscobusSlave::getCommand() {
  return command;
}

void DiscobusSlave::setAddress(uint8_t addr) {
  myAddress = addr;
}

uint8_t DiscobusSlave::getAddress() {
  return myAddress;
}

void DiscobusSlave::setResponseHandler(DiscobusResponseFunction handler) {
  responseHandler = handler;
}

void DiscobusSlave::startMessage() {
  flags = 0;
  length = 0;
  address = 0;
  lastAddr = 0xFF;
  dataIndex = 0;
  dataBuffer[0] = '\0';
  fullDataLength = 0;
  fullDataIndex = 0;
  dataStartOffset = 0;
  errCount = 0;
  messageCRC = ~0;
}

uint8_t DiscobusSlave::read() {
  checkDaisyChainPolarity();

  // Move onto the next message
  if (parseState == MESSAGE_READY) {
    parseState = NO_MESSAGE;
  }

  // No new data, but our prev daisy line became enabled
  if (command == CMD_ADDRESS && parsePos == ADDR_UNSET && isPrevDaisyEnabled() && !serial->available()){
    processAddressing(lastAddr);
  }

  // Handle incoming bytes
  while (serial->available()) {
    if(parse(serial->read()) == 1 && !isResponseMessage()) {

      if (command == CMD_RESET) {
        resetNode();
      }

      return 1;
    }
  }
  return 0;
}

/**
 * Parse the next byte off the bus.
 * Returns 1 if a full message has been received, 0 if not.
 */
uint8_t DiscobusSlave::parse(uint8_t b) {

  if (parseState == HEADER_SECTION) {
    parseHeader(b);
  }
  else if (parseState == DATA_SECTION) {
    if (command == CMD_ADDRESS) {
      processAddressing(b);
    } else {
      processData(b);
    }
  }
  // Validate CRC
  else if (parseState == END_SECTION) {
    uint8_t crcByte;

    if (parsePos == DATA_POS) { // First byte
      crcByte = (messageCRC >> 8) & 0xFF;
      parsePos = EOM1_POS;
    }
    else { // Second byte
      crcByte = messageCRC & 0xFF;
      parsePos = EOM2_POS;
    }

    // Validate each byte
    if (crcByte != b) {
      parseState = NO_MESSAGE; // no match, abort
    }
    else if (parsePos == EOM2_POS) {
      parseState = MESSAGE_READY;
      return 1;
    }
  }
  // Second start byte
  else if (parseState == START_SECTION) {
    if (b == SOM) {
      startMessage();
      parsePos = SOM2_POS;
      parseState = HEADER_SECTION;
    }
    // No second 0xFF, so invalid start to message
    else {
      parseState = NO_MESSAGE;
    }
  }
  // First start byte
  else if (parseState == NO_MESSAGE && b == SOM) {
    parsePos = SOM1_POS;
    parseState = START_SECTION;
  }

  return 0;
}

void DiscobusSlave::parseHeader(uint8_t b) {
  messageCRC = _crc16_update(messageCRC, b);

  // Header flags
  if (parsePos == SOM2_POS) {
    parsePos = HEADER_FLAGS_POS;
    flags = b;
  }
  // Address
  else if (parsePos == HEADER_FLAGS_POS) {
    parsePos = HEADER_ADDR_POS;
    address = b;
  }
  // Command
  else if (parsePos == HEADER_ADDR_POS) {
    parsePos = HEADER_CMD_POS;
    command = b;
  }
  // Length, 1st byte
  else if (parsePos == HEADER_CMD_POS) {
    parsePos = HEADER_LEN1_POS;

    // in batch mode, the first length byte is the number of nodes
    if (inBatchMode()) {
      numNodes = b;
    } else {
      length = b;
      fullDataLength = b;
      dataStartOffset = 0;
      parseState = DATA_SECTION;
    }
  }
  // Length, 2nd byte (if in batch mode)
  else if (parsePos == HEADER_LEN1_POS) {
    length = b;

    if (myAddress != 0) {
      fullDataLength = length * numNodes;
      dataStartOffset = (myAddress - 1) * length; // Where our data starts in the message
    }
    else {
      // We don't have an address, so cannot read message
      length = 0;
    }

    parsePos = HEADER_LEN2_POS;
    parseState = DATA_SECTION;
  }

  // Finishing header
  if (parseState == DATA_SECTION) {

    // On to addressing
    if (command == CMD_ADDRESS) {
      parsePos = ADDR_WAITING;
    }

    // No data, continue to CRC
    else if (length == 0) {
      parsePos = DATA_POS;
      parseState = END_SECTION;
    }

    // If in response message and we're the first node, move straight to sending a response
    else if (isResponseMessage() && myAddress == 1) {
      sendResponse();
    }
  }
}

void DiscobusSlave::processData(uint8_t b) {
  messageCRC = _crc16_update(messageCRC, b);
  parsePos = DATA_POS;
  
  // If we're in our data section, fill data buffer
  if (fullDataIndex >= dataStartOffset && dataIndex < length){
    dataBuffer[dataIndex++] = b;
    dataBuffer[dataIndex] = '\0';
  }

  fullDataIndex++;

  // It's our turn to respond with some data
  if (isResponseMessage() && fullDataIndex == dataStartOffset) {
    sendResponse();
    return;
  }

  // Done with data
  if (fullDataIndex >= fullDataLength) {
    parseState = END_SECTION;
  }
}


void DiscobusSlave::processAddressing(uint8_t b) {

  // We still waiting for an address
  if (myAddress == 0 && isPrevDaisyEnabled() && !serial->available()){

    // Address confirmation
    if (parsePos == ADDR_SENT) {
      if (b == lastAddr) {
        parsePos = ADDR_CONFIRMED;
        myAddress = b;
        setNextDaisyValue(1);
        
        // Max address is 0xFF
        if (b == 0xFF) {
          doneAddressing();
        }
        return;
      }
      // Not confirmed, try again
      else {
        errCount++;

        // After too many errors, give up and enable the next node
        if (errCount >= MAX_ADDR_ERRORS) {
          myAddress = 0;
          parsePos = ADDR_ERROR;
          setNextDaisyValue(1);
        }
        // Master ending message
        else if (b == 0xFF) {
          myAddress = 0;
          parsePos = ADDR_ERROR;
        }
        else {
          parsePos = ADDR_UNSET;
          lastAddr = b;
        }
        return;
      }
    }
    // This might be ours, send tentative new address and wait for confirmation
    else if(b >= lastAddr) {
      b++;
      parsePos = ADDR_SENT;
      _delay_us(200);
      serial->enable_write();
      serial->write(b);
      serial->enable_read();
      lastAddr = b;
      return;
    }
  }

  // Done when we see two 0xFF
  if (parsePos != ADDR_SENT && lastAddr == b && b == 0xFF) {
    doneAddressing();
  }

  lastAddr = b;

  if (parsePos == ADDR_WAITING) {
    parsePos = ADDR_UNSET;
  }
}

void DiscobusSlave::doneAddressing() {
  dataIndex = 0;
  dataBuffer[dataIndex++] = myAddress;
  dataBuffer[dataIndex] = '\0';
  parseState = MESSAGE_READY;
}

void DiscobusSlave::sendResponse() {
  if (responseHandler) {
    uint8_t i;
    responseHandler(command, dataBuffer, length);

    // Make sure we're not butting up against other data that was just received
    _delay_us(150);

    // Write response buffer to stream
    serial->enable_write();
    for (i = 0; i < length; i++) {
      serial->write(dataBuffer[i]);
      messageCRC = _crc16_update(messageCRC, dataBuffer[i]);
      fullDataIndex++;
    }
    serial->enable_read();
  }
}
