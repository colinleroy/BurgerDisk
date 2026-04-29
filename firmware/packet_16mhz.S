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
          rjmp pop_and_ret

/******************************************************************************
Function: ReceivePacket
Parameters: packet_buffer pointer
Returns: status (not used yet, always returns 0)
Description: This handles the ACK and REQ lines and reads a packet into the 
packet_buffer. Warning: returns with interrupts off, must be followed by either
AckPacket or IgnorePacket.
Returns 0 on success, 1 if bus deactivated, 2 if start byte not found

Register use:
r19: got packet_start
r20: header bytes counter
r21: computed checksum
r22: line state
r23: current byte
r24: sync cycle counter
r25: bit counter/timeout counter
r26: X index
r27: X index
r28: MSB byte, then expected checksum byte
r29: number of bytes before next MSB byte
r17: numodd
r18: numgrp
*****************************************************************************/
.global ReceivePacket
ReceivePacket:
          cli
          push r19                    ;Backup every register we use. Only r19 is
          push r20                    ;necessary *apparently* but heh.
          push r21
          push r22
          push r23
          push r26
          push r27
          push r28
          push r29
          push r17
          push r18

          mov  XL,r24                 ;move packet buffer pointer into X
          mov  XH,r25

          mov  ZL,r24                 ;decoded data pointer
          mov  ZH,r25
          adiw z, 13

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
          rjmp first_transition                                                    ; 3 cycles min, 7 cycles max

          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;Don't wait forever, verify that REQ is still high
          rjmp recv_cancel                                           ;if not, return with an error
          rjmp recv_start

first_transition:
                                                                     ; wait for half a bit, 2us (32 cycles total)
          ldi  r24,6                                                 ; 19 cycles waster - previous checks ate some of the 32
3:        dec  r24                                                   ;(1*9 decrement)
          brne 3b                                                    ;(3*6 loop, +2)
          nop

next_byte:                                      ; next_byte to load_bit: 1 + 11(next_bit)                                   = 12
          ldi  r25,8                            ; 1

next_bit:                                       ; next_bit to load_bit: 3(noskip)+8(line_is_up) or 4(skip)+7(line_is_down)          = 11
                                                ; noskip | skip
          cbi _SFR_IO_ADDR(WR_PORT_LED),PIN_LED ; 2        2     Toggle LED to get a reference on logic captures
          sbi _SFR_IO_ADDR(WR_PORT_LED),PIN_LED ; 4        4

          sbic _SFR_IO_ADDR(RD_PORT_WR),PIN_WR  ; 5      | 6
          rjmp line_is_up                       ; 7      |
          rjmp line_is_down                     ;        | 8

line_is_up:                  ; line_is_up to load_bit: 3+5(line_was_down) if noskip; 2+6(line_is_up_cont) if skip = 8
                             ; noskip  | skip
          sbrc r22,0         ; 1       | 2
          rjmp line_was_down ; 3
line_is_up_cont:             ; line_is_up_cont to load_bit: 6 cycles
          ldi  r22,1         ; 1
          sec                ; 2
          nop                ; 3
          nop                ; 4
          rjmp load_bit      ; 6

line_is_down:                ; line_is_down to load_bit: 3+4(line_was_up) if noskip, 2+5(line_is_down_cont) if skip = 7
                             ; noskip  | skip
          sbrc r22,0         ; 1       | 2
          rjmp line_was_up   ; 3
line_is_down_cont:           ; line_is_down_cont to load_bit: 5 cycles
          ldi  r22,0         ; 1
          clc                ; 2
          nop                ; 3
          rjmp load_bit      ; 5

line_was_up:                 ; line_was_up to load_bit: 4 cycles
          ldi  r22,0         ; 1
          sec                ; 2
          rjmp load_bit      ; 4

line_was_down:               ; line_was_down to load_bit: 5 cycles
          ldi  r22,1         ; 1
          clc                ; 2
          nop                ; 3
          nop                ; 4
          nop                ; 5

load_bit:                    ; We are now at 15 cycles since next_bit
          rol  r23           ; 1 - total 16
          dec  r25           ; 1 - total 17
          breq byte_done     ; 1 - total 18 assuming we don't branch

          ldi  r24,14        ; waste loop: 1 + 14*3 + 2 = 45 cycles
4:        dec  r24
          brne 4b
                             ; We are now at 63 cycles since next_bit (18 + 45)
          nop                ; 1 - total 64
          rjmp next_bit

byte_done:                   ; We have a full byte and must go back to next_byte in a 64 cycle timeframe
                             ; We come from load_bit and are at 14 cycles since last bit.
                             ; No need to count cycles here, we'll resync on the next change (MSB always set)
          cpi  r19,0         ;check flag (r19) to see if we have received the start byte
          brne store_byte    ;yes, hop around the start byte check and continue (3 cycles)
          cpi  r23,0xC3      ;else see if its the packet start byte (these cycles are ignored)
          brne wait_next     ;no, must be sync byte, skip and go get next byte
          ldi  r19,1         ;yes, set r19=1 as flag to say we have received the start of the packet
store_byte:
          st   x+,r23        ;17                    ;2   save byte in buffer

wait_next:                   ; Wait for next transition, start of next byte
          ldi  r25,100       ;18                    ;1   timeout counter if we are at the end
          cpi  r22,1         ;19                    ;1   check for status of last bit
          breq wait_down     ;20  ;21               ;1/2

                             ; Wait for new byte to begin, so we can resync
wait_up:
          sbic _SFR_IO_ADDR(RD_PORT_WR),PIN_WR ; 1 / 2
          rjmp got_transition                  ; 3      we have read a bit
          dec  r25
          brne wait_up
          rjmp recv_done

wait_down:
          sbis _SFR_IO_ADDR(RD_PORT_WR),PIN_WR
          rjmp got_transition                  ; we have read a bit
          dec  r25
          brne wait_down
          rjmp recv_done                       ; we have timed out, must be end of packet

                                               ;Unreachable from previous branches
verify_chksum_h:                               ;inlined here for branching reach
          ldi  r19, 7                          ;this is the last step. nothing to do next
          rol  r23                             ;get relevant bits of checksum byte
          andi r23,0xAA
          or   r28,r23                         ;expected checksum ready in r28
          rjmp waste3                          ;9 cycles incl jump

got_transition:                               ; Compute checksum in 18 cycles :-)
          cpi  r19,3
          breq compute_odd_chksum             ; takes 15 cycles + 3 = 18
          cpi  r19,4
          breq compute_grp_chksum             ; takes 13 cycles + 5 = 18
          cpi  r19,2
          breq chksum_hdr                     ; takes 11 cycles + 7 = 18
          cpi  r19,5
          breq verify_chksum_h                ; takes 9 cycles + 9 = 18
          cpi  r19,1
          breq packet_started                 ; takes 7 cycles + 11 = 18
          rjmp waste6                         ; takes 12 cycles + 6 = 18

packet_started:
          ldi  r20, 7                          ;set num header bytes to 7
          clr  r21                             ;clear checksum
          ldi  r19, 2                          ;next state = chksum_hdr
          rjmp waste2                          ;took 5 cycles incl jump, 7 with waste

chksum_hdr:
          eor  r21, r23                        ;checksum one header byte
          dec  r20                             ;dec num_header bytes to check
          brne check_odds
get_numgrp:
          andi r23, 0x7F                       ;clear high bit
          mov  r18, r23                        ;store numgrps
          ldi  r19, 3                          ;next step is data checksumming of the odd bytes
          nop
          nop
          nop
          rjmp next_byte
check_odds:
          cpi  r20, 1                          ;if it's second to last, it's the number of odd bytes.
          brne skip
get_numodd:
          andi r23, 0x7F                      ;clear high bit
          mov  r17, r23                       ;store numodd
          clr  r29                            ;we'll need to fetch MSBs so note we have no high bits available
          rjmp next_byte                      ;took 11 cycles incl jump, 18 total
skip:
          rjmp waste2                         ;took 9 cycles+2, 18 total

compute_odd_chksum:
          nop
          nop
          cpi  r17, 0                          ;more odd bytes?
          breq odd_chksum_done
          cpi  r29, 0                          ;do we have the MSB byte?
          breq fetch_odd_msb                   ;if no, fetch it
eor_odd:
          rol  r23                             ;get rid of odd byte's high bit
          clc
          rol  r28                             ;get high bit from current MSB byte
          ror  r23                             ;put it in odd byte
          eor  r21, r23                        ;update checksum
          dec  r17                             ;one odd byte done
          st   z+, r23                         ;store the byte
          rjmp next_byte                       ;15 cycles incl jump
fetch_odd_msb:
          mov  r29, r17                        ;all odd bytes need a single MSB
          mov  r28, r23                        ;store MSB for odd
          rol  r28                             ;get rid of its high bit
          rjmp waste3                          ;15 cycles incl jump

odd_chksum_done:                               ;we're done with odd bytes
          cpi  r18, 0                          ;do we have group bytes?
          breq verify_chksum_l                 ;if no groups, go start verifying the checksum

          ldi  r19, 4                          ;next step is checksumming groups
          dec  r18                             ;dec groups for next time
          mov  r28, r23                        ;store current MSB byte
          rol  r28                             ;get rid of its high bit
          ldi  r29, 7                          ;set available MSB bits
          nop
          rjmp next_byte                       ;15 cycles incl jump

verify_chksum_l:
          ldi  r19, 5                          ;next byte is checksum high
          andi r23, 0x55                       ;mask relevant bits
          mov  r28, r23                        ;and store in expected checksum byte
          rjmp next_byte                       ;15 cycles incl jump

compute_grp_chksum:
          cpi  r29, 0                          ;do we need to fetch MSBs?
          breq next_group
cont_group:
          dec  r29                             ;decrement number of available MSBs
          rol  r23                             ;clear non-decoded data byte's high bit
          clc
          rol  r28                             ;current MSB to carry
          ror  r23                             ;carry to current byte
          eor  r21, r23                        ;update computed checksum
          st   z+, r23                         ;and store it
          nop
          nop
          rjmp next_byte                       ;13 cycles incl jump
next_group:
          nop
          dec  r18                             ;finished a group, dec their number
          brpl store_group_msb                 ; if there's more, store byte as MSB
          rjmp verify_chksum_l                 ; 8 cycles so far... 13 total

store_group_msb:
          mov  r28, r23                        ;store MSB
          rol  r28                             ;get rid of its high bit
          ldi  r29, 7                          ;7 MSBs available
          nop
          rjmp next_byte                       ;13 cycles incl jump

recv_done:
          clr  r23
          st   x+,r23                          ;save zero byte in buffer to mark end

          clr  r25

          cpi  r19, 7                          ;have we finished the packet?
          brne nopkt_error

          clr  r24                             ;return no error
          cp   r28, r21                        ;check checksum...
          brne chk_error
          rjmp pop_and_ret

chk_error:
          ; In case of checksum error, wait for REQ to go low so caller can retry
          ; without a race condition on ACK/REQ
          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;wait for REQ line to go low
          rjmp ret_chkerr                                            ;this indicates host has acknowledged ACK
          rjmp chk_error
ret_chkerr:
          ldi  r24, 8                          ;return checksum error
          rjmp pop_and_ret

nopkt_error:
          ; same for incomplete packet
          sbis _SFR_IO_ADDR(RD_PORT_REQ),PIN_REQ                     ;wait for REQ line to go low
          rjmp ret_nopkterr                                          ;this indicates host has acknowledged ACK
          rjmp nopkt_error
ret_nopkterr:
          mov  r24, r19                        ;return step as error
          ori  r24, 0x80                       ;with high bit in case it's zero
          rjmp pop_and_ret

waste6:   nop
waste5:   nop
waste4:   nop
waste3:   nop
waste2:   rjmp next_byte

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
          rjmp sline_is_down                                              ;64+1             ;48+1       ;2   bit is clear
sline_is_up:
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
sline_is_down:  dec  r25                                                  ;2                   ;1
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

pop_and_ret:
          pop  r18
          pop  r17
          pop  r29
          pop  r28
          pop  r27
          pop  r26
          pop  r23
          pop  r22
          pop  r21
          pop  r20
          pop  r19
          ret
