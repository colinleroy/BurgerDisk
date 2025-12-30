//*****************************************************************************
// BurgerDisk firmware.
// This device provides the Smartport-capable Apple II with an SD-card-based
// hard-drive, like the SmartportSD, SP2SD or FloppyEmu projects do.
// Contrary to those, the BurgerDisk firmware handles a daisy-chain port
// and allows you to have more Smartport devices and/or dumb floppy drives
// connected behind it.
// The usual Apple II daisy-chaining rules apply:
// - first the 3.5" floppy disk drives
// - followed by Smartport devices
// - followed by dumb disk drives.
//
// The code of this firmware is based on:
// 
// - Apple //c Smartport Compact Flash adapter
//      Written by Robert Justice <rjustice(at)internode.on.net>
// - Ported to Arduino UNO with SD Card adapter
//      Written by Andrea Ottaviani <andrea.ottaviani.69(at)gmail.com>
// - FAT filesystem support
//      Written by Katherine Stark
// - Daisy chaining based on SmartportVHD's reverse engineering
//      Written by Cedric Peltier
//
// The firmware will open and present between one and four partitions.
// If a config.txt file exists on the SD card, it will use the first four
// lines as filenames for the partitions to open. An optional fifth line
// containing "debug=1" will turn debug messages on.
// If no config.txt file exists, the firmware will search for PARTx.po
// files, with x between 1 and 4.
//
// The firmware is compatible with SPIISD v1 and SPIISD v2 boards. It fixes
// two bugs in that firmware:
// - Too slow init preventing the partitions to be visible by ProDOS after
//   booting from internal floppy,
// - It fixes a non-recoverable freeze on A2S4100 Apple //c with the memory
//   expansion board.
// Of course, running this firmware on an SPIISD board will not allow for
// daisy chaining.
// The slow initialization problem has two parts, one of them is a fix in the
// code, the other one depends on the Arduino boot process and this one must
// be fixed by using an AVR programmer to upload the firmware.
//
// FIXME: This code uses the SDFat library, written by Bill Greiman. It builds
// with SDFat version 2.1.2, which is outdated. 
//
// This firmware is licensed under the GPL v3.
//
//*****************************************************************************

#include "SdFat.h" // SDFat version 2.1.2
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "packet_codecs.h"
#include "sp_pins.h"
#include "sp_low.h"
#include "sp_vals.h"
#include "log.h"

#define SPI_CLOCK SD_SCK_MHZ(50)
#define SD_CONFIG SdSpiConfig(PIN_CHIP_SELECT, DEDICATED_SPI, SPI_CLOCK)

#define MAX_PARTITIONS 4
int open_partitions = 0;
int debug = 0;
unsigned char *packet_buffer;

// We need to remember several things about a device, not just its ID
struct device {
  File sdf;
  unsigned char device_id;              //to hold assigned device id's for the partitions
  unsigned long blocks;                 //how many 512-byte blocks this image has
  unsigned int header_offset;           //Some image files have headers, skip this many bytes to avoid them
};

struct device devices[MAX_PARTITIONS];
SdFat sdcard;

static void log_io_err(const __FlashStringHelper *op, int partition, int block_num) {
  Serial.print(op);
  Serial.print(F(" error on partition "));
  Serial.print(partition);
  Serial.print(F(", block "));
  Serial.println(block_num);
}

//SD card init and images opening
static int storage_init_done = 0;
static void init_storage(void) {
  if (storage_init_done) {
    return;
  }
  DEBUGN(F("Free memory before opening images: "), freeMemory(), DEC);

  // Not enough RAM for SDFat to open a file if the standard packet_buffer
  // is allocated.
  free(packet_buffer);

  if (!sdcard.begin(SD_CONFIG)) {
    LOG(F("Error init card"));
    led_err();
  }

  SdFile myFile("config.txt", O_READ);
  // check for open error
  LOG(F("Opening partitions"));
  if (myFile.isOpen()) {
    unsigned char n;
    packet_buffer = (unsigned char *)malloc(100);
    for(unsigned char i = 0; i < MAX_PARTITIONS; i++) {
      n = myFile.fgets((char*)packet_buffer, 100);
      if(n > 1) {
        if (packet_buffer[n - 1] == '\n') {
          packet_buffer[n-1] = 0;
        }
        LOG((char*)packet_buffer);

        open_image(devices[i],(char*)packet_buffer);
        if(!devices[i].sdf.isOpen()){
          LOG(F("Image open error!"));
        }
        else {
          open_partitions++;
          if ((packet_buffer[n-4]=='2')&&((packet_buffer[n-3]&0xdf)=='M')&&((packet_buffer[n-2]&0xdf)=='G')) {
            devices[i].header_offset=64;
          }
          else {
            devices[i].header_offset=0;
          }
        }
      } else {
        break;
      }
    }

    /* Now get parameters */
    while (myFile.fgets((char*)packet_buffer, 100) > 0) {
      if (!strncmp((const char *)packet_buffer, "debug=", 6)) {
        debug = (packet_buffer[6] == '1');
      }
    }
    free(packet_buffer);
    myFile.close();

  } else {
    LOG(F("No config.txt. Searching for images."));
    for (unsigned char i = 0; i < MAX_PARTITIONS; i++) {
      String prefix = F("PART");
      open_image(devices[i], prefix+(i+1)+".po");
      if (devices[i].sdf.isOpen()) {
        open_partitions++;
      }
    }
  }
  // Realloc standard packet_buffer
  packet_buffer = (unsigned char *)malloc(605);
  memset(packet_buffer, 0, 605);
  DEBUGN(F("Free memory now "), freeMemory(), DEC);

  storage_init_done = 1;
}

//Arduino boot setup - serial, pinmodes.
void setup (void) {
  // LED
  SET_LED_OUT;
  SET_LED_HIGH;

  // Serial init
  Serial.begin(230400);
  LOG(F("BurgerDisk v1.0"));

  packet_buffer = (unsigned char *)malloc(605);

  /* Main pins on upstream port */
  SET_WR_HIGH; SET_WR_IN;     //WR: input pullup
  SET_DRV1_IN; SET_DRV1_HIGH; //DRV1 & 2: input
  SET_DRV2_IN; SET_DRV2_HIGH;

  SP_ACK_MUTE();
  SP_RD_MUTE();

  /* Daisy pins */
  SET_DAISY_PH3_OUT;          // PH3: output
  daisy_ph3_disable();

  SET_DAISY_DRV1_OUT;         // DRV1 & 2: output
  SET_DAISY_DRV2_OUT;
  daisy_diskII_mirror();

  SET_DAISY_HDSEL_IN;         // HDSEL: input pullup
  SET_DAISY_HDSEL_HIGH;

  LOG(F("Ready"));

  SET_LED_LOW;
}

// Helper to match a Smartport device_id to one of our images
static int get_device_partition(int device_id) {
  for  (int p = 0; p < MAX_PARTITIONS; p++) {
    if (devices[p].device_id == device_id) {
      return p;
    }
  }
  return -1;
}

// Helper to extract the block number from the Smartport packet
static unsigned long int smartport_get_block_num_from_buf(void) {
  unsigned long int block_num;
  unsigned char LBH, LBL, LBN, LBT;

  LBH = packet_buffer[16]; //high order bits
  LBN = packet_buffer[19]; //block number low
  LBL = packet_buffer[20]; //block number middle
  LBT = packet_buffer[21]; //block number high

  block_num = (LBN & 0x7f) | (((unsigned short)LBH << 3) & 0x80);
  block_num = block_num + ( ((unsigned long)((LBL & 0x7f) | (((unsigned short)LBH << 4) & 0x80))) << 8);
  block_num = block_num + ( ((unsigned long)((LBT & 0x7f) | (((unsigned short)LBH << 5) & 0x80))) << 16);

  return block_num;
}

//Get Smartport state from phases pins
static inline SP_State smartport_get_state(void) {
  if (PHASES_BUS_RESET) {
    return SP_BUS_RESET;
  }
  if (PHASES_BUS_ENABLE) {
    return SP_BUS_ENABLED;
  }
  return SP_BUS_DISABLED;
}


static inline void daisy_ph3_mirror(void) {
  if (RD_PORT_PHASES & _BV(PIN_PH3)) {
    SET_DAISY_PH3_HIGH;
  } else {
    SET_DAISY_PH3_LOW;
  }
}

static inline void daisy_ph3_disable() {
  SET_DAISY_PH3_LOW;
}

static inline void daisy_diskII_mirror(void) {
  if (DRV1_IS_HIGH) {
    SET_DAISY_DRV1_HIGH;
  } else {
    SET_DAISY_DRV1_LOW;
  }

  if (DRV2_IS_HIGH) {
    SET_DAISY_DRV2_HIGH;
  } else {
    SET_DAISY_DRV2_LOW;
  }
}

static inline void daisy_diskII_disable(void) {
  SET_DAISY_DRV1_HIGH;
  SET_DAISY_DRV2_HIGH;
}

//SMARTPORT COMMAND HANDLERS ---------------------------------------------------

//Smartport RESET handler
static unsigned char number_partitions_initialised = 0;
static unsigned char device_init_done = 0;
static void smartport_device_reset(void) {
  LOG(F("RESET"));

  //reset number of partitions init'd
  number_partitions_initialised = 0;
  device_init_done = 0;

  //clear device_id table
  for (int partition = 0; partition < MAX_PARTITIONS; partition++) {
    devices[partition].device_id = 0;
  }

  daisy_diskII_disable();

  // Ready. Wait for reset to clear
  while(smartport_get_state() == SP_BUS_RESET);
}

//Smartport STATUS handler
static void smartport_answer_status(int partition, unsigned char extended) {
  unsigned char status_code;

  status_code = (packet_buffer[extended ? 21 : 19] & 0x7f);

  // if statcode=3, then status with device info block
  if (status_code == 0x03) {
    if (extended) {
      LOG(F("Extended status DIB - Not implemented!"));
    } else {
      DEBUG(F("Sending DIB"));
      encode_status_dib_reply_packet(devices[partition].device_id, devices[partition].blocks);
    }
  } else {
    // just return device status
    if (extended) {
      encode_extended_status_reply_packet(devices[partition].device_id, devices[partition].blocks);
    } else {
      encode_status_reply_packet(devices[partition].device_id, devices[partition].blocks);
    }
  }
  SendPacket( (unsigned char*) packet_buffer);
  DEBUGN(F("STATUS DID "), devices[partition].device_id, HEX);
}

//Smartport READ handler
static void smartport_read_block(int partition) {
  unsigned long int block_num;
  block_num = smartport_get_block_num_from_buf();

  DEBUGN(F("READ DID "), devices[partition].device_id, HEX);
  DEBUGN(F(" Block "), block_num, DEC);

  if (!devices[partition].sdf.seekSet(block_num*512+devices[partition].header_offset)) {
    log_io_err(F("Seek"), partition, block_num);
  }

  //Read block from SD Card
  if (!devices[partition].sdf.read((unsigned char*) packet_buffer, 512)) {
    log_io_err(F("Read"), partition, block_num);
  }
  encode_data_packet(devices[partition].device_id);

  SendPacket( (unsigned char*) packet_buffer);
}

//Smartport WRITE handler
static void smartport_write_block(int partition) {
  unsigned long int block_num;
  block_num = smartport_get_block_num_from_buf();

  DEBUGN(F("WRITE DID "), devices[partition].device_id, HEX);
  DEBUGN(F(" Block "), block_num, DEC);

  //get write data packet
  ReceivePacket( (unsigned char*) packet_buffer);
  AckPacket();
  int status = decode_data_packet();
  if (status == 0) {
    if (!devices[partition].sdf.seekSet(block_num*512+devices[partition].header_offset)) {
      log_io_err(F("Seek"), partition, block_num);
      goto err;
    }
    // Write block to SD Card
    if (!devices[partition].sdf.write((unsigned char*) packet_buffer, 512)) {
      log_io_err(F("Write"), partition, block_num);
err:
      status = 6;
    }
  }

  //now return status code to host
  encode_write_status_packet(devices[partition].device_id, status);
  SendPacket( (unsigned char*) packet_buffer);
}

//Smartport FORMAT handler
static void smartport_format(int partition) {
  encode_init_reply_packet(devices[partition].device_id, 0x80); //just send back a successful response
  SendPacket( (unsigned char*) packet_buffer);
}

//Smartport INIT handler
static void smartport_init(unsigned char dev_id) {
  int status, p;

  devices[number_partitions_initialised].device_id = dev_id; //remember device id for partition
  p = number_partitions_initialised;
  number_partitions_initialised++;

  // now we have time to init our partitions before acking
  init_storage();

  if (number_partitions_initialised < open_partitions) { //are all init'd yet
    status = 0x80;
  } else {
    status = DAISY_HDSEL_IS_HIGH ? 0xff : 0x80;
    device_init_done = 1; //Mark init done
    number_partitions_initialised = 0; // Reset variable for potential next INIT
  }
  encode_init_reply_packet(dev_id, status);

  SendPacket( (unsigned char*) packet_buffer);
  DEBUGN(F("INIT P "), p, DEC);
  DEBUGN(F(" DID: "), dev_id, HEX);
  DEBUGN(F(" MORE: "), status == 0x80, HEX);
}

//Not-for-us packet handler
//Mute lines, and wait until Smartport is disabled again
static void IgnorePacket(void) {
  SP_ACK_MUTE();
  SP_RD_MUTE();
  while(smartport_get_state() == SP_BUS_ENABLED);
  interrupts();
}
//------------------------------------------------------------------------------

//*****************************************************************************
// Function: main loop
// Parameters: none
// Returns: 0
//
// Description: Main function for Apple //c Smartport Compact Flash adpater
//*****************************************************************************
#pragma GCC optimize("-O3")

void loop() {
  unsigned char dev_id;
  SP_Command command;
  SP_State smartport_state;
  int partition;

  SP_ACK_MUTE();
  SP_RD_MUTE();

  while (1) {
    noInterrupts();
    daisy_diskII_mirror();

    if (device_init_done) {
      daisy_ph3_mirror();
    }

    smartport_state = smartport_get_state();
    if (smartport_state == SP_BUS_DISABLED) {
      continue;
    }

    interrupts();
    partition = -1;

    switch (smartport_state) {
    case SP_BUS_RESET:
      daisy_ph3_disable();
      smartport_device_reset();
      break;

    case SP_BUS_ENABLED:
      daisy_diskII_disable();

      // We can come back here after handling a previous smartport packet, as
      // it happens the bus stays enabled a bit longer. ReceivePacket() will
      // return with a non-zero code if the smartport bus gets disabled without
      // receiving anything.
      if (ReceivePacket( (unsigned char*) packet_buffer) != 0) {
        // Smartbus is now disabled, we did not get a command. Loop back.
        break;
      }

      dev_id = packet_buffer[6];
      command = (SP_Command) packet_buffer[14];

      if (command == SP_INIT && !device_init_done) {
        AckPacket();
      } else if ((partition = get_device_partition(dev_id)) != -1) {
        AckPacket();
      } else {
        IgnorePacket();
        break;
      }

      SET_LED_HIGH;

      switch (command) {
      case SP_INIT:
        //we may not have a dev_id/partition match yet
        smartport_init(dev_id);
        break;

      case SP_STATUS:
        smartport_answer_status(partition, 0);
        break;

      case SP_EXT_STATUS:
        smartport_answer_status(partition, 1);
        break;

      case SP_EXT_READ:
      case SP_READ:
        smartport_read_block(partition);
        break;

      case SP_WRITE:
        smartport_write_block(partition);
        break;

      case SP_FORMAT:
        smartport_format(partition);
        break;

      case SP_EXT_WRITE:
      case SP_EXT_FORMAT:
      case SP_EXT_INIT:
      default:
        LOGN(F("Command not implemented: "), packet_buffer[14], HEX);
        break;
      }
      SET_LED_LOW;
      break;

    case SP_BUS_DISABLED:
      // Nothing more to do here
      break;
    }
  }
}
#pragma GCC optimize("-Os")

//*****************************************************************************
// Function: led_err
// Parameters: none
// Returns: nonthing
//
// Description: Flashes status led for show error status
// Reboot needed
//*****************************************************************************

void led_err(void)
{
  interrupts();
  LOG(F("Error - Require reboot"));

  while (1) {
    SET_LED_HIGH;
    delay(500);
    SET_LED_LOW;
    delay(500);
  }
}

extern void *__brkval;
extern char __bss_end;
int freeMemory() {
  int free_memory;
  if ((int)__brkval == 0) {
    // if no heap use from end of bss section
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  } else {
    // use from top of stack to heap
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  return free_memory;
}

bool open_image(struct device &d, String filename ) {
  d.sdf = sdcard.open(filename, O_RDWR);
  if (!d.sdf.isOpen()) {
    return false;
  }
  if (!d.sdf.isFile()) {
    return false;
  }

  if (d.sdf.size() == 0) {
    return false;
  }

  d.blocks = d.sdf.size() >> 9;
  return true;
}
