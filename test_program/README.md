# Test Program

This is an example of how a main program can kick off the bootloader programming mode. 

The program blinks an LED and watches for the programming command. When it gets that message from the programmer, 
it will set the `EEPROM_RUN_APP` value to `0` and then reset. The bootloader will take it from there.

In this case, the program is using the [Disco Bus protocol](https://github.com/jgillick/Disco-Bus-Protocol) to communicate
with the programer. When the programmer sends a `0xF0` command, the program resets into programming mode. Your application can use
whatever method you'd like, the important thing demonstrated here is setting the EEPROM value and resetting the device.

## Setup

 * To start fresh, erase the device completely. For example: `avrdude -p atmega328p -c usbtiny -e`
 * Setup the bootloader to program on EEPROM value (this is the default):
   * Open `config.h`
   * set `BOOTLOAD_ON_EEPROM` to `1`
   * set `EEPROM_RUN_APP` to `(uint8_t*) 0x00`
 * Compile bootloader
 * Follow the [instructions here](/SETUP.md) to burn the bootloader to the device. 
 * The bootloader should default into programming mode.
 * Add an LED to `PB2`
 * Compile this test program for your device:
    * Change the `MCU` & `F_CPU` variables in `Makefile` to match your device
    * Run: `make`
 * Use a [programmer](/README.md#programmers) to send `test_program.hex` to the device. 
 
You should now see the LED on `PB2` blinking. 
 
## Updating the firmware
 
As mentioned earlier, this program is expecting to see a `0xF0` Disco Bus command to tell it to reset into programming mode.
This means that you'll need to send this message before your programmer attempts to send the new program.

If you're using [Node Multi-Booloader](https://github.com/jgillick/node-multibootloader), the CLI program 
can do this for you. Simply pass `--command <command>`. For example:

```bash
multiprogrammer --baud 115200 --device /dev/cu.usbDevice0 -page-size 128 --command 0xF0 ./program.hex
```

Now, update the program to change the blink speed (change `_delay_ms(500)` to `_delay_ms(2000)`), recompile it and send it
with the programmer.

If everything worked the LED should be blinking in 1 second intervals.
