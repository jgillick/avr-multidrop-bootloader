/*****************************************************************************
*
* MultiDrop bootloader
*
****************************************************************************/

#include <avr/eeprom.h>
// #include <avr/boot.h>
#include <avr/wdt.h>

#include "config.h"
#include "common.h"
#include "serial.h"

static uint8_t numPagesWritten = 0;
static uint16_t pageAddress = 0;

void bootloader();
void writeNextPage();
void rebootRunProgram();

int main(void) {

#ifdef DEBUG

  bootloader();

#else

  // Run the program
  if (eeprom_read_byte(EEPROM_RUN_APP) == 1) {
    // asm("jmp 0000");
  }
  // Run the bootloader
  else {
    bootloader();
  }

#endif

}

// The main bootloader program
void bootloader() {

  // Setup
  signalEnable();
  serialSetup();

#ifdef DEBUG
  DDRB |= (1 << PB2); // debug LED
  PORTB |= (1 << PB2);
  serialWrite(0x01);
#endif

  // Parse serial
  while (1) {
    uint8_t status = watchSerial(numPagesWritten);
    if (status == STATUS_PAGE_READY) {
      writeNextPage();
    }
    else if(status == STATUS_DONE) {
      rebootRunProgram();
    }
  }

#ifdef DEBUG
  serialWrite(0x20);
#endif
}

// Write the next page of the program to flash
void writeNextPage() {

#ifdef DEBUG
  serialWrite(0x02);
  serialWrite(numPagesWritten);
#endif

  // Wait until we're ready
#ifndef DEBUG
  // eeprom_busy_wait();
  // boot_page_erase(pageAddress);
  // boot_spm_busy_wait();
#endif

  // Write to page buffer
  for (uint8_t i = 0; i < SPM_PAGESIZE; i += 2) {
    uint16_t word = pageData[i];
    word += pageData[i + 1] << 8;
#ifndef DEBUG
    // boot_page_fill(pageAddress + 1, word);
#endif
  }

  // Store buffer in flash page.
#ifndef DEBUG
  // boot_page_write(pageAddress);
  // boot_spm_busy_wait();
#endif

  pageAddress += SPM_PAGESIZE;
  numPagesWritten++;
}

// Start the program
void rebootRunProgram() {
#ifdef DEBUG
  serialWrite(0x86);
#else
  eeprom_update_byte(EEPROM_RUN_APP, 1);
#endif

  // Reboot
  wdt_enable(WDTO_15MS);
  while(1);
}