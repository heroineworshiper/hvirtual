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
 *  MBMotionEstComp.c, motion estimation/compensation module
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
 *	16.11.2001 rewrote/tweaked search algorithms; pross@cs.rmit.edu.au
 *  10.11.2001 support for sad16/sad8 functions
 *  28.08.2001 reactivated MODE_INTER4V for EXT_MODE
 *  24.08.2001 removed MODE_INTER4V_Q, disabled MODE_INTER4V for EXT_MODE
 *	22.08.2001 added MODE_INTER4V_Q			
 *  20.08.2001 added pragma to get rid of internal compiler error with VC6
 *             idea by Cyril. Thanks.
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#include "enc_mbfunctions.h"
#include "encoder.h"
#include "enc_image.h"
#include "timer.h"
#include <assert.h>
#include <stdio.h>
#include "sad.h"
#include "compensate.h"


#if defined(_MMX_)

#if defined(_XMM_)

#define SAD16		sad16_xmm
#define SAD8		sad8_xmm
#define DEV16		dev16_xmm

#else

#define SAD16		sad16_mmx
#define SAD8		sad8_mmx
#define DEV16		dev16_mmx

#endif

#define COMPENSATE	compensate_mmx

#else

#define SAD16		sad16
#define SAD8		sad8
#define DEV16		dev16
#define COMPENSATE	compensate

#endif

// very large value
#define MV_MAX_ERROR	(4096 * 256)

// stop search if sdelta < THRESHOLD
#define MV16_THRESHOLD	192
#define MV8_THRESHOLD	56

/* sad16(0,0) bias; mpeg4 spec suggests nb/2+1 */
/* nb  = vop pixels * 2^(bpp-8) */
#define MV16_00_BIAS	(128+1)

/* INTER bias for INTER/INTRA decision; mpeg4 spec suggests 2*nb */
#define INTER_BIAS	432

/* Parameters which control inter/inter4v decision */
#define IMV16X16			5

/* vector map (vlc delta size) smoother parameters */
#define NEIGH_TEND_16X16	2
#define NEIGH_TEND_8X8		2


// fast ((A)/2)*2
#define EVEN(A)   (((A)<0?(A)+1:(A)) & ~1)


#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#define ABS(X) (((X)>0)?(X):-(X))
#define SIGN(X) (((X)>0)?1:-1)


/* diamond search stuff
   keep the the sequence in circular order (so optimization works)
*/

typedef struct
{
	int32_t dx;
	int32_t dy;
}
DPOINT;


static const DPOINT diamond_small[4] = 
{
	{0, 1}, {1, 0}, {0, -1}, {-1, 0}
};


static const DPOINT diamond_large[8] =
{
	{0, 2}, {1, 1}, {2, 0}, {1, -1}, {0, -2}, {-1, -1}, {-2, 0}, {-1, 1}
};





/*
static __inline const uint8_t * getref16(
				const uint8_t * const refn,
				const uint8_t * const refh,
				const uint8_t * const refv,
				const uint8_t * const refhv,
				const uint32_t x, const uint32_t y,
				const int32_t dx, const int32_t dy,
				const uint32_t stride)
{
	switch ( ((dx&1)<<1) + (dy&1) )		// ((dx%2)?2:0)+((dy%2)?1:0)
    {
	case 0 : return refn + (x*16+dx/2) + (y*16+dy/2)*stride;
    case 1 : return refv + (x*16+dx/2) + (y*16+(dy-1)/2)*stride;
	case 2 : return refh + (x*16+(dx-1)/2) + (y*16+dy/2)*stride;
	default : 
	case 3 : return refhv + (x*16+(dx-1)/2) + (y*16+(dy-1)/2)*stride;
	}
}


static __inline const uint8_t * getref8(
				const uint8_t * const refn,
				const uint8_t * const refh,
				const uint8_t * const refv,
				const uint8_t * const refhv,
				const uint32_t x, const uint32_t y,
				const int32_t dx, const int32_t dy,
				const uint32_t stride)
{
	switch ( ((dx&1)<<1) + (dy&1) )
    {
	case 0 : return refn + (x*8+dx/2) + (y*8+dy/2)*stride;
    case 1 : return refv + (x*8+dx/2) + (y*8+(dy-1)/2)*stride;
	case 2 : return refh + (x*8+(dx-1)/2) + (y*8+dy/2)*stride;
	default : 
	case 3 : return refhv + (x*8+(dx-1)/2) + (y*8+(dy-1)/2)*stride;
	}
} */

/*
typedef struct
{
    uint32_t code;				 // right justified 
    uint32_t len;
}
VLCtable;


static const VLCtable mvtab[33] = {
    {1, 1}, {1, 2}, {1, 3}, {1, 4}, {3, 6}, {5, 7}, {4, 7}, {3, 7},
    {11, 9}, {10, 9}, {9, 9}, {17, 10}, {16, 10}, {15, 10}, {14, 10}, {13, 10},
    {12, 10}, {11, 10}, {10, 10}, {9, 10}, {8, 10}, {7, 10}, {6, 10}, {5, 10},
    {4, 10}, {7, 11}, {6, 11}, {5, 11}, {4, 11}, {3, 11}, {2, 11}, {3, 12},
    {2, 12}
}; */


static const uint32_t mvtab[33] = {
    1,  2,  3,  4,  6,  7,  7,  7,
    9,  9,  9,  10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10,
    10, 11, 11, 11, 11, 11, 11, 12, 12
};


static __inline uint32_t mv_bits(int32_t component, const uint8_t iFcode)
{
    if (component == 0)
		return 1;

    if (component < 0)
		component = -component;

    if (iFcode == 1)
    {
		if (component > 32)
		    component = 32;

		return mvtab[component] + 1;
    }

    component += (1 << (iFcode - 1)) - 1;
    component >>= (iFcode - 1);

    if (component > 32)
		component = 32;

    return mvtab[component] + 1 + iFcode - 1;
}


static __inline uint32_t calc_delta_16(const int32_t dx, const int32_t dy, const uint8_t iFcode)
{
    return NEIGH_TEND_16X16 * (mv_bits(dx, iFcode) + mv_bits(dy, iFcode));
}

static __inline uint32_t calc_delta_8(const int32_t dx, const int32_t dy, const uint8_t iFcode)

{
    return NEIGH_TEND_8X8 * (mv_bits(dx, iFcode) + mv_bits(dy, iFcode));

}




/* calculate the pmv (predicted motion vector)
	(take the median of surrounding motion vectors)
	
	(x,y) = the macroblock
	block = the block within the macroblock
*/
static __inline void get_pmv(const Macroblock * const pMBs,
							const uint32_t x, const uint32_t y,
							const uint32_t x_dim,
							const uint32_t block,
							int32_t * const pred_x, int32_t * const pred_y)
{
    int x1, x2, x3;
	int y1, y2, y3;
    int xin1, xin2, xin3;
    int yin1, yin2, yin3;
    int vec1, vec2, vec3;

    uint32_t index = x + y * x_dim;

	// first row (special case)
    if (y == 0 && (block == 0 || block == 1))
    {
		if (x == 0 && block == 0)		// first column
		{
			*pred_x = 0;
			*pred_y = 0;
			return;
		}
		if (block == 1)
		{
			MotionVector mv = pMBs[index].mvs[0];
			*pred_x = mv.x;
			*pred_y = mv.y;
			return;
		}
		// else
		{
			MotionVector mv = pMBs[index - 1].mvs[1];
			*pred_x = mv.x;
			*pred_y = mv.y;
			return;
		}
    }

	/*
		MODE_INTER, vm18 page 48
		MODE_INTER4V vm18 page 51

					(x,y-1)		(x+1,y-1)
					[   |   ]	[	|   ]
					[ 2 | 3 ]	[ 2 |   ]

		(x-1,y)		(x,y)		(x+1,y)
		[   | 1 ]	[ 0 | 1 ]	[ 0 |   ]
		[   | 3 ]	[ 2 | 3 ]	[	|   ]
	*/

    switch (block)
    {
	case 0:
		xin1 = x - 1;	yin1 = y;		vec1 = 1;
		xin2 = x;		yin2 = y - 1;	vec2 = 2;
		xin3 = x + 1;	yin3 = y - 1;	vec3 = 2;
		break;
	case 1:
		xin1 = x;		yin1 = y;		vec1 = 0;
		xin2 = x;		yin2 = y - 1;   vec2 = 3;
		xin3 = x + 1;	yin3 = y - 1;	vec3 = 2;
	    break;
	case 2:
		xin1 = x - 1;	yin1 = y;		vec1 = 3;
		xin2 = x;		yin2 = y;		vec2 = 0;
		xin3 = x;		yin3 = y;		vec3 = 1;
	    break;
	default:
		xin1 = x;		yin1 = y;		vec1 = 2;
		xin2 = x;		yin2 = y;		vec2 = 0;
		xin3 = x;		yin3 = y;		vec3 = 1;
    }

//printf("get_pmv 1\n");

	if (xin1 < 0 || /* yin1 < 0  || */ xin1 >= (int32_t)x_dim)
	{
	    x1 = 0;
		y1 = 0;
	}
	else
	{
		const MotionVector * const mv = &(pMBs[xin1 + yin1 * x_dim].mvs[vec1]); 
		x1 = mv->x;
		y1 = mv->y;
	}

	if (xin2 < 0 || /* yin2 < 0 || */ xin2 >= (int32_t)x_dim)
	{
		x2 = 0;
		y2 = 0;
	}
	else
	{
		const MotionVector * const mv = &(pMBs[xin2 + yin2 * x_dim].mvs[vec2]); 
		x2 = mv->x;
		y2 = mv->y;
	}

	if (xin3 < 0 || /* yin3 < 0 || */ xin3 >= (int32_t)x_dim)
	{
	    x3 = 0;
		y3 = 0;
	}
	else
	{
		const MotionVector * const mv = &(pMBs[xin3 + yin3 * x_dim].mvs[vec3]); 
		x3 = mv->x;
		y3 = mv->y;
	}
//printf("get_pmv 2\n");

	// median

	*pred_x = MIN(MAX(x1, x2), MIN(MAX(x2, x3), MAX(x1, x3)));
	*pred_y = MIN(MAX(y1, y2), MIN(MAX(y2, y3), MAX(y1, y3)));
//printf("get_pmv 3\n");
}



/* calculate the min/max range (in halfpixels) relative to the pmv
*/

static void __inline get_range(
			int32_t * const min_dx, int32_t * const max_dx,
			int32_t * const min_dy, int32_t * const max_dy,
			const uint32_t x, const uint32_t y, 
			const uint32_t block,					// block dimension, 8 or 16
			const uint32_t width, const uint32_t height,
			const int32_t pred_x, const int32_t pred_y,
			const uint32_t edge,
			const uint32_t fcode)
{
	const int search_range = 32 << (fcode - 1);
    int high = search_range - 1;
    int low = -search_range;

    *max_dx = MIN(MIN(
				high,
				((int32_t)(width + edge - 2) - (int32_t)(x*block) - pred_x)),
				(high - pred_x));

    *max_dy = MIN(MIN(
				high,
				((int32_t)(height + edge - 2) - (int32_t)(y*block) - pred_y)),
				(high - pred_y));

    *min_dx = MAX(MAX(
				low,
				(-(int32_t)(edge + x*block) + 2 - pred_x)),
				(low - pred_x));

    *min_dy = MAX(MAX(
				low,
				(-(int32_t)(edge + y*block) + 2 - pred_y)),
				(low - pred_y));
}



/* getref: calculate reference image pointer 
the decision to use interpolation h/v/hv or the normal image is
based on dx & dy.
*/

static __inline const uint8_t * get_ref(
				const uint8_t * const refn,
				const uint8_t * const refh,
				const uint8_t * const refv,
				const uint8_t * const refhv,
				const uint32_t x, 
				const uint32_t y,
				const uint32_t block,					// block dimension, 8 or 16
				const int32_t dx, 
				const int32_t dy,
				const uint32_t stride)
{
	switch ( ((dx&1)<<1) + (dy&1) )		// ((dx%2)?2:0)+((dy%2)?1:0)
    {
		case 0 : return refn + (int)((x*block+dx/2) + (y*block+dy/2)*stride);
    	case 1 : return refv + (int)((x*block+dx/2) + (y*block+(dy-1)/2)*stride);
		case 2 : return refh + (int)((x*block+(dx-1)/2) + (y*block+dy/2)*stride);

		default : 
		case 3 : 
		{
			uint8_t *result;
//printf("get_ref 1 %p %d %p\n", refhv - 784, dy * stride, refhv + (int32_t)(dy * stride) );
			result = refhv + 
				(int)((x * block + (dx - 1) / 2) + 
				(y * block + (dy - 1) / 2) * stride);
//printf("get_ref 2\n");
			return result;
		}
	}
}



// ********************************************************************

/**
    Search for a single motion vector corresponding to macroblock (x,y),
    starting from motion vector <pred_x, pred_y> ( in half-pixels ),
    with search window determined by search_range & iFcode.
    Store the result in '*pmv' and return optimal SAD.
     pRef - reconstructed image
     pRefH - reconstructed image, interpolated along H axis.
     pRefV, 
	 pRefHV - same as above
**/

static int32_t MotionSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const Image * const pCur,
					const int x, const int y,
					const int pred_x, int pred_y,
					const uint32_t iFcode, 
					const int iQuant,
					const int iQuality, 
					MotionVector * const pmv)
{
	const uint32_t iEdgedWidth = pCur->iEdgedWidth;
	const uint8_t * cur = pCur->pY + x*16 + y*16*iEdgedWidth;
	const uint32_t iEdgeSize = iEdgedWidth - pCur->iWidth;
	const DPOINT * diamond;
    
	int32_t min_dx, max_dx;
    int32_t min_dy, max_dy;
	int32_t dx, dy;
	int32_t center_dx, center_dy;

    int32_t best_sdelta;
    int32_t best_dx, best_dy;
	
	int32_t best_sdelta2;
	int32_t best_dx2, best_dy2;
	
    uint32_t point;
	uint32_t best_point;
    uint32_t count;
  
    uint32_t first_pass = 1;

    
	if (iQuality <= 4)
		first_pass = 0;

//	if (pCur->iWidth % 16)
//		iEdgeSize += (pCur->iWidth % 16) - 16;

    get_range(
			&min_dx, &max_dx,
			&min_dy, &max_dy,
			x, y, 16,
			pCur->iWidth, pCur->iHeight,
			pred_x, pred_y,
			iEdgeSize,
			iFcode);

	min_dx = EVEN(min_dx);
	max_dx = EVEN(max_dx);
	min_dy = EVEN(min_dy);
	max_dy = EVEN(max_dy);
	
	// for 1st pass search, center=(0,0)
	center_dx = EVEN(-pred_x);
	center_dy = EVEN(-pred_y);

	// set sdelta2 to max (since we may not do 2nd pass)

	best_sdelta2 = MV_MAX_ERROR;
    best_dx2 = center_dx;
    best_dy2 = center_dy;

  start:

	// sad/delta for center

	best_sdelta = SAD16(cur,
				get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, pred_x + center_dx, pred_y + center_dy, iEdgedWidth),
				iEdgedWidth, 
				MV_MAX_ERROR);
	if (center_dx == -pred_x && center_dy == -pred_y && best_sdelta <= iQuant * 96)
	{
		best_sdelta -= MV16_00_BIAS;
	}
	best_sdelta += calc_delta_16(center_dx, center_dy, (uint8_t)iFcode) * iQuant;
	best_dx = center_dx;
	best_dy = center_dy;

	diamond = diamond_large;
    point = 0;
    count = 8;
    best_point = 99;	//0;

	if (!(best_sdelta < MV16_THRESHOLD))
    while (1)
    {
		while (count--)
		{
			int32_t sdelta;

		    dx = center_dx + 2*diamond[point].dx;
		    dy = center_dy + 2*diamond[point].dy;

			if (dx < min_dx || dx > max_dx || dy < min_dy || dy > max_dy)
			{
				point =  (point + 1) & 7;		// % 8
				continue;
			}

			if (dx == 0 && dy == 0)
			{
				first_pass = 0;
			}

			sdelta = SAD16(cur,
						get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, pred_x + dx, pred_y + dy, iEdgedWidth),
						iEdgedWidth,
						best_sdelta);

			sdelta += calc_delta_16(dx, dy, (uint8_t)iFcode) * iQuant;

			if (sdelta < best_sdelta)
			{
				best_sdelta = sdelta;
				best_dx = dx;
				best_dy = dy;
				if (best_sdelta < MV16_THRESHOLD)
				{
					break;
				}
				best_point = point;
			}

			point =  (point + 1) & 7;		// % 8
		}

		if (best_sdelta < MV16_THRESHOLD)
		{
			break;
		}
		if (diamond == diamond_small)
		{
			break;
		}
		
	    if ((best_dx == center_dx) && (best_dy == center_dy))
	    {
			diamond = diamond_small;
			point = 0;
			count = 4;
	    }
	    else
	    {
			if (best_dx == center_dx || best_dy == center_dy)
			{
				point = (best_point + 6) & 7;   // % 8
				count = 5;
			}
			else
			{
				point = (best_point + 7) & 7;   // % 8
				count = 3;
			}
			
			if (best_point == 99)
			{
				// we're getting cases where min=-30 & max=-34
				// under normal circumstances we should never get here!
				// likely cause: buggy obtainrange16
//				char tmp[1000];
//				wsprintf(tmp, "16c:%i,%i  min:%i,%i  max:%i,%i\n", center_dx, center_dy, min_dx, min_dy, max_dx, max_dy);
//				OutputDebugString(tmp);

				// set point=6 for binary compatibility with latest cvs snapshot
				point = 6;
			}
			
			center_dx = best_dx;
			center_dy = best_dy;
	    }
	}
	
    if (first_pass)
    {
		best_sdelta2 = best_sdelta;
		best_dx2 = best_dx;
		best_dy2 = best_dy;

		// perform 2nd pass search, center=pmv
		first_pass = 0;
		center_dx = 0;
		center_dy = 0;
		goto start;
    }

	if (best_sdelta2 < best_sdelta)
	{
		best_sdelta = best_sdelta2;
		best_dx = best_dx2;
		best_dy = best_dy2;
    }


	/*
	refinement step
	(only neccessary when diamond points are multiplied by 2)
	*/

	center_dx = best_dx;
	center_dy = best_dy;

	if (!(best_sdelta < MV16_THRESHOLD))
	for (dx = center_dx - 1; dx <= center_dx + 1; dx++)
	{
		for (dy = center_dy - 1; dy <= center_dy + 1; dy++)
	    {
			int32_t sdelta;
			
		    if ((dx == center_dx && dy == center_dy) ||
			    dx < min_dx || dx > max_dx || dy < min_dy || dy > max_dy)
			{
				continue;
			}

			sdelta = SAD16(cur,
					get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, pred_x + dx, pred_y + dy, iEdgedWidth),
					iEdgedWidth,
					best_sdelta);

			sdelta += calc_delta_16(dx, dy, (uint8_t)iFcode) * iQuant;

			if (sdelta < best_sdelta)
			{
				best_sdelta = sdelta;
				best_dx = dx;
				best_dy = dy;
				if (sdelta < MV16_THRESHOLD)
				{
					break;
				}
			}
		}
	}

	pmv->x = pred_x + best_dx;
	pmv->y = pred_y + best_dy;

    return best_sdelta;
}



static uint32_t MotionSearch8(
				const uint8_t * const pRef,
				const uint8_t * const pRefH,
				const uint8_t * const pRefV,
				const uint8_t * const pRefHV,
				const Image * const pCur,
				const uint32_t x, const uint32_t y,
				const int32_t pred_x, const int32_t pred_y,
				const int32_t start_x, const int32_t start_y,
				const uint32_t iFcode,
				const uint32_t iQuant,
				MotionVector * const pmv)
{
	const uint32_t iEdgedWidth = pCur->iEdgedWidth;
	const uint8_t * cur = pCur->pY + x*8 + y*8*iEdgedWidth;
	const uint32_t iEdgeSize = iEdgedWidth - pCur->iWidth;
	const DPOINT * diamond;

    int32_t min_dx, max_dx;
    int32_t min_dy, max_dy;
    int32_t dx, dy;
    int32_t center_dx, center_dy;
	
    uint32_t best_sdelta;
	int32_t best_dx, best_dy;
	
    uint32_t point;
	uint32_t best_point;
    uint32_t count;

//	if (pCur->iWidth % 16)
//		iEdgeSize += (pCur->iWidth % 16) - 16;




//printf("MotionSearch8 1\n");
    get_range(
			&min_dx, &max_dx,
			&min_dy, &max_dy,
			x, y, 8,
			pCur->iWidth, pCur->iHeight,
			pred_x, pred_y,
			iEdgeSize,
			iFcode);

//printf("MotionSearch8 1\n");
	min_dx = EVEN(min_dx);
	max_dx = EVEN(max_dx);
	min_dy = EVEN(min_dy);
	max_dy = EVEN(max_dy);

	// center search on start_x/y (mv from previous frame)

//printf("MotionSearch8 1\n");
	center_dx = EVEN(start_x - pred_x);
    center_dy = EVEN(start_y - pred_y);

//printf("MotionSearch8 1\n");
	if (center_dx < min_dx) 
		center_dx = min_dx;
	else if (center_dx > max_dx) 
		center_dx = max_dx;

//printf("MotionSearch8 1\n");
	if (center_dy < min_dy) 
		center_dy = min_dy;
	else if (center_dy > max_dy) 
		center_dy = max_dy;

//printf("MotionSearch8 1\n");
	// sad/delta for center
	
	best_sdelta = SAD8(cur,
			get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, pred_x + center_dx, pred_y + center_dy, iEdgedWidth),
			iEdgedWidth);
	best_sdelta += calc_delta_8(center_dx, center_dy, (uint8_t)iFcode) * iQuant;

//printf("MotionSearch8 1\n");
	if (best_sdelta < MV8_THRESHOLD) {
		pmv->x = pred_x + center_dx;
		pmv->y = pred_y + center_dy;
	    return best_sdelta;
	}
	best_dx = center_dx;
	best_dy = center_dy;

//printf("MotionSearch8 1\n");

	diamond = diamond_large;
    point = 0;
    count = 8;
    best_point = 99;

//printf("MotionSearch8 1\n");
    while(1)
    {
		while(count--)
		{
			uint32_t sdelta;
			uint8_t *tmp;

			dx = center_dx + diamond[point].dx;
			dy = center_dy + diamond[point].dy;

		    if (dx < min_dx || dx > max_dx || dy < min_dy || dy > max_dy)
		    {
				point =  (point + 1) & 7;		// % 8
				continue;
		    }

//printf("MotionSearch8 1 %d\n", iEdgedWidth);
			tmp = get_ref(pRef, 
						pRefH, 
						pRefV, 
						pRefHV, 
						x, 
						y, 
						8, 
						pred_x + dx, 
						pred_y + dy, 
						iEdgedWidth);
//printf("MotionSearch8 1 %p %p %d\n", cur, tmp, iEdgedWidth);

 			sdelta = SAD8(cur,
					tmp,
					iEdgedWidth);
//printf("MotionSearch8 1\n");

			sdelta += calc_delta_8(dx, dy, (uint8_t)iFcode) * iQuant;
//printf("MotionSearch8 1\n");

			if (sdelta < best_sdelta)
			{
				if (sdelta < MV8_THRESHOLD) {
					pmv->x = pred_x + dx;
					pmv->y = pred_y + dy;
				    return sdelta;
				}
				best_sdelta = sdelta;
				best_dx = dx;
				best_dy = dy;
				best_point = point;
			}

//printf("MotionSearch8 2\n");
 			point =  (point + 1) & 7;		// % 8
		}

		if (diamond == diamond_small)
		{
			break;
		}

		if (best_dx == center_dx && best_dy == center_dy)
		{
			diamond = diamond_small;
			point = 0;
			count = 4;
		}
		else
		{
			if (best_dx == center_dx || best_dy == center_dy)
			{
				point = (best_point + 6) & 7;   // % 8
				count = 5;
			}
			else
			{
				point = (best_point + 7) & 7;   // % 8
				count = 3;
			}
			
			if (best_point == 99)
			{
				// we're getting cases where min=-30 & max=-34
				// under normal circumstances we should never get here!
				// likely cause: buggy obtainrange16
//				char tmp[1000];
//				wsprintf(tmp, "8c:%i,%i  min:%i,%i  max:%i,%i\n", center_dx, center_dy, min_dx, min_dy, max_dx, max_dy);
//				OutputDebugString(tmp);

				// set point=6 for binary compatibility with latest cvs snapshot
				point = 6;
			}
			
			center_dx = best_dx;
			center_dy = best_dy;
	    }
    }

//printf("MotionSearch8 1\n");
	/*
	refinement step
	(only neccessary when diamond points are multiplied by 2)

	center_dx = best_dx;
	center_dy = best_dy;

//printf("MotionSearch8 1\n");
	if (!(best_sdelta < MV8_THRESHOLD))
	for (dx = center_dx - 1; dx <= center_dx + 1; dx++)
	{
		for (dy = center_dy - 1; dy <= center_dy + 1; dy++)
	    {
			uint32_t sdelta;
			
		    if ((dx == center_dx && dy == center_dy) ||
			    dx < min_dx || dx > max_dx || dy < min_dy || dy > max_dy)
			{
				continue;
			}

			sdelta = SAD8(cur,
					get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, pred_x + dx, pred_y + dy, iEdgedWidth),
					iEdgedWidth);

			sdelta += calc_delta_8(dx, dy, (uint8_t)iFcode) * iQuant;

			if (sdelta < best_sdelta)
			{
				best_sdelta = sdelta;
				best_dx = dx;
				best_dy = dy;
				if (sdelta < MV8_THRESHOLD)
				{
					break;
				}
			}
		}
	} */

//printf("MotionSearch8 2\n");
	pmv->x = pred_x + best_dx;
	pmv->y = pred_y + best_dy;

    return best_sdelta;
}




static __inline void CompensateBlock(Image * const pVcur,
				     const Image * const pRefN,
					 const Image * const pRefH,
				     const Image * const pRefV,
					 const Image * const pRefHV,
				     uint32_t x, uint32_t y,
					 const int32_t comp,
					 const int32_t dx,  const int dy,
					 int16_t * const dct_codes)
{
    uint32_t stride = pVcur->iEdgedWidth;  // (comp ? 2 : 1);
    uint8_t * pCur;
    const uint8_t * pRef;
    const Image * pVref;
	int32_t ddx;
	int32_t ddy;

//printf("CompensateBlock 1\n");
	switch ( ((dx&1)<<1) + (dy&1) )   // ((dx%2)?2:0)+((dy%2)?1:0)
    {
    case 0:
		pVref = pRefN;
		ddx = dx/2;
		ddy = dy/2;
		break;

    case 1:
		pVref = pRefV;
		ddx = dx/2;
		ddy = (dy-1)/2;
		break;

    case 2:
		pVref = pRefH;
		ddx = (dx-1)/2;
		ddy = dy/2;
		break;

    case 3:
    default:
		pVref = pRefHV;
		ddx = (dx-1)/2;
		ddy = (dy-1)/2;
		break;
    }

//printf("CompensateBlock 1\n");
    switch (comp)
    {
    case 0:
		pCur = pVcur->pY;
		pRef = pVref->pY;
		break;

    case 1:
		pCur = pVcur->pU;
		pRef = pVref->pU;
		stride /=2;
		break;

    case 2:
	default:
		pCur = pVcur->pV;
		pRef = pVref->pV;
		stride /=2;
		break;
    }

//printf("CompensateBlock 1 %p %p\n", pRef, pCur);
	pCur += (int)(y * stride + x);
    pRef += (int)((y + ddy) * stride + x + ddx);
//printf("CompensateBlock 2 %p %p\n", pRef, pCur);

	COMPENSATE(dct_codes, pCur, pRef, stride);
//printf("CompensateBlock 2\n");
}



#define SEARCH16	MotionSearch16
#define SEARCH8		MotionSearch8


bool MBMotionEstComp(
			const MBParam * const pParam,
		    const uint32_t j,
			const uint32_t i,
		    const Image * const pRef,
			const Image * const pRefH,
		    const Image * const pRefV,
			const Image * const pRefHV,
		    Image * const pCurrent,
		    int16_t dct_codes[][64],
			const int inter4v_mode)

{
    static const uint32_t roundtab[16] =
		{ 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 };


    const uint32_t iWcount = pCurrent->iMbWcount;
	Macroblock * const pMB = pCurrent->pMBs + j + i * iWcount;
 
 
	int32_t pred_x;
	int32_t pred_y;

    MotionVector mv16;

    int32_t sad8_result = 0;
    int32_t sad16_result;
    int32_t deviation;

	int32_t sum;
    int32_t dx, dy;


//printf("MBMotionEstComp 1\n");   
	get_pmv(pCurrent->pMBs, j, i, iWcount, 0, &pred_x, &pred_y);
    sad16_result = SEARCH16(pRef->pY, pRefH->pY, pRefV->pY, pRefHV->pY,
				pCurrent, 
				j, i, pred_x, pred_y,
				pParam->fixed_code,
				pParam->quant,pParam->quality, 
				&mv16); 
//printf("MBMotionEstComp 1\n");   

    if (pParam->quality > 3)
    {
//printf("MBMotionEstComp 1\n");   
		sad8_result = SEARCH8(pRef->pY, 
				pRefH->pY, 
				pRefV->pY, 
				pRefHV->pY,
			    pCurrent, 
				2 * j, 2 * i, 
				pred_x, 
				pred_y,
			    mv16.x, 
				mv16.y, 
				pParam->fixed_code,
			    pParam->quant, 
				&pMB->mvs[0]);

//printf("MBMotionEstComp 1\n");   
		get_pmv(pCurrent->pMBs, j, i, iWcount, 1, &pred_x, &pred_y);
//printf("MBMotionEstComp 1\n");   
		sad8_result += SEARCH8(pRef->pY, 
			pRefH->pY, 
			pRefV->pY, 
			pRefHV->pY,
			pCurrent, 
			2 * j + 1, 
			2 * i, 
			pred_x, 
			pred_y,
			mv16.x, 
			mv16.y, 
			pParam->fixed_code,
			pParam->quant, 
			&pMB->mvs[1]);

//printf("MBMotionEstComp 1\n");   
		get_pmv(pCurrent->pMBs, j, i, iWcount, 2, &pred_x, &pred_y);
//printf("MBMotionEstComp 1\n");   
		sad8_result += SEARCH8(pRef->pY, pRefH->pY, pRefV->pY, pRefHV->pY,
			      pCurrent, 2 * j, 2 * i + 1, pred_x, pred_y,
			      mv16.x, mv16.y, pParam->fixed_code,
			      pParam->quant, &pMB->mvs[2]);

//printf("MBMotionEstComp 1\n");   
		get_pmv(pCurrent->pMBs, j, i, iWcount, 3, &pred_x, &pred_y);
//printf("MBMotionEstComp 1\n");   
		sad8_result += SEARCH8(pRef->pY, pRefH->pY, pRefV->pY, pRefHV->pY,
			      pCurrent, 2 * j + 1, 2 * i + 1, pred_x,
			      pred_y, mv16.x, mv16.y,
			      pParam->fixed_code, pParam->quant,
			      &pMB->mvs[3]);

//printf("MBMotionEstComp 1\n");   
    }


//printf("MBMotionEstComp 1\n");   
    
	/* decide: MODE_INTER or MODE_INTER4V 
		mpeg4:   if (sad8 < sad16 - nb/2+1) use_inter4v
	*/

	if (inter4v_mode == 1) {
		if ((pParam->quality <= 3) || 
			(sad16_result < (sad8_result + (IMV16X16 * pParam->quant)))) { 
			
			sad8_result = sad16_result;
			pMB->mode = MODE_INTER;
			pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = mv16.x;
			pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = mv16.y;
		}
		else
			pMB->mode = MODE_INTER4V;
	}
	else 
	{
		sad8_result = sad16_result;
		pMB->mode = MODE_INTER;
		pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = mv16.x;
		pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = mv16.y;
	}

//printf("MBMotionEstComp 1\n");   

	/* decide: MODE_INTER/4V or MODE_INTRA 
	    if (dev_intra < sad_inter - 2 * nb) use_intra
	*/

	deviation = DEV16(pCurrent->pY + j*16 + i*16*pCurrent->iEdgedWidth, pCurrent->iEdgedWidth);
	
    if (deviation < sad8_result - INTER_BIAS)
    {
		pMB->mode = MODE_INTRA;
		pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = 0;
		pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = 0;

		return 1;
    }
//printf("MBMotionEstComp 1\n");   
    

/** Motion compensation **/

    switch (pMB->mode)
    {
    case MODE_INTER:
    case MODE_INTER_Q:
		dx = pMB->mvs[0].x;
		dy = pMB->mvs[0].y;

		/*if (pParam->quality <= 3)
		{
		    assert(!(dx % 2));
		    assert(!(dy % 2));
		} */

		CompensateBlock(pCurrent,
			pRef, pRefH, pRefV, pRefHV,
			16 * j, 16 * i, 0, dx, dy, dct_codes[0]);
	
		CompensateBlock(pCurrent,
			pRef, pRefH, pRefV, pRefHV,
			16 * j + 8, 16 * i, 0, dx, dy, dct_codes[1]);

		CompensateBlock(pCurrent,
			pRef, pRefH, pRefV, pRefHV,
			16 * j, 16 * i + 8, 0, dx, dy, dct_codes[2]);

		CompensateBlock(pCurrent,
			pRef, pRefH, pRefV, pRefHV,
			16 * j + 8, 16 * i + 8, 0, dx, dy, dct_codes[3]);


		if (!(dx & 3))		// % 4
		    dx /= 2;
		else
		    dx = (dx >> 1) | 1;

		if (!(dy & 3))		// % 4
		    dy /= 2;
		else
		    dy = (dy >> 1) | 1;

		CompensateBlock(pCurrent,
			pRef, pRefH, pRefV, pRefHV,
			8*j, 8*i, 1, dx, dy, dct_codes[4]);

		CompensateBlock(pCurrent,
			pRef, pRefH, pRefV, pRefHV,
			8*j, 8*i, 2, dx, dy, dct_codes[5]);

		break;
   
	
	case MODE_INTER4V:
		// assert(pParam->quality >= 4);
	

		CompensateBlock(pCurrent,
			pRef, pRefH, pRefV, pRefHV,
			16 * j, 16 * i, 0, pMB->mvs[0].x, pMB->mvs[0].y,
			dct_codes[0]);

		CompensateBlock(pCurrent, pRef, pRefH, pRefV, pRefHV,
			16 * j + 8, 16 * i, 0, pMB->mvs[1].x,
			pMB->mvs[1].y, dct_codes[1]);

		CompensateBlock(pCurrent, pRef, pRefH, pRefV, pRefHV,
			16 * j, 16 * i + 8, 0, pMB->mvs[2].x,
			pMB->mvs[2].y, dct_codes[2]);

		CompensateBlock(pCurrent, pRef, pRefH, pRefV, pRefHV,
			16 * j + 8, 16 * i + 8, 0, pMB->mvs[3].x,
			pMB->mvs[3].y, dct_codes[3]);


		sum = pMB->mvs[0].x + pMB->mvs[1].x + pMB->mvs[2].x + pMB->mvs[3].x;
	
		if (sum == 0)
		    dx = 0;
		else
		    dx = SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2);

		sum = pMB->mvs[0].y + pMB->mvs[1].y + pMB->mvs[2].y + pMB->mvs[3].y;

		if (sum == 0)
		    dy = 0;
		else
		    dy = SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2);

		CompensateBlock(pCurrent, pRef, pRefH, pRefV, pRefHV,
			8*j, 8*i, 1, dx, dy, dct_codes[4]);

		CompensateBlock(pCurrent, pRef, pRefH, pRefV, pRefHV,
			8*j, 8*i, 2, dx, dy, dct_codes[5]);
	
		break;


    case MODE_INTRA:
    case MODE_INTRA_Q:

	break;

    }						 // switch

//printf("MBMotionEstComp 2\n");   
	return 0;

    // return (pMB->mode == MODE_INTRA || pMB->mode == MODE_INTRA_Q);
}
