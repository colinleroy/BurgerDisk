#ifndef __SP_VALS
#define __SP_VALS

typedef enum _SP_State {
  SP_BUS_DISABLED,
  SP_BUS_RESET,
  SP_BUS_ENABLED
} SP_State;

typedef enum _SP_Command {
  SP_STATUS     = 0x80,
  SP_READ       = 0x81,
  SP_WRITE      = 0x82,
  SP_FORMAT     = 0x83,
  SP_INIT       = 0x85,

  SP_EXT_STATUS = 0xC0,
  SP_EXT_READ   = 0xC1,
  SP_EXT_WRITE  = 0xC2,
  SP_EXT_FORMAT = 0xC3,
  SP_EXT_INIT   = 0xC5
} SP_Command;

typedef enum _SP_Error {
  SP_SUCCESS    = 0x00,
  SP_BUSERR     = 0x06,
  SP_IOERROR    = 0x27,
  SP_NODRIVE    = 0x2B,
  SP_BADBLOCK   = 0x2D,
  SP_OFFLINE    = 0x2F
} SP_Error;

extern unsigned char identifier;

#endif
