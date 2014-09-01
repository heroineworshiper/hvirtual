include "init.s"


while:
	ldy #text
	jsr printf

loop_end:
	bra while

include "putc.s"
include "printf.s"


text:	.asciz	"Hello world.\n"

