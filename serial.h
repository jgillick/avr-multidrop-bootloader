
/*****************************************************************************
*
* Parses messages coming in via the serial line and adds bytes to the page
* buffer.
*
******************************************************************************/

#ifndef SERIAL_H
#define SERIAL_H

#define STATUS_NONE 0
#define STATUS_PAGE_READY 1
#define STATUS_DONE 2

extern uint8_t pageData[SPM_PAGESIZE];

// Watch the serial line for new data and
// return when a complete message has been received.
extern uint8_t watchSerial(uint8_t lastPageLoaded);

#endif