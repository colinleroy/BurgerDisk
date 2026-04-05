# Fitting the Mini's PCB into a full-size BurgerDisk enclosure
If you have a Mini BurgerDisk and would like to house it in the
full-size BurgerDisk enclosure (for style, or to get a full-size
SD card, etc), this is possible with a bit of tinkering.

The Mini's PCB comes with two cuttable traces - one for the onboard LED's power,
and one for the onboard's MicroSD reader. It also comes with headers to connect
an external LED and card reader instead.

This is what you start with (LED header already soldered in here, forgive the
oversight):
![The Mini PCB](./pictures/mini-upgrade/mini-pcb.jpeg)

## BOM (bill of materials)
The upgrade requires:
- A full-size enclosure (SMD version)
- A microSD or SD card module
- A ⌀3mm LED
- A resistor (the LED is 5V powered, check the required values. 150Ω is good
  for a blue LED)
- A 2-pin Dupond male header
- A 6-pin Dupond male header
- A 2-pin Dupond cable with female headers on one end
- A 6-pin Dupond cable with female headers on both ends
- A short length of ⌀3mm heatshrink

This is what you need:
![The upgrade kit](./pictures/mini-upgrade/upgrade-requirements.jpeg)

## Solder the headers on the LED and MicroSD headers on the PCB
![Headers soldered](./pictures/mini-upgrade/adding-headers.jpeg)

## Cut the traces powering the internal components
The traces are under the ISP header, clearly identified:
![The traces to cut](./pictures/mini-upgrade/solder-cut-spots.jpeg)

Cut them with a sharp knife.
![Cutting the traces](./pictures/mini-upgrade/cutting-traces.jpeg)

Once done, verify that there is no continuity between the surrounding pads.
![The traces, cut](./pictures/mini-upgrade/traces-cut.jpeg)

If you want to go back to a Mini enclosure at a later point, reconnecting these
will be possible with two solder points.

## Assemble the LED and resistor on their cable
Cut the LED's long leg shorter.
![Cutting the LED's anode](./pictures/mini-upgrade/led-cutting.jpeg)

Cut the resistor's legs shorter, and tin them with solder. Do the same with
the LED's legs and the wires.
![Tin the resistor with solder](./pictures/mini-upgrade/resistor-tinning.jpeg)

Solder the resistor on the LED's short leg, add a bit of heatshrink, and solder
to the wire.
![Soldering the resistor on the LED](./pictures/mini-upgrade/soldering-resistor.jpeg)
![Adding the heatshrink](./pictures/mini-upgrade/assembling-led.jpeg)
![Soldering to the wire](./pictures/mini-upgrade/led-assembled.jpeg)

Add a bit of heatshrink around the LED's body, to prevent light from going out
in all directions.
![Heatshrinking the LED](./pictures/mini-upgrade/LED-heatshrink.jpeg)

## Assemble your kit in the full-size enclosure
Install the PCB in the full-size enclosure, with the provided spacers:
- 2mm under and 7mm on top if you have a PCB-type DB19F connector
- 2+7mm under if you have a solder-cups DB19F connector

Screw the card reader module in place, insert the LED in its holder, and connect
both to the PCB.

![The result](./pictures/mini-upgrade/done.jpeg)

You're done!
