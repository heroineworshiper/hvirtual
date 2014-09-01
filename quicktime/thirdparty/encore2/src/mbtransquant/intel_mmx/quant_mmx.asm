;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx quantization/dequantization
; *
; *	This program is an implementation of a part of one or more MPEG-4
; *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
; *	to use this software module in hardware or software products are
; *	advised that its use may infringe existing patents or copyrights, and
; *	any such use would be at such party's own risk.  The original
; *	developer of this software module and his/her company, and subsequent
; *	editors and their companies, will have no liability for use of this
; *	software or modifications or derivatives thereof.
; *
; *	This program is free software; you can redistribute it and/or modify
; *	it under the terms of the GNU General Public License as published by
; *	the Free Software Foundation; either version 2 of the License, or
; *	(at your option) any later version.
; *
; *	This program is distributed in the hope that it will be useful,
; *	but WITHOUT ANY WARRANTY; without even the implied warranty of
; *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *	GNU General Public License for more details.
; *
; *	You should have received a copy of the GNU General Public License
; *	along with this program; if not, write to the Free Software
; *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * 19.11.2001  quant_inter_mmx now returns sum of abs. coefficient values
; *	04.11.2001	nasm version; (c)2001 peter ross <pross@cs.rmit.edu.au>
; *
; *************************************************************************/


bits 32

section .data


align 16

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

plus_one times 4	dw	 1

;===========================================================================
;
; subtract by Q/2 table
;
;===========================================================================

%macro MMX_SUB  1
times 4 dw %1 / 2
%endmacro

mmx_sub
		MMX_SUB 0
		MMX_SUB 1
		MMX_SUB 2
		MMX_SUB 3
		MMX_SUB 4
		MMX_SUB 5
		MMX_SUB 6
		MMX_SUB 7
		MMX_SUB 8
		MMX_SUB 9
		MMX_SUB 10
		MMX_SUB 11
		MMX_SUB 12
		MMX_SUB 13
		MMX_SUB 14
		MMX_SUB 15
		MMX_SUB 16
		MMX_SUB 17
		MMX_SUB 18
		MMX_SUB 19
		MMX_SUB 20
		MMX_SUB 21
		MMX_SUB 22
		MMX_SUB 23
		MMX_SUB 24
		MMX_SUB 25
		MMX_SUB 26
		MMX_SUB 27
		MMX_SUB 28
		MMX_SUB 29
		MMX_SUB 30
		MMX_SUB 31



;===========================================================================
;
; divide by 2Q table 
;
; use a shift of 16 to take full advantage of _pmulhw_
; for q=1, _pmulhw_ will overflow so it is treated seperately
; (3dnow2 provides _pmulhuw_ which wont cause overflow)
;
;===========================================================================

%macro MMX_DIV  1
times 4 dw  (1 << 16) / (%1 * 2) + 1
%endmacro

mmx_div
		MMX_DIV 1
		MMX_DIV 1
		MMX_DIV 2
		MMX_DIV 3
		MMX_DIV 4
		MMX_DIV 5
		MMX_DIV 6
		MMX_DIV 7
		MMX_DIV 8
		MMX_DIV 9
		MMX_DIV 10
		MMX_DIV 11
		MMX_DIV 12
		MMX_DIV 13
		MMX_DIV 14
		MMX_DIV 15
		MMX_DIV 16
		MMX_DIV 17
		MMX_DIV 18
		MMX_DIV 19
		MMX_DIV 20
		MMX_DIV 21
		MMX_DIV 22
		MMX_DIV 23
		MMX_DIV 24
		MMX_DIV 25
		MMX_DIV 26
		MMX_DIV 27
		MMX_DIV 28
		MMX_DIV 29
		MMX_DIV 30
		MMX_DIV 31



;===========================================================================
;
; add by (odd(Q) ? Q : Q - 1) table
;
;===========================================================================

%macro MMX_ADD  1
%if %1 % 1 != 0
times 4 dw %1
%else
times 4 dw %1 - 1
%endif
%endmacro

mmx_add
		MMX_ADD 0
		MMX_ADD 1
		MMX_ADD 2
		MMX_ADD 3
		MMX_ADD 4
		MMX_ADD 5
		MMX_ADD 6
		MMX_ADD 7
		MMX_ADD 8
		MMX_ADD 9
		MMX_ADD 10
		MMX_ADD 11
		MMX_ADD 12
		MMX_ADD 13
		MMX_ADD 14
		MMX_ADD 15
		MMX_ADD 16
		MMX_ADD 17
		MMX_ADD 18
		MMX_ADD 19
		MMX_ADD 20
		MMX_ADD 21
		MMX_ADD 22
		MMX_ADD 23
		MMX_ADD 24
		MMX_ADD 25
		MMX_ADD 26
		MMX_ADD 27
		MMX_ADD 28
		MMX_ADD 29
		MMX_ADD 30
		MMX_ADD 31


;===========================================================================
;
; multiple by 2Q table
;
;===========================================================================

%macro MMX_MUL  1
times 4 dw %1 * 2
%endmacro

mmx_mul
		MMX_MUL 0
		MMX_MUL 1
		MMX_MUL 2
		MMX_MUL 3
		MMX_MUL 4
		MMX_MUL 5
		MMX_MUL 6
		MMX_MUL 7
		MMX_MUL 8
		MMX_MUL 9
		MMX_MUL 10
		MMX_MUL 11
		MMX_MUL 12
		MMX_MUL 13
		MMX_MUL 14
		MMX_MUL 15
		MMX_MUL 16
		MMX_MUL 17
		MMX_MUL 18
		MMX_MUL 19
		MMX_MUL 20
		MMX_MUL 21
		MMX_MUL 22
		MMX_MUL 23
		MMX_MUL 24
		MMX_MUL 25
		MMX_MUL 26
		MMX_MUL 27
		MMX_MUL 28
		MMX_MUL 29
		MMX_MUL 30
		MMX_MUL 31



section .text


;===========================================================================
;
; void quant_intra_mmx(int16_t * coeff, 
;					const int16_t const * data,
;					const uint32_t quant,
;					const uint32_t dcscalar);
;
;===========================================================================

align 16
cglobal enc_quant_intra_mmx
enc_quant_intra_mmx

		push	esi
		push	edi

		mov	edi, [esp + 8 + 4]		; coeff
		mov	esi, [esp + 8 + 8]		; data
		mov	eax, [esp + 8 + 12]		; quant

		cmp	al, 1
		mov	ecx, 8
		jz	.q1loop

		movq	mm7, [mmx_div + eax * 8]

.loop
		movq	mm0, [esi]		; mm0 = [1st]
		movq	mm3, [esi + 8]	; 
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;
		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		; 
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		;
		pmulhw	mm0, mm7		; mm0 = (mm0 / 2Q) >> 16
		pmulhw	mm3, mm7		;
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4		;
		movq	[edi], mm0
		movq	[edi + 8], mm3
		
		add 	esi, 16
		add 	edi, 16
		dec 	ecx
		jnz 	.loop 

.done	
		mov 	ecx, [esp + 8 + 16]	; dcscalar
		mov 	eax, ecx
		movsx 	edx, word [esi - 128]	; data[0]
		
		shr 	eax, 1			; eax = data[0] + dcscalar>>1
		add 	eax, edx		;

		cdq 				; expand eax -> edx:eax
		idiv	ecx			; eax = edx:eax / dcscalar
		
		mov	[edi - 128], ax		; data[0] = ax

		pop	edi
		pop	esi

		ret				

.q1loop
		movq	mm0, [esi]		; mm0 = [1st]
		movq	mm3, [esi + 8]	; 
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;
		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		; 
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		; 
		psrlw	mm0, 1			; mm0 >>= 1   (/2)
		psrlw	mm3, 1			;
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		pxor	mm3, mm4        	;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3

		add	esi, 16
		add	edi, 16
		dec	ecx
		jnz	.q1loop
		jmp	short .done



;===========================================================================
;
; uint32_t quant_inter_mmx(int16_t * coeff,
;					const int16_t const * data,
;					const uint32_t quant);
;
;===========================================================================

align 16
cglobal enc_quant_inter_mmx
enc_quant_inter_mmx

		push	esi
		push	edi

		mov	edi, [esp + 8 + 4]		; coeff
		mov	esi, [esp + 8 + 8]		; data
		mov	eax, [esp + 8 + 12]		; quant

		pxor	mm5, mm5			; present

		movq	mm6, [mmx_sub + eax * 8]	; sub
					
		cmp	al, 1
		mov	ecx, 8
		jz .q1loop

		movq	mm7, [mmx_div + eax * 8]	; divider

.loop
		movq	mm0, [esi]		; mm0 = [1st]
		movq	mm3, [esi + 8]	; 
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;
		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		; 
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		; 
		psubusw	mm0, mm6		; mm0 -= sub (unsigned, dont go < 0)
		psubusw	mm3, mm6		;
		pmulhw	mm0, mm7		; mm0 = (mm0 / 2Q) >> 16
		pmulhw	mm3, mm7		; 
		movq    mm2, mm0		; mm0 -> mm2
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		psubw	mm0, mm1		; undisplace
		movq    mm1, mm3		; mm3 -> mm1
		pxor	mm3, mm4		
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3
		
		paddw   mm2, mm1		; mm1 + mm2 -> mm2
		pmaddwd mm2, [plus_one] ;
		movq    mm0, mm2		;
		psrlq   mm0, 32			;
		paddd   mm0, mm2		;
		paddd   mm5, mm0		; sum += abs(coeff0) + abs(coeff1) + ... + abs(coeff7)

		add	esi, 16
		add	edi, 16
		dec	ecx
		jnz	.loop 

.done
		movd	eax, mm5		; return sum

		pop	edi
		pop	esi

		ret

.q1loop
		movq	mm0, [esi]		; mm0 = [1st]
		movq	mm3, [esi + 8]		; 
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;
		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		; 
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		; 
		psubusw	mm0, mm6		; mm0 -= sub (unsigned, dont go < 0)
		psubusw	mm3, mm6		;
		psrlw	mm0, 1			; mm0 >>= 1   (/2)
		psrlw	mm3, 1			;
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3
		
		paddw   mm0, mm3		; mm0 + mm3 -> mm0
		pmaddwd mm0, [plus_one]	;
		movq    mm2, mm0		;
		psrlq   mm2, 32			;
		paddd   mm0, mm2		;
		paddd   mm5, mm0		; sum += abs(coeff0) + abs(coeff1) + ... + abs(coeff7)

		add	esi, 16
		add	edi, 16
		dec	ecx
		jnz	.q1loop

		jmp	.done



;===========================================================================
;
; void dequant_intra_mmx(int16_t *data,
;					const int16_t const *coeff,
;					const uint32_t quant,
;					const uint32_t dcscalar);
;
;===========================================================================

align 16
cglobal enc_dequant_intra_mmx
enc_dequant_intra_mmx

		push	esi
		push	edi

		mov	edi, [esp + 8 + 4]		; data
		mov	esi, [esp + 8 + 8]		; coeff
		mov	eax, [esp + 8 + 12]		; quant

		movq	mm6, [mmx_add + eax * 8]
		mov	ecx, 8
		movq	mm7, [mmx_mul + eax * 8]
		
.loop
		movq	mm0, [esi]		; mm0 = [1st]
		movq	mm3, [esi + 8]	; 
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;
		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		; 
		pxor	mm2, mm2		; mm2 = 0
		pxor	mm5, mm5		;
		pcmpeqw	mm2, mm0		; mm2 = (0 == mm0)
		pcmpeqw	mm5, mm3		; 
		pandn   mm2, mm6		; mm2 = (iszero ? 0 : add)
		pandn   mm5, mm6		;
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		; 
		pmullw	mm0, mm7		; mm0 *= 2Q
		pmullw	mm3, mm7		; 
		paddw	mm0, mm2		; mm0 += mm2 (add)
		paddw	mm3, mm5		;
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3

		add	esi, 16
		add	edi, 16
		dec	ecx
		jnz	.loop

		mov	ax, [esi - 128]		; ax = data[0]
		imul 	ax, [esp + 8 + 16]	; eax = data[0] * dcscalar
		mov	[edi - 128], ax		; data[0] = ax

		pop	edi
		pop	esi

		ret



;===========================================================================
;
; void dequant_inter_mmx(int16_t * data,
;					const int16_t * const coeff,
;					const uint32_t quant);
;
;===========================================================================

align 16
cglobal enc_dequant_inter_mmx
enc_dequant_inter_mmx

		push 	esi
		push 	edi

		mov 	edi, [esp + 8 + 4]	; data
		mov 	esi, [esp + 8 + 8]	; coeff
		mov 	eax, [esp + 8 + 12]	; quant

		movq	mm6, [mmx_add + eax * 8]
		movq	mm7, [mmx_mul + eax * 8]
		mov	ecx, 8
				
.loop
		movq	mm0, [esi]		; mm0 = [1st]
		movq	mm3, [esi + 8]		; 
		pxor	mm1, mm1		; mm1 = 0
		pxor	mm4, mm4		;
		pcmpgtw	mm1, mm0		; mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		; 
		pxor	mm2, mm2		; mm2 = 0
		pxor	mm5, mm5		;
		pcmpeqw	mm2, mm0		; mm2 = (0 == mm0)
		pcmpeqw	mm5, mm3		; 
		pandn   mm2, mm6		; mm2 = (iszero ? 0 : add)
		pandn   mm5, mm6		;
		pxor	mm0, mm1		; mm0 = |mm0|
		pxor	mm3, mm4		; 
		psubw	mm0, mm1		; displace
		psubw	mm3, mm4		; 
		pmullw	mm0, mm7		; mm0 *= 2Q
		pmullw	mm3, mm7		; 
		paddw	mm0, mm2		; mm0 += mm2 (add)
		paddw	mm3, mm5		;
		pxor	mm0, mm1		; mm0 *= sign(mm0)
		pxor	mm3, mm4		;
		psubw	mm0, mm1		; undisplace
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3

		add	esi, 16
		add	edi, 16
		dec	ecx
		jnz	.loop

		pop 	edi
		pop 	esi

		ret
