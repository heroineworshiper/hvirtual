#ifndef _TRANSFER_H_
#define _TRANSFER_H_


#include "enc_portab.h"


/* plain c */

void enc_transfer_8to16copy(int16_t * const dst,
					const uint8_t * const src,
					uint32_t stride);

void enc_transfer_16to8copy(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride);

void enc_transfer_16to8add(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride);


/* mmx */

void enc_transfer_8to16copy_mmx(int16_t * const dst,
					const uint8_t * const src,
					uint32_t stride);

void enc_transfer_16to8copy_mmx(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride);

void enc_transfer_16to8add_mmx(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride);


#endif /* _TRANSFER_H_ */
