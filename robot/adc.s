; A/D conversion for channel 0 with 12 bit accuracy.
; Uses oversampling and averaging.
; overwrites all registers except X.
; the result is in D
; the user must set the ADCTL for automatic conversion of channel 0


; test procedure

; include "init.s"
;main:
;	ldaa #SCAN         ; Constant for automatic conversion for channel 0
;	staa ADCTL         ; Start the conversion process
;
;loop:
;	jsr getsample      ; get the sample
;	jsr putdec         ; print decimal result
;	ldab #' '          ; flush the buffer
;	jsr putc
;	jsr putc
;	jsr putc
;	jsr putc
;	jsr putc
;	jsr putc
;	jsr putc
;	jsr putc
;	ldab #'\n'         ; newline
;	jsr putc
;	bra loop
;
;include "putc.s"
;include "putdec.s"



getsample:
; set the counter to the total 8 bit samples to add for a single 16 bit result
	ldy #0x100

	ldd #0x0           ; reset the result
	std result
	std result + 2

get8:
	ldab ADCTL         ; A/D status
	andb #CCF          ; conversion done?
	beq get8           ; not done

	ldaa #0x0
	ldab ADR1          ; get 8 bit result


	addd result + 2    ; add to least 16 bit result with carry
	std result + 2     ; store result
	bcc next           ; add carry to greatest 8 bits
	inc result + 1

next:
	dey                ; decrease sample count
	bne get8           ; need more samples

	ldd result + 1     ; get greatest 16 bits
	rts

; 32 bit sample result
result:
	nop
	nop
	nop
	nop



