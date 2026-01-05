//*****************************************************************************
//
// Apple //c Smartport Compact Flash adapter
// Written by Robert Justice  email: rjustice(at)internode.on.net
//
// Packet encoding and decoding routines.
//
// MIT-licensed:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the “Software”), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Doc: http://www.applelogic.org/files/GSFIRMWAREREF1.pdf
//
//*****************************************************************************

#include <string.h>

extern unsigned char *packet_buffer;

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
  int count;
  signed char grpbyte, grpcount;
  unsigned char checksum = 0, grpmsb;
  unsigned char *src, *dst;

  // Calculate checksum of sector bytes before we destroy them
  for (count = 0; count < 512; count++) {
    // xor all the data bytes
    checksum ^= packet_buffer[count];
  }

  // Start assembling the packet at the rear and work
  // your way to the front so we don't overwrite data
  // we haven't encoded yet

  //grps of 7
  src = packet_buffer + 1 + (72*7);
  dst = packet_buffer + 17 + 6 + (72*8);
  for (grpcount = 72; grpcount >= 0; grpcount --) {
    grpmsb = 0;
    for (grpbyte = 6; grpbyte >= 0; grpbyte--) {
      // compute msb group byte
      grpmsb = grpmsb | ((src[grpbyte] >> (grpbyte + 1)) & (0x80 >> (grpbyte + 1)));
      // now add the group data bytes bits 6-0
      *dst-- = src[grpbyte] | 0x80;
    }
    *dst-- = grpmsb | 0x80;
    src -= 7;
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
    checksum ^= packet_buffer[count];
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
  int count;
  signed char grpbyte, grpcount;
  unsigned char checksum = 0, grpmsb;
  unsigned char *src, *dst;

    // Calculate checksum of sector bytes before we destroy them
    for (count = 0; count < 512; count++) {
      checksum = checksum ^ packet_buffer[count];
    }

  // Start assembling the packet at the rear and work
  // your way to the front so we don't overwrite data
  // we haven't encoded yet

  //grps of 7
  src = packet_buffer + 1 + (72*7);
  dst = packet_buffer + 17 + 6 + (72*8);
  for (grpcount = 72; grpcount >= 0; grpcount --) {
    grpmsb = 0;
    for (grpbyte = 6; grpbyte >= 0; grpbyte--) {
      // compute msb group byte
      grpmsb = grpmsb | ((src[grpbyte] >> (grpbyte + 1)) & (0x80 >> (grpbyte + 1)));
      // now add the group data bytes bits 6-0
      *dst-- = src[grpbyte] | 0x80;
    }
    *dst-- = grpmsb | 0x80;
    src -= 7;
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
#pragma GCC optimize("-O3")
int decode_data_packet (void)
{
  int grpbyte, count;
  unsigned char numgrps, numodd;
  unsigned char checksum = 0, bit0to6, bit7, oddbits, evenbits;
  unsigned char *src;
  unsigned int out_byte, num_final_bytes;

  //Handle arbitrary length packets :)
  numodd = packet_buffer[11] & 0x7f;
  numgrps = packet_buffer[12] & 0x7f;

  // First, checksum  packet header, because we're about to destroy it
  for (count = 6; count < 13; count++) {
    // now xor the packet header bytes
    checksum ^= packet_buffer[count];
  }

  evenbits = packet_buffer[599] & 0x55;
  oddbits = (packet_buffer[600] & 0x55 ) << 1;

  //add oddbyte(s), 1 in a 512 data packet
  for(int i = 0; i < numodd; i++) {
    packet_buffer[i] = ((packet_buffer[13] << (i+1)) & 0x80) | (packet_buffer[14+i] & 0x7f);
  }

  // 73 grps of 7 in a 512 byte packet
  src = packet_buffer + 15;
  num_final_bytes = 1 + (numgrps * 7);
  out_byte = 0;
  checksum ^= packet_buffer[out_byte];
  while (out_byte < num_final_bytes) {
    for (grpbyte = 1; grpbyte < 8; grpbyte++) {
      bit7 = (src[0] << grpbyte) & 0x80;
      bit0to6 = (src[grpbyte]) & 0x7f;
      out_byte++;
      packet_buffer[out_byte] = bit7 | bit0to6;
      if (out_byte < 512) {
        checksum ^= packet_buffer[out_byte];
      }
    }
    src += 8;
  }

  if (checksum == (oddbits | evenbits))
    return 0; //noerror
  else
    return 6; //smartport bus error code
}
#pragma GCC optimize("-Os")

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
  int count;
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
  int count;
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
void encode_status_reply_packet (unsigned char device_id, unsigned long blocks)
{
  int count;
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
  data[1] = blocks & 0xff;
  data[2] = (blocks >> 8 ) & 0xff;
  data[3] = (blocks >> 16 ) & 0xff;

  init_packet_buffer(device_id);
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
void encode_extended_status_reply_packet (unsigned char device_id, unsigned long blocks)
{
  int count;
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
  data[1] = blocks & 0xff;
  data[2] = (blocks >> 8 ) & 0xff;
  data[3] = (blocks >> 16 ) & 0xff;
  data[4] = (blocks >> 24 ) & 0xff;

  init_packet_buffer(device_id);
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
  int count;
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
void encode_status_dib_reply_packet (unsigned char device_id, unsigned long blocks)
{
  int grpbyte, grpcount, i, count;
  int grpnum, oddnum;
  unsigned char checksum = 0, grpmsb;
  unsigned char group_buffer[7];
  unsigned char data[25];
  //data buffer=25: 3 x Grp7 + 4 odds
  grpnum=3;
  oddnum=4;

  //* write data buffer first (25 bytes) 3 grp7 + 4 odds
  data[0] = 0xf8; //general status - f8 = 11111000
  //number of blocks =0x00ffff = 65525 or 32mb
  data[1] = blocks & 0xff; //block size 1
  data[2] = (blocks >> 8 ) & 0xff; //block size 2
  data[3] = (blocks >> 16 ) & 0xff ; //block size 3
  data[4] = 0x0a; //ID string length - 10 chars
  data[5] = 'B';
  data[6] = 'u';
  data[7] = 'r';
  data[8] = 'g';
  data[9] = 'e';
  data[10] = 'r';
  data[11] = 'D';
  data[12] = 'i';
  data[13] = 's';
  data[14] = 'k';
  data[15] = ' ';
  data[16] = ' ';
  data[17] = ' ';
  data[18] = ' ';
  data[19] = ' ';
  data[20] = ' ';  //ID string (16 chars total)
  data[21] = 0x02; //Device type    - 0x02  harddisk
  data[22] = 0x20; //Device Subtype - 0x0a
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

  init_packet_buffer(device_id);
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
void encode_extended_status_dib_reply_packet (unsigned char device_id, unsigned long blocks)
{
  int count;
  unsigned char checksum = 0;

  init_packet_buffer(device_id);
  packet_buffer[9] = 0x81;  //TYPE -status
  packet_buffer[10] = 0x80; //AUX
  packet_buffer[11] = 0x83; //STAT - data status
  packet_buffer[12] = 0x80; //ODDCNT - 4 data bytes
  packet_buffer[13] = 0x83; //GRP7CNT - 3 grps of 7
  packet_buffer[14] = 0xf0; //grp1 msb
  packet_buffer[15] = 0xf8; //general status - f8
  //number of blocks =0x00ffff = 65525 or 32mb
  packet_buffer[16] = blocks & 0xff; //block size 1
  packet_buffer[17] = (blocks >> 8 ) & 0xff; //block size 2
  packet_buffer[18] = ((blocks >> 16 ) & 0xff) | 0x80 ; //block size 3 - why is the high bit set?
  packet_buffer[19] = ((blocks >> 24 ) & 0xff) | 0x80 ; //block size 4 - why is the high bit set?
  packet_buffer[20] = 0x8a; //ID string length - 10 chars
  packet_buffer[21] = 'B';
  packet_buffer[22] = 'u';  //ID string (16 chars total)
  packet_buffer[23] = 0x80; //grp2 msb
  packet_buffer[24] = 'r';
  packet_buffer[25] = 'g';
  packet_buffer[26] = 'e';
  packet_buffer[27] = 'r';
  packet_buffer[28] = 'D';
  packet_buffer[29] = 'i';
  packet_buffer[30] = 's';
  packet_buffer[31] = 0x80; //grp3 msb
  packet_buffer[32] = 'k';
  packet_buffer[33] = ' ';
  packet_buffer[34] = ' ';
  packet_buffer[35] = ' ';
  packet_buffer[36] = ' ';
  packet_buffer[37] = ' ';
  packet_buffer[38] = ' ';
  packet_buffer[39] = 0x80; //odd msb
  packet_buffer[40] = 0x02; //Device type    - 0x02  harddisk
  packet_buffer[41] = 0x20; //Device Subtype - 0x20
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
