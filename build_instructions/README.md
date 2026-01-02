# Building the BurgerDisk

## BOM (bill of materials)
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

## Printing the enclosure
You can use the .stl files provided in this repository. If you want to modify
the enclosure, use the .obj file.
It is suggested to print the enclosure first, as it will help you align the
female DB19 of the daisy port.

Once printed, remove the "covers" in the back of the bottom part so that both
the MicroSD slot and the DB19 slot are open.

## Building the main PCB
**All elements are to be soldered on the top-side of the PCB**, the side with the
markings, except for the MicroSD card module.

### Daisy DB19
Align the female DB19 connector with the Daisy out pads on the PCB. Insert both into
the enclosure. Make sure the DB19 chassis is stuck between the wall and the
notches. Make sure the notches on the enclosure are inside the PCB mounting holes.

Realign the DB19 with the pads; pay close attention to the alignment, as the
DB19 will basically be un-desolderable with basic equipment. Solder one pin.

Remove the PCB and DB19 from the enclosure, and finish soldering the 18 remaining
pins (there are 10 on the top, 9 on the bottom in total).

### Resistors
Insert the 150Ω resistor in its spot, marked "150Ω" at the bottom of the PCB.

Insert the 330Ω resistor in its spot at the top-left of the PCB.

Solder them. Resistors are not polarized, so you can put them either way.

### Diodes
Insert the 1N5818 diode in its spot, marked "1N5818", near the Daisy out port.
Mind that diodes **are** polarized, so make sure the line on the PCB silkscreen and
the line on the diode are facing the same way.

Insert the two 1N4448 diodes in their spots, marked "1N4448". One is at the
bottom of the PCB, one is right over the IDC20 port. Mind the polarity line too.

Solder them.

### 2-pin headers
Insert 2-pin male headers in the "IIgs", "SPadv" and "UART" spots. Solder them.
They might not want to stay upright, so solder one pin, then put your finger on
the header and re-heat the solder to have the jumpers vertical. You can then
solder their second pin.

### Input IDC-20 connector
Insert the IDC-20 connector in its spot. Mind the notch position, it must be
towards the top of the PCB. Solder it.

### Arduino Nano
Solder the Nano's 6-pin header for programming on top of the Nano. The top of
the Nano is the side with the USB connector and the reset button.

This is not strictly necessary, but I like to put the Nano on a "socket" so that
it is easy to replace if necessary.

Assemble the Nano's two 15-pin connector with the 15-pin female connectors.
Insert these on the PCB, in the "NANO" spot, with the female headers towards
the PCB. 

Insert the Nano on top of its 15-pins connectors. Make sure the USB port is
facing left, over the "USB conn" marking on the PCB. Solder the 30 pins to the
Nano.

Reverse the PCB, and solder the female headers' 30 pins to the PCB.

You can [program the Arduino at this point](../firmware/README.md).

### MicroSD card module
The MicroSD card module is the only module to be soldered on the bottom side of
the PCB. This requires straightening the 6 pins of the module, with a small pair
of pliers.

Position the MicroSD module in its spot from underneath the PCB. Make sure the
pin markings correspond (GND to GND, etc). There are two connectors on the PCB
for the MicroSD module, choose the connector so that the module sticks 5mm out
of the PCB.

Solder them.

### LED
If you plan on using the enclosure, connect the LED using about 15 cm of wire.
Otherwise you can connect the LED directly on the PCB.

You have options there:
- direct connection to the PCB
- solder a 2-pin header on the PCB, and use Dupont connectors
- solder wires to the LED and to the PCB

Mind that there is a bug in Fritzing that makes the LED solder pads inverted:
contrary to the standard, the square one is *not* the cathode. The cathode
(short leg of the LED, on the side where there is a small flat on the LED's body)
must be connected to the "-" side of the LED connector, where the flat is noted
on the PCB's silkscreen. The LED's anode goes on the other side, identified "+",
in the square solder pad.

## The main connection cable
Because of its daisy-chaining capabilities, the cable to plug BurgerDisk to the
Apple II is more complicated than most DB19-to-IDC20 adapters.

The correct DB19-to-IDC20 adapter for this project is provided in this
repository.

It replicates the wiring of the **Apple 3.5 drive cable**, so using such a cable
from a broken floppy drive is also an option. I suppose, but did not verify, that
it would also work with a Unidisk cable. The necessary wiring is as follows:

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

This is the wiring provided by the DB19-to-IDC20 adapter in this repository.

### Assembling the adapter-based cable
Align the male DB19 connector with the pads on the adapter PCB. Pay attention
to the alignment, as the DB19 connector will basically be un-desolderable.
Solder it.

Insert the IDC-20 connector in the adapter PCB. It must on the top-side of the
PCB, where the IDC-20 is outlined on the silkscreen. Pay attention to the notch
position, which should be towards the DB19 connector. Solder it.

## Quick test
Before plugging things in, it is good to check if nothing is inserted the wrong
way. Using a multimeter in "continuity" test mode,
- Verify ground continuity:
  - between the MicroSD module's GND and the three top-right pins of the IDC20
  - between the top-right pin of the IDC20 and the 4th top-right pin of the Nano
  - between the top-right pin of the IDC20 and the three top-most pins of the
    DB19 connector, front of the PCB
  - between the top-right pin of the IDC20 and the GND pin of the UART
- Verify +5V:
  - between pin 11 of the IDC20 connector (top row, 5th starting from left) and
    pin 6 of the Daisy DB19 (6th from the top, front side of the PCB)
  - plug your input cable, and verify continuity between its +5V pin (pin 6, 6th
    from the top when the DB19 is facing left, front side) and pin 6 of the
    Daisy DB19 (6th from the top, front side of the PCB)

## Assembling in the enclosure
Put the PCB back in the enclosure's bottom, making sure the enclosure notches
are in the PCB mounting holes and the daisy DB19 connector's chassis sits
between the enclosure wall and notches. Plug the input cable in, and route it
out of the enclosure via its dedicated hole in the back.

Put the enclosure's top on the bottom, making sure the notches are aligned so
that the PCB does not move around. Turn the enclosure on its back, and screw it
closed with four M2 x 12mm screws.

## Hardware configuration
If you're going to use your BurgerDisk with an Apple IIc or IIgs, close the "SPadv"
jumper. If you're going to use it with a IIgs, close the "IIgs" jumper.
Never close these jumpers if you're going to use the BurgerDisk on an Apple II
with a non-Smartport controller card, **even in conjunction with a SoftSP card**.
