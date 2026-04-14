#ifndef __PACKET_CODECS_H
#define __PACKET_CODECS_H

#include "sp_vals.h"

extern "C" {

void encode_data_packet (unsigned char source, unsigned char extended, SP_Error status);   //encode smartport 512 byte data packet
unsigned char decode_data_packet (unsigned char extended);                   //decode smartport 512 byte data packet
void encode_write_status_packet(unsigned char source, unsigned char extended, SP_Error status);
void encode_init_reply_packet (unsigned char source, SP_Error status);
void encode_status_reply_packet (unsigned char device_id, unsigned long blocks);
void encode_extended_status_reply_packet (unsigned char device_id, unsigned long blocks);
void encode_status_dib_reply_packet (unsigned char device_id, unsigned long blocks);
void encode_extended_status_dib_reply_packet (unsigned char device_id, unsigned long blocks);

}

#endif
