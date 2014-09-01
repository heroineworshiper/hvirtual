#include "mbtransquant/transferidct.h"



void EncTransferIDCT_add(uint8_t* pDest, int16_t* pSrc, int stride)

{

//not scheduled

    __asm__ __volatile__

    (

    "movl $8, %%edi\n"

    "pxor %%mm2, %%mm2\n"

"1:\n"

    "movq (%%edx), %%mm0\n"

    "movq 8(%%edx), %%mm1\n"

    "movq (%%ecx), %%mm3\n"

    "movq %%mm3, %%mm4\n"

    "punpcklbw %%mm2, %%mm3\n"

    "punpckhbw %%mm2, %%mm4\n"

    "paddsw %%mm3, %%mm0\n"

    "paddsw %%mm4, %%mm1\n"

    "packuswb %%mm1, %%mm0\n"

    "movq %%mm0, (%%ecx)\n"

    "addl $16, %%edx\n"

    "addl %%eax, %%ecx\n"

    "decl %%edi\n"

    "jnz 1b\n"

    "emms\n"

    :

    : "c" (pDest), "d"(pSrc), "a"(stride)

    : "edi"

    );

}



void EncTransferIDCT_copy(uint8_t* pDest, int16_t* pSrc, int stride)

{

    int x, y;

    __asm__ __volatile__

    (

    "movq (%%edx), %%mm0\n"

    "packuswb 8(%%edx), %%mm0\n"

    "movq 16(%%edx), %%mm1\n"

    "packuswb 24(%%edx), %%mm1\n"

    "movq 32(%%edx), %%mm2\n"

    "packuswb 40(%%edx), %%mm2\n"

    "movq 48(%%edx), %%mm3\n"

    "packuswb 56(%%edx), %%mm3\n"



    "movq %%mm0, (%%ecx)\n"

    "addl %%eax, %%ecx\n"

    "movq %%mm1, (%%ecx)\n"

    "addl %%eax, %%ecx\n"

    "movq %%mm2, (%%ecx)\n"

    "addl %%eax, %%ecx\n"

    "movq %%mm3, (%%ecx)\n"

    "addl %%eax, %%ecx\n"

    

    "movq 64(%%edx), %%mm0\n"

    "addl $64, %%edx\n"

    "packuswb 8(%%edx), %%mm0\n"

    "movq 16(%%edx), %%mm1\n"

    "packuswb 24(%%edx), %%mm1\n"

    "movq 32(%%edx), %%mm2\n"

    "packuswb 40(%%edx), %%mm2\n"

    "movq 48(%%edx), %%mm3\n"

    "packuswb 56(%%edx), %%mm3\n"

    

    "movq %%mm0, (%%ecx)\n"

    "addl %%eax, %%ecx\n"

    "movq %%mm1, (%%ecx)\n"

    "addl %%eax, %%ecx\n"

    "movq %%mm2, (%%ecx)\n"

    "addl %%eax, %%ecx\n"

    "movq %%mm3, (%%ecx)\n"

    

    "emms\n"

    :

    : "c" (pDest), "d"(pSrc), "a"(stride)

//    : "edi"

    );


	/*

    for(y=0; y<8; y++)

        for(x=0; x<8; x++)

        {

    	    int16_t tmp=pSrc[x+y*8];

	    if(tmp<0) tmp=0;

	    if(tmp>255) tmp=255;

	    pDest[x+y*stride]=(uint8_t)tmp;

        }


*/

}

/*

void EncTransferFDCT_sub(uint8_t* pSrc1, uint8_t* pSrc2, 

    int16_t* pDest, int stride1, int stride2)

{

    __asm__ __volatile__

    (

    "movl %4, %%edi\n"

    "movl %2, %%esi\n"

    "pushl %%ebx\n"

    "movl %%edi, %%ebx\n"

    "movl $8, %%edi\n"

    "pxor %%mm1, %%mm1\n"

"1:\n"

    "movq (%%edx), %%mm0\n"

    "movq %%mm0, %%mm2\n"

    "punpcklbw %%mm1, %%mm2\n"

    "punpckhbw %%mm1, %%mm0\n"



    "movq (%%esi), %%mm3\n"

    "movq %%mm3, %%mm4\n"

    "punpcklbw %%mm1, %%mm4\n"

    "punpckhbw %%mm1, %%mm3\n"



    "psubsw %%mm3, %%mm0\n"

    "psubsw %%mm4, %%mm2\n"

    

    "movq %%mm2, (%%ecx)\n"

    "movq %%mm0, 8(%%ecx)\n"

    "addl $16, %%ecx\n"

    "addl %%eax, %%edx\n"

    "addl %%ebx, %%esi\n"

    "decl %%edi\n"

    "jnz 1b\n"

    "popl %%ebx\n"

    "emms\n"

    :

    : "c" (pDest), "d"(pSrc1), "g" (pSrc2), "a"(stride1), "g" (stride2)

    : "esi", "edi"

    );

}

*/

void EncTransferFDCT_sub(const uint8_t* pSrc, uint8_t* pSrc2,  

    int16_t* pDest, int stride)

{

    int i, j;

    __asm__ __volatile__

    (

    "movl %3, %%esi\n"

    "movl $8, %%edi\n"

    "pxor %%mm1, %%mm1\n"

"1:\n"

    "movq (%%edx), %%mm0\n"

    "movq %%mm0, %%mm2\n"

    "punpcklbw %%mm1, %%mm2\n"

    "punpckhbw %%mm1, %%mm0\n"



    "movq (%%esi), %%mm3\n"

    "movq %%mm3, %%mm4\n"

    "movq %%mm3, %%mm5\n"

    "punpcklbw %%mm1, %%mm4\n"

    "punpckhbw %%mm1, %%mm3\n"



    "psubsw %%mm3, %%mm0\n"

    "psubsw %%mm4, %%mm2\n"

    

    "movq %%mm2, (%%ecx)\n"

    "movq %%mm0, 8(%%ecx)\n"

    "movq %%mm5, (%%edx)\n"

    "addl $16, %%ecx\n"

    "addl %%eax, %%edx\n"

    "addl %%eax, %%esi\n"

    "decl %%edi\n"

    "jnz 1b\n"

    "emms\n"

    :

    : "d"(pSrc2), "c" (pDest), "a"(stride), "g" (pSrc)

    : "esi", "edi"

    );

    return;



    for(i=0; i<8; i++)

    {

	for(j=0; j<8; j++)

	{

	    uint8_t tmp2=pSrc[j];

	    uint8_t tmp=pSrc2[j];

	    pSrc2[j]=tmp2;

	    pDest[j]=(int16_t)tmp-(int16_t)tmp2;

	}	

	pSrc2+=stride;

	pSrc+=stride;

	pDest+=8;	

    }

}



void EncTransferFDCT_copy(uint8_t* pSrc, int16_t* pDest, int stride)

{

    __asm__ __volatile__

    (

    "movl $8, %%edi\n"

    "pxor %%mm1, %%mm1\n"

"1:\n"

    "movq (%%edx), %%mm0\n"

    "movq %%mm0, %%mm2\n"

    "punpcklbw %%mm1, %%mm2\n"

    "movq %%mm2, (%%ecx)\n"

    "punpckhbw %%mm1, %%mm0\n"

    "movq %%mm0, 8(%%ecx)\n"

    "addl $16, %%ecx\n"

    "addl %%eax, %%edx\n"

    "decl %%edi\n"

    "jnz 1b\n"

    "emms\n"

    :

    : "c" (pDest), "d"(pSrc), "a"(stride)

    : "edi"

    );

}

