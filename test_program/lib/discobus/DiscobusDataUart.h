
#ifndef DiscobusDataUart_H
#define DiscobusDataUart_H

#include "DiscobusData.h"
#include <avr/io.h>

class DiscobusDataUart : public DiscobusData {
public:
  DiscobusDataUart();

  // Hook into the UART at `baud` and start receiving data
  void begin(uint32_t baud);

  // How many bytes are available in the RX buffer
  uint8_t available();

  // Read a byte from the RX buffer
  uint8_t read();

  // Write something to the TX line
  void write(uint8_t);

  // Send everything in the TX buffer and return when the
  // final frame has been transmitted out
  void flush();

  // Clears the RX buffer
  void clear();

  // Not implemented
  void enable_write();
  void enable_read();
};

#endif