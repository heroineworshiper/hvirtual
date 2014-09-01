; print the hex value of byte b

puthex:
	pshb                         ; print top 4 bits
	lsrb
	lsrb
	lsrb
	lsrb
	cmpb #0x9                    ; add ascii base
	bhi do_letter1
	addb #'0'
	jsr putc
	bra next_digit
do_letter1:
	addb #'W'
	jsr putc
next_digit:                      ; print bottom 4 bits
	pulb
	andb #0x0f
	cmpb #0x9                    ; add ascii base
	bhi do_letter2
	addb #'0'
	jsr putc
	rts
do_letter2:
	addb #'W'
	jsr putc
	rts





