include "registers.s"

	lds	#STACK_BASE        ; Set stack pointer
	ldx #REGS              ; Set register pointer for single byte access
	bset SPCR,x DWOM       ; put port D in wire or mode
	ldd #BAUD1800 * 0x100 + SCI_ENABLE   ; set baud rate and enable serial port 
	staa BAUD,x            ; with as few instructions as possible
	stab SCCR2,x

	ldab 0xfffe
	jsr puthex
	ldab 0xffff
	jsr puthex
	ldab #'\n'
	jsr putc

while:
	ldy #text
	jsr printf
	bra while

include "putc.s"
include "puthex.s"
include "printf.s"


text:	.asciz	"Welcome to the PROM.\n"
