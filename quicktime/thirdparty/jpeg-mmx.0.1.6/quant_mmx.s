;
;  Copyright (C) 2000 Andrew Stevens <as@comlab.ox.ac.uk>

;
;  This program is free software; you can redistribute it and/or
;  modify it under the terms of the GNU General Public License
;  as published by the Free Software Foundation; either version 2
;  of the License, or (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
;
;
;
;  quant_mmx.s:  MMX optimized DCT coefficient quantization sub-routine


global jcquant_mmx
;void jcquant_mmx( INT16 *psrc, INT16, *pdst, INT16 *pqf, INT16 *piqf,
;				   unsigned int shift )


; eax = row counter...
; ebx = pqf   ;  quant factors
; ecx = piqf  ;  (2^16/quant factors) 
; edx = temp
; edi = psrc
; esi = pdst

; mm0 = 0
; mm1 = *piqf -> iqf
; mm2 = *psrc -> src
; mm3 = rounding corrections... / temp
; mm4 = sign
; mm5 = shift
; FREE mm6
; mm7 = temp

			
		
SECTION .text
		

align 32
jcquant_mmx:
	push ebp				; save frame pointer
	mov ebp, esp		; link
	push eax
	push ebx
	push ecx
	push esi     
	push edi

	mov esi, [ebp+8]	; get psrc
	mov edi, [ebp+12]   ; get pdst
	mov ebx, [ebp+16]	; get pqf
	mov ecx,  [ebp+20]  ; get piqf
	movd mm5, [ebp+24]			; get shift
	mov eax,  16		; 16 quads to do
	pxor mm0, mm0
		
	jmp nextquadniq

align 32
nextquadniq:
	movq mm4, mm0			; mm4 = 0
	movq mm2, [esi]			; mm2 = *psrc
	add  esi, 8

								;
	pcmpgtw mm4, mm2       ; mm4 = *psrc < 0
	movq    mm7, mm2       ; mm7 = *psrc
	psllw   mm7, 1         ; mm7 = 2*(*psrc)
	pand    mm7, mm4       ; mm7 = 2*(*psrc)*(*psrc < 0)
	psubw   mm2, mm7       ; mm2 = abs(*psrc)


	;;
	;; Now less do the div...
    ;;  We assuming the DCT coefficients are held to 3 binary places.
    ;; To avoid overflows we first halve them and then later divide by 4
    ;; instead of 8. 

		
	;; First we add rounding factor... (*pqf)/2
	movq    mm7, [ebx]     ; mm7 = *pqf>>1
	psrlw   mm7, 1
	paddw   mm2, mm7       ; mm2 = abs(*psrc)+((*pqf)/2) = "p"
	
			
	movq mm7,mm0			;  mm7 = 0
	pmulhw  mm2, [ecx]      ; mm2 = (p*iqm) >> 16 ~= p/*qm

	;;
	;; To hide the latency lets update some more pointers...
	sub   eax, 1
	add   ecx, 8		; 4 word's 
	psrlw mm2, mm5		; Correct for DCT / quantisation coefficient scaling
	add   ebx, 8

	;;
	;; Now correct the sign mm4 = *psrc < 0
	;;
	
	psubw mm7, mm2        ; mm7 = -2*mm2
	psllw mm7, 1
	pand  mm7, mm4       ; mm7 = -2*mm2 * (*psrc < 0)
	paddw mm2, mm7       ; mm2 = samesign(*psrc, mm2 )

		;;
		;;  Store the quantised words....
		;;

	movq [edi], mm2
	add   edi, 8
	test eax, eax
	
	jnz near nextquadniq

	
return:
	pop edi
	pop esi
	pop ecx
	pop ebx
	pop eax
	pop ebp			; restore stack pointer

	emms			; clear mmx registers
	ret			



