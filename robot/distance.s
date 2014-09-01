; Read the photodiode with ambient reduction

include "init.s"

   ldaa #SCAN		  ; Constant for automatic conversion for channel 0
   staa ADCTL		  ; Start the conversion process

loop:
	ldy #0x1          ; delay for LED cool off
	jsr delay

	jsr getsample     ; get emitter off sample and stow it away
	std EMITTER_OFF

	ldaa #PB0         ; switch on emitter
	staa PORTB

	ldy #0x1          ; delay for LED warmup
	jsr delay

	jsr getsample     ; get emitter on sample and stow it away
	std EMITTER_ON

	ldaa #0x0         ; switch off emitter
	staa PORTB


	ldd EMITTER_ON    ; subtract off value from on value
	subd EMITTER_OFF
	ldx #0x8          ; divide more error out
	idiv
	xgdx

	jsr putdec        ; print it out
	ldab #' '
	jsr putc
	jsr putc
	jsr putc
	jsr putc
	jsr putc
	jsr putc
	jsr putc
	jsr putc
	ldab #'\n'
	jsr putc
	bra loop

;	bcc gotit         ; clamp result to 0
;	ldaa #0x0
;gotit:
;	tab               ; print result
;	jsr putgraph
;	bra loop

;	ldaa #0x0
;	jsr putdec
;	ldab #' '
;	jsr putc
;	bra loop


EMITTER_OFF:
	nop
	nop

EMITTER_ON:
	nop
	nop

include "adc.s"
include "delay.s"
include "putc.s"
include "putdec.s"
;include "putgraph.s"

