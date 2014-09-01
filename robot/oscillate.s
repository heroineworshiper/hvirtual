; toggle B0 really fast

include "init.s"


PERIOD = 0x7530

	ldd counter                         ; set first alarm
	std TOC2,x
	ldaa #OL2                           ; set to invert on alarm
	staa TCTL1,x
	ldy #levels                         ; get ready to send levels to user

	ldaa #MULT                          ; Set manual conversions of channels 0 - 3
	staa ADCTL,x                        ; Start the conversion process


loop:
test_alarm:
	brclr TFLG1,x OC2F test_input       ; alarm hasn't gone off yet

	bclr TFLG1,x ~OC2F                  ; clear alarm
	ldd #PERIOD                         ; increase counter by period
	addd counter
	std counter
	std TOC2,x                          ; store result in alarm




test_input:
	brclr ADCTL,x CCF test_output       ; conversion not done yet

	ldab ADR1,x                         ; get conversion result1
	stab an0_level                      ; store for printing
	ldab ADR2,x                         ; get conversion result2
	stab an1_level                      ; store for printing
	ldab ADR3,x                         ; get conversion result3
	stab an2_level                      ; store for printing
	ldab ADR4,x                         ; get conversion result4
	stab an3_level                      ; store for printing
	ldaa #MULT                          ; Set manual conversions of channels 0 - 3
	staa ADCTL,x                        ; Start the conversion process



test_output:
	brclr SCSR,x TDRE loop              ; output not ready

	ldab 0,y                            ; print input level
	stab SCDR,x

	iny                                 ; advance to next level
	cpy #end_levels
	bne loop
	ldy #levels
	bra loop




include "putc.s"

counter:    .word   0x0000

levels:
an0_level:  .byte   0x0
an1_level:  .byte   0x0
an2_level:  .byte   0x0
an3_level:  .byte   0x0
end_levels:


