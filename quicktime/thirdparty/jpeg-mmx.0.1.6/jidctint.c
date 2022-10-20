/*
 * jidctint.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a slow-but-accurate integer implementation of the
 * inverse DCT (Discrete Cosine Transform).  In the IJG code, this routine
 * must also perform dequantization of the input coefficients.
 *
 * A 2-D IDCT can be done by 1-D IDCT on each column followed by 1-D IDCT
 * on each row (or vice versa, but it's more convenient to emit a row at
 * a time).  Direct algorithms are also available, but they are much more
 * complex and seem not to be any faster when reduced to code.
 *
 * This implementation is based on an algorithm described in
 *   C. Loeffler, A. Ligtenberg and G. Moschytz, "Practical Fast 1-D DCT
 *   Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
 *   Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
 * The primary algorithm described there uses 11 multiplies and 29 adds.
 * We use their alternate method with 12 multiplies and 32 adds.
 * The advantage of this method is that no data path contains more than one
 * multiplication; this allows a very simple and accurate implementation in
 * scaled fixed-point arithmetic, with a minimal number of shifts.
 */

 /***************************************************************************
 *
 *      This program has been developed by Intel Corporation.  
 *      You have Intel's permission to incorporate this code 
 *      into your product, royalty free.  Intel has various 
 *      intellectual property rights which it may assert under
 *      certain circumstances, such as if another manufacturer's
 *      processor mis-identifies itself as being "GenuineIntel"
 *      when the CPUID instruction is executed.
 *
 *      Intel specifically disclaims all warranties, express or
 *      implied, and all liability, including consequential and
 *      other indirect damages, for the use of this code, 
 *      including liability for infringement of any proprietary
 *      rights, and including the warranties of merchantability
 *      and fitness for a particular purpose.  Intel does not 
 *      assume any responsibility for any errors which may 
 *      appear in this code nor any responsibility to update it.
 *
 *  *  Other brands and names are the property of their respective
 *     owners.
 *
 *  Copyright (c) 1997, Intel Corporation.  All rights reserved.
 ***************************************************************************/


#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		/* Private declarations for DCT subsystem */

#ifdef DCT_ISLOW_SUPPORTED


/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. /* deliberate syntax err */
#endif


/*
 * The poop on this scaling stuff is as follows:
 *
 * Each 1-D IDCT step produces outputs which are a factor of sqrt(N)
 * larger than the true IDCT outputs.  The final outputs are therefore
 * a factor of N larger than desired; since N=8 this can be cured by
 * a simple right shift at the end of the algorithm.  The advantage of
 * this arrangement is that we save two multiplications per 1-D IDCT,
 * because the y0 and y4 inputs need not be divided by sqrt(N).
 *
 * We have to do addition and subtraction of the integer inputs, which
 * is no problem, and multiplication by fractional constants, which is
 * a problem to do in integer arithmetic.  We multiply all the constants
 * by CONST_SCALE and convert them to integer constants (thus retaining
 * CONST_BITS bits of precision in the constants).  After doing a
 * multiplication we have to divide the product by CONST_SCALE, with proper
 * rounding, to produce the correct output.  This division can be done
 * cheaply as a right shift of CONST_BITS bits.  We postpone shifting
 * as long as possible so that partial sums can be added together with
 * full fractional precision.
 *
 * The outputs of the first pass are scaled up by PASS1_BITS bits so that
 * they are represented to better-than-integral precision.  These outputs
 * require BITS_IN_JSAMPLE + PASS1_BITS + 3 bits; this fits in a 16-bit word
 * with the recommended scaling.  (To scale up 12-bit sample data further, an
 * intermediate INT32 array would be needed.)
 *
 * To avoid overflow of the 32-bit intermediate results in pass 2, we must
 * have BITS_IN_JSAMPLE + CONST_BITS + PASS1_BITS <= 26.  Error analysis
 * shows that the values given below are the most effective.
 */

#if BITS_IN_JSAMPLE == 8
#define CONST_BITS  13
#define PASS1_BITS  2
#else
#define CONST_BITS  13
#define PASS1_BITS  1		/* lose a little precision to avoid overflow */
#endif

/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
 * causing a lot of useless floating-point operations at run time.
 * To get around this we use the following pre-calculated constants.
 * If you change CONST_BITS you may want to add appropriate values.
 * (With a reasonable C compiler, you can just rely on the FIX() macro...)
 */

#if CONST_BITS == 13
#define FIX_0_298631336  ((INT32)  2446)	/* FIX(0.298631336) */
#define FIX_0_390180644  ((INT32)  3196)	/* FIX(0.390180644) */
#define FIX_0_541196100  ((INT32)  4433)	/* FIX(0.541196100) */
#define FIX_0_765366865  ((INT32)  6270)	/* FIX(0.765366865) */
#define FIX_0_899976223  ((INT32)  7373)	/* FIX(0.899976223) */
#define FIX_1_175875602  ((INT32)  9633)	/* FIX(1.175875602) */
#define FIX_1_501321110  ((INT32)  12299)	/* FIX(1.501321110) */
#define FIX_1_847759065  ((INT32)  15137)	/* FIX(1.847759065) */
#define FIX_1_961570560  ((INT32)  16069)	/* FIX(1.961570560) */
#define FIX_2_053119869  ((INT32)  16819)	/* FIX(2.053119869) */
#define FIX_2_562915447  ((INT32)  20995)	/* FIX(2.562915447) */
#define FIX_3_072711026  ((INT32)  25172)	/* FIX(3.072711026) */
#else
#define FIX_0_298631336  FIX(0.298631336)
#define FIX_0_390180644  FIX(0.390180644)
#define FIX_0_541196100  FIX(0.541196100)
#define FIX_0_765366865  FIX(0.765366865)
#define FIX_0_899976223  FIX(0.899976223)
#define FIX_1_175875602  FIX(1.175875602)
#define FIX_1_501321110  FIX(1.501321110)
#define FIX_1_847759065  FIX(1.847759065)
#define FIX_1_961570560  FIX(1.961570560)
#define FIX_2_053119869  FIX(2.053119869)
#define FIX_2_562915447  FIX(2.562915447)
#define FIX_3_072711026  FIX(3.072711026)
#endif


/* Multiply an INT32 variable by an INT32 constant to yield an INT32 result.
 * For 8-bit samples with the recommended scaling, all the variable
 * and constant values involved are no more than 16 bits wide, so a
 * 16x16->32 bit multiply can be used instead of a full 32x32 multiply.
 * For 12-bit samples, a full 32-bit multiplication will be needed.
 */

#if BITS_IN_JSAMPLE == 8
#define MULTIPLY(var,const)  MULTIPLY16C16(var,const)
#else
#define MULTIPLY(var,const)  ((var) * (const))
#endif


/* Dequantize a coefficient by multiplying it by the multiplier-table
 * entry; produce an int result.  In this module, both inputs and result
 * are 16 bits or less, so either int or short multiply will work.
 */

#define DEQUANTIZE(coef,quantval)  (((ISLOW_MULT_TYPE) (coef)) * (quantval))





/*
 * Perform dequantization and inverse DCT on one block of coefficients.
 */
#define __int64 unsigned long long
	static const	__int64 fix_029_n089n196	= 0x098ea46e098ea46eLL;
	static const	__int64 fix_n196_n089		= 0xc13be333c13be333LL;
	static const	__int64 fix_205_n256n039	= 0x41b3a18141b3a181LL;
	static const	__int64 fix_n039_n256		= 0xf384adfdf384adfdLL;
	static const	__int64 fix_307n256_n196	= 0x1051c13b1051c13bLL;
	static const	__int64 fix_n256_n196		= 0xadfdc13badfdc13bLL;
	static const	__int64 fix_150_n089n039	= 0x300bd6b7300bd6b7LL;
	static const	__int64 fix_n039_n089		= 0xf384e333f384e333LL;
	static const	__int64 fix_117_117			= 0x25a125a125a125a1LL;
	static const	__int64 fix_054_054p076		= 0x115129cf115129cfLL;
	static const	__int64 fix_054n184_054		= 0xd6301151d6301151LL;

	static const	__int64 fix_054n184 		= 0xd630d630d630d630LL;
	static const	__int64 fix_054				= 0x1151115111511151LL;
	static const	__int64 fix_054p076			= 0x29cf29cf29cf29cfLL;
	static const	__int64 fix_n196p307n256	= 0xd18cd18cd18cd18cLL;
	static const	__int64 fix_n089n039p150	= 0x06c206c206c206c2LL;
	static const	__int64 fix_n256			= 0xadfdadfdadfdadfdLL;
	static const	__int64 fix_n039			= 0xf384f384f384f384LL;
	static const	__int64 fix_n256n039p205	= 0xe334e334e334e334LL;
	static const	__int64 fix_n196			= 0xc13bc13bc13bc13bLL;
	static const	__int64 fix_n089			= 0xe333e333e333e333LL;
	static const	__int64 fix_n089n196p029	= 0xadfcadfcadfcadfcLL;

	static const  __int64 const_0x2xx8		= 0x0000010000000100LL;
	static const  __int64 const_0x0808		= 0x0808080808080808LL;

static inline void domidct8x8llmW(short *inptr, short *quantptr, int *wsptr,
				   JSAMPARRAY outptr, int output_col);

GLOBAL(void)
jpeg_idct_islow (j_decompress_ptr cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col)
{
// Cinelerra:
// Putting on the stack didn't work for some reason.
    int *workspace = malloc(sizeof(int) * (DCTSIZE2 + 4));	/* buffers data between passes */

    domidct8x8llmW(coef_block, 
		compptr->dct_table, 
		workspace, 
		output_buf, 
		output_col);

	free(workspace);
}
  
  
__inline void domidct8x8llmW(short *inptr, short *quantptr, int *wsptr,
				   JSAMPARRAY outptr, int output_col)
{

#if defined(HAVE_MMX_INTEL_MNEMONICS)
	__asm{
    
	mov		edi, quantptr
	mov		ebx, inptr
	mov		esi, wsptr

	add		esi, 0x07		;align wsptr to qword
	and		esi, 0xfffffff8	;align wsptr to qword
	
	mov		eax, esi

	/* Pass 1. */
    
	movq		mm0, [ebx + 8*4]	;p1(1,0)
	pmullw		mm0, [edi + 8*4]	;p1(1,1)
	
    movq		mm1, [ebx + 8*12]	;p1(2,0)
	pmullw		mm1, [edi + 8*12]	;p1(2,1)

	movq		mm6, [ebx + 8*0]	;p1(5,0)
	pmullw		mm6, [edi + 8*0]	;p1(5,1)

	movq		mm2, mm0				;p1(3,0)
	movq		mm7, [ebx + 8*8]	;p1(6,0)

	punpcklwd	mm0, mm1				;p1(3,1)
	pmullw		mm7, [edi + 8*8]	;p1(6,1)

	movq		mm4, mm0				;p1(3,2)
	punpckhwd	mm2, mm1				;p1(3,4)

	pmaddwd		mm0, fix_054n184_054	;p1(3,3)
	movq		mm5, mm2				;p1(3,5)

	pmaddwd		mm2, fix_054n184_054	;p1(3,6)
	pxor		mm1, mm1	;p1(7,0)
    
	pmaddwd		mm4, fix_054_054p076		;p1(4,0)
	punpcklwd   mm1, mm6	;p1(7,1)

	pmaddwd		mm5, fix_054_054p076		;p1(4,1)
	psrad		mm1, 3		;p1(7,2)

	pxor		mm3, mm3	;p1(7,3)

	punpcklwd	mm3, mm7	;p1(7,4)
	
	psrad		mm3, 3		;p1(7,5)
	
	paddd		mm1, mm3	;p1(7,6)
	
	movq		mm3, mm1	;p1(7,7)	

	paddd		mm1, mm4	;p1(7,8)
	
	psubd		mm3, mm4	;p1(7,9)

	movq		[esi + 8*16], mm1	;p1(7,10)
	pxor		mm4, mm4	;p1(7,12)

	movq		[esi + 8*22], mm3	;p1(7,11)
	punpckhwd	mm4, mm6	;p1(7,13)

	psrad		mm4, 3		;p1(7,14)
	pxor		mm1, mm1	;p1(7,15)
	
	punpckhwd	mm1, mm7	;p1(7,16)
	
	psrad		mm1, 3		;p1(7,17)
	
	paddd		mm4, mm1	;p1(7,18)
	
	movq		mm3, mm4	;p1(7,19)	
	pxor		mm1, mm1	;p1(8,0)

	paddd		mm3, mm5	;p1(7,20)
	punpcklwd	mm1, mm6	;p1(8,1)

	psubd		mm4, mm5	;p1(7,21)
	psrad		mm1, 3		;p1(8,2)

	movq		[esi + 8*17], mm3	;p1(7,22)
	pxor		mm5, mm5	;p1(8,3)
	
	movq		[esi + 8*23], mm4	;p1(7,23)
	punpcklwd	mm5, mm7	;p1(8,4)

	psrad		mm5, 3		;p1(8,5)
	pxor		mm4, mm4	;p1(8,12)

	psubd		mm1, mm5	;p1(8,6)	
	punpckhwd	mm4, mm6	;p1(8,13)

	movq		mm3, mm1	;p1(8,7)
	psrad		mm4, 3		;p1(8,14)

	paddd		mm1, mm0	;p1(8,8)
	pxor		mm5, mm5	;p1(8,15)

	psubd		mm3, mm0	;p1(8,9)
	movq		mm0, [ebx + 8*14]	;p1(9,0)

	punpckhwd	mm5, mm7	;p1(8,16)
	pmullw		mm0, [edi + 8*14]	;p1(9,1)

	movq		[esi + 8*18], mm1	;p1(8,10)
	psrad		mm5, 3		;p1(8,17)

	movq		[esi + 8*20], mm3	;p1(8,11)
	psubd		mm4, mm5	;p1(8,18)

	movq		mm3, mm4	;p1(8,19)
	movq		mm1, [ebx + 8*6]	;p1(10,0)

	paddd		mm3, mm2	;p1(8,20)
	pmullw		mm1, [edi + 8*6]	;p1(10,1)

	psubd		mm4, mm2	;p1(8,21)
	movq		mm5, mm0			;p1(11,1)

	movq		[esi + 8*21], mm4	;p1(8,23)

	movq		[esi + 8*19], mm3	;p1(8,22)
	movq		mm4, mm0			;p1(11,0)

	punpcklwd	mm4, mm1			;p1(11,2)
	movq		mm2, [ebx + 8*10]	;p1(12,0)

	punpckhwd	mm5, mm1			;p1(11,4)	
	pmullw		mm2, [edi + 8*10]	;p1(12,1)

	movq		mm3, [ebx + 8*2]	;p1(13,0)

	pmullw		mm3, [edi + 8*2]	;p1(13,1)
	movq		mm6, mm2			;p1(14,0)

	pmaddwd		mm4, fix_117_117	;p1(11,3)
	movq		mm7, mm2			;p1(14,1)
	
	pmaddwd		mm5, fix_117_117	;p1(11,5)
	punpcklwd	mm6, mm3			;p1(14,2)

	pmaddwd		mm6, fix_117_117	;p1(14,3)
	punpckhwd	mm7, mm3			;p1(14,4)
	
	pmaddwd		mm7, fix_117_117	;p1(14,5)
	paddd		mm4, mm6			;p1(15,0)

	paddd		mm5, mm7			;p1(15,1)
   	movq		[esi+8*24], mm4		;p1(15,2)

	movq		[esi+8*25], mm5		;p1(15,3)
	movq		mm6, mm0				;p1(16,0)

	movq		mm7, mm3				;p1(16,3)
	punpcklwd	mm6, mm2				;p1(16,1)
	
	punpcklwd	mm7, mm3				;p1(16,4)
	pmaddwd		mm6, fix_n039_n089		;p1(16,2)

	pmaddwd		mm7, fix_150_n089n039	;p1(16,5)
	movq		mm4, mm0				;p1(16,12)

	paddd		mm6, [esi+8*24]			;p1(16,6)
	punpckhwd	mm4, mm2				;p1(16,13)

	paddd		mm6, mm7				;p1(16,7)
	pmaddwd		mm4, fix_n039_n089		;p1(16,14)

	movq		mm7, mm6				;p1(16,8)
	paddd		mm4, [esi+8*25]			;p1(16,18)

	movq		mm5, mm3				;p1(16,15)
	paddd		mm6, [esi + 8*16]		;p1(16,9)

	punpckhwd	mm5, mm3				;p1(16,16)
	paddd		mm6, const_0x2xx8		;p1(16,10)
	
	psrad		mm6, 9					;p1(16,11)
	pmaddwd		mm5, fix_150_n089n039	;p1(16,17)

	paddd		mm4, mm5				;p1(16,19)

	movq		mm5, mm4				;p1(16,20)

	paddd		mm4, [esi + 8*17]		;p1(16,21)
	
	paddd		mm4, const_0x2xx8		;p1(16,22)
	
	psrad		mm4, 9					;p1(16,23)

	packssdw	mm6, mm4				;p1(16,24)
	
	movq		[esi + 8*0], mm6		;p1(16,25)
		
	movq		mm4, [esi + 8*16]		;p1(16,26)
	
	psubd		mm4, mm7				;p1(16,27)
	movq		mm6, [esi + 8*17]		;p1(16,30)
	
	paddd		mm4, const_0x2xx8		;p1(16,28)
	movq		mm7, mm1				;p1(17,3)

	psrad		mm4, 9					;p1(16,29)
	psubd		mm6, mm5				;p1(16,31)

	paddd		mm6, const_0x2xx8		;p1(16,32)
	punpcklwd	mm7, mm1				;p1(17,4)

	pmaddwd		mm7, fix_307n256_n196	;p1(17,5)
	psrad		mm6, 9					;p1(16,33)

	packssdw	mm4, mm6				;p1(16,34)
	movq		[esi + 8*14], mm4		;p1(16,35)

	movq		mm6, mm0				;p1(17,0)
	movq		mm4, mm0				;p1(17,12)

	punpcklwd	mm6, mm2				;p1(17,1)
	punpckhwd	mm4, mm2				;p1(17,13)
	
	pmaddwd		mm6, fix_n256_n196		;p1(17,2)
	movq		mm5, mm1				;p1(17,15)

	paddd		mm6, [esi+8*24]			;p1(17,6)
	punpckhwd	mm5, mm1				;p1(17,16)
	
	paddd		mm6, mm7				;p1(17,7)
	pmaddwd		mm4, fix_n256_n196		;p1(17,14)

	movq		mm7, mm6				;p1(17,8)
	pmaddwd		mm5, fix_307n256_n196	;p1(17,17)
	
	paddd		mm6, [esi + 8*18]		;p1(17,9)
	
	paddd		mm6, const_0x2xx8		;p1(17,10)
	
	psrad		mm6, 9					;p1(17,11)
	paddd		mm4, [esi+8*25]			;p1(17,18)

	paddd		mm4, mm5				;p1(17,19)

	movq		mm5, mm4				;p1(17,20)
	
	paddd		mm4, [esi + 8*19]		;p1(17,21)
	
	paddd		mm4, const_0x2xx8		;p1(17,22)
	
	psrad		mm4, 9					;p1(17,23)

	packssdw	mm6, mm4				;p1(17,24)
	
	movq		[esi + 8*2], mm6		;p1(17,25)
	
	movq		mm4, [esi + 8*18]		;p1(17,26)

	movq		mm6, [esi + 8*19]		;p1(17,30)
	psubd		mm4, mm7				;p1(17,27)
	
	paddd		mm4, const_0x2xx8		;p1(17,28)
	psubd		mm6, mm5				;p1(17,31)

	psrad		mm4, 9					;p1(17,29)
	paddd		mm6, const_0x2xx8		;p1(17,32)

	psrad		mm6, 9					;p1(17,33)
	movq		mm7, mm2				;p1(18,3)

	packssdw	mm4, mm6				;p1(17,34)
	movq		[esi + 8*12], mm4		;p1(17,35)

	movq		mm6, mm1				;p1(18,0)
	punpcklwd	mm7, mm2				;p1(18,4)

	punpcklwd	mm6, mm3				;p1(18,1)
	pmaddwd		mm7, fix_205_n256n039	;p1(18,5)

	pmaddwd		mm6, fix_n039_n256		;p1(18,2)
	movq		mm4, mm1				;p1(18,12)

	paddd		mm6, [esi+8*24]			;p1(18,6)
	punpckhwd	mm4, mm3				;p1(18,13)

	paddd		mm6, mm7				;p1(18,7)
	pmaddwd		mm4, fix_n039_n256		;p1(18,14)

	movq		mm7, mm6				;p1(18,8)
	movq		mm5, mm2				;p1(18,15)

	paddd		mm6, [esi + 8*20]		;p1(18,9)
	punpckhwd	mm5, mm2				;p1(18,16)
	
	paddd		mm6, const_0x2xx8		;p1(18,10)
	
	psrad		mm6, 9					;p1(18,11)
	pmaddwd		mm5, fix_205_n256n039	;p1(18,17)

	paddd		mm4, [esi+8*25]			;p1(18,18)

	paddd		mm4, mm5				;p1(18,19)

	movq		mm5, mm4				;p1(18,20)
	
	paddd		mm4, [esi + 8*21]		;p1(18,21)
	
	paddd		mm4, const_0x2xx8		;p1(18,22)
	
	psrad		mm4, 9					;p1(18,23)

	packssdw	mm6, mm4				;p1(18,24)
	
	movq		[esi + 8*4], mm6		;p1(18,25)

	movq		mm4, [esi + 8*20]		;p1(18,26)

	psubd		mm4, mm7				;p1(18,27)
	
	paddd		mm4, const_0x2xx8		;p1(18,28)
	movq		mm7, mm0				;p1(19,3)
	
	psrad		mm4, 9					;p1(18,29)
	movq		mm6, [esi + 8*21]		;p1(18,30)
	
	psubd		mm6, mm5				;p1(18,31)
	punpcklwd	mm7, mm0				;p1(19,4)
	
	paddd		mm6, const_0x2xx8		;p1(18,32)
	
	psrad		mm6, 9					;p1(18,33)
	pmaddwd		mm7, fix_029_n089n196	;p1(19,5)

	packssdw	mm4, mm6				;p1(18,34)

	movq		[esi + 8*10], mm4		;p1(18,35)
	movq		mm6, mm3				;p1(19,0)

	punpcklwd	mm6, mm1				;p1(19,1)	
	movq		mm5, mm0				;p1(19,15)
	
	pmaddwd		mm6, fix_n196_n089		;p1(19,2)
	punpckhwd	mm5, mm0				;p1(19,16)


	paddd		mm6, [esi+8*24]			;p1(19,6)
	movq		mm4, mm3				;p1(19,12)

	paddd		mm6, mm7				;p1(19,7)
	punpckhwd	mm4, mm1				;p1(19,13)


	movq		mm7, mm6				;p1(19,8)
	pmaddwd		mm4, fix_n196_n089  	;p1(19,14)

	paddd		mm6, [esi + 8*22]		;p1(19,9)

	pmaddwd		mm5, fix_029_n089n196	;p1(19,17)
	
	paddd		mm6, const_0x2xx8		;p1(19,10)
	
	psrad		mm6, 9					;p1(19,11)
	paddd		mm4, [esi+8*25]			;p1(19,18)

	paddd		mm4, mm5				;p1(19,19)

	movq		mm5, mm4				;p1(19,20)
	paddd		mm4, [esi + 8*23]		;p1(19,21)
	paddd		mm4, const_0x2xx8		;p1(19,22)
	psrad		mm4, 9					;p1(19,23)

	packssdw	mm6, mm4				;p1(19,24)
	movq		[esi + 8*6], mm6		;p1(19,25)

	movq		mm4, [esi + 8*22]		;p1(19,26)
	
	psubd		mm4, mm7				;p1(19,27)
	movq		mm6, [esi + 8*23]		;p1(19,30)
	
	paddd		mm4, const_0x2xx8		;p1(19,28)
	psubd		mm6, mm5				;p1(19,31)

	psrad		mm4, 9					;p1(19,29)
	paddd		mm6, const_0x2xx8		;p1(19,32)

	psrad		mm6, 9					;p1(19,33)

	packssdw	mm4, mm6				;p1(19,34)
	movq		[esi + 8*8], mm4		;p1(19,35)

//************************************************************  

	add		edi, 8
	add		ebx, 8
	add		esi, 8
  
//************************************************************  
	/* Pass 1. */
    
	movq		mm0, [ebx + 8*4]	;p1(1,0)
	pmullw		mm0, [edi + 8*4]	;p1(1,1)
	
    movq		mm1, [ebx + 8*12]	;p1(2,0)
	pmullw		mm1, [edi + 8*12]	;p1(2,1)

	movq		mm6, [ebx + 8*0]	;p1(5,0)
	pmullw		mm6, [edi + 8*0]	;p1(5,1)

	movq		mm2, mm0				;p1(3,0)
	movq		mm7, [ebx + 8*8]	;p1(6,0)

	punpcklwd	mm0, mm1				;p1(3,1)
	pmullw		mm7, [edi + 8*8]	;p1(6,1)

	movq		mm4, mm0				;p1(3,2)
	punpckhwd	mm2, mm1				;p1(3,4)

	pmaddwd		mm0, fix_054n184_054	;p1(3,3)
	movq		mm5, mm2				;p1(3,5)

	pmaddwd		mm2, fix_054n184_054	;p1(3,6)
	pxor		mm1, mm1	;p1(7,0)
    
	pmaddwd		mm4, fix_054_054p076		;p1(4,0)
	punpcklwd   mm1, mm6	;p1(7,1)

	pmaddwd		mm5, fix_054_054p076		;p1(4,1)
	psrad		mm1, 3		;p1(7,2)

	pxor		mm3, mm3	;p1(7,3)

	punpcklwd	mm3, mm7	;p1(7,4)
	
	psrad		mm3, 3		;p1(7,5)
	
	paddd		mm1, mm3	;p1(7,6)
	
	movq		mm3, mm1	;p1(7,7)	

	paddd		mm1, mm4	;p1(7,8)
	
	psubd		mm3, mm4	;p1(7,9)

	movq		[esi + 8*16], mm1	;p1(7,10)
	pxor		mm4, mm4	;p1(7,12)

	movq		[esi + 8*22], mm3	;p1(7,11)
	punpckhwd	mm4, mm6	;p1(7,13)

	psrad		mm4, 3		;p1(7,14)
	pxor		mm1, mm1	;p1(7,15)
	
	punpckhwd	mm1, mm7	;p1(7,16)
	
	psrad		mm1, 3		;p1(7,17)
	
	paddd		mm4, mm1	;p1(7,18)
	
	movq		mm3, mm4	;p1(7,19)	
	pxor		mm1, mm1	;p1(8,0)

	paddd		mm3, mm5	;p1(7,20)
	punpcklwd	mm1, mm6	;p1(8,1)

	psubd		mm4, mm5	;p1(7,21)
	psrad		mm1, 3		;p1(8,2)

	movq		[esi + 8*17], mm3	;p1(7,22)
	pxor		mm5, mm5	;p1(8,3)
	
	movq		[esi + 8*23], mm4	;p1(7,23)
	punpcklwd	mm5, mm7	;p1(8,4)

	psrad		mm5, 3		;p1(8,5)
	pxor		mm4, mm4	;p1(8,12)

	psubd		mm1, mm5	;p1(8,6)	
	punpckhwd	mm4, mm6	;p1(8,13)

	movq		mm3, mm1	;p1(8,7)
	psrad		mm4, 3		;p1(8,14)

	paddd		mm1, mm0	;p1(8,8)
	pxor		mm5, mm5	;p1(8,15)

	psubd		mm3, mm0	;p1(8,9)
	movq		mm0, [ebx + 8*14]	;p1(9,0)

	punpckhwd	mm5, mm7	;p1(8,16)
	pmullw		mm0, [edi + 8*14]	;p1(9,1)

	movq		[esi + 8*18], mm1	;p1(8,10)
	psrad		mm5, 3		;p1(8,17)

	movq		[esi + 8*20], mm3	;p1(8,11)
	psubd		mm4, mm5	;p1(8,18)

	movq		mm3, mm4	;p1(8,19)
	movq		mm1, [ebx + 8*6]	;p1(10,0)

	paddd		mm3, mm2	;p1(8,20)
	pmullw		mm1, [edi + 8*6]	;p1(10,1)

	psubd		mm4, mm2	;p1(8,21)
	movq		mm5, mm0			;p1(11,1)

	movq		[esi + 8*21], mm4	;p1(8,23)

	movq		[esi + 8*19], mm3	;p1(8,22)
	movq		mm4, mm0			;p1(11,0)

	punpcklwd	mm4, mm1			;p1(11,2)
	movq		mm2, [ebx + 8*10]	;p1(12,0)

	punpckhwd	mm5, mm1			;p1(11,4)	
	pmullw		mm2, [edi + 8*10]	;p1(12,1)

	movq		mm3, [ebx + 8*2]	;p1(13,0)

	pmullw		mm3, [edi + 8*2]	;p1(13,1)
	movq		mm6, mm2			;p1(14,0)

	pmaddwd		mm4, fix_117_117	;p1(11,3)
	movq		mm7, mm2			;p1(14,1)
	
	pmaddwd		mm5, fix_117_117	;p1(11,5)
	punpcklwd	mm6, mm3			;p1(14,2)

	pmaddwd		mm6, fix_117_117	;p1(14,3)
	punpckhwd	mm7, mm3			;p1(14,4)
	
	pmaddwd		mm7, fix_117_117	;p1(14,5)
	paddd		mm4, mm6			;p1(15,0)

	paddd		mm5, mm7			;p1(15,1)
   	movq		[esi+8*24], mm4		;p1(15,2)

	movq		[esi+8*25], mm5		;p1(15,3)
	movq		mm6, mm0				;p1(16,0)

	movq		mm7, mm3				;p1(16,3)
	punpcklwd	mm6, mm2				;p1(16,1)
	
	punpcklwd	mm7, mm3				;p1(16,4)
	pmaddwd		mm6, fix_n039_n089		;p1(16,2)

	pmaddwd		mm7, fix_150_n089n039	;p1(16,5)
	movq		mm4, mm0				;p1(16,12)

	paddd		mm6, [esi+8*24]			;p1(16,6)
	punpckhwd	mm4, mm2				;p1(16,13)

	paddd		mm6, mm7				;p1(16,7)
	pmaddwd		mm4, fix_n039_n089		;p1(16,14)

	movq		mm7, mm6				;p1(16,8)
	paddd		mm4, [esi+8*25]			;p1(16,18)

	movq		mm5, mm3				;p1(16,15)
	paddd		mm6, [esi + 8*16]		;p1(16,9)

	punpckhwd	mm5, mm3				;p1(16,16)
	paddd		mm6, const_0x2xx8		;p1(16,10)
	
	psrad		mm6, 9					;p1(16,11)
	pmaddwd		mm5, fix_150_n089n039	;p1(16,17)

	paddd		mm4, mm5				;p1(16,19)

	movq		mm5, mm4				;p1(16,20)

	paddd		mm4, [esi + 8*17]		;p1(16,21)
	
	paddd		mm4, const_0x2xx8		;p1(16,22)
	
	psrad		mm4, 9					;p1(16,23)

	packssdw	mm6, mm4				;p1(16,24)
	
	movq		[esi + 8*0], mm6		;p1(16,25)
		
	movq		mm4, [esi + 8*16]		;p1(16,26)
	
	psubd		mm4, mm7				;p1(16,27)
	movq		mm6, [esi + 8*17]		;p1(16,30)
	
	paddd		mm4, const_0x2xx8		;p1(16,28)
	movq		mm7, mm1				;p1(17,3)

	psrad		mm4, 9					;p1(16,29)
	psubd		mm6, mm5				;p1(16,31)

	paddd		mm6, const_0x2xx8		;p1(16,32)
	punpcklwd	mm7, mm1				;p1(17,4)

	pmaddwd		mm7, fix_307n256_n196	;p1(17,5)
	psrad		mm6, 9					;p1(16,33)

	packssdw	mm4, mm6				;p1(16,34)
	movq		[esi + 8*14], mm4		;p1(16,35)

	movq		mm6, mm0				;p1(17,0)
	movq		mm4, mm0				;p1(17,12)

	punpcklwd	mm6, mm2				;p1(17,1)
	punpckhwd	mm4, mm2				;p1(17,13)
	
	pmaddwd		mm6, fix_n256_n196		;p1(17,2)
	movq		mm5, mm1				;p1(17,15)

	paddd		mm6, [esi+8*24]			;p1(17,6)
	punpckhwd	mm5, mm1				;p1(17,16)
	
	paddd		mm6, mm7				;p1(17,7)
	pmaddwd		mm4, fix_n256_n196		;p1(17,14)

	movq		mm7, mm6				;p1(17,8)
	pmaddwd		mm5, fix_307n256_n196	;p1(17,17)
	
	paddd		mm6, [esi + 8*18]		;p1(17,9)
	
	paddd		mm6, const_0x2xx8		;p1(17,10)
	
	psrad		mm6, 9					;p1(17,11)
	paddd		mm4, [esi+8*25]			;p1(17,18)

	paddd		mm4, mm5				;p1(17,19)

	movq		mm5, mm4				;p1(17,20)
	
	paddd		mm4, [esi + 8*19]		;p1(17,21)
	
	paddd		mm4, const_0x2xx8		;p1(17,22)
	
	psrad		mm4, 9					;p1(17,23)

	packssdw	mm6, mm4				;p1(17,24)
	
	movq		[esi + 8*2], mm6		;p1(17,25)
	
	movq		mm4, [esi + 8*18]		;p1(17,26)

	movq		mm6, [esi + 8*19]		;p1(17,30)
	psubd		mm4, mm7				;p1(17,27)
	
	paddd		mm4, const_0x2xx8		;p1(17,28)
	psubd		mm6, mm5				;p1(17,31)

	psrad		mm4, 9					;p1(17,29)
	paddd		mm6, const_0x2xx8		;p1(17,32)

	psrad		mm6, 9					;p1(17,33)
	movq		mm7, mm2				;p1(18,3)

	packssdw	mm4, mm6				;p1(17,34)
	movq		[esi + 8*12], mm4		;p1(17,35)

	movq		mm6, mm1				;p1(18,0)
	punpcklwd	mm7, mm2				;p1(18,4)

	punpcklwd	mm6, mm3				;p1(18,1)
	pmaddwd		mm7, fix_205_n256n039	;p1(18,5)

	pmaddwd		mm6, fix_n039_n256		;p1(18,2)
	movq		mm4, mm1				;p1(18,12)

	paddd		mm6, [esi+8*24]			;p1(18,6)
	punpckhwd	mm4, mm3				;p1(18,13)

	paddd		mm6, mm7				;p1(18,7)
	pmaddwd		mm4, fix_n039_n256		;p1(18,14)

	movq		mm7, mm6				;p1(18,8)
	movq		mm5, mm2				;p1(18,15)

	paddd		mm6, [esi + 8*20]		;p1(18,9)
	punpckhwd	mm5, mm2				;p1(18,16)
	
	paddd		mm6, const_0x2xx8		;p1(18,10)
	
	psrad		mm6, 9					;p1(18,11)
	pmaddwd		mm5, fix_205_n256n039	;p1(18,17)

	paddd		mm4, [esi+8*25]			;p1(18,18)

	paddd		mm4, mm5				;p1(18,19)

	movq		mm5, mm4				;p1(18,20)
	
	paddd		mm4, [esi + 8*21]		;p1(18,21)
	
	paddd		mm4, const_0x2xx8		;p1(18,22)
	
	psrad		mm4, 9					;p1(18,23)

	packssdw	mm6, mm4				;p1(18,24)
	
	movq		[esi + 8*4], mm6		;p1(18,25)

	movq		mm4, [esi + 8*20]		;p1(18,26)

	psubd		mm4, mm7				;p1(18,27)
	
	paddd		mm4, const_0x2xx8		;p1(18,28)
	movq		mm7, mm0				;p1(19,3)
	
	psrad		mm4, 9					;p1(18,29)
	movq		mm6, [esi + 8*21]		;p1(18,30)
	
	psubd		mm6, mm5				;p1(18,31)
	punpcklwd	mm7, mm0				;p1(19,4)
	
	paddd		mm6, const_0x2xx8		;p1(18,32)
	
	psrad		mm6, 9					;p1(18,33)
	pmaddwd		mm7, fix_029_n089n196	;p1(19,5)

	packssdw	mm4, mm6				;p1(18,34)

	movq		[esi + 8*10], mm4		;p1(18,35)
	movq		mm6, mm3				;p1(19,0)

	punpcklwd	mm6, mm1				;p1(19,1)	
	movq		mm5, mm0				;p1(19,15)
	
	pmaddwd		mm6, fix_n196_n089		;p1(19,2)
	punpckhwd	mm5, mm0				;p1(19,16)


	paddd		mm6, [esi+8*24]			;p1(19,6)
	movq		mm4, mm3				;p1(19,12)

	paddd		mm6, mm7				;p1(19,7)
	punpckhwd	mm4, mm1				;p1(19,13)


	movq		mm7, mm6				;p1(19,8)
	pmaddwd		mm4, fix_n196_n089  	;p1(19,14)

	paddd		mm6, [esi + 8*22]		;p1(19,9)

	pmaddwd		mm5, fix_029_n089n196	;p1(19,17)
	
	paddd		mm6, const_0x2xx8		;p1(19,10)
	
	psrad		mm6, 9					;p1(19,11)
	paddd		mm4, [esi+8*25]			;p1(19,18)

	paddd		mm4, mm5				;p1(19,19)

	movq		mm5, mm4				;p1(19,20)
	paddd		mm4, [esi + 8*23]		;p1(19,21)
	paddd		mm4, const_0x2xx8		;p1(19,22)
	psrad		mm4, 9					;p1(19,23)

	packssdw	mm6, mm4				;p1(19,24)
	movq		[esi + 8*6], mm6		;p1(19,25)

	movq		mm4, [esi + 8*22]		;p1(19,26)
	
	psubd		mm4, mm7				;p1(19,27)
	movq		mm6, [esi + 8*23]		;p1(19,30)
	
	paddd		mm4, const_0x2xx8		;p1(19,28)
	psubd		mm6, mm5				;p1(19,31)

	psrad		mm4, 9					;p1(19,29)
	paddd		mm6, const_0x2xx8		;p1(19,32)

	psrad		mm6, 9					;p1(19,33)

	packssdw	mm4, mm6				;p1(19,34)
	movq		[esi + 8*8], mm4		;p1(19,35)
 
//************************************************************  
	mov			esi, eax

	mov			edi, outptr
	

//transpose 4 rows of wsptr

	movq		mm0, [esi+8*0]		;tran(0)
	
	movq		mm1, mm0			;tran(1)
	movq		mm2, [esi+8*2]		;tran(2)		

	punpcklwd	mm0, mm2			;tran(3)
	movq		mm3, [esi+8*4]		;tran(5)

	punpckhwd	mm1, mm2			;tran(4)
	movq		mm5, [esi+8*6]		;tran(7)

	movq		mm4, mm3			;tran(6)
	movq		mm6, mm0			;tran(10)

	punpcklwd	mm3, mm5			;tran(8)
	movq		mm7, mm1			;tran(11)

	punpckldq	mm0, mm3			;tran(12)

	punpckhwd	mm4, mm5			;tran(9)
	movq		[esi+8*0], mm0		;tran(16)
	
	punpckhdq	mm6, mm3			;tran(13)
	movq		mm0, [esi+8*1]		;tran(20)

	punpckldq	mm1, mm4			;tran(14)
	movq		[esi+8*2], mm6		;tran(17)
	
	punpckhdq	mm7, mm4			;tran(15)
	movq		[esi+8*4], mm1		;tran(18)

	movq		mm1, mm0			;tran(21)
	movq		mm3, [esi+8*5]		;tran(25)

	movq		mm2, [esi+8*3]		;tran(22)
	movq		mm4, mm3			;tran(26)

	punpcklwd	mm0, mm2			;tran(23)
	movq		[esi+8*6], mm7		;tran(19)

	punpckhwd	mm1, mm2			;tran(24)
	movq		mm5, [esi+8*7]		;tran(27)

	punpcklwd	mm3, mm5			;tran(28)
	movq		mm6, mm0			;tran(30)

	movq		mm7, mm1			;tran(31)
	punpckhdq	mm6, mm3			;tran(33)

	punpckhwd	mm4, mm5			;tran(29)
	movq		mm2, mm6			;p2(1,0)

	punpckhdq	mm7, mm4			;tran(35)
	movq		mm5, [esi + 8*2]	;p2(1,2)

	paddw		mm2, mm7			;p2(1,1)
	paddw		mm5, [esi + 8*6]	;p2(1,3)

	punpckldq	mm0, mm3			;tran(32)
	paddw		mm2, mm5			;p2(1,4)

	punpckldq	mm1, mm4			;tran(34)
	movq		mm5, [esi + 8*2]		;p2(3,0)

	pmulhw		mm2, fix_117_117	;p2(1,5)
	movq		mm4, mm7				;p2(2,0)

	pmulhw		mm4, fixn089n196p029	;p2(2,1)
	movq		mm3, mm6					;p2(6,0)

	pmulhw		mm3, fix_n256n039p205		;p2(6,1)

	pmulhw		mm5, fix_n089			;p2(3,1)

	movq		[eax + 8*24], mm2	;p2(1,6)

	movq		mm2, [esi + 8*6]	;p2(4,0)

	pmulhw		mm2, fix_n196		;p2(4,1)

	paddw		mm4, [eax + 8*24]			;p2(5,0)

	paddw		mm3, [eax + 8*24]		;p2(9,0)

	paddw		mm5, mm2					;p2(5,1)

	movq		mm2, [esi + 8*2]		;p2(7,0)
	paddw		mm5, mm4					;p2(5,2)
	pmulhw		mm2, fix_n039			;p2(7,1)

	movq		[esi + 8*1], mm5			;p2(5,3)

	movq		mm4, [esi + 8*6]	;p2(8,0)
	movq		mm5, mm6			;p2(10,0)

	pmulhw		mm4, fix_n256		;p2(8,1)

	pmulhw		mm5, fix_n039		;p2(10,1)

	pmulhw		mm6, fix_n256			;p2(15,0)

	paddw		mm2, mm4				;p2(9,1)

	movq		mm4, mm7			;p2(11,0)
	
	pmulhw		mm4, fix_n089		;p2(11,1)
	paddw		mm2, mm3				;p2(9,2)
	
	movq		[esi + 8*3], mm2		;p2(9,3)

	movq		mm3, [esi + 8*2]		;p2(13,0)

	pmulhw		mm7, fix_n196			;p2(16,0)

	pmulhw		mm3, fix_n089n039p150	;p2(13,1)
	paddw		mm5, mm4			;p2(12,0)


	paddw		mm5, [eax + 8*24]		;p2(14,0)

	movq		mm2, [esi + 8*6]		;p2(18,0)

	pmulhw		mm2, fix_n196p307n256	;p2(18,1)
	paddw		mm5, mm3				;p2(14,1)

	movq		[esi + 8*5], mm5		;p2(14,2)
	paddw		mm6, mm7				;p2(17,0)

	paddw		mm6, [eax + 8*24]		;p2(19,0)
	movq		mm3, mm1				;p2(21,0)

	movq		mm4, [esi + 8*4]		;p2(20,0)
	paddw		mm6, mm2				;p2(19,1)

	movq		[esi + 8*7], mm6		;p2(19,2)	
	movq		mm5, mm4				;p2(20,1)

	movq		mm7, [esi + 8*0]		;p2(26,0)

	pmulhw		mm4, fix_054p076		;p2(20,2)	
	psubw		mm7, mm0				;p2(27,0)

	pmulhw		mm3, fix_054			;p2(21,1)
	movq		mm2, mm0				;p2(26,1)

	pmulhw		mm5, fix_054			;p2(23,0)
	psraw		mm7, 3					;p2(27,1)

	paddw		mm2, [esi + 8*0]		;p2(26,2)
	movq		mm6, mm7				;p2(28,0)

	pmulhw		mm1, fix_054n184		;p2(24,0)
	psraw		mm2, 3					;p2(26,3)			

	paddw		mm4, mm3				;p2(22,0)	
	paddw		mm5, mm1				;p2(25,0)

	psubw		mm6, mm5				;p2(29,0)	
	movq		mm3, mm2				;p2(30,0)

	paddw		mm2, mm4				;p2(30,1)		
	paddw		mm7, mm5				;p2(28,1)			

	movq		mm1, mm2				;p2(32,0)
	psubw		mm3, mm4				;p2(31,0)		

	paddw		mm1, [esi + 8*5]		;p2(32,1)
	movq		mm0, mm7				;p2(33,0)

	psubw		mm2, [esi + 8*5]		;p2(32,2)
	movq		mm4, mm6				;p2(34,0)
	
	paddw		mm1, const_0x0808		;p2(32,3)
	
	paddw		mm2, const_0x0808		;p2(32,4)
	psraw		mm1, 4					;p2(32,5)

	psraw		mm2, 4					;p2(32,6)
	paddw		mm7, [esi + 8*7]		;p2(33,1)

	packuswb	mm1, mm2				;p2(32,7)
	psubw		mm0, [esi + 8*7]		;p2(33,2)

	paddw		mm7, const_0x0808		;p2(33,3)

	paddw		mm0, const_0x0808		;p2(33,4)
	psraw		mm7, 4					;p2(33,5)
	
	psraw		mm0, 4					;p2(33,6)
	paddw		mm4, [esi + 8*3]		;p2(34,1)

	packuswb	mm7, mm0				;p2(33,7)
	psubw		mm6, [esi + 8*3]		;p2(34,2)

	paddw		mm4, const_0x0808		;p2(34,3)
	movq		mm5, mm3				;p2(35,0)
	
	paddw		mm6, const_0x0808		;p2(34,4)
	psraw		mm4, 4					;p2(34,5)
	
	psraw		mm6, 4					;p2(34,6)
	paddw		mm3, [esi + 8*1]		;p2(35,1)

	packuswb	mm4, mm6				;p2(34,7)
	psubw		mm5, [esi + 8*1]		;p2(35,2)

	movq		mm0, mm1				;p2(36,0)
	paddw		mm3, const_0x0808		;p2(35,3)
	
	paddw		mm5, const_0x0808		;p2(35,4)
	punpcklbw	mm0, mm7				;p2(36,1)

	psraw		mm3, 4					;p2(35,5)
	movq		mm2, mm4				;p2(37,0)

	psraw		mm5, 4					;p2(35,6)
	movq		mm6, mm0				;p2(38,0)
	
	packuswb	mm3, mm5				;p2(35,7)
	mov			ebx, [edi]				;p2(42,0)

	punpckhbw	mm7, mm1				;p2(36,2)
	mov			ecx, [edi+4]			;p2(42,1)

	punpcklbw	mm2, mm3				;p2(37,1)	
	mov			edx, [edi+8]			;p2(42,2)

	punpckhbw	mm3, mm4				;p2(37,2)	
	add			ebx, output_col			;p2(42,3)

	punpcklwd	mm0, mm2				;p2(38,1)	
	movq		mm5, mm3				;p2(39,0)
	
	punpckhwd	mm6, mm2				;p2(38,2)	
	movq		mm1, mm0				;p2(40,0)

	punpcklwd	mm3, mm7				;p2(39,1)	
	add			ecx, output_col			;p2(42,4)

	add			edx, output_col			;p2(42,5)
	punpckldq	mm0, mm3				;p2(40,1)	

	punpckhdq	mm1, mm3				;p2(40,2)	
	movq		[ebx], mm0				;p2(43,0)	

	punpckhwd	mm5, mm7				;p2(39,2)	
	movq		[ecx], mm1				;p2(43,1)	

	movq		mm4, mm6				;p2(41,0)
	mov			ebx, [edi+12]			;p2(43,3)

	punpckldq	mm4, mm5				;p2(41,1)	
	add			ebx, output_col			;p2(43,4)

	punpckhdq	mm6, mm5				;p2(41,2)	
	movq		[edx], mm4				;p2(43,2)		

	movq		[ebx], mm6				;p2(43,5)

//************************************************************
//	Process next 4 rows

	add			esi, 64
	add			edi, 16

//transpose next 4 rows of wsptr

	movq		mm0, [esi+8*0]		;tran(0)
	
	movq		mm1, mm0			;tran(1)
	movq		mm2, [esi+8*2]		;tran(2)		

	punpcklwd	mm0, mm2			;tran(3)
	movq		mm3, [esi+8*4]		;tran(5)

	punpckhwd	mm1, mm2			;tran(4)
	movq		mm5, [esi+8*6]		;tran(7)

	movq		mm4, mm3			;tran(6)
	movq		mm6, mm0			;tran(10)

	punpcklwd	mm3, mm5			;tran(8)
	movq		mm7, mm1			;tran(11)

	punpckldq	mm0, mm3			;tran(12)

	punpckhwd	mm4, mm5			;tran(9)
	movq		[esi+8*0], mm0		;tran(16)
	
	punpckhdq	mm6, mm3			;tran(13)
	movq		mm0, [esi+8*1]		;tran(20)

	punpckldq	mm1, mm4			;tran(14)
	movq		[esi+8*2], mm6		;tran(17)
	
	punpckhdq	mm7, mm4			;tran(15)
	movq		[esi+8*4], mm1		;tran(18)

	movq		mm1, mm0			;tran(21)
	movq		mm3, [esi+8*5]		;tran(25)

	movq		mm2, [esi+8*3]		;tran(22)
	movq		mm4, mm3			;tran(26)

	punpcklwd	mm0, mm2			;tran(23)
	movq		[esi+8*6], mm7		;tran(19)

	punpckhwd	mm1, mm2			;tran(24)
	movq		mm5, [esi+8*7]		;tran(27)

	punpcklwd	mm3, mm5			;tran(28)
	movq		mm6, mm0			;tran(30)

	movq		mm7, mm1			;tran(31)
	punpckhdq	mm6, mm3			;tran(33)

	punpckhwd	mm4, mm5			;tran(29)
	movq		mm2, mm6			;p2(1,0)

	punpckhdq	mm7, mm4			;tran(35)
	movq		mm5, [esi + 8*2]	;p2(1,2)

	paddw		mm2, mm7			;p2(1,1)
	paddw		mm5, [esi + 8*6]	;p2(1,3)

	punpckldq	mm0, mm3			;tran(32)
	paddw		mm2, mm5			;p2(1,4)

	punpckldq	mm1, mm4			;tran(34)
	movq		mm5, [esi + 8*2]		;p2(3,0)

	pmulhw		mm2, fix_117_117	;p2(1,5)
	movq		mm4, mm7				;p2(2,0)

	pmulhw		mm4, fixn089n196p029	;p2(2,1)
	movq		mm3, mm6					;p2(6,0)

	pmulhw		mm3, fix_n256n039p205		;p2(6,1)

	pmulhw		mm5, fix_n089			;p2(3,1)

	movq		[eax + 8*24], mm2	;p2(1,6)

	movq		mm2, [esi + 8*6]	;p2(4,0)

	pmulhw		mm2, fix_n196		;p2(4,1)

	paddw		mm4, [eax + 8*24]			;p2(5,0)

	paddw		mm3, [eax + 8*24]		;p2(9,0)

	paddw		mm5, mm2					;p2(5,1)

	movq		mm2, [esi + 8*2]		;p2(7,0)
	paddw		mm5, mm4					;p2(5,2)
	pmulhw		mm2, fix_n039			;p2(7,1)

	movq		[esi + 8*1], mm5			;p2(5,3)

	movq		mm4, [esi + 8*6]	;p2(8,0)
	movq		mm5, mm6			;p2(10,0)

	pmulhw		mm4, fix_n256		;p2(8,1)

	pmulhw		mm5, fix_n039		;p2(10,1)

	pmulhw		mm6, fix_n256			;p2(15,0)

	paddw		mm2, mm4				;p2(9,1)

	movq		mm4, mm7			;p2(11,0)
	
	pmulhw		mm4, fix_n089		;p2(11,1)
	paddw		mm2, mm3				;p2(9,2)
	
	movq		[esi + 8*3], mm2		;p2(9,3)

	movq		mm3, [esi + 8*2]		;p2(13,0)

	pmulhw		mm7, fix_n196			;p2(16,0)

	pmulhw		mm3, fix_n089n039p150	;p2(13,1)
	paddw		mm5, mm4			;p2(12,0)


	paddw		mm5, [eax + 8*24]		;p2(14,0)

	movq		mm2, [esi + 8*6]		;p2(18,0)

	pmulhw		mm2, fix_n196p307n256	;p2(18,1)
	paddw		mm5, mm3				;p2(14,1)

	movq		[esi + 8*5], mm5		;p2(14,2)
	paddw		mm6, mm7				;p2(17,0)

	paddw		mm6, [eax + 8*24]		;p2(19,0)
	movq		mm3, mm1				;p2(21,0)

	movq		mm4, [esi + 8*4]		;p2(20,0)
	paddw		mm6, mm2				;p2(19,1)

	movq		[esi + 8*7], mm6		;p2(19,2)	
	movq		mm5, mm4				;p2(20,1)

	movq		mm7, [esi + 8*0]		;p2(26,0)

	pmulhw		mm4, fix_054p076		;p2(20,2)	
	psubw		mm7, mm0				;p2(27,0)

	pmulhw		mm3, fix_054			;p2(21,1)
	movq		mm2, mm0				;p2(26,1)

	pmulhw		mm5, fix_054			;p2(23,0)
	psraw		mm7, 3					;p2(27,1)

	paddw		mm2, [esi + 8*0]		;p2(26,2)
	movq		mm6, mm7				;p2(28,0)

	pmulhw		mm1, fix_054n184		;p2(24,0)
	psraw		mm2, 3					;p2(26,3)			

	paddw		mm4, mm3				;p2(22,0)	
	paddw		mm5, mm1				;p2(25,0)

	psubw		mm6, mm5				;p2(29,0)	
	movq		mm3, mm2				;p2(30,0)

	paddw		mm2, mm4				;p2(30,1)		
	paddw		mm7, mm5				;p2(28,1)			

	movq		mm1, mm2				;p2(32,0)
	psubw		mm3, mm4				;p2(31,0)		

	paddw		mm1, [esi + 8*5]		;p2(32,1)
	movq		mm0, mm7				;p2(33,0)

	psubw		mm2, [esi + 8*5]		;p2(32,2)
	movq		mm4, mm6				;p2(34,0)
	
	paddw		mm1, const_0x0808		;p2(32,3)
	
	paddw		mm2, const_0x0808		;p2(32,4)
	psraw		mm1, 4					;p2(32,5)

	psraw		mm2, 4					;p2(32,6)
	paddw		mm7, [esi + 8*7]		;p2(33,1)

	packuswb	mm1, mm2				;p2(32,7)
	psubw		mm0, [esi + 8*7]		;p2(33,2)

	paddw		mm7, const_0x0808		;p2(33,3)

	paddw		mm0, const_0x0808		;p2(33,4)
	psraw		mm7, 4					;p2(33,5)
	
	psraw		mm0, 4					;p2(33,6)
	paddw		mm4, [esi + 8*3]		;p2(34,1)

	packuswb	mm7, mm0				;p2(33,7)
	psubw		mm6, [esi + 8*3]		;p2(34,2)

	paddw		mm4, const_0x0808		;p2(34,3)
	movq		mm5, mm3				;p2(35,0)
	
	paddw		mm6, const_0x0808		;p2(34,4)
	psraw		mm4, 4					;p2(34,5)
	
	psraw		mm6, 4					;p2(34,6)
	paddw		mm3, [esi + 8*1]		;p2(35,1)

	packuswb	mm4, mm6				;p2(34,7)
	psubw		mm5, [esi + 8*1]		;p2(35,2)

	movq		mm0, mm1				;p2(36,0)
	paddw		mm3, const_0x0808		;p2(35,3)
	
	paddw		mm5, const_0x0808		;p2(35,4)
	punpcklbw	mm0, mm7				;p2(36,1)

	psraw		mm3, 4					;p2(35,5)
	movq		mm2, mm4				;p2(37,0)

	psraw		mm5, 4					;p2(35,6)
	movq		mm6, mm0				;p2(38,0)
	
	packuswb	mm3, mm5				;p2(35,7)
	mov			ebx, [edi]				;p2(42,0)

	punpckhbw	mm7, mm1				;p2(36,2)
	mov			ecx, [edi+4]			;p2(42,1)

	punpcklbw	mm2, mm3				;p2(37,1)	
	mov			edx, [edi+8]			;p2(42,2)

	punpckhbw	mm3, mm4				;p2(37,2)	
	add			ebx, output_col			;p2(42,3)

	punpcklwd	mm0, mm2				;p2(38,1)	
	movq		mm5, mm3				;p2(39,0)
	
	punpckhwd	mm6, mm2				;p2(38,2)	
	movq		mm1, mm0				;p2(40,0)

	punpcklwd	mm3, mm7				;p2(39,1)	
	add			ecx, output_col			;p2(42,4)

	add			edx, output_col			;p2(42,5)
	punpckldq	mm0, mm3				;p2(40,1)	

	punpckhdq	mm1, mm3				;p2(40,2)	
	movq		[ebx], mm0				;p2(43,0)	

	punpckhwd	mm5, mm7				;p2(39,2)	
	movq		[ecx], mm1				;p2(43,1)	

	movq		mm4, mm6				;p2(41,0)
	mov			ebx, [edi+12]			;p2(43,3)

	punpckldq	mm4, mm5				;p2(41,1)	
	add			ebx, output_col			;p2(43,4)

	punpckhdq	mm6, mm5				;p2(41,2)	
	movq		[edx], mm4				;p2(43,2)		

	movq		[ebx], mm6				;p2(43,5)

//************************************************************************

	emms

	}
#endif









#if defined(HAVE_MMX_ATT_MNEMONICS) 
        __asm__ (

        "pushl           %%ebx\n\t"
        "movl            %0, %%edi \n\t"
        "movl            %1, %%ebx \n\t"
        "movl            %2, %%esi \n\t"

        "addl            $0x07,%%esi \n\t"      //align wsptr to qword
        "andl            $0xfffffff8,%%esi \n\t" //align wsptr to qword

        "movl            %%esi,%%eax \n\t"

        /* Pass 1. */

        "movq            8*4(%%ebx),%%mm0 \n\t" //p1(1,0)
        "pmullw          8*4(%%edi),%%mm0 \n\t" //p1(1,1)

    "movq                8*12(%%ebx),%%mm1 \n\t" //p1(2,0)
        "pmullw          8*12(%%edi),%%mm1 \n\t" //p1(2,1)

        "movq            8*0(%%ebx),%%mm6 \n\t" //p1(5,0)
        "pmullw          8*0(%%edi),%%mm6 \n\t" //p1(5,1)

        "movq            %%mm0,%%mm2 \n\t"                      //p1(3,0)
        "movq            8*8(%%ebx),%%mm7 \n\t" //p1(6,0)

        "punpcklwd       %%mm1,%%mm0 \n\t"                      //p1(3,1)
        "pmullw          8*8(%%edi),%%mm7 \n\t" //p1(6,1)

        "movq            %%mm0,%%mm4 \n\t"                      //p1(3,2)
        "punpckhwd       %%mm1,%%mm2 \n\t"                      //p1(3,4)

        "pmaddwd         %5,%%mm0 \n\t" //p1(3,3)
        "movq            %%mm2,%%mm5 \n\t"                      //p1(3,5)

        "pmaddwd         %5,%%mm2 \n\t" //p1(3,6)
        "pxor            %%mm1,%%mm1 \n\t" //p1(7,0)

        "pmaddwd         %6,%%mm4 \n\t"   //p1(4,0)
        "punpcklwd   %%mm6,%%mm1     \n\t" //p1(7,1)

        "pmaddwd         %6,%%mm5 \n\t"   //p1(4,1)
        "psrad           $3,%%mm1    \n\t" //p1(7,2)

        "pxor            %%mm3,%%mm3 \n\t" //p1(7,3)

        "punpcklwd       %%mm7,%%mm3 \n\t" //p1(7,4)

        "psrad           $3,%%mm3    \n\t" //p1(7,5)

        "paddd           %%mm3,%%mm1 \n\t" //p1(7,6)

        "movq            %%mm1,%%mm3 \n\t" //p1(7,7)        

        "paddd           %%mm4,%%mm1 \n\t" //p1(7,8)

        "psubd           %%mm4,%%mm3 \n\t" //p1(7,9)

        "movq            %%mm1,8*16(%%esi) \n\t" //p1(7,10)
        "pxor            %%mm4,%%mm4 \n\t" //p1(7,12)

        "movq            %%mm3,8*22(%%esi) \n\t" //p1(7,11)
        "punpckhwd       %%mm6,%%mm4 \n\t" //p1(7,13)

        "psrad           $3,%%mm4    \n\t" //p1(7,14)
        "pxor            %%mm1,%%mm1 \n\t" //p1(7,15)

        "punpckhwd       %%mm7,%%mm1 \n\t" //p1(7,16)

        "psrad           $3,%%mm1    \n\t" //p1(7,17)

        "paddd           %%mm1,%%mm4 \n\t" //p1(7,18)

        "movq            %%mm4,%%mm3 \n\t" //p1(7,19)       
        "pxor            %%mm1,%%mm1 \n\t" //p1(8,0)

        "paddd           %%mm5,%%mm3 \n\t" //p1(7,20)
        "punpcklwd       %%mm6,%%mm1 \n\t" //p1(8,1)

        "psubd           %%mm5,%%mm4 \n\t" //p1(7,21)
        "psrad           $3,%%mm1    \n\t" //p1(8,2)

        "movq            %%mm3,8*17(%%esi) \n\t" //p1(7,22)
        "pxor            %%mm5,%%mm5 \n\t" //p1(8,3)

        "movq            %%mm4,8*23(%%esi) \n\t" //p1(7,23)
        "punpcklwd       %%mm7,%%mm5 \n\t" //p1(8,4)

        "psrad           $3,%%mm5    \n\t" //p1(8,5)
        "pxor            %%mm4,%%mm4 \n\t" //p1(8,12)

        "psubd           %%mm5,%%mm1 \n\t" //p1(8,6)        
        "punpckhwd       %%mm6,%%mm4 \n\t" //p1(8,13)

        "movq            %%mm1,%%mm3 \n\t" //p1(8,7)
        "psrad           $3,%%mm4    \n\t" //p1(8,14)

        "paddd           %%mm0,%%mm1 \n\t" //p1(8,8)
        "pxor            %%mm5,%%mm5 \n\t" //p1(8,15)

        "psubd           %%mm0,%%mm3 \n\t" //p1(8,9)
        "movq            8*14(%%ebx),%%mm0 \n\t" //p1(9,0)

        "punpckhwd       %%mm7,%%mm5 \n\t" //p1(8,16)
        "pmullw          8*14(%%edi),%%mm0 \n\t" //p1(9,1)

        "movq            %%mm1,8*18(%%esi) \n\t" //p1(8,10)
        "psrad           $3,%%mm5    \n\t" //p1(8,17)

        "movq            %%mm3,8*20(%%esi) \n\t" //p1(8,11)
        "psubd           %%mm5,%%mm4 \n\t" //p1(8,18)

        "movq            %%mm4,%%mm3 \n\t" //p1(8,19)
        "movq            8*6(%%ebx),%%mm1 \n\t" //p1(10,0)

        "paddd           %%mm2,%%mm3 \n\t" //p1(8,20)
        "pmullw          8*6(%%edi),%%mm1 \n\t" //p1(10,1)

        "psubd           %%mm2,%%mm4 \n\t" //p1(8,21)
        "movq            %%mm0,%%mm5 \n\t"              //p1(11,1)

        "movq            %%mm4,8*21(%%esi) \n\t" //p1(8,23)

        "movq            %%mm3,8*19(%%esi) \n\t" //p1(8,22)
        "movq            %%mm0,%%mm4 \n\t"              //p1(11,0)

        "punpcklwd       %%mm1,%%mm4 \n\t"              //p1(11,2)
        "movq            8*10(%%ebx),%%mm2 \n\t" //p1(12,0)

        "punpckhwd       %%mm1,%%mm5 \n\t"              //p1(11,4)       
        "pmullw          8*10(%%edi),%%mm2 \n\t" //p1(12,1)

        "movq            8*2(%%ebx),%%mm3 \n\t" //p1(13,0)

        "pmullw          8*2(%%edi),%%mm3 \n\t" //p1(13,1)
        "movq            %%mm2,%%mm6 \n\t"              //p1(14,0)

        "pmaddwd         %7,%%mm4 \n\t" //p1(11,3)
        "movq            %%mm2,%%mm7 \n\t"              //p1(14,1)

        "pmaddwd         %7,%%mm5 \n\t" //p1(11,5)
        "punpcklwd       %%mm3,%%mm6 \n\t"              //p1(14,2)

        "pmaddwd         %7,%%mm6 \n\t" //p1(14,3)
        "punpckhwd       %%mm3,%%mm7 \n\t"              //p1(14,4)

        "pmaddwd         %7,%%mm7 \n\t" //p1(14,5)
        "paddd           %%mm6,%%mm4 \n\t"              //p1(15,0)

        "paddd           %%mm7,%%mm5 \n\t"              //p1(15,1)
        "movq            %%mm4,8*24(%%esi) \n\t" //p1(15,2)

        "movq            %%mm5,8*25(%%esi) \n\t" //p1(15,3)
        "movq            %%mm0,%%mm6 \n\t"                      //p1(16,0)

        "movq            %%mm3,%%mm7 \n\t"                      //p1(16,3)
        "punpcklwd       %%mm2,%%mm6 \n\t"                      //p1(16,1)

        "punpcklwd       %%mm3,%%mm7 \n\t"                      //p1(16,4)
        "pmaddwd         %8,%%mm6 \n\t"     //p1(16,2)

        "pmaddwd         %9,%%mm7 \n\t" //p1(16,5)
        "movq            %%mm0,%%mm4 \n\t"                      //p1(16,12)

        "paddd           8*24(%%esi),%%mm6 \n\t"        //p1(16,6)
        "punpckhwd       %%mm2,%%mm4 \n\t"                      //p1(16,13)

        "paddd           %%mm7,%%mm6 \n\t"                      //p1(16,7)
        "pmaddwd         %8,%%mm4 \n\t"     //p1(16,14)

        "movq            %%mm6,%%mm7 \n\t"                      //p1(16,8)
        "paddd           8*25(%%esi),%%mm4 \n\t"        //p1(16,18)

        "movq            %%mm3,%%mm5 \n\t"                      //p1(16,15)
        "paddd           8*16(%%esi),%%mm6 \n\t"        //p1(16,9)

        "punpckhwd       %%mm3,%%mm5 \n\t"                      //p1(16,16)
        "paddd           %10,%%mm6 \n\t"      //p1(16,10)

        "psrad           $9,%%mm6    \n\t"                      //p1(16,11)
        "pmaddwd         %9,%%mm5 \n\t" //p1(16,17)

        "paddd           %%mm5,%%mm4 \n\t"                      //p1(16,19)

        "movq            %%mm4,%%mm5 \n\t"                      //p1(16,20)

        "paddd           8*17(%%esi),%%mm4 \n\t"        //p1(16,21)

        "paddd           %10,%%mm4 \n\t"      //p1(16,22)

        "psrad           $9,%%mm4    \n\t"                      //p1(16,23)

        "packssdw        %%mm4,%%mm6 \n\t"                      //p1(16,24)

        "movq            %%mm6,8*0(%%esi) \n\t"         //p1(16,25)

        "movq            8*16(%%esi),%%mm4 \n\t"        //p1(16,26)

        "psubd           %%mm7,%%mm4 \n\t"                      //p1(16,27)
        "movq            8*17(%%esi),%%mm6 \n\t"        //p1(16,30)

        "paddd           %10,%%mm4 \n\t"      //p1(16,28)
        "movq            %%mm1,%%mm7 \n\t"                      //p1(17,3)

        "psrad           $9,%%mm4    \n\t"                      //p1(16,29)
        "psubd           %%mm5,%%mm6 \n\t"                      //p1(16,31)

        "paddd           %10,%%mm6 \n\t"      //p1(16,32)
        "punpcklwd       %%mm1,%%mm7 \n\t"                      //p1(17,4)

        "pmaddwd         %11,%%mm7 \n\t" //p1(17,5)
        "psrad           $9,%%mm6    \n\t"                      //p1(16,33)

        "packssdw        %%mm6,%%mm4 \n\t"                      //p1(16,34)
        "movq            %%mm4,8*14(%%esi) \n\t"        //p1(16,35)

        "movq            %%mm0,%%mm6 \n\t"                      //p1(17,0)
        "movq            %%mm0,%%mm4 \n\t"                      //p1(17,12)

        "punpcklwd       %%mm2,%%mm6 \n\t"                      //p1(17,1)
        "punpckhwd       %%mm2,%%mm4 \n\t"                      //p1(17,13)

        "pmaddwd         %12,%%mm6 \n\t"     //p1(17,2)
        "movq            %%mm1,%%mm5 \n\t"                      //p1(17,15)

        "paddd           8*24(%%esi),%%mm6 \n\t"        //p1(17,6)
        "punpckhwd       %%mm1,%%mm5 \n\t"                      //p1(17,16)

        "paddd           %%mm7,%%mm6 \n\t"                      //p1(17,7)
        "pmaddwd         %12,%%mm4 \n\t"     //p1(17,14)

        "movq            %%mm6,%%mm7 \n\t"                      //p1(17,8)
        "pmaddwd         %11,%%mm5 \n\t" //p1(17,17)

        "paddd           8*18(%%esi),%%mm6 \n\t"        //p1(17,9)

        "paddd           %10,%%mm6 \n\t"      //p1(17,10)

        "psrad           $9,%%mm6    \n\t"                      //p1(17,11)
        "paddd           8*25(%%esi),%%mm4 \n\t"        //p1(17,18)

        "paddd           %%mm5,%%mm4 \n\t"                      //p1(17,19)

        "movq            %%mm4,%%mm5 \n\t"                      //p1(17,20)

        "paddd           8*19(%%esi),%%mm4 \n\t"        //p1(17,21)

        "paddd           %10,%%mm4 \n\t"      //p1(17,22)

        "psrad           $9,%%mm4    \n\t"                      //p1(17,23)

        "packssdw        %%mm4,%%mm6 \n\t"                      //p1(17,24)

        "movq            %%mm6,8*2(%%esi) \n\t"         //p1(17,25)

        "movq            8*18(%%esi),%%mm4 \n\t"        //p1(17,26)

        "movq            8*19(%%esi),%%mm6 \n\t"        //p1(17,30)
        "psubd           %%mm7,%%mm4 \n\t"                      //p1(17,27)

        "paddd           %10,%%mm4 \n\t"      //p1(17,28)
        "psubd           %%mm5,%%mm6 \n\t"                      //p1(17,31)

        "psrad           $9,%%mm4    \n\t"                      //p1(17,29)
        "paddd           %10,%%mm6 \n\t"      //p1(17,32)

        "psrad           $9,%%mm6    \n\t"                      //p1(17,33)
        "movq            %%mm2,%%mm7 \n\t"                      //p1(18,3)

        "packssdw        %%mm6,%%mm4 \n\t"                      //p1(17,34)
        "movq            %%mm4,8*12(%%esi) \n\t"        //p1(17,35)

        "movq            %%mm1,%%mm6 \n\t"                      //p1(18,0)
        "punpcklwd       %%mm2,%%mm7 \n\t"                      //p1(18,4)

        "punpcklwd       %%mm3,%%mm6 \n\t"                      //p1(18,1)
        "pmaddwd         %13,%%mm7 \n\t" //p1(18,5)

        "pmaddwd         %14,%%mm6 \n\t"     //p1(18,2)
        "movq            %%mm1,%%mm4 \n\t"                      //p1(18,12)

        "paddd           8*24(%%esi),%%mm6 \n\t"        //p1(18,6)
        "punpckhwd       %%mm3,%%mm4 \n\t"                      //p1(18,13)

        "paddd           %%mm7,%%mm6 \n\t"                      //p1(18,7)
        "pmaddwd         %14,%%mm4 \n\t"     //p1(18,14)

        "movq            %%mm6,%%mm7 \n\t"                      //p1(18,8)
        "movq            %%mm2,%%mm5 \n\t"                      //p1(18,15)

        "paddd           8*20(%%esi),%%mm6 \n\t"        //p1(18,9)
        "punpckhwd       %%mm2,%%mm5 \n\t"                      //p1(18,16)

        "paddd           %10,%%mm6 \n\t"      //p1(18,10)

        "psrad           $9,%%mm6    \n\t"                      //p1(18,11)
        "pmaddwd         %13,%%mm5 \n\t" //p1(18,17)

        "paddd           8*25(%%esi),%%mm4 \n\t"        //p1(18,18)

        "paddd           %%mm5,%%mm4 \n\t"                      //p1(18,19)

        "movq            %%mm4,%%mm5 \n\t"                      //p1(18,20)

        "paddd           8*21(%%esi),%%mm4 \n\t"        //p1(18,21)

        "paddd           %10,%%mm4 \n\t"      //p1(18,22)

        "psrad           $9,%%mm4    \n\t"                      //p1(18,23)

        "packssdw        %%mm4,%%mm6 \n\t"                      //p1(18,24)

        "movq            %%mm6,8*4(%%esi) \n\t"         //p1(18,25)

        "movq            8*20(%%esi),%%mm4 \n\t"        //p1(18,26)

        "psubd           %%mm7,%%mm4 \n\t"                      //p1(18,27)

        "paddd           %10,%%mm4 \n\t"      //p1(18,28)
        "movq            %%mm0,%%mm7 \n\t"                      //p1(19,3)

        "psrad           $9,%%mm4    \n\t"                      //p1(18,29)
        "movq            8*21(%%esi),%%mm6 \n\t"        //p1(18,30)

        "psubd           %%mm5,%%mm6 \n\t"                      //p1(18,31)
        "punpcklwd       %%mm0,%%mm7 \n\t"                      //p1(19,4)

        "paddd           %10,%%mm6 \n\t"      //p1(18,32)

        "psrad           $9,%%mm6    \n\t"                      //p1(18,33)
        "pmaddwd         %15,%%mm7 \n\t" //p1(19,5)

        "packssdw        %%mm6,%%mm4 \n\t"                      //p1(18,34)

        "movq            %%mm4,8*10(%%esi) \n\t"        //p1(18,35)
        "movq            %%mm3,%%mm6 \n\t"                      //p1(19,0)

        "punpcklwd       %%mm1,%%mm6 \n\t"                      //p1(19,1)       
        "movq            %%mm0,%%mm5 \n\t"                      //p1(19,15)

        "pmaddwd         %16,%%mm6 \n\t"     //p1(19,2)
        "punpckhwd       %%mm0,%%mm5 \n\t"                      //p1(19,16)


        "paddd           8*24(%%esi),%%mm6 \n\t"        //p1(19,6)
        "movq            %%mm3,%%mm4 \n\t"                      //p1(19,12)

        "paddd           %%mm7,%%mm6 \n\t"                      //p1(19,7)
        "punpckhwd       %%mm1,%%mm4 \n\t"                      //p1(19,13)


        "movq            %%mm6,%%mm7 \n\t"                      //p1(19,8)
        "pmaddwd         %16,%%mm4 \n\t" //p1(19,14)

        "paddd           8*22(%%esi),%%mm6 \n\t"        //p1(19,9)

        "pmaddwd         %15,%%mm5 \n\t" //p1(19,17)

        "paddd           %10,%%mm6 \n\t"      //p1(19,10)

        "psrad           $9,%%mm6    \n\t"                      //p1(19,11)
        "paddd           8*25(%%esi),%%mm4 \n\t"        //p1(19,18)

        "paddd           %%mm5,%%mm4 \n\t"                      //p1(19,19)

        "movq            %%mm4,%%mm5 \n\t"                      //p1(19,20)
        "paddd           8*23(%%esi),%%mm4 \n\t"        //p1(19,21)
        "paddd           %10,%%mm4 \n\t"      //p1(19,22)
        "psrad           $9,%%mm4    \n\t"                      //p1(19,23)

        "packssdw        %%mm4,%%mm6 \n\t"                      //p1(19,24)
        "movq            %%mm6,8*6(%%esi) \n\t"         //p1(19,25)

        "movq            8*22(%%esi),%%mm4 \n\t"        //p1(19,26)

        "psubd           %%mm7,%%mm4 \n\t"                      //p1(19,27)
        "movq            8*23(%%esi),%%mm6 \n\t"        //p1(19,30)

        "paddd           %10,%%mm4 \n\t"      //p1(19,28)
        "psubd           %%mm5,%%mm6 \n\t"                      //p1(19,31)

        "psrad           $9,%%mm4    \n\t"                      //p1(19,29)
        "paddd           %10,%%mm6 \n\t"      //p1(19,32)

        "psrad           $9,%%mm6    \n\t"                      //p1(19,33)

        "packssdw        %%mm6,%%mm4 \n\t"                      //p1(19,34)
        "movq            %%mm4,8*8(%%esi) \n\t"         //p1(19,35)

        "addl            $8,%%edi    \n\t"
        "addl            $8,%%ebx    \n\t"
        "addl            $8,%%esi    \n\t"

        /* Pass 1. */

        "movq            8*4(%%ebx),%%mm0 \n\t" //p1(1,0)
        "pmullw          8*4(%%edi),%%mm0 \n\t" //p1(1,1)

    "movq                8*12(%%ebx),%%mm1 \n\t" //p1(2,0)
        "pmullw          8*12(%%edi),%%mm1 \n\t" //p1(2,1)

        "movq            8*0(%%ebx),%%mm6 \n\t" //p1(5,0)
        "pmullw          8*0(%%edi),%%mm6 \n\t" //p1(5,1)

        "movq            %%mm0,%%mm2 \n\t"                      //p1(3,0)
        "movq            8*8(%%ebx),%%mm7 \n\t" //p1(6,0)

        "punpcklwd       %%mm1,%%mm0 \n\t"                      //p1(3,1)
        "pmullw          8*8(%%edi),%%mm7 \n\t" //p1(6,1)

        "movq            %%mm0,%%mm4 \n\t"                      //p1(3,2)
        "punpckhwd       %%mm1,%%mm2 \n\t"                      //p1(3,4)

        "pmaddwd         %5,%%mm0 \n\t" //p1(3,3)
        "movq            %%mm2,%%mm5 \n\t"                      //p1(3,5)

        "pmaddwd         %5,%%mm2 \n\t" //p1(3,6)
        "pxor            %%mm1,%%mm1 \n\t" //p1(7,0)

        "pmaddwd         %6,%%mm4 \n\t"   //p1(4,0)
        "punpcklwd   %%mm6,%%mm1     \n\t" //p1(7,1)

        "pmaddwd         %6,%%mm5 \n\t"   //p1(4,1)
        "psrad           $3,%%mm1    \n\t" //p1(7,2)

        "pxor            %%mm3,%%mm3 \n\t" //p1(7,3)

        "punpcklwd       %%mm7,%%mm3 \n\t" //p1(7,4)

        "psrad           $3,%%mm3    \n\t" //p1(7,5)

        "paddd           %%mm3,%%mm1 \n\t" //p1(7,6)

        "movq            %%mm1,%%mm3 \n\t" //p1(7,7)        

        "paddd           %%mm4,%%mm1 \n\t" //p1(7,8)

        "psubd           %%mm4,%%mm3 \n\t" //p1(7,9)

        "movq            %%mm1,8*16(%%esi) \n\t" //p1(7,10)
        "pxor            %%mm4,%%mm4 \n\t" //p1(7,12)

        "movq            %%mm3,8*22(%%esi) \n\t" //p1(7,11)
        "punpckhwd       %%mm6,%%mm4 \n\t" //p1(7,13)

        "psrad           $3,%%mm4    \n\t" //p1(7,14)
        "pxor            %%mm1,%%mm1 \n\t" //p1(7,15)

        "punpckhwd       %%mm7,%%mm1 \n\t" //p1(7,16)

        "psrad           $3,%%mm1    \n\t" //p1(7,17)

        "paddd           %%mm1,%%mm4 \n\t" //p1(7,18)

        "movq            %%mm4,%%mm3 \n\t" //p1(7,19)       
        "pxor            %%mm1,%%mm1 \n\t" //p1(8,0)

        "paddd           %%mm5,%%mm3 \n\t" //p1(7,20)
        "punpcklwd       %%mm6,%%mm1 \n\t" //p1(8,1)

        "psubd           %%mm5,%%mm4 \n\t" //p1(7,21)
        "psrad           $3,%%mm1    \n\t" //p1(8,2)

        "movq            %%mm3,8*17(%%esi) \n\t" //p1(7,22)
        "pxor            %%mm5,%%mm5 \n\t" //p1(8,3)

        "movq            %%mm4,8*23(%%esi) \n\t" //p1(7,23)
        "punpcklwd       %%mm7,%%mm5 \n\t" //p1(8,4)

        "psrad           $3,%%mm5    \n\t" //p1(8,5)
        "pxor            %%mm4,%%mm4 \n\t" //p1(8,12)

        "psubd           %%mm5,%%mm1 \n\t" //p1(8,6)        
        "punpckhwd       %%mm6,%%mm4 \n\t" //p1(8,13)

        "movq            %%mm1,%%mm3 \n\t" //p1(8,7)
        "psrad           $3,%%mm4    \n\t" //p1(8,14)

        "paddd           %%mm0,%%mm1 \n\t" //p1(8,8)
        "pxor            %%mm5,%%mm5 \n\t" //p1(8,15)

        "psubd           %%mm0,%%mm3 \n\t" //p1(8,9)
        "movq            8*14(%%ebx),%%mm0 \n\t" //p1(9,0)

        "punpckhwd       %%mm7,%%mm5 \n\t" //p1(8,16)
        "pmullw          8*14(%%edi),%%mm0 \n\t" //p1(9,1)

        "movq            %%mm1,8*18(%%esi) \n\t" //p1(8,10)
        "psrad           $3,%%mm5    \n\t" //p1(8,17)

        "movq            %%mm3,8*20(%%esi) \n\t" //p1(8,11)
        "psubd           %%mm5,%%mm4 \n\t" //p1(8,18)

        "movq            %%mm4,%%mm3 \n\t" //p1(8,19)
        "movq            8*6(%%ebx),%%mm1 \n\t" //p1(10,0)

        "paddd           %%mm2,%%mm3 \n\t" //p1(8,20)
        "pmullw          8*6(%%edi),%%mm1 \n\t" //p1(10,1)

        "psubd           %%mm2,%%mm4 \n\t" //p1(8,21)
        "movq            %%mm0,%%mm5 \n\t"              //p1(11,1)

        "movq            %%mm4,8*21(%%esi) \n\t" //p1(8,23)

        "movq            %%mm3,8*19(%%esi) \n\t" //p1(8,22)
        "movq            %%mm0,%%mm4 \n\t"              //p1(11,0)

        "punpcklwd       %%mm1,%%mm4 \n\t"              //p1(11,2)
        "movq            8*10(%%ebx),%%mm2 \n\t" //p1(12,0)

        "punpckhwd       %%mm1,%%mm5 \n\t"              //p1(11,4)       
        "pmullw          8*10(%%edi),%%mm2 \n\t" //p1(12,1)

        "movq            8*2(%%ebx),%%mm3 \n\t" //p1(13,0)

        "pmullw          8*2(%%edi),%%mm3 \n\t" //p1(13,1)
        "movq            %%mm2,%%mm6 \n\t"              //p1(14,0)

        "pmaddwd         %7,%%mm4 \n\t" //p1(11,3)
        "movq            %%mm2,%%mm7 \n\t"              //p1(14,1)

        "pmaddwd         %7,%%mm5 \n\t" //p1(11,5)
        "punpcklwd       %%mm3,%%mm6 \n\t"              //p1(14,2)

        "pmaddwd         %7,%%mm6 \n\t" //p1(14,3)
        "punpckhwd       %%mm3,%%mm7 \n\t"              //p1(14,4)

        "pmaddwd         %7,%%mm7 \n\t" //p1(14,5)
        "paddd           %%mm6,%%mm4 \n\t"              //p1(15,0)

        "paddd           %%mm7,%%mm5 \n\t"              //p1(15,1)
        "movq            %%mm4,8*24(%%esi) \n\t" //p1(15,2)

        "movq            %%mm5,8*25(%%esi) \n\t" //p1(15,3)
        "movq            %%mm0,%%mm6 \n\t"                      //p1(16,0)

        "movq            %%mm3,%%mm7 \n\t"                      //p1(16,3)
        "punpcklwd       %%mm2,%%mm6 \n\t"                      //p1(16,1)

        "punpcklwd       %%mm3,%%mm7 \n\t"                      //p1(16,4)
        "pmaddwd         %8,%%mm6 \n\t"     //p1(16,2)

        "pmaddwd         %9,%%mm7 \n\t" //p1(16,5)
        "movq            %%mm0,%%mm4 \n\t"                      //p1(16,12)

        "paddd           8*24(%%esi),%%mm6 \n\t"        //p1(16,6)
        "punpckhwd       %%mm2,%%mm4 \n\t"                      //p1(16,13)

        "paddd           %%mm7,%%mm6 \n\t"                      //p1(16,7)
        "pmaddwd         %8,%%mm4 \n\t"     //p1(16,14)

        "movq            %%mm6,%%mm7 \n\t"                      //p1(16,8)
        "paddd           8*25(%%esi),%%mm4 \n\t"        //p1(16,18)

        "movq            %%mm3,%%mm5 \n\t"                      //p1(16,15)
        "paddd           8*16(%%esi),%%mm6 \n\t"        //p1(16,9)

        "punpckhwd       %%mm3,%%mm5 \n\t"                      //p1(16,16)
        "paddd           %10,%%mm6 \n\t"      //p1(16,10)

        "psrad           $9,%%mm6    \n\t"                      //p1(16,11)
        "pmaddwd         %9,%%mm5 \n\t" //p1(16,17)

        "paddd           %%mm5,%%mm4 \n\t"                      //p1(16,19)

        "movq            %%mm4,%%mm5 \n\t"                      //p1(16,20)

        "paddd           8*17(%%esi),%%mm4 \n\t"        //p1(16,21)

        "paddd           %10,%%mm4 \n\t"      //p1(16,22)

        "psrad           $9,%%mm4    \n\t"                      //p1(16,23)

        "packssdw        %%mm4,%%mm6 \n\t"                      //p1(16,24)

        "movq            %%mm6,8*0(%%esi) \n\t"         //p1(16,25)

        "movq            8*16(%%esi),%%mm4 \n\t"        //p1(16,26)

        "psubd           %%mm7,%%mm4 \n\t"                      //p1(16,27)
        "movq            8*17(%%esi),%%mm6 \n\t"        //p1(16,30)

        "paddd           %10,%%mm4 \n\t"      //p1(16,28)
        "movq            %%mm1,%%mm7 \n\t"                      //p1(17,3)

        "psrad           $9,%%mm4    \n\t"                      //p1(16,29)
        "psubd           %%mm5,%%mm6 \n\t"                      //p1(16,31)

        "paddd           %10,%%mm6 \n\t"      //p1(16,32)
        "punpcklwd       %%mm1,%%mm7 \n\t"                      //p1(17,4)

        "pmaddwd         %11,%%mm7 \n\t" //p1(17,5)
        "psrad           $9,%%mm6    \n\t"                      //p1(16,33)

        "packssdw        %%mm6,%%mm4 \n\t"                      //p1(16,34)
        "movq            %%mm4,8*14(%%esi) \n\t"        //p1(16,35)

        "movq            %%mm0,%%mm6 \n\t"                      //p1(17,0)
        "movq            %%mm0,%%mm4 \n\t"                      //p1(17,12)

        "punpcklwd       %%mm2,%%mm6 \n\t"                      //p1(17,1)
        "punpckhwd       %%mm2,%%mm4 \n\t"                      //p1(17,13)

        "pmaddwd         %12,%%mm6 \n\t"     //p1(17,2)
        "movq            %%mm1,%%mm5 \n\t"                      //p1(17,15)

        "paddd           8*24(%%esi),%%mm6 \n\t"        //p1(17,6)
        "punpckhwd       %%mm1,%%mm5 \n\t"                      //p1(17,16)

        "paddd           %%mm7,%%mm6 \n\t"                      //p1(17,7)
        "pmaddwd         %12,%%mm4 \n\t"     //p1(17,14)

        "movq            %%mm6,%%mm7 \n\t"                      //p1(17,8)
        "pmaddwd         %11,%%mm5 \n\t" //p1(17,17)

        "paddd           8*18(%%esi),%%mm6 \n\t"        //p1(17,9)

        "paddd           %10,%%mm6 \n\t"      //p1(17,10)

        "psrad           $9,%%mm6    \n\t"                      //p1(17,11)
        "paddd           8*25(%%esi),%%mm4 \n\t"        //p1(17,18)

        "paddd           %%mm5,%%mm4 \n\t"                      //p1(17,19)

        "movq            %%mm4,%%mm5 \n\t"                      //p1(17,20)

        "paddd           8*19(%%esi),%%mm4 \n\t"        //p1(17,21)

        "paddd           %10,%%mm4 \n\t"      //p1(17,22)

        "psrad           $9,%%mm4    \n\t"                      //p1(17,23)

        "packssdw        %%mm4,%%mm6 \n\t"                      //p1(17,24)

        "movq            %%mm6,8*2(%%esi) \n\t"         //p1(17,25)

        "movq            8*18(%%esi),%%mm4 \n\t"        //p1(17,26)

        "movq            8*19(%%esi),%%mm6 \n\t"        //p1(17,30)
        "psubd           %%mm7,%%mm4 \n\t"                      //p1(17,27)

        "paddd           %10,%%mm4 \n\t"      //p1(17,28)
        "psubd           %%mm5,%%mm6 \n\t"                      //p1(17,31)

        "psrad           $9,%%mm4    \n\t"                      //p1(17,29)
        "paddd           %10,%%mm6 \n\t"      //p1(17,32)

        "psrad           $9,%%mm6    \n\t"                      //p1(17,33)
        "movq            %%mm2,%%mm7 \n\t"                      //p1(18,3)

        "packssdw        %%mm6,%%mm4 \n\t"                      //p1(17,34)
        "movq            %%mm4,8*12(%%esi) \n\t"        //p1(17,35)

        "movq            %%mm1,%%mm6 \n\t"                      //p1(18,0)
        "punpcklwd       %%mm2,%%mm7 \n\t"                      //p1(18,4)

        "punpcklwd       %%mm3,%%mm6 \n\t"                      //p1(18,1)
        "pmaddwd         %13,%%mm7 \n\t" //p1(18,5)

        "pmaddwd         %14,%%mm6 \n\t"     //p1(18,2)
        "movq            %%mm1,%%mm4 \n\t"                      //p1(18,12)

        "paddd           8*24(%%esi),%%mm6 \n\t"        //p1(18,6)
        "punpckhwd       %%mm3,%%mm4 \n\t"                      //p1(18,13)

        "paddd           %%mm7,%%mm6 \n\t"                      //p1(18,7)
        "pmaddwd         %14,%%mm4 \n\t"     //p1(18,14)

        "movq            %%mm6,%%mm7 \n\t"                      //p1(18,8)
        "movq            %%mm2,%%mm5 \n\t"                      //p1(18,15)

        "paddd           8*20(%%esi),%%mm6 \n\t"        //p1(18,9)
        "punpckhwd       %%mm2,%%mm5 \n\t"                      //p1(18,16)

        "paddd           %10,%%mm6 \n\t"      //p1(18,10)

        "psrad           $9,%%mm6    \n\t"                      //p1(18,11)
        "pmaddwd         %13,%%mm5 \n\t" //p1(18,17)

        "paddd           8*25(%%esi),%%mm4 \n\t"        //p1(18,18)

        "paddd           %%mm5,%%mm4 \n\t"                      //p1(18,19)

        "movq            %%mm4,%%mm5 \n\t"                      //p1(18,20)

        "paddd           8*21(%%esi),%%mm4 \n\t"        //p1(18,21)

        "paddd           %10,%%mm4 \n\t"      //p1(18,22)

        "psrad           $9,%%mm4    \n\t"                      //p1(18,23)

        "packssdw        %%mm4,%%mm6 \n\t"                      //p1(18,24)

        "movq            %%mm6,8*4(%%esi) \n\t"         //p1(18,25)

        "movq            8*20(%%esi),%%mm4 \n\t"        //p1(18,26)

        "psubd           %%mm7,%%mm4 \n\t"                      //p1(18,27)

        "paddd           %10,%%mm4 \n\t"      //p1(18,28)
        "movq            %%mm0,%%mm7 \n\t"                      //p1(19,3)

        "psrad           $9,%%mm4    \n\t"                      //p1(18,29)
        "movq            8*21(%%esi),%%mm6 \n\t"        //p1(18,30)

        "psubd           %%mm5,%%mm6 \n\t"                      //p1(18,31)
        "punpcklwd       %%mm0,%%mm7 \n\t"                      //p1(19,4)

        "paddd           %10,%%mm6 \n\t"      //p1(18,32)

        "psrad           $9,%%mm6    \n\t"                      //p1(18,33)
        "pmaddwd         %15,%%mm7 \n\t" //p1(19,5)

        "packssdw        %%mm6,%%mm4 \n\t"                      //p1(18,34)

        "movq            %%mm4,8*10(%%esi) \n\t"        //p1(18,35)
        "movq            %%mm3,%%mm6 \n\t"                      //p1(19,0)

        "punpcklwd       %%mm1,%%mm6 \n\t"                      //p1(19,1)       
        "movq            %%mm0,%%mm5 \n\t"                      //p1(19,15)

        "pmaddwd         %16,%%mm6 \n\t"     //p1(19,2)
        "punpckhwd       %%mm0,%%mm5 \n\t"                      //p1(19,16)


        "paddd           8*24(%%esi),%%mm6 \n\t"        //p1(19,6)
        "movq            %%mm3,%%mm4 \n\t"                      //p1(19,12)

        "paddd           %%mm7,%%mm6 \n\t"                      //p1(19,7)
        "punpckhwd       %%mm1,%%mm4 \n\t"                      //p1(19,13)


        "movq            %%mm6,%%mm7 \n\t"                      //p1(19,8)
        "pmaddwd         %16,%%mm4 \n\t" //p1(19,14)

        "paddd           8*22(%%esi),%%mm6 \n\t"        //p1(19,9)

        "pmaddwd         %15,%%mm5 \n\t" //p1(19,17)

        "paddd           %10,%%mm6 \n\t"      //p1(19,10)

        "psrad           $9,%%mm6    \n\t"                      //p1(19,11)
        "paddd           8*25(%%esi),%%mm4 \n\t"        //p1(19,18)

        "paddd           %%mm5,%%mm4 \n\t"                      //p1(19,19)

        "movq            %%mm4,%%mm5 \n\t"                      //p1(19,20)
        "paddd           8*23(%%esi),%%mm4 \n\t"        //p1(19,21)
        "paddd           %10,%%mm4 \n\t"      //p1(19,22)
        "psrad           $9,%%mm4    \n\t"                      //p1(19,23)

        "packssdw        %%mm4,%%mm6 \n\t"                      //p1(19,24)
        "movq            %%mm6,8*6(%%esi) \n\t"         //p1(19,25)

        "movq            8*22(%%esi),%%mm4 \n\t"        //p1(19,26)

        "psubd           %%mm7,%%mm4 \n\t"                      //p1(19,27)
        "movq            8*23(%%esi),%%mm6 \n\t"        //p1(19,30)

        "paddd           %10,%%mm4 \n\t"      //p1(19,28)
        "psubd           %%mm5,%%mm6 \n\t"                      //p1(19,31)

        "psrad           $9,%%mm4    \n\t"                      //p1(19,29)
        "paddd           %10,%%mm6 \n\t"      //p1(19,32)

        "psrad           $9,%%mm6    \n\t"                      //p1(19,33)

        "packssdw        %%mm6,%%mm4 \n\t"                      //p1(19,34)
        "movq            %%mm4,8*8(%%esi) \n\t"         //p1(19,35)

        "movl                    %%eax,%%esi \n\t"

        "movl                    %3, %%edi \n\t"


        "movq            8*0(%%esi),%%mm0 \n\t" //tran(0)

        "movq            %%mm0,%%mm1 \n\t"              //tran(1)
        "movq            8*2(%%esi),%%mm2 \n\t" //tran(2)                

        "punpcklwd       %%mm2,%%mm0 \n\t"              //tran(3)
        "movq            8*4(%%esi),%%mm3 \n\t" //tran(5)

        "punpckhwd       %%mm2,%%mm1 \n\t"              //tran(4)
        "movq            8*6(%%esi),%%mm5 \n\t" //tran(7)

        "movq            %%mm3,%%mm4 \n\t"              //tran(6)
        "movq            %%mm0,%%mm6 \n\t"              //tran(10)

        "punpcklwd       %%mm5,%%mm3 \n\t"              //tran(8)
        "movq            %%mm1,%%mm7 \n\t"              //tran(11)

        "punpckldq       %%mm3,%%mm0 \n\t"              //tran(12)

        "punpckhwd       %%mm5,%%mm4 \n\t"              //tran(9)
        "movq            %%mm0,8*0(%%esi) \n\t" //tran(16)

        "punpckhdq       %%mm3,%%mm6 \n\t"              //tran(13)
        "movq            8*1(%%esi),%%mm0 \n\t" //tran(20)

        "punpckldq       %%mm4,%%mm1 \n\t"              //tran(14)
        "movq            %%mm6,8*2(%%esi) \n\t" //tran(17)

        "punpckhdq       %%mm4,%%mm7 \n\t"              //tran(15)
        "movq            %%mm1,8*4(%%esi) \n\t" //tran(18)

        "movq            %%mm0,%%mm1 \n\t"              //tran(21)
        "movq            8*5(%%esi),%%mm3 \n\t" //tran(25)

        "movq            8*3(%%esi),%%mm2 \n\t" //tran(22)
        "movq            %%mm3,%%mm4 \n\t"              //tran(26)

        "punpcklwd       %%mm2,%%mm0 \n\t"              //tran(23)
        "movq            %%mm7,8*6(%%esi) \n\t" //tran(19)

        "punpckhwd       %%mm2,%%mm1 \n\t"              //tran(24)
        "movq            8*7(%%esi),%%mm5 \n\t" //tran(27)

        "punpcklwd       %%mm5,%%mm3 \n\t"              //tran(28)
        "movq            %%mm0,%%mm6 \n\t"              //tran(30)

        "movq            %%mm1,%%mm7 \n\t"              //tran(31)
        "punpckhdq       %%mm3,%%mm6 \n\t"              //tran(33)

        "punpckhwd       %%mm5,%%mm4 \n\t"              //tran(29)
        "movq            %%mm6,%%mm2 \n\t"              //p2(1,0)

        "punpckhdq       %%mm4,%%mm7 \n\t"              //tran(35)
        "movq            8*2(%%esi),%%mm5 \n\t" //p2(1,2)

        "paddw           %%mm7,%%mm2 \n\t"              //p2(1,1)
        "paddw           8*6(%%esi),%%mm5 \n\t" //p2(1,3)

        "punpckldq       %%mm3,%%mm0 \n\t"              //tran(32)
        "paddw           %%mm5,%%mm2 \n\t"              //p2(1,4)

        "punpckldq       %%mm4,%%mm1 \n\t"              //tran(34)
        "movq            8*2(%%esi),%%mm5 \n\t"         //p2(3,0)

        "pmulhw          %7,%%mm2 \n\t" //p2(1,5)
        "movq            %%mm7,%%mm4 \n\t"                      //p2(2,0)

        "pmulhw          %17,%%mm4 \n\t" //p2(2,1)
        "movq            %%mm6,%%mm3 \n\t"                              //p2(6,0)

        "pmulhw          %18,%%mm3 \n\t"  //p2(6,1)

        "pmulhw          %19,%%mm5 \n\t"          //p2(3,1)

        "movq            %%mm2,8*24(%%eax) \n\t" //p2(1,6)

        "movq            8*6(%%esi),%%mm2 \n\t" //p2(4,0)

        "pmulhw          %20,%%mm2 \n\t"  //p2(4,1)

        "paddw           8*24(%%eax),%%mm4 \n\t"                //p2(5,0)

        "paddw           8*24(%%eax),%%mm3 \n\t"        //p2(9,0)

        "paddw           %%mm2,%%mm5 \n\t"                              //p2(5,1)

        "movq            8*2(%%esi),%%mm2 \n\t"         //p2(7,0)
        "paddw           %%mm4,%%mm5 \n\t"                              //p2(5,2)
        "pmulhw          %21,%%mm2 \n\t"          //p2(7,1)

        "movq            %%mm5,8*1(%%esi) \n\t"                 //p2(5,3)

        "movq            8*6(%%esi),%%mm4 \n\t" //p2(8,0)
        "movq            %%mm6,%%mm5 \n\t"              //p2(10,0)

        "pmulhw          %22,%%mm4 \n\t"  //p2(8,1)

        "pmulhw          %21,%%mm5 \n\t"  //p2(10,1)

        "pmulhw          %22,%%mm6 \n\t"          //p2(15,0)

        "paddw           %%mm4,%%mm2 \n\t"                      //p2(9,1)

        "movq            %%mm7,%%mm4 \n\t"              //p2(11,0)

        "pmulhw          %19,%%mm4 \n\t"  //p2(11,1)
        "paddw           %%mm3,%%mm2 \n\t"                      //p2(9,2)

        "movq            %%mm2,8*3(%%esi) \n\t"         //p2(9,3)

        "movq            8*2(%%esi),%%mm3 \n\t"         //p2(13,0)

        "pmulhw          %20,%%mm7 \n\t"          //p2(16,0)

        "pmulhw          %23,%%mm3 \n\t" //p2(13,1)
        "paddw           %%mm4,%%mm5 \n\t"              //p2(12,0)


        "paddw           8*24(%%eax),%%mm5 \n\t"        //p2(14,0)

        "movq            8*6(%%esi),%%mm2 \n\t"         //p2(18,0)

        "pmulhw          %24,%%mm2 \n\t" //p2(18,1)
        "paddw           %%mm3,%%mm5 \n\t"                      //p2(14,1)

        "movq            %%mm5,8*5(%%esi) \n\t"         //p2(14,2)
        "paddw           %%mm7,%%mm6 \n\t"                      //p2(17,0)

        "paddw           8*24(%%eax),%%mm6 \n\t"        //p2(19,0)
        "movq            %%mm1,%%mm3 \n\t"                      //p2(21,0)

        "movq            8*4(%%esi),%%mm4 \n\t"         //p2(20,0)
        "paddw           %%mm2,%%mm6 \n\t"                      //p2(19,1)

        "movq            %%mm6,8*7(%%esi) \n\t"         //p2(19,2)       
        "movq            %%mm4,%%mm5 \n\t"                      //p2(20,1)

        "movq            8*0(%%esi),%%mm7 \n\t"         //p2(26,0)

        "pmulhw          %25,%%mm4 \n\t"       //p2(20,2)       
        "psubw           %%mm0,%%mm7 \n\t"                      //p2(27,0)

        "pmulhw          %26,%%mm3 \n\t"           //p2(21,1)
        "movq            %%mm0,%%mm2 \n\t"                      //p2(26,1)

        "pmulhw          %26,%%mm5 \n\t"           //p2(23,0)
        "psraw           $3,%%mm7    \n\t"                      //p2(27,1)

        "paddw           8*0(%%esi),%%mm2 \n\t"         //p2(26,2)
        "movq            %%mm7,%%mm6 \n\t"                      //p2(28,0)

        "pmulhw          %27,%%mm1 \n\t"       //p2(24,0)
        "psraw           $3,%%mm2    \n\t"                      //p2(26,3)                       

        "paddw           %%mm3,%%mm4 \n\t"                      //p2(22,0)       
        "paddw           %%mm1,%%mm5 \n\t"                      //p2(25,0)

        "psubw           %%mm5,%%mm6 \n\t"                      //p2(29,0)       
        "movq            %%mm2,%%mm3 \n\t"                      //p2(30,0)

        "paddw           %%mm4,%%mm2 \n\t"                      //p2(30,1)               
        "paddw           %%mm5,%%mm7 \n\t"                      //p2(28,1)                       

        "movq            %%mm2,%%mm1 \n\t"                      //p2(32,0)
        "psubw           %%mm4,%%mm3 \n\t"                      //p2(31,0)               

        "paddw           8*5(%%esi),%%mm1 \n\t"         //p2(32,1)
        "movq            %%mm7,%%mm0 \n\t"                      //p2(33,0)

        "psubw           8*5(%%esi),%%mm2 \n\t"         //p2(32,2)
        "movq            %%mm6,%%mm4 \n\t"                      //p2(34,0)

        "paddw           %28,%%mm1 \n\t"      //p2(32,3)

        "paddw           %28,%%mm2 \n\t"      //p2(32,4)
        "psraw           $4,%%mm1    \n\t"                      //p2(32,5)

        "psraw           $4,%%mm2    \n\t"                      //p2(32,6)
        "paddw           8*7(%%esi),%%mm7 \n\t"         //p2(33,1)

        "packuswb        %%mm2,%%mm1 \n\t"                      //p2(32,7)
        "psubw           8*7(%%esi),%%mm0 \n\t"         //p2(33,2)

        "paddw           %28,%%mm7 \n\t"      //p2(33,3)

        "paddw           %28,%%mm0 \n\t"      //p2(33,4)
        "psraw           $4,%%mm7    \n\t"                      //p2(33,5)

        "psraw           $4,%%mm0    \n\t"                      //p2(33,6)
        "paddw           8*3(%%esi),%%mm4 \n\t"         //p2(34,1)

        "packuswb        %%mm0,%%mm7 \n\t"                      //p2(33,7)
        "psubw           8*3(%%esi),%%mm6 \n\t"         //p2(34,2)

        "paddw           %28,%%mm4 \n\t"      //p2(34,3)
        "movq            %%mm3,%%mm5 \n\t"                      //p2(35,0)

        "paddw           %28,%%mm6 \n\t"      //p2(34,4)
        "psraw           $4,%%mm4    \n\t"                      //p2(34,5)

        "psraw           $4,%%mm6    \n\t"                      //p2(34,6)
        "paddw           8*1(%%esi),%%mm3 \n\t"         //p2(35,1)

        "packuswb        %%mm6,%%mm4 \n\t"                      //p2(34,7)
        "psubw           8*1(%%esi),%%mm5 \n\t"         //p2(35,2)

        "movq            %%mm1,%%mm0 \n\t"                      //p2(36,0)
        "paddw           %28,%%mm3 \n\t"      //p2(35,3)

        "paddw           %28,%%mm5 \n\t"      //p2(35,4)
        "punpcklbw       %%mm7,%%mm0 \n\t"                      //p2(36,1)

        "psraw           $4,%%mm3    \n\t"                      //p2(35,5)
        "movq            %%mm4,%%mm2 \n\t"                      //p2(37,0)

        "psraw           $4,%%mm5    \n\t"                      //p2(35,6)
        "movq            %%mm0,%%mm6 \n\t"                      //p2(38,0)

        "packuswb        %%mm5,%%mm3 \n\t"                      //p2(35,7)
        "movl                    (%%edi),%%ebx \n\t"                    //p2(42,0)

        "punpckhbw       %%mm1,%%mm7 \n\t"                      //p2(36,2)
        "movl                    4(%%edi),%%ecx \n\t"           //p2(42,1)

        "punpcklbw       %%mm3,%%mm2 \n\t"                      //p2(37,1)       
        "movl                    8(%%edi),%%edx \n\t"           //p2(42,2)

        "punpckhbw       %%mm4,%%mm3 \n\t"                      //p2(37,2)       
        "addl                    %4, %%ebx \n\t"       //p2(42,3)

        "punpcklwd       %%mm2,%%mm0 \n\t"                      //p2(38,1)       
        "movq            %%mm3,%%mm5 \n\t"                      //p2(39,0)

        "punpckhwd       %%mm2,%%mm6 \n\t"                      //p2(38,2)       
        "movq            %%mm0,%%mm1 \n\t"                      //p2(40,0)

        "punpcklwd       %%mm7,%%mm3 \n\t"                      //p2(39,1)       
        "addl                    %4, %%ecx \n\t"       //p2(42,4)

        "addl                    %4, %%edx \n\t"       //p2(42,5)
        "punpckldq       %%mm3,%%mm0 \n\t"                      //p2(40,1)       

        "punpckhdq       %%mm3,%%mm1 \n\t"                      //p2(40,2)       
        "movq            %%mm0,(%%ebx) \n\t"                    //p2(43,0)       

        "punpckhwd       %%mm7,%%mm5 \n\t"                      //p2(39,2)       
        "movq            %%mm1,(%%ecx) \n\t"                    //p2(43,1)       

        "movq            %%mm6,%%mm4 \n\t"                      //p2(41,0)
        "movl                    12(%%edi),%%ebx \n\t"          //p2(43,3)

        "punpckldq       %%mm5,%%mm4 \n\t"                      //p2(41,1)       
        "addl                    %4, %%ebx \n\t"       //p2(43,4)

        "punpckhdq       %%mm5,%%mm6 \n\t"                      //p2(41,2)       
        "movq            %%mm4,(%%edx) \n\t"                    //p2(43,2)               

        "movq            %%mm6,(%%ebx) \n\t"                    //p2(43,5)


                                                                   /*   Process next 4 rows */

        "addl                    $64,%%esi \n\t"
        "addl                    $16,%%edi \n\t"

                                                                   /*transpose next 4 rows of wsptr */

        "movq            8*0(%%esi),%%mm0 \n\t" //tran(0)

        "movq            %%mm0,%%mm1 \n\t"              //tran(1)
        "movq            8*2(%%esi),%%mm2 \n\t" //tran(2)                

        "punpcklwd       %%mm2,%%mm0 \n\t"              //tran(3)
        "movq            8*4(%%esi),%%mm3 \n\t" //tran(5)

        "punpckhwd       %%mm2,%%mm1 \n\t"              //tran(4)
        "movq            8*6(%%esi),%%mm5 \n\t" //tran(7)

        "movq            %%mm3,%%mm4 \n\t"              //tran(6)
        "movq            %%mm0,%%mm6 \n\t"              //tran(10)

        "punpcklwd       %%mm5,%%mm3 \n\t"              //tran(8)
        "movq            %%mm1,%%mm7 \n\t"              //tran(11)

        "punpckldq       %%mm3,%%mm0 \n\t"              //tran(12)

        "punpckhwd       %%mm5,%%mm4 \n\t"              //tran(9)
        "movq            %%mm0,8*0(%%esi) \n\t" //tran(16)

        "punpckhdq       %%mm3,%%mm6 \n\t"              //tran(13)
        "movq            8*1(%%esi),%%mm0 \n\t" //tran(20)

        "punpckldq       %%mm4,%%mm1 \n\t"              //tran(14)
        "movq            %%mm6,8*2(%%esi) \n\t" //tran(17)

        "punpckhdq       %%mm4,%%mm7 \n\t"              //tran(15)
        "movq            %%mm1,8*4(%%esi) \n\t" //tran(18)

        "movq            %%mm0,%%mm1 \n\t"              //tran(21)
        "movq            8*5(%%esi),%%mm3 \n\t" //tran(25)

        "movq            8*3(%%esi),%%mm2 \n\t" //tran(22)
        "movq            %%mm3,%%mm4 \n\t"              //tran(26)

        "punpcklwd       %%mm2,%%mm0 \n\t"              //tran(23)
        "movq            %%mm7,8*6(%%esi) \n\t" //tran(19)

        "punpckhwd       %%mm2,%%mm1 \n\t"              //tran(24)
        "movq            8*7(%%esi),%%mm5 \n\t" //tran(27)

        "punpcklwd       %%mm5,%%mm3 \n\t"              //tran(28)
        "movq            %%mm0,%%mm6 \n\t"              //tran(30)

        "movq            %%mm1,%%mm7 \n\t"              //tran(31)
        "punpckhdq       %%mm3,%%mm6 \n\t"              //tran(33)

        "punpckhwd       %%mm5,%%mm4 \n\t"              //tran(29)
        "movq            %%mm6,%%mm2 \n\t"              //p2(1,0)

        "punpckhdq       %%mm4,%%mm7 \n\t"              //tran(35)
        "movq            8*2(%%esi),%%mm5 \n\t" //p2(1,2)

        "paddw           %%mm7,%%mm2 \n\t"              //p2(1,1)
        "paddw           8*6(%%esi),%%mm5 \n\t" //p2(1,3)

        "punpckldq       %%mm3,%%mm0 \n\t"              //tran(32)
        "paddw           %%mm5,%%mm2 \n\t"              //p2(1,4)

        "punpckldq       %%mm4,%%mm1 \n\t"              //tran(34)
        "movq            8*2(%%esi),%%mm5 \n\t"         //p2(3,0)

        "pmulhw          %7,%%mm2 \n\t" //p2(1,5)
        "movq            %%mm7,%%mm4 \n\t"                      //p2(2,0)

        "pmulhw          %17,%%mm4 \n\t" //p2(2,1)
        "movq            %%mm6,%%mm3 \n\t"                              //p2(6,0)

        "pmulhw          %18,%%mm3 \n\t"  //p2(6,1)

        "pmulhw          %19,%%mm5 \n\t"          //p2(3,1)

        "movq            %%mm2,8*24(%%eax) \n\t" //p2(1,6)

        "movq            8*6(%%esi),%%mm2 \n\t" //p2(4,0)

        "pmulhw          %20,%%mm2 \n\t"  //p2(4,1)

        "paddw           8*24(%%eax),%%mm4 \n\t"                //p2(5,0)

        "paddw           8*24(%%eax),%%mm3 \n\t"        //p2(9,0)

        "paddw           %%mm2,%%mm5 \n\t"                              //p2(5,1)

        "movq            8*2(%%esi),%%mm2 \n\t"         //p2(7,0)
        "paddw           %%mm4,%%mm5 \n\t"                              //p2(5,2)
        "pmulhw          %21,%%mm2 \n\t"          //p2(7,1)

        "movq            %%mm5,8*1(%%esi) \n\t"                 //p2(5,3)

        "movq            8*6(%%esi),%%mm4 \n\t" //p2(8,0)
        "movq            %%mm6,%%mm5 \n\t"              //p2(10,0)

        "pmulhw          %22,%%mm4 \n\t"  //p2(8,1)

        "pmulhw          %21,%%mm5 \n\t"  //p2(10,1)

        "pmulhw          %22,%%mm6 \n\t"          //p2(15,0)

        "paddw           %%mm4,%%mm2 \n\t"                      //p2(9,1)

        "movq            %%mm7,%%mm4 \n\t"              //p2(11,0)

        "pmulhw          %19,%%mm4 \n\t"  //p2(11,1)
        "paddw           %%mm3,%%mm2 \n\t"                      //p2(9,2)

        "movq            %%mm2,8*3(%%esi) \n\t"         //p2(9,3)

        "movq            8*2(%%esi),%%mm3 \n\t"         //p2(13,0)

        "pmulhw          %20,%%mm7 \n\t"          //p2(16,0)

        "pmulhw          %23,%%mm3 \n\t" //p2(13,1)
        "paddw           %%mm4,%%mm5 \n\t"              //p2(12,0)


        "paddw           8*24(%%eax),%%mm5 \n\t"        //p2(14,0)

        "movq            8*6(%%esi),%%mm2 \n\t"         //p2(18,0)

        "pmulhw          %24,%%mm2 \n\t" //p2(18,1)
        "paddw           %%mm3,%%mm5 \n\t"                      //p2(14,1)

        "movq            %%mm5,8*5(%%esi) \n\t"         //p2(14,2)
        "paddw           %%mm7,%%mm6 \n\t"                      //p2(17,0)

        "paddw           8*24(%%eax),%%mm6 \n\t"        //p2(19,0)
        "movq            %%mm1,%%mm3 \n\t"                      //p2(21,0)

        "movq            8*4(%%esi),%%mm4 \n\t"         //p2(20,0)
        "paddw           %%mm2,%%mm6 \n\t"                      //p2(19,1)

        "movq            %%mm6,8*7(%%esi) \n\t"         //p2(19,2)       
        "movq            %%mm4,%%mm5 \n\t"                      //p2(20,1)

        "movq            8*0(%%esi),%%mm7 \n\t"         //p2(26,0)

        "pmulhw          %25,%%mm4 \n\t"       //p2(20,2)       
        "psubw           %%mm0,%%mm7 \n\t"                      //p2(27,0)

        "pmulhw          %26,%%mm3 \n\t"           //p2(21,1)
        "movq            %%mm0,%%mm2 \n\t"                      //p2(26,1)

        "pmulhw          %26,%%mm5 \n\t"           //p2(23,0)
        "psraw           $3,%%mm7    \n\t"                      //p2(27,1)

        "paddw           8*0(%%esi),%%mm2 \n\t"         //p2(26,2)
        "movq            %%mm7,%%mm6 \n\t"                      //p2(28,0)

        "pmulhw          %27,%%mm1 \n\t"       //p2(24,0)
        "psraw           $3,%%mm2    \n\t"                      //p2(26,3)                       

        "paddw           %%mm3,%%mm4 \n\t"                      //p2(22,0)       
        "paddw           %%mm1,%%mm5 \n\t"                      //p2(25,0)

        "psubw           %%mm5,%%mm6 \n\t"                      //p2(29,0)       
        "movq            %%mm2,%%mm3 \n\t"                      //p2(30,0)

        "paddw           %%mm4,%%mm2 \n\t"                      //p2(30,1)               
        "paddw           %%mm5,%%mm7 \n\t"                      //p2(28,1)                       

        "movq            %%mm2,%%mm1 \n\t"                      //p2(32,0)
        "psubw           %%mm4,%%mm3 \n\t"                      //p2(31,0)               

        "paddw           8*5(%%esi),%%mm1 \n\t"         //p2(32,1)
        "movq            %%mm7,%%mm0 \n\t"                      //p2(33,0)

        "psubw           8*5(%%esi),%%mm2 \n\t"         //p2(32,2)
        "movq            %%mm6,%%mm4 \n\t"                      //p2(34,0)

        "paddw           %28,%%mm1 \n\t"      //p2(32,3)

        "paddw           %28,%%mm2 \n\t"      //p2(32,4)
        "psraw           $4,%%mm1    \n\t"                      //p2(32,5)

        "psraw           $4,%%mm2    \n\t"                      //p2(32,6)
        "paddw           8*7(%%esi),%%mm7 \n\t"         //p2(33,1)

        "packuswb        %%mm2,%%mm1 \n\t"                      //p2(32,7)
        "psubw           8*7(%%esi),%%mm0 \n\t"         //p2(33,2)

        "paddw           %28,%%mm7 \n\t"      //p2(33,3)

        "paddw           %28,%%mm0 \n\t"      //p2(33,4)
        "psraw           $4,%%mm7    \n\t"                      //p2(33,5)

        "psraw           $4,%%mm0    \n\t"                      //p2(33,6)
        "paddw           8*3(%%esi),%%mm4 \n\t"         //p2(34,1)

        "packuswb        %%mm0,%%mm7 \n\t"                      //p2(33,7)
        "psubw           8*3(%%esi),%%mm6 \n\t"         //p2(34,2)

        "paddw           %28,%%mm4 \n\t"      //p2(34,3)
        "movq            %%mm3,%%mm5 \n\t"                      //p2(35,0)

        "paddw           %28,%%mm6 \n\t"      //p2(34,4)
        "psraw           $4,%%mm4    \n\t"                      //p2(34,5)

        "psraw           $4,%%mm6    \n\t"                      //p2(34,6)
        "paddw           8*1(%%esi),%%mm3 \n\t"         //p2(35,1)

        "packuswb        %%mm6,%%mm4 \n\t"                      //p2(34,7)
        "psubw           8*1(%%esi),%%mm5 \n\t"         //p2(35,2)

        "movq            %%mm1,%%mm0 \n\t"                      //p2(36,0)
        "paddw           %28,%%mm3 \n\t"      //p2(35,3)

        "paddw           %28,%%mm5 \n\t"      //p2(35,4)
        "punpcklbw       %%mm7,%%mm0 \n\t"                      //p2(36,1)

        "psraw           $4,%%mm3    \n\t"                      //p2(35,5)
        "movq            %%mm4,%%mm2 \n\t"                      //p2(37,0)

        "psraw           $4,%%mm5    \n\t"                      //p2(35,6)
        "movq            %%mm0,%%mm6 \n\t"                      //p2(38,0)

        "packuswb        %%mm5,%%mm3 \n\t"                      //p2(35,7)
        "movl                    (%%edi),%%ebx \n\t"                    //p2(42,0)

        "punpckhbw       %%mm1,%%mm7 \n\t"                      //p2(36,2)
        "movl                    4(%%edi),%%ecx \n\t"           //p2(42,1)

        "punpcklbw       %%mm3,%%mm2 \n\t"                      //p2(37,1)       
        "movl                    8(%%edi),%%edx \n\t"           //p2(42,2)

        "punpckhbw       %%mm4,%%mm3 \n\t"                      //p2(37,2)       
        "addl                    %4, %%ebx \n\t"       //p2(42,3)

        "punpcklwd       %%mm2,%%mm0 \n\t"                      //p2(38,1)       
        "movq            %%mm3,%%mm5 \n\t"                      //p2(39,0)

        "punpckhwd       %%mm2,%%mm6 \n\t"                      //p2(38,2)       
        "movq            %%mm0,%%mm1 \n\t"                      //p2(40,0)

        "punpcklwd       %%mm7,%%mm3 \n\t"                      //p2(39,1)       
        "addl                    %4, %%ecx \n\t"       //p2(42,4)

        "addl                    %4, %%edx \n\t"       //p2(42,5)
        "punpckldq       %%mm3,%%mm0 \n\t"                      //p2(40,1)       

        "punpckhdq       %%mm3,%%mm1 \n\t"                      //p2(40,2)       
        "movq            %%mm0,(%%ebx) \n\t"                    //p2(43,0)       

        "punpckhwd       %%mm7,%%mm5 \n\t"                      //p2(39,2)       
        "movq            %%mm1,(%%ecx) \n\t"                    //p2(43,1)       

        "movq            %%mm6,%%mm4 \n\t"                      //p2(41,0)
        "movl                    12(%%edi),%%ebx \n\t"          //p2(43,3)

        "punpckldq       %%mm5,%%mm4 \n\t"                      //p2(41,1)       
        "addl                    %4, %%ebx \n\t"       //p2(43,4)

        "punpckhdq       %%mm5,%%mm6 \n\t"                      //p2(41,2)       
        "movq            %%mm4,(%%edx) \n\t"                    //p2(43,2)               

        "movq            %%mm6,(%%ebx) \n\t"                    //p2(43,5)

                                                                   /************************************************************************/

        "emms                        \n\t"

	:
	//      %0           %1             %2       %3            %4
	: "m"(quantptr), "m"(inptr), "m"(wsptr), "m"(outptr), "g"(output_col),
	//    %5                    %6                    %7
	  "m"(fix_054n184_054), "m"(fix_054_054p076), "m"(fix_117_117),
	//    %8                  %9                     %10
	  "m"(fix_n039_n089), "m"(fix_150_n089n039), "m"(const_0x2xx8),
	//    %11                    %12                 %13
	  "m"(fix_307n256_n196), "m"(fix_n256_n196), "m"(fix_205_n256n039),
	//    %14                 %15                    %16
	  "m"(fix_n039_n256), "m"(fix_029_n089n196), "m"(fix_n196_n089),
	//    %17                    %18                    %19
	  "m"(fix_n089n196p029), "m"(fix_n256n039p205), "m"(fix_n089),
	//    %20            %21            %22            %23
	  "m"(fix_n196), "m"(fix_n039), "m"(fix_n256), "m"(fix_n089n039p150),
	//    %24                    %25               %26           %27
	  "m"(fix_n196p307n256), "m"(fix_054p076), "m"(fix_054), "m"(fix_054n184),
	//    %28
	  "m"(const_0x0808)
	: "eax", "ebx", "ecx", "edx", "edi", "esi", "cc", "memory", "st"
        );
#endif /* ATT style assembler */








}


#endif /* DCT_ISLOW_SUPPORTED */
