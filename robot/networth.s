include "init.s"


		ldy #0x96
		ldx #networth
		jsr printf

while:
		xgdy
		cpd #0x1000      ; decriment based on size
		bls step1
		subd #0x100
step1:
		cpd #0x100
		bls step2
		subd #0x10
step2:
		xgdy
		dey
		pshy             ; store Y
		ldab #'$'        ; $
		jsr putc
		xgdy             ; load Y into putdec argument
		jsr putdec       ; print it
		ldab #' '        ; space
		jsr putc
		ldab #'\r'       ; carriage return
		jsr putc
		puly             ; advance Y
		cpy #0x0
		bne while        ; loop until 0
		ldy #0xea66
		ldx #restart
		jsr printf
		bra while

include "printf.s"
include "putc.s"
include "putdec.s"


networth:	.asciz	"Your net worth:\n"
restart:    .asciz  "\nApartment equity loan.\n"
