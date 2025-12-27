#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "sp_pins.h"

void daisy_ph3_mirror(void) {
  if (RD_PORT_PHASES & _BV(PIN_PH3)) {
    SET_DAISY_PH3_HIGH;
  } else {
    SET_DAISY_PH3_LOW;
  }
}

void daisy_ph3_disable() {
  SET_DAISY_PH3_LOW;
}

void daisy_diskII_mirror(void) {
  if (DRV1_IS_HIGH) {
    SET_DAISY_DRV1_HIGH;
  } else {
    SET_DAISY_DRV1_LOW;
  }

  if (DRV2_IS_HIGH) {
    SET_DAISY_DRV2_HIGH;
  } else {
    SET_DAISY_DRV2_LOW;
  }
}

void daisy_diskII_disable(void) {
  SET_DAISY_DRV1_HIGH;
  SET_DAISY_DRV2_HIGH;
}
