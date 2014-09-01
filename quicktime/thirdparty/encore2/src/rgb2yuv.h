#ifndef __ENCORE_RGB2YUV_H
#define __ENCORE_RGB2YUV_H






#include "enc_portab.h"







int RGB2YUV(int x_dim, int y_dim, uint8_t *bmp, uint8_t *y_out,
	    uint8_t *u_out, uint8_t *v_out, int x_stride, int flip);
int YUV2YUV(int x_dim, int y_dim, uint8_t *bmp, uint8_t *y_out,
	    uint8_t *u_out, uint8_t *v_out, int x_stride, int flip);
void init_rgb2yuv();

void yuv422_to_yuv420p(int x_dim, int y_dim, uint8_t *bmp, 
					  uint8_t *y_out, uint8_t *u_out, uint8_t *v_out, 
					  int x_stride, int flip);


#endif
