include "init.s"


while:
		inc counter

		ldy #help     ; print instructions
		jsr printf
		jsr getc      ; Get keypress

		ldy #result   ; Print debugging information
		pshb
		jsr printf
		pulb
		ldaa #0x0
		jsr putdec
		ldab #0xa
		jsr putc

		ldaa counter
		cmpa #0x0      ; switch statement here
		bne case1
		ldy #fortune0
		bra print_it
case1:
		cmpa #0x1
		bne case2
		ldy #fortune1
		bra print_it
case2:
		cmpa #0x2
		bne case3
		ldy #fortune2
		bra print_it
case3:
		clr counter
		ldy #fortune3
		bra print_it


print_it:
		jsr printf
		bra while




include "getc.s"
include "printf.s"
include "putdec.s"
include "putc.s"


help:	   .asciz	"Press a key to see your fortune.\n"
result:    .asciz   "Got character #"
fortune0:  .asciz   "You're an idiot!\n"
fortune1:  .asciz   "If you can't do it, have someone else do it!\n"
fortune2:  .asciz   "Jesus Christ!\n"
fortune3:  .asciz   "You're a floating point gate array!\n"
counter:   .byte    0x0
