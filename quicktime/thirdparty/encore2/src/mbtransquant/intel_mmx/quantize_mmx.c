/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	mmx quantization/dequantization
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *	02.11.2001	created <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include "../quantize.h"

/* subtract by Q/2 table */

#define ZSUB(X)		((X) / 2)
#define MSUB(X)		ZSUB((X)),ZSUB((X)),ZSUB((X)),ZSUB((X))



static const int16_t mmx_sub[32*4] =
{
	MSUB(0),	MSUB(1),	MSUB(2),	MSUB(3),
	MSUB(4),	MSUB(5),	MSUB(6),	MSUB(7),
	MSUB(8),	MSUB(9),	MSUB(10),	MSUB(11),
	MSUB(12),	MSUB(13),	MSUB(14),	MSUB(15),
	MSUB(16),	MSUB(17),	MSUB(18),	MSUB(19),
	MSUB(20),	MSUB(21),	MSUB(22),	MSUB(23),
	MSUB(24),	MSUB(25),	MSUB(26),	MSUB(27),
	MSUB(28),	MSUB(29),	MSUB(30),	MSUB(31)
};



/* divide by 2Q table 
  use a shift of 16 to take full advantage of _pmulhw_
  for q=1, _pmulhw_ will overflow so it is treated seperately
  (3dnow2 provides _pmulhuw_ which wont cause overflow)
*/



#define ZDIV(X)		((1L << 16) / ((X)*2) + 1)
#define MDIV(X)		ZDIV((X)),ZDIV((X)),ZDIV((X)),ZDIV((X))
#define MDIV_0		0,0,0,0

static const uint16_t mmx_div[32*4] =
{
	MDIV_0,		MDIV(1),	MDIV(2),	MDIV(3),
	MDIV(4),	MDIV(5),	MDIV(6),	MDIV(7),
	MDIV(8),	MDIV(9),	MDIV(10),	MDIV(11),
	MDIV(12),	MDIV(13),	MDIV(14),	MDIV(15),
	MDIV(16),	MDIV(17),	MDIV(18),	MDIV(19),
	MDIV(20),	MDIV(21),	MDIV(22),	MDIV(23),
	MDIV(24),	MDIV(25),	MDIV(26),	MDIV(27),
	MDIV(28),	MDIV(29),	MDIV(30),	MDIV(31)
}; 


/* add by (odd(Q) ? Q : Q - 1) table */

#define ZADD(X)		((X) & 1 ? (X) : (X) - 1)
#define MADD(X)		ZADD((X)),ZADD((X)),ZADD((X)),ZADD((X))


static const int16_t mmx_add[32*4] =
{
	MADD(0),	MADD(1),	MADD(2),	MADD(3),
	MADD(4),	MADD(5),	MADD(6),	MADD(7),
	MADD(8),	MADD(9),	MADD(10),	MADD(11),
	MADD(12),	MADD(13),	MADD(14),	MADD(15),
	MADD(16),	MADD(17),	MADD(18),	MADD(19),
	MADD(20),	MADD(21),	MADD(22),	MADD(23),
	MADD(24),	MADD(25),	MADD(26),	MADD(27),
	MADD(28),	MADD(29),	MADD(30),	MADD(31)
};


/* multiple by 2Q table */
#define ZMUL(X)		((X)*2)
#define MUL(X)		ZMUL((X)),ZMUL((X)),ZMUL((X)),ZMUL((X))


static const int16_t mmx_mul[32*4] =
{
	MUL(0),		MUL(1),		MUL(2),		MUL(3),
	MUL(4),		MUL(5),		MUL(6),		MUL(7),
	MUL(8),		MUL(9),		MUL(10),	MUL(11),
	MUL(12),	MUL(13),	MUL(14),	MUL(15),
	MUL(16),	MUL(17),	MUL(18),	MUL(19),
	MUL(20),	MUL(21),	MUL(22),	MUL(23),
	MUL(24),	MUL(25),	MUL(26),	MUL(27),
	MUL(28),	MUL(29),	MUL(30),	MUL(31)
};


void enc_quant_intra_mmx(int16_t * coeff, const int16_t * data, const uint32_t quant, const uint32_t dcscalar)
{
	_asm {
		mov		esi, data
		mov		edi, coeff

		mov		eax, quant
		cmp		eax, 1
		mov		ecx, 8
		jz		q1loop
	
		movq	mm7, [mmx_div + eax * 8]

xloop:
		movq	mm0, [esi]		// mm0 = [1st]
		movq	mm3, [esi + 8]	// 
		pxor	mm1, mm1		// mm1 = 0
		pxor	mm4, mm4		//
		pcmpgtw	mm1, mm0		// mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		// 
		pxor	mm0, mm1		// mm0 = |mm0|
		pxor	mm3, mm4		// 
		psubw	mm0, mm1		// displace
		psubw	mm3, mm4		// 
		pmulhw	mm0, mm7		// mm0 = (mm0 / 2Q) >> 16
		pmulhw	mm3, mm7		// 
		pxor	mm0, mm1		// mm0 *= sign(mm0)
		pxor	mm3, mm4
		psubw	mm0, mm1		// undisplace
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3

		add		esi, 16
		add		edi, 16
		dec		ecx
		jnz		xloop 

		jmp short done


q1loop:
		movq	mm0, [esi]		// mm0 = [1st]
		movq	mm3, [esi + 8]	// 
		pxor	mm1, mm1		// mm1 = 0
		pxor	mm4, mm4		//
		pcmpgtw	mm1, mm0		// mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		// 
		pxor	mm0, mm1		// mm0 = |mm0|
		pxor	mm3, mm4		// 
		psubw	mm0, mm1		// displace
		psubw	mm3, mm4		// 
		psrlw	mm0, 1			// mm0 >>= 1   (/2)
		psrlw	mm3, 1			//
		pxor	mm0, mm1		// mm0 *= sign(mm0)
		pxor	mm3, mm4
		psubw	mm0, mm1		// undisplace
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3

		add		esi, 16
		add		edi, 16
		dec		ecx
		jnz		q1loop

done:

	}
	*coeff = (*data + ((int32_t)dcscalar >> 1)) / (int32_t)dcscalar;
}



uint32_t enc_quant_inter_mmx(int16_t * coeff, const int16_t * data, const uint32_t quant)
{
	_asm {
		mov		esi, data
		mov		edi, coeff
		mov		eax, quant

		pxor	mm5, mm5					// present
		movq	mm6, [mmx_sub + eax * 8]
					
		cmp		eax, 1
		mov		ecx, 8
		jz		q1loop

		movq	mm7, [mmx_div + eax * 8]	// divider

xloop:
		movq	mm0, [esi]		// mm0 = [1st]
		movq	mm3, [esi + 8]	// 
		pxor	mm1, mm1		// mm1 = 0
		pxor	mm4, mm4		//
		pcmpgtw	mm1, mm0		// mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		// 
		pxor	mm0, mm1		// mm0 = |mm0|
		pxor	mm3, mm4		// 
		psubw	mm0, mm1		// displace
		psubw	mm3, mm4		// 
		psubusw	mm0, mm6		// mm0 -= sub (unsigned, dont go < 0)
		psubusw	mm3, mm6		//
		pmulhw	mm0, mm7		// mm0 = (mm0 / 2Q) >> 16
		pmulhw	mm3, mm7		// 
		pxor	mm0, mm1		// mm0 *= sign(mm0)
		pxor	mm3, mm4
		psubw	mm0, mm1		// undisplace
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3
		por		mm5, mm0		// set present
		por		mm5, mm3

		add		esi, 16
		add		edi, 16
		dec		ecx
		jnz		xloop 

		jmp short done


q1loop:
		movq	mm0, [esi]		// mm0 = [1st]
		movq	mm3, [esi + 8]	// 
		pxor	mm1, mm1		// mm1 = 0
		pxor	mm4, mm4		//
		pcmpgtw	mm1, mm0		// mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		// 
		pxor	mm0, mm1		// mm0 = |mm0|
		pxor	mm3, mm4		// 
		psubw	mm0, mm1		// displace
		psubw	mm3, mm4		// 
		psubusw	mm0, mm6		// mm0 -= sub (unsigned, dont go < 0)
		psubusw	mm3, mm6		//
		psrlw	mm0, 1			// mm0 >>= 1   (/2)
		psrlw	mm3, 1			//
		pxor	mm0, mm1		// mm0 *= sign(mm0)
		pxor	mm3, mm4
		psubw	mm0, mm1		// undisplace
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3
		por		mm5, mm0		// set present
		por		mm5, mm3

		add		esi, 16
		add		edi, 16
		dec		ecx
		jnz		q1loop

done:	
		movq	mm0, mm5		// pack present into dword
		psrlq	mm5, 32
		por		mm0, mm5

		movd	eax, mm0		// return present

	}
}




void enc_dequant_intra_mmx(int16_t *data, const int16_t *coeff, const uint32_t quant, const uint32_t dcscalar)
{
	dequant_inter_mmx(data, coeff, quant);
	*data = *coeff * dcscalar;
}



void enc_dequant_inter_mmx(int16_t * data, const int16_t * coeff, const uint32_t quant)
{
	_asm {
		mov		esi, coeff
		mov		edi, data
		mov		eax, quant

		movq	mm6, [mmx_add + eax * 8]
		movq	mm7, [mmx_mul + eax * 8]
		
		mov		ecx, 8
xloop:
		movq	mm0, [esi]		// mm0 = [1st]
		movq	mm3, [esi + 8]	// 
		pxor	mm1, mm1		// mm1 = 0
		pxor	mm4, mm4		//
		pcmpgtw	mm1, mm0		// mm1 = (0 > mm0)
		pcmpgtw	mm4, mm3		// 
		pxor	mm2, mm2		// mm2 = 0
		pxor	mm5, mm5		//
		pcmpeqw	mm2, mm0		// mm2 = (0 == mm0)
		pcmpeqw	mm5, mm3		// 
		pandn   mm2, mm6		// mm2 = (iszero ? 0 : add)
		pandn   mm5, mm6
		pxor	mm0, mm1		// mm0 = |mm0|
		pxor	mm3, mm4		// 
		psubw	mm0, mm1		// displace
		psubw	mm3, mm4		// 
		pmullw	mm0, mm7		// mm0 *= 2Q
		pmullw	mm3, mm7		// 
		paddw	mm0, mm2		// mm0 += mm2 (add)
		paddw	mm3, mm5
		pxor	mm0, mm1		// mm0 *= sign(mm0)
		pxor	mm3, mm4
		psubw	mm0, mm1		// undisplace
		psubw	mm3, mm4
		movq	[edi], mm0
		movq	[edi + 8], mm3

		add		esi, 16
		add		edi, 16
		dec		ecx

		jnz		xloop 

	}
}
