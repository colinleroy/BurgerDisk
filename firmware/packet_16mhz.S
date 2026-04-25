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
// Returns 0 on success, 1 if bus deactivated, 2 if start byte not found
//*****************************************************************************
.global ReceivePacket

ReceivePacket:
          cli
          mov  XL,r24                                                ;move packet buffer pointer into X
          mov  XH,r25

          clr  r19                    ;clear start of packet found flag (r19)
          st  x+, r19
          st  x+, r19
          st  x+, r19
          st  x+, r19
          st  x+, r19

          rcall SP_ACK_ON

recv_wait_ack:
          ; Verify that the bus is still enabled!
          sbic _SFR_IO_ADDR(RD_PORT_PHASES),PIN_PH1                  ;if phase1 is now down, bus is now disabled
          sbis _SFR_IO_ADDR(RD_PORT_PHASES),PIN_PH3                  ;otherwise, if phase3 is still up, bus is still enabled
          rjmp recv_cancel                                           ;if bus disabled, return with an error
          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;1/2 wait for req line to go high
          rjmp recv_wait_ack                                         ;this indicates host is about to send packet

          ldi  r22,1
recv_start:
          sbis _SFR_IO_ADDR(RD_PORT_WR),PIN_WR
          rjmp 2f                                                    ; 3 cycles min, 7 cycles max

          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;Don't wait forever, verify that REQ is still high
          rjmp recv_cancel                                           ;if not, return with an error
          rjmp recv_start
                                                                     ; wait for half a bit, 2us (32 cycles total)
2:        ldi  r24,10                                                ; 30 cycles waster
3:        dec  r24                                                   ;(1*9 decrement)
          brne 3b                                                    ;(3*8 loop, +2)

nxtbyte:                  ; nxtbyte to loadbit: 1 + 11(nxtbit)                                   = 12
          ldi  r25,8      ; 1

nxtbit:                   ; nxtbit to loadbit: 3(noskip)+8(bitset) or 4(skip)+7(bitclr)          = 11
                                                ; noskip | skip
          sbic _SFR_IO_ADDR(RD_PORT_WR),PIN_WR  ; 1      | 2
          rjmp bitset                           ; 3      |
          rjmp bitclr                           ;        | 4

bitset:                   ; bitset to loadbit: 3+5(carryclr) if noskip; 2+6(bitset_cont) if skip = 8
                          ; noskip  | skip
          sbrc r22,0      ; 1       | 2
          rjmp carryclr   ; 3
bitset_cont:              ; bitset_cont to loadbit: 6 cycles
          ldi  r22,1      ; 1
          sec             ; 2
          nop             ; 3
          nop             ; 4
          rjmp loadbit    ; 6

bitclr:                   ; bitclr to loadbit: 3+4(carryset) if noskip, 2+5(bitclr_cont) if skip = 7
                          ; noskip  | skip
          sbrc r22,0      ; 1       | 2
          rjmp carryset   ; 3
bitlcr_cont:              ; bitclr_cont to loadbit: 5 cycles
          ldi  r22,0      ; 1
          clc             ; 2
          nop             ; 3
          rjmp loadbit    ; 5

carryset:                 ; carryset to loadbit: 4 cycles
          ldi  r22,0      ; 1
          sec             ; 2
          rjmp loadbit    ; 4

carryclr:                 ; carryclr to loadbit: 5 cycles
          ldi  r22,1      ; 1
          clc             ; 2
          nop             ; 3
          nop             ; 4
          nop             ; 5

loadbit:                  ; We are now at 11 cycles since nxtbit
          rol  r23        ; 1 - total 12
          dec  r25        ; 1 - total 13
          breq havebyte   ; 1 - total 14 assuming we don't branch

          ldi  r24,15     ; waste loop: 1 + 14*3 + 2 = 45 cycles
4:        dec  r24
          brne 4b
                          ; We are now at 59 cycles since nxtbit (14 + 45)
          nop             ; 1 - total 60
          nop             ; 1 - total 61
          nop             ; 1 - total 62
          rjmp nxtbit     ; 2 - total 64 since nxtbit

havebyte:                 ; We have a full byte and must go back to nxtbyte in a 64 cycle timeframe
                          ; We come from loadbit and are at 14 cycles since last bit.
                          ; No need to count cycles here, we'll resync on the next change (MSB always set)
handle_byte:
          cpi  r19,1      ;check flag (r19) to see if we have received the start byte
          breq pktstrtd   ;yes, hop around the start byte check and continue (3 cycles)
          cpi  r23,0xC3   ;else see if its the packet start byte (these cycles are ignored)
          brne getnext    ;no, must be sync byte, skip and go get next byte
          ldi  r19,1      ;yes, set r19=1 as flag to say we have received the start of the packet
pktstrtd:
          st   x+,r23     ;17                    ;2   save byte in buffer
getnext:
          ldi  r25,100    ;18                    ;1   timeout counter if we are at the end
          cpi  r22,1      ;19                    ;1   check for status of last bit
          breq wasset     ;20  ;21               ;1/2

                          ; Wait for new byte to begin

wasclr:   sbic _SFR_IO_ADDR(RD_PORT_WR),PIN_WR ;
          rjmp havesbit                        ; we have read a bit
          dec  r25
          breq recv_done                       ; we have timed out, must be end of packet
          rjmp wasclr

wasset:   sbis _SFR_IO_ADDR(RD_PORT_WR),PIN_WR
          rjmp havesbit                        ; we have read a bit
          dec  r25
          breq recv_done                       ; we have timed out, must be end of packet
          rjmp wasset

                                               ; Resync - 3 to 10 cycles since bit has been toggled
havesbit:                                      ; wait for half a bit, 2us (32 cycles total)
                                               ; Must waste 22-29 cycles.

          ldi  r24,7                           ;delay total of 21 cycles
5:        dec  r24
          brne 5b
          rjmp nxtbyte

recv_done:
          clr  r23
          st   x+,r23                          ;save zero byte in buffer to mark end

          cpi  r19,1                           ;check flag (r19) to see if we have received the start byte
          breq ok                              ;yes, return success
          ldi  r24,2
          clr  r25
          ret

ok:
          clr  r25                             ;return no error (for now)
          clr  r24
          ret

.global AckPacket
AckPacket:
          rcall SP_ACK_OFF                                           ;pull ACK low,

wait_req_low:
          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;wait for REQ line to go low
          rjmp req_is_low                                            ;this indicates host has acknowledged ACK
          rjmp wait_req_low

req_is_low:
          sei
          ret

.global IgnorePacket
IgnorePacket:
          rcall SP_ACK_MUTE
          rcall SP_RD_MUTE
          rjmp wait_req_low                                          ;wait for REQ line to go low
          ; Fallthrough
send_cancel:
          rcall SP_ACK_OFF
          rcall SP_ACK_MUTE
          rcall SP_RD_MUTE
          ldi  r24,1                               ;setup return value, 1=error
          clr  r25
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

          cbi  _SFR_IO_ADDR(WR_PORT_RD),PIN_RD                       ; Set RD low
          sbi  _SFR_IO_ADDR(DIR_PORT_RD),PIN_RD                      ; Set RD as output
          rcall SP_ACK_ON

send_wait_ack:
          sbic _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;wait for req line to go high
          rjmp send_start                                            ;this indicates host is ready to receive packet

          ; Verify that the bus is still enabled!
          sbic _SFR_IO_ADDR(RD_PORT_PHASES),PIN_PH1                  ;if phase1 is now down, bus is now disabled
          sbis _SFR_IO_ADDR(RD_PORT_PHASES),PIN_PH3                  ;otherwise, if phase3 is still up, bus is still enabled
          rjmp send_cancel                                           ;if bus disabled, return with an error
          rjmp send_wait_ack

send_start:                                                          ;Totals for loops             ;
          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;Don't wait forever, verify that REQ is still high
          rjmp send_cancel                                           ;if not, return with an error

nxtsbyte: ld   r23,x+                                               ;59               ;43         ;2   get first byte from buffer
          cpi  r23,0                                                ;60               ;44         ;1   zero marks end of data
          breq send_end                                             ;61               ;45         ;1/2

          ldi  r25,8                                                ;62               ;46         ;1   8bits to read
                                                                    ;Clr  ;Set
nxtsbit:  sbrs r23,7                                                ;64(Set) ;64      ;47   ;48   ;1/2 send bit 7 first
                                                                    ;63(Clr)
          rjmp sbitclr                                              ;64+1             ;48+1       ;2   bit is clear
sbitset:
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
