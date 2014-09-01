; bit in the master clock high byte we tie the laser voltage to.
; if it's too low, we'll miss cycles
LASER_MASK         = 0x80

; analog input values (bytes)
LEVELS             = ADR1
LEVELS_END         = ADR4 + 1

; values for first block of servos, the hard servos (words)
SERVOS1            = TOC2 + REGS
SERVOS1_END        = SERVOS1 + 4 * 2

; the start of the program will be overwritten with these state variables.
; this reduces the download time

; values for second block of servos, the soft servos (words)
SERVOS2            =      0x0
SERVO5             =      0x0
SERVO6             =      0x2
SERVO7             =      0x4
SERVOS2_END        =      SERVOS2 + 3 * 2

; current level being read (word)
CURRENT_LEVEL      =      SERVOS2_END

; pointer to current servo being written (word)
CURRENT_SERVO      =      CURRENT_LEVEL + 2

; temporary for high byte of next servo value being read (byte)
SERVO_TEMP         =      CURRENT_SERVO + 2



include "init.s"

	ldaa #MULT                          ; Set manual A/D conversions of channels 0 - 3
	staa ADCTL,x                        ; Start the first A/D conversion

	bset TCTL1,x OM2 + OM3 + OM4 + OM5           ; set all falling edges on alarms
	bset OC1M,x OC1M6 + OC1M5 + OC1M4 + OC1M3    ; set all rising edges on 0
	bset OC1D,x OC1D6 + OC1D5 + OC1D4 + OC1D3    ; set all rising edges on 0
	clra
	clrb
	std TOC1,x


	ldd #LEVELS                         ; Initialize level pointer
	std CURRENT_LEVEL
	ldd #SERVOS1                        ; Initialize servo pointer
	std CURRENT_SERVO

; wait for input data before starting, so the software servos don't go crazy
wait_begin:
	brclr SCSR,x RDRF wait_begin

loop:
; update software servos
	ldd TCNT,x
	ldy SERVO5                          ; ignore if servo value is 0
	beq test_servo6

test_servo5:
	cpd SERVO5                          ; if timer > servo5 value
	bhi servo5_low                      ; set pin to low
	bset PORTB,x PB0                    ; otherwise set pin to high
	bra test_servo6
servo5_low:
	bclr PORTB,x PB0

test_servo6:
	ldy SERVO6                          ; ignore if servo value is 0
	beq test_servo7

	cpd SERVO6                          ; if timer > servo6 value
	bhi servo6_low                      ; set pin to low
	bset PORTB,x PB1                    ; otherwise set pin to high
	bra test_servo7
servo6_low:
	bclr PORTB,x PB1

test_servo7:
	ldy SERVO7                          ; ignore if servo value is 0
	beq update_laser

	cpd SERVO7                          ; if timer > servo7 value
	bhi servo7_low                      ; set pin to low
	bset PORTB,x PB2                    ; otherwise set pin to high
	bra update_laser
servo7_low:
	bclr PORTB,x PB2




; update laser
update_laser:
	brclr TCNT,x LASER_MASK laser_low   ; bit is clear. laser is off
	bset PORTB,x PB3                    ; bit is set.  laser is on
	bra test_output
laser_low:
	bclr PORTB,x PB3












test_output:
	brclr SCSR,x TDRE test_input        ; output not ready

	brclr ADCTL,x CCF test_input        ; conversion not done yet


	ldy CURRENT_LEVEL                   ; print current level
	ldab 0,y                            ; print input level
	stab SCDR,x

	iny                                 ; advance to next level
	cpy #LEVELS_END                     ; if pointer at end of levels wrap it
	bne test_output_done
	ldy #LEVELS
	bset ADCTL,x MULT                   ; Start manual conversion of channels 0 - 3

test_output_done:
	sty CURRENT_LEVEL







test_input:
	brclr SCSR,x RDRF loop              ; no data on input
	ldab SCDR,x                         ; get byte from input

	ldaa state
	bne low_byte                        ; need low byte

	stab SERVO_TEMP                     ; store high byte
	inc state
	bra loop


low_byte:
	ldaa SERVO_TEMP                     ; load high byte to finish the word
	ldy CURRENT_SERVO
	std 0,y                             ; store in current servo
	dec state                           ; return to high byte state
	iny                                 ; advance to next servo
	iny

	cpy #SERVOS1_END                   ; jump to SERVOS2 if we're at the last servo
	bne wrap2
	ldy #SERVOS2

wrap2:
	cpy #SERVOS2_END                   ; jump to SERVOS1 if we're at the last servo
	bne low_byte_done
	ldy #SERVOS1

	
low_byte_done:
	sty CURRENT_SERVO                   ; store current servo


	bra loop







; 0 = high byte being recieved
; 1 = low byte being recieved
state:             .byte        0x00


