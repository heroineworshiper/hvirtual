/*
 * the input data is transposed and each 16 bit element in the 8x8 matrix
 * is left aligned:
 * for example in 11...1110000 format
 * If the iDCT is of I macroblock then 0.5 needs to be added to the;DC Component
 * (element[0][0] of the matrix)
 */


/* When we're using MMX assembler, weights are applied here in the 8x8
   iDCT prescale instead of _dv_weight_88_inverse  */
	
/* If we're using MMX assembler, fold weights into the iDCT
   prescale here instead of  _dv_weight_init */

.section .note.GNU-stack, "", @progbits

.text
	.align 4
.globl _dv_idct_block_mmx_x86_64
	.type	 _dv_idct_block_mmx_x86_64,@function
_dv_idct_block_mmx_x86_64:
/* void _dv_idct_88(dv_coeff_t *block) */
	/* argument block=rdi */
	
	mov	 preSC@GOTPCREL(%rip), %r11

/* 
 *	column 0: even part
 *  use V4, V12, V0, V8 to produce V22..V25
 */
	movq		8*12(%r11), %mm0	/* maybe the first mul can be done together */
						/* with the dequantization in iHuff module */
	pmulhw 	8*12(%rdi), %mm0		/* V12 */
	movq		8*4(%r11), %mm1
	pmulhw 	8*4(%rdi), %mm1		/* V4 */
	movq 		(%r11), %mm3
	psraw $1, %mm0				/* t64=t66 */
	pmulhw (%rdi), %mm3		/* V0 */
	movq 8*8(%r11), %mm5		/* duplicate V4 */
	movq %mm1, %mm2			/* added 11/1/96 */
	pmulhw 8*8(%rdi),%mm5		/* V8 */
	psubsw %mm0, %mm1		/* V16 */
	pmulhw x5a825a825a825a82(%rip), %mm1	/* 23170 ->V18 */
	paddsw %mm0, %mm2		/* V17 */
	movq %mm2, %mm0			/* duplicate V17 */
	psraw $1, %mm2			/* t75=t82 */
	psraw $2, %mm0			/* t72 */
	movq %mm3, %mm4			/* duplicate V0 */
	paddsw %mm5, %mm3		/* V19 */
	psubsw %mm5, %mm4		/* V20 ;mm5 free */
/* moved from the block below */
	movq 8*10(%r11), %mm7
	psraw $1, %mm3			/* t74=t81 */
	movq %mm3, %mm6			/* duplicate t74=t81 */
	psraw $2, %mm4			/* t77=t79 */
	psubsw %mm0, %mm1		/* V21 ; mm0 free */
	paddsw %mm2, %mm3		/* V22 */
	movq %mm1, %mm5			/* duplicate V21 */
	paddsw %mm4, %mm1		/* V23 */
	movq %mm3, 8*4(%rdi)		/* V22 */
	psubsw %mm5, %mm4		/* V24; mm5 free */
	movq %mm1, 8*12(%rdi)		/* V23 */
	psubsw %mm2, %mm6		/* V25; mm2 free */
	movq %mm4, (%rdi)		/* V24 */
/* keep mm6 alive all along the next block */
	/* movq %mm6, 8*8(%rdi) 	V25 */
/* column 0: odd part
 * use V2, V6, V10, V14 to produce V31, V39, V40, V41
 */
/* moved above: movq 8*10(%r11), %mm7 */

	pmulhw 8*10(%rdi), %mm7		/* V10 */
	movq 8*6(%r11), %mm0
	pmulhw 8*6(%rdi), %mm0		/* V6 */
	movq 8*2(%r11), %mm5
	movq %mm7, %mm3			/* duplicate V10 */
	pmulhw 8*2(%rdi), %mm5		/* V2 */
	movq 8*14(%r11), %mm4
	psubsw %mm0, %mm7		/* V26 */
	pmulhw 8*14(%rdi), %mm4		/* V14 */
	paddsw %mm0, %mm3		/* V29 ; free mm0 */
	movq %mm7, %mm1			/* duplicate V26 */
	psraw $1, %mm3			/* t91=t94 */
	pmulhw x539f539f539f539f(%rip),%mm7	/* V33 */
	psraw $1, %mm1			/* t96 */
	movq %mm5, %mm0			/* duplicate V2 */
	psraw $2, %mm4			/* t85=t87 */
	paddsw %mm4,%mm5		/* V27 */
	psubsw %mm4, %mm0		/* V28 ; free mm4 */
	movq %mm0, %mm2			/* duplicate V28 */
	psraw $1, %mm5			/* t90=t93 */
	pmulhw x4546454645464546(%rip),%mm0	/* V35 */
	psraw $1, %mm2			/* t97 */
	movq %mm5, %mm4			/* duplicate t90=t93 */
	psubsw %mm2, %mm1		/* V32 ; free mm2 */
	pmulhw x61f861f861f861f8(%rip),%mm1	/* V36 */
	psllw $1, %mm7			/* t107 */
	paddsw %mm3, %mm5		/* V31 */
	psubsw %mm3, %mm4		/* V30 ; free mm3 */
	pmulhw x5a825a825a825a82(%rip),%mm4	/* V34 */
	nop
	psubsw %mm1, %mm0		/* V38 */
	psubsw %mm7, %mm1		/* V37 ; free mm7 */
	psllw $1, %mm1			/* t114 */
/* move from the next block */
	movq %mm6, %mm3			/* duplicate V25 */
/* move from the next block */
	movq 8*4(%rdi), %mm7		/* V22 */
	psllw $1, %mm0			/* t110 */
	psubsw %mm5, %mm0		/* V39 (mm5 needed for next block) */
	psllw $2, %mm4			/* t112 */
/* moved from the next block */
	movq 8*12(%rdi), %mm2		/* V23 */
	psubsw %mm0, %mm4		/* V40 */
	paddsw %mm4, %mm1		/* V41; free mm0 */
/* moved from the next block */
	psllw $1, %mm2			/* t117=t125 */
/* column 0: output butterfly */
/* moved above:
 * movq %mm6, %mm3			duplicate V25
 * movq 8*4(%rdi), %mm7			V22
 * movq 8*12(%rdi), %mm2		V23
 * psllw $1, %mm2			t117=t125
 */
	psubsw %mm1, %mm6		/* tm6 */
	paddsw %mm1, %mm3		/* tm8; free mm1 */
	movq %mm7, %mm1			/* duplicate V22 */
	paddsw %mm5, %mm7		/* tm0 */
	movq %mm3, 8*8(%rdi)		/* tm8; free mm3 */
	psubsw %mm5, %mm1		/* tm14; free mm5 */
	movq %mm6, 8*6(%rdi)		/* tm6; free mm6 */
	movq %mm2, %mm3			/* duplicate t117=t125 */
	movq (%rdi), %mm6		/* V24 */
	paddsw %mm0, %mm2		/* tm2 */
	movq %mm7, (%rdi)		/* tm0; free mm7 */
	psubsw %mm0, %mm3		/* tm12; free mm0 */
	movq %mm1, 8*14(%rdi)		/* tm14; free mm1 */
	psllw $1, %mm6			/* t119=t123 */
	movq %mm2, 8*2(%rdi)		/* tm2; free mm2 */
	movq %mm6, %mm0			/* duplicate t119=t123 */
	movq %mm3, 8*12(%rdi)		/* tm12; free mm3 */
	paddsw %mm4, %mm6		/* tm4 */
/* moved from next block */
	movq 8*5(%r11), %mm1
	psubsw %mm4, %mm0		/* tm10; free mm4 */
/* moved from next block */
	pmulhw 8*5(%rdi), %mm1		/* V5 */
	movq %mm6, 8*4(%rdi)		/* tm4; free mm6 */
	movq %mm0, 8*10(%rdi)		/* tm10; free mm0 */
/* column 1: even part
 * use V5, V13, V1, V9 to produce V56..V59
 */
/* moved to prev block:
 *	movq 8*5(%r11), %mm1
 *	pmulhw 8*5(%rdi), %mm1		 V5
 */
	movq 8*13(%r11), %mm7
	psllw $1, %mm1			/* t128=t130 */
	pmulhw 8*13(%rdi), %mm7		/* V13 */
	movq %mm1, %mm2			/* duplicate t128=t130 */
	movq 8(%r11), %mm3
	pmulhw 8(%rdi), %mm3		/* V1 */
	movq 8*9(%r11), %mm5
	psubsw %mm7, %mm1		/* V50 */
	pmulhw 8*9(%rdi), %mm5		/* V9 */
	paddsw %mm7, %mm2		/* V51 */
	pmulhw x5a825a825a825a82(%rip), %mm1	/* 23170 ->V52 */
	movq %mm2, %mm6			/* duplicate V51 */
	psraw $1, %mm2			/* t138=t144 */
	movq %mm3, %mm4			/* duplicate V1 */
	psraw $2, %mm6			/* t136 */
	paddsw %mm5, %mm3		/* V53 */
	psubsw %mm5, %mm4		/* V54 ;mm5 free */
	movq %mm3, %mm7			/* duplicate V53 */
/* moved from next block */
	movq 8*11(%r11), %mm0
	psraw $1, %mm4			/* t140=t142 */
	psubsw %mm6, %mm1		/* V55 ; mm6 free */
	paddsw %mm2, %mm3		/* V56 */
	movq %mm4, %mm5			/* duplicate t140=t142 */
	paddsw %mm1, %mm4		/* V57 */
	movq %mm3, 8*5(%rdi)		/* V56 */
	psubsw %mm1, %mm5		/* V58; mm1 free */
	movq %mm4, 8*13(%rdi)		/* V57 */
	psubsw %mm2, %mm7		/* V59; mm2 free */
	movq %mm5, 8*9(%rdi)		/* V58 */
/* keep mm7 alive all along the next block
 *	movq %mm7, 8(%rdi)		V59
 * moved above
 *	movq 8*11(%r11), %mm0
 */
	pmulhw 8*11(%rdi), %mm0		/* V11 */
	movq 8*7(%r11), %mm6
	pmulhw 8*7(%rdi), %mm6		/* V7 */
	movq 8*15(%r11), %mm4
	movq %mm0, %mm3			/* duplicate V11 */
	pmulhw 8*15(%rdi), %mm4		/* V15 */
	movq 8*3(%r11), %mm5
	psllw $1, %mm6			/* t146=t152 */
	pmulhw 8*3(%rdi), %mm5		/* V3 */
	paddsw %mm6, %mm0		/* V63 */
/* note that V15 computation has a correction step: 
 * this is a 'magic' constant that rebiases the results to be closer to the
 * expected result.  this magic constant can be refined to reduce the error
 * even more by doing the correction step in a later stage when the number
 * is actually multiplied by 16
 */
	paddw x0005000200010001(%rip), %mm4
	psubsw %mm6, %mm3		/* V60 ; free mm6 */
	psraw $1, %mm0			/* t154=t156 */
	movq %mm3, %mm1			/* duplicate V60 */
	pmulhw x539f539f539f539f(%rip), %mm1	/* V67 */
	movq %mm5, %mm6			/* duplicate V3 */
	psraw $2, %mm4			/* t148=t150 */
	paddsw %mm4, %mm5		/* V61 */
	psubsw %mm4, %mm6		/* V62 ; free mm4 */
	movq %mm5, %mm4			/* duplicate V61 */
	psllw $1, %mm1			/* t169 */
	paddsw %mm0, %mm5		/* V65 -> result */
	psubsw %mm0, %mm4		/* V64 ; free mm0 */
	pmulhw x5a825a825a825a82(%rip), %mm4	/* V68 */
	psraw $1, %mm3			/* t158 */
	psubsw %mm6, %mm3		/* V66 */
	movq %mm5, %mm2			/* duplicate V65 */
	pmulhw x61f861f861f861f8(%rip), %mm3	/* V70 */
	psllw $1, %mm6			/* t165 */
	pmulhw x4546454645464546(%rip), %mm6	/* V69 */
	psraw $1, %mm2			/* t172 */
/* moved from next block */
	movq 8*5(%rdi), %mm0		/* V56 */
	psllw $1, %mm4			/* t174 */
/* moved from next block */
	psraw $1, %mm0			/* t177=t188 */
	nop
	psubsw %mm3, %mm6		/* V72 */
	psubsw %mm1, %mm3		/* V71 ; free mm1 */
	psubsw %mm2, %mm6		/* V73 ; free mm2 */
/* moved from next block */
	psraw $1, %mm5			/* t178=t189 */
	psubsw %mm6, %mm4		/* V74 */
/* moved from next block */
	movq %mm0, %mm1			/* duplicate t177=t188 */
	paddsw %mm4, %mm3		/* V75 */
/* moved from next block */
	paddsw %mm5, %mm0		/* tm1 */
/* location
 *  5 - V56
 * 13 - V57
 *  9 - V58
 *  X - V59, mm7
 *  X - V65, mm5
 *  X - V73, mm6
 *  X - V74, mm4
 *  X - V75, mm3
 * free mm0, mm1 & mm2
 * moved above
 *	movq 8*5(%rdi), %mm0		V56
 *	psllw $1, %mm0			t177=t188 ! new !!
 *	psllw $1, %mm5			t178=t189 ! new !!
 *	movq %mm0, %mm1			duplicate t177=t188
 *	paddsw %mm5, %mm0		tm1
 */
	movq 8*13(%rdi), %mm2		/* V57 */
	psubsw %mm5, %mm1		/* tm15; free mm5 */
	movq %mm0, 8(%rdi)		/* tm1; free mm0 */
	psraw $1, %mm7			/* t182=t184 ! new !! */
/* save the store as used directly in the transpose
 *	movq %mm1, 120(%rdi)		tm15; free mm1
 */
	movq %mm7, %mm5			/* duplicate t182=t184 */
	psubsw %mm3, %mm7		/* tm7 */
	paddsw %mm3, %mm5		/* tm9; free mm3 */
	movq 8*9(%rdi), %mm0		/* V58 */
	movq %mm2, %mm3			/* duplicate V57 */
	movq %mm7, 8*7(%rdi)		/* tm7; free mm7 */
	psubsw %mm6, %mm3		/* tm13 */
	paddsw %mm6, %mm2		/* tm3 ; free mm6 */
/* moved up from the transpose */
	movq %mm3, %mm7
/* moved up from the transpose */
	punpcklwd %mm1, %mm3
	movq %mm0, %mm6			/* duplicate V58 */
	movq %mm2, 8*3(%rdi)		/* tm3; free mm2 */
	paddsw %mm4, %mm0		/* tm5 */
	psubsw %mm4, %mm6		/* tm11; free mm4 */
/* moved up from the transpose */
	punpckhwd %mm1, %mm7
	movq %mm0, 8*5(%rdi)		/* tm5; free mm0 */
/* moved up from the transpose */
	movq %mm5, %mm2
/* transpose - M4 part
 *  ---------       ---------
 * | M1 | M2 |     | M1'| M3'|
 *  ---------  -->  ---------
 * | M3 | M4 |     | M2'| M4'|
 *  ---------       ---------
 * Two alternatives: use full mmword approach so the following code can be
 * scheduled before the transpose is done without stores, or use the faster
 * half mmword stores (when possible)
 */
	movd %mm3, 8*9+4(%rdi)		/* MS part of tmt9 */
	punpcklwd %mm6, %mm5
	movd %mm7, 8*13+4(%rdi)		/* MS part of tmt13 */
	punpckhwd %mm6, %mm2
	movd %mm5, 8*9(%rdi)		/* LS part of tmt9 */
	punpckhdq %mm3, %mm5		/* free mm3 */
	movd %mm2, 8*13(%rdi)		/* LS part of tmt13 */
	punpckhdq %mm7, %mm2		/* free mm7 */
/* moved up from the M3 transpose */
	movq 8*8(%rdi), %mm0
/* moved up from the M3 transpose */
	movq 8*10(%rdi), %mm1
/* moved up from the M3 transpose */
	movq %mm0, %mm3
/* shuffle the rest of the data, and write it with 2 mmword writes */
	movq %mm5, 8*11(%rdi)		/* tmt11 */
/* moved up from the M3 transpose */
	punpcklwd %mm1, %mm0
	movq %mm2, 8*15(%rdi)		/* tmt15 */
/* moved up from the M3 transpose */
	punpckhwd %mm1, %mm3
/* transpose - M3 part
 * moved up to previous code section
 *	movq 8*8(%rdi), %mm0
 *	movq 8*10(%rdi), %mm1
 *	movq %mm0, %mm3
 *	punpcklwd %mm1, %mm0
 *	punpckhwd %mm1, %mm3
 */
	movq 8*12(%rdi), %mm6
	movq 8*14(%rdi), %mm4
	movq %mm6, %mm2
/* shuffle the data and write the lower parts of the transposed in 4 dwords */
	punpcklwd %mm4, %mm6
	movq %mm0, %mm1
	punpckhdq %mm6, %mm1
	movq %mm3, %mm7
	punpckhwd %mm4, %mm2		/* free mm4 */
	punpckldq %mm6, %mm0		/* free mm6 */
/* moved from next block */
	movq 8*13(%rdi), %mm4		/* tmt13 */
	punpckldq %mm2, %mm3
	punpckhdq %mm2, %mm7		/* free mm2 */
/* moved from next block */
	movq %mm3, %mm5			/* duplicate tmt5 */
/* column 1: even part (after transpose)
* moved above
*	movq %mm3, %mm5			duplicate tmt5
*	movq 8*13(%rdi), %mm4		tmt13
*/
	psubsw %mm4, %mm3		/* V134 */
	pmulhw x5a825a825a825a82(%rip), %mm3	/* 23170 ->V136 */
	movq 8*9(%rdi), %mm6		/* tmt9 */
	paddsw %mm4, %mm5		/* V135 ; mm4 free */
	movq %mm0, %mm4			/* duplicate tmt1 */
	paddsw %mm6, %mm0		/* V137 */
	psubsw %mm6, %mm4		/* V138 ; mm6 free */
	psllw $2, %mm3			/* t290 */
	psubsw %mm5, %mm3		/* V139 */
	movq %mm0, %mm6			/* duplicate V137 */
	paddsw %mm5, %mm0		/* V140 */
	movq %mm4, %mm2			/* duplicate V138 */
	paddsw %mm3, %mm2		/* V141 */
	psubsw %mm3, %mm4		/* V142 ; mm3 free */
	movq %mm0, 8*9(%rdi)		/* V140 */
	psubsw %mm5, %mm6		/* V143 ; mm5 free */
/* moved from next block */
	movq 8*11(%rdi), %mm0		/* tmt11 */
	movq %mm2, 8*13(%rdi)		/* V141 */
/* moved from next block */
	movq %mm0, %mm2			/* duplicate tmt11 */
/* column 1: odd part (after transpose) */
/* moved up to the prev block
 *	movq 8*11(%rdi), %mm0		tmt11
 *	movq %mm0, %mm2			duplicate tmt11
 */
	movq 8*15(%rdi), %mm5		/* tmt15 */
	psubsw %mm7, %mm0		/* V144 */
	movq %mm0, %mm3			/* duplicate V144 */
	paddsw %mm7, %mm2		/* V147 ; free mm7 */
	pmulhw x539f539f539f539f(%rip), %mm0	/* 21407-> V151 */
	movq %mm1, %mm7			/* duplicate tmt3 */
	paddsw %mm5, %mm7		/* V145 */
	psubsw %mm5, %mm1		/* V146 ; free mm5 */
	psubsw %mm1, %mm3		/* V150 */
	movq %mm7, %mm5			/* duplicate V145 */
	pmulhw x4546454645464546(%rip), %mm1	/* 17734-> V153 */
	psubsw %mm2, %mm5		/* V148 */
	pmulhw x61f861f861f861f8(%rip), %mm3	/* 25080-> V154 */
	psllw $2, %mm0			/* t311 */
	pmulhw x5a825a825a825a82(%rip), %mm5	/* 23170-> V152 */
	paddsw %mm2, %mm7		/* V149 ; free mm2 */
	psllw $1, %mm1			/* t313 */
	nop	/* without the nop - freeze here for one clock */
	movq %mm3, %mm2			/* duplicate V154 */
	psubsw %mm0, %mm3		/* V155 ; free mm0 */
	psubsw %mm2, %mm1		/* V156 ; free mm2 */
/* moved from the next block */
	movq %mm6, %mm2			/* duplicate V143 */
/* moved from the next block */
	movq 8*13(%rdi), %mm0		/* V141 */
	psllw $1, %mm1			/* t315 */
	psubsw %mm7, %mm1		/* V157 (keep V149) */
	psllw $2, %mm5			/* t317 */
	psubsw %mm1, %mm5		/* V158 */
	psllw $1, %mm3			/* t319 */
	paddsw %mm5, %mm3		/* V159 */
/* column 1: output butterfly (after transform)
 * moved to the prev block
 *	movq %mm6, %mm2			duplicate V143
 *	movq 8*13(%rdi), %mm0		V141
 */
	psubsw %mm3, %mm2		/* V163 */
	paddsw %mm3, %mm6		/* V164 ; free mm3 */
	movq %mm4, %mm3			/* duplicate V142 */
	psubsw %mm5, %mm4		/* V165 ; free mm5 */
	movq %mm2, scratch7(%rip)		/* out7 */
	psraw $4, %mm6
	psraw $4, %mm4
	paddsw %mm5, %mm3		/* V162 */
	movq 8*9(%rdi), %mm2		/* V140 */
	movq %mm0, %mm5			/* duplicate V141 */
/* in order not to perculate this line up,
 * we read 72(%rdi) very near to this location
 */
	movq %mm6, 8*9(%rdi)		/* out9 */
	paddsw %mm1, %mm0		/* V161 */
	movq %mm3, scratch5(%rip)		/* out5 */
	psubsw %mm1, %mm5		/* V166 ; free mm1 */
	movq %mm4, 8*11(%rdi)		/* out11 */
	psraw $4, %mm5
	movq %mm0, scratch3(%rip)		/* out3 */
	movq %mm2, %mm4			/* duplicate V140 */
	movq %mm5, 8*13(%rdi)		/* out13 */
	paddsw %mm7, %mm2		/* V160 */
/* moved from the next block */
	movq 8(%rdi), %mm0
	psubsw %mm7, %mm4		/* V167 ; free mm7 */
/* moved from the next block */
	movq 8*3(%rdi), %mm7
	psraw $4, %mm4
	movq %mm2, scratch1(%rip)		/* out1 */
/* moved from the next block */
	movq %mm0, %mm1
	movq %mm4, 8*15(%rdi)		/* out15 */
/* moved from the next block */
	punpcklwd %mm7, %mm0
/* transpose - M2 parts
 * moved up to the prev block
 *	movq 8(%rdi), %mm0
 *	movq 8*3(%rdi), %mm7
 *	movq %mm0, %mm1
 *	punpcklwd %mm7, %mm0
 */
	movq 8*5(%rdi), %mm5
	punpckhwd %mm7, %mm1
	movq 8*7(%rdi), %mm4
	movq %mm5, %mm3
/* shuffle the data and write the lower parts of the trasposed in 4 dwords */
	movd %mm0, 8*8(%rdi)		/* LS part of tmt8 */
	punpcklwd %mm4, %mm5
	movd %mm1, 8*12(%rdi)		/* LS part of tmt12 */
	punpckhwd %mm4, %mm3
	movd %mm5, 8*8+4(%rdi)		/* MS part of tmt8 */
	punpckhdq %mm5, %mm0		/* tmt10 */
	movd %mm3, 8*12+4(%rdi)		/* MS part of tmt12 */
	punpckhdq %mm3, %mm1		/* tmt14 */
/* transpose - M1 parts */
	movq (%rdi), %mm7
	movq 8*2(%rdi), %mm2
	movq %mm7, %mm6
	movq 8*4(%rdi), %mm5
	punpcklwd %mm2, %mm7
	movq 8*6(%rdi), %mm4
	punpckhwd %mm2, %mm6		/* free mm2 */
	movq %mm5, %mm3
	punpcklwd %mm4, %mm5
	punpckhwd %mm4, %mm3		/* free mm4 */
	movq %mm7, %mm2
	movq %mm6, %mm4
	punpckldq %mm5, %mm7		/* tmt0 */
	punpckhdq %mm5, %mm2		/* tmt2 ; free mm5 */
/* shuffle the rest of the data, and write it with 2 mmword writes */
	punpckldq %mm3, %mm6		/* tmt4 */
/* moved from next block */
	movq %mm2, %mm5			/* duplicate tmt2 */
	punpckhdq %mm3, %mm4		/* tmt6 ; free mm3 */
/* moved from next block */
	movq %mm0, %mm3			/* duplicate tmt10 */
/* column 0: odd part (after transpose)
 *moved up to prev block
 *	movq %mm0, %mm3			duplicate tmt10
 *	movq %mm2, %mm5			duplicate tmt2
 */
	psubsw %mm4, %mm0		/* V110 */
	paddsw %mm4, %mm3		/* V113 ; free mm4 */
	movq %mm0, %mm4			/* duplicate V110 */
	paddsw %mm1, %mm2		/* V111 */
	pmulhw x539f539f539f539f(%rip), %mm0	/* 21407-> V117 */
	psubsw %mm1, %mm5		/* V112 ; free mm1 */
	psubsw %mm5, %mm4		/* V116 */
	movq %mm2, %mm1			/* duplicate V111 */
	pmulhw x4546454645464546(%rip), %mm5	/* 17734-> V119 */
	psubsw %mm3, %mm2		/* V114 */
	pmulhw x61f861f861f861f8(%rip), %mm4	/* 25080-> V120 */
	paddsw %mm3, %mm1		/* V115 ; free mm3 */
	pmulhw x5a825a825a825a82(%rip), %mm2	/* 23170-> V118 */
	psllw $2, %mm0			/* t266 */
	movq %mm1, (%rdi)		/* save V115 */
	psllw $1, %mm5			/* t268 */
	psubsw %mm4, %mm5		/* V122 */
	psubsw %mm0, %mm4		/* V121 ; free mm0 */
	psllw $1, %mm5			/* t270 */
	psubsw %mm1, %mm5		/* V123 ; free mm1 */
	psllw $2, %mm2			/* t272 */
	psubsw %mm5, %mm2		/* V124 (keep V123) */
	psllw $1, %mm4			/* t274 */
	movq %mm5, 8*2(%rdi)		/* save V123 ; free mm5 */
	paddsw %mm2, %mm4		/* V125 (keep V124) */
/* column 0: even part (after transpose) */
	movq 8*12(%rdi), %mm0		/* tmt12 */
	movq %mm6, %mm3			/* duplicate tmt4 */
	psubsw %mm0, %mm6		/* V100 */
	paddsw %mm0, %mm3		/* V101 ; free mm0 */
	pmulhw x5a825a825a825a82(%rip), %mm6	/* 23170 ->V102 */
	movq %mm7, %mm5			/* duplicate tmt0 */
	movq 8*8(%rdi), %mm1		/* tmt8 */
	paddsw %mm1, %mm7		/* V103 */
	psubsw %mm1, %mm5		/* V104 ; free mm1 */
	movq %mm7, %mm0			/* duplicate V103 */
	psllw $2, %mm6			/* t245 */
	paddsw %mm3, %mm7		/* V106 */
	movq %mm5, %mm1			/* duplicate V104 */
	psubsw %mm3, %mm6		/* V105 */
	psubsw %mm3, %mm0		/* V109; free mm3 */
	paddsw %mm6, %mm5		/* V107 */
	psubsw %mm6, %mm1		/* V108 ; free mm6 */
/* column 0: output butterfly (after transform) */
	movq %mm1, %mm3			/* duplicate V108 */
	paddsw %mm2, %mm1		/* out4 */
	psraw $4, %mm1
	psubsw %mm2, %mm3		/* out10 ; free mm2 */
	psraw $4, %mm3
	movq %mm0, %mm6			/* duplicate V109 */
	movq %mm1, 8*4(%rdi)		/* out4 ; free mm1 */
	psubsw %mm4, %mm0		/* out6 */
	movq %mm3, 8*10(%rdi)		/* out10 ; free mm3 */
	psraw $4, %mm0
	paddsw %mm4, %mm6		/* out8 ; free mm4 */
	movq %mm7, %mm1			/* duplicate V106 */
	movq %mm0, 8*6(%rdi)		/* out6 ; free mm0 */
	psraw $4, %mm6
	movq (%rdi), %mm4		/* V115 */
	movq %mm6, 8*8(%rdi)		/* out8 ; free mm6 */
	movq %mm5, %mm2			/* duplicate V107 */
	movq 8*2(%rdi), %mm3		/* V123 */
	paddsw %mm4, %mm7		/* out0 */
/* moved up from next block */
	movq scratch3(%rip), %mm0
	psraw $4, %mm7
/* moved up from next block */
	movq scratch5(%rip), %mm6 
	psubsw %mm4, %mm1		/* out14 ; free mm4 */
	paddsw %mm3, %mm5		/* out2 */
	psraw $4, %mm1
	movq %mm7, (%rdi)		/* out0 ; free mm7 */
	psraw $4, %mm5
	movq %mm1, 8*14(%rdi)		/* out14 ; free mm1 */
	psubsw %mm3, %mm2		/* out12 ; free mm3 */
	movq %mm5, 8*2(%rdi)		/* out2 ; free mm5 */
	psraw $4, %mm2
/* moved up to the prev block */
	movq scratch7(%rip), %mm4
/* moved up to the prev block */
	psraw $4, %mm0
	movq %mm2, 8*12(%rdi)		/* out12 ; free mm2 */
/* moved up to the prev block */
	psraw $4, %mm6
/* move back the data to its correct place
* moved up to the prev block
 *	movq scratch3, %mm0
 *	movq scratch5, %mm6
 *	movq scratch7, %mm4
 *	psraw $4, %mm0
 *	psraw $4, %mm6
*/
	movq scratch1(%rip), %mm1
	psraw $4, %mm4
	movq %mm0, 8*3(%rdi)		/* out3 */
	psraw $4, %mm1
	movq %mm6, 8*5(%rdi)		/* out5 */
	movq %mm4, 8*7(%rdi)		/* out7 */
	movq %mm1, 8(%rdi)		/* out1 */
	
	ret


.data
	.align 8
	.type	x0005000200010001,@object
	.size	x0005000200010001,8
x0005000200010001:
	.long	0x00010001,0x00050002
	.align 8
	.type	x0040000000000000,@object
	.size	x0040000000000000,8
x0040000000000000:
	.long	0, 0x00400000
	.align 8
	.type	x5a825a825a825a82,@object
	.size	x5a825a825a825a82,8
x5a825a825a825a82:
	.long	0x5a825a82, 0x5a825a82
	.align 8
	.type	x539f539f539f539f,@object
	.size	x539f539f539f539f,8
x539f539f539f539f:
	.long	0x539f539f,0x539f539f
	.align 8
	.type	x4546454645464546,@object
	.size	x4546454645464546,8
x4546454645464546:
	.long	0x45464546,0x45464546
	.align 8
	.type	x61f861f861f861f8,@object
	.size	x61f861f861f861f8,8
x61f861f861f861f8:
	.long	0x61f861f8,0x61f861f8
	.align 8
	.type	 scratch1,@object
	.size	 scratch1,8
scratch1:
	.long 0,0
	.align 8
	.type	 scratch3,@object
	.size	 scratch3,8
scratch3:
	.long 0,0
	.align 8
	.type	 scratch5,@object
	.size	 scratch5,8
scratch5:
	.long 0,0
	.align 8
	.type	 scratch7,@object
	.size	 scratch7,8
scratch7:
	.long 0,0
	.type	 x0,@object
	.size	 x0,8
x0:
	.long 0,0
	.align 8

