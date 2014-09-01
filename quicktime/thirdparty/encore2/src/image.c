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
 *  vop.c, various VOP-level utility functions.
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Eugene Kuznetsov
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/ 


/**************************************************************************
 *
 *  Modifications:
 *
 *  23.11.2001  added CopyImages() (Isibaar)
 *	10.11.2001	support for new c/mmx/3dnow interpolation
 *
 **************************************************************************/

    
#include "enctypes.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "mbmotionestcomp/halfpel.h"

#define PUT_COMPONENT(p,v,i) 	\
    tmp=(unsigned int)(v); 	\
    if(tmp < 0x10000) 		\
	p[i]=tmp>>8; 		\
    else			\
	p[i]=(tmp >> 24) ^ 0xff;

const int iEdgeSize = 32;

int CreateImage(Image * pImage, int width, int height)

{

    assert(pImage);

    pImage->iWidth = width;

    pImage->iHeight = height;

    pImage->iMbWcount = (width + 15) / 16;

    pImage->iMbHcount = (height + 15) / 16;

//    pImage->iEdgedWidth=width+2*iEdgeSize;

//    pImage->iEdgedHeight=height+2*iEdgeSize;

    pImage->iEdgedWidth = pImage->iMbWcount * 16 + 2 * iEdgeSize;

    pImage->iEdgedHeight = pImage->iMbHcount * 16 + 2 * iEdgeSize;



    /** allocate a little more memory, so that MMX routines won't run over

    the buffer **/

    pImage->pY =

	(uint8_t *) malloc(pImage->iEdgedWidth * pImage->iEdgedHeight + 64);

    if (pImage->pY == 0)

	return -1;

    pImage->pY += (iEdgeSize + iEdgeSize * pImage->iEdgedWidth);

    pImage->pU =

	(uint8_t *) malloc(pImage->iEdgedWidth * pImage->iEdgedHeight / 4 + 64);

    if (pImage->pU == 0)

    {

	free(pImage->pY - (iEdgeSize + iEdgeSize * pImage->iEdgedWidth));

	return -1;

    }

    pImage->pU += (iEdgeSize / 2 + iEdgeSize / 2 * pImage->iEdgedWidth / 2);

    pImage->pV =

	(uint8_t *) malloc(pImage->iEdgedWidth * pImage->iEdgedHeight / 4 + 64);

    if (pImage->pV == 0)

    {

	free(pImage->pY - (iEdgeSize + iEdgeSize * pImage->iEdgedWidth));

	free(pImage->pU -

	     (iEdgeSize / 2 + iEdgeSize / 2 * pImage->iEdgedWidth / 2));

	return -1;

    }

    pImage->pV += (iEdgeSize / 2 + iEdgeSize / 2 * pImage->iEdgedWidth / 2);

    pImage->pMBs =

	(Macroblock *) malloc(pImage->iMbWcount * pImage->iMbHcount *



			      sizeof(Macroblock));

    if (pImage->pMBs == 0)

    {

	free(pImage->pY - (iEdgeSize + iEdgeSize * pImage->iEdgedWidth));

	free(pImage->pU -

	     (iEdgeSize / 2 + iEdgeSize / 2 * pImage->iEdgedWidth / 2));

	free(pImage->pV -

	     (iEdgeSize / 2 + iEdgeSize / 2 * pImage->iEdgedWidth / 2));

	return -1;

    }

    return 0;

}



void FreeImage(Image * pImage)

{

    uint8_t *pTmp;



    assert(pImage->pY);

    assert(pImage->pU);

    assert(pImage->pV);

    pImage->pY -= (iEdgeSize + iEdgeSize * (pImage->iEdgedWidth));

    pImage->pU -= (iEdgeSize / 2 + iEdgeSize / 2 * (pImage->iEdgedWidth) / 2);

    pImage->pV -= (iEdgeSize / 2 + iEdgeSize / 2 * (pImage->iEdgedWidth) / 2);

    pTmp = pImage->pY;

    pImage->pY = 0;

    free(pTmp);

    free(pImage->pU);

    free(pImage->pV);

}

void CopyImages(Image *pImage1, Image *pImage2) {
	memcpy(pImage1->pY, pImage2->pY, pImage1->iEdgedWidth * pImage1->iHeight);
	memcpy(pImage1->pU, pImage2->pU, pImage1->iEdgedWidth * pImage1->iHeight / 4);
	memcpy(pImage1->pV, pImage2->pV, pImage1->iEdgedWidth * pImage1->iHeight / 4);
}

void SwapImages(Image * pImage1, Image * pImage2)

{

    uint8_t *tmp;



    assert(pImage1);

    assert(pImage2);

    assert(pImage1->iWidth == pImage2->iWidth);

    assert(pImage1->iHeight == pImage2->iHeight);



    tmp = pImage1->pY;

    pImage1->pY = pImage2->pY;

    pImage2->pY = tmp;

    tmp = pImage1->pU;

    pImage1->pU = pImage2->pU;

    pImage2->pU = tmp;

    tmp = pImage1->pV;

    pImage1->pV = pImage2->pV;

    pImage2->pV = tmp;

}



void SetEdges(Image * pImage)

{

    uint8_t *c_ptr;

    unsigned char *src_ptr;



    int i;

    int c_width;

    int _width, _height;



    assert(pImage);

    c_width = pImage->iEdgedWidth;

    _width = pImage->iWidth;

    _height = pImage->iHeight;

// Y

    c_ptr = pImage->pY - (iEdgeSize + iEdgeSize * c_width);

    src_ptr = pImage->pY;

    for (i = 0; i < iEdgeSize; i++)

    {

	memset(c_ptr, *src_ptr, iEdgeSize);

	memcpy(c_ptr + iEdgeSize, src_ptr, _width);

	memset(c_ptr + c_width - iEdgeSize, src_ptr[_width - 1], iEdgeSize);

	c_ptr += c_width;

    }

    for (i = 0; i < _height; i++)

    {

	memset(c_ptr, *src_ptr, iEdgeSize);

	memset(c_ptr + c_width - iEdgeSize, src_ptr[_width - 1], iEdgeSize);

	c_ptr += c_width;

	src_ptr += c_width;

    }

    src_ptr -= c_width;

    for (i = 0; i < iEdgeSize; i++)

    {

	memset(c_ptr, *src_ptr, iEdgeSize);

	memcpy(c_ptr + iEdgeSize, src_ptr, _width);

	memset(c_ptr + c_width - iEdgeSize, src_ptr[_width - 1], iEdgeSize);

	c_ptr += c_width;

    }



//U

    c_ptr = pImage->pU - (iEdgeSize / 2 + iEdgeSize / 2 * c_width / 2);

    src_ptr = pImage->pU;

    for (i = 0; i < iEdgeSize / 2; i++)

	


    {

	memset(c_ptr, src_ptr[0], iEdgeSize / 2);

	memcpy(c_ptr + iEdgeSize / 2, src_ptr, _width / 2);

	memset(c_ptr + c_width / 2 - iEdgeSize / 2, src_ptr[_width / 2 - 1],

	       iEdgeSize / 2);

	c_ptr += c_width / 2;

    }

    for (i = 0; i < _height / 2; i++)

    {

	memset(c_ptr, src_ptr[0], iEdgeSize / 2);

	memset(c_ptr + c_width / 2 - iEdgeSize / 2, src_ptr[_width / 2 - 1],

	       iEdgeSize / 2);

	c_ptr += c_width / 2;

	src_ptr += c_width / 2;

    }

    src_ptr -= c_width / 2;

    for (i = 0; i < iEdgeSize / 2; i++)

	


    {

	memset(c_ptr, src_ptr[0], iEdgeSize / 2);

	memcpy(c_ptr + iEdgeSize / 2, src_ptr, _width / 2);

	memset(c_ptr + c_width / 2 - iEdgeSize / 2, src_ptr[_width / 2 - 1],

	       iEdgeSize / 2);

	c_ptr += c_width / 2;

    }

// V

    c_ptr = pImage->pV - (iEdgeSize / 2 + iEdgeSize / 2 * c_width / 2);

    src_ptr = pImage->pV;

    for (i = 0; i < iEdgeSize / 2; i++)

	


    {

	memset(c_ptr, src_ptr[0], iEdgeSize / 2);

	memcpy(c_ptr + iEdgeSize / 2, src_ptr, _width / 2);

	memset(c_ptr + c_width / 2 - iEdgeSize / 2, src_ptr[_width / 2 - 1],

	       iEdgeSize / 2);

	c_ptr += c_width / 2;

    }

    for (i = 0; i < _height / 2; i++)

    {

	memset(c_ptr, src_ptr[0], iEdgeSize / 2);

	memset(c_ptr + c_width / 2 - iEdgeSize / 2, src_ptr[_width / 2 - 1],

	       iEdgeSize / 2);

	c_ptr += c_width / 2;

	src_ptr += c_width / 2;

    }

    src_ptr -= c_width / 2;

    for (i = 0; i < iEdgeSize / 2; i++)

	


    {

	memset(c_ptr, src_ptr[0], iEdgeSize / 2);

	memcpy(c_ptr + iEdgeSize / 2, src_ptr, _width / 2);

	memset(c_ptr + c_width / 2 - iEdgeSize / 2, src_ptr[_width / 2 - 1],

	       iEdgeSize / 2);

	c_ptr += c_width / 2;

    }
}



#if (defined(_3DNOW_))

#define interpolate_halfpel_h  interpolate_halfpel_h_3dn
#define interpolate_halfpel_v  interpolate_halfpel_v_3dn
#define interpolate_halfpel_hv  interpolate_halfpel_hv_mmx

#elif defined(_MMX_)

#define interpolate_halfpel_h  interpolate_halfpel_h_mmx
#define interpolate_halfpel_v  interpolate_halfpel_v_mmx
#define interpolate_halfpel_hv  interpolate_halfpel_hv_mmx

#endif


void Interpolate(const Image * pRef, Image * pInterH, Image * pInterV,
		 Image * pInterHV, const int iRounding, int iChromOnly)
{
    uint32_t iSize = iEdgeSize;
    uint32_t offset;

//Y
    if (!iChromOnly)
    {
		offset = iSize * (pRef->iEdgedWidth + 1);

		interpolate_halfpel_h(
			pInterH->pY - offset,
			pRef->pY - offset, 
			pRef->iEdgedWidth, pRef->iEdgedHeight,
			iRounding);

		interpolate_halfpel_v(
			pInterV->pY - offset,
			pRef->pY - offset, 
			pRef->iEdgedWidth, pRef->iEdgedHeight,
			iRounding);

		 interpolate_halfpel_hv(
			pInterHV->pY - offset,
			pRef->pY - offset,
			pRef->iEdgedWidth, pRef->iEdgedHeight,
			iRounding);

    }
// U
    iSize /= 2;
    offset = iSize * (pRef->iEdgedWidth / 2 + 1);

    interpolate_halfpel_h(
		pInterH->pU - offset,
		pRef->pU - offset, 
		pRef->iEdgedWidth / 2, pRef->iEdgedHeight / 2,
		iRounding);

    interpolate_halfpel_v(
		pInterV->pU - offset,
		pRef->pU - offset, 
		pRef->iEdgedWidth / 2, pRef->iEdgedHeight / 2,
		iRounding);

    interpolate_halfpel_hv(
		pInterHV->pU - offset,
		pRef->pU - offset, 
		pRef->iEdgedWidth / 2, pRef->iEdgedHeight / 2,
		iRounding);

// V
    interpolate_halfpel_h(
		pInterH->pV - offset,
		pRef->pV - offset, 
		pRef->iEdgedWidth / 2, pRef->iEdgedHeight / 2,
		iRounding);

    interpolate_halfpel_v(
		pInterV->pV - offset,
		pRef->pV - offset, 
		pRef->iEdgedWidth / 2, pRef->iEdgedHeight / 2,
		iRounding);

     interpolate_halfpel_hv(
		pInterHV->pV - offset,
		pRef->pV - offset, 
		pRef->iEdgedWidth / 2, pRef->iEdgedHeight / 2,
		iRounding);
}



#ifdef LINUX



struct lookuptable

{

    int32_t m_plY[256];

    int32_t m_plRV[256];

    int32_t m_plGV[256];

    int32_t m_plGU[256];

    int32_t m_plBU[256];

};

static struct lookuptable lut;

static void init_yuv2rgb()

{

    int i;



    for (i = 0; i < 256; i++)

    {

	if (i >= 16)

	    if (i > 240)

		lut.m_plY[i] = lut.m_plY[240];

	    else

		lut.m_plY[i] = 298 * (i - 16);

	else

	    lut.m_plY[i] = 0;

	if ((i >= 16) && (i <= 240))

	{

	    lut.m_plRV[i] = 408 * (i - 128);

	    lut.m_plGV[i] = -208 * (i - 128);

	    lut.m_plGU[i] = -100 * (i - 128);

	    lut.m_plBU[i] = 517 * (i - 128);

	}

	else if (i < 16)

	{

	    lut.m_plRV[i] = 408 * (16 - 128);

	    lut.m_plGV[i] = -208 * (16 - 128);

	    lut.m_plGU[i] = -100 * (16 - 128);

	    lut.m_plBU[i] = 517 * (16 - 128);

	}

	else

	{

	    lut.m_plRV[i] = lut.m_plRV[240];

	    lut.m_plGV[i] = lut.m_plGV[240];

	    lut.m_plGU[i] = lut.m_plGU[240];

	    lut.m_plBU[i] = lut.m_plBU[240];

	}

    }

}

static void yuv2rgb_24(uint8_t *puc_y, int stride_y,

		       uint8_t *puc_u, uint8_t *puc_v, int stride_uv,

		       uint8_t *puc_out, int width_y, int height_y,

		       unsigned int _stride_out)

{



    int x, y;

    int stride_diff = 6 * _stride_out - 3 * width_y;



    if (height_y < 0)

    {

	/* 

	   we are flipping our output upside-down */

	height_y = -height_y;

	puc_y += (height_y - 1) * stride_y;

	puc_u += (height_y / 2 - 1) * stride_uv;

	puc_v += (height_y / 2 - 1) * stride_uv;

	stride_y = -stride_y;

	stride_uv = -stride_uv;

    }



    for (y = 0; y < height_y; y += 2)

    {

	uint8_t *pY = puc_y;

	uint8_t *pY1 = puc_y + stride_y;

	uint8_t *pU = puc_u;

	uint8_t *pV = puc_v;

	uint8_t *pOut2 = puc_out + 3 * _stride_out;



	for (x = 0; x < width_y; x += 2)

	{

	    int R, G, B;

	    int Y;

	    unsigned int tmp;



	    R = lut.m_plRV[*pV];

	    G = lut.m_plGV[*pV];

	    pV++;

	    G += lut.m_plGU[*pU];

	    B = lut.m_plBU[*pU];

	    pU++;

	    Y = lut.m_plY[*pY];

	    pY++;

	    PUT_COMPONENT(puc_out, B + Y, 0);

	    PUT_COMPONENT(puc_out, G + Y, 1);

	    PUT_COMPONENT(puc_out, R + Y, 2);

	    Y = lut.m_plY[*pY];

	    pY++;

	    PUT_COMPONENT(puc_out, B + Y, 3);

	    PUT_COMPONENT(puc_out, G + Y, 4);

	    PUT_COMPONENT(puc_out, R + Y, 5);

	    Y = lut.m_plY[*pY1];

	    pY1++;

	    PUT_COMPONENT(pOut2, B + Y, 0);

	    PUT_COMPONENT(pOut2, G + Y, 1);

	    PUT_COMPONENT(pOut2, R + Y, 2);

	    Y = lut.m_plY[*pY1];

	    pY1++;

	    PUT_COMPONENT(pOut2, B + Y, 3);

	    PUT_COMPONENT(pOut2, G + Y, 4);

	    PUT_COMPONENT(pOut2, R + Y, 5);

	    puc_out += 6;

	    pOut2 += 6;

	}



	puc_y += 2 * stride_y;

	puc_u += stride_uv;

	puc_v += stride_uv;

	puc_out += stride_diff;

    }

}

typedef struct 


{

    

int biSize;

     

int biWidth;

     

int biHeight;

     

short biPlanes;

     

short biBitCount;

     

int biCompression;

     

int biSizeImage;

     

int biXPelsPerMeter;

     

int biYPelsPerMeter;

     

int biClrUsed;

     

int biClrImportant;

 

}

BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;






#include <unistd.h>

#include <fcntl.h>

void DumpImage(Image * pImage, const char *filename)

{

    int bs = abs(pImage->iWidth * pImage->iHeight * 3);

    short bfh[7];

    uint8_t *pPix;

    static int needs_init = 1;

    BITMAPINFOHEADER bi;

    int fd;



    

if (needs_init)

    {

	needs_init = 0;

	init_yuv2rgb();

    }

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 00666);

    

if (fd < 0)

	return;

    pPix = malloc(bs);

    yuv2rgb_24(pImage->pY, pImage->iEdgedWidth,

	       pImage->pU, pImage->pV, pImage->iEdgedWidth / 2,

	       pPix, pImage->iWidth, -pImage->iHeight, pImage->iWidth);

    bfh[0] = 'B' + 256 * 'M';

    *(int *) &bfh[1] = bs + 0x36;

    *(int *) &bfh[3] = 0;

    *(int *) &bfh[5] = 0x36;



    bi.biSize = sizeof(bi);

    bi.biWidth = pImage->iWidth;

    bi.biHeight = pImage->iHeight;

    bi.biBitCount = 24;

    bi.biCompression = 0;

    bi.biSizeImage = 0;

    bi.biPlanes = 1;



    write(fd, bfh, 14);

    write(fd, &bi, 40);

    write(fd, pPix, bs);

    close(fd);

    free(pPix);

}

#endif

