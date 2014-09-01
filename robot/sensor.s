; input compare test
; prints something when edges are detected



include "init.s"




SENSOR0 = IC1F
SENSOR1 = IC2F
SENSOR2 = IC3F
SENSOR3 = IC4F
LASER_TIMER = 0x1


; Laser on
bset PORTB,x PB0
ldy #LASER_TIMER

loop:
; Update laser
;	dey
;	bne laser_done
	ldaa PORTB,x
	eora #PB0
	staa PORTB,x
;	ldy #LASER_TIMER


laser_done:
	ldaa PORTA,x
;	anda #SENSOR0 + SENSOR1 + SENSOR2 + SENSOR3
;	beq loop

	tab
	andb #SENSOR3
	beq off1
	ldab #'*'
	bra print1
off1:
	ldab #'-'
print1:
	jsr putc

	tab
	andb #SENSOR2
	beq off2
	ldab #'*'
	bra print2
off2:
	ldab #'-'
print2:
	jsr putc

	tab
	andb #SENSOR1
	beq off3
	ldab #'*'
	bra print3
off3:
	ldab #'-'
print3:
	jsr putc

	tab
	andb #SENSOR0
	beq off4
	ldab #'*'
	bra print4
off4:
	ldab #'-'
print4:
	jsr putc

	ldab #'\n'
	jsr putc
	bra loop




include "putc.s"




