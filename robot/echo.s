include "init.s"






while:

	ldd 0x2000
	cpd TCNT,x
	bhi while              ; timer is below minimum range
	ldd 0xf000
	cpd TCNT,x
	blo while              ; timer is above minimum range
	brclr SCSR,x RDRF while   ; serial port empty

	jsr getc
	jsr putc
	bra while

include "getc.s"
include "putc.s"
