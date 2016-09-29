/*****************************************************************************
*
* MultiDrop bootloader
*
****************************************************************************/

#include <avr/boot.h>

#include "serial.h"

#define SERIAL_BAUD 115200

int main(void) {
  serialSetup(SERIAL_BAUD);

  while (1) {
    watchSerial(0);
  }
}