; prints the char in accumulator B
; x must point to register start before calling this
putc:
		brclr SCSR,x TDRE putc ; wait for output to be ready on serial port
		stab  SCDR,x	       ; send it
		rts

