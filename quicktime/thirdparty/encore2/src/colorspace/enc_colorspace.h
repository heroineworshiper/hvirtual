#ifndef _COLORSPACE_H
#define _COLORSPACE_H

#include "enc_portab.h"


void rgb24_to_yuv(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride);

void rgb32_to_yuv(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride);

void yuv_to_yuv(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
				uint8_t *src, int width, int height, int stride);

void yuyv_to_yuv(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride);

void uyvy_to_yuv(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride);


/* mmx */

void rgb24_to_yuv_mmx(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride);

void rgb32_to_yuv_mmx(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride);

void yuv_to_yuv_mmx(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride);

/* xmm */

void yuv_to_yuv_xmm(uint8_t *y_out, uint8_t *u_out, uint8_t *v_out,
					uint8_t *src, int width, int height, int stride);


#endif /* _COLORSPACE_H_ */
