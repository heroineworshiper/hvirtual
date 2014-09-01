; PWM excercisor.  Takes keyboard input to change angle of servos 2,3.


include "init.s"


FULL_COUNTER = 0x800
CENTER       = 0x1200
FULL_CLOCK   = 0x1d00



; initialize servos

	bset TCTL1,x OM2 + OM3 + OM4 + OM5           ; set all falling edges on alarms
	bset OC1M,x OC1M6 + OC1M5 + OC1M4 + OC1M3    ; set all rising edges on 0
	bset OC1D,x OC1D6 + OC1D5 + OC1D4 + OC1D3    ; set all rising edges on 0
	clra
	clrb
	std TOC1,x

	ldd #CENTER
	std TOC2,x                              ; set falling edge
	std TOC3,x
	std TOC4,x
	std TOC5,x
	bclr PACTL,x I4O5                       ; enable OC5




loop:
; The MC68HC11E1 lacks the right memory configuration to support interrupts.
; We must use a polling event dispatcher instead.  It calls subroutines if 
; they need attention.  This may have less latency than the interrupt mechanism.

input_event:
; Run user input handler if serial port has input
	brclr SCSR,x RDRF loop


	ldab SCDR,x                 ; get character

	cmpb #'s'                   ; got left
	bne input1
	ldy TOC2,x
	dey
	sty TOC2,x
;	jsr debug
	bra loop


input1:
	cmpb #'d'                   ; got center
	bne input2
	ldy #CENTER
	sty TOC2,x
;	jsr debug
	bra loop

input2:
	cmpb #'f'                   ; got right
	bne input3
	ldy TOC2,x
	iny
	sty TOC2,x
;	jsr debug
	bra loop

input3:
	cmpb #'a'                   ; got fast left
	bne input4
	ldd TOC2,x
	subd #0x80
	std TOC2,x
;	jsr debug
	bra loop

input4:
	cmpb #'g'                   ; got fast right
	bne input5
	ldd TOC2,x
	addd #0x80
	std TOC2,x
;	jsr debug
	bra loop

input5:
	cmpb #'x'                   ; got left
	bne input6
	ldy TOC3,x
	dey
	sty TOC3,x
;	jsr debug
	bra loop

input6:
	cmpb #'c'                   ; got center
	bne input7
	ldy #CENTER
	sty TOC3,x
;	jsr debug
	bra loop

input7:
	cmpb #'v'                   ; got right
	bne input8
	ldy TOC3,x
	iny
	sty TOC3,x
;	jsr debug
	bra loop

input8:
	cmpb #'z'                   ; got fast left
	bne input9
	ldd TOC3,x
	subd #0x80
	std TOC3,x
;	jsr debug
	bra loop

input9:
	cmpb #'b'                   ; got fast right
	bne input10
	ldd TOC3,x
	addd #0x80
	std TOC3,x

input10:
	bra loop




