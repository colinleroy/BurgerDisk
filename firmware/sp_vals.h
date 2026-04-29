#ifndef __SP_VALS
#define __SP_VALS

typedef enum _SP_State {
  SP_BUS_DISABLED,
  SP_BUS_RESET,
  SP_BUS_ENABLED
} SP_State;

typedef enum _SP_Command {
  SP_NO_COMMAND = 0x00,

  SP_STATUS     = 0x00,
  SP_READ       = 0x01,
  SP_WRITE      = 0x02,
  SP_FORMAT     = 0x03,
  SP_INIT       = 0x05
} SP_Command;

typedef enum _SP_Status_Code {
  SP_STATUS_SIMPLE = 0x00,
  SP_STATUS_DCB    = 0x01,
  SP_STATUS_NLSTAT = 0x02,
  SP_STATUS_DIB    = 0x03
} SP_Status_Code;

typedef enum _SP_Error {
  SP_SUCCESS    = 0x00,
  SP_BADCMD     = 0x01,
  SP_BUSERR     = 0x06,
  SP_BADCTL     = 0x21,
  SP_IOERROR    = 0x27,
  SP_NODRIVE    = 0x2B,
  SP_BADBLOCK   = 0x2D,
  SP_OFFLINE    = 0x2F
} SP_Error;

typedef enum _SP_Packet_byte {
  PACKET_SYNC1 = 0x00,
  PACKET_SYNC2,
  PACKET_SYNC3,
  PACKET_SYNC4,
  PACKET_SYNC5,
  PACKET_BEGIN,
  PACKET_DEST,
  PACKET_SOURCE,
  PACKET_TYPE,
  PACKET_AUXTYPE,
  PACKET_DATA_STATUS,
  PACKET_NUM_ODD,
  PACKET_NUM_GRP,
  PACKET_DATA_START,
} SP_Packet_Byte;

#define PACKET_COMMAND       (PACKET_DATA_START)
#define PACKET_NUM_PARAMS    (PACKET_DATA_START+1)

#define PACKET_STATCODE_TYPE (PACKET_DATA_START+4)

#define PACKET_BLOCK_NUM_L   (PACKET_DATA_START+4)
#define PACKET_BLOCK_NUM_M   (PACKET_DATA_START+5)
#define PACKET_BLOCK_NUM_H   (PACKET_DATA_START+6)

extern unsigned char identifier;

#endif
