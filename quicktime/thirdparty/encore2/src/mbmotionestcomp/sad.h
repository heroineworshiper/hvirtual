#ifndef _SAD_H_
#define _SAD_H_


#include "enc_portab.h"


/* plain c */


uint32_t sad16(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride,
				const uint32_t best_sad);

uint32_t sad8(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);

uint32_t dev16(const uint8_t * const cur,
				const uint32_t stride);

/* mmx */


uint32_t sad16_mmx(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride,
				const uint32_t best_sad);

uint32_t sad8_mmx(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);


uint32_t dev16_mmx(const uint8_t * const cur,
				const uint32_t stride);


/* xmm */

uint32_t sad16_xmm(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride,
				const uint32_t best_sad);

uint32_t sad8_xmm(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);

uint32_t dev16_xmm(const uint8_t * const cur,
				const uint32_t stride);


#endif /* _SAD_H_ */
