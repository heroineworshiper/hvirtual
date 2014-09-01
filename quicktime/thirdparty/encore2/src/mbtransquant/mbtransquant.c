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
  *  mbtransquant.c                                                            *
  *                                                                            *
  *  Copyright (C) 2001 - Peter Ross <pross@cs.rmit.edu.au>                    *
  *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>                  *
  *                                                                            *
  *  For more information visit the XviD homepage: http://www.xvid.org         *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  Revision history:                                                         *
  *                                                                            *
  *  19.11.2001 introduced coefficient thresholding (Isibaar)                  *
  *  17.11.2001 initial version                                                *
  *                                                                            *
  ******************************************************************************/


#include "enc_transfer.h"
#include "enc_dct.h"
#include "enc_quantize.h"
#include "enc_mbfunctions.h"
#include "enc_timer.h"

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))

#define TOOSMALL_LIMIT 8 // skip blocks having a coefficient sum below this value

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


/* this isnt pretty, but its better than 20 ifdefs */

#if defined(_MMX_)

#define FDCT				enc_fdct_mmx

#if defined(_XMM_)
#define IDCT				enc_idct_sse
#else
#define IDCT				enc_idct_mmx
#endif

#define QUANT_INTRA			enc_quant_intra_mmx
#define QUANT_INTER			enc_quant_inter_mmx
#define DEQUANT_INTRA		enc_dequant_intra_mmx
#define DEQUANT_INTER		enc_dequant_inter_mmx

#define TRANSFER_8TO16COPY	enc_transfer_8to16copy_mmx
#define TRANSFER_16TO8COPY	enc_transfer_16to8copy_mmx
#define TRANSFER_16TO8ADD	enc_transfer_16to8add_mmx

#else

#define FDCT				enc_fdct_int32
#define IDCT				enc_idct_int32

#define QUANT_INTRA			enc_quant_intra
#define QUANT_INTER			enc_quant_inter
#define DEQUANT_INTRA		enc_dequant_intra
#define DEQUANT_INTER		enc_dequant_inter

#define TRANSFER_8TO16COPY	enc_transfer_8to16copy
#define TRANSFER_16TO8COPY	enc_transfer_16to8copy
#define TRANSFER_16TO8ADD	enc_transfer_16to8add

#endif


void MBTransQuantIntra(const MBParam *pParam,
		       const uint16_t x_pos,
		       const uint16_t y_pos,
		       int16_t data[][64], 
			   int16_t qcoeff[][64], 
			   Image * const pCurrent)
{
	uint8_t i;
	uint16_t stride = pCurrent->iEdgedWidth;
	uint8_t iQuant = pParam->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;


//printf("MBTransQuantIntra 1\n");
    pY_Cur = pCurrent->pY + (y_pos << 4) * stride + (x_pos << 4);
    pU_Cur = pCurrent->pU + (y_pos << 3) * (stride >> 1) + (x_pos << 3);
    pV_Cur = pCurrent->pV + (y_pos << 3) * (stride >> 1) + (x_pos << 3);
    
//printf("MBTransQuantIntra 1 %d\n", iQuant);
	for(i = 0; i < 6; i++) {
		uint8_t iDcScaler = get_dc_scaler(iQuant, (i < 4) ? 1 : 0);

		start_timer();
		
		switch(i) {
		case 0 :
			TRANSFER_8TO16COPY(data[0], pY_Cur, stride);
			break;
		case 1 :
			TRANSFER_8TO16COPY(data[1], pY_Cur + 8, stride);
			break;
		case 2 :
		    TRANSFER_8TO16COPY(data[2], pY_Cur + 8 * stride, stride);
			break;
		case 3 :
			TRANSFER_8TO16COPY(data[3], pY_Cur + 8 * stride + 8, stride);
			break;
		case 4 :
			TRANSFER_8TO16COPY(data[4], pU_Cur, stride / 2);
			break;
		case 5 :
			TRANSFER_8TO16COPY(data[5], pV_Cur, stride / 2);
			break;
		}
		stop_transfer_timer();
//printf("MBTransQuantIntra 2\n");

		start_timer();
		FDCT(data[i]);
		stop_dct_timer();

//printf("MBTransQuantIntra 3\n");
		start_timer();
		QUANT_INTRA(qcoeff[i], data[i], iQuant, iDcScaler);
		stop_quant_timer();

//printf("MBTransQuantIntra 4\n");
		start_timer();
		DEQUANT_INTRA(data[i], qcoeff[i], iQuant, iDcScaler);
		stop_iquant_timer();

//printf("MBTransQuantIntra 5\n");
		start_timer();
		IDCT(data[i]);
		stop_idct_timer();

//printf("MBTransQuantIntra 6\n");
		start_timer();
		
//printf("MBTransQuantIntra 7\n");
		switch(i) {
		case 0:
			TRANSFER_16TO8COPY(pY_Cur, data[0], stride);
			break;
		case 1:
			TRANSFER_16TO8COPY(pY_Cur + 8, data[1], stride);
			break;
		case 2:
			TRANSFER_16TO8COPY(pY_Cur + 8 * stride, data[2], stride);
			break;
		case 3:
			TRANSFER_16TO8COPY(pY_Cur + 8 + 8 * stride, data[3], stride);
			break;
		case 4:
			TRANSFER_16TO8COPY(pU_Cur, data[4], stride / 2);
			break;
		case 5:
			TRANSFER_16TO8COPY(pV_Cur, data[5], stride / 2);
			break;
		}
		stop_transfer_timer();
//printf("MBTransQuantIntra 8\n");
    }
}


uint8_t MBTransQuantInter(const MBParam *pParam, const uint16_t x_pos, const uint16_t y_pos,
						  int16_t data[][64], int16_t qcoeff[][64], Image * const pCurrent)
{
    uint8_t i;
    uint16_t stride2 = pCurrent->iEdgedWidth;
    uint8_t iQuant = pParam->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
    uint8_t cbp = 0;
	uint32_t sum;
    
    pY_Cur = pCurrent->pY + (y_pos << 4) * stride2 + (x_pos << 4);
    pU_Cur = pCurrent->pU + (y_pos << 3) * (stride2 >> 1) + (x_pos << 3);
    pV_Cur = pCurrent->pV + (y_pos << 3) * (stride2 >> 1) + (x_pos << 3);

    for(i = 0; i < 6; i++) {
		/* 
		no need to transfer 8->16-bit
		(this is performed already in motion compensation) 
		*/
		start_timer();
		FDCT(data[i]);
		stop_dct_timer();

		start_timer();
		sum = QUANT_INTER(qcoeff[i], data[i], iQuant);
		stop_quant_timer();

		if(sum * iQuant >= TOOSMALL_LIMIT) { // skip block ?

			start_timer();
			DEQUANT_INTER(data[i], qcoeff[i], iQuant);
			stop_iquant_timer();

			cbp |= 1 << (5 - i);

			start_timer();
			IDCT(data[i]);
			stop_idct_timer();

			start_timer();
			
			switch(i) {
			case 0:
				TRANSFER_16TO8ADD(pY_Cur, data[0], stride2);
				break;
			case 1:
				TRANSFER_16TO8ADD(pY_Cur + 8, data[1], stride2);
				break;
			case 2:
				TRANSFER_16TO8ADD(pY_Cur + 8 * stride2, data[2], stride2);
				break;
			case 3:
				TRANSFER_16TO8ADD(pY_Cur + 8 + 8 * stride2, data[3], stride2);
				break;
			case 4:
				TRANSFER_16TO8ADD(pU_Cur, data[4], stride2 / 2);
				break;
			case 5:
				TRANSFER_16TO8ADD(pV_Cur, data[5], stride2 / 2);
				break;
			}
			stop_transfer_timer();
		}
	}
    return cbp;
}
