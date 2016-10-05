# MultiDrop Bootloader for AVR

A bootloader for AVR devices on a multidrop bus, like RS485, where 
all devices can be programmed at once.


## How it works

Somehow the master node will inform the slaves that it is time to be programed.
(most likely by sending a special message on the bus -- this is implementation specific)
The slave nodes will then set the EEPROM value `RUN_APP` to `0` and reset themselves.

Upon reset, the bootloader checks the `RUN_APP` value, and if it is 0, will go
into programming mode and enable the signal line.

Master sends a `START` message, to inform the slave nodes that program data is 
about to be sent. When the message has been received, the nodes disable the signal line. 

Master will now send a message stating the next page number to be sent, followed
by a message with that page's data.

Every message ends with a 16 bit CRC to verify the data was received without 
corruption. If the CRC does not match, the slave node will enable the signal
line to inform master of the error.

Master will continue to send all the pages down to the nodes. 

When complete, if the signal line is enabled, it resends the data; starting
at the page where signal line becoming enabled.

When all pages have been sent successfully (or max tries has been reached), master
will send a message telling the nodes to run their programs. Now the bootloaders
will set the EEPROM value `RUN_APP` to `1` and reset themselves.

### The Signal Line 

Since we can assume there will be multiple devices listening on the same
communication lines, we cannot have effective 2-way communication between
master and the nodes to be programmed (slave nodes). Instead, the communication
bus needs an additional shared line, called signal, that any slave node can
enable (enable low) to inform the master device of errors.

![Bus Layout](./diagrams/bus.png)

## Messages

Messages follow the [DiscoBus protocol format](https://github.com/jgillick/Disco-Bus-Protocol/blob/master/docs/messages.md).

### Start Message

Informs the bootloader that the first page of data is about to be sent.

**Message Type Code:** `0xF1`

### Page Number Message

Sends the number of the page about to be sent.

**Message Type Code:** `0xF2`

### Page Message

Sends the entire page to the bootloader.

**Message Type Code:** `0xF3`


### End Message

Informs the bootloader that all the data has been sent an it can reset.

**Message Type Code:** `0xF4`
