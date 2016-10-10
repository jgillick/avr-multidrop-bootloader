
#ifndef DiscobusData_H
#define DiscobusData_H

/************************************************************************************
 *  A virtual class that is inherited to define how data is received from the
 *  bus transceiver. Most transceiver will connect to your microcontroller via one
 *  of the UART ports. In that case, you can create a custom class to interface with
 *  with the appropriate port.
 *
 *  DiscobusUart has been subclassed from this to interface with the UART0 port of
 *  Atmega8 chips.
 *
 ************************************************************************************/

#include <avr/io.h>

class DiscobusData {
public:

  // Hook up to the data line
  virtual void begin(uint32_t baud);

  // How many bytes are available in the RX buffer
  virtual uint8_t available();

  // Read a byte from the RX buffer
  virtual uint8_t read();

  // Write a byte to the TX line
  virtual void write(uint8_t);

  // Send everything in the TX buffer with blocking
  virtual void flush();

  // Clears the RX buffer
  virtual void clear();

  // Enables writing from the data stream (only required for 485 and similar protocols)
  virtual void enable_write();

  // Enables reading from the data stream (only required for 485 and similar protocols)
  virtual void enable_read();
};

#endif