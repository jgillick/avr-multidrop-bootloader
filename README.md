# MultiDrop Bootloader for AVR

A bootloader for AVR devices on a multidrop bus, like RS485, where
all devices can be programmed at once.

 * [How it works](#how-it-works)
 * [Programmers](#programmers)
 * [Setup](#setup)
 * [The Signal Line](#the-signal-line)
 * [How to get into program mode](#how-to-get-into-program-mode)
   * [Pin value on reset](#pin-value-on-reset)
   * [With EEPROM](#with-eeprom)
 * [Communication](#communication)
   * [Communication Protocol](#communication-protocol)
 * [Version Checking](#version-checking)


## How it works

1. When the microcontroller starts up, it will first check if the bootloader should run.
(see the  section "[How to get into the bootloader](#how-to-get-into-the-bootloader)").

2. The nodes will enable the signal line, to inform the programmer they are connected.

3. The programmer sends a `START` message to the nodes via the communication bus
(via UART RX or similar) to inform the nodes the version of the program to be sent.
All nodes will check this version number against what they're programed with (via EEPROM values)
and will ignore the rest of the program if the version is the same.
(see the ["Versioning"](#versioning) section)

4. Nodes disable the signal line to inform the programmer that they are ready to receive.

5. The programmer sense a message with the page number that is about to be sent.
(starting from 0) followed by a message containing the page's data.
(this step is continued for all pages of data)

6. Any node that detects corrupt data, or a page that is out of order, will enable
the signal line.

7. When all pages have been sent, if the signal line is enabled, the programmer will
resends the pages; starting at the page where signal line became enabled.

8. When pages have been sent without errors, the programmer will send the `END` message,
informing all nodes to exit the booloader and start their programs.

## Programmers

Here's the list of programmers that support this bootloader:

 * [Node Multibootloader](https://github.com/jgillick/node-multibootloader) - Node library with command line programming interface.

## Setup

Setup instructions are [here](/SETUP.md/).

## The Signal Line

Since we can assume there will be multiple devices listening on the same communication line,
we cannot have effective 2-way communication between master and the nodes to be programmed.
Instead, the bus needs an additional shared line, called signal, that any nodes can
enable (enable low) to inform the programmer of errors.

![Bus Layout](./diagrams/bus.png)

By default this is set to pin `PD7`, but it can be configured in the "Signal Line" section
of `config.h`.

## How to get into program mode

By default, when the device resets, the bootloader will run first and then jump to the main program.
So how do we get the bootloader to enter into programming mode? There are two configurable ways that
are supported.

In both methods the main program will need to reset the device, so it can enter the bootloader.
This can be accomplished using a watchdog timer reset.

### Pin value on reset

This method detects a pin state when the bootloader begins (i.e. a button press).
If the pin state matches what we expect, the bootloader will enter programming mode.

For example, the the programmer can raise pin `PD2` to `HIGH`, then then main program should reset the device so the bootloader can see this and enter programming mode.

Set this option with following settings in `config.h`:

```c
#define BOOTLOAD_ON_PIN   1

#define BOOTLOAD_PIN_NUM  PD2
#define BOOTLOAD_PIN_DDR  DDRD
#define BOOTLOAD_PIN_REG  PIND
#define BOOTLOAD_PIN_VAL  1
```

`BOOTLOAD_ON_PIN` enables this method, and the rest of the values state that we're expecting
pin `PD2` to be `HIGH` (1) in order to enter programming mode.

### With EEPROM value

This method checks the value of a byte in EEPROM. If the value is `1`, it jumps to the main program,
otherwise it enters programming mode. (this is the method used in the [test program](/test_program/))

For example:
 * The main program sets the EEPROM value to `0` and resets the device.
 * The bootloader see's the value is set to `0` and enters programing mode
 * The new program is written to the device's flash and the bootloader jumps to the new main program.
 * The main program sets the EEPROM value to `1` so the device will automatically start the program at next reset (otherwise it will be stuck in programming mode)

**IMPORTANT:** The bootloader will automatically set this value to `0` before starting your program --
in case the program was corrupted (failsafe) -- so your program will need to set this value back to `1` to confirm
it's working. Otherwise, the next time your device reboots, it will stay in the bootloader.

**Configure**
```c
// Enable
#define BOOTLOAD_ON_EEPROM 1

// EEPROM address
#define EEPROM_RUN_APP (uint8_t*) 0
```

In this example, we're enabling the EEPROM setting (`BOOTLOAD_ON_EEPROM`) and checking the
EEPROM value at address `0` (`(uint8_t*)0`).


## Communication

By default we communication is expected to be via the default UART RX input at 115200 baud.
You can easily change how it receives data in the "Communications" section of `config.h`.

  * `SERIAL_BAUD` - The serial baud rate
  * `commSetup` - Initializes the communication channel (by default it sets up the serial port).
  * `commReceive` - Receives and returns a single byte from the communication channel.

### Communication Protocol

All communication between the programmer and nodes follow the [DiscoBus protocol](https://github.com/jgillick/Disco-Bus-Protocol/blob/master/docs/messages.md).
The program is divided up into flash pages, the size of which is different for all devices (see `SPM_PAGESIZE`
or check your datasheet for "page size"), and sent one page at a time.

The following commands are sent by the programer (the command codes can be changed in `config.h`):

 * Start (`0xF1`) - Starts the process and sends the version number of the incoming program.
 * Page number (`0xF2`) - Sends the page number that is about to be sent.
 * Page date (`0xF3`) - Sends the page of data.
 * End (`0xF4`) - Programming is complete.

## Version Checking

The start message will send the version of the incomin gprogram, which  the bootloader will compare
against a couple values in the EEPROM. If it matches the current version, the rest of the programming
messages will be ignored.

**NOTE:** These values must be set by the program, the bootloader will not set the versio number.

Configure versioning in `config.h` with the following settings:

 * `USE_VERSIOING` - Comment this out to disable the feature.
 * `VERSION_MAJOR` - The EEPROM address of the major version number.
 * `VERSION_MINOR` - The EEPROM address of the minor version number.

