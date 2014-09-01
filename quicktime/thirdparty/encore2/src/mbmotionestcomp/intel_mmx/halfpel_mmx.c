/**************************************************************************
 *                                                                        *
 * This code is developed by John Funnell.  This software is an           *
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
 *  halfpel_mmx.c, half-pixel interpolation routines, MMX version, 
 *   Intel syntax
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  John Funnell
 *  Eugene Kuznetsov
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/*

 interpolate_halfpel.c

 Generates interpolated planes for use by the encoder in half-pel motion estimation 
 and motion compensation.
 
 John Funnell, 23 March 2001

 (C) Project Mayo 2001
 
*/
#include "../halfpel.h"

//#define PENTIUMIII

#ifdef PENTIUMIII
#define MOVNTQ movntq
#else
#define MOVNTQ movq
#define prefetchnta //
#endif

const uint64_t mm_7f7f7f7f7f7f7f7f = 0x7f7f7f7f7f7f7f7f;
const uint64_t mm_0002000200020002 = 0x0002000200020002;
const uint64_t mm_0001000100010001 = 0x0001000100010001;
const uint64_t mm_0202020202020202 = 0x0202020202020202;
const uint64_t mm_3f3f3f3f3f3f3f3f = 0x3f3f3f3f3f3f3f3f;
const uint64_t mm_0101010101010101 = 0x0101010101010101;
const uint64_t mm_f0f0f0f0f0f0f0f0 = 0xf0f0f0f0f0f0f0f0;
const uint64_t mm_0f0f0f0f0f0f0f0f = 0x0f0f0f0f0f0f0f0f;

static __inline void interpolate_halfpel_h_noround(
	uint8_t *src, 
	uint8_t *dstH, 
	int width,  /* width % 16 == 0 */
	int height)
{
	int y, xcount, flyback;
	
//	flyback = stride - width;

		xcount = width >> 4; 
		__asm {
			push eax 
			push ebx 
			push edi
			push esi
			mov eax, src
			mov ebx, dstH
			movq mm7, mm_7f7f7f7f7f7f7f7f;
			mov edi, height
		horiz_round0_loop_y:
			mov esi, xcount
		horiz_round0_loop_x:
			movq mm0, [eax]
			movq mm4, [eax+8]
			movq mm1, [eax+1]
			movq mm5, [eax+9]
			movq mm2, mm0
			pxor mm0, mm1
			por mm1, mm2
			psrlq mm0, 1
			add eax, 16
			pand mm0, mm7
			psubb mm1, mm0
			MOVNTQ [ebx], mm1
			movq mm6, mm4
			pxor mm4, mm5
			por mm5, mm6
			psrlq mm4, 1
			pand mm4, mm7
			psubb mm5, mm4
			MOVNTQ [ebx+8], mm5
			add ebx, 16
			dec esi
			jne horiz_round0_loop_x
//			add eax, flyback
//			add ebx, flyback
			dec edi
			jne horiz_round0_loop_y
			pop esi
			pop edi
			pop ebx
			pop eax
			emms
		}
}
static __inline void interpolate_halfpel_v_noround(
	uint8_t *src, 
	uint8_t *dstV, 
	int width,  /* width % 16 == 0 */
	int height)
{
	int y, xcount, flyback;
	
//	flyback = stride - width;

		xcount = width >> 4; 
		__asm {
			push eax 
			push ebx 
			push ecx 
			push edi
			push esi
			mov eax, src
			mov ecx, src
			add ecx, width
			mov ebx, dstV
			movq mm7, mm_7f7f7f7f7f7f7f7f;
			mov edi, height
		vert_round0_loop_y:
			mov esi, xcount
		vert_round0_loop_x:
			movq mm0, [eax]
			movq mm4, [eax+8]
			movq mm1, [ecx]
			movq mm5, [ecx+8]
			movq mm2, mm0
			pxor mm0, mm1
			por mm1, mm2
			psrlq mm0, 1
			add eax, 16
			pand mm0, mm7
			psubb mm1, mm0
			MOVNTQ [ebx], mm1
			movq mm6, mm4
			pxor mm4, mm5
			por mm5, mm6
			psrlq mm4, 1
			add ecx, 16
			pand mm4, mm7
			psubb mm5, mm4
			MOVNTQ [ebx+8], mm5
			add ebx, 16
			dec esi
			jne vert_round0_loop_x
//			add eax, flyback
//			add ebx, flyback
//			add ecx, flyback
			dec edi
			jne vert_round0_loop_y
			pop esi
			pop edi
			pop ecx
			pop ebx
			pop eax
			emms
		}
}

static __inline void interpolate_halfpel_hv_noround(
	uint8_t *src, 
	uint8_t *dstHV, 
	int width,  /* width % 16 == 0 */
	int height)
{
}

void interpolate_halfpel_h(
	uint8_t *src, 
	uint8_t *dstH, 
	int width,  /* width % 16 == 0 */
	int height,
	int rounding)
{
    int i;
    if(!rounding)
    {
	interpolate_halfpel_h_noround(src, dstH, width, height);
	return;
    }
    for(i=0; i<width*height-1; i++)
    {
        *dstH++=((uint32_t)src[0]+(uint32_t)src[1]+1-rounding)>>1;
        src++;
    }

}

void interpolate_halfpel_v(
	uint8_t *src, 
	uint8_t *dstV, 
	int width,  /* width % 16 == 0 */
	int height,
	int rounding)
{
    int i;
    if(!rounding)
    {
	interpolate_halfpel_v_noround(src, dstV, width, height);
	return;
    }
    for(i=0; i<width*(height-1); i++)
    {
        *dstV++=((uint32_t)src[0]+(uint32_t)src[width]+1-rounding)>>1;
        src++;
    }

}

void interpolate_halfpel_hv(
	uint8_t *src, 
	uint8_t *dstHV, 
	int width,  /* width % 16 == 0 */
	int height,
	int rounding)
{
	int y, xcount, flyback;
	
//	flyback = stride - width;

		xcount = width >> 4; 
		__asm {
			push eax 
			push ebx 
			push ecx 
			push edi
			push esi
			mov eax, rounding
			test eax, eax
			jnz has_rounding
			movq mm6, mm_0002000200020002
			jmp cont
		has_rounding:
			movq mm6, mm_0001000100010001
		cont:
			mov eax, src
			mov ecx, src
			add ecx, width
			mov ebx, dstHV
			pxor mm7, mm7
			mov edi, height
		horizvert_round0_loop_y:
			mov esi, xcount
		horizvert_round0_loop_x:
		    	movq mm0, [eax]
				movq mm1, mm0
				punpcklbw mm0, mm7
				movq mm2, [eax+1]
				punpckhbw mm1, mm7
				movq mm3, mm2
				punpcklbw mm2, mm7
				paddw mm0, mm6
				punpckhbw mm3, mm7
				paddw mm1, mm6
				movq mm4, [ecx]
				paddw mm0, mm2
				movq mm5, mm4
				punpcklbw mm4, mm7
				paddw mm1, mm3
				punpckhbw mm5, mm7
				paddw mm0, mm4
				paddw mm1, mm5
				movq mm4, [ecx+1]
				movq mm5, mm4
				punpcklbw mm4, mm7
//			prefetchnta [ecx+32]
				punpckhbw mm5, mm7
				paddw mm4, mm0
				paddw mm5, mm1
				psrlw mm4, 2
				movq mm0, [eax+8]
				psrlw mm5, 2
				movq mm1, mm0
				punpcklbw mm0, mm7
				movq mm2, [eax+9]
				punpckhbw mm1, mm7
				movq mm3, mm2
				packuswb mm4, mm5
				paddw mm0, mm6
				punpcklbw mm2, mm7
			add eax, 16
				punpckhbw mm3, mm7
				MOVNTQ [ebx], mm4
				paddw mm1, mm6
				movq mm4, [ecx+8]
				paddw mm0, mm2
				movq mm5, mm4
				punpcklbw mm4, mm7
				paddw mm1, mm3
				punpckhbw mm5, mm7
				paddw mm0, mm4
				paddw mm1, mm5
				movq mm4, [ecx+9]
				movq mm5, mm4
				punpcklbw mm4, mm7
			add ecx, 16
				punpckhbw mm5, mm7
				paddw mm4, mm0
				paddw mm5, mm1
				psrlw mm4, 2
				psrlw mm5, 2
				packuswb mm4, mm5
				MOVNTQ [ebx+8], mm4
			add ebx, 16
			dec esi
			jne horizvert_round0_loop_x
//			add eax, flyback
//			add ebx, flyback
//			add ecx, flyback
			dec edi
			jne horizvert_round0_loop_y
			pop esi
			pop edi
			pop ecx
			pop ebx
			pop eax
			emms
		}
}
