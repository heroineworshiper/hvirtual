#ifndef _HALFPEL_H_
#define _HALFPEL_H_


#include "enc_portab.h"


/* plain c */

void interpolate_halfpel_h(uint8_t * const dst, 
						const uint8_t * const src,
						const uint32_t width, 
						const uint32_t height,
						const uint32_t rounding);

void interpolate_halfpel_v(uint8_t * const dst, 
						const uint8_t * const src,
						const uint32_t width,
						const uint32_t height, 
						const uint32_t rounding);

void interpolate_halfpel_hv(uint8_t * const dst,
						const uint8_t * const src,
						const uint32_t width,
						const uint32_t height, 
						const uint32_t rounding);



/* mmx & 3dnow */

void interpolate_halfpel_h_3dn(uint8_t * const dst,
						const uint8_t * const src,
						const uint32_t width,
						const uint32_t height,
						const uint32_t rounding);

void interpolate_halfpel_h_mmx(uint8_t * const dst,
						const uint8_t * const src,
						const uint32_t width,
						const uint32_t height,
						const uint32_t rounding);

void interpolate_halfpel_v_3dn(uint8_t * const dst,
						const uint8_t * const src,
						const uint32_t width,
						const uint32_t height, 
						const uint32_t rounding);

void interpolate_halfpel_v_mmx(uint8_t * const dst,
						const uint8_t * const src,
						const uint32_t width,
						const uint32_t height, 
						const uint32_t rounding);

void interpolate_halfpel_hv_mmx(uint8_t * const dst,
						const uint8_t * const src,
						const uint32_t width,
						const uint32_t height, 
						const uint32_t rounding);


#endif /* _HALFPEL_H_ */
