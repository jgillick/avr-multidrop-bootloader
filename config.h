/*****************************************************************************
*
* Configuration settings for the bootloader
*
****************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG

////////////////////////////////////////////
/// Configure how bootloader is activated
////////////////////////////////////////////

// Enter programming mode when the EEPROM value
// for address EEPROM_RUN_APP is 0
#define BOOTLOAD_ON_EEPROM 1
#define EEPROM_RUN_APP (uint8_t*) 0x00

// Enter programming modewhen a pin matches
// the on/off value defined by BOOTLOAD_PIN_VAL (1 = HIGH)
#define BOOTLOAD_ON_PIN   0
#define BOOTLOAD_PIN_NUM  PD2
#define BOOTLOAD_PIN_DDR  DDRD
#define BOOTLOAD_PIN_REG  PIND
#define BOOTLOAD_PIN_VAL  1


////////////////////////////////////////////
/// Versioning
////////////////////////////////////////////

// The version stored in EEPROM will be compared to the
// version sent to the bootloader. If they're the same, the
// device will skip programming.

// Comment this out to disable
#define USE_VERSIOING 0

// The EEPROM address of where the major and minimum version numbers
// are stored. These need to be stored by your program, the bootloader
// does not write to these locations.
#define VERSION_MAJOR (uint8_t*) 0x01
#define VERSION_MINOR (uint8_t*) 0x02


////////////////////////////////////////////
/// Signal Line
////////////////////////////////////////////

#define SIGNAL_BIT PD7
#define SIGNAL_DDR_REG DDRD
#define SIGNAL_PIN_REG PIND


////////////////////////////////////////////
/// RS485
////////////////////////////////////////////

// Using an RS485 transciever
#define USE_RS485 1

// Define the pin attached to the transciever's DE & RE pins
#define RS485_DE_BIT      PD2
#define RS485_DE_DDR_REG  DDRD
#define RS485_DE_PORT_REG PORTD


////////////////////////////////////////////
/// Communications
////////////////////////////////////////////

#define SERIAL_BAUD 115200

// Setup the communication channel (by default using the UART and a RS485 transciever)
inline void commSetup() {
  PORTD |= (1 << PD0); // Enable pull-up on RX pin

  UCSR0B = (1<<RXEN0); // Enable RX
  UCSR0C = 1<<UCSZ01 | 1<<UCSZ00; // Frame format (8-bit, 1 stop bit)

  // Set baud
  UBRR0 =  (unsigned char) (((F_CPU) + 8UL * (SERIAL_BAUD)) / (16UL * (SERIAL_BAUD)) - 1UL);

  // RS485
  // Pull DE/RE pin LOW for receiving
#if USE_RS485 == 1
  RS485_DE_DDR_REG |= (1 << RS485_DE_BIT);
  RS485_DE_PORT_REG &= ~(1 << RS485_DE_BIT);
#endif

#ifdef DEBUG
  UCSR0B |= (1<<TXEN0); // Enable TX
#endif
}

// Receive the next byte of data
inline uint8_t commReceive() {
  while (!(UCSR0A & (1<<RXC0))); // wait for data
  return UDR0;
}

#ifdef DEBUG
inline void commWrite(uint8_t byte) {
	while (!(UCSR0A & (1<<UDRE0)));	// Wait for empty transmit buffer
	UDR0 = byte;	// send byte
}
#endif


////////////////////////////////////////////
/// Bus Message Commands
////////////////////////////////////////////

// Start programming
#define MSG_CMD_PROG_START 0xF1

// Receive the next page number
#define MSG_CMD_PAGE_NUM   0xF2

// Receive the next page of data
#define MSG_CMD_PAGE_DATA  0xF3

// Finish programming and start the main program
#define MSG_CMD_PROG_END   0xF4

#endif
