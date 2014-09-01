; print the unsigned value of double accumulator D in decimal
; overwrites all registers.  Sets x to register offset.


putdec:
	ldy #0x0         ; skip 0's
	ldx #0x2710      ; print ten thousands
	jsr do_digit
	ldx #0x3e8       ; print thousands
	jsr do_digit
	ldx #0x64        ; print hundreds
	jsr do_digit
	ldx #0xa         ; print tens
	jsr do_digit
	addb #0x30       ; print ones
	jsr putc
	rts


do_digit:
	idiv             ; divide D by X.  Quotient -> X  Remainder -> D
	beq skip         ; skip if 0
	ldy #0x1         ; print all 0's for now on
skip:
	cpy #0x1
	bne really_skip  ; skip 0's and current digit is a 0.
	std remainder    ; store remainder for the putc
	xgdx             ; Put quotient in putc argument
	addb #0x30       ; Convert to ascii
	ldx #REGS
	jsr putc         ; Print it
	ldd remainder    ; Recover remainder
really_skip:
	rts


remainder:      .word       0x0000
