; counts down x by the number of times stored in y


delay:
	ldx #0xffff
delay1:
	dex
	bne delay1
	dey
	bne delay
	rts
