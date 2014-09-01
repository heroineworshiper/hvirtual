; input compare test
; prints something when falling edge is detected



include "init.s"





ldaa #EDG1B + EDG2B + EDG3B             ; set to falling edge on IC1, IC2, IC3
staa TCTL2,x


loop:
	brclr TFLG1,x IC1F + IC2F + IC3F loop    ; wait for transition

	ldab TFLG1,x                  ; print value of register
	andb #IC1F + IC2F + IC3F
	jsr puthex
	ldab #'\n'
	jsr putc
	bset TFLG1,x 0xff        ; clear transition flag
	bra loop



include "putc.s"
include "puthex.s"



