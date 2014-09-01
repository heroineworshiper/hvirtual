include "registers.s"


; performs routine commands to initialize the runtime environment

	ldx #REGS              ; Set register pointer for single byte access


; serial port configuration
doserial:
	ldd #BAUD23040 * 0x100 + SCI_ENABLE   ; set baud rate with as few instructions as possible
	staa BAUD,x
	stab SCCR2,x

	ldy #0xffff            ; wait for host to reset its speed
synchronize:
	dey
	bne synchronize



; A/D initialization
	bset OPTION,x #ADPU   ; Set bit in option register to enable A/D charge pump
	lds	#STACK_BASE        ; Set stack pointer

