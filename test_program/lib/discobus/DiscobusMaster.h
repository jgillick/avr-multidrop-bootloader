

#ifndef DiscobusMaster_H
#define DiscobusMaster_H

#include <avr/io.h>
#include <stdint.h>
#include "Discobus.h"

// How many times master will try to get a node's address, before deciding it is done
#ifndef MD_MASTER_ADDR_MAX_TRIES
#define MD_MASTER_ADDR_MAX_TRIES 4
#endif

class DiscobusMaster: public Discobus {

public:
  enum adr_state_t {
    ADR_WAITING,
    ADR_DONE,
    ADR_ERROR
  };
  uint8_t nodeNum;

  DiscobusMaster(DiscobusData *_serial);

  // Set the number of nodes on the bus
  void setNodeLength(uint8_t);

  // Add the pin and registers for the next daisy chain line
  // This is for master nodes that only have an out line and the
  // bus does not come back around to master
  void addNextDaisyChain(volatile uint8_t next_pin_num,
                         volatile uint8_t* next_ddr_register,
                         volatile uint8_t* next_port_register,
                         volatile uint8_t* next_pin_register);

  // Start a new message to send
  uint8_t startMessage(uint8_t command,
                      uint8_t destination=BROADCAST_ADDRESS,
                      uint8_t dataLength=0,
                      uint8_t batchMode=false,
                      uint8_t responseMessage=false);

  // Send a reset message to all nodes, which tells them to forget their address and
  // drop their daisy lines to low.
  void resetAllNodes();

  // Start the addressing processes
  // You'll need to call `checkForAddresses` regularly to handle the addressing process
  //   * time: The current system time (used for timeout).
  //   * timeout: (optional) How long master will wait for each node to respond (based on time units).
  //              You will need to call checkForAddresses frequently to check for responses and timeout nodes
  void startAddressing(uint32_t time, uint32_t timeout=10);

  // Check for new addresses received
  adr_state_t checkForAddresses(uint32_t time);

  // When sending a response request message, we need three more values:
  //   * buff: The buffer to store the responses for all nodes. This needs to be initialized
  //        large enough for everything (number of nodes * size of response for each).
  //   * time: The current system time (used for timeout).
  //   * timeout: How long master will wait for any node to respond. You will need to call
  //        checkForResponses frequently to update the current timestamp
  //   * defaultResponse: If a node doesn't respond by the timeout, this is the response that
  //        is registered for that node. This must be an array, the same length as the expected
  //        node response.
  void setResponseSettings(uint8_t buff[], uint32_t time, uint32_t timeout, uint8_t defaultResponse[]);

  // Call regularly to check for node responses.
  //  - time: Used to keep accurate time and timeout nodes who take too long to respond
  // Return: true when all nodes have responded
  uint8_t checkForResponses(uint32_t time);

  // Send a single byte of data
  uint8_t sendData(uint8_t data);

  // Send several bytes of data
  uint8_t sendData(uint8_t *data, uint16_t len);

  // Send the message
  uint8_t finishMessage();

private:
  enum State {
    EOM,
    HEADER_SENT,
    ADDRESSING,
    DATA_SENDING
  };

  uint32_t timeoutTime,
           timeoutDuration,
           addrTimeoutDuration;
  uint16_t responseIndex;

  uint8_t  destAddress,
           dataLength,
           state,
           dontTimeout,
           waitingOnNodes,
           nodeAddressTries,
           lastAddressReceived;

  uint8_t *responseBuff,
          *defaultResponseValues;

  // Send a byte and, optionally, update the messageCRC value
  void sendByte(uint8_t b, uint8_t directionCntrl=0, uint8_t updateCRC=1);
};

#endif
