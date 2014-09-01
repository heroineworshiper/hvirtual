;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx motion compensation
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
; *	06.11.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
; *
; *************************************************************************/


bits 32

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

section .text


;===========================================================================
;
; void compensate_mmx(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * const ref,
;				const uint32_t stride);
;
;===========================================================================

align 16
cglobal compensate_mmx
compensate_mmx
		push	esi
		push	edi

		mov	edi, [esp + 8 + 4]		; dct [out]
		mov	edx, [esp + 8 + 8]		; cur [in/out]
		mov	esi, [esp + 8 + 12]		; ref [in]
		mov ecx, [esp + 8 + 16]		; stride [in]
		
		pxor mm7, mm7		; mm7 = zero
		
		mov eax, 8

.loop
		movq mm0, [edx]			; mm01 = [cur]
		movq mm1, mm0
		punpcklbw mm0, mm7
		punpckhbw mm1, mm7

		movq mm2, [esi]			; mm23 = [ref]
		movq mm3, mm2
		
		movq [edx], mm2			; [cur] = [ref]

		punpcklbw mm2, mm7
		punpckhbw mm3, mm7

		psubsw mm0, mm2			; mm01 -= mm23
		psubsw mm1, mm3

		movq [edi], mm0			; dct[] = mm01
		movq [edi + 8], mm1

		add edx, ecx
		add esi, ecx
		add edi, 16

		dec eax
		jnz .loop

		pop edi
		pop esi

		ret