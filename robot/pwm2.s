; PWM excercisor.  Takes keyboard input to change angle of servos 2,3.


include "init.s"


FULL_COUNTER = 0x800
CENTER       = 0x1200
FULL_CLOCK   = 0x1d00

ON           = 0x4000
OFF          = 0xea60 - ON

	clra
	clrb
	std TOC2,x                              ; set rising edge

loop:
	ldaa #OM2 + OL2
	staa TCTL1,x

wait_rising:
	brclr TFLG1,x OC2F wait_rising

	ldd #ON                                ; set falling edge
	addd TOC2,x
	std TOC2,x
	bclr TFLG1,x ~OC2F                       ; reset flag

	ldaa #OM2
	staa TCTL1,x

wait_falling:
	brclr TFLG1,x OC2F wait_falling

	ldd #OFF                                ; set rising edge
	addd TOC2,x
	std TOC2,x
	bclr TFLG1,x ~OC2F

	bra loop

