#ifndef __SP_PINS
#define __SP_PINS

#define PIN_PH0         2       // D2
#define PIN_PH1         3       // D3
#define PIN_PH2         4       // D4
#define PIN_PH3         5       // D5

#define PINS_PHASES     (_BV(PIN_PH0)|_BV(PIN_PH1)|_BV(PIN_PH2)|_BV(PIN_PH3))
#define WR_PORT_PHASES  PORTD
#define RD_PORT_PHASES  PIND

#define PIN_RD          6       // D6
#define PIN_WR          7       // D7

#define WR_PORT_REQ     PORTD   // Define the PORT to REQ                - D0 to D7
#define RD_PORT_REQ     PIND
#define DIR_PORT_REQ    DDRD
#define PIN_REQ         PIN_PH0    // Define the PIN number to REQ (PH0)    - PORTD bit2: D2

//WRPROT on A5
#define WR_PORT_ACK     PORTC   // Define the PORT to ACK                - A0 to A5
#define RD_PORT_ACK     PINC
#define DIR_PORT_ACK    DDRC
#define PIN_WRPROT      5       // A5 (portC)
#define PIN_ACK         PIN_WRPROT // Define the PIN number to ACK (WRPROT) - PORTC bit5: A5

#endif
