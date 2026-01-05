#ifndef __LOG
#define __LOG

#define LOG(str)               do { Serial.println(str);                          } while (0)
#define LOGN(str, num, base)   do { Serial.print(str); Serial.println(num, base); } while (0)
#define DEBUG(str)             do { if (debug) LOG(str);                          } while (0)
#define DEBUGN(str, num, base) do { if (debug) LOGN(str, num, base);              } while (0)
#define DEBUG_CMD(cmd, did, block) do { \
  if (debug) {                          \
    Serial.write(cmd);                  \
    Serial.print(did, HEX);             \
    Serial.write('/');                  \
    Serial.println(block, DEC);         \
  }                                     \
} while (0)

#endif
