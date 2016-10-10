
#include "DiscobusData485.h"

DiscobusData485::DiscobusData485(volatile uint8_t de_pin_num,
                                   volatile uint8_t* de_ddr_register,
                                   volatile uint8_t* de_port_register):
                                   de_pin(de_pin_num),
                                   de_ddr(de_ddr_register),
                                   de_port(de_port_register) {

  *de_ddr |= (1 << de_pin_num);
  *de_port &= ~(1 << de_pin_num);
}

void DiscobusData485::enable_write() {
  *de_port |= (1 << de_pin);
}

void DiscobusData485::enable_read() {
  flush();
  *de_port &= ~(1 << de_pin);
}

void DiscobusData485::write(uint8_t b) {
  DiscobusDataUart::write(b);
}
