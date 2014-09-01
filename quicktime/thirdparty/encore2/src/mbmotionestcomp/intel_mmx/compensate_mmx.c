#include "../compensate.h"

void Compensate(const uint8_t* pSrc, uint8_t* pSrc2,  
    int16_t* pDest, int stride)
{
	__asm
	{
		mov edx, pSrc2
	 	mov	ecx, pDest
		mov eax, stride
		mov esi, pSrc
		mov edi, 8
		pxor mm1, mm1
p2:
		movq mm0, [edx]
		movq mm2, mm0
		punpcklbw mm2, mm1
		punpckhbw mm0, mm1
		movq mm3, [esi]
		movq mm4, mm3
		movq mm5, mm3
		punpcklbw mm4, mm1
		punpckhbw mm3, mm1

		psubsw mm0, mm3
		psubsw mm2, mm4

		movq [ecx], mm2
		movq [ecx+8], mm0
		movq [edx], mm5

		add ecx, 16
		add edx, eax
		add esi, eax
		dec edi
		jnz p2
		emms
	}
}
