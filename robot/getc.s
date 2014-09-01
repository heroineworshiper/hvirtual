; Get input character with blocking and store it in accumulator B
; overwrites B with character
getc:
	brclr SCSR,x RDRF getc	    ; loop until character
	ldab SCDR,x  	            ; get it
	rts

