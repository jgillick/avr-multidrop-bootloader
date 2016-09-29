
/*****************************************************************************
*
* Parses messages coming in via the serial line and adds bytes to the page 
* buffer.
*
******************************************************************************/

#ifndef SERIAL_H
#define SERIAL_H

extern uint8_t pageNum;
extern uint8_t pageDataLen;
extern uint8_t pageReady;
extern uint8_t pageData[ SPM_PAGESIZE * 2];

extern uint8_t programDone;

// Setup the serial line
extern void serialSetup(uint32_t baud);

// Watch the serial line for new data and 
// return when a complete message has been received.
extern void watchSerial(uint8_t lastPageLoaded);

#endif