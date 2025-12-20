#ifndef __LOG
#define __LOG

void LOG(const __FlashStringHelper *str);
void LOG(const char *str);
void LOGN(const __FlashStringHelper *str, int num, int base);

#define DEBUG(str)             do { if (debug) LOG(str);             } while (0)
#define DEBUGN(str, num, base) do { if (debug) LOGN(str, num, base); } while (0)

#endif
