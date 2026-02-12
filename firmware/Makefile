PROGRAMMER := arduino:avrispmkii
BOARD := arduino:avr:nano:cpu=atmega328
LIBRARIES := SdFat:2.3.0
GIT_VERSION := $(shell git describe --match=nervermatchanything --always --abbrev=8 --dirty)

all: upload

setup:
	arduino --install-library $(LIBRARIES)
	#arduino --pref programmer=$(PROGRAMMER)

fuse:
	avrdude -v -p m328p -c avrisp2 -U lfuse:w:0xFF:m -U hfuse:w:0xDA:m -U efuse:w:0xFD:m

version:
	echo "#define GIT_VERSION \"$(GIT_VERSION)\"" > version.h

upload: version
	arduino --upload --board $(BOARD) --useprogrammer firmware.ino
