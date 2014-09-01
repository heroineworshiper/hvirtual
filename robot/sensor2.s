; input compare test
; prints something when falling edge is detected



include "init.s"




bset PACTL,x PAMOD                 ; enable input capture 3

ldaa #0xff                         ; set to all edges on IC1, IC2, IC3, IC4
staa TCTL2,x


loop:
	ldab PORTA,x                   ; print value of register
	andb #IC1F + IC2F + IC3F + IC4F
	beq nothing
	jsr puthex
	ldab #'\n'
	jsr putc
nothing:
	bra loop



include "putc.s"
include "puthex.s"




