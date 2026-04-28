//
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
// This firmware is licensed under the GPL v3.
//
//*****************************************************************************

#include "SdFat.h" // tested with SDFat version 2.3.0
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "packet_codecs.h"
#include "sp_pins.h"
#include "sp_low.h"
#include "sp_vals.h"
#include "log.h"
#if __has_include("version.h")
#include "version.h"
#else
#define GIT_VERSION "UNDEFINED VERSION"
#endif

#define SPI_CLOCK SD_SCK_MHZ(50)
#define SD_CONFIG SdSpiConfig(PIN_CHIP_SELECT, DEDICATED_SPI, SPI_CLOCK)

#define MAX_PARTITIONS 4

#define _MAX_FILENAME_LEN 80
#define _2MG_HEADER_LEN   0x0F

unsigned char open_partitions = 0;
unsigned char debug = 0;
unsigned char force_next_smartport = 1;
unsigned char extended_option = 0;

unsigned char *packet_buffer;
unsigned char identifier = '0';

// We need to remember several things about a device, not just its ID
struct device {
  File sdf;
  unsigned char device_id;              //to hold assigned device id's for the partitions
  unsigned long blocks;                 //how many 512-byte blocks this image has
  unsigned char header_offset;           //Some image files have headers, skip this many bytes to avoid them
};

struct device devices[MAX_PARTITIONS];
SdFat sdcard;

static void log_io_err(const __FlashStringHelper *op, unsigned char partition, int block_num) {
  Serial.print(op);
  Serial.print(F(" error on partition "));
  Serial.print(partition);
  Serial.print(F(", block "));
  Serial.print(block_num);
  Serial.print(F(" - errCode 0x"));
  Serial.println(sdcard.sdErrorCode(), HEX);

  deinit_storage();
  init_storage();
}

static void deinit_storage(void) {
  // cleanup previous possibly opened files
  while (open_partitions > 0) {
    open_partitions--;
    if (devices[open_partitions].sdf.isOpen()) {
      devices[open_partitions].sdf.close();
      devices[open_partitions].blocks = 0;
    }
  }
  sdcard.end();
}

static void init_storage(void) {
  if (open_partitions > 0) {
    return;
  }

  if (!sdcard.begin(SD_CONFIG)) {
    sdcard.initErrorPrint();
    return;
  }

  // check for open error
  LOG(F("Opening partitions"));
  open_partitions = 0;

  // Not enough RAM for SDFat to open a file if the standard packet_buffer
  // is allocated.
  free(packet_buffer);
  packet_buffer = (unsigned char *)malloc(_MAX_FILENAME_LEN + _2MG_HEADER_LEN + 1);

  SdFile configFile("config.txt", O_READ);
  if (configFile.isOpen()) {
    unsigned char n;
    while (open_partitions < MAX_PARTITIONS) {
      n = configFile.fgets((char*)packet_buffer, _MAX_FILENAME_LEN - 1);
      if(n > 1) {
        if (packet_buffer[n - 1] == '\n') {
          packet_buffer[n-1] = 0;
        }
        LOG((char*)packet_buffer);

        open_image(devices[open_partitions],(char*)packet_buffer);
        if(!devices[open_partitions].sdf.isOpen()) {
          LOG(F(" open error"));
        } else {
          if ((packet_buffer[n-4]       == '2')
           &&((packet_buffer[n-3]&0xdf) == 'M')
           &&((packet_buffer[n-2]&0xdf) == 'G')) {
            /* https://gswv.apple2.org.za/a2zine/Docs/DiskImage_2MG_Info.txt */
            /* Read header after name, check ProDOS sector order */
            if (devices[open_partitions].sdf.read(packet_buffer+_MAX_FILENAME_LEN, 0x0F) < _2MG_HEADER_LEN
             || packet_buffer[_MAX_FILENAME_LEN+0x0C] != 0x01) {
              LOG(F(" not in ProDOS format"));
              devices[open_partitions].sdf.close();
              continue;
            }
            /* Apply offset */
            devices[open_partitions].header_offset=packet_buffer[_MAX_FILENAME_LEN+0x08];
          } else {
            devices[open_partitions].header_offset=0;
          }
          open_partitions++;
        }
      } else {
        break;
      }
    }
    configFile.close();
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

  /* Now get parameters */
  SdFile optionsFile("options.txt", O_READ);
  if (optionsFile.isOpen()) {
    while (optionsFile.fgets((char*)packet_buffer, _MAX_FILENAME_LEN) > 0) {
      if (!strncmp((const char *)packet_buffer, "debug=", 6)) {
        debug = (packet_buffer[6] == '1');
      } else if (!strncmp((const char *)packet_buffer, "force_next_smartport=", 21)) {
        force_next_smartport = (packet_buffer[21] == '1');
      } else if (!strncmp((const char *)packet_buffer, "extended=", 9)) {
        extended_option = (packet_buffer[9] == '1');
      }
    }
    optionsFile.close();
  }

  // Realloc standard packet_buffer
  free(packet_buffer);
  packet_buffer = (unsigned char *)malloc(605);
  memset(packet_buffer, 0, 605);
  DEBUGN(F("Free memory now "), freeMemory(), DEC);
}

//Arduino boot setup - serial, pinmodes.
void setup (void) {
  // LED
  SET_LED_OUT;
  SET_LED_HIGH;

  // Serial init
  Serial.begin(230400);
  LOG(F("BurgerDisk version " GIT_VERSION));

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
#define NO_PARTITION 0xFF

static unsigned char get_device_partition(unsigned char device_id) {
  for  (unsigned char p = 0; p < MAX_PARTITIONS; p++) {
    if (devices[p].device_id == device_id) {
      return p;
    }
  }
  return NO_PARTITION;
}

// Helper to extract the block number from the Smartport packet
static unsigned long int smartport_get_block_num_from_buf(unsigned char extended) {
  unsigned long int block_num;
  unsigned char LBE, LBH, LBL, LBN, LBT;

  if (!extended) {
    LBH = packet_buffer[16]; //high order bits
    LBN = packet_buffer[19]; //block number low
    LBL = packet_buffer[20]; //block number middle
    LBT = packet_buffer[21]; //block number high
    block_num = (LBN & 0x7f) | (((unsigned short)LBH << 3) & 0x80);
    block_num = block_num + ( ((unsigned long)((LBL & 0x7f) | (((unsigned short)LBH << 4) & 0x80))) << 8);
    block_num = block_num + ( ((unsigned long)((LBT & 0x7f) | (((unsigned short)LBH << 5) & 0x80))) << 16);
  } else {
    LBH = packet_buffer[16]; //high order bits
    LBN = packet_buffer[18]; //block number low
    LBL = packet_buffer[19]; //block number middle
    LBT = packet_buffer[20]; //block number high
    LBE = packet_buffer[21]; //extra block number
    block_num = (LBN & 0x7f) | (((unsigned short)LBH << 2) & 0x80);
    block_num = block_num + ( ((unsigned long)((LBL & 0x7f) | (((unsigned short)LBH << 3) & 0x80))) << 8);
    block_num = block_num + ( ((unsigned long)((LBT & 0x7f) | (((unsigned short)LBH << 4) & 0x80))) << 16);
    block_num = block_num + ( ((unsigned long)((LBE & 0x7f) | (((unsigned short)LBH << 5) & 0x80))) << 24);
  }

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
static unsigned char never_got_reset = 1;
static void smartport_device_reset(void) {
  LOG(F("RESET"));

  //reset number of partitions init'd
  number_partitions_initialised = 0;
  device_init_done = 0;
  never_got_reset = 0;

  deinit_storage();
  daisy_diskII_disable();

  // Ready. Wait for reset to clear
  while(smartport_get_state() == SP_BUS_RESET);
}

//Smartport STATUS handler
static void smartport_answer_status(unsigned char partition, unsigned char extended) {
  SP_Status_Code status_code;

  if (open_partitions == 0)
    init_storage();

  status_code = (SP_Status_Code)(packet_buffer[extended ? 18:19] & 0x7f);
  DEBUG_CMD(extended?'S':'s', devices[partition].device_id, status_code);

  switch (status_code) {
    case SP_STATUS_DIB:
      encode_status_dib_reply_packet(devices[partition].device_id, extended, devices[partition].blocks);
      break;
    case SP_STATUS_SIMPLE:
      encode_status_reply_packet(devices[partition].device_id, extended, SP_SUCCESS, devices[partition].blocks);
      break;
    default:
      encode_status_reply_packet(devices[partition].device_id, extended, SP_BADCTL, devices[partition].blocks);
      break;
  }
  SendPacket( (unsigned char*) packet_buffer);
}

//Smartport READ handler
static void smartport_read_block(unsigned char partition, unsigned char extended) {
  unsigned long int block_num;
  SP_Error status = SP_SUCCESS;
  unsigned char tries = 3;
  int r;

  block_num = smartport_get_block_num_from_buf(extended);

read_again:
  if (open_partitions == 0)
    init_storage();

  DEBUG_CMD(extended?'R':'r', devices[partition].device_id, block_num);

  if (!devices[partition].sdf.isOpen()) {
    status = SP_OFFLINE;
    goto reply;
  }

  if (!devices[partition].sdf.seekSet(block_num*512+devices[partition].header_offset)) {
    log_io_err(F("Seek"), partition, block_num);
    status = SP_BADBLOCK;
    goto reply;
  }

  //Read block from SD Card
  if ((r = devices[partition].sdf.read((unsigned char*) packet_buffer, 512)) != 512) {
    log_io_err(F("Read"), partition, block_num);
    if (tries--) {
      goto read_again;
    }
    status = SP_IOERROR;
  }
reply:
  if (status != SP_SUCCESS)
    DEBUG_CMD('E', devices[partition].device_id, status);

  encode_data_packet(devices[partition].device_id, extended, status);
  SendPacket( (unsigned char*) packet_buffer);
}

//Smartport WRITE handler
static void smartport_write_block(unsigned char partition, unsigned char extended) {
  unsigned long int block_num;
  SP_Error status;
  unsigned int numodd, numgrps, bytes_to_write;
  unsigned char tries = 5;

  block_num = smartport_get_block_num_from_buf(extended);

try_again:
  if (ReceivePacket((unsigned char*) packet_buffer) != 0) {
    if (tries--) {
      Serial.print('.');
      goto try_again;
    }
  }
  AckPacket();

  numodd  = packet_buffer[11]&0x7F;
  numgrps = packet_buffer[12]&0x7F;
  bytes_to_write = numodd + numgrps*7;

  // DumpPacket(packet_buffer, 2);

  DEBUG_CMD(extended?'W':'w', devices[partition].device_id, block_num);

  if (decode_data_packet(extended) != 0 || bytes_to_write != 512) {
    status = SP_BADCMD;
    goto reply;
  }

  if (open_partitions == 0)
    init_storage();

  if (!devices[partition].sdf.isOpen()) {
    status = SP_OFFLINE;
    goto reply;
  }

  status = SP_SUCCESS;

  if (!devices[partition].sdf.seekSet(block_num*512+devices[partition].header_offset)) {
    log_io_err(F("Seek"), partition, block_num);
    status = SP_BADBLOCK;
    goto reply;
  }
  // Write block to SD Card
  if (devices[partition].sdf.write((unsigned char*) packet_buffer, bytes_to_write) != bytes_to_write) {
    log_io_err(F("Write"), partition, block_num);
    // Sadly no retry here, ase init_storage() destroys packet_buffer and we
    // don't have enough RAM to do otherwise.
    status = SP_IOERROR;
  }
  if (!devices[partition].sdf.sync()) {
    log_io_err(F("Sync"), partition, block_num);
    status = SP_IOERROR;
  }

reply:
  if (status != SP_SUCCESS)
    DEBUG_CMD('E', devices[partition].device_id, status);

  //now return status code to host
  encode_write_status_packet(devices[partition].device_id, extended, status);
  SendPacket( (unsigned char*) packet_buffer);
}

//Smartport FORMAT handler
static void smartport_format(unsigned char partition, unsigned char extended) {
  encode_init_reply_packet(devices[partition].device_id, extended, SP_SUCCESS); //just send back a successful response
  SendPacket( (unsigned char*) packet_buffer);
}

//Smartport INIT handler
static void smartport_init(unsigned char dev_id, unsigned char extended) {
  SP_Error status;

  devices[number_partitions_initialised].device_id = dev_id; //remember device id for partition
  number_partitions_initialised++;

  if (number_partitions_initialised < MAX_PARTITIONS) { //are all init'd yet
    status = SP_SUCCESS;
    if (number_partitions_initialised == 1) {
      init_storage();
      identifier = (dev_id & 0x7F) + '0';
    }
  } else {
    status = (DAISY_HDSEL_IS_LOW || force_next_smartport) ? SP_SUCCESS : SP_NODRIVE;
    device_init_done = 1;               //Mark init done
    number_partitions_initialised = 0;  // Reset variable for potential next INIT
  }

  DEBUG_CMD('I', dev_id, status == SP_SUCCESS);

  encode_init_reply_packet(dev_id, extended, status);
  SendPacket( (unsigned char*) packet_buffer);
}

//*****************************************************************************
// Function: main loop
// Parameters: none
// Returns: 0
//
// Description: Main function for Apple //c Smartport Compact Flash adpater
//*****************************************************************************
#pragma GCC optimize("-O3")

void loop() {
  unsigned char dev_id, r;
  SP_Command command;
  SP_State smartport_state;
  unsigned char partition, command_extended;

  SP_ACK_MUTE();
  SP_RD_MUTE();

  while (1) {
    if (device_init_done || never_got_reset) {
      daisy_ph3_mirror();
    }

    if (open_partitions == 0) {
      led_err();
    }

    smartport_state = smartport_get_state();
    partition = -1;

    switch (smartport_state) {
    case SP_BUS_RESET:
      daisy_ph3_disable();
      smartport_device_reset();
      device_init_done = 0;
      break;

    case SP_BUS_ENABLED:
      daisy_diskII_disable();

      // We can come back here after handling a previous smartport packet, as
      // it happens the bus stays enabled a bit longer. ReceivePacket() will
      // return with a non-zero code if the smartport bus gets disabled without
      // receiving anything.
      if ((r = ReceivePacket((unsigned char*) packet_buffer)) != 0) {
        break;
      }

      dev_id = packet_buffer[6];
      command = (SP_Command) packet_buffer[14];

      if ((command & ~COMMAND_EXTENDED_BIT) == SP_INIT && !device_init_done) {
        AckPacket();
      } else if ((partition = get_device_partition(dev_id)) != NO_PARTITION) {
        AckPacket();
      } else {
        DumpPacket(packet_buffer, 1);
        IgnorePacket();
        // Smartbus is now disabled, packet has been ignored. Loop back.
        break;
      }

      SET_LED_HIGH;

      command_extended = (command & COMMAND_EXTENDED_BIT) != 0;
      command = (SP_Command)(command & ~COMMAND_EXTENDED_BIT);
      switch (command) {
      case SP_INIT:
        //we may not have a dev_id/partition match yet
        smartport_init(dev_id, command_extended);
        break;

      case SP_STATUS:
        smartport_answer_status(partition, command_extended);
        break;

      case SP_READ:
        smartport_read_block(partition, command_extended);
        break;

      case SP_WRITE:
        smartport_write_block(partition, command_extended);
        break;

      case SP_FORMAT:
        smartport_format(partition, command_extended);
        break;

      default:
        LOGN(F("E:"), packet_buffer[14], HEX);
        encode_init_reply_packet(devices[partition].device_id, command_extended, SP_BADCMD);
        SendPacket( (unsigned char*) packet_buffer);
        break;
      }
      SET_LED_LOW;
      break;

    case SP_BUS_DISABLED:
      daisy_diskII_mirror();
      break;
    }

    // Clear packet for next time
    memset(packet_buffer, 0, 30);
    // I would have preferred to re-mute lines from the assembly funcs,
    // but this triggers a timing problem with SoftSP cards. Cf commit
    // 7b9ef299d98dfbd96c21fb1e522ccb702f9b7ec0
    SP_ACK_MUTE();
    SP_RD_MUTE();
  }
}
#pragma GCC optimize("-Os")

//*****************************************************************************
// Function: led_err
// Parameters: none
// Returns: nothing
//
// Description: Flashes status led to show error status
//*****************************************************************************

void led_err(void)
{
  static unsigned long last_switch = 0;
  static char last_state = 0;

  // Account for counter rollover
  if (millis() < last_switch) {
    last_switch = 0;
  }

  // Toggle led every 250ms
  if (millis() - last_switch > 250) {
    last_switch = millis();
    last_state = !last_state;
    if (last_state) {
      SET_LED_HIGH;
    } else {
      SET_LED_LOW;
    }
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

//Debug helper - dump a packet's start
static void DumpPacket(unsigned char *buffer, unsigned char type) {
  if (!debug) {
    return;
  }
  if (type == 1) {
    Serial.print(F("Ignored "));
  }
  if (type == 2) {
    Serial.print(F("Data: "));
  } else {
    Serial.print(F("Packet from: "));
    Serial.print(buffer[7], HEX);
    Serial.print(F(", To: "));
    Serial.print(buffer[6], HEX);
    Serial.print(F(", Command: "));
    Serial.print(buffer[14], HEX);
    Serial.println();
  }
  for (unsigned char i = 5; i < 35; i++) {
    Serial.print(' ');
    Serial.print(buffer[i], HEX);
  }
  Serial.println();
}

static void FullPacketDump(unsigned char decoded) {
  int i;
  if (decoded == 0) {
    Serial.print("Encoded packet:");
  } else {
    Serial.print("Decoded packet:");
  }
  for (i = 0; i < 606; i++) {
    if (i % 8 == 0) {
      Serial.println();
      Serial.print(i, DEC);
    }
    Serial.print(' ');
    Serial.print(packet_buffer[i], HEX);
  }
  Serial.println();
}

static void dump(char *str, unsigned char i) {
  Serial.print(str);
  Serial.print("= 0x");
  Serial.println(i, HEX);
}
