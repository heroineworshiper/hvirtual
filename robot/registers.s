; registers

REGS		=	0x1000	    ; Device register location




; EEPROM

EEPROM      =   0xb600      ; EEPROM start address

BPROT       =   0x35        ; block protect

PPROG       =   0x3b        ; EEPROM programming control
ODD         =   0x80        ; program odd rows in half of PROM
EVEN        =   0x40        ; program even rows in half of PROM
ELAT        =   0x20        ; enable address and data bus for programming
BYTE        =   0x10        ; enable byte erase mode
ROW         =   0x8         ; enable row/all erase mode
ERASE       =   0x4         ; enable erase mode
EELAT       =   0x2         ; enable address and data bus for programming
EPGM        =   0x1         ; enable PROM programming voltage

; Analog to digital
ADCTL       =   0x30 ; Analog to digital status/control

CCF         =   0x80  ; 4 conversions complete
MULT        =   0x10  ; multiple channel conversion
SCAN        =   0x20  ; continuous scan control

ADR1        =   0x31 ; Analog to digital result 1
ADR2        =   0x32 ; Analog to digital result 2
ADR3        =   0x33 ; Analog to digital result 3
ADR4        =   0x34 ; Analog to digital result 4





; input capture edge to detect
TCTL2       =   0x21
EDG1B       =   0x20
EDG1A       =   0x10
EDG2B       =   0x8
EDG2A       =   0x4
EDG3B       =   0x2
EDG3A       =   0x1



; timer
TOC1        =   0x16 		; output compare value 1
TOC2        =   0x18 		; output compare value 2
TOC3        =   0x1a 		; output compare value 3
TOC4        =   0x1c 		; output compare value 4
TOC5        =   0x1e 		; output compare value 5


TMSK1       =   0x22        ; interrupt mask #1
OC1I        =   0x80        ; request interrupt for output compare 1
OC2I        =   0x40        ; request interrupt for output compare 2
OC3I        =   0x20        ; request interrupt for output compare 3
OC4I        =   0x10        ; request interrupt for output compare 4
OC5I        =   0x8         ; request interrupt for output compare 5



TFLG1       =   0x23        ; timer interrupt flag #1
OC1F        =   0x80        ; output compare 1 has sent an interrupt
OC2F        =   0x40        ; output compare 2 has sent an interrupt
OC3F        =   0x20        ; output compare 3 has sent an interrupt
OC4F        =   0x10        ; output compare 4 has sent an interrupt
OC5F        =   0x8         ; output compare 5 has sent an interrupt
IC1F        =   0x4         ; input capture 1 has detected a change
IC2F        =   0x2         ; input capture 2 has detected a change
IC3F        =   0x1         ; input capture 3 has detected a change
IC4F        =   0x8         ; input capture 4 has detected a change

TMSK2       =   0x24        ; interrupt mask #2

TFLG2       =   0x25        ; timer interrupt flag #2


TCTL1       =   0x20        ; output compare timer control
OM2         =   0x80        ; action for each pin
OL2         =   0x40
OM3         =   0x20
OL3         =   0x10
OM4         =   0x08
OL4         =   0x04
OM5         =   0x02
OL5         =   0x01






OC1M        =   0xc  ; output compare 1 mask
OC1M7       =   0x80
OC1M6       =   0x40
OC1M5       =   0x20
OC1M4       =   0x10
OC1M3       =   0x08

OC1D        =   0xd  ; output compare 1 data
OC1D7       =   0x80
OC1D6       =   0x40
OC1D5       =   0x20
OC1D4       =   0x10
OC1D3       =   0x08


TCNT        =   0xe  ; the timer value



; Serial port

; baud rate register
BAUD        =   0x2b

; these values are known to work with the Heroine board
BAUD1800    =   0x33    ; 1800 for 12 Mhz
BAUD14400   =   0x30    ; 14400 for 12 Mhz
BAUD23040   =   0x21    ; 23040 for 12 Mhz


; SCI status register
SCSR		=	0x2E

RDRF		=	0x20 ; SCSR "Received Data Ready" flag
TDRE		=	0x80 ; SCSR "Transmit Data Register Empty" flag


; SCI data register
SCDR		=	0x2F	

; serial peripheral control
SPCR        =   0x28
DWOM        =   0x20 ; wired OR mode

; serial control 1
SCCR1       =   0x2c


; serial control 2
SCCR2       =   0x2d

; Transmitter and receiver enable, no interrupts
SCI_ENABLE	=	0x0c




; port A
PORTA       =   0x0
OC1         =   0x80
OC2         =   0x40
OC3         =   0x20
OC4         =   0x10
OC5         =   0x8
IC1         =   0x4
IC2         =   0x2
IC3         =   0x1
IC4         =   0X8


; pulse accumulator control
PACTL       =   0x26
DDRA7       =   0x80               ; set if OC1 is an output
I4O5        =   0x4                ; 1 if IC4 is desired.  0 if OC5 is desired
PAMOD       =   0x20               ; Event counter or gated time accumulation





; Port B output only
PORTB       =   0x4

PB0         =   0x1
PB1         =   0x2
PB2         =   0x4
PB3         =   0x8






; stack pointer

STACK_BASE	=	0x1ff






; mode control bits
HPRIO       =   0x3c
SMOD        =   0x40
RBOOT       =   0x80


; interrupt vectors
; not available on cheap models
INTERRUPT_VECTOR_OC1I = 0xffe8     ; output compare 1 interrupt


; system options
OPTION      =   0x39

ADPU        =   0x80          ; A/D converter charge pump enable












