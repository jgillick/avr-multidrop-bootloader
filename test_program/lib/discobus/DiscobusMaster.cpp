
#include <util/crc16.h>

#include "DiscobusMaster.h"

#define BATCH_FLAG            0b00000001
#define RESPONSE_MESSAGE_FLAG 0b00000010

DiscobusMaster::DiscobusMaster(DiscobusData *serial) : Discobus(serial) {
  state = EOM;
  nodeNum = 0;
}

void DiscobusMaster::setNodeLength(uint8_t num) {
  nodeNum = num;
}

void DiscobusMaster::addNextDaisyChain(volatile uint8_t next_pin_num,
                                        volatile uint8_t* next_ddr_register,
                                        volatile uint8_t* next_port_register,
                                        volatile uint8_t* next_pin_register) {

  d1_num  = next_pin_num;
  d1_ddr  = next_ddr_register;
  d1_port = next_port_register;
  d1_pin  = next_pin_register;

  *d1_ddr |= (1 << d1_num);
  *d1_port |= (1 << d1_num);

  daisy_prev = 0;
  daisy_next = 1;
}

uint8_t DiscobusMaster::startMessage(uint8_t command,
                                      uint8_t destinationAddr,
                                      uint8_t dataLen,
                                      uint8_t batchMode,
                                      uint8_t responseMessage) {

  state = 0;
  messageCRC = ~0;
  dataLength = dataLen;
  destAddress = destinationAddr;

  uint8_t flags = 0;
  if (batchMode) {
    flags |= BATCH_FLAG;
  }
  if (responseMessage) {
    flags |= RESPONSE_MESSAGE_FLAG;

    // Don't timeout on first check
    dontTimeout = true;
  }

  // Start sending header
  serial->enable_write();
  sendByte(0xFF);
  sendByte(0xFF);
  sendByte(flags);
  sendByte(destAddress);
  sendByte(command);

  // Length
  if (batchMode) {
    sendByte(nodeNum);
  }
  sendByte(dataLength);
  serial->enable_read();

  state = HEADER_SENT;
  return 1;
}

void DiscobusMaster::resetAllNodes() {
  startMessage(CMD_RESET, BROADCAST_ADDRESS);
  finishMessage();
}

void DiscobusMaster::startAddressing(uint32_t time, uint32_t timeout) {
  nodeNum = 0;
  lastAddressReceived = 0;
  nodeAddressTries = 0;
  addrTimeoutDuration = timeout;
  timeoutTime = time + addrTimeoutDuration;

  // Start address message
  startMessage(CMD_ADDRESS, BROADCAST_ADDRESS, 2, true, true);
  state = ADDRESSING;

  // First address
  setNextDaisyValue(1);
  sendByte(0x00, true);

  // Don't timeout on first check
  dontTimeout = true;
}

void DiscobusMaster::setResponseSettings(uint8_t *buff, uint32_t time, uint32_t timeout, uint8_t *defaultResponse) {
  responseIndex = 0;
  responseBuff = buff;
  timeoutDuration = timeout;
  defaultResponseValues = defaultResponse;
  timeoutTime = time + timeoutDuration;

  if (destAddress == BROADCAST_ADDRESS) {
    waitingOnNodes = nodeNum;
  } else {
    waitingOnNodes = 1;
  }
}

uint8_t DiscobusMaster::checkForResponses(uint32_t time) {
  uint8_t b, i;

  if (dontTimeout) {
    timeoutTime = time + timeoutDuration;
  }
  dontTimeout = false;

  // Received all responses
  if (waitingOnNodes == 0) {
    finishMessage();
    return true;
  }

  // Get more responses
  while (serial->available()) {
    b = serial->read();
    responseBuff[responseIndex] = b;
    messageCRC = _crc16_update(messageCRC, b);

    responseIndex++;
    dontTimeout = true;

    // Have we received all the data for this node?
    if (responseIndex % dataLength == 0) {
      waitingOnNodes--;
    }
  }

  // Node timeout, send default response
  if (time > timeoutTime) {

    // It's possible the node sent a partial response, so send whatever is left
    for (i = responseIndex % dataLength; i < dataLength; i++) {
      sendByte(defaultResponseValues[i], true);
      responseBuff[responseIndex] = defaultResponseValues[i];
      responseIndex++;
    }

    dontTimeout = true;
    waitingOnNodes--;
  }

  if (waitingOnNodes == 0) {
    finishMessage();
    return true;
  }
  return false;
}

DiscobusMaster::adr_state_t DiscobusMaster::checkForAddresses(uint32_t time) {
  uint8_t b;

  if (dontTimeout) {
    timeoutTime = time + addrTimeoutDuration;
  }
  dontTimeout = false;

  if (state != ADDRESSING) return ADR_DONE;

  // If master's prev daisy chain is HIGH, then the network has gone full circle
  if (isPrevDaisyEnabled() == 1) {
    finishMessage();
    return ADR_DONE;
  }

  // Receive next address
  if (serial->available()) {
    dontTimeout = true; // skip timing out next call
    while (serial->available()) {
      b = serial->read();
    }

    // Verify it's 1 larger than the last address and send confirmation
    if (b == lastAddressReceived + 1) {
      nodeNum++;
      lastAddressReceived = b;
      nodeAddressTries = 0;
      sendByte(b, true);
    }
    // Invalid address
    else {

      // Max tries, end in error
      nodeAddressTries++;
      if (nodeAddressTries > MD_MASTER_ADDR_MAX_TRIES) {
        return ADR_ERROR;
      }
      // Send last valid address again
      else {
        serial->enable_write();
        sendByte(0x00);
        sendByte(lastAddressReceived);
        serial->enable_read();
      }
    }
    return ADR_WAITING;
  }

  // Node timeout, finish
  if (time > timeoutTime) {
    finishMessage();
    if (nodeNum > 0) {
      return ADR_DONE;
    } else {
      return ADR_ERROR;
    }
  }

  // Max nodes
  if (lastAddressReceived == 255) {
    finishMessage();
    return ADR_DONE;
  }
  return ADR_WAITING;
}

uint8_t DiscobusMaster::sendData(uint8_t d) {
  if (state == EOM) return 0;

  sendByte(d, true);
  state = DATA_SENDING;
  return 1;
}

uint8_t DiscobusMaster::sendData(uint8_t *data, uint16_t len) {
  if (state == EOM) return 0;

  serial->enable_write();
  for(uint16_t i = 0; i < len; i++) {
    sendByte(data[i]);
  }
  serial->enable_read();
  state = DATA_SENDING;
  return 1;
}

void DiscobusMaster::sendByte(uint8_t b, uint8_t directionCntrl, uint8_t updateCRC) {
  if (directionCntrl) serial->enable_write();
  serial->write(b);
  if (directionCntrl) serial->enable_read();

  if (updateCRC) {
    messageCRC = _crc16_update(messageCRC, b);
  }
}

uint8_t DiscobusMaster::finishMessage() {
  if (state == EOM) return 0;

  serial->enable_write();

  // Send NULL message to end the addressing stage
  if (state == ADDRESSING) {

    // If the last address is 255, we've already sent 0xFF twice
    if (lastAddressReceived < 0xFF) {
      sendByte(0xFF);
      sendByte(0xFF);
    }

    // Send null message, just in case
    messageCRC = ~0;
    sendByte(0x00);     // flags
    sendByte(0x00);     // broadcast address
    sendByte(CMD_NULL); // command
    sendByte(0);        // length
  }

  // End CRC
  sendByte((messageCRC >> 8) & 0xFF, false, false);
  sendByte(messageCRC & 0xff, false, false);

  serial->enable_read();

  state = EOM;
  return 1;
}

