#ifndef _QUANTIZE_H_
#define _QUANTIZE_H_


#include "../enctypes.h"


/* plain c */

void dec_quant_intra(int16_t * coeff,
				const int16_t * data,
				const uint8_t quant,
				const uint8_t dcscalar);

void dec_dequant_intra(int16_t *data,
				const int16_t *coeff,
				const uint8_t quant,
				const uint8_t dcscalar);

int dec_quant_inter(int16_t *coeff,
				const int16_t *data,
				const uint8_t quant);

void dec_dequant_inter(int16_t *data,
				const int16_t *coeff,
				const uint8_t quant);


/* mmx */

void dec_quant_intra_mmx(int16_t * coeff,
					const int16_t * const data,
					const uint32_t quant,
					const uint32_t dcscalar);

void dec_dequant_intra_mmx(int16_t *data,
					const int16_t * const coeff,
					const uint32_t quant,
					const uint32_t dcscalar);

uint32_t dec_quant_inter_mmx(int16_t * coeff,
					const int16_t * const data,
					const uint32_t quant);

void dec_dequant_inter_mmx(int16_t * data,
					const int16_t * const coeff,
					const uint32_t quant);


#endif /* _QUANTIZE_H_ */
