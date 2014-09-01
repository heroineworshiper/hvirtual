include "registers.s"

	lds	#STACK_BASE        ; Set stack pointer
	ldx #REGS              ; Set register pointer for single byte access


	ldy #EEPROM
loop:
	ldab 0,y
;	jsr putc
	jsr puthex
	ldab #' '
	jsr putc
	iny
	bra loop

include "putc.s"
include "puthex.s"
