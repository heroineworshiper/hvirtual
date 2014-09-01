/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	quantization/dequantization
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *  19.11.2001  quant_inter now returns sum of abs. coefficient values
 *	02.11.2001	added const to function args <pross@cs.rmit.edu.au>
 *	28.10.2001	total rewrite <pross@cs.rmit.edu.au>
 *
 *************************************************************************/

#include <math.h>
#include "enc_quantize.h"
#include "enctypes.h"


/*	mutliply+shift division table
	you would think 16-bit shift would be faster(?)
	(we only need 13bits precision)
*/

#define SCALEBITS	16
#define FIX(X)		((1L << SCALEBITS) / (X) + 1)

static const uint32_t multipliers[32] =
{
	0,			FIX(2),		FIX(4),		FIX(6),
	FIX(8),		FIX(10),	FIX(12),	FIX(14),
	FIX(16),	FIX(18),	FIX(20),	FIX(22),
	FIX(24),	FIX(26),	FIX(28),	FIX(30),
	FIX(32),	FIX(34),	FIX(36),	FIX(38),
	FIX(40),	FIX(42),	FIX(44),	FIX(46),
	FIX(48),	FIX(50),	FIX(52),	FIX(54),
	FIX(56),	FIX(58),	FIX(60),	FIX(62)
}; 



/* quantize intra-block

    coeff [out]		quantized coefficients
	data  [in]		data block
	quant			quantizer [1,31]
	dcscalar		dcscalar

	h.263 intra,
		level = sign(cof) * (|cof| / (2 x q))

	NOTE: expects data input range [-2048,2047]
*/


void enc_quant_intra(int16_t * coeff, 
	const int16_t * data, 
	const uint8_t quant, 
	const uint8_t dcscalar)
{
	const uint32_t mult = multipliers[quant];
	const uint16_t quant_m_2 = quant << 1;
    uint32_t i;

//if(quant != 5) printf("enc_quant_intra 1 %d %d\n", quant, dcscalar);
	*coeff++ = (*data++ + (dcscalar / 2)) / dcscalar;

	for (i = 63; i; i--) {
		int16_t acLevel = *data++;
//printf("enc_quant_intra 2\n");

		if (acLevel < 0) {
//printf("enc_quant_intra 3\n");
			acLevel = -acLevel;
			if (acLevel < quant_m_2) {
				*coeff++ = 0;
				continue;
			}
			acLevel = (acLevel * mult) >> SCALEBITS;
			*coeff++ = -acLevel;
//printf("enc_quant_intra 4\n");
		} else {
//printf("enc_quant_intra 5\n");
			if (acLevel < quant_m_2) {
				*coeff++ = 0;
				continue;
			}
			*coeff++ = (acLevel * mult) >> SCALEBITS;
//printf("enc_quant_intra 6\n");
		}
	}
}



/*	dequantize intra-block 

	data  [out]		dequantized data block
	coeff [in]		quantized coefficients
	quant			quantizer [1,31]
	dcscalar		dcscalar

	NOTE: does not limit output range [-128,127]

*/
	

void enc_dequant_intra(int16_t *data, const int16_t *coeff, const uint8_t quant, const uint8_t dcscalar)
{
	const uint16_t quant_m_2 = quant << 1;
	const uint16_t quant_add = (quant & 1 ? quant : quant - 1);
    uint32_t i;

	*data++ = *coeff++  * dcscalar;

	for (i = 63; i; i--) {
		int16_t acLevel = *coeff++;
		if (acLevel < 0) {
			*data++ = (acLevel * quant_m_2) - quant_add;
		} else if (acLevel > 0) {
			*data++ = (acLevel * quant_m_2) + quant_add;
		} else {
			*data++ = 0;
		}
	}
}




/*	quantize inter-block

    coeff [out]		quantized coefficients
	data  [in]		data block
	quant			quantizer [1,31]

	returns !0 if any non-zero coefficients found

	h.263 inter,	
		level = sign(cof) * (|cof| - q/2) / (2 x q)

	NOTE: expects data input range [-2048,2047]
*/

int enc_quant_inter(int16_t *coeff, const int16_t *data, const uint8_t quant)
{
	const uint32_t mult = multipliers[quant];
	const uint16_t quant_m_2 = quant << 1;
	const uint16_t quant_d_2 = quant >> 1;
	int sum = 0;
	uint32_t i;
//if(quant != 5) printf("enc_quant_inter 1 %d\n", quant);

	for (i = 64; i; i--) {
		int16_t acLevel = *data++;
		
		if (acLevel < 0) {
			acLevel = (-acLevel) - quant_d_2;
			if (acLevel < quant_m_2) {
				*coeff++ = 0;
				continue;
			}

			acLevel = (acLevel * mult) >> SCALEBITS;
			sum += acLevel;
			*coeff = -acLevel;
			*coeff++;
		} else {
			acLevel -= quant_d_2;
			if (acLevel < quant_m_2) {
				*coeff++ = 0;
				continue;
			}
			*coeff = (acLevel * mult) >> SCALEBITS;
			sum += *coeff;
			*coeff++;
		}
	}
	return sum;
}



/* dequantize inter-block 

	data  [out]		dequantized data block
	coeff [in]		quantized coefficients
	quant			quantizer [1,31]
	dcscalar		dcscalar

	NOTE: does not limit output range [-128,127]

*/


void enc_dequant_inter(int16_t *data, const int16_t *coeff, const uint8_t quant)
{
	const uint16_t quant_m_2 = quant << 1;
	const uint16_t quant_add = (quant & 1 ? quant : quant - 1);
	uint32_t i;

	for (i = 64; i; i--) {
		int16_t acLevel = *coeff++;
		
		if (acLevel < 0) {
			*data++ = (acLevel * quant_m_2) - quant_add;
		} else if (acLevel > 0) {
			*data++ = (acLevel * quant_m_2) + quant_add;
		} else {
			*data++ = 0;
		}
	}
}
