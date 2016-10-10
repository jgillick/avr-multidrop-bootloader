#include "Discobus.h"
#include "DiscobusData.h"


Discobus::Discobus(DiscobusData *_serial) : serial(_serial) {
}

void Discobus::addDaisyChain(volatile uint8_t d1_pin_number,
                              volatile uint8_t* d1_ddr_register,
                              volatile uint8_t* d1_port_register,
                              volatile uint8_t* d1_pin_register,

                              volatile uint8_t d2_pin_number,
                              volatile uint8_t* d2_ddr_register,
                              volatile uint8_t* d2_port_register,
                              volatile uint8_t* d2_pin_register,

                              uint8_t set_polarity) {

  d1_num  = d1_pin_number;
  d1_ddr  = d1_ddr_register;
  d1_port = d1_port_register;
  d1_pin  = d1_pin_register;

  d2_num  = d2_pin_number;
  d2_ddr  = d2_ddr_register;
  d2_port = d2_port_register;
  d2_pin  = d2_pin_register;

  // Init polarity checks
  daisy_prev = 0;
  daisy_next = 0;

  // Init registers & internal pull-ups
  *d1_ddr &= ~(1 << d1_num);
  *d1_port |= (1 << d1_num);

  *d2_ddr &= ~(1 << d2_num);
  *d2_port |= (1 << d2_num);

  if (set_polarity) {
    setDaisyChainPolarity(1, 2);
  }
}

void Discobus::checkDaisyChainPolarity() {
  if (daisy_prev && daisy_next) return;

  uint8_t d1 = !(*d1_pin & (1 << d1_num)),
          d2 = !(*d2_pin & (1 << d2_num));

  // No clear polarity
  if (d1 == d2) {
    return;
  }
  else if (d1) {
    setDaisyChainPolarity(1, 2);
  }
  else if (d2) {
    setDaisyChainPolarity(2, 1);
  }
}

uint8_t Discobus::getDaisyChainPrev() {
  return daisy_prev;
}

uint8_t Discobus::getDaisyChainNext() {
  return daisy_next;
}

void Discobus::setDaisyChainPolarity(uint8_t prev, uint8_t next) {
  // Invalid values
  if (prev == 0 || prev > 2 || next == 0 || next > 2) return;

  daisy_prev = prev;
  daisy_next = next;

  // Change pin to output
  if (daisy_next == 1) {
    *d1_ddr |= (1 << d1_num);
  } else {
    *d2_ddr |= (1 << d2_num);
  }
}

uint8_t Discobus::setNextDaisyValue(uint8_t val) {
  if (!daisy_next) return 0;

  uint8_t mask = (daisy_next == 2) ? (1 << d2_num) : (1 << d1_num);
  volatile uint8_t* ddr = (daisy_next == 2) ? d2_ddr : d1_ddr;
  volatile uint8_t* port = (daisy_next == 2) ? d2_port : d1_port;

  // Set as output
  *ddr |= mask;

  // Active low
  if (val) {
    *port &= ~mask;
  } else {
    *port |= mask;
  }
  return 1;
}

uint8_t Discobus::isPrevDaisyEnabled() {
  if (!daisy_prev) return 0;

  uint8_t mask = (daisy_prev == 2) ? (1 << d2_num) : (1 << d1_num);
  volatile uint8_t* pin = (daisy_prev == 2) ? d2_pin : d1_pin;

  return !(*pin & mask);
}

