
# Setup

Setting up your bus to be programmed is remarkably easy.

## Burning the bootloader



## Communication

Now you're going to need a way to communicate with your nodes. Using an FTDI breakout board is the easiest
way to go, but you need to use on that provides access to the `DSR` for signal and `TXEN` (for the RS485 transceiver).

### BOB-12731 FTDI Breakout
The [BOB-12731](https://www.sparkfun.com/products/12731) FTDI breakout board, by SparkFun, provides all the necessary pins for
a full RS485 multi-programmer setup.

### Custom - DiscoDongle

The [DiscoDongle](https://github.com/jgillick/disco-dongle) is a FTDI to RS485 adapter that is specially
designed for this purpose. It's an open design, so you can easily make one yourself or order one from
MacroFab (see README for instructions).

## Circuit

Using an FTDI and cheap RS485 transceivers (MAX485 in the diagram, but I personally prefer [ISL81487](https://www.digikey.com/product-search/en?keywords=ISL81487))
you can easily connect your programmer to multiple devices on a single bus.

_CIRCUIT TBD_
