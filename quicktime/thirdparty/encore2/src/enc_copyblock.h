/**************************************************************************
 *
 *  Modifications:
 *
 *	28.10.2001 merged & inlined c/mmx EncCopyBlock
 *
 **************************************************************************/

#ifndef ENC_COPYBLOCK_H_
#define ENC_COPYBLOCK_H_

#include "portab.h"


static void __inline EncCopyBlock(uint8_t* pSrc, int16_t* pDest, int stride)
{

#if defined(WIN32) && defined(_MMX_)

    __asm
	{
		mov ecx, pDest
		mov edx, pSrc
		mov eax, stride
		mov edi, 8
		pxor mm1, mm1
p3:

		movq mm0, [edx]
		movq mm2, mm0
		punpcklbw mm2, mm1
		movq [ecx], mm2
		punpckhbw mm0, mm1
		movq [ecx+8], mm0
		add ecx, 16
		add edx, eax
		dec edi

		jnz p3
		emms
	}
#else if

    int x, y;

    for (y = 0; y < 8; y++)
		for (x = 0; x < 8; x++)
			pDest[x + y * 8] = (int16_t) pSrc[x + y * stride];

#endif
}


#endif /* _COPYBLOCK_H_ */
