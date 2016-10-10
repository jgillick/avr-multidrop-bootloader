/************************************************************************************
 *  The base class that master and slave classes extend from.
 ************************************************************************************/

#ifndef Discobus_H
#define Discobus_H

// Maximum size of the message buffer
#define MSG_BUFFER_LEN  150

#include <avr/io.h>
#include <stdint.h>
#include "DiscobusData.h"

#define CMD_RESET   0xFA
#define CMD_ADDRESS 0xFB
#define CMD_NULL    0xFF

class Discobus {

public:
  static const uint8_t BROADCAST_ADDRESS = 0;
  static const uint8_t BATCH_FLAG = 0b00000001;
  static const uint8_t RESPONSE_MESSAGE_FLAG = 0b00000010;

  Discobus(DiscobusData*);

  // Add the pin and registers for the daisy chain lines.
  // To automatically define the polarity as d1=prev and d2=next, pass `set_polarity` as `true`.
  // Otherwise, polarity will be determined at runtime by setting the first to
  // be enabled as the previous daisy line.
  // (you can also set polarity with the setDaisyChainPolarity() method)
  void addDaisyChain(volatile uint8_t d1_pin_num,
                     volatile uint8_t* d1_ddr_register,
                     volatile uint8_t* d1_port_register,
                     volatile uint8_t* d1_pin_register,

                     volatile uint8_t d2_pin_num,
                     volatile uint8_t* d2_ddr_register,
                     volatile uint8_t* d2_port_register,
                     volatile uint8_t* d2_pin_register,

                     uint8_t set_polarity=false);

  // Get the daisy chain number (1 or 2) for the previous node
  // this will return 0 if the polarity has not been determined yet
  uint8_t getDaisyChainPrev();

  // Get the daisy chain number (1 or 2) for the next node
  // this will return 0 if the polarity has not been determined yet
  uint8_t getDaisyChainNext();

  // Set the daisy chain polarity
  // next and prev should be set to 1 or 2, which matches
  // the pins for d1_* and d2_* defined in addDaisyChain()
  void setDaisyChainPolarity(uint8_t prev, uint8_t next);


protected:

  enum Masks {
    batch_mode = 0x80,
    response_msg = 0x40
  };

  DiscobusData *serial;

  uint16_t messageCRC;

  // Daisy chain pin registers
  volatile uint8_t d1_num,
                   d2_num;
  volatile uint8_t* d1_ddr;
  volatile uint8_t* d1_port;
  volatile uint8_t* d1_pin;
  volatile uint8_t* d2_ddr;
  volatile uint8_t* d2_port;
  volatile uint8_t* d2_pin;

  uint8_t daisy_prev,
          daisy_next;

  // Try to determine the next/prev daisy chain pins
  void checkDaisyChainPolarity();

  // Set the next daisy chain pin to active or inactive (1 or 0)
  uint8_t setNextDaisyValue(uint8_t val);

  // Get the value (1 or 0) from the prev daisy chain pin
  uint8_t isPrevDaisyEnabled();

};

#endif
