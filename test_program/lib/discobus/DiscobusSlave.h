
#ifndef DiscobusSlave_H
#define DiscobusSlave_H

#include <avr/io.h>
#include <stdint.h>
#include "Discobus.h"

typedef void (*DiscobusResponseFunction)(uint8_t command, uint8_t *buff, uint8_t len);

#ifndef MD_MAX_DATA_LEN
#define MD_MAX_DATA_LEN 10
#endif

/**
  Discobus Slave class
*/
class DiscobusSlave: public Discobus {

public:
  DiscobusSlave(DiscobusData *_serial);

  // Reset the node's stat and unset it's address.
  void resetNode();

  // Set our address on the network
  // Without setting this, the only messages we'll receive are broadcasts
  void setAddress(uint8_t);

  // Get our address on the network
  uint8_t getAddress();

  // Reads the latest data on the serial line
  // returns 1 if a new message is ready
  uint8_t read();

  // There a new message ready to read
  uint8_t hasNewMessage();

  // Is this message addressed to our node
  // (directly or indirectly via a broadcast message)
  uint8_t isAddressedToMe();

  // The message command
  uint8_t getCommand();

  // Return the data of the message
  uint8_t* getData();

  // Return the length of the message data
  uint8_t getDataLen();

  // Does this message require a response.
  uint8_t isResponseMessage();

  // Is the current message in batch mode
  uint8_t inBatchMode();

  // Set to the function that will provide the proper
  // data for a response message. It is  best to keep
  // this function short and quick, because it will be
  // a blocking action.
  void setResponseHandler(DiscobusResponseFunction handler);

private:
  DiscobusResponseFunction responseHandler;

  enum msg_state_t {
    NO_MESSAGE,
    START_SECTION,
    HEADER_SECTION,
    DATA_SECTION,
    END_SECTION,
    MESSAGE_READY
  };

  enum ms_position_t {
    SOM1_POS,        // Start of message (first byte)
    SOM2_POS,        // Start of message (second byte)
    HEADER_FLAGS_POS,
    HEADER_ADDR_POS,
    HEADER_CMD_POS,
    HEADER_LEN1_POS,
    HEADER_LEN2_POS,
    DATA_POS,
    EOM1_POS,
    EOM2_POS,
    ADDR_WAITING,    // Addressing: command started, but no initial address seen
    ADDR_UNSET,      // Addressing: addressed not received for this node
    ADDR_SENT,       // Addressing: sent address to master
    ADDR_CONFIRMED,  // Addressing: master confirmed address
    ADDR_ERROR,      // Addressing: ended in error
  };

  enum msg_state_t  parseState;
  enum ms_position_t parsePos;

  uint8_t flags,
          address,
          command,
          length,
          numNodes,
          myAddress,
          dataIndex,
          lastAddr,
          errCount;

  // Batch mode values
  uint16_t fullDataLength,  // Length of the entire data section for all nodes
           fullDataIndex,   // The actual index of the entire data section
           dataStartOffset; // Where this node's data starts.

  uint8_t dataBuffer[MD_MAX_DATA_LEN + 1];

  // Start a new message by resetting all values
  void startMessage();

  // Continue parsing the current message from the latest received byte
  uint8_t parse(uint8_t b);

  // Parse the header section of the message
  void parseHeader(uint8_t);

  // Process the data section of the message
  void processData(uint8_t);

  // Process the addressing response part of the addressing message
  void processAddressing(uint8_t);

  // Finish the addressing message
  void doneAddressing();

  // Send a response to a message
  void sendResponse();
};

#endif
