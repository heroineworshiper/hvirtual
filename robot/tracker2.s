; track photodiode with laser
; used photodiode array

include "init.s"





; commands
SET_SERVOS     =    0x01
GET_DETECTOR   =    0x10

; initialize servos

	ldd #0x0
	std TOC2,x                              ; set first alarm
	std TOC3,x                              ; set first alarm
	std TOC4,x                              ; set first alarm
	bset PACTL,x I4O5                       ; enable input capture 4
	clr PORTA                               ; set all pins to 0


	ldaa #OL2 + OL3 + OL4                   ; make pins invert on every alarm
	staa TCTL1,x                            ; the first alarm raises the pin

; set up detector
	ldaa #0xff                              ; detect any transition on sensors
	staa TCTL2,x
	bset TFLG1,x 0xff                       ; clear all input and output flags by setting to 1






loop:
; The MC68HC11E1 lacks the right memory configuration to support interrupts.
; We must use a polling event dispatcher instead.  It calls subroutines if 
; they need attention.  This may have less latency than the interrupt mechanism.

servo_event:
; Update PWM values if any counter has triggered
		brclr TFLG1,x OC2F + OC3F + OC4F servo_done

; The way TFLG1 works, BSET triggers all the bits no matter what the mask is.
; BCLR however, manages to trigger only one bit.
	update_servo2:
		brclr TFLG1,x OC2F update_servo3   ; servo 2 not triggered

		bclr TFLG1,x ~OC2F           ; clear flag
		brset PORTA,x OC2 high2     ; pin is high
		ldd #0x0
		std TOC2,x                  ; set alarm to pulse start (0)
		bra update_servo3
	high2:
		ldd servo2_time             ; set alarm to pulse end
		std TOC2,x



	update_servo3:
		brclr TFLG1,x OC3F update_servo4   ; servo 3 not triggered

		bclr TFLG1,x ~OC3F           ; clear flag
		brset PORTA,x OC3 high3     ; pin is high
		ldd #0x0
		std TOC3,x                  ; set alarm to pulse start (0)
		bra update_servo4
	high3:
		ldd servo3_time             ; set alarm to pulse end
		std TOC3,x


	update_servo4:
		brclr TFLG1,x OC4F servo_done   ; servo 4 not triggered

		bclr TFLG1,x ~OC4F           ; clear flag
		brset PORTA,x OC4 high4     ; pin is high
		ldd #0x0
		std TOC4,x                  ; set alarm to pulse start (0)
		bra servo_done
	high4:
		ldd servo4_time             ; set alarm to pulse end
		std TOC4,x




servo_done:




; set detector_high if the detector's continuous value is high
detector_event:
		brclr PORTA,x IC1 + IC2 + IC3 + IC4 detector_event_done

		ldaa PORTA,x                 ; detector is high
		anda #IC1 + IC2 + IC3 + IC4  ; set the bits in the detector variable
		staa detector_high

detector_event_done:



input_event:
; No command on serial port
		brclr SCSR,x RDRF input_done


		ldab SCDR,x                 ; get command


; servo direction command
		cmpb #SET_SERVOS
		bne input2                  ; false

		jsr getc                    ; load y position into d
		tba
		jsr getc
		std servo3_time             ; store position
		jsr getc                    ; load x position into d
		tba
		jsr getc
		std servo4_time             ; store position

		bra input_done

; debug
;		ldab servo4_time
;		jsr putc
;		ldab servo4_time + 1
;		jsr putc

		bra input_done



; get detector status
input2:
		cmpb #GET_DETECTOR
		bne input_done                    ; false

; if the detector either transitioned or was high, this returns true
		ldab TFLG1,x                      ; test transition flag
		andb #IC1F + IC2F + IC3F + IC4F   ; filter just the inputs
		orab detector_high                ; add continuous value
		clr detector_high                 ; clear continuous value
		bclr TFLG1,x ~(IC1F + IC2F + IC3F + IC4F)          ; clear transition flag
		jsr putc                          ; send it off



input_done:



bra loop


include "getc.s"
include "putc.s"

; pulse width
servo2_time:       .word        0x1200
servo3_time:       .word        0x1200
servo4_time:       .word        0x1200

; detector result to be set if IC1 is high at any time
detector_high:     .byte        0x0





