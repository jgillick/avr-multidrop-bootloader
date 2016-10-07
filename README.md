# MultiDrop Bootloader for AVR

A bootloader for AVR devices on a multidrop bus, like RS485, where
all devices can be programmed at once.


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

## The Signal Line

Since we can assume there will be multiple devices listening on the same communication line,
we cannot have effective 2-way communication between master and the nodes to be programmed.
Instead, the bus needs an additional shared line, called signal, that any nodes can
enable (enable low) to inform the programmer of errors.

![Bus Layout](./diagrams/bus.png)

By default this is set to pin `PD7`, but it can be configured in the "Signal Line" section
of `config.h`.

## How to get into the bootloader

The bootloader will start automatically when the microcontroller boots up. So something
in the main program will need to tell the device to reboot. Either a special command or
button press. Once the device has rebooted, we need to tell the bootloader to receive a
program. There are two configurable ways baked into the program.

### Input on boot

This method detects a pin state when the bootloader begins (i.e. a button press).
If the pin state matches what we expect, the bootloader will enter programming mode.

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

### EEPROM value

This method checks the value of a byte in EEPROM. If the value is `1`, it jumps to the firmware,
otherwise it enters programming mode.

**IMPORTANT:** The bootloader will automatically set this value to `1` before starting your program --
in case the program was corrupted -- so your program will need to set this value back to `0` to confirm
it's working. Otherwise, the next time your device reboots, it will stay in the bootloader.

```c
#define BOOTLOAD_ON_EEPROM 1
#define EEPROM_RUN_APP (uint8_t*) 0
```

In this example, we're enabling the EEPROM setting (`BOOTLOAD_ON_EEPROM`) and checking the
EEPROM value at address `0` (`(uint8_t*)0`).


## Messages

All messaging follows the [DiscoBus protocol](https://github.com/jgillick/Disco-Bus-Protocol/blob/master/docs/messages.md),
with the following commands used to program the device (the command codes can be changed in `config.h`):

The program is divided up into flash pages, the size of which is different for all devices (see `SPM_PAGESIZE`
or check your datasheet for "page size"), and sent one page at a time.

 * Start (`0xF1`) - Sends the version number of the incoming program.
 * Page number (`0xF2`) - Sends the page number that is about to be sent.
 * Page date (`0xF3`) - Sends the page of data.
 * End (`0xF4`) - Programming is complete.


## Communication

By default we expect all messages to be sent via the default UART RX input at 115200 baud.
You can easily change how it receives data in the "Communications" section of `config.h`.

  * `SERIAL_BAUD` - The serial baud rate
  * `commSetup` - Initializes the communication channel (by default it sets up the serial port).
  * `commReceive` - Receives and returns a single byte from the communication channel.

## Versions

The start message will send the version of the incomin gprogram, which  the bootloader will compare
against a couple values in the EEPROM. If it matches the current version, the rest of the programming
messages will be ignored.

**NOTE:** These values must be set by the program, the bootloader will not set the versio number.

Configure versioning in `config.h` with the following settings:

 * `USE_VERSIOING` - Comment this out to disable the feature.
 * `VERSION_MAJOR` - The EEPROM address of the major version number.
 * `VERSION_MINOR` - The EEPROM address of the minor version number.

