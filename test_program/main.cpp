/*****************************************************************************
*
* A simple blink program that blinks an LED on PB2 and jumps to the
* bootloader when the serial bus tells it to.
*
****************************************************************************/

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "DiscobusSlave.h"
#include "DiscobusData485.h"


////////////////////////////////////////////
/// Settings
////////////////////////////////////////////

// Communication speed
#define SERIAL_BAUD 115200

// Firmware version
#define VERSION_MAJOR 1
#define VERSION_MINOR 0

// Command that sends the program to the bootloader
#define BOOTLOADER_CMD 0xF0

// EEPROM addresses
#define EEPROM_RUN_APP        (uint8_t*) 0x00
#define EEPROM_VERSION_MAJOR  (uint8_t*) 0x01
#define EEPROM_VERSION_MINOR  (uint8_t*) 0x02

////////////////////////////////////////////
/// Prototypes
////////////////////////////////////////////

void setOkay();
void rebootToBootloader();

////////////////////////////////////////////
/// Program
////////////////////////////////////////////

int main() {
  DDRB |= (1 << PB2);

  DiscobusData485 rs485(PD2, &DDRD, &PORTD);
  DiscobusSlave comm(&rs485);
  rs485.begin(SERIAL_BAUD);

  setOkay();

  uint8_t ledVal = 1;
  while(true) {

    // Reboot into bootloader when we receive the bootloader command
    comm.read();
    if (comm.hasNewMessage() && comm.isAddressedToMe() && comm.getCommand() == BOOTLOADER_CMD) {
      rebootToBootloader();
    }

    // Blink LED
    if (ledVal) {
      PORTB &= ~(1 << PB2);
      ledVal = 0;
    } else {
      PORTB |= (1 << PB2);
      ledVal = 1;
    }
    _delay_ms(500);
  }
}

// Set the EEPROM value to run the program on next start
void setOkay() {
  eeprom_update_byte(EEPROM_RUN_APP, 1);
  eeprom_update_byte(EEPROM_VERSION_MAJOR, VERSION_MAJOR);
  eeprom_update_byte(EEPROM_VERSION_MINOR, VERSION_MINOR);
}

// Change EEPROM value to triggerl bootloader then reboot
void rebootToBootloader() {
  eeprom_update_byte(EEPROM_RUN_APP, 0);
  wdt_enable(WDTO_15MS);
  while(1);
}
