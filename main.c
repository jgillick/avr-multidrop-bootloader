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

  // Disable watchdog timer (might still be running after reset)
  MCUSR &= ~(1<<WDRF);
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  WDTCSR = 0x00;

  // Run the bootloader
  if (shouldRunBootloader()){
    bootloader();
  }
  // Run the program
  else {

    // Default back to bootloader if there are errors in the program
#if BOOTLOAD_ON_EEPROM == 1
  eeprom_update_byte(EEPROM_RUN_APP, 0xFF);
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
  boot_page_erase(pageAddress);
  boot_spm_busy_wait();

  // Write to page buffer
  uint16_t word;
  for (uint8_t i = 0; i < SPM_PAGESIZE; i += 2) {
    word = pageData[i];
    word |= pageData[i + 1] << 8;
    boot_page_fill(pageAddress + i, word);
  }

  // Save to flash
  boot_page_write(pageAddress);
  boot_spm_busy_wait();

  pageAddress += SPM_PAGESIZE;
  numPagesWritten++;
}

// Start the program
static void finishedProgramming() {

  // Reenable RWW-section again.
  boot_rww_enable();

  signalDisable();

  // Reset
#if BOOTLOAD_ON_EEPROM == 1
  eeprom_update_byte(EEPROM_RUN_APP, 1);
#endif
  wdt_enable(WDTO_15MS);
  while(1);
}