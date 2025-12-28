# BurgerDisk - a daisy-chainable Smartport hard drive

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

When daisy-chaining, the usual Apple II rules apply:
- first the 3.5" floppy disk drives
- followed by Smartport devices
- followed by dumb disk drives.

The code of this firmware is based on:
- Apple //c Smartport Compact Flash adapter, written by Robert Justice <rjustice(at)internode.on.net>
- Ported to Arduino UNO with SD Card adapter, written by Andrea Ottaviani <andrea.ottaviani.69(at)gmail.com>
- FAT filesystem support, written by Katherine Stark
- Daisy chaining is based on SmartportVHD's reverse engineering, written by Cedric Peltier

## Wiring BurgerDisk to the Apple II
Because of its daisy-chaining capabilities, the cable to plug BurgerDisk to the
Apple II is more complicated than the usual DB19-to-IDC20 adaptors. I am
successfully using an Apple 3.5 drive cable. Its wiring is as follows:

```
Male DB19, seen from front:

1  2  3  4  5  6  7  8  9  10
 11 12 13 14 15 16 17 18 19

Female IDC20, seen from front, ==== is the connector's notch:

20  18  16  14  12  10  8  6  4  2
19  17  15  13  11  9   7  5  3  1
                ====

Wiring:

DB19 pin      IDC20 pin      line
1             1              GND
2             3              GND
3             5              GND
4             7              EN35
5             9              -12V
6             11             +5V
7             13             +12V
8             15             +12V
9             17             DRV2
10            20             WRPROT
11            2              PH0
12            4              PH1
13            6              PH2
14            8              PH3
15            10             WREQ
16            12             HDSEL
17            14             DRV1
18            16             RDDATA
19            18             WRDATA
```

It should be possible to plug a BurgerDisk to the internal port of an Apple IIc,
or to a DiskII controller on an Apple II with a SoftSP card. In this case,
*disconnect the DRV2 jumper on the BurgerDisk PCB*, or you will fry the Arduino
Nano by feeding it +12V. *This is untested, proceed with caution*.

## The daisy output port
"DB19" is very outdated and hard to procure. One can find them on eBay, but it
will probably be the most expensive part of the BurgerDisk. Connecting a daisy
output port to the BurgerDisk can be done in two ways:
- using an angled connector desoldered/salvaged from a broken daisy-chainable
  Apple disk drive (in this case, make sure to apply tape to the border connector
  for isolation, and be prepared to bend pins a bit as, *FIXME*, the pins spacing
  is slightly wrong on the PCB).
- using a flat DB19 connector and soldering it on the border pads of the PCB.

## Using BurgerDisk
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
Of course, running this firmware on an SPIISD board will not allow for
daisy chaining.
The slow initialization problem has two parts, one of them is fixed in the
code, the other one depends on the Arduino boot process and this one must
be fixed *by using an AVR programmer* to upload the firmware.

## Licensing
This firmware is licensed under the GPL v3.

The PCB is licensed under CC BY-SA 4.0.

This means that anyone is welcome to use, modify, distribute and sell hardware
based on BurgerDisk. Using and modifying does not require you to do anything.
Distributing and/or selling requires you to provide the source files for the
firmware and the PCB, under the same licenses.
