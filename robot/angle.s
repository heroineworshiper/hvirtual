; track photodiode with laser


include "init.s"


; commands
SET_SERVO3     =    0x01
GET_DETECTOR   =    0x02

; initialize servos

	ldd #0x0
	std TOC2,x                              ; set first alarm
	std TOC3,x                              ; set first alarm
	std TOC4,x                              ; set first alarm
	std TOC5,x                              ; set first alarm
	bclr PACTL,x PAMOD                      ; enable servo 5
	clr PORTA                               ; set all pins to 0


	ldaa #OL2 + OL3 + OL4 + OL5             ; make pins invert on every alarm
	staa TCTL1,x                            ; the first alarm raises the pin

; set up detector
	ldaa #EDG1A + EDG1B                     ; detect any transition on sensor
	staa TCTL2,x
	bset TFLG1,x 0xff                       ; clear all input and output flags by setting to 1






loop:
; The MC68HC11E1 lacks the right memory configuration to support interrupts.
; We must use a polling event dispatcher instead.  It calls subroutines if 
; they need attention.  This may have less latency than the interrupt mechanism.

servo_event:
; Update PWM values if any counter has triggered
		brclr TFLG1,x OC2F + OC3F + OC4F + OC5F servo_done

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
		brclr TFLG1,x OC4F update_servo5   ; servo 4 not triggered

		bclr TFLG1,x ~OC4F           ; clear flag
		brset PORTA,x OC4 high4     ; pin is high
		ldd #0x0
		std TOC4,x                  ; set alarm to pulse start (0)
		bra update_servo5
	high4:
		ldd servo4_time             ; set alarm to pulse end
		std TOC4,x





	update_servo5:
		brclr TFLG1,x OC5F servo_done  ; servo 5 not triggered

		bclr TFLG1,x ~OC5F           ; clear flag
		brset PORTA,x OC5 high5     ; pin is high
		ldd #0x0
		std TOC5,x                  ; set alarm to pulse start (0)
		bra servo_done
	high5:
		ldd servo5_time             ; set alarm to pulse end
		std TOC5,x

servo_done:




; set detector_high if the detector is high
detector_event:
		brclr PORTA,x IC1 detector_event_done
		ldaa #0x1
		staa detector_high

detector_event_done:


input_event:
		brclr SCSR,x RDRF input_done

		ldab SCDR,x                 ; get command


; servo direction command
		cmpb #SET_SERVO3            ; false
		bne input2

		jsr getc                    ; load position into d
		tba
		jsr getc
		std servo3_time             ; store position
		bra input_done

; get detector command
input2:
		cmpb #GET_DETECTOR
		bne input_done              ; false

; if the detector either transitioned or was high, this returns true
		ldab TFLG1,x                ; test transition flag
		andb #IC1F
		orab detector_high          ; test continuous value
		clr detector_high           ; clear continuous value
		bclr TFLG1,x ~IC1F          ; clear transition flag
		jsr putc                    ; send it off



input_done:




bra loop


include "getc.s"
include "putc.s"

; pulse width
servo2_time:       .word        0x1200
servo3_time:       .word        0x1200
servo4_time:       .word        0x1200
servo5_time:       .word        0x1200

; detector result to be set if IC1 is high at any time
detector_high:     .byte        0x0





