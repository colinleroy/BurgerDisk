# BurgerDisk - a daisy-chainable Smartport hard drive

**Warning - everything in git, and not a release, is not necessarily fully tested.**

This device provides the Smartport-capable Apple II with an SD-card-based
hard-drive, like the SmartportSD, SPIISD or FloppyEmu projects do.

Contrary to those, the BurgerDisk firmware handles a daisy-chain port
and allows you to have more Smartport devices and/or dumb floppy drives
connected behind it.

Contrary to the SPIISD v2 and FloppyEmu projects, BurgerDisk is free software and
free hardware, and licensed in a way to keep it that way.

Contrary to the SPIISD v2 and FloppyEmu projects, BurgerDisk has no screen UI,
and contrary to the SPIISD v1 project, it has no "boot partition" selection
button. The Apple II will be able to boot from the first partition when booting
from slot 5. This is both a choice - I wanted this device to resemble a
period-correct hard drive - and a limitation - there are no extra digital GPIOs
available on the Nano board.

The code of this firmware is based on:
- Apple //c Smartport Compact Flash adapter, written by Robert Justice <rjustice(at)internode.on.net>
- Ported to Arduino UNO with SD Card adapter, written by Andrea Ottaviani <andrea.ottaviani.69(at)gmail.com>
- FAT filesystem support, written by Katherine Stark
- Daisy chaining is based on SmartportVHD's reverse engineering, written by Cedric Peltier

## Building the BurgerDisk
Full instructions for building your own BurgerDisk are available in the
[BurgerDisk build instruction page](build_instructions/README.md).

## Using the BurgerDisk
### Plugging in
Plug the BurgerDisk to your Apple II, or to the previous Smartport device in the
chain. Plug the next device to the daisy port of the BurgerDisk.

When daisy-chaining, the usual Apple II rules apply:
- first the dumb 3.5" floppy disk drives (like the Apple 3.5 drive)
- followed by the smart 3.5" disk drives (like the Unidisk 3.5)
- followed by Smartport devices (like BurgerDisk, Fujinet, SPIISD, etc)
- followed by dumb disk drives (like the Disk //c)

So far, BurgerDisk has been tested in the following configurations:
- Alone on the //c and IIgs external floppy connector
- computer => BurgerDisk => Disk //c
- computer => Unidisk 3.5 => BurgerDisk
- computer => Unidisk 3.5 => BurgerDisk => Disk //c

It has been tested with the following computers:
- Apple //c A2S4000, ROM3
- Apple //c A2S4100, ROM4x
- Apple IIgs, ROM 01

### Configuring disk images
The firmware will open and present between one and four partitions.
If a config.txt file exists on the SD card, it will use the first four
lines as filenames for the partitions to open. An optional fifth line
containing "debug=1" will turn debug messages on.

For example, this will present `prodos.po` and `total_replay.po`, and turn
debug on:
```
prodos.po
total_replay.po


debug=1
```

If no config.txt file exists, the firmware will search for PARTx.po
files, with x between 1 and 4.

## Compatibility notes
The firmware is compatible with SPIISD v1 and SPIISD v2 boards. It fixes
two bugs in their firmwares:
- Too slow init preventing the partitions to be visible by ProDOS after booting
  from internal floppy,
- A non-recoverable freeze on A2S4100 Apple //c with the memory expansion board.

But,
- Of course, running this firmware on an SPIISD board will not allow for daisy chaining.
- For daisy-chaining, the BurgerDisk firmware makes use of pins that the SPIISD
  boards use for button(s) and/or LCD. If you want to replace the SPIISD firmware
  with BurgerDisk's, it is safer to unsolder the buttons and remove the LCD. The
  BurgerDisk firmware does not handle any buttons or LCD anyway.

## Licensing
This firmware is licensed under the GPL v3.

The PCBs and enclosure are licensed under CC BY-SA 4.0.

This means that anyone is welcome to **use, modify, distribute and sell hardware**
based on BurgerDisk. Using and modifying does not require you to do anything.
**Distributing and/or selling requires you to provide the source files for the
firmware and the PCBs, under the same licenses**. It can just be a link to this
repository if no modification is made. If modifications are made, think free
software, and consider submitting pull requests, it could help avoiding
fragmentation.

## Thanks and acknowledgments

Many thanks to Robert Justice, Katherine Stark and Andrea Ottaviani for the
initial work on a simple SD-based Smartport device.

Many thanks to Cedric Peltier for his work on SmartportVHD, which does support
daisy-chaining, but is sadly abandoned, and based on a weird-ass development
board. It was very well documented and helped me a lot implementing daisy-chaining.

Many thanks to Ralph Irving, who spent many hours beta-testing and pinpointing
bugs I could not have fixed without his help (for lack of hardware).

Thanks also to Steve Chamberlin of [BMOW](https://www.bigmessowires.com/), for
documenting a lot of their Smartport/FloppyEmu work on their blog.
