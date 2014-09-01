/**************************************************************************
 *                                                                        *
 * This code is developed by Eugene Kuznetsov.  This software is an       *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 *
 *  halfpel_mmx_linux.c, half-pixel interpolation routines, MMX version, 
 *   AT&T syntax
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Eugene Kuznetsov
 *  John Funnell
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

#include "mbmotionestcomp/halfpel.h"

//#define PENTIUMIII
/* Having PENTIUMIII defined makes no noticeable difference in performance */

#ifdef PENTIUMIII
#define MOVNTQ "movntq"
#else
#define MOVNTQ "movq"
#endif

const uint64_t mm_7f7f7f7f7f7f7f7f = 0x7f7f7f7f7f7f7f7fLL;
const uint64_t mm_0002000200020002 = 0x0002000200020002LL;
const uint64_t mm_0001000100010001 = 0x0001000100010001LL;
const uint64_t mm_0202020202020202 = 0x0202020202020202LL;
const uint64_t mm_3f3f3f3f3f3f3f3f = 0x3f3f3f3f3f3f3f3fLL;
const uint64_t mm_0101010101010101 = 0x0101010101010101LL;
const uint64_t mm_f0f0f0f0f0f0f0f0 = 0xf0f0f0f0f0f0f0f0LL;
const uint64_t mm_0f0f0f0f0f0f0f0f = 0x0f0f0f0f0f0f0f0fLL;
const uint64_t mm_FE = 0xfefefefefefefefeLL;
static __inline void interpolate_halfpel_h_noround(
	uint8_t *src, 
	uint8_t *dstH, 
	int width,  /* width % 16 == 0 */
	int height)
{
	__asm__ __volatile__ 
	(
	"movl %3, %%edi\n"
	"pushl %%ebx\n"
	"movl %%ecx, %%ebx\n"
	"movq mm_7f7f7f7f7f7f7f7f, %%mm7\n"
"horiz_round0_loop_y:\n"
	"movl %%edx, %%esi\n"
"horiz_round0_loop_x:\n"
	"movq (%%eax), %%mm0\n"
	"movq 8(%%eax), %%mm4\n"
	"movq 1(%%eax), %%mm1\n"
	"movq 9(%%eax), %%mm5\n"
	
	"movq %%mm0, %%mm2\n"
	"pxor %%mm1, %%mm0\n" // mm0 = src[0] ^ src[1]
	"por %%mm2, %%mm1\n"  // mm1 = src[0] | src[1]
	"psrlq $1, %%mm0\n" 
	"addl $16, %%eax\n"
	"pand %%mm7, %%mm0\n" 
	"psubb %%mm0, %%mm1\n" 
	MOVNTQ " %%mm1, (%%ebx)\n"
	
	"movq %%mm4, %%mm6\n"
	"pxor %%mm5, %%mm4\n"	
	"por %%mm6, %%mm5\n"
	"psrlq $1, %%mm4\n"
	"pand %%mm7, %%mm4\n"
	"psubb %%mm4, %%mm5\n"
	MOVNTQ " %%mm5, 8(%%ebx)\n"
	
	"addl $16, %%ebx\n"
	"decl %%esi\n"
	"jne horiz_round0_loop_x\n"
	"decl %%edi\n"
	"jne horiz_round0_loop_y\n"
	
	"emms\n"		
	"popl %%ebx\n"
	:
	: "a" (src), "c" (dstH), "d" (width >> 4), "g" (height)
	: "esi", "edi"
	);
}
static __inline void interpolate_halfpel_h_round(
	uint8_t *src, 
	uint8_t *dstH, 
	int width,  /* width % 16 == 0 */
	int height)
{
	__asm__ __volatile__ 
	(
	"movl %3, %%edi\n"
	"pushl %%ebx\n"
	"movl %%ecx, %%ebx\n"
//	"movq mm_7f7f7f7f7f7f7f7f, %%mm7\n"
	"movq mm_FE, %%mm3\n"
"horiz_round1_loop_y:\n"
	"movl %%edx, %%esi\n"
"horiz_round1_loop_x:\n"
	"movq (%%eax), %%mm0\n"
	"movq 1(%%eax), %%mm1\n"
	
//(a+b)/2=((a&b) + ((a^b)>>1))
	"movq %%mm0, %%mm2\n"
	"pxor %%mm1, %%mm2\n"
	"pand %%mm3, %%mm2\n"
	"pand %%mm1, %%mm0\n"
	"psrlq $1, %%mm2\n"
	"paddb %%mm2, %%mm0\n"
	MOVNTQ " %%mm0, (%%ebx)\n"

//	"movq 8(%%eax), %%mm4\n"
//	"movq 9(%%eax), %%mm5\n"
//	"movq %%mm4, %%mm2\n"
//	"pxor %%mm5, %%mm2\n"
//	"pand %%mm3, %%mm2\n"
//	"pand %%mm5, %%mm4\n"
//	"psrlq $1, %%mm2\n"
//	"paddb %%mm2, %%mm4\n"
//	MOVNTQ " %%mm4, 8(%%ebx)\n"

	"addl $8, %%ebx\n"
	"addl $8, %%eax\n"
	"decl %%esi\n"
	"jne horiz_round1_loop_x\n"
	"decl %%edi\n"
	"jne horiz_round1_loop_y\n"
	
	"emms\n"		
	"popl %%ebx\n"
	:
	: "a" (src), "c" (dstH), "d" (width >> 3), "g" (height)
	: "esi", "edi"
	);
}

static __inline void interpolate_halfpel_v_noround(
	uint8_t *src, 
	uint8_t *dstV, 
	int width,  
	int height)
{
	__asm__ __volatile__ 
	(
	"movl %3, %%edi\n"
	"pushl %%ebx\n"
	"movl %%ecx, %%ebx\n"
	"movq mm_7f7f7f7f7f7f7f7f, %%mm7\n"
"vert_round0_loop_y:\n"
	"movl %%edx, %%esi\n"
	"shrl $4, %%esi\n"
"vert_round0_loop_x:\n"
	"movq (%%eax), %%mm0\n"
	"movq 8(%%eax), %%mm4\n"
	"movq (%%eax,%%edx), %%mm1\n"
	"movq 8(%%eax,%%edx), %%mm5\n"

	"movq %%mm0, %%mm2\n"
	"pxor %%mm1, %%mm0\n"			
	"por %%mm2, %%mm1\n"
	"psrlq $1, %%mm0\n"
	"addl $16, %%eax\n"
	"pand %%mm7, %%mm0\n"
	"psubb %%mm0, %%mm1\n"
	MOVNTQ " %%mm1, (%%ebx)\n"

	"movq %%mm4, %%mm6\n"
	"pxor %%mm5, %%mm4\n"
	"por %%mm6, %%mm5\n"
	"psrlq $1, %%mm4\n"
	"pand %%mm7, %%mm4\n"
	"psubb %%mm4, %%mm5\n"
	MOVNTQ " %%mm5, 8(%%ebx)\n"

	"addl $16, %%ebx\n"
	"decl %%esi\n"
	"jne vert_round0_loop_x\n"
	"decl %%edi\n"
	"jne vert_round0_loop_y\n"
	
	"emms\n"		
	"popl %%ebx\n"
	:
	: "a" (src), "c" (dstV), "d" (width), "g" (height)
	: "esi", "edi"
	);
}

static __inline void interpolate_halfpel_v_round(
	uint8_t *src, 
	uint8_t *dstV, 
	int width,  
	int height)
{
	__asm__ __volatile__ 
	(
	"movl %3, %%edi\n"
	"pushl %%ebx\n"
	"movl %%ecx, %%ebx\n"
	"movq mm_FE, %%mm3\n"
"vert_round1_loop_y:\n"
	"movl %%edx, %%esi\n"
	"shrl $3, %%esi\n"
"vert_round1_loop_x:\n"
	"movq (%%eax), %%mm0\n"
	"movq (%%eax,%%edx), %%mm1\n"
	
//(a+b)/2=((a&b) + ((a^b)>>1))
	"movq %%mm0, %%mm2\n"
	"pxor %%mm1, %%mm2\n"
	"pand %%mm3, %%mm2\n"
	"pand %%mm1, %%mm0\n"
	"psrlq $1, %%mm2\n"
	"paddb %%mm2, %%mm0\n"
	MOVNTQ " %%mm0, (%%ebx)\n"

	"addl $8, %%ebx\n"
	"addl $8, %%eax\n"
	"decl %%esi\n"
	"jne vert_round1_loop_x\n"
	"decl %%edi\n"
	"jne vert_round1_loop_y\n"
	
	"emms\n"		
	"popl %%ebx\n"
	:
	: "a" (src), "c" (dstV), "d" (width), "g" (height)
	: "esi", "edi"
	);
}

void interpolate_halfpel_h(
	uint8_t *src, 
	uint8_t *dstH, 
	int width,  /* width % 16 == 0 */
	int height,
	int rounding)
{
    if(!rounding)
	interpolate_halfpel_h_noround(src, dstH, width, height);
    else
	interpolate_halfpel_h_round(src, dstH, width, height);
}

void interpolate_halfpel_v(
	uint8_t *src, 
	uint8_t *dstV, 
	int width,  /* width % 16 == 0 */
	int height,
	int rounding)
{
    if(!rounding)
	interpolate_halfpel_v_noround(src, dstV, width, height);
    else
	interpolate_halfpel_v_round(src, dstV, width, height);
}

void interpolate_halfpel_hv(
	uint8_t *src, 
	uint8_t *dstHV, 
	int width,  
	int height,
	int rounding)
{
		__asm__ __volatile__
		(
		"movl %5, %%eax\n"
		"testl %%eax, %%eax\n"
		"jnz 1f\n"
		"movq mm_0002000200020002, %%mm6\n"
		"jmp 2f\n"
"1:\n"
		"movq mm_0001000100010001, %%mm6\n"
"2:\n"
		"movl %0, %%eax\n"
		"movl %3, %%edi\n"
		"movl %1, %%esi\n"
		"pushl %%ebx\n"
		"movl %%esi, %%ebx\n"
		"pxor %%mm7, %%mm7\n"
"horizvert_round0_loop_y:\n"
		"movl %%edx, %%esi\n"
"horizvert_round0_loop_x:\n"
		"movq (%%eax), %%mm0\n"		
		"movq %%mm0, %%mm1\n"
		"punpcklbw %%mm7, %%mm0\n"
		"movq 1(%%eax), %%mm2\n"
		"punpckhbw %%mm7, %%mm1\n"
		"movq %%mm2, %%mm3\n"
		"punpcklbw %%mm7, %%mm2\n"
		"paddw %%mm6, %%mm0\n"
		"punpckhbw %%mm7, %%mm3\n"
		"paddw %%mm6, %%mm1\n"
		"movq (%%ecx), %%mm4\n"
		"paddw %%mm2, %%mm0\n"
		"movq %%mm4, %%mm5\n"
		"punpcklbw %%mm7, %%mm4\n"
		"paddw %%mm3, %%mm1\n"
		"punpckhbw %%mm7, %%mm5\n"
		"paddw %%mm4, %%mm0\n"
		"paddw %%mm5, %%mm1\n"
		"movq 1(%%ecx), %%mm4\n"
		"movq %%mm4, %%mm5\n"
		"punpcklbw %%mm7, %%mm4\n"
#ifdef PENTIUMIII
		"prefetchnta 32(%%ecx)\n"
#endif
		"punpckhbw %%mm7, %%mm5\n"
		"paddw %%mm0, %%mm4\n"
		"paddw %%mm1, %%mm5\n"
		"psrlw $2, %%mm4\n"
		"movq 8(%%eax), %%mm0\n"
		"psrlw $2, %%mm5\n"
		"movq  %%mm0, %%mm1\n"
		"punpcklbw %%mm7, %%mm0\n"
		"movq 9(%%eax), %%mm2\n"
		"punpckhbw %%mm7, %%mm1\n"
		"movq %%mm2, %%mm3\n"
		"packuswb %%mm5, %%mm4\n"
		"paddw %%mm6, %%mm0\n"
		"punpcklbw %%mm7, %%mm2\n"
		"addl $16, %%eax\n"
		"punpckhbw %%mm7, %%mm3\n"
		MOVNTQ " %%mm4, (%%ebx)\n"
		"paddw %%mm6, %%mm1\n"
		"movq 8(%%ecx), %%mm4\n"
		"paddw %%mm2, %%mm0\n"
		"movq %%mm4, %%mm5\n"
		"punpcklbw %%mm7, %%mm4\n"
		"paddw %%mm3, %%mm1\n"
		"punpckhbw %%mm7, %%mm5\n"
		"paddw %%mm4, %%mm0\n"
		"paddw %%mm5, %%mm1\n"
		"movq 9(%%ecx), %%mm4\n"
		"movq %%mm4, %%mm5\n"
		"punpcklbw %%mm7, %%mm4\n"
		"addl $16, %%ecx\n"
		"punpckhbw %%mm7, %%mm5\n"
		"paddw %%mm0, %%mm4\n"
		"paddw %%mm1, %%mm5\n"
		"psrlw $2, %%mm4\n"
		"psrlw $2, %%mm5\n"
		"packuswb %%mm5, %%mm4\n"
		MOVNTQ " %%mm4, 8(%%ebx)\n"
		"addl $16, %%ebx\n"
		"decl %%esi\n"
		"jne horizvert_round0_loop_x\n"
		"decl %%edi\n"
		"jne horizvert_round0_loop_y\n"
		
		"popl %%ebx\n"
		"emms\n"
		:
		: "g"(src), "g" (dstHV), "c"(src+width), 
		    "g"(height), "d"(width >> 4), "g"(rounding)
		: "eax", "esi", "edi"
    );
}
