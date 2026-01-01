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

The code of this firmware is based on:
- Apple //c Smartport Compact Flash adapter, written by Robert Justice <rjustice(at)internode.on.net>
- Ported to Arduino UNO with SD Card adapter, written by Andrea Ottaviani <andrea.ottaviani.69(at)gmail.com>
- FAT filesystem support, written by Katherine Stark
- Daisy chaining is based on SmartportVHD's reverse engineering, written by Cedric Peltier

## Using BurgerDisk

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
Of course, running this firmware on an SPIISD board will not allow for
daisy chaining.
The slow initialization problem has two parts, one of them is fixed in the
code, the other one depends on the Arduino boot process and this one must
be fixed *by using an AVR programmer* to upload the firmware.

## Licensing
This firmware is licensed under the GPL v3.

The PCBs are licensed under CC BY-SA 4.0.

This means that anyone is welcome to use, modify, distribute and sell hardware
based on BurgerDisk. Using and modifying does not require you to do anything.
Distributing and/or selling requires you to provide the source files for the
firmware and the PCBs, under the same licenses. It can just be a link to this
repository if no modification is made. If modifications are made, think free
software, and consider submitting pull requests, it could help avoiding
fragmentation.

## Building the BurgerDisk

### Wiring BurgerDisk to the Apple II
Because of its daisy-chaining capabilities, the cable to plug BurgerDisk to the
Apple II is more complicated than most DB19-to-IDC20 adapters.

The correct DB19-to-IDC20 adapter for this project is provided in this
repository.

It mirrors the wiring of the Apple 3.5 drive cable, so using such a cable from
a broken floppy drive is also an option. I suppose, but did not verify, that it
would also work with a Unidisk cable. The wiring is as follows:

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
or to a DiskII controller on an Apple II with a SoftSP card. In this case, the 
IDC20's pin 17 is likely tied to +12V with pins 13,15,19.
*Disconnect the DRV2 jumper on the BurgerDisk PCB*, or you will fry the Arduino
Nano by feeding it +12V. *This is untested, proceed with caution*.
Please note that some existing adapters tie IDC20's pin 17 to +12V too!

If you proceed with the adapter instead of a salvaged cable, you can get male
DB19 connectors [on ebay at varying prices](https://www.ebay.com/sch/i.html?_nkw=male+DB19).

### The daisy output port
"DB19" is very outdated and hard to procure. One can 
[find them on eBay](https://www.ebay.com/sch/i.html?_nkw=female+DB19), but it
will probably be the most expensive part of the BurgerDisk. Connecting a daisy
output port to the BurgerDisk can be done in two ways:
- using an angled connector desoldered/salvaged from a broken daisy-chainable
  Apple disk drive (in this case, make sure to apply tape to the border connector
  for isolation, and be prepared to bend pins a bit as, *FIXME*, the pins spacing
  is slightly wrong on the PCB).
- using a flat DB19 connector and soldering it on the border pads of the PCB.

### BOM (bill of materials)
Mind that the links provided here might be dead by the time you read this, and
that most of them have options (number of pins, resistor values, etc) that you
should double-check before buying.

Please note that this is work in progress, that the latest PCB revision is untested
yet, and the enclosure is completely untested.

For the main board, you will need:
- the main PCB
- one [Arduino Nano with an Atmega328p processor](https://aliexpress.com/item/1005006773519913.html)
- one [STK500 AVR ISP programmer](https://aliexpress.com/item/1005006205386137.html) for uploading the firmware to the Arduino
- one [microSD module](https://aliexpress.com/item/1005008633757049.html)
- one [IDC20 connector, male, 2.54mm pitch](https://aliexpress.com/item/1005001400147026.html)
- two [2-pin male headers, 2.54mm pitch](https://aliexpress.com/item/1005006181780843.html)
- one [jumper, 2.54mm pitch](https://aliexpress.com/item/4000583368141.html)
- one [330 ohms resistor, 1/4W](https://aliexpress.com/item/32952657927.html)
- one [150 ohms resistor, 1/4W](https://aliexpress.com/item/32952657927.html)
- one [1N5818 diode](https://aliexpress.com/item/4000055728807.html)
- two [1N4448 diodes](https://aliexpress.com/item/1005008591345474.html)
- optionally, [one female DB19 connector](https://www.ebay.com/itm/165875193091) if you want daisy-chaining
- optionally, [two 15-pin female header](https://aliexpress.com/item/1005006934014275.html) for easy Arduino removal
- optionally, one 3mm LED, 3V, 20mA, [with](https://aliexpress.com/item/1005007602705761.html) or [without](https://fr.aliexpress.com/item/1005003320296052.html) wires depending on whether you plan on using the enclosure

For connecting to the computer, you will need:
- an Apple 3.5 drive cable or equivalent, or make it yourself using
  - the [DB19 to IDC20 PCB](https://www.pcbway.com/project/shareproject/BurgerDisk_DB19_to_IDC20_adapter_d6ce25bb.html)
  - an [IDC20 cable, 2.54mm pitch](https://aliexpress.com/item/1005003853804182.html), at least 50cm
  - a second [IDC20 connector, male, 2.54mm pitch](https://aliexpress.com/item/1005001400147026.html)
  - one [male DB19 connector](https://www.ebay.com/itm/257181325655)

For the enclosure, you will need:
- the enclosure's STL files
- four [M2 12mm screws](https://aliexpress.com/item/1005007219475077.html)

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
