
# Setup

Setting up your bus to be programmed is remarkably easy.

## FTDI

The first thing you're going to need is a way to communicate with your nodes. Using an FTDI breakout board is the easiest 
way to go, but you need to use on that provides access to the `DSR` for signal and `TXEN` if you choose to communicate via RS485.

### BOB-12731 
The [BOB-12731](https://www.sparkfun.com/products/12731) FTDI breakout board, by SparkFun, provides all the necessary pins for
a full RS485 multi-programmer setup.

### Custom - DiscoDongle

The DiscoDongle is an open circuit design that will connect your computer to the RS485 bus to your devices with the signal line
ready to go. This basically acheives the same thing as the BOB-12731 with an RS485 transciever circuit, above. __LINK TBD__

## Simple Example: Programmer to Node

This is a simple example of how to wire up to a single node via the built-in UART. Since this bootloader is built
for multi-node programming, this setup is not very common or as effective as many other single-node programming bootloaders.

## Multi-node example with RS485

Using an FTDI and cheap RS485 trancievers (MAX485 in the diagram, but I personally prefer [ISL81487](https://www.digikey.com/product-search/en?keywords=ISL81487))
you can easily connect your programmer to multiple devices on a single bus.
