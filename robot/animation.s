include "init.s"

while:
		ldx #0x40     ; Load counter
go_right:
		ldab #' '	  ; Load a character into accumulator B
		jsr putc      ; call the putc subroutine
		dex           ; decrease x
		bne go_right  ; repeat until x is 0
		ldx #0x40     ; Load counter
go_left:
		ldab #0x8	  ; Load a character into accumulator B
		jsr putc      ; call the putc subroutine
		dex           ; decrease x
		bne go_left  ; repeat until x is 0
		bra while     ; loop

include "putc.s"
