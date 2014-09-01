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
 *  mad.c, utility functions that calculate MADs and SADs.
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Eugene Kuznetsov
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

#include "mbmotionestcomp/mad.h"
#define ABS(X) (((X)>0)?(X):-(X))
float MAD_Image(const Image* pIm, const Image* pImage)
{
    int x, y;
    int32_t sum=0;
    int iStride=pImage->iEdgedWidth;
    for(y=0; y<pImage->iHeight; y++)
	for(x=0; x<pImage->iWidth; x++)
	    sum+=ABS((int32_t)pImage->pY[x+y*iStride]-(int32_t)pIm->pY[x+y*iStride]);
    iStride/=2;
    for(y=0; y<pImage->iHeight/2; y++)
	for(x=0; x<pImage->iWidth/2; x++)
	    sum+=ABS((int32_t)pImage->pU[x+y*iStride]-(int32_t)pIm->pU[x+y*iStride]);
    for(y=0; y<pImage->iHeight/2; y++)
	for(x=0; x<pImage->iWidth/2; x++)
	    sum+=ABS((int32_t)pImage->pV[x+y*iStride]-(int32_t)pIm->pV[x+y*iStride]);
    return ((float)sum)/(pImage->iWidth*pImage->iHeight*3/2);
}

// x & y in blocks ( 8 pixel units )
// dx & dy in pixels
static const int64_t mm_FFFFFFFFFFFFFFFF=0xFFFFFFFFFFFFFFFFLL;

#define SAD_INIT 		\
	"movl %1, %%ecx\n" 	\
	"movl %2, %%edx\n" 	\
	"movq mm_FFFFFFFFFFFFFFFF, %%mm7\n" \
	"pxor %%mm0, %%mm0\n" 	\
	"pxor %%mm1, %%mm1\n"

#define SAD_ONE_STEP(X) 	\
	"movq " #X "(%%ecx), %%mm2\n" \
	"movq " #X "(%%edx), %%mm3\n" \
				    \
	"movq %%mm2, %%mm4\n"	\
	"movq %%mm3, %%mm5\n"	\
	"punpcklbw %%mm0, %%mm2\n" \
	"punpckhbw %%mm0, %%mm4\n" \
	"punpcklbw %%mm0, %%mm3\n" \
	"punpckhbw %%mm0, %%mm5\n" \
				\
	"psubw %%mm2, %%mm3\n" 	\
	"psubw %%mm4, %%mm5\n" 	\
				\
	"movq %%mm3, %%mm2\n" 	\
	"movq %%mm5, %%mm4\n" 	\
	"pcmpgtw %%mm0, %%mm2\n"\
	"pcmpgtw %%mm0, %%mm4\n"\
	"pxor %%mm7, %%mm2\n" 	\
	"pxor %%mm7, %%mm4\n"	\
	"pxor %%mm2, %%mm3\n" 	\
	"pxor %%mm4, %%mm5\n"	\
	"psubw %%mm2, %%mm3\n"	\
	"psubw %%mm4, %%mm5\n"	\
	"paddusw %%mm3, %%mm1\n"\
	"paddusw %%mm5, %%mm1\n"	

#define SAD_PACK 		\
	"movq %%mm1, %%mm2\n" 	\
	"psrlq $32, %%mm1\n" 	\
	"paddusw %%mm2, %%mm1\n"\
	"movq %%mm1, %%mm2\n" 	\
	"psrlq $16, %%mm1\n" 	\
	"paddusw %%mm2, %%mm1\n"\
	"movd %%mm1, %%ecx\n" 	\
	"andl $0xFFFF, %%ecx\n"
	
int32_t SAD_Block(const Image* pIm, const Image* pImage,
     int x, int y,
     int dx, int dy, 
     int sad_opt,
     int component)
{
    int32_t sum=0;
    const uint8_t *pRef;
    const uint8_t *pCur;
    int iEdgedWidth=pImage->iEdgedWidth;
    switch(component)
    {
    case 0:
	pRef=pIm->pY+x*8+y*8*pImage->iEdgedWidth;
	pCur=pImage->pY+(x*8+dx)+(y*8+dy)*pImage->iEdgedWidth;
	break;
    case 1:
	pRef=pIm->pU+x*8+y*8*pImage->iEdgedWidth/2;
	pCur=pImage->pU+(x*8+dx)+(y*8+dy)*pImage->iEdgedWidth/2;
	break;
    case 2:
    default:
	pRef=pIm->pV+x*8+y*8*pImage->iEdgedWidth/2;
	pCur=pImage->pV+(x*8+dx)+(y*8+dy)*pImage->iEdgedWidth/2;
	break;
    }
    if(component)
	iEdgedWidth/=2;

    __asm__ __volatile__
    (
	
    SAD_INIT
    "movl $8, %%edi\n"
    
    "1:\n"	
    SAD_ONE_STEP(0)
    "addl %3, %%ecx\n"
    "addl %3, %%edx\n"
    "decl %%edi\n"
    "jnz 1b\n"

    SAD_PACK

    : "=c" (sum)
    : "m" (pRef), "m" (pCur), "m"(iEdgedWidth)
    : "eax", "edx", "edi"
    );
    return sum;
}

int32_t SAD_Macroblock(const Image* pIm, 
    const Image* pImageN, const Image* pImageH, const Image* pImageV, const Image* pImageHV,
    int x, int y, int dx, int dy, int sad_opt, int iQuality)
{
    const Image* pImage;
    int32_t sum=0;
    const uint8_t *pRef;
    const uint8_t *pCur;
    int iEdgedWidth=pImageN->iEdgedWidth;
    switch(((dx%2)?2:0)+((dy%2)?1:0))
    {
    case 0:
	pImage=pImageN;
	break;
    case 1:
	pImage=pImageV;
	dy--;
	break;
    case 2:
	pImage=pImageH;
	dx--;
	break;
    case 3:
    default:
	pImage=pImageHV;
	dx--;
	dy--;
	break;
    }
    dx/=2;
    dy/=2;

    pRef=pIm->pY+x*16+y*16*iEdgedWidth;
    pCur=pImage->pY+(x*16+dx)+(y*16+dy)*iEdgedWidth;

    switch(iQuality)
    {
    case 1:
	__asm__ __volatile__
	(
	SAD_INIT
	"movl $4, %%edi\n"
	
	"1:\n"
	SAD_ONE_STEP(0)
	SAD_ONE_STEP(8)		
	"addl %3, %%ecx\n"
	"addl %3, %%edx\n"
	"decl %%edi\n"
	"jnz 1b\n"

	SAD_PACK
	
	: "=c" (sum)
	: "g" (pRef), "g" (pCur), "m"(4*iEdgedWidth)
	: "eax", "edx", "edi"
	);
        return sum*4;
    case 2:
    	__asm__ __volatile__
	(
	SAD_INIT
	"movl $8, %%edi\n"
	
	"1:\n"
	SAD_ONE_STEP(0)
	SAD_ONE_STEP(8)		
	"addl %3, %%ecx\n"
	"addl %3, %%edx\n"
	"decl %%edi\n"
	"jnz 1b\n"

	SAD_PACK
	
	: "=c" (sum)
	: "g" (pRef), "g" (pCur), "m"(2*iEdgedWidth)
	: "eax", "edx", "edi"
	);
        return sum*2;
    default:
	__asm__ __volatile__
	(
	SAD_INIT
	"movl $16, %%edi\n"
	
	"1:\n"
	SAD_ONE_STEP(0)
	SAD_ONE_STEP(8)		
	"addl %3, %%ecx\n"
	"addl %3, %%edx\n"
	"decl %%edi\n"
	"jnz 1b\n"

	SAD_PACK
	
	: "=c" (sum)
	: "m" (pRef), "m" (pCur), "m"(iEdgedWidth)
	: "eax", "edx", "edi"
	);
        return sum;
    }
}

int32_t SAD_Deviation_MB(const Image* pIm, int x, int y)
{
    int32_t sum=0, avg=0;
    const uint8_t *pRef;
    int i, j;

    pRef=pIm->pY+x*16+y*16*pIm->iEdgedWidth;
    for(i=0; i<16; i++)
    {
	for(j=0; j<16; j++)
	    sum+=(int32_t)pRef[j];
	pRef+=pIm->iEdgedWidth;
    }
    sum/=256;
    pRef=pIm->pY+x*16+y*16*pIm->iEdgedWidth;
    for(i=0; i<16; i++)
    {
	for(j=0; j<16; j++)
	    avg+=ABS((int32_t)pRef[j]-sum);
	pRef+=pIm->iEdgedWidth;
    }
    return avg;
}
