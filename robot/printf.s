; prints the string pointed to by index Y
; X must point to register block
; overwrites Y, B
printf:
		ldab 0,y            ; Copy memory location pointed to by X to B accumulator
		beq done            ; Quit if the value was 0
		jsr putc            ; call putc
		iny                 ; increase index Y
		bra printf          ; loop
done:
		rts


