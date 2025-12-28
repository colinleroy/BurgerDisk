#ifndef __SP_PINS
#define __SP_PINS

#include <avr/io.h>

// Phases, D2 to D5 (portD)
#define PIN_PH0         2       // D2
#define PIN_PH1         3       // D3
#define PIN_PH2         4       // D4
#define PIN_PH3         5       // D5

#define PINS_PHASES     (_BV(PIN_PH0)|_BV(PIN_PH1)|_BV(PIN_PH2)|_BV(PIN_PH3))
#define WR_PORT_PHASES  PORTD
#define RD_PORT_PHASES  PIND

#define PHASES_BUS_RESET   ((RD_PORT_PHASES & PINS_PHASES) == (_BV(PIN_PH0)|_BV(PIN_PH2)))
#define PHASES_BUS_ENABLE  ((RD_PORT_PHASES & (_BV(PIN_PH1)|_BV(PIN_PH3))) == (_BV(PIN_PH1)|_BV(PIN_PH3)))
#define PHASES_BUS_DISABLE (!(PHASES_BUS_ENABLE))

// RDDATA on D6
#define WR_PORT_RD      PORTD
#define RD_PORT_RD      PIND
#define DIR_PORT_RD     DDRD
#define PIN_RD          6       // D6
#define SET_RD_HIGH     (WR_PORT_RD |= _BV(PIN_RD))
#define SET_RD_LOW      (WR_PORT_RD &= ~_BV(PIN_RD))
#define SET_RD_OUT      (DIR_PORT_RD |= _BV(PIN_RD))
#define SET_RD_IN       (DIR_PORT_RD &= ~(_BV(PIN_RD)))

// WRDATA on D7
#define WR_PORT_WR      PORTD
#define RD_PORT_WR      PIND
#define DIR_PORT_WR     DDRD
#define PIN_WR          7       // D7
#define SET_WR_HIGH     (WR_PORT_WR |= _BV(PIN_WR))
#define SET_WR_LOW      (WR_PORT_WR &= ~_BV(PIN_WR))
#define SET_WR_OUT      (DIR_PORT_WR |= _BV(PIN_WR))
#define SET_WR_IN       (DIR_PORT_WR &= ~(_BV(PIN_WR)))

// REQ aka PH0, on D2
#define WR_PORT_REQ     PORTD
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

//DRV1 on A0
#define WR_PORT_DRV1    PORTC
#define RD_PORT_DRV1    PINC
#define DIR_PORT_DRV1   DDRC
#define PIN_DRV1        0       // A0 (portC bit 0)
#define SET_DRV1_IN     (DIR_PORT_DRV1 &= ~(_BV(PIN_DRV1)))
#define DRV1_IS_HIGH    (RD_PORT_DRV1 & _BV(PIN_DRV1))
#define DRV1_IS_LOW     (!(DRV1_IS_HIGH))

//DRV2 on D9
#define WR_PORT_DRV2    PORTB
#define RD_PORT_DRV2    PINB
#define DIR_PORT_DRV2   DDRB
#define PIN_DRV2        1       // D9 (portB bit 1)
#define SET_DRV2_IN     (DIR_PORT_DRV2 &= ~(_BV(PIN_DRV2)))
#define DRV2_IS_HIGH    (RD_PORT_DRV2 & _BV(PIN_DRV2))
#define DRV2_IS_LOW     (!(DRV2_IS_HIGH))

//----------------------------------------------
// Daisy pins
//----------------------------------------------

#define WR_PORT_DAISY_PH3      PORTB
#define RD_PORT_DAISY_PH3      PINB
#define DIR_PORT_DAISY_PH3     DDRB
#define PIN_DAISY_PH3          0       // D8, portB bit 0
#define SET_DAISY_PH3_HIGH     (WR_PORT_DAISY_PH3 |= _BV(PIN_DAISY_PH3))
#define SET_DAISY_PH3_LOW      (WR_PORT_DAISY_PH3 &= ~_BV(PIN_DAISY_PH3))
#define SET_DAISY_PH3_OUT      (DIR_PORT_DAISY_PH3 |= _BV(PIN_DAISY_PH3))

#define WR_PORT_DAISY_DRV1     PORTC
#define RD_PORT_DAISY_DRV1     PINC
#define DIR_PORT_DAISY_DRV1    DDRC
#define PIN_DAISY_DRV1         2       // A2
#define SET_DAISY_DRV1_HIGH    (WR_PORT_DAISY_DRV1 |= _BV(PIN_DAISY_DRV1))
#define SET_DAISY_DRV1_LOW     (WR_PORT_DAISY_DRV1 &= ~_BV(PIN_DAISY_DRV1))
#define SET_DAISY_DRV1_OUT     (DIR_PORT_DAISY_DRV1 |= _BV(PIN_DAISY_DRV1))

#define WR_PORT_DAISY_DRV2     PORTC
#define RD_PORT_DAISY_DRV2     PINC
#define DIR_PORT_DAISY_DRV2    DDRC
#define PIN_DAISY_DRV2         3       // A3
#define SET_DAISY_DRV2_HIGH    (WR_PORT_DAISY_DRV2 |= _BV(PIN_DAISY_DRV2))
#define SET_DAISY_DRV2_LOW     (WR_PORT_DAISY_DRV2 &= ~_BV(PIN_DAISY_DRV2))
#define SET_DAISY_DRV2_OUT     (DIR_PORT_DAISY_DRV2 |= _BV(PIN_DAISY_DRV2))


#define WR_PORT_DAISY_HDSEL    PORTC
#define RD_PORT_DAISY_HDSEL    PINC
#define DIR_PORT_DAISY_HDSEL   DDRC
#define PIN_DAISY_HDSEL        1       // A1
#define SET_DAISY_HDSEL_IN     (DIR_PORT_DAISY_HDSEL &= ~_BV(PIN_DAISY_HDSEL))
#define SET_DAISY_HDSEL_HIGH   (WR_PORT_DAISY_HDSEL |= _BV(PIN_DAISY_HDSEL))
#define DAISY_HDSEL_IS_HIGH    (RD_PORT_DAISY_HDSEL & _BV(PIN_DAISY_HDSEL))
#define DAISY_HDSEL_IS_LOW     (!(DAISY_HDSEL_IS_HIGH))

#define WR_PORT_LED            PORTC
#define RD_PORT_LED            PINC
#define DIR_PORT_LED           DDRC
#define PIN_LED                4       // A4
#define SET_LED_OUT            (DIR_PORT_LED |= _BV(PIN_LED))
#define SET_LED_HIGH           (WR_PORT_LED |= _BV(PIN_LED))
#define SET_LED_LOW            (WR_PORT_LED &= ~_BV(PIN_LED))

#define PIN_CHIP_SELECT        10      // D10

#endif
