/*****************************************************************************
*
* MultiDrop bootloader
*
****************************************************************************/

#include <avr/eeprom.h>
#include <avr/boot.h>
#include <avr/wdt.h>

#include "config.h"
#include "shared.h"
#include "comm.h"

uint8_t numPagesWritten = 0;
uint16_t pageAddress = 0;

static uint8_t shouldRunBootloader();
static void bootloader();
static void writeNextPage();
static void finishedProgramming();

int main(void) {
#ifdef DEBUG
  DDRB |= (1 << PB2);
#endif

  // Run the bootloader
  if (shouldRunBootloader()){
    bootloader();
  }
  // Run the program
  else {
#ifdef DEBUG
  PORTB |= (1 << PB2);
#endif
    asm("jmp 0000");
  }

}

/**
* Checks if the bootloader should run
* based on the values set in config.h
*
* Returns 1 if the bootloader should run,
* otherwise 0.
*/
static uint8_t shouldRunBootloader() {

#if BOOTLOAD_ON_EEPROM == 1
  if (eeprom_read_byte(EEPROM_RUN_APP) != 1) {
    return 1;
  }
#endif

#if BOOTLOAD_ON_PIN == 1
  BOOTLOAD_PIN_DDR &= ~(1 << BOOTLOAD_PIN_NUM);
  if (BOOTLOAD_PIN_REG & (1<<BOOTLOAD_PIN_NUM)) {
    return 1;
  }
#endif

  return 0;
}

// The main bootloader program
static void bootloader() {

  // Setup
  signalEnable();
  commSetup();

  // Parse serial
  while (1) {
    uint8_t status = watchSerial(numPagesWritten);
    if (status == STATUS_PAGE_READY) {
      writeNextPage();
    }
    else if(status == STATUS_DONE) {
      finishedProgramming();
      return;
    }
  }
}

// Write the next page of the program to flash
static void writeNextPage() {

  // Erase page
  boot_page_erase_safe(pageAddress);

  // Write to page buffer
  uint16_t word;
  for (uint8_t i = 0; i < SPM_PAGESIZE; i += 2) {
    word = pageData[i];
    word |= pageData[i + 1] << 8;
    boot_page_fill_safe(pageAddress + i, word);
  }

  // Save to flash
  boot_page_write_safe(pageAddress);

  // Reenable RWW-section again.
  boot_rww_enable_safe();

  pageAddress += SPM_PAGESIZE;
  numPagesWritten++;
}

// Start the program
static void finishedProgramming() {

#if BOOTLOAD_ON_EEPROM == 1
  eeprom_write_byte(EEPROM_RUN_APP, 1);
#endif

  signalDisable();

  // Jump to program
  asm("jmp 0000");
}