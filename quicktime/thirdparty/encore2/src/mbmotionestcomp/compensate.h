#ifndef _COMPENSATE_H_
#define _COMPENSATE_H_

#include "enc_portab.h"


void compensate(int16_t * const dct,
				uint8_t * const cur,
				const uint8_t * ref,
				const uint32_t stride);

void compensate_mmx(int16_t * const dct,
				uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);


#endif /* _COMPENSATE_H_ */
