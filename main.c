/*****************************************************************************
*
* MultiDrop bootloader
*
****************************************************************************/

#include <avr/eeprom.h>
#include <avr/boot.h>
#include <avr/wdt.h>

#include "config.h"
#include "macros.h"
#include "serial.h"

static uint8_t numPagesWritten = 0;
static uint16_t pageAddress = 0;

void bootloader();
void writeNextPage();
void rebootRunProgram();

int main(void) {

  // Run the program
  if (eeprom_read_byte(EEPROM_RUN_APP) == 1) {
    asm("jmp 0000");
  }
  // Run the bootloader
  else {
    bootloader();
  }
  
}

// The main bootloader program
void bootloader() {

  // Setup
  signalEnable(); 
  serialSetup();

  // Parse serial  
  while (1) {
    watchSerial(numPagesWritten);

    if (pageReady) {
      writeNextPage();
    }
    else if(programDone) {
      rebootRunProgram();
    }
  }
}

// Write the next page of the program to flash
void writeNextPage() {

  // Wait until we're ready
  eeprom_busy_wait();
  boot_page_erase(pageAddress);
  boot_spm_busy_wait();

  // Write to page buffer
  for (uint8_t i = 0; i < SPM_PAGESIZE; i += 2) {
    uint16_t word = pageData[i];
    word += pageData[i + 1] << 8;
    boot_page_fill(pageAddress + 1, word);
  }

  // Store buffer in flash page.
  boot_page_write(pageAddress);
  boot_spm_busy_wait();

  pageAddress += SPM_PAGESIZE;
  numPagesWritten++;
}

// Start the program
void rebootRunProgram() {
  eeprom_update_byte(EEPROM_RUN_APP, 1);

  // Reboot
  wdt_enable(WDTO_15MS);
  while(1);
}