#ifndef __SP_PINS
#define __SP_PINS

#define PIN_PH0         2       // D2
#define PIN_PH1         3       // D3
#define PIN_PH2         4       // D4
#define PIN_PH3         5       // D5

#define PINS_PHASES     (_BV(PIN_PH0)|_BV(PIN_PH1)|_BV(PIN_PH2)|_BV(PIN_PH3))
#define WR_PORT_PHASES  PORTD
#define RD_PORT_PHASES  PIND

#define WR_PORT_RD      PORTD   // Define the PORT to RD                 - D0 to D7
#define RD_PORT_RD      PIND
#define DIR_PORT_RD     DDRD
#define PIN_RD          6       // D6
#define SET_RD_HIGH     (WR_PORT_RD |= _BV(PIN_RD))
#define SET_RD_LOW      (WR_PORT_RD &= ~_BV(PIN_RD))
#define SET_RD_OUT      (DIR_PORT_RD |= _BV(PIN_RD))
#define SET_RD_IN       (DIR_PORT_RD &= ~(_BV(PIN_RD)))

#define WR_PORT_WR      PORTD   // Define the PORT to WR                 - D0 to D7
#define RD_PORT_WR      PIND
#define DIR_PORT_WR     DDRD
#define PIN_WR          7       // D7
#define SET_WR_HIGH     (WR_PORT_WR |= _BV(PIN_WR))
#define SET_WR_LOW      (WR_PORT_WR &= ~_BV(PIN_WR))
#define SET_WR_OUT      (DIR_PORT_WR |= _BV(PIN_WR))
#define SET_WR_IN       (DIR_PORT_WR &= ~(_BV(PIN_WR)))

#define WR_PORT_REQ     PORTD   // Define the PORT to REQ                - D0 to D7
#define RD_PORT_REQ     PIND
#define DIR_PORT_REQ    DDRD
#define PIN_REQ         PIN_PH0    // Define the PIN number to REQ (PH0)    - PORTD bit2: D2
#define REQ_IS_HIGH     (RD_PORT_REQ & _BV(PIN_REQ))
#define REQ_IS_LOW      (!(REQ_IS_HIGH))
#define WAIT_REQ_LOW    while (REQ_IS_HIGH)

//WRPROT on A5
#define WR_PORT_ACK     PORTC   // Define the PORT to ACK                - A0 to A5
#define RD_PORT_ACK     PINC
#define DIR_PORT_ACK    DDRC
#define PIN_WRPROT      5       // A5 (portC)
#define PIN_ACK         PIN_WRPROT // Define the PIN number to ACK (WRPROT) - PORTC bit5: A5
#define SET_ACK_HIGH    (WR_PORT_ACK |= _BV(PIN_ACK))
#define SET_ACK_LOW     (WR_PORT_ACK &= ~_BV(PIN_ACK))
#define SET_ACK_OUT     (DIR_PORT_ACK |= _BV(PIN_ACK))
#define SET_ACK_IN      (DIR_PORT_ACK &= ~(_BV(PIN_ACK)))

#define ACK_IS_HIGH     (RD_PORT_ACK & _BV(PIN_ACK))
#define ACK_IS_LOW      (!(ACK_IS_HIGH))
#define WAIT_ACK_HIGH   while (ACK_IS_LOW)
#define WAIT_ACK_LOW    while (ACK_IS_HIGH)

#endif
