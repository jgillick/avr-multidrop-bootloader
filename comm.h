
/*****************************************************************************
*
* Parses messages coming in via the serial line and adds bytes to the page
* buffer.
*
******************************************************************************/

#ifndef COMM_H
#define COMM_H

#define STATUS_NONE 0
#define STATUS_PAGE_READY 1
#define STATUS_DONE 2

// The data for a single page of the program
extern uint8_t pageData[SPM_PAGESIZE];

// Watch the serial line for new data and
// return when a complete message has been received.
extern uint8_t watchSerial(uint8_t lastPageLoaded);

#endif