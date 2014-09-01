; load program from serial port and program it into the eeprom at the default
; baud rate

include "registers.s"



	lds	#STACK_BASE        ; Set stack pointer
	ldx #REGS              ; Set register pointer for single byte access
	clr BPROT,x            ; enable EEPROM programming
	ldy #EEPROM            ; Start of destination

loop:
	


getc:
	brclr SCSR,x RDRF getc     ; wait for input character
	ldaa SCDR,x

; erase destination byte
	ldab #BYTE + ERASE + EELAT
	stab PPROG,x               ; Set to BYTE erase mode
	stab 0,y                   ; Write any data to address to be erased
	ldab #BYTE + ERASE + EELAT + EPGM
	stab PPROG,x               ; Turn on high voltage
	jsr dly10                  ; Delay 10 ms
	clr PPROG,x                ; Turn off high voltage and set to READ mode

; store new byte
	ldab #EELAT
	stab PPROG,x               ; Enable data bus for programming
	staa 0,y                   ; Store data to EEPROM address
	ldab #EELAT + EPGM
	stab PPROG,x               ; Turn on programming voltage
	jsr dly10                  ; Delay 10 ms
	clr PPROG,x                ; Turn off high voltage and set to READ mode


putc:
	brclr SCSR,x TDRE putc     ; wait for output to be ready on serial port
	ldaa 0,y                   ; get byte from EEPROM address
	staa SCDR,x                ; send byte for verification


	iny
	bra getc

	
dly10:
	pshx
	ldx #0x1388                ; 5000 * 6 / 3000000 = 10ms
dly10loop:
	dex
	bne dly10loop
	pulx
	rts
