#include "../copyblock.h"

void EncCopyBlock(uint8_t* pSrc, int16_t* pDest, int stride)
{
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
}
