//*****************************************************************************
//
// Apple //c Smartport Compact Flash adapter
// Written by Robert Justice  email: rjustice(at)internode.on.net
// 
// Assembler routines for sending and receiving the smartport packets
// These are timing sensitive and assume a clk frequency of 16Mhz
//
// MIT-licensed:
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the “Software”), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
//*****************************************************************************

// required for register definitions
#include <avr/io.h>
#include "sp_pins.h"

.global SP_ACK_MUTE
SP_ACK_MUTE:
          cbi  _SFR_IO_ADDR(DIR_PORT_ACK),PIN_ACK                    ; Set ACK input
          cbi  _SFR_IO_ADDR(WR_PORT_ACK),PIN_ACK                     ; Set ACK low
          ret

.global SP_ACK_ON
SP_ACK_ON:
          sbi  _SFR_IO_ADDR(WR_PORT_ACK),PIN_ACK                     ; ACK high
          sbi  _SFR_IO_ADDR(DIR_PORT_ACK),PIN_ACK                    ; ACK output
          ret

.global SP_ACK_OFF
SP_ACK_OFF:
          cbi  _SFR_IO_ADDR(WR_PORT_ACK),PIN_ACK                     ; ACK low
          sbi  _SFR_IO_ADDR(DIR_PORT_ACK),PIN_ACK                    ; ACK output
          ret

.global SP_RD_MUTE
SP_RD_MUTE:
          cbi  _SFR_IO_ADDR(DIR_PORT_RD),PIN_RD                      ; Set RD as input
          sbi  _SFR_IO_ADDR(WR_PORT_RD),PIN_RD                       ; Set RD high
          ret

recv_cancel:
          rcall SP_ACK_OFF
          rcall SP_ACK_MUTE
          rcall SP_RD_MUTE
          ldi  r24,1                               ;setup return value, 1=error
          clr  r25
          sei
          ret

//*****************************************************************************
// Function: ReceivePacket
// Parameters: packet_buffer pointer
// Returns: status (not used yet, always returns 0)
//
// Description: This handles the ACK and REQ lines and reads a packet into the 
// packet_buffer. Warning: returns with interrupts off, must be followed by either
// AckPacket or IgnorePacket.
// 
//*****************************************************************************
.global ReceivePacket

ReceivePacket:
          cli
          mov  XL,r24                                                ;move packet buffer pointer into X
          mov  XH,r25

          rcall SP_ACK_ON

recv_wait_ack:
          sbic _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;1/2 wait for req line to go high
          rjmp recv_start                                            ;this indicates host is about to send packet

          ; Verify that the bus is still enabled!
          sbic _SFR_IO_ADDR(RD_PORT_PHASES),PIN_PH1                  ;if phase1 is now down, bus is now disabled
          sbis _SFR_IO_ADDR(RD_PORT_PHASES),PIN_PH3                  ;otherwise, if phase3 is still up, bus is still enabled
          rjmp recv_cancel                                           ;if bus disabled, return with an error
          rjmp recv_wait_ack                                         ;otherwise, keep waiting

recv_start:
          ldi  r22,1                                                 ;1   remember tx line status when previously sampled
          sbis _SFR_IO_ADDR(RD_PORT_WR),PIN_WR                       ;1/2 wait for txd line to go low
          rjmp 2f                                                    ;2   txd cleared, start of packet data

          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;Don't wait forever, verify that REQ is still high
          rjmp recv_cancel                                           ;if not, return with an error

          rjmp recv_start                                            ;2
                                                                     ;    wait for half a bit, 2us (28 cycles total) --> 32 cycles for 64Mhz
                                                                     ;    this is so we sample mid point  --> I think this is ok for 16Mhz
2:        ldi  r24,10                                                ;1   |delay total of 30 cycles
3:        dec  r24                                                   ;1   | each loop +3 final loop +2
          brne 3b                                                    ;1/2 | 1 + 9x3 + 1x2 = 30

nxtbyte:                                                             ;    full cycle time for each byte is 32us
          ldi  r25,8                                                 ;1   8bits to read

nxtbit:   sbic _SFR_IO_ADDR(RD_PORT_WR),PIN_WR                       ;2   ;2    ;1  ;1      ;1/2 now read a bit, cycle time is 4us
          rjmp bitset                                                ;3  ;3      ;2   bit is set
          rjmp bitclr                                                ;4   ;4                ;2   bit is clr

bitset:   sbrc r22,0                                                 ;4  ;5      ;1/2 test previous bit recv'd
          rjmp carryclr                                              ;6          ;2   bit set, then we have a zero
          ldi  r22,1                                                 ;6      ;1   remember prev tx bit is set
          sec                                                        ;7      ;1   else we have a one
          nop                                                        ;8      ;1
          nop                                                        ;9      ;1
          rjmp loadbit                                               ;11     ;2

bitclr:   sbrc r22,0                                                 ;5   ;6                ;1/2 test previous bit recv'd
          rjmp carryset                                              ;7                     ;2   bit set, then we have a one
          ldi  r22,0                                                 ;7                ;1   remember prev tx bit is clr
          clc                                                        ;8                ;1   else we have a zero
          nop                                                        ;9                ;1
          rjmp loadbit                                               ;11               ;2

carryset: ldi  r22,0                                                 ;8                     ;1
          sec                                                        ;9                     ;1   remember prev tx bit is clr
          rjmp loadbit                                               ;11                    ;2

carryclr: ldi  r22,1                                                 ;7           ;1   remember prev tx bit is set
          clc                                                        ;8           ;1
          nop                                                        ;9           ;1
          nop                                                        ;10          ;1
          nop                                                        ;11          ;1

loadbit:  rol  r23                                                   ;12  ;12  ;12  ;12     ;1   shift bit(carry) into r23
          dec  r25                                                   ;13  ;13  ;13  ;13     ;1   dec bit counter
          breq havebyte                                              ;14  ;14  ;14  ;14     ;1/2
                                                                     ;    delay to make up the rest of the 4us --> 64 cycles for 16Mhz (8 more)
                                     ;                                            2 more loops +2 nops

          ldi  r24,15                                                ;1   |delay total of 45 cycles
4:        dec  r24                                                   ;1   | each loop +3 final loop +2
          brne 4b                                                    ;1/2 | 1 + 14x3 + 1x2 = 45
                                                                     ;59  ;59  ;59  ;59
          nop                                                        ;60  ;60  ;60  ;60     ;1
          nop                                                        ;61  ;61  ;61  ;61     ;1
          nop                                                        ;62  ;62  ;62  ;62     ;1
          rjmp nxtbit                                                ;64  ;64  ;64  ;64     ;2   get next bit. --> 64 cycles = 4us for 16Mhz
                                                                     ;--------------------------------------
havebyte:
          st   x+,r23                                                ;17                    ;2   save byte in buffer
          ldi  r25,100 ;era 100!!!!!!!!!                             ;18                    ;1   timeout counter if we are at the end
          cpi  r22,1                                                 ;19                    ;1   check for status of last bit
          breq wasset                                                ;20  ;21               ;1/2

wasclr:   sbic _SFR_IO_ADDR(RD_PORT_WR),PIN_WR                       ;21                    ;1/2 now read a bit, wait for transition to 1
          rjmp havesbit                                              ;23                    ;2   now set, lets get the next byte
          dec  r25                                                   ;1
          breq recv_done                                             ;1/2 we have timed out, must be end of packet
          rjmp wasclr                                                ;2   lets test again

wasset:   sbis _SFR_IO_ADDR(RD_PORT_WR),PIN_WR                       ;22               ;1/2 now read a bit, wait for transition to 0
          rjmp havesbit                                              ;24               ;2   now clr, lets get the next byte
          dec  r25                                                   ;1
          breq recv_done                                             ;1/2 we have timed out, must be end of packet
          rjmp wasset                                                ;2   lets test again

havesbit:                                                            ;    wait for half a bit, 2us (28 cycles total) --> 32 cycles for 16MHz
                                                                     ;    this is so we sample mid point --> again, i think this was long before, so try as is
          ldi  r24,7                                                 ;1   |delay total of 21 cycles
5:        dec  r24                                                   ;1   | each loop +3 final loop +2
          brne 5b                                                    ;1/2 | 1 + 6x3 + 1x2 = 21
                                                                     ;44  ;45
          rjmp nxtbyte                                               ;46  ;47               ;2   get next byte

recv_done:
          clr  r23
          st   x+,r23                                                ;save zero byte in buffer to mark end

          clr  r25                                                   ;return no error (for now)
          clr  r24
          ret

.global AckPacket
AckPacket:
          rcall SP_ACK_OFF                                           ;pull ACK low,

ack_wait:
          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;wait for REQ line to go low
          rjmp ack_done                                              ;this indicates host has acknowledged ACK
          rjmp ack_wait

ack_done:
          sei
          ret

.global IgnorePacket
IgnorePacket:
          rcall SP_ACK_MUTE
          rcall SP_RD_MUTE

ignore_wait:
          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;wait for REQ line to go low
          rjmp ignore_done                                           ;(the other device and computer have finished)
          rjmp ignore_wait

ignore_done:
          sei
          ret

//*****************************************************************************
// Function: SendPacket
// Parameters: packet_buffer pointer
// Returns: status (not used yet, always returns 0)
//
// Description: This handles the ACK and REQ lines and sends the packet from the
// pointer passed to it. (packet_buffer)
//
//*****************************************************************************
.global SendPacket

SendPacket:
          cli
          mov  XL,r24                                                ;mov buffer pointer into X
          mov  XH,r25

          sbi  _SFR_IO_ADDR(DIR_PORT_RD),PIN_RD                      ; Set RD as output

          rcall SP_ACK_ON

send_wait_ack:
          sbic _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;wait for req line to go high
          rjmp send_start                                            ;this indicates host is ready to receive packet
          rjmp send_wait_ack

send_start:                                                          ;Totals for loops             ;

nxtsbyte: ld   r23,x+                                               ;59               ;43         ;2   get first byte from buffer
          cpi  r23,0                                                ;60               ;44         ;1   zero marks end of data
          breq send_end                                             ;61               ;45         ;1/2

          ldi  r25,8                                                ;62               ;46         ;1   8bits to read
                                                                    ;Clr  ;Set
nxtsbit:  sbrs r23,7                                                ;64(Set) ;64      ;47   ;48   ;1/2 send bit 7 first
                                                                    ;63(Clr)
          rjmp sbitclr                                              ;64+1             ;48+1       ;2   bit is clear
          sbi  _SFR_IO_ADDR(WR_PORT_RD),PIN_RD                      ;2    ;2   set bit for 1us (14 cycles)-->16 cycles for 16Mhz(2 more)

          ldi  r24,4                                                ;1   |delay total of 12 cycles
3:        dec  r24                                                  ;1   | each loop +3 final loop +2
          brne 3b                                                   ;1/2 | 1 + 3x3 + 1x2 = 11
                                                                    ;14
          nop                                                       ;15   ;1
          nop                                                       ;16   ;1

          cbi  _SFR_IO_ADDR(WR_PORT_RD),PIN_RD                      ;2    ;2   clr bit for 3us (42 cycles)-->48 cycles for 16Mhz(6 more)
                                                                    ;                                            2 more loops
          dec  r25                                                  ;3    ;1   dec bit counter
          breq nxtsbyt1                                             ;4    ;1/2
          rol  r23                                                  ;5    ;1

          ldi  r24,13                                               ;1   |delay total of 39 cycles
4:        dec  r24                                                  ;1   |  each loop +3 final loop +2
          brne 4b                                                   ;1/2 |  1 + 12x3 + 1x2 = 39
                                                                    ;44

          rjmp nxtsbit                                              ;46   ;2

nxtsbyt1:                                                           ;5          ;    delay to makeup 3us (42 cycles total)-->48 cycles for 16Mhz(6 more)
                                                                    ;                                            2 more loops
          ldi  r24,11                                               ;1   |delay total of 33 cycles
5:        dec  r24                                                  ;1   | each loop +3 final loop +2
          brne 5b                                                   ;1/2 | 1 + 10x3 + 1x2 = 33
                                                                    ;38
          nop                                                       ;39         ;1
          rjmp nxtsbyte                                             ;41         ;2

; bit is clr, we need to check if its the last one, otherwise delay for 4us before next bit
sbitclr:  dec  r25                                                  ;2                   ;1
          breq nxtsbycl                                             ;4       ;3                   ;1/2 end of byte, delay then get nxt
          rol  r23                                                  ;4                   ;1
                                                                    ;    delay to makeup 4us (56 cycles total)-->64 cycles for 16Mhz(8 more)
                                                                    ;                                            2 more loops +2 nops
          ldi  r24,18                                               ;1   |delay total of 54 cycles
6:        dec  r24                                                  ;1   | each loop +3 final loop +2
          brne 6b                                                   ;1/2 | 1 + 17x3 + 1x2 = 54
                                                                    ;58
          nop                                                       ;59                  ;1
          nop                                                       ;60                ;1
          rjmp nxtsbit                                              ;62                  ;2

nxtsbycl:                                                           ;    delay to makeup 4us (56 cycles total)-->64 cycles for 16Mhz(8 more)
                                                                    ;                                            2 more loops +2 nops
                                                                    ;
          ldi  r24,16                                               ;1   |delay total of 48 cycles
7:        dec  r24                                                  ;1   | each loop +3 final loop +2
          brne 7b                                                   ;1/2 | 1 + 15x3 + 1x2 = 48
                                                                    ;52
          nop                                                       ;53                           ;1
          nop                                                       ;54                           ;1
          nop                                                       ;55                           ;1
          rjmp nxtsbyte                                             ;57                           ;2


send_end:
          rcall SP_ACK_OFF

send_wait_end_ack:
          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                    ;wait for REQ line to go low
          rjmp send_done                                            ;this indicates host has acknowledged ACK
          rjmp send_wait_end_ack

send_done:
          rcall SP_RD_MUTE
          clr  r25
          clr  r24                                                   ;return no error
          sei
          ret
