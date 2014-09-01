;/******************************************************************************
; *                                                                            *
; *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
; *                                                                            *
; *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
; *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
; *  software module in hardware or software products are advised that its     *
; *  use may infringe existing patents or copyrights, and any such use         *
; *  would be at such party's own risk.  The original developer of this        *
; *  software module and his/her company, and subsequent editors and their     *
; *  companies, will have no liability for use of this software or             *
; *  modifications or derivatives thereof.                                     *
; *                                                                            *
; *  XviD is free software; you can redistribute it and/or modify it           *
; *  under the terms of the GNU General Public License as published by         *
; *  the Free Software Foundation; either version 2 of the License, or         *
; *  (at your option) any later version.                                       *
; *                                                                            *
; *  XviD is distributed in the hope that it will be useful, but               *
; *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
; *  GNU General Public License for more details.                              *
; *                                                                            *
; *  You should have received a copy of the GNU General Public License         *
; *  along with this program; if not, write to the Free Software               *
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
; *                                                                            *
; ******************************************************************************/
;
;/******************************************************************************
; *                                                                            *
; *  idct_mmx.asm, MMX optimized inverse DCT                                   *
; *                                                                            *
; *  Initial version provided by Intel at Application Note AP-922              *
; *  http://developer.intel.com/vtune/cbts/strmsimd/922down.htm                *
; *  Copyright (C) 1999 Intel Corporation,                                     *
; *                                                                            *
; *  corrected and further optimized in idct8x8_xmm.asm                        *
; *  Copyright (C) 2000-2001 Peter Gubanov <peter@elecard.net.ru>              *
; *  Rounding trick Copyright (c) 2000 Michel Lespinasse <walken@zoy.org>      *
; *                                                                            *
; *  http://www.elecard.com/peter/idct.html                                    *
; *  http://www.linuxvideo.org/mpeg2dec/                                       *
; *                                                                            *
; *  ported to NASM and some minor changes                                     *
; *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>,                 *
; *                                                                            *
; *  For more information visit the XviD homepage: http://www.xvid.org         *
; *                                                                            *
; ******************************************************************************/
;
;/******************************************************************************
; *                                                                            *
; *  Revision history:                                                         *
; *                                                                            *
; *  07.11.2001 bugfix, idct now meets IEEE-1180 (Isibaar)                     *
; *  02.11.2001 initial version  (Isibaar)                                     *
; *                                                                            *
; ******************************************************************************/


BITS 32

GLOBAL enc__idct_mmx
GLOBAL enc_idct_mmx

%define INP_8	eax + 8
%define OUT_8	eax + 8


%define INP_0	eax
%define INP_1	eax + 16
%define INP_2	eax + 32
%define INP_3	eax + 48
%define INP_4	eax + 64
%define INP_5	eax + 80
%define INP_6	eax + 96
%define INP_7	eax + 112

%define OUT_0	eax
%define OUT_1	eax + 16
%define OUT_2	eax + 32
%define OUT_3	eax + 48
%define OUT_4	eax + 64
%define OUT_5	eax + 80
%define OUT_6	eax + 96
%define OUT_7	eax + 112

%define TABLE_0 ecx
%define TABLE_1 ecx + 64
%define TABLE_2 ecx + 128
%define TABLE_3 ecx + 192
%define TABLE_4 ecx + 256
%define TABLE_5 ecx + 320
%define TABLE_6 ecx + 384
%define TABLE_7 ecx + 448

%define ROUNDER_0 ebx
%define ROUNDER_1 ebx + 8
%define ROUNDER_2 ebx + 16
%define ROUNDER_3 ebx + 24
%define ROUNDER_4 ebx + 32
%define ROUNDER_5 ebx + 40
%define ROUNDER_6 ebx + 48
%define ROUNDER_7 ebx + 56

SECTION .data

ALIGN 16

BITS_INV_ACC	equ 5
SHIFT_INV_ROW	equ 16 - BITS_INV_ACC
SHIFT_INV_COL	equ 1 + BITS_INV_ACC


rounder		dd  65536,  65536, ; rounder_0
			dd   3597,   3597, ; rounder_1
			dd   2260,   2260, ; rounder_2
			dd   1203,   1203, ; rounder_3
			dd      0,      0, ; rounder_4
			dd    120,    120, ; rounder_5
			dd    512,    512, ; rounder_6
			dd    512,    512  ; rounder_7

tg_1_16		dw  13036,  13036,  13036,  13036
tg_2_16		dw  27146,  27146,  27146,  27146
tg_3_16		dw -21746, -21746, -21746, -21746

ocos_4_16	dw  23170,  23170,  23170,  23170

full_tab	dw  16384,  16384,  16384, -16384,   ; movq-> w06 w04 w02 w00 
			dw  21407,   8867,   8867, -21407,   ;        w07 w05 w03 w01 
			dw  16384, -16384,  16384,  16384,   ;        w14 w12 w10 w08 
			dw  -8867,  21407, -21407,  -8867,   ;        w15 w13 w11 w09 
			dw  22725,  12873,  19266, -22725,   ;        w22 w20 w18 w16 
			dw  19266,   4520,  -4520, -12873,   ;        w23 w21 w19 w17 
			dw  12873,   4520,   4520,  19266,   ;        w30 w28 w26 w24 
			dw -22725,  19266, -12873, -22725,   ;        w31 w29 w27 w25 

			dw  22725,  22725,  22725, -22725,   ; movq-> w06 w04 w02 w00 
			dw  29692,  12299,  12299, -29692,   ;        w07 w05 w03 w01 
			dw  22725, -22725,  22725,  22725,   ;        w14 w12 w10 w08 
			dw -12299,  29692, -29692, -12299,   ;        w15 w13 w11 w09 
			dw  31521,  17855,  26722, -31521,   ;        w22 w20 w18 w16 
			dw  26722,   6270,  -6270, -17855,   ;        w23 w21 w19 w17 
			dw  17855,   6270,   6270,  26722,   ;        w30 w28 w26 w24 
			dw -31521,  26722, -17855, -31521,   ;        w31 w29 w27 w25 

			dw  21407,  21407,  21407, -21407,   ; movq-> w06 w04 w02 w00 
			dw  27969,  11585,  11585, -27969,   ;        w07 w05 w03 w01 
			dw  21407, -21407,  21407,  21407,   ;        w14 w12 w10 w08 
			dw -11585,  27969, -27969, -11585,   ;        w15 w13 w11 w09 
			dw  29692,  16819,  25172, -29692,   ;        w22 w20 w18 w16 
			dw  25172,   5906,  -5906, -16819,   ;        w23 w21 w19 w17 
			dw  16819,   5906,   5906,  25172,   ;        w30 w28 w26 w24 
			dw -29692,  25172, -16819, -29692,   ;        w31 w29 w27 w25 

			dw  19266,  19266,  19266, -19266,   ; movq-> w06 w04 w02 w00 
			dw  25172,  10426,  10426, -25172,   ;        w07 w05 w03 w01 
			dw  19266, -19266,  19266,  19266,   ;        w14 w12 w10 w08 
			dw -10426,  25172, -25172, -10426,   ;        w15 w13 w11 w09 
			dw  26722,  15137,  22654, -26722,   ;        w22 w20 w18 w16 
			dw  22654,   5315,  -5315, -15137,   ;        w23 w21 w19 w17 
			dw  15137,   5315,   5315,  22654,   ;        w30 w28 w26 w24 
			dw -26722,  22654, -15137, -26722,   ;        w31 w29 w27 w25 

			dw  16384,  16384,  16384, -16384,   ; movq-> w06 w04 w02 w00 
			dw  21407,   8867,   8867, -21407,   ;        w07 w05 w03 w01 
			dw  16384, -16384,  16384,  16384,   ;        w14 w12 w10 w08 
			dw  -8867,  21407, -21407,  -8867,   ;        w15 w13 w11 w09 
			dw  22725,  12873,  19266, -22725,   ;        w22 w20 w18 w16 
			dw  19266,   4520,  -4520, -12873,   ;        w23 w21 w19 w17 
			dw  12873,   4520,   4520,  19266,   ;        w30 w28 w26 w24 
			dw -22725,  19266, -12873, -22725,   ;        w31 w29 w27 w25 

			dw  19266,  19266,  19266, -19266,   ; movq-> w06 w04 w02 w00 
			dw  25172,  10426,  10426, -25172,   ;        w07 w05 w03 w01 
			dw  19266, -19266,  19266,  19266,   ;        w14 w12 w10 w08 
			dw -10426,  25172, -25172, -10426,   ;        w15 w13 w11 w09 
			dw  26722,  15137,  22654, -26722,   ;        w22 w20 w18 w16 
			dw  22654,   5315,  -5315, -15137,   ;        w23 w21 w19 w17 
			dw  15137,   5315,   5315,  22654,   ;        w30 w28 w26 w24 
			dw -26722,  22654, -15137, -26722,   ;        w31 w29 w27 w25 

			dw  21407,  21407,  21407, -21407,   ; movq-> w06 w04 w02 w00 
			dw  27969,  11585,  11585, -27969,   ;        w07 w05 w03 w01 
			dw  21407, -21407,  21407,  21407,   ;        w14 w12 w10 w08 
			dw -11585,  27969, -27969, -11585,   ;        w15 w13 w11 w09 
			dw  29692,  16819,  25172, -29692,   ;        w22 w20 w18 w16 
			dw  25172,   5906,  -5906, -16819,   ;        w23 w21 w19 w17 
			dw  16819,   5906,   5906,  25172,   ;        w30 w28 w26 w24 
			dw -29692,  25172, -16819, -29692,   ;        w31 w29 w27 w25 

			dw  22725,  22725,  22725, -22725,   ; movq-> w06 w04 w02 w00 
			dw  29692,  12299,  12299, -29692,   ;        w07 w05 w03 w01 
			dw  22725, -22725,  22725,  22725,   ;        w14 w12 w10 w08 
			dw -12299,  29692, -29692, -12299,   ;        w15 w13 w11 w09 
			dw  31521,  17855,  26722, -31521,   ;        w22 w20 w18 w16 
			dw  26722,   6270,  -6270, -17855,   ;        w23 w21 w19 w17 
			dw  17855,   6270,   6270,  26722,   ;        w30 w28 w26 w24 
			dw -31521,  26722, -17855, -31521,   ;        w31 w29 w27 w25 


SECTION .text

;;void enc_idct_mmx(short *block);
enc__idct_mmx:
enc_idct_mmx:
	
    push ebx
    push edi

    mov INP_0, dword [esp + 12]	; block
    mov TABLE_0, full_tab
    mov ROUNDER_0, rounder

	movq mm0, [INP_0] 			; 0	; x3 x2 x1 x0

	movq mm1, [INP_0+8]			; 1	; x7 x6 x5 x4
	movq mm2, mm0 				; 2	; x3 x2 x1 x0

	movq mm3, [TABLE_0]			; 3	; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

	movq mm5, mm0 				; 5	; x5 x1 x4 x0
	punpckldq mm0, mm0 			; x4 x0 x4 x0

	movq mm4, [TABLE_0+8] 		; 4	; w07 w05 w03 w01
	punpckhwd mm2, mm1			; 1	; x7 x3 x6 x2

	pmaddwd mm3, mm0 			; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 				; 6 	; x7 x3 x6 x2

	movq mm1, [TABLE_0+32] 		; 1 	; w22 w20 w18 w16
	punpckldq mm2, mm2 			; x6 x2 x6 x2

	pmaddwd mm4, mm2 			; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 			; x5 x1 x5 x1

	pmaddwd mm0, [TABLE_0+16] 	; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 			; x7 x3 x7 x3

	movq mm7, [TABLE_0+40] 		; 7 	; w23 w21 w19 w17
	pmaddwd mm1, mm5 			; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3, [ROUNDER_0] 		; +rounder
	pmaddwd mm7, mm6 			; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2, [TABLE_0+24] 	; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 				; 4 	; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5, [TABLE_0+48] 	; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 				; 4 	; a1 a0

	pmaddwd mm6, [TABLE_0+56] 	; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 				; 7 	; b1=sum(odd1) b0=sum(odd0)

	paddd mm0, [ROUNDER_0]		; +rounder
	psubd mm3, mm1 				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW 	; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 				; 4 	; a1+b1 a0+b0

	paddd mm0, mm2 				; 2 	; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW 	; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 				; 6 	; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 				; 4 	; a3 a2

	paddd mm0, mm5 				; a3+b3 a2+b2
	psubd mm4, mm5 				; 5 	; a3-b3 a2-b2

	psrad mm0, SHIFT_INV_ROW 	; y3=a3+b3 y2=a2+b2
	psrad mm4, SHIFT_INV_ROW 	; y4=a3-b3 y5=a2-b2

	packssdw mm1, mm0 			; 0 	; y3 y2 y1 y0
	packssdw mm4, mm3 			; 3 	; y6 y7 y4 y5

	movq mm7, mm4 				; 7 	; y6 y7 y4 y5
	psrld mm4, 16 				; 0 y6 0 y4

	pslld mm7, 16 				; y7 0 y5 0
	movq [OUT_0], mm1 			; 1 	; save y3 y2 y1 y0
                             	
	por mm7, mm4 				; 4 	; y7 y6 y5 y4
	movq [OUT_0+8], mm7 		; 7 	; save y7 y6 y5 y4

	movq mm0, [INP_1] 			; 0	; x3 x2 x1 x0

	movq mm1, [INP_1+8]			; 1	; x7 x6 x5 x4
	movq mm2, mm0 				; 2	; x3 x2 x1 x0

	movq mm3, [TABLE_1]			; 3	; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

	movq mm5, mm0 				; 5	; x5 x1 x4 x0
	punpckldq mm0, mm0 			; x4 x0 x4 x0

	movq mm4, [TABLE_1+8] 		; 4	; w07 w05 w03 w01
	punpckhwd mm2, mm1			; 1	; x7 x3 x6 x2

	pmaddwd mm3, mm0 			; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 				; 6 	; x7 x3 x6 x2

	movq mm1, [TABLE_1+32] 		; 1 	; w22 w20 w18 w16
	punpckldq mm2, mm2 			; x6 x2 x6 x2

	pmaddwd mm4, mm2 			; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 			; x5 x1 x5 x1

	pmaddwd mm0, [TABLE_1+16] 		; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 			; x7 x3 x7 x3

	movq mm7, [TABLE_1+40] 		; 7 	; w23 w21 w19 w17
	pmaddwd mm1, mm5 			; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3, [ROUNDER_1] 		; +rounder
	pmaddwd mm7, mm6 			; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2, [TABLE_1+24]	; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 				; 4 	; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5, [TABLE_1+48]	; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 				; 4 	; a1 a0

	pmaddwd mm6, [TABLE_1+56]	; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 				; 7 	; b1=sum(odd1) b0=sum(odd0)

	paddd mm0, [ROUNDER_1]		; +rounder
	psubd mm3, mm1 				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW 	; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 				; 4 	; a1+b1 a0+b0

	paddd mm0, mm2 				; 2 	; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW 	; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 				; 6 	; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 				; 4 	; a3 a2

	paddd mm0, mm5 				; a3+b3 a2+b2
	psubd mm4, mm5 				; 5 	; a3-b3 a2-b2

	psrad mm0, SHIFT_INV_ROW 	; y3=a3+b3 y2=a2+b2
	psrad mm4, SHIFT_INV_ROW 	; y4=a3-b3 y5=a2-b2

	packssdw mm1, mm0 			; 0 	; y3 y2 y1 y0
	packssdw mm4, mm3 			; 3 	; y6 y7 y4 y5

	movq mm7, mm4 				; 7 	; y6 y7 y4 y5
	psrld mm4, 16 				; 0 y6 0 y4

	pslld mm7, 16 				; y7 0 y5 0
	movq [OUT_1], mm1 			; 1 	; save y3 y2 y1 y0
                             	
	por mm7, mm4 				; 4 	; y7 y6 y5 y4
	movq [OUT_1+8], mm7 		; 7 	; save y7 y6 y5 y4

	movq mm0, [INP_2] 			; 0	; x3 x2 x1 x0

	movq mm1, [INP_2+8]			; 1	; x7 x6 x5 x4
	movq mm2, mm0 				; 2	; x3 x2 x1 x0

	movq mm3, [TABLE_2]			; 3	; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

	movq mm5, mm0 				; 5	; x5 x1 x4 x0
	punpckldq mm0, mm0 			; x4 x0 x4 x0

	movq mm4, [TABLE_2+8] 		; 4	; w07 w05 w03 w01
	punpckhwd mm2, mm1			; 1	; x7 x3 x6 x2

	pmaddwd mm3, mm0 			; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 				; 6 	; x7 x3 x6 x2

	movq mm1, [TABLE_2+32] 		; 1 	; w22 w20 w18 w16
	punpckldq mm2, mm2 			; x6 x2 x6 x2

	pmaddwd mm4, mm2 			; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 			; x5 x1 x5 x1

	pmaddwd mm0, [TABLE_2+16] 	; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 			; x7 x3 x7 x3

	movq mm7, [TABLE_2+40] 		; 7 	; w23 w21 w19 w17
	pmaddwd mm1, mm5 			; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3, [ROUNDER_2] 		; +rounder
	pmaddwd mm7, mm6 			; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2, [TABLE_2+24] 	; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 				; 4 	; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5, [TABLE_2+48] 	; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 				; 4 	; a1 a0

	pmaddwd mm6, [TABLE_2+56] 	; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 				; 7 	; b1=sum(odd1) b0=sum(odd0)

	paddd mm0, [ROUNDER_2]		; +rounder
	psubd mm3, mm1 				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW 	; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 				; 4 	; a1+b1 a0+b0

	paddd mm0, mm2 			; 2 	; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW 		; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 			; 6 	; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 			; 4 	; a3 a2

	paddd mm0, mm5 				; a3+b3 a2+b2
	psubd mm4, mm5 			; 5 	; a3-b3 a2-b2

	psrad mm0, SHIFT_INV_ROW 		; y3=a3+b3 y2=a2+b2
	psrad mm4, SHIFT_INV_ROW 		; y4=a3-b3 y5=a2-b2

	packssdw mm1, mm0 		; 0 	; y3 y2 y1 y0
	packssdw mm4, mm3 		; 3 	; y6 y7 y4 y5

	movq mm7, mm4 			; 7 	; y6 y7 y4 y5
	psrld mm4, 16 				; 0 y6 0 y4

	pslld mm7, 16 				; y7 0 y5 0
	movq [OUT_2], mm1 		; 1 	; save y3 y2 y1 y0
                             	
	por mm7, mm4 			; 4 	; y7 y6 y5 y4
	movq [OUT_2+8], mm7 		; 7 	; save y7 y6 y5 y4

	movq mm0, [INP_3] 		; 0	; x3 x2 x1 x0

	movq mm1, [INP_3+8]		; 1	; x7 x6 x5 x4
	movq mm2, mm0 			; 2	; x3 x2 x1 x0

	movq mm3, [TABLE_3]		; 3	; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

	movq mm5, mm0 			; 5	; x5 x1 x4 x0
	punpckldq mm0, mm0 			; x4 x0 x4 x0

	movq mm4, [TABLE_3+8] 	; 4	; w07 w05 w03 w01
	punpckhwd mm2, mm1		; 1	; x7 x3 x6 x2

	pmaddwd mm3, mm0 			; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 			; 6 	; x7 x3 x6 x2

	movq mm1, [TABLE_3+32] 	; 1 	; w22 w20 w18 w16
	punpckldq mm2, mm2 			; x6 x2 x6 x2

	pmaddwd mm4, mm2 			; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 			; x5 x1 x5 x1

	pmaddwd mm0, [TABLE_3+16] 		; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 			; x7 x3 x7 x3

	movq mm7, [TABLE_3+40] 	; 7 	; w23 w21 w19 w17
	pmaddwd mm1, mm5 			; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3, [ROUNDER_3] 		; +rounder
	pmaddwd mm7, mm6 			; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2, [TABLE_3+24] 		; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 			; 4 	; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5, [TABLE_3+48] 		; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 			; 4 	; a1 a0

	pmaddwd mm6, [TABLE_3+56] 		; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 			; 7 	; b1=sum(odd1) b0=sum(odd0)

	paddd mm0, [ROUNDER_3]		; +rounder
	psubd mm3, mm1 				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW 		; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 			; 4 	; a1+b1 a0+b0

	paddd mm0, mm2 			; 2 	; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW 		; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 			; 6 	; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 			; 4 	; a3 a2

	paddd mm0, mm5 				; a3+b3 a2+b2
	psubd mm4, mm5 			; 5 	; a3-b3 a2-b2

	psrad mm0, SHIFT_INV_ROW 		; y3=a3+b3 y2=a2+b2
	psrad mm4, SHIFT_INV_ROW 		; y4=a3-b3 y5=a2-b2

	packssdw mm1, mm0 		; 0 	; y3 y2 y1 y0
	packssdw mm4, mm3 		; 3 	; y6 y7 y4 y5

	movq mm7, mm4 			; 7 	; y6 y7 y4 y5
	psrld mm4, 16 				; 0 y6 0 y4

	pslld mm7, 16 				; y7 0 y5 0
	movq [OUT_3], mm1 		; 1 	; save y3 y2 y1 y0
                             	
	por mm7, mm4 			; 4 	; y7 y6 y5 y4
	movq [OUT_3+8], mm7 		; 7 	; save y7 y6 y5 y4

	movq mm0, [INP_4] 		; 0	; x3 x2 x1 x0

	movq mm1, [INP_4+8]		; 1	; x7 x6 x5 x4
	movq mm2, mm0 			; 2	; x3 x2 x1 x0

	movq mm3, [TABLE_4]		; 3	; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

	movq mm5, mm0 			; 5	; x5 x1 x4 x0
	punpckldq mm0, mm0 			; x4 x0 x4 x0

	movq mm4, [TABLE_4+8] 	; 4	; w07 w05 w03 w01
	punpckhwd mm2, mm1		; 1	; x7 x3 x6 x2

	pmaddwd mm3, mm0 			; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 			; 6 	; x7 x3 x6 x2

	movq mm1, [TABLE_4+32] 	; 1 	; w22 w20 w18 w16
	punpckldq mm2, mm2 			; x6 x2 x6 x2

	pmaddwd mm4, mm2 			; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 			; x5 x1 x5 x1

	pmaddwd mm0, [TABLE_4+16] 		; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 			; x7 x3 x7 x3

	movq mm7, [TABLE_4+40] 	; 7 	; w23 w21 w19 w17
	pmaddwd mm1, mm5 			; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3, [ROUNDER_4] 		; +rounder
	pmaddwd mm7, mm6 			; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2, [TABLE_4+24] 		; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 			; 4 	; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5, [TABLE_4+48] 		; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 			; 4 	; a1 a0

	pmaddwd mm6, [TABLE_4+56] 		; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 			; 7 	; b1=sum(odd1) b0=sum(odd0)

	paddd mm0, [ROUNDER_4]		; +rounder
	psubd mm3, mm1 				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW 		; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 			; 4 	; a1+b1 a0+b0

	paddd mm0, mm2 			; 2 	; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW 		; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 			; 6 	; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 			; 4 	; a3 a2

	paddd mm0, mm5 				; a3+b3 a2+b2
	psubd mm4, mm5 			; 5 	; a3-b3 a2-b2

	psrad mm0, SHIFT_INV_ROW 		; y3=a3+b3 y2=a2+b2
	psrad mm4, SHIFT_INV_ROW 		; y4=a3-b3 y5=a2-b2

	packssdw mm1, mm0 		; 0 	; y3 y2 y1 y0
	packssdw mm4, mm3 		; 3 	; y6 y7 y4 y5

	movq mm7, mm4 			; 7 	; y6 y7 y4 y5
	psrld mm4, 16 				; 0 y6 0 y4

	pslld mm7, 16 				; y7 0 y5 0
	movq [OUT_4], mm1 		; 1 	; save y3 y2 y1 y0
                             	
	por mm7, mm4 			; 4 	; y7 y6 y5 y4
	movq [OUT_4+8], mm7 		; 7 	; save y7 y6 y5 y4

	movq mm0, [INP_5] 		; 0	; x3 x2 x1 x0

	movq mm1, [INP_5+8]		; 1	; x7 x6 x5 x4
	movq mm2, mm0 			; 2	; x3 x2 x1 x0

	movq mm3, [TABLE_5]		; 3	; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

	movq mm5, mm0 			; 5	; x5 x1 x4 x0
	punpckldq mm0, mm0 			; x4 x0 x4 x0

	movq mm4, [TABLE_5+8] 	; 4	; w07 w05 w03 w01
	punpckhwd mm2, mm1		; 1	; x7 x3 x6 x2

	pmaddwd mm3, mm0 			; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 			; 6 	; x7 x3 x6 x2

	movq mm1, [TABLE_5+32] 	; 1 	; w22 w20 w18 w16
	punpckldq mm2, mm2 			; x6 x2 x6 x2

	pmaddwd mm4, mm2 			; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 			; x5 x1 x5 x1

	pmaddwd mm0, [TABLE_5+16] 		; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 			; x7 x3 x7 x3

	movq mm7, [TABLE_5+40] 	; 7 	; w23 w21 w19 w17
	pmaddwd mm1, mm5 			; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3, [ROUNDER_5] 		; +rounder
	pmaddwd mm7, mm6 			; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2, [TABLE_5+24] 		; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 			; 4 	; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5, [TABLE_5+48] 		; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 			; 4 	; a1 a0

	pmaddwd mm6, [TABLE_5+56] 		; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 			; 7 	; b1=sum(odd1) b0=sum(odd0)

	paddd mm0, [ROUNDER_5]		; +rounder
	psubd mm3, mm1 				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW 		; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 			; 4 	; a1+b1 a0+b0

	paddd mm0, mm2 			; 2 	; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW 		; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 			; 6 	; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 			; 4 	; a3 a2

	paddd mm0, mm5 				; a3+b3 a2+b2
	psubd mm4, mm5 			; 5 	; a3-b3 a2-b2

	psrad mm0, SHIFT_INV_ROW 		; y3=a3+b3 y2=a2+b2
	psrad mm4, SHIFT_INV_ROW 		; y4=a3-b3 y5=a2-b2

	packssdw mm1, mm0 		; 0 	; y3 y2 y1 y0
	packssdw mm4, mm3 		; 3 	; y6 y7 y4 y5

	movq mm7, mm4 			; 7 	; y6 y7 y4 y5
	psrld mm4, 16 				; 0 y6 0 y4

	pslld mm7, 16 				; y7 0 y5 0
	movq [OUT_5], mm1 		; 1 	; save y3 y2 y1 y0
                             	
	por mm7, mm4 			; 4 	; y7 y6 y5 y4
	movq [OUT_5+8], mm7 		; 7 	; save y7 y6 y5 y4

	movq mm0, [INP_6] 		; 0	; x3 x2 x1 x0

	movq mm1, [INP_6+8]		; 1	; x7 x6 x5 x4
	movq mm2, mm0 			; 2	; x3 x2 x1 x0

	movq mm3, [TABLE_6]		; 3	; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

	movq mm5, mm0 			; 5	; x5 x1 x4 x0
	punpckldq mm0, mm0 			; x4 x0 x4 x0

	movq mm4, [TABLE_6+8] 	; 4	; w07 w05 w03 w01
	punpckhwd mm2, mm1		; 1	; x7 x3 x6 x2

	pmaddwd mm3, mm0 			; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 			; 6 	; x7 x3 x6 x2

	movq mm1, [TABLE_6+32] 	; 1 	; w22 w20 w18 w16
	punpckldq mm2, mm2 			; x6 x2 x6 x2

	pmaddwd mm4, mm2 			; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 			; x5 x1 x5 x1

	pmaddwd mm0, [TABLE_6+16] 		; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 			; x7 x3 x7 x3

	movq mm7, [TABLE_6+40] 	; 7 	; w23 w21 w19 w17
	pmaddwd mm1, mm5 			; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3, [ROUNDER_6] 		; +rounder
	pmaddwd mm7, mm6 			; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2, [TABLE_6+24] 		; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 			; 4 	; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5, [TABLE_6+48] 		; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 			; 4 	; a1 a0

	pmaddwd mm6, [TABLE_6+56] 		; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 			; 7 	; b1=sum(odd1) b0=sum(odd0)

	paddd mm0, [ROUNDER_6]		; +rounder
	psubd mm3, mm1 				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW 		; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 			; 4 	; a1+b1 a0+b0

	paddd mm0, mm2 			; 2 	; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW 		; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 			; 6 	; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 			; 4 	; a3 a2

	paddd mm0, mm5 				; a3+b3 a2+b2
	psubd mm4, mm5 			; 5 	; a3-b3 a2-b2

	psrad mm0, SHIFT_INV_ROW 		; y3=a3+b3 y2=a2+b2
	psrad mm4, SHIFT_INV_ROW 		; y4=a3-b3 y5=a2-b2

	packssdw mm1, mm0 		; 0 	; y3 y2 y1 y0
	packssdw mm4, mm3 		; 3 	; y6 y7 y4 y5

	movq mm7, mm4 			; 7 	; y6 y7 y4 y5
	psrld mm4, 16 				; 0 y6 0 y4

	pslld mm7, 16 				; y7 0 y5 0
	movq [OUT_6], mm1 		; 1 	; save y3 y2 y1 y0
                             	
	por mm7, mm4 			; 4 	; y7 y6 y5 y4
	movq [OUT_6+8], mm7 		; 7 	; save y7 y6 y5 y4

	movq mm0, [INP_7] 		; 0	; x3 x2 x1 x0

	movq mm1, [INP_7+8]		; 1	; x7 x6 x5 x4
	movq mm2, mm0 			; 2	; x3 x2 x1 x0

	movq mm3, [TABLE_7]		; 3	; w06 w04 w02 w00
	punpcklwd mm0, mm1 			; x5 x1 x4 x0

	movq mm5, mm0 			; 5	; x5 x1 x4 x0
	punpckldq mm0, mm0 			; x4 x0 x4 x0

	movq mm4, [TABLE_7+8] 	; 4	; w07 w05 w03 w01
	punpckhwd mm2, mm1		; 1	; x7 x3 x6 x2

	pmaddwd mm3, mm0 			; x4*w06+x0*w04 x4*w02+x0*w00
	movq mm6, mm2 			; 6 	; x7 x3 x6 x2

	movq mm1, [TABLE_7+32] 	; 1 	; w22 w20 w18 w16
	punpckldq mm2, mm2 			; x6 x2 x6 x2

	pmaddwd mm4, mm2 			; x6*w07+x2*w05 x6*w03+x2*w01
	punpckhdq mm5, mm5 			; x5 x1 x5 x1

	pmaddwd mm0, [TABLE_7+16] 		; x4*w14+x0*w12 x4*w10+x0*w08
	punpckhdq mm6, mm6 			; x7 x3 x7 x3

	movq mm7, [TABLE_7+40] 	; 7 	; w23 w21 w19 w17
	pmaddwd mm1, mm5 			; x5*w22+x1*w20 x5*w18+x1*w16

	paddd mm3, [ROUNDER_7] 		; +rounder
	pmaddwd mm7, mm6 			; x7*w23+x3*w21 x7*w19+x3*w17

	pmaddwd mm2, [TABLE_7+24] 		; x6*w15+x2*w13 x6*w11+x2*w09
	paddd mm3, mm4 			; 4 	; a1=sum(even1) a0=sum(even0)

	pmaddwd mm5, [TABLE_7+48] 		; x5*w30+x1*w28 x5*w26+x1*w24
	movq mm4, mm3 			; 4 	; a1 a0

	pmaddwd mm6, [TABLE_7+56] 		; x7*w31+x3*w29 x7*w27+x3*w25
	paddd mm1, mm7 			; 7 	; b1=sum(odd1) b0=sum(odd0)

	paddd mm0, [ROUNDER_7]		; +rounder
	psubd mm3, mm1 				; a1-b1 a0-b0

	psrad mm3, SHIFT_INV_ROW 		; y6=a1-b1 y7=a0-b0
	paddd mm1, mm4 			; 4 	; a1+b1 a0+b0

	paddd mm0, mm2 			; 2 	; a3=sum(even3) a2=sum(even2)
	psrad mm1, SHIFT_INV_ROW 		; y1=a1+b1 y0=a0+b0

	paddd mm5, mm6 			; 6 	; b3=sum(odd3) b2=sum(odd2)
	movq mm4, mm0 			; 4 	; a3 a2

	paddd mm0, mm5 				; a3+b3 a2+b2
	psubd mm4, mm5 			; 5 	; a3-b3 a2-b2

	psrad mm0, SHIFT_INV_ROW 		; y3=a3+b3 y2=a2+b2
	psrad mm4, SHIFT_INV_ROW 		; y4=a3-b3 y5=a2-b2

	packssdw mm1, mm0 		; 0 	; y3 y2 y1 y0
	packssdw mm4, mm3 		; 3 	; y6 y7 y4 y5

	movq mm7, mm4 			; 7 	; y6 y7 y4 y5
	psrld mm4, 16 				; 0 y6 0 y4

	pslld mm7, 16 				; y7 0 y5 0
	movq [OUT_7], mm1 		; 1 	; save y3 y2 y1 y0
                             	
	por mm7, mm4 			; 4 	; y7 y6 y5 y4
	movq [OUT_7+8], mm7 		; 7 	; save y7 y6 y5 y4

	movq	mm0, [tg_3_16]

	movq	mm3, [INP_0+16*3]
	movq	mm1, mm0			; tg_3_16

	movq	mm5, [INP_0+16*5]
	pmulhw	mm0, mm3			; x3*(tg_3_16-1)

	movq	mm4, [tg_1_16]
	pmulhw	mm1, mm5			; x5*(tg_3_16-1)

	movq	mm7, [INP_0+16*7]
	movq	mm2, mm4			; tg_1_16

	movq	mm6, [INP_0+16*1]
	pmulhw	mm4, mm7			; x7*tg_1_16

	paddsw	mm0, mm3			; x3*tg_3_16
	pmulhw	mm2, mm6			; x1*tg_1_16

	paddsw	mm1, mm3			; x3+x5*(tg_3_16-1)
	psubsw	mm0, mm5			; x3*tg_3_16-x5 = tm35

	movq	mm3, [ocos_4_16]
	paddsw	mm1, mm5			; x3+x5*tg_3_16 = tp35

	paddsw	mm4, mm6			; x1+tg_1_16*x7 = tp17
	psubsw	mm2, mm7			; x1*tg_1_16-x7 = tm17

	movq	mm5, mm4			; tp17
	movq	mm6, mm2			; tm17

	paddsw	mm5, mm1			; tp17+tp35 = b0
	psubsw	mm6, mm0			; tm17-tm35 = b3

	psubsw	mm4, mm1			; tp17-tp35 = t1
	paddsw	mm2, mm0			; tm17+tm35 = t2

	movq	mm7, [tg_2_16]
	movq	mm1, mm4			; t1

;	movq	[SCRATCH+0], mm5	; save b0
	movq	[OUT_0+3*16], mm5	; save b0
	paddsw	mm1, mm2			; t1+t2

;	movq	[SCRATCH+8], mm6	; save b3
	movq	[OUT_0+5*16], mm6	; save b3
	psubsw	mm4, mm2			; t1-t2

	movq	mm5, [INP_0+2*16]
	movq	mm0, mm7			; tg_2_16

	movq	mm6, [INP_0+6*16]
	pmulhw	mm0, mm5			; x2*tg_2_16

	pmulhw	mm7, mm6			; x6*tg_2_16
; slot
	pmulhw	mm1, mm3			; ocos_4_16*(t1+t2) = b1/2
; slot
	movq	mm2, [INP_0+0*16]
	pmulhw	mm4, mm3			; ocos_4_16*(t1-t2) = b2/2

	psubsw	mm0, mm6			; t2*tg_2_16-x6 = tm26
	movq	mm3, mm2			; x0

	movq	mm6, [INP_0+4*16]
	paddsw	mm7, mm5			; x2+x6*tg_2_16 = tp26

	paddsw	mm2, mm6			; x0+x4 = tp04
	psubsw	mm3, mm6			; x0-x4 = tm04

	movq	mm5, mm2			; tp04
	movq	mm6, mm3			; tm04

	psubsw	mm2, mm7			; tp04-tp26 = a3
	paddsw	mm3, mm0			; tm04+tm26 = a1

	paddsw mm1, mm1				; b1
	paddsw mm4, mm4				; b2

	paddsw	mm5, mm7			; tp04+tp26 = a0
	psubsw	mm6, mm0			; tm04-tm26 = a2

	movq	mm7, mm3			; a1
	movq	mm0, mm6			; a2

	paddsw	mm3, mm1			; a1+b1
	paddsw	mm6, mm4			; a2+b2

	psraw	mm3, SHIFT_INV_COL		; dst1
	psubsw	mm7, mm1			; a1-b1

	psraw	mm6, SHIFT_INV_COL		; dst2
	psubsw	mm0, mm4			; a2-b2

;	movq	mm1, [SCRATCH+0]	; load b0
	movq	mm1, [OUT_0+3*16]	; load b0
	psraw	mm7, SHIFT_INV_COL		; dst6

	movq	mm4, mm5			; a0
	psraw	mm0, SHIFT_INV_COL		; dst5

	movq	[OUT_0+1*16], mm3
	paddsw	mm5, mm1			; a0+b0

	movq	[OUT_0+2*16], mm6
	psubsw	mm4, mm1			; a0-b0

;	movq	mm3, [SCRATCH+8]	; load b3
	movq	mm3, [OUT_0+5*16]	; load b3
	psraw	mm5, SHIFT_INV_COL		; dst0

	movq	mm6, mm2			; a3
	psraw	mm4, SHIFT_INV_COL		; dst7

	movq	[OUT_0+5*16], mm0
	paddsw	mm2, mm3			; a3+b3

	movq	[OUT_0+6*16], mm7
	psubsw	mm6, mm3			; a3-b3

	movq	[OUT_0+0*16], mm5
	psraw	mm2, SHIFT_INV_COL		; dst3

	movq	[OUT_0+7*16], mm4
	psraw	mm6, SHIFT_INV_COL		; dst4

	movq	[OUT_0+3*16], mm2

	movq	[OUT_0+4*16], mm6

	movq	mm0, [tg_3_16]

	movq	mm3, [INP_8+16*3]
	movq	mm1, mm0			; tg_3_16

	movq	mm5, [INP_8+16*5]
	pmulhw	mm0, mm3			; x3*(tg_3_16-1)

	movq	mm4, [tg_1_16]
	pmulhw	mm1, mm5			; x5*(tg_3_16-1)

	movq	mm7, [INP_8+16*7]
	movq	mm2, mm4			; tg_1_16

	movq	mm6, [INP_8+16*1]
	pmulhw	mm4, mm7			; x7*tg_1_16

	paddsw	mm0, mm3			; x3*tg_3_16
	pmulhw	mm2, mm6			; x1*tg_1_16

	paddsw	mm1, mm3			; x3+x5*(tg_3_16-1)
	psubsw	mm0, mm5			; x3*tg_3_16-x5 = tm35

	movq	mm3, [ocos_4_16]
	paddsw	mm1, mm5			; x3+x5*tg_3_16 = tp35

	paddsw	mm4, mm6			; x1+tg_1_16*x7 = tp17
	psubsw	mm2, mm7			; x1*tg_1_16-x7 = tm17

	movq	mm5, mm4			; tp17
	movq	mm6, mm2			; tm17

	paddsw	mm5, mm1			; tp17+tp35 = b0
	psubsw	mm6, mm0			; tm17-tm35 = b3

	psubsw	mm4, mm1			; tp17-tp35 = t1
	paddsw	mm2, mm0			; tm17+tm35 = t2

	movq	mm7, [tg_2_16]
	movq	mm1, mm4			; t1

;	movq	[SCRATCH+0], mm5	; save b0
	movq	[OUT_8+3*16], mm5	; save b0
	paddsw	mm1, mm2			; t1+t2

;	movq	[SCRATCH+8], mm6	; save b3
	movq	[OUT_8+5*16], mm6	; save b3
	psubsw	mm4, mm2			; t1-t2

	movq	mm5, [INP_8+2*16]
	movq	mm0, mm7			; tg_2_16

	movq	mm6, [INP_8+6*16]
	pmulhw	mm0, mm5			; x2*tg_2_16

	pmulhw	mm7, mm6			; x6*tg_2_16
; slot
	pmulhw	mm1, mm3			; ocos_4_16*(t1+t2) = b1/2
; slot
	movq	mm2, [INP_8+0*16]
	pmulhw	mm4, mm3			; ocos_4_16*(t1-t2) = b2/2

	psubsw	mm0, mm6			; t2*tg_2_16-x6 = tm26
	movq	mm3, mm2			; x0

	movq	mm6, [INP_8+4*16]
	paddsw	mm7, mm5			; x2+x6*tg_2_16 = tp26

	paddsw	mm2, mm6			; x0+x4 = tp04
	psubsw	mm3, mm6			; x0-x4 = tm04

	movq	mm5, mm2			; tp04
	movq	mm6, mm3			; tm04

	psubsw	mm2, mm7			; tp04-tp26 = a3
	paddsw	mm3, mm0			; tm04+tm26 = a1

	paddsw mm1, mm1				; b1
	paddsw mm4, mm4				; b2

	paddsw	mm5, mm7			; tp04+tp26 = a0
	psubsw	mm6, mm0			; tm04-tm26 = a2

	movq	mm7, mm3			; a1
	movq	mm0, mm6			; a2

	paddsw	mm3, mm1			; a1+b1
	paddsw	mm6, mm4			; a2+b2

	psraw	mm3, SHIFT_INV_COL		; dst1
	psubsw	mm7, mm1			; a1-b1

	psraw	mm6, SHIFT_INV_COL		; dst2
	psubsw	mm0, mm4			; a2-b2

;	movq	mm1, [SCRATCH+0]	; load b0
	movq	mm1, [OUT_8+3*16]	; load b0
	psraw	mm7, SHIFT_INV_COL		; dst6

	movq	mm4, mm5			; a0
	psraw	mm0, SHIFT_INV_COL		; dst5

	movq	[OUT_8+1*16], mm3
	paddsw	mm5, mm1			; a0+b0

	movq	[OUT_8+2*16], mm6
	psubsw	mm4, mm1			; a0-b0

;	movq	mm3, [SCRATCH+8]	; load b3
	movq	mm3, [OUT_8+5*16]	; load b3
	psraw	mm5, SHIFT_INV_COL		; dst0

	movq	mm6, mm2			; a3
	psraw	mm4, SHIFT_INV_COL		; dst7

	movq	[OUT_8+5*16], mm0
	paddsw	mm2, mm3			; a3+b3

	movq	[OUT_8+6*16], mm7
	psubsw	mm6, mm3			; a3-b3

	movq	[OUT_8+0*16], mm5
	psraw	mm2, SHIFT_INV_COL		; dst3

	movq	[OUT_8+7*16], mm4
	psraw	mm6, SHIFT_INV_COL		; dst4

	movq	[OUT_8+3*16], mm2

	movq	[OUT_8+4*16], mm6

	pop edi
	pop ebx
	
	emms

	ret    
