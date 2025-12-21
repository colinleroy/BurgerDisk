#ifndef __SP_LOW
#define __SP_LOW

extern "C" unsigned char ReceivePacket(unsigned char*); //Receive smartport packet assembler function
extern "C" void AckPacket(void);
extern "C" unsigned char SendPacket(unsigned char*);    //send smartport packet assembler function

extern "C" void SP_ACK_MUTE(void);
extern "C" void SP_ACK_ON(void);
extern "C" void SP_ACK_OFF(void);
extern "C" void SP_RD_MUTE(void);

#endif
