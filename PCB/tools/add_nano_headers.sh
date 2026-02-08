#!/bin/bash

INPUT_BOM_FILE="$1"
INPUT_POS_FILE="$2"

if [ "$INPUT_BOM_FILE" = "" ]; then
  echo "Missing BOM file"
  exit 1
fi
if [ "$INPUT_POS_FILE" = "" ]; then
  echo "Missing POS file"
  exit 1
fi

NANO_PIN_SPACING='15.24'

X=$(grep "Arduino_Nano_v3" $INPUT_POS_FILE | cut -d',' -f4)
Y=$(grep "Arduino_Nano_v3" $INPUT_POS_FILE | cut -d',' -f5)

if [ "$X" = "" ]; then
  echo "No Nano in design"
  exit 0
fi

OTHER_Y=$(echo "print(\"{:0.6f}\".format($Y+$NANO_PIN_SPACING))"|python3)

echo "\"A2\",\"Nano_conn_1\",\"Nano connector 1\",$X,$Y,0.000000,top" >> $INPUT_POS_FILE
echo "\"A3\",\"Nano_conn_2\",\"Nano connector 2\",$X,$OTHER_Y,0.000000,top" >> $INPUT_POS_FILE

echo '"Nano connector 1","A2,A3","Connector_PinSocket_2.54mm:PinHeader_1x15_P2.54mm_Vertical","C25503121"' >> $INPUT_BOM_FILE
