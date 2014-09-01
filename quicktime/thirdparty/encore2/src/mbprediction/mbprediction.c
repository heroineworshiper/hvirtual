 /******************************************************************************
  *                                                                            *
  *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
  *                                                                            *
  *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
  *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
  *  software module in hardware or software products are advised that its     *
  *  use may infringe existing patents or copyrights, and any such use         *
  *  would be at such party's own risk.  The original developer of this        *
  *  software module and his/her company, and subsequent editors and their     *
  *  companies, will have no liability for use of this software or             *
  *  modifications or derivatives thereof.                                     *
  *                                                                            *
  *  XviD is free software; you can redistribute it and/or modify it           *
  *  under the terms of the GNU General Public License as published by         *
  *  the Free Software Foundation; either version 2 of the License, or         *
  *  (at your option) any later version.                                       *
  *                                                                            *
  *  XviD is distributed in the hope that it will be useful, but               *
  *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
  *  GNU General Public License for more details.                              *
  *                                                                            *
  *  You should have received a copy of the GNU General Public License         *
  *  along with this program; if not, write to the Free Software               *
  *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  mbprediction.c                                                            *
  *                                                                            *
  *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>                  *
  *  Copyright (C) 2001 - Peter Ross <pross@cs.rmit.edu.au>                    *
  *                                                                            *
  *  For more information visit the XviD homepage: http://www.xvid.org         *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  Revision history:                                                         *
  *                                                                            *
  *  17.11.2001 initial version                                                *
  *                                                                            *
  ******************************************************************************/


#include <assert.h>
#include "enctypes.h"
#include "enc_mbfunctions.h"

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#define _div_div(a, b) (a>0) ? (a+(b>>1))/b : (a-(b>>1))/b

/* 
	scale 'predictor coefficient' to match the current coefficient

       scaled_coeff = (pred_coeff * pred_quant) // current_quant 
*/
#define _rescale(predict_quant, current_quant, coeff)   (coeff != 0) ?  \
	_div_div((coeff) * (predict_quant), (current_quant))    : 0

static const int16_t default_acdc_values[15] = { 1024,
    0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0
};


static __inline uint8_t calc_cbp_intra(int16_t codes[6][64])
/*             
    PLEASE NOTE:
    This function has been duplicated in MBTransQuant and MBPrediction
    to enforce modularity
*/
{
    uint8_t i, j;
    uint8_t cbp = 0;
	uint8_t shl_val[6] = {32, 16, 8, 4, 2, 1};

    for (i = 0; i < 6; i++) {
		for (j = 1; j < 64; j++) {
			int16_t value = codes[i][j];

			if (value != 0) {
				cbp |= shl_val[i];
				break;
			}
		}
    }
    return cbp;
}


static __inline int8_t get_dc_scaler(int8_t quant, bool lum)
/*             
    PLEASE NOTE:
    This function has been duplicated in MBTransQuant and MBPrediction
    to enforce modularity
*/    
{
    int8_t dc_scaler;

	if(quant > 0 && quant < 5) {
        dc_scaler = 8;
		return dc_scaler;
	}

	if(quant < 25 && !lum) {
        dc_scaler = (quant + 13) >> 1;
		return dc_scaler;
	}

	if(quant < 9) {
        dc_scaler = quant << 1;
		return dc_scaler;
	}

    if(quant < 25) {
        dc_scaler = quant + 8;
		return dc_scaler;
	}

	if(lum)
		dc_scaler = (quant << 1) - 16;
	else
        dc_scaler = quant - 6;

    return dc_scaler;
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
/*             
    PLEASE NOTE:
    This function has been duplicated in MBMotionEstComp and MBPrediction
    to enforce modularity
*/    
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

	// median

	*pred_x = MIN(MAX(x1, x2), MIN(MAX(x2, x3), MAX(x1, x3)));
	*pred_y = MIN(MAX(y1, y2), MIN(MAX(y2, y3), MAX(y1, y3)));
}


/*
  perform dc/ac prediction on a single block

  returns how much is saved by performing ac prediction

	[  diag  ] [   top   ]
	[  left  ] [ current ]

*/
static int32_t calc_acdc_prediction(Macroblock *pMBs, uint16_t x, uint16_t y,
									uint16_t x_dim, uint8_t block, int16_t dct_codes[64],
									uint8_t iDcScaler, int32_t current_quant)
{
    int16_t *left, *top, *diag, *current;

    int32_t left_quant = current_quant;
    int32_t top_quant = current_quant;

    const int16_t *pLeft = default_acdc_values;
    const int16_t *pTop = default_acdc_values;
    const int16_t *pDiag = default_acdc_values;

    int16_t *pCurrent;
    int32_t S1 = 0, S2 = 0;
    uint8_t i;
    uint32_t index = x + y * x_dim;
    int16_t dc_pred;
    uint8_t *acpred_direction = &pMBs[index].acpred_directions[block];

	left = top = diag = current = 0;

	/* grab left,top and diag macroblocks */

	/* left macroblock */

    if(x && (pMBs[index - 1].mode == MODE_INTRA 
		|| pMBs[index - 1].mode == MODE_INTRA_Q)) {

		left = pMBs[index - 1].pred_values[0];
		left_quant = pMBs[index - 1].aquant;
	}
    
	/* top macroblock */
	
	if(y && (pMBs[index - x_dim].mode == MODE_INTRA 
		|| pMBs[index - x_dim].mode == MODE_INTRA_Q)) {

		top = pMBs[index - x_dim].pred_values[0];
		top_quant = pMBs[index - x_dim].aquant;
    }
    
	/* diag macroblock */
	
	if(x && y && (pMBs[index - 1 - x_dim].mode == MODE_INTRA 
		|| pMBs[index - 1 - x_dim].mode == MODE_INTRA_Q)) {

		diag = pMBs[index - 1 - x_dim].pred_values[0];
	}

    current = pMBs[index].pred_values[0];
	pCurrent = current + block * MBPRED_SIZE;
    

	/* now grab pLeft, pTop, pDiag _blocks_ */
	
	switch (block) {
	
	case 0: 
		if(left)
			pLeft = left + MBPRED_SIZE;
		
		if(top)
			pTop = top + (MBPRED_SIZE << 1);
		
		if(diag)
			pDiag = diag + 3 * MBPRED_SIZE;
		
		break;
	
	case 1:
		pLeft = current;
		left_quant = current_quant;
		
		if(top) {
			pTop = top + 3 * MBPRED_SIZE;
			pDiag = top + (MBPRED_SIZE << 1);
		}
		
		break;
	
	case 2:
		if(left) {
			pLeft = left + 3 * MBPRED_SIZE;
			pDiag = left + MBPRED_SIZE;
		}
		
		pTop = current;
		top_quant = current_quant;

		
		break;
	
	case 3:
		pLeft = current + (MBPRED_SIZE << 1);
		left_quant = current_quant;
		
		pTop = current + MBPRED_SIZE;
		top_quant = current_quant;
		
		pDiag = current;
		
		break;
	
	case 4:
		if(left)
			pLeft = left + (MBPRED_SIZE << 2);
		
		if(top)
			pTop = top + (MBPRED_SIZE << 2);
		
		if(diag)
			pDiag = diag + (MBPRED_SIZE << 2);
		
		break;
	
	case 5:
		if(left)
			pLeft = left + 5 * MBPRED_SIZE;
		if(top)
			pTop = top + 5 * MBPRED_SIZE;
		if(diag)
			pDiag = diag + 5 * MBPRED_SIZE;
		break;
	}

    
    /*	determine ac prediction direction & dc predictor */

    if(abs(pLeft[0] - pDiag[0]) < abs(pDiag[0] - pTop[0])) {
		*acpred_direction = 1;             // vertical
		dc_pred = _div_div(pTop[0], iDcScaler);
    }
    else {
		*acpred_direction = 2;             // horizontal
		dc_pred = _div_div(pLeft[0], iDcScaler);
    }
    

	/* store current coeffs to pred_values[] */

    pCurrent[0] = dct_codes[0] * iDcScaler;
	for(i = 1; i < 8; i++) {
		pCurrent[i] = dct_codes[i];




//	    assert(pCurrent[i] >= -256);
//		assert(pCurrent[i] <= 256);




		pCurrent[i + 7] = dct_codes[i * 8];








//		assert(pCurrent[i + 7] >= -256);
//		assert(pCurrent[i + 7] <= 256);





    }

	/* 
	subtract DC/AC prediction from current values
	whilst calculating S1/S2 
	   
	S1/S2 are used  to determine if its worth predicting for AC
		S1 = sum of all (dct_codes - prediction)
		S2 = sum of all dct_codes
	*/

	dct_codes[0] -= dc_pred;

	if(*acpred_direction == 1) {
		for(i = 1; i < 8; i++) {
			int16_t level;








//			assert(pTop[i] >= -256);
//			assert(pTop[i] <= 256);










			level = dct_codes[i];
			S2 += abs(level);
			level -= _rescale(top_quant, current_quant, pTop[i]);
			S1 += abs(level);
			dct_codes[i] = level;
		}
	}
    else {
		for(i = 1; i < 8; i++) {
			int16_t level;









//			assert(pLeft[i + 7] >= -256);
//			assert(pLeft[i + 7] <= 256);









			level = dct_codes[i*8];
			S2 += abs(level);
			level -= _rescale(left_quant, current_quant,pLeft[i + 7]);
			S1 += abs(level);
			dct_codes[i*8] = level;
		}
    }
    
    return S2 - S1;
}


void MBPrediction(const MBParam *pParam, uint16_t x_pos, uint16_t y_pos,
				  uint16_t x_dim, int16_t qcoeff[][64], Macroblock *MB_array)
{
    int8_t i, mv_count, iDcScaler, iQuant = pParam->quant;
	int32_t pred_x, pred_y, S = 0;
	int16_t qcoeff_backup[6][64];

    Macroblock *pMB = &MB_array[x_pos + y_pos * x_dim];

    if((pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q)) {
		memcpy(qcoeff_backup, qcoeff, sizeof(qcoeff_backup));

		for(i = 0; i < 6; i++) {   
			iDcScaler = get_dc_scaler(iQuant, (i < 4) ? 1 : 0);

			S += calc_acdc_prediction(MB_array, x_pos, y_pos, x_dim, i,
                      qcoeff[i], iDcScaler, iQuant);
		}

		/* if its not worth performing ac prediction, restore coeff
		   values and 'disable' ac prediction. */
		if(S < 0) {
			for(i = 0; i < 6; i++) {
				pMB->acpred_directions[i] = 0;
				memcpy(&qcoeff[i][1], &qcoeff_backup[i][1], 63 * sizeof(int16_t));
			}
		}

		pMB->cbp = calc_cbp_intra(qcoeff);
    }
    else {
	    mv_count = (pMB->mode == MODE_INTER4V) ? 4 : 1;

		for(i = 0; i < mv_count; i++) {
			get_pmv(MB_array, x_pos, y_pos, x_dim, i, &pred_x, &pred_y);

			pMB->pmvs[i].x = pMB->mvs[i].x - pred_x;
			pMB->pmvs[i].y = pMB->mvs[i].y - pred_y;
		}
    }
}
