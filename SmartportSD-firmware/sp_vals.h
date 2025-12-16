#ifndef __SP_VALS
#define __SP_VALS

typedef enum _SP_State {
  SP_BOOTING,
  SP_BUS_RESET,
  SP_ENABLED,
  SP_DISABLED
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

#endif
