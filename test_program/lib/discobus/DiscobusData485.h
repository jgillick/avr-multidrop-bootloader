
#ifndef DiscobusData485_H
#define DiscobusData485_H

#include "DiscobusDataUart.h"
#include <avr/io.h>

class DiscobusData485 : public DiscobusDataUart {
public:
  DiscobusData485(volatile uint8_t de_pin_num,
                   volatile uint8_t* de_ddr_register,
                   volatile uint8_t* de_port_register);

  void write(uint8_t byte);
  void enable_write();
  void enable_read();

private:
  volatile uint8_t de_pin;
  volatile uint8_t* de_ddr;
  volatile uint8_t* de_port;
};

#endif