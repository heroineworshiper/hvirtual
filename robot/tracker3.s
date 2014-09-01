; the start of the program will be overwritten with state variables
; current level being written
current_level:


include "init.s"


FULL_COUNTER = 0x800
CENTER       = 0x1200
FULL_CLOCK   = 0x1d00


LASER_PERIOD = 0x7530                   ; 100 hz

; current servo being written
current_servo:



; initialization
	ldaa #OL2 + OL3 + OL4               ; set laser and servos to invert on alarm

; high byte of next servo value being read
servo_temp:


	staa TCTL1,x

	ldaa #MULT                          ; Set manual conversions of channels 0 - 3
	staa ADCTL,x                        ; Start the conversion process

	ldd #levels                         ; Initialize level table
	std current_level
	ldd #servos                         ; Initialize servo table
	std current_servo






loop:
test_laser:
	brclr TFLG1,x OC2F test_servo3      ; laser hasn't gone off yet

	bclr TFLG1,x ~OC2F                  ; clear alarm
	ldd #LASER_PERIOD                   ; increase laser counter by period
	addd TOC2,x
	std TOC2,x                          ; store result in alarm









test_servo3:
	brclr TFLG1,x OC3F test_servo4      ; servo 3 not triggered

	bclr TFLG1,x ~OC3F                  ; reset flag
	brset PORTA,x OC3 high3             ; pin is high
	ldd #0x0                            ; set to pulse start
	std TOC3,x
	bra test_servo4
high3:
	ldd servo3_time                     ; set to pulse end
	std TOC3,x






test_servo4:
	brclr TFLG1,x OC4F test_sensor      ; servo 4 not triggered

	bclr TFLG1,x ~OC4F                  ; clear flag
	brset PORTA,x OC4 high4     		; pin is high
	ldd #0x0
	std TOC4,x                  		; set alarm to pulse start (0)
	bra test_sensor
high4:
	ldd servo4_time             		; set alarm to pulse end
	std TOC4,x










test_sensor:
	brclr ADCTL,x CCF test_output       ; conversion not done yet

; copy sensor results
	ldd ADR1,x                          ; copy sensors 1, 2
	std an0_level
	ldd ADR3,x                          ; copy sensors 3, 4
	std an2_level

	bset ADCTL,x MULT                   ; Start manual conversion of channels 0 - 3







test_output:
	brclr SCSR,x TDRE test_input        ; output not ready

	ldy current_level
	ldab 0,y                            ; print input level
	stab SCDR,x

	iny                                 ; advance to next level
	cpy #end_levels                     ; if pointer at end of levels wrap it
	bne test_output_done
	ldy #levels

test_output_done:
	sty current_level







test_input:
	brclr SCSR,x RDRF loop              ; no data on input
	ldab SCDR,x                         ; get byte of servo value

	ldaa state
	bne low_byte                        ; need low byte

	stab servo_temp                           ; store high byte
	inc state
	bra loop

low_byte:
	ldaa servo_temp                     ; load high byte to finish the word
	ldy current_servo
	std 0,y                             ; store in current servo
	dec state                           ; return to high byte state
	iny                                 ; advance to next servo
	iny
	cpy end_servos                      ; wrap to first servo
	bne low_byte_done
	ldy #servos
low_byte_done:
	sty current_servo

	bra loop






levels:
an0_level:  	  .byte   		0x00
an1_level:  	  .byte   		0x00
an2_level:  	  .byte   		0x00
an3_level:  	  .byte   		0x00
end_levels:






servos:
servo3_time:       .word        0x1200
servo4_time:       .word        0x1200
end_servos:


; 0 = high byte being recieved
; 1 = low byte being recieved
state:             .byte        0x00


