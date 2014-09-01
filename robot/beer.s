include "init.s"


		ldy #0x63
while:
		pshy             ; store Y
		xgdy             ; load Y into putdec argument
		jsr putdec       ; print it
		ldx #phrase1     ; print phrase 1
		jsr printf
		puly             ; recover Y from stack
		pshy
		xgdy
		jsr putdec
		ldx #phrase2     ; print phrase 2
		jsr printf
		puly
		dey
		pshy
		xgdy
		jsr putdec
		ldx #phrase3
		jsr printf
		puly
		cpy #0x0
		bne while        ; loop until 0

include "printf.s"
include "putc.s"
include "putdec.s"


phrase1:	.asciz	" engineers on the wall.\n"
phrase2:	.asciz	" engineers.\nLay one off, what do you have?\n"
phrase3:	.asciz	" engineers on the wall.\n\n"
