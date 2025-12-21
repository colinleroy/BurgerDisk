//*****************************************************************************
//
// Apple //c Smartport Compact Flash adapter
// Written by Robert Justice  email: rjustice(at)internode.on.net
// Ported to Arduino UNO with SD Card adapter by Andrea Ottaviani email: andrea.ottaviani.69(at)gmail.com
////
// Apple disk interface is connected as follows:
// wrprot = pa5 (ack) (output)
// ph0    = pd2 (req) (input)
// ph1    = pd3       (input)
// ph2    = pd4       (input)
// ph3    = pd5       (input)
// rddata = pd6       (output from avr)
// wrdata = pd7       (input to avr)
//
//led i/o = pa4  (for led on when i/o on boxed version)
//
//
// Serial port was connected for debug purposes. Most of the prints have been commented out.
// I left these in and these can be uncommented as required for debugging. Sometimes the
// prints add to much delay, so you need to be carefull when adding these in.
//
// NOTE: This is uses the ata code from the fat/fat32/ata drivers written by
//       Angelo Bannack and Giordano Bruno Wolaniuk and ported to mega32 by Murray Horn.
//
//*****************************************************************************

//x #include <SPI.h>
#include "SdFat.h" // SDFat version 2.1.2
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "sp_pins.h"
#include "sp_low.h"
#include "sp_vals.h"
#include "log.h"
#include "daisy.h"

#define PIN_CHIP_SELECT 10      // D10
#define PIN_LED         18      // A4

#define MAX_PARTITIONS 4
int open_partitions = 0;

int debug = 0;

void encode_data_packet (unsigned char source);   //encode smartport 512 byte data packet
int  decode_data_packet (void);                   //decode smartport 512 byte data packet
void encode_write_status_packet(unsigned char source, unsigned char status);
void encode_init_reply_packet (unsigned char source, unsigned char status);
void encode_status_reply_packet (struct device d);
int  packet_length (void);
int partition;
bool is_valid_image(File imageFile);

//unsigned char packet_buffer[768];   //smartport packet buffer
//unsigned char packet_buffer[605];   //smartport packet buffer
unsigned char *packet_buffer;         //Wing
//unsigned char sector_buffer[512];   //ata sector data buffer
unsigned char status, packet_byte;
int count;

static void TIMESTAMP(void) {
  Serial.print(micros());
  Serial.print(F("µs - "));
}

void LOG(const __FlashStringHelper *str) {
  TIMESTAMP();
  Serial.println(str);
}

void LOG(const char *str) {
  TIMESTAMP();
  Serial.println(str);
}

void LOGN(const __FlashStringHelper *str, int num, int base) {
  TIMESTAMP();
  Serial.print(str);
  Serial.println(num, base);
}

// We need to remember several things about a device, not just its ID
struct device{
  File sdf;
  unsigned char device_id;              //to hold assigned device id's for the partitions
  unsigned long blocks;                 //how many 512-byte blocks this image has
  unsigned int header_offset;           //Some image files have headers, skip this many bytes to avoid them
  //bool online;                          //Whether this image is currently available
                                          //No longer used, user devices[...].sdf.isOpen() instead
  bool writeable;
};

device devices[MAX_PARTITIONS];

//The circuit:
//    SD card attached to SPI bus as follows:
// ** MOSI - pin 11 on Arduino Uno/Duemilanove/Diecimila
// ** MISO - pin 12 on Arduino Uno/Duemilanove/Diecimila
// ** CLK - pin 13 on Arduino Uno/Duemilanove/Diecimila
// ** CS - depends on your SD card shield or module.
//     Pin 10 used here

// Change the value of PIN_CHIP_SELECT if your hardware does
// not use the default value, SS.  Common values are:
// Arduino Ethernet shield: pin 4
// Sparkfun SD shield: pin 8
// Adafruit SD shields and modules: pin 10

/*
   Set DISABLE_CHIP_SELECT to disable a second SPI device.
   For example, with the Ethernet shield, set DISABLE_CHIP_SELECT
   to 10 to disable the Ethernet controller.
*/
const int8_t DISABLE_CHIP_SELECT = 0;

SdFat sdcard;

//------------------------------------------------------------------------------


static void log_io_err(const __FlashStringHelper *op, int partition, int block_num) {
  Serial.print(op);
  Serial.print(F(" error on partition "));
  Serial.print(partition);
  Serial.print(F(", block "));
  Serial.println(block_num);
}

static int storage_init_done = 0;
static int device_init_done = 0;

static void init_storage(void) {
  if (storage_init_done) {
    return;
  }
  DEBUGN(F("Free memory before opening images: "), freeMemory(), DEC);

  // Not enough RAM for SDFat to open a file if the packet_buffer
  // is allocated.
  free(packet_buffer);

  if (!sdcard.begin(PIN_CHIP_SELECT, SPI_HALF_SPEED)) {
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
    Serial.println(F("No config.txt. Searching for images."));
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

void setup (void) {
  // LED
  digitalWrite(PIN_LED, HIGH);
  pinMode(PIN_LED, OUTPUT);

  // Serial init
  Serial.begin(230400);
  LOG(F("SmartportSD v1.18"));

  packet_buffer = (unsigned char *)malloc(605);

  /* Main pins on upstream port */
  SET_WR_HIGH; SET_WR_IN;     //WR: input pullup
  SET_DRV1_IN;                //DRV1 & 2: input
  SET_DRV2_IN;

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

  digitalWrite(PIN_LED, LOW);
}

static int get_device_partition(int device_id) {
  for  (int p = 0; p < MAX_PARTITIONS; p++) {
    if (devices[p].device_id == device_id) {
      return p;
    }
  }
  return -1;
}

static inline SP_State smartport_get_state(void) {
  if (PHASES_BUS_RESET) {
    return SP_BUS_RESET;
  }
  if (PHASES_BUS_ENABLE) {
    return SP_BUS_ENABLED;
  }
  return SP_BUS_DISABLED;
}

int number_partitions_initialised = 0;

static void smartport_device_reset(void) {
  LOG(F("SP BUS RESET"));

  //reset number of partitions init'd
  number_partitions_initialised = 0;
  device_init_done = 0;

  //clear device_id table
  for (partition = 0; partition < MAX_PARTITIONS; partition++) {
    devices[partition].device_id = 0;
  }

  daisy_diskII_disable();

  // Ready. Wait for reset to clear
  while(smartport_get_state() == SP_BUS_RESET);
}

static void smartport_answer_status(unsigned char dev_id, unsigned char extended) {
  unsigned char status_code;
  DEBUG(F("SP STATUS"));

  partition = get_device_partition(dev_id);
  if (partition != -1 && devices[partition].sdf.isOpen()) {
    status_code = (packet_buffer[extended ? 21 : 19] & 0x7f);

    // if statcode=3, then status with device info block
    if (status_code == 0x03) {
      if (extended) {
        LOG(F("Extended status DIB - Not implemented!"));
      } else {
        DEBUG(F("Sending DIB"));
        encode_status_dib_reply_packet(devices[partition]);
        delay(50);
      }
    } else {
      // just return device status
      if (extended) {
        encode_extended_status_reply_packet(devices[partition]);
      } else {
        encode_status_reply_packet(devices[partition]);
      }
    }
    status = SendPacket( (unsigned char*) packet_buffer);
  }
}

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

static void smartport_read_block(unsigned char dev_id) {
  unsigned long int block_num;

  partition = get_device_partition(dev_id);
  if (partition != -1) {  //yes it is, then do the read
    block_num = smartport_get_block_num_from_buf();

    if(devices[partition].sdf.isOpen()) {
      if (!devices[partition].sdf.seekSet(block_num*512+devices[partition].header_offset)) {
        log_io_err(F("Seek"), partition, block_num);
      }
    }

    //Read block from SD Card
    if (!devices[partition].sdf.read((unsigned char*) packet_buffer, 512)) {
      log_io_err(F("Read"), partition, block_num);
    }
    encode_data_packet(dev_id);

    status = SendPacket( (unsigned char*) packet_buffer);
  }
}

static void smartport_write_block(unsigned char dev_id) {
  unsigned long int block_num;

  partition = get_device_partition(dev_id);
  if (partition != -1) {  //yes it is, then do the write
    block_num = smartport_get_block_num_from_buf();

    //get write data packet
    ReceivePacket( (unsigned char*) packet_buffer);
    AckPacket();
    status = decode_data_packet();
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
    encode_write_status_packet(dev_id, status);
    status = SendPacket( (unsigned char*) packet_buffer);
  }
}

static void smartport_format(unsigned char dev_id) {
  partition = get_device_partition(dev_id);
  if (partition != -1) {  //yes it is, then do the read
    encode_init_reply_packet(dev_id, 0x80); //just send back a successful response
    status = SendPacket( (unsigned char*) packet_buffer);
  }
}

static void smartport_init(unsigned char dev_id) {
  LOG(F("SP INIT"));
  devices[number_partitions_initialised].device_id = dev_id; //remember device id for partition
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

  DEBUGN(F("Init device id: "), dev_id, HEX);
  DEBUGN(F("More devices: "), status == 0x80, HEX);
  status = SendPacket( (unsigned char*) packet_buffer);
}

static void IgnorePacket(unsigned char dev_id) {
  SP_ACK_MUTE();
  SP_RD_MUTE();
  while(smartport_get_state() == SP_BUS_ENABLED);
  interrupts();
  DEBUGN(F("Ignored packet for "), dev_id, HEX);
}

static void DumpPacket(void) {
  Serial.print(F("Packet from: "));
  Serial.print(packet_buffer[7], HEX);
  Serial.print(F(", To: "));
  Serial.print(packet_buffer[6], HEX);
  Serial.print(F(", Command: "));
  Serial.print(packet_buffer[14], HEX);
  Serial.println();
  for (int i = 0; i < 30; i++) {
    Serial.print(' ');
    Serial.print(packet_buffer[i], HEX);
  }
  Serial.println();
}

//*****************************************************************************
// Function: main loop
// Parameters: none
// Returns: 0
//
// Description: Main function for Apple //c Smartport Compact Flash adpater
//*****************************************************************************
void loop() {
  unsigned char dev_id;
  SP_Command command;
  SP_State smartport_state;

  SP_ACK_MUTE();
  SP_RD_MUTE();

  // read phase lines to check for smartport reset or enable
  smartport_state = smartport_get_state();

  switch (smartport_state) {
  case SP_BUS_RESET:
    daisy_ph3_disable();
    smartport_device_reset();
    break;

  case SP_BUS_ENABLED:
    daisy_diskII_disable();
    ReceivePacket( (unsigned char*) packet_buffer);
    dev_id = packet_buffer[6];
    //source_id = packet_buffer[7];
    command = (SP_Command) packet_buffer[14];

    if (command == SP_INIT && !device_init_done) {
      AckPacket();
    } else if (get_device_partition(dev_id) != -1) {
      AckPacket();
    } else {
      IgnorePacket(dev_id);
      break;
    }

    if (debug) {
      DumpPacket();
    }

    digitalWrite(PIN_LED, HIGH);

    switch (command) {
    case SP_INIT:
      smartport_init(dev_id);
      break;

    case SP_STATUS:
      smartport_answer_status(dev_id, 0);
      break;

    case SP_EXT_STATUS:
      smartport_answer_status(dev_id, 1);
      break;

    case SP_EXT_READ:
    case SP_READ:
      smartport_read_block(dev_id);
      break;

    case SP_WRITE:
      smartport_write_block(dev_id);
      break;

    case SP_FORMAT:
      smartport_format(dev_id);
      break;

    case SP_EXT_WRITE:
    case SP_EXT_FORMAT:
    case SP_EXT_INIT:
    default:
      LOGN(F("Command not implemented: "), packet_buffer[14], HEX);
      break;
    }
    digitalWrite(PIN_LED, LOW);

    break;

  case SP_BOOTING:
  case SP_BUS_DISABLED:
    daisy_diskII_mirror();
    break;
  }

  if (device_init_done) {
    daisy_ph3_mirror();
  }
}

static void init_packet_buffer(unsigned char source) {
  packet_buffer[0] = 0xff;  //sync bytes
  packet_buffer[1] = 0x3f;
  packet_buffer[2] = 0xcf;
  packet_buffer[3] = 0xf3;
  packet_buffer[4] = 0xfc;
  packet_buffer[5] = 0xff;
  packet_buffer[6] = 0xc3;  //PBEGIN - start byte
  packet_buffer[7] = 0x80;  //DEST - dest id - host
  packet_buffer[8] = source; //SRC - source id - us
}

//*****************************************************************************
// Function: encode_data_packet
// Parameters: source id
// Returns: none
//
// Description: encode 512 byte data packet for read block command from host
// requires the data to be in the packet buffer, and builds the smartport
// packet IN PLACE in the packet buffer
//*****************************************************************************
void encode_data_packet (unsigned char source)
{
  int grpbyte, grpcount;
  unsigned char checksum = 0, grpmsb;
  unsigned char group_buffer[7];

  // Calculate checksum of sector bytes before we destroy them
    for (count = 0; count < 512; count++) // xor all the data bytes
    checksum = checksum ^ packet_buffer[count];

  // Start assembling the packet at the rear and work
  // your way to the front so we don't overwrite data
  // we haven't encoded yet

  //grps of 7
  for (grpcount = 72; grpcount >= 0; grpcount--) //73
  {
    memcpy(group_buffer, packet_buffer + 1 + (grpcount * 7), 7);
    // add group msb byte
    grpmsb = 0;
    for (grpbyte = 0; grpbyte < 7; grpbyte++)
      grpmsb = grpmsb | ((group_buffer[grpbyte] >> (grpbyte + 1)) & (0x80 >> (grpbyte + 1)));
    packet_buffer[16 + (grpcount * 8)] = grpmsb | 0x80; // set msb to one

    // now add the group data bytes bits 6-0
    for (grpbyte = 0; grpbyte < 7; grpbyte++)
      packet_buffer[17 + (grpcount * 8) + grpbyte] = group_buffer[grpbyte] | 0x80;

  }

  //total number of packet data bytes for 512 data bytes is 584
  //odd byte
  packet_buffer[14] = ((packet_buffer[0] >> 1) & 0x40) | 0x80;
  packet_buffer[15] = packet_buffer[0] | 0x80;

  init_packet_buffer(source);
  packet_buffer[9] = 0x82;  //TYPE - 0x82 = data
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = 0x80; //STAT
  packet_buffer[12] = 0x81; //ODDCNT  - 1 odd byte for 512 byte packet
  packet_buffer[13] = 0xC9; //GRP7CNT - 73 groups of 7 bytes for 512 byte packet

  // xor the packet header bytes
  for (count = 7; count < 14; count++) {
    checksum = checksum ^ packet_buffer[count];
  }

  packet_buffer[600] = checksum | 0xaa;      // 1 c6 1 c4 1 c2 1 c0
  packet_buffer[601] = checksum >> 1 | 0xaa; // 1 c7 1 c5 1 c3 1 c1

  //end bytes
  packet_buffer[602] = 0xc8;  //pkt end
  packet_buffer[603] = 0x00;  //mark the end of the packet_buffer

}

//*****************************************************************************
// Function: encode_extended_data_packet
// Parameters: source id
// Returns: none
//
// Description: encode 512 byte data packet for read block command from host
// requires the data to be in the packet buffer, and builds the smartport
// packet IN PLACE in the packet buffer
//*****************************************************************************
void encode_extended_data_packet (unsigned char source)
{
  int grpbyte, grpcount;
  unsigned char checksum = 0, grpmsb;
  unsigned char group_buffer[7];

    // Calculate checksum of sector bytes before we destroy them
    for (count = 0; count < 512; count++) {
      checksum = checksum ^ packet_buffer[count];
    }

  // Start assembling the packet at the rear and work
  // your way to the front so we don't overwrite data
  // we haven't encoded yet

  //grps of 7
  for (grpcount = 72; grpcount >= 0; grpcount--) //73
  {
    memcpy(group_buffer, packet_buffer + 1 + (grpcount * 7), 7);
    // add group msb byte
    grpmsb = 0;
    for (grpbyte = 0; grpbyte < 7; grpbyte++) {
      grpmsb = grpmsb | ((group_buffer[grpbyte] >> (grpbyte + 1)) & (0x80 >> (grpbyte + 1)));
    }
    packet_buffer[16 + (grpcount * 8)] = grpmsb | 0x80; // set msb to one

    // now add the group data bytes bits 6-0
    for (grpbyte = 0; grpbyte < 7; grpbyte++) {
      packet_buffer[17 + (grpcount * 8) + grpbyte] = group_buffer[grpbyte] | 0x80;
    }
  }

  //total number of packet data bytes for 512 data bytes is 584
  //odd byte
  packet_buffer[14] = ((packet_buffer[0] >> 1) & 0x40) | 0x80;
  packet_buffer[15] = packet_buffer[0] | 0x80;

  init_packet_buffer(source);
  packet_buffer[9] = 0xC2;  //TYPE - 0xC2 = extended data
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = 0x80; //STAT
  packet_buffer[12] = 0x81; //ODDCNT  - 1 odd byte for 512 byte packet
  packet_buffer[13] = 0xC9; //GRP7CNT - 73 groups of 7 bytes for 512 byte packet

  // now xor the packet header bytes
  for (count = 7; count < 14; count++) {
    checksum = checksum ^ packet_buffer[count];
  }
  packet_buffer[600] = checksum | 0xaa;      // 1 c6 1 c4 1 c2 1 c0
  packet_buffer[601] = checksum >> 1 | 0xaa; // 1 c7 1 c5 1 c3 1 c1

  //end bytes
  packet_buffer[602] = 0xc8;  //pkt end
  packet_buffer[603] = 0x00;  //mark the end of the packet_buffer

}


//*****************************************************************************
// Function: decode_data_packet
// Parameters: none
// Returns: error code, > 0 = error encountered
//
// Description: decode 512 byte data packet for write block command from host
// decodes the data from the packet_buffer IN-PLACE!
//*****************************************************************************
int decode_data_packet (void)
{
  int grpbyte, grpcount;
  unsigned char numgrps, numodd;
  unsigned char checksum = 0, bit0to6, bit7, oddbits, evenbits;
  unsigned char group_buffer[8];

  //Handle arbitrary length packets :)
  numodd = packet_buffer[11] & 0x7f;
  numgrps = packet_buffer[12] & 0x7f;

  // First, checksum  packet header, because we're about to destroy it
  for (count = 6; count < 13; count++) // now xor the packet header bytes
  checksum = checksum ^ packet_buffer[count];

  evenbits = packet_buffer[599] & 0x55;
  oddbits = (packet_buffer[600] & 0x55 ) << 1;

  //add oddbyte(s), 1 in a 512 data packet
  for(int i = 0; i < numodd; i++) {
    packet_buffer[i] = ((packet_buffer[13] << (i+1)) & 0x80) | (packet_buffer[14+i] & 0x7f);
  }

  // 73 grps of 7 in a 512 byte packet
  for (grpcount = 0; grpcount < numgrps; grpcount++)
  {
    memcpy(group_buffer, packet_buffer + 15 + (grpcount * 8), 8);
    for (grpbyte = 0; grpbyte < 7; grpbyte++) {
      bit7 = (group_buffer[0] << (grpbyte + 1)) & 0x80;
      bit0to6 = (group_buffer[grpbyte + 1]) & 0x7f;
      packet_buffer[1 + (grpcount * 7) + grpbyte] = bit7 | bit0to6;
    }
  }

  //verify checksum
  for (count = 0; count < 512; count++) {
    checksum = checksum ^ packet_buffer[count];
  }

  if (checksum == (oddbits | evenbits))
    return 0; //noerror
  else
    return 6; //smartport bus error code

}

//*****************************************************************************
// Function: encode_write_status_packet
// Parameters: source,status
// Returns: none
//
// Description: this is the reply to the write block data packet. The reply
// indicates the status of the write block cmd.
//*****************************************************************************
void encode_write_status_packet(unsigned char source, unsigned char status)
{
  unsigned char checksum = 0;

  init_packet_buffer(source);
  packet_buffer[9] = 0x81;  //TYPE
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = status | 0x80; //STAT
  packet_buffer[12] = 0x80; //ODDCNT
  packet_buffer[13] = 0x80; //GRP7CNT

  // xor the packet header bytes
  for (count = 7; count < 14; count++) {
    checksum = checksum ^ packet_buffer[count];
  }

  packet_buffer[14] = checksum | 0xaa;      // 1 c6 1 c4 1 c2 1 c0
  packet_buffer[15] = checksum >> 1 | 0xaa; // 1 c7 1 c5 1 c3 1 c1

  packet_buffer[16] = 0xc8;  //pkt end
  packet_buffer[17] = 0x00;  //mark the end of the packet_buffer

}

//*****************************************************************************
// Function: encode_init_reply_packet
// Parameters: source
// Returns: none
//
// Description: this is the reply to the init command packet. A reply indicates
// the original dest id has a device on the bus. If the STAT byte is 0, (0x80)
// then this is not the last device in the chain. This is written to support up
// to 4 partions, i.e. devices, so we need to specify when we are doing the last
// init reply.
//*****************************************************************************
void encode_init_reply_packet (unsigned char source, unsigned char status)
{
  unsigned char checksum = 0;

  init_packet_buffer(source);
  packet_buffer[9] = 0x80;  //TYPE
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = status; //STAT - data status

  packet_buffer[12] = 0x80; //ODDCNT
  packet_buffer[13] = 0x80; //GRP7CNT

  // xor the packet header bytes
  for (count = 7; count < 14; count++) {
    checksum = checksum ^ packet_buffer[count];
  }
  packet_buffer[14] = checksum | 0xaa;      // 1 c6 1 c4 1 c2 1 c0
  packet_buffer[15] = checksum >> 1 | 0xaa; // 1 c7 1 c5 1 c3 1 c1

  packet_buffer[16] = 0xc8; //PEND
  packet_buffer[17] = 0x00; //end of packet in buffer

}

//*****************************************************************************
// Function: encode_status_reply_packet
// Parameters: source
// Returns: none
//
// Description: this is the reply to the status command packet. The reply
// includes following:
// data byte 1 is general info.
// data byte 2-4 number of blocks. 2 is the LSB and 4 the MSB.
// Size determined from image file.
//*****************************************************************************
void encode_status_reply_packet (device d)
{

  unsigned char checksum = 0;
  unsigned char data[4];

  //Build the contents of the packet
  //Info byte
  //Bit 7: Block  device
  //Bit 6: Write allowed
  //Bit 5: Read allowed
  //Bit 4: Device online or disk in drive
  //Bit 3: Format allowed
  //Bit 2: Media write protected
  //Bit 1: Currently interrupting (//c only)
  //Bit 0: Currently open (char devices only)
  data[0] = 0b11111000;
  //Disk size
  data[1] = d.blocks & 0xff;
  data[2] = (d.blocks >> 8 ) & 0xff;
  data[3] = (d.blocks >> 16 ) & 0xff;

  init_packet_buffer(d.device_id);
  packet_buffer[9] = 0x81;  //TYPE -status
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = 0x80; //STAT - data status
  packet_buffer[12] = 0x84; //ODDCNT - 4 data bytes
  packet_buffer[13] = 0x80; //GRP7CNT
  //4 odd bytes
  packet_buffer[14] = 0x80 | ((data[0]>> 1) & 0x40) | ((data[1]>>2) & 0x20) | (( data[2]>>3) & 0x10) | ((data[3]>>4) & 0x08 ); //odd msb
  packet_buffer[15] = data[0] | 0x80; //data 1
  packet_buffer[16] = data[1] | 0x80; //data 2
  packet_buffer[17] = data[2] | 0x80; //data 3
  packet_buffer[18] = data[3] | 0x80; //data 4

  //calc the data bytes checksum
  for(int i = 0; i < 4; i++) {
    checksum ^= data[i];
  }

  // xor the packet header bytes
  for (count = 7; count < 14; count++) {
    checksum = checksum ^ packet_buffer[count];
  }
  packet_buffer[19] = checksum | 0xaa;      // 1 c6 1 c4 1 c2 1 c0
  packet_buffer[20] = checksum >> 1 | 0xaa; // 1 c7 1 c5 1 c3 1 c1

  packet_buffer[21] = 0xc8; //PEND
  packet_buffer[22] = 0x00; //end of packet in buffer

}


//*****************************************************************************
// Function: encode_long_status_reply_packet
// Parameters: source
// Returns: none
//
// Description: this is the reply to the extended status command packet. The reply
// includes following:
// data byte 1
// data byte 2-5 number of blocks. 2 is the LSB and 5 the MSB.
// Size determined from image file.
//*****************************************************************************
void encode_extended_status_reply_packet (device d)
{
  unsigned char checksum = 0;

  unsigned char data[5];

  //Build the contents of the packet
  //Info byte
  //Bit 7: Block  device
  //Bit 6: Write allowed
  //Bit 5: Read allowed
  //Bit 4: Device online or disk in drive
  //Bit 3: Format allowed
  //Bit 2: Media write protected
  //Bit 1: Currently interrupting (//c only)
  //Bit 0: Currently open (char devices only)
  data[0] = 0b11111000;
  //Disk size
  data[1] = d.blocks & 0xff;
  data[2] = (d.blocks >> 8 ) & 0xff;
  data[3] = (d.blocks >> 16 ) & 0xff;
  data[4] = (d.blocks >> 24 ) & 0xff;

  init_packet_buffer(d.device_id);
  packet_buffer[9] = 0xC1;  //TYPE - extended status
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = 0x80; //STAT - data status
  packet_buffer[12] = 0x85; //ODDCNT - 5 data bytes
  packet_buffer[13] = 0x80; //GRP7CNT
  //5 odd bytes
  packet_buffer[14] = 0x80 | ((data[0]>> 1) & 0x40) | ((data[1]>>2) & 0x20) | (( data[2]>>3) & 0x10) | ((data[3]>>4) & 0x08 ) | ((data[4] >> 5) & 0x04) ; //odd msb
  packet_buffer[15] = data[0] | 0x80; //data 1
  packet_buffer[16] = data[1] | 0x80; //data 2
  packet_buffer[17] = data[2] | 0x80; //data 3
  packet_buffer[18] = data[3] | 0x80; //data 4
  packet_buffer[19] = data[4] | 0x80; //data 5

  // calc the data bytes checksum
  for(int i = 0; i < 5; i++) {
    checksum ^= data[i];
  }

  //calc the data bytes checksum
  for (count = 7; count < 14; count++) {
    checksum = checksum ^ packet_buffer[count];
  }
  packet_buffer[20] = checksum | 0xaa;      // 1 c6 1 c4 1 c2 1 c0
  packet_buffer[21] = checksum >> 1 | 0xaa; // 1 c7 1 c5 1 c3 1 c1

  packet_buffer[22] = 0xc8; //PEND
  packet_buffer[23] = 0x00; //end of packet in buffer

}
void encode_error_reply_packet (unsigned char source)
{
  unsigned char checksum = 0;

  init_packet_buffer(source);
  packet_buffer[9] = 0x80;  //TYPE -status
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = 0xA1; //STAT - data status - error
  packet_buffer[12] = 0x80; //ODDCNT - 0 data bytes
  packet_buffer[13] = 0x80; //GRP7CNT

  // xor the packet header bytes
  for (count = 7; count < 14; count++) {
    checksum = checksum ^ packet_buffer[count];
  }

  packet_buffer[14] = checksum | 0xaa;      // 1 c6 1 c4 1 c2 1 c0
  packet_buffer[15] = checksum >> 1 | 0xaa; // 1 c7 1 c5 1 c3 1 c1

  packet_buffer[16] = 0xc8; //PEND
  packet_buffer[17] = 0x00; //end of packet in buffer

}

//*****************************************************************************
// Function: encode_status_dib_reply_packet
// Parameters: source
// Returns: none
//
// Description: this is the reply to the status command 03 packet. The reply
// includes following:
// data byte 1
// data byte 2-4 number of blocks. 2 is the LSB and 4 the MSB.
// Calculated from actual image file size.
//*****************************************************************************
void encode_status_dib_reply_packet (device d)
{
  int grpbyte, grpcount, i;
  int grpnum, oddnum;
  unsigned char checksum = 0, grpmsb;
  unsigned char group_buffer[7];
  unsigned char data[25];
  //data buffer=25: 3 x Grp7 + 4 odds
  grpnum=3;
  oddnum=4;

  //* write data buffer first (25 bytes) 3 grp7 + 4 odds
  data[0] = 0xf8; //general status - f8
  //number of blocks =0x00ffff = 65525 or 32mb
  data[1] = d.blocks & 0xff; //block size 1
  data[2] = (d.blocks >> 8 ) & 0xff; //block size 2
  data[3] = (d.blocks >> 16 ) & 0xff ; //block size 3
  data[4] = 0x0b; //ID string length - 11 chars
  data[5] = 'S';
  data[6] = 'M';
  data[7] = 'A';
  data[8] = 'R';
  data[9] = 'T';
  data[10] = 'P';
  data[11] = 'O';
  data[12] = 'R';
  data[13] = 'T';
  data[14] = 'S';
  data[15] = 'D';
  data[16] = ' ';
  data[17] = ' ';
  data[18] = ' ';
  data[19] = ' ';
  data[20] = ' ';  //ID string (16 chars total)
  data[21] = 0x02; //Device type    - 0x02  harddisk
  data[22] = 0x0a; //Device Subtype - 0x0a
  data[23] = 0x01; //Firmware version 2 bytes
  data[24] = 0x0f; //

  // Calculate checksum of sector bytes before we destroy them
  for (count = 0; count < 25; count++) {
    checksum = checksum ^ data[count];
  }

  // Start assembling the packet at the rear and work
  // your way to the front so we don't overwrite data
  // we haven't encoded yet

  //grps of 7
  for (grpcount = grpnum-1; grpcount >= 0; grpcount--) // 3
  {
    for (i = 0;i < 7;i++) {
      group_buffer[i]=data[i + oddnum + (grpcount * 7)];
    }
    // add group msb byte
    grpmsb = 0;
    for (grpbyte = 0; grpbyte < 7; grpbyte++) {
      grpmsb = grpmsb | ((group_buffer[grpbyte] >> (grpbyte + 1)) & (0x80 >> (grpbyte + 1)));
    }
    packet_buffer[(14 + oddnum + 1) + (grpcount * 8)] = grpmsb | 0x80; // set msb to one

    // now add the group data bytes bits 6-0
    for (grpbyte = 0; grpbyte < 7; grpbyte++) {
      packet_buffer[(14 + oddnum + 2) + (grpcount * 8) + grpbyte] = group_buffer[grpbyte] | 0x80;
    }
  }

  //odd byte
  packet_buffer[14] = 0x80 | ((data[0]>> 1) & 0x40) | ((data[1]>>2) & 0x20) | (( data[2]>>3) & 0x10) | ((data[3]>>4) & 0x08 ); //odd msb
  packet_buffer[15] = data[0] | 0x80;
  packet_buffer[16] = data[1] | 0x80;
  packet_buffer[17] = data[2] | 0x80;
  packet_buffer[18] = data[3] | 0x80;;

  init_packet_buffer(d.device_id);
  packet_buffer[9] = 0x81;  //TYPE -status
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = 0x80; //STAT - data status
  packet_buffer[12] = 0x84; //ODDCNT - 4 data bytes
  packet_buffer[13] = 0x83; //GRP7CNT - 3 grps of 7

  // xor the packet header bytes
  for (count = 7; count < 14; count++) {
    checksum = checksum ^ packet_buffer[count];
  }
  packet_buffer[43] = checksum | 0xaa;      // 1 c6 1 c4 1 c2 1 c0
  packet_buffer[44] = checksum >> 1 | 0xaa; // 1 c7 1 c5 1 c3 1 c1

  packet_buffer[45] = 0xc8; //PEND
  packet_buffer[46] = 0x00; //end of packet in buffer
}


//*****************************************************************************
// Function: encode_long_status_dib_reply_packet
// Parameters: source
// Returns: none
//
// Description: this is the reply to the status command 03 packet. The reply
// includes following:
// data byte 1
// data byte 2-5 number of blocks. 2 is the LSB and 5 the MSB.
// Calculated from actual image file size.
//*****************************************************************************
void encode_extended_status_dib_reply_packet (device d)
{
  unsigned char checksum = 0;

  init_packet_buffer(d.device_id);
  packet_buffer[9] = 0x81;  //TYPE -status
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = 0x83; //STAT - data status
  packet_buffer[12] = 0x80; //ODDCNT - 4 data bytes
  packet_buffer[13] = 0x83; //GRP7CNT - 3 grps of 7
  packet_buffer[14] = 0xf0; //grp1 msb
  packet_buffer[15] = 0xf8; //general status - f8
  //number of blocks =0x00ffff = 65525 or 32mb
  packet_buffer[16] = d.blocks & 0xff; //block size 1
  packet_buffer[17] = (d.blocks >> 8 ) & 0xff; //block size 2
  packet_buffer[18] = ((d.blocks >> 16 ) & 0xff) | 0x80 ; //block size 3 - why is the high bit set?
  packet_buffer[19] = ((d.blocks >> 24 ) & 0xff) | 0x80 ; //block size 4 - why is the high bit set?
  packet_buffer[20] = 0x8d; //ID string length - 13 chars
  packet_buffer[21] = 'S';
  packet_buffer[22] = 'm';  //ID string (16 chars total)
  packet_buffer[23] = 0x80; //grp2 msb
  packet_buffer[24] = 'a';
  packet_buffer[25] = 'r';
  packet_buffer[26] = 't';
  packet_buffer[27] = 'p';
  packet_buffer[28] = 'o';
  packet_buffer[29] = 'r';
  packet_buffer[30] = 't';
  packet_buffer[31] = 0x80; //grp3 msb
  packet_buffer[32] = ' ';
  packet_buffer[33] = 'S';
  packet_buffer[34] = 'D';
  packet_buffer[35] = ' ';
  packet_buffer[36] = ' ';
  packet_buffer[37] = ' ';
  packet_buffer[38] = ' ';
  packet_buffer[39] = 0x80; //odd msb
  packet_buffer[40] = 0x02; //Device type    - 0x02  harddisk
  packet_buffer[41] = 0x00; //Device Subtype - 0x20
  packet_buffer[42] = 0x01; //Firmware version 2 bytes
  packet_buffer[43]=  0x0f;
  packet_buffer[44] = 0x90; //

  // xor the packet bytes
  for (count = 7; count < 45; count++) {
    checksum = checksum ^ packet_buffer[count];
  }
  packet_buffer[45] = checksum | 0xaa;      // 1 c6 1 c4 1 c2 1 c0
  packet_buffer[46] = checksum >> 1 | 0xaa; // 1 c7 1 c5 1 c3 1 c1

  packet_buffer[47] = 0xc8; //PEND
  packet_buffer[48] = 0x00; //end of packet in buffer

}


//*****************************************************************************
// Function: print_packet
// Parameters: pointer to data, number of bytes to be printed
// Returns: none
//
// Description: prints packet data for debug purposes to the serial port
//*****************************************************************************
void print_packet (unsigned char* data, int bytes)
{
  int count, row;
  char tbs[8];
  char xx;

  Serial.print(F("\r\n"));
  for (count = 0; count < bytes; count = count + 16) {
    sprintf(tbs, "%04X: ", count);
    Serial.print(tbs);
    for (row = 0; row < 16; row++) {
      if (count + row >= bytes)
        Serial.print(F("   "));
      else {
        Serial.print(data[count + row], HEX);
        Serial.print(F(" "));
      }
    }
    Serial.print(F("-"));
    for (row = 0; row < 16; row++) {
      if ((data[count + row] > 31) && (count + row < bytes) && (data[count + row] < 129))
      {
        xx = data[count + row];
        Serial.print(xx);
      }
      else
        Serial.print(F("."));
    }
    Serial.print(F("\r\n"));
  }
}

//*****************************************************************************
// Function: packet_length
// Parameters: none
// Returns: length
//
// Description: Calculates the length of the packet in the packet_buffer.
// A zero marks the end of the packet data.
//*****************************************************************************
int packet_length (void)
{
  int x = 0;

  while (packet_buffer[x++]);
  return x - 1; // point to last packet byte = C8
}

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
  Serial.println(F("Require reboot"));

  while (1) {
    digitalWrite(PIN_LED, HIGH);
    delay(500);
    digitalWrite(PIN_LED, LOW);
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


// TODO: Allow image files with headers, too
// TODO: Respect read-only bit in header

bool open_image( device &d, String filename ){
  Serial.print(F("Testing file "));
  Serial.print(filename);

  d.sdf = sdcard.open(filename, O_RDWR);
  if (!d.sdf.isOpen()) {
    Serial.println(F(": Can not open."));
    return false;
  }
  if (!d.sdf.isFile()) {
    Serial.println(F(": Is a directory."));
    return false;
  }

  if (d.sdf.size() == 0) {
    Serial.println(F(": File is empty"));
    return false;
  }

  Serial.println(F(": OK"));
  d.blocks = d.sdf.size() >> 9;
  return true;
}
