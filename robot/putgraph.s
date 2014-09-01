; print bar of length in b with a maximum width of 64
; overwrites a, b

putgraph:
	tba              ; get padding width
	nega             ; subtract total width from bar width to get padding
	adda #0x40
	psha             ; store putc registers
	pshb             
graph1:
	pulb             ; test for completion of bar
	tstb
	beq graph2       ; break
	decb             ; continue
	pshb
	ldab #'*'        ; print graph
	jsr putc 
	bra graph1
graph2:
	pula             ; test for completion of padding
	tsta
	beq graph3       ; break
	deca             ; continue
	psha
	ldab #' '        ; print padding
	jsr putc
	bra graph2
graph3:              ; line feed
	ldab #'\n'
	jsr putc
	rts

