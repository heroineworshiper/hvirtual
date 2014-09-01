/**************************************************************************
 *                                                                        *
 * This code is developed by Adam Li.  This software is an                *
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
 *  rgb2yuv.c, 24-bit RGB bitmap to YUV converter
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/ 

 /**************************************************************************
 *
 *  Modifications:
 *
 *  14.10.2001 added yuv4:2:2_packed (yuy2) support
 *	18.09.2001 only half the line should be copied for u- and v-plane
 *			   in YUV2YUV() - thanks Klaus!
 *  26.08.2001 eliminated compiler warning
 *  19.08.2001 fixed error in init_rgb2yuv
 *             now also works correct for flipped images
 *  10.08.2001 fixed "too dark problem" - now correct brightness
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

  
/* This file contains RGB to YUV transformation functions.                */ 
    

#ifdef WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include "rgb2yuv.h"
#include "enc_portab.h"

static int32_t RGBYUV02570[256], RGBYUV05040[256], RGBYUV00980[256];
static int32_t RGBYUV01480[256], RGBYUV02910[256], RGBYUV04390[256];
static int32_t RGBYUV03680[256], RGBYUV00710[256];

//void InitLookupTable();


/************************************************************************
 *
 *  int RGB2YUV (int x_dim, int y_dim, void *bmp, YUV *yuv)
 *
 *	Purpose :	It takes a 24-bit RGB bitmap and convert it into
 *				YUV (4:1:1) format
 *
 *  Input :		x_dim	the x dimension of the bitmap
 *				y_dim	the y dimension of the bitmap
 *				bmp		pointer to the buffer of the bitmap
 *				yuv		pointer to the YUV structure
 *
 *  Output :	0		OK
 *				1		wrong dimension
 *				2		memory allocation error
 *
 *	Side Effect :
 *				None
 *
 *	Date :		09/28/2000
 *
 *  Contacts:
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 ************************************************************************/ 


int RGB2YUV(int x_dim, int y_dim, uint8_t *bmp, uint8_t *y_out,
	     uint8_t *u_out, uint8_t *v_out, int x_stride, int flip) 

{
    
int i, j, size;

    
uint8_t *b;
    
uint8_t *y, *u, *v;
    
uint8_t *y_buffer, *sub_u_buf, *sub_v_buf;

    


	// check to see if x_dim and y_dim are divisible by 2

	if ((x_dim % 2) || (y_dim % 2))
	return 1;
    
size = x_dim * y_dim;
    

y_buffer = (uint8_t *) y_out;
    
sub_u_buf = (uint8_t *) u_out;
    
sub_v_buf = (uint8_t *) v_out;
    
b = (uint8_t *) bmp;
    
y = y_buffer;
    
u = sub_u_buf;
    
v = sub_v_buf;
    

if (flip)
    {
	
for (j = 0; j < y_dim; j++)
	    

	{
	    
y = y_buffer + (y_dim - j - 1) * x_stride;
	    
u = sub_u_buf + (y_dim / 2 - j / 2 - 1) * x_stride / 2;
	    
v = sub_v_buf + (y_dim / 2 - j / 2 - 1) * x_stride / 2;
 
if (!(j % 2))
	    {
		for (i = 0; i < x_dim / 2; i++)
		{
		    y[0] = (uint8_t)
			((RGBYUV02570
			  [b[2]] + RGBYUV05040[b[1]] +
			  RGBYUV00980[b[0]] + 0x100000) >> 16);
		    
y[1] =
			(uint8_t) (
				   (RGBYUV02570[b[5]] +
				    RGBYUV05040[b[4]] +
				    RGBYUV00980[b[3]] + 0x100000) >> 16);
		    
y += 2;
		    
*u =
			(uint8_t) (
				   (RGBYUV01480[b[5]] +
				    RGBYUV02910[b[4]] +
				    RGBYUV04390[b[3]] +
				    0x800000) >> 16);
		    
*v =
			(uint8_t) (
				   (RGBYUV04390[b[5]] +
				    RGBYUV03680[b[4]] +
				    RGBYUV00710[b[3]] + 0x800000) >> 16);
		    u++;
		    
v++;
		    
b += 6;
		
}
	    }
	    else
		for (i = 0; i < x_dim; i++)
		{
		    *y = (uint8_t)
			((RGBYUV02570
			  [b[2]] + RGBYUV05040[b[1]] +
			  RGBYUV00980[b[0]] + 0x100000) >> 16);

y++;
		    
b += 3;
		
}
	
}
    
}
    else
    {
	
for (j = 0; j < y_dim; j++)
{
	    
y = y_buffer + j * x_stride;
	    
u = sub_u_buf + j / 2 * x_stride / 2;
	    
v = sub_v_buf + j / 2 * x_stride / 2;
	    
if (!(j % 2))
	    {

		for (i = 0; i < x_dim / 2; i++)
		{
		    y[0] = (uint8_t)
			((RGBYUV02570
			  [b[2]] + RGBYUV05040[b[1]] +
			  RGBYUV00980[b[0]] + 0x100000) >> 16);
		    
y[1] =
			(uint8_t) (
				   (RGBYUV02570[b[5]] +
				    RGBYUV05040[b[4]] +
				    RGBYUV00980[b[3]] + 0x100000) >> 16);
		    
y += 2;
		    
*u =
			(uint8_t) (
				   (RGBYUV01480[b[5]] +
				    RGBYUV02910[b[4]] +
				    RGBYUV04390[b[3]] + 0x800000) >> 16);
		    
*v =
			(uint8_t) (
				   (RGBYUV04390[b[5]] +
				    RGBYUV03680[b[4]] +
				    RGBYUV00710[b[3]] + 0x800000) >> 16);
		    u++;

v++;
		    
b += 6;
		
}
	    }
	    else
		for (i = 0; i < x_dim; i++)
		{
		    *y = (uint8_t)
			((RGBYUV02570
			  [b[2]] + RGBYUV05040[b[1]] +
			  RGBYUV00980[b[0]] + 0x100000) >> 16);
		    
y++;
		    
b += 3;
		
}
	
}
    
}
    
return 0;

}

int YUV2YUV(int x_dim, int y_dim, uint8_t *bmp, uint8_t *y_out,
	     uint8_t *u_out, uint8_t *v_out, int x_stride, int flip) 

{
    int i;

    if (flip)
    {
	for (i = 0; i < y_dim; i++)
	{
	    memcpy(y_out + (y_dim - i - 1) * x_stride, bmp, x_dim);
	    bmp += x_dim;
	}
	for (i = 0; i < y_dim / 2; i++)
	{
//	    memcpy(u_out + (y_dim / 2 - i - 1) * x_stride / 2, bmp, x_dim);
	    memcpy(u_out + (y_dim / 2 - i - 1) * x_stride / 2, bmp, x_dim / 2);
		bmp += x_dim / 2;
	}
	for (i = 0; i < y_dim / 2; i++)
	{
//		memcpy(v_out + (y_dim / 2 - i - 1) * x_stride / 2, bmp, x_dim);
	    memcpy(v_out + (y_dim / 2 - i - 1) * x_stride / 2, bmp, x_dim / 2);
		bmp += x_dim / 2;
	}
    }
    else
    {
	for (i = 0; i < y_dim; i++)
	{
	    memcpy(y_out + i * x_stride, bmp, x_dim);
	    bmp += x_dim;
	}
	for (i = 0; i < y_dim / 2; i++)
	{
//	    memcpy(u_out + i * x_stride / 2, bmp, x_dim);
	    memcpy(u_out + i * x_stride / 2, bmp, x_dim / 2);
		bmp += x_dim / 2;
	}
	for (i = 0; i < y_dim / 2; i++)
	{
//	    memcpy(v_out + i * x_stride / 2, bmp, x_dim);
	    memcpy(v_out + i * x_stride / 2, bmp, x_dim / 2);
		bmp += x_dim / 2;
	}
    }
    return 0;
}


void init_rgb2yuv() 
{
    
int i;

for (i = 0; i < 256; i++)
	RGBYUV02570[i] = (int32_t) (0.2570 * i * 65536);
    
for (i = 0; i < 256; i++)
	RGBYUV05040[i] = (int32_t) (0.5040 * i * 65536);
    
for (i = 0; i < 256; i++)
	RGBYUV00980[i] = (int32_t) (0.0980 * i * 65536);

for (i = 0; i < 256; i++)
	RGBYUV01480[i] = -(int32_t) (0.1480 * i * 65536);
    
for (i = 0; i < 256; i++)
	RGBYUV02910[i] = -(int32_t) (0.2910 * i * 65536);

for (i = 0; i < 256; i++)
	RGBYUV04390[i] = (int32_t) (0.4390 * i * 65536);

for (i = 0; i < 256; i++)
	RGBYUV03680[i] = -(int32_t) (0.3680 * i * 65536);
    
for (i = 0; i < 256; i++)
	RGBYUV00710[i] = -(int32_t) (0.0710 * i * 65536);

}


/* yuv4:2:2_packed (yuy2) -> yuv4:2:0 planer
   
   NOTE: does not handle flipping */

void yuv422_to_yuv420p(int x_dim, int y_dim, uint8_t *bmp, 
					  uint8_t *y_out, uint8_t *u_out, uint8_t *v_out, 
					  int x_stride, int flip) 
{

	int dif = x_stride - x_dim;
    int x, y;

	for (y = y_dim; y; y -= 2) {
        
		for (x = x_dim; x; x -= 2) {
            *y_out++ = *bmp++;
			*u_out++ = *bmp++;
			*y_out++ = *bmp++;
			*v_out++ = *bmp++;
         }

		y_out += dif;
		u_out += dif >> 1;
		v_out += dif >> 1; 

		for (x = x_dim; x; x -= 2) {
			*y_out++ = *bmp++;
			bmp++;
			*y_out++ = *bmp++;
			bmp++;
		}

		y_out += dif;
    }

}

