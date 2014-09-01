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

#include "../mad.h"
#define ABS(X) (((X)>0)?(X):-(X))
float MAD_Image(const Vop* pIm, const Vop* pVop)
{
    int x, y;
    int32_t sum=0;
//    int iStride=pVop->iWidth;
    int iStride2=pVop->iEdgedWidth;
    for(y=0; y<pVop->iHeight; y++)
	for(x=0; x<pVop->iWidth; x++)
	    sum+=ABS((int32_t)pVop->pY[x+y*iStride2]-(int32_t)pIm->pY[x+y*iStride2]);
//    iStride/=2;
    iStride2/=2;	    
    for(y=0; y<pVop->iHeight/2; y++)
	for(x=0; x<pVop->iWidth/2; x++)
	    sum+=ABS((int32_t)pVop->pU[x+y*iStride2]-(int32_t)pIm->pU[x+y*iStride2]);
    for(y=0; y<pVop->iHeight/2; y++)
	for(x=0; x<pVop->iWidth/2; x++)
	    sum+=ABS((int32_t)pVop->pV[x+y*iStride2]-(int32_t)pIm->pV[x+y*iStride2]);
    return ((float)sum)/(pVop->iWidth*pVop->iHeight*3/2);
}

// x & y in blocks ( 8 pixel units )
// dx & dy in pixels
static const int64_t mm_FFFFFFFFFFFFFFFF=0xFFFFFFFFFFFFFFFFi64;

#define SAD_INIT \
	__asm xor eax, eax \
	__asm movq mm7, mm_FFFFFFFFFFFFFFFF \
	__asm pxor mm0, mm0 \
	__asm pxor mm1, mm1
//	"movl %1, %%ecx\n" 	
//	"movl %2, %%edx\n" 	

#define SAD_ONE_STEP(X) 	\
	__asm movq mm2, [ecx+X]		\
	__asm movq mm3, [edx+X]		\
							\
	__asm movq mm4, mm2 \
	__asm movq mm5, mm3 \
	__asm punpcklbw mm2, mm0	\
	__asm punpckhbw mm4, mm0	\
	__asm punpcklbw mm3, mm0	\
	__asm punpckhbw mm5, mm0	\
							\
	__asm psubw mm3, mm2		\
	__asm psubw mm5, mm4		\
							\
	__asm movq mm2, mm3			\
	__asm movq mm4, mm5			\
	__asm pcmpgtw mm2, mm0		\
	__asm pcmpgtw mm4, mm0		\
	__asm pxor mm2, mm7			\
	__asm pxor mm4, mm7			\
	__asm pxor mm3, mm2			\
	__asm pxor mm5, mm4			\
	__asm psubw mm3, mm2		\
	__asm psubw mm5, mm4		\
	__asm paddusw mm1, mm3		\
	__asm paddusw mm1, mm5		
							
#define SAD_PACK 		\
	__asm movq mm2, mm1		\
	__asm psrlq mm1, 32		\
	__asm paddusw mm1, mm2	\
	__asm movq mm2, mm1		\
	__asm psrlq mm1, 16		\
	__asm paddusw mm1, mm2	\
	__asm movd ecx, mm1		\
	__asm and ecx, 0xFFFF
		
int32_t SAD_Block(const Vop* pIm, const Vop* pVop,
     int x, int y,
     int dx, int dy, 
     int sad_opt,
     int component)
{
    int32_t sum=0;
    int i, j;
    const uint8_t *pRef;
    const uint8_t *pCur;
    int iWidth=pVop->iWidth;
    int iEdgedWidth=pVop->iEdgedWidth;
    switch(component)
    {
    case 0:
	pRef=pIm->pY+x*8+y*8*pVop->iEdgedWidth;
	pCur=pVop->pY+(x*8+dx)+(y*8+dy)*pVop->iEdgedWidth;
	break;
    case 1:
	pRef=pIm->pU+x*8+y*8*pVop->iEdgedWidth/2;
	pCur=pVop->pU+(x*8+dx)+(y*8+dy)*pVop->iEdgedWidth/2;
	break;
    case 2:
    default:
	pRef=pIm->pV+x*8+y*8*pVop->iEdgedWidth/2;
	pCur=pVop->pV+(x*8+dx)+(y*8+dy)*pVop->iEdgedWidth/2;
	break;
    }
    if(component)
    {
	iWidth/=2;
	iEdgedWidth/=2;
    }

    SAD_INIT
	__asm mov edi, 8
	__asm mov ecx, pRef
	__asm mov edx, pCur
    
 p1:
    SAD_ONE_STEP(0)
	__asm add ecx, iEdgedWidth
	__asm add edx, iEdgedWidth
    __asm dec edi
    __asm jnz p1

    SAD_PACK
	__asm mov sum, ecx

    return sum;
}

int32_t SAD_Macroblock(const Vop* pIm, 
    const Vop* pVopN, const Vop* pVopH, const Vop* pVopV, const Vop* pVopHV,
    int x, int y, int dx, int dy, int sad_opt, int iQuality)
{
    const Vop* pVop;
    int32_t sum=0;
    int i, j;
    const uint8_t *pRef;
    const uint8_t *pCur;
    int iWidth=pVopN->iEdgedWidth;
    int iEdgedWidth=pVopN->iEdgedWidth;
    switch(((dx%2)?2:0)+((dy%2)?1:0))
    {
    case 0:
	pVop=pVopN;
	break;
    case 1:
	pVop=pVopV;
	dy--;
	break;
    case 2:
	pVop=pVopH;
	dx--;
	break;
    case 3:
    default:
	pVop=pVopHV;
	dx--;
	dy--;
	break;
    }
    dx/=2;
    dy/=2;

    pRef=pIm->pY+x*16+y*16*iEdgedWidth;
    pCur=pVop->pY+(x*16+dx)+(y*16+dy)*iEdgedWidth;

    switch(iQuality)
    {
	case 1:
	iEdgedWidth*=4;
	iEdgedWidth*=4;
	 SAD_INIT
	__asm mov edi, 4
	__asm mov ecx, pRef
	__asm mov edx, pCur
    
 p4:
    SAD_ONE_STEP(0)
	SAD_ONE_STEP(8)
	__asm add ecx, iEdgedWidth
	__asm add edx, iEdgedWidth
    __asm dec edi
    __asm jnz p4

    SAD_PACK
	__asm mov sum, ecx

     return sum*4;
case 2:
	iEdgedWidth*=2;
	iEdgedWidth*=2;
	 SAD_INIT
	__asm mov edi, 8
	__asm mov ecx, pRef
	__asm mov edx, pCur
    
 p3:
    SAD_ONE_STEP(0)
	SAD_ONE_STEP(8)
	__asm add ecx, iEdgedWidth
	__asm add edx, iEdgedWidth
    __asm dec edi
    __asm jnz p3

    SAD_PACK
	__asm mov sum, ecx

     return sum*2;
default:
	SAD_INIT
	__asm mov ecx, pRef
	__asm mov edx, pCur
	__asm mov edi, 16
    
 p2:
    SAD_ONE_STEP(0)
	SAD_ONE_STEP(8)
	__asm add ecx, iEdgedWidth
	__asm add edx, iEdgedWidth
    __asm dec edi
    __asm jnz p2

    SAD_PACK
	__asm mov sum, ecx

     return sum;
    }
}

int32_t SAD_Deviation_MB(const Vop* pIm, int x, int y)
{
    int32_t sum=0, avg=0;
    const uint8_t *pRef;
    int i, j;
    int width=pIm->iEdgedWidth;

    pRef=pIm->pY+x*16+y*16*width;
    for(i=0; i<16; i++)
    {
	for(j=0; j<16; j++)
	    sum+=(int32_t)pRef[j];
	pRef+=width;
    }
    sum/=256;
    pRef=pIm->pY+x*16+y*16*width;
    for(i=0; i<16; i++)
    {
	for(j=0; j<16; j++)
	    avg+=ABS((int32_t)pRef[j]-sum);
	pRef+=width;
    }
    return avg;
}
