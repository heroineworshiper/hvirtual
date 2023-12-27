/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "colormodels2.h"
#include "cmodel_priv.h"
#include <stdint.h>
#include <stdio.h>




#define SCALE_FLOAT(in, out, function) \
{ /* scope allows reuse of the function name */ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            DEFAULT_HEAD \
            function(&output_row, (float*)(input_row + column_table[j] * in_pixelsize)); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            DEFAULT_HEAD \
            function(&output_row, (float*)(input_row + j * in_pixelsize)); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}


// pack YUV422
#define SCALE_422_OUT(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            DEFAULT_HEAD \
            function(&output_row, (float*)(input_row + column_table[j] * in_pixelsize), j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            DEFAULT_HEAD \
            function(&output_row, (float*)(input_row + j * in_pixelsize), j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}



#define SCALE_420P_OUT(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV420P_OUT_HEAD \
            function(output_y, output_u, output_v, (float*)(input_row + column_table[j] * in_pixelsize), j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV420P_OUT_HEAD \
            function(output_y, output_u, output_v, (float*)(input_row + j * in_pixelsize), j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_);

#define SCALE_422P_OUT(in, out, function) \
    void function##__(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV422P_OUT_HEAD \
            function(output_y, output_u, output_v, (float*)(input_row + column_table[j] * in_pixelsize), j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV422P_OUT_HEAD \
            function(output_y, output_u, output_v, (float*)(input_row + j * in_pixelsize), j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##__);

#define SCALE_444P_OUT(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV444P_OUT_HEAD \
            function(output_y, output_u, output_v, (float*)(input_row + column_table[j] * in_pixelsize), j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV444P_OUT_HEAD \
            function(output_y, output_u, output_v, (float*)(input_row + j * in_pixelsize), j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_);



static inline void RGB_FLOAT_to_RGB8(unsigned char *(*output), float *input)
{
	unsigned char r, g, b;
	r = (unsigned char)(CLIP(input[0], 0, 1) * 0x3);
	g = (unsigned char)(CLIP(input[1], 0, 1) * 0x7);
	b = (unsigned char)(CLIP(input[2], 0, 1) * 0x3);

	*(*output) = (r << 6) +
			     (g << 2) +
		 	     b;
	(*output)++;
}

static inline void RGB_FLOAT_to_BGR565(unsigned char *(*output), float *input)
{
	unsigned char r, g, b;
	r = (unsigned char)(CLIP(input[0], 0, 1) * 0x1f);
	g = (unsigned char)(CLIP(input[1], 0, 1) * 0x3f);
	b = (unsigned char)(CLIP(input[2], 0, 1) * 0x1f);

	*(uint16_t*)(*output) = (b << 11) |
		(g << 5) |
		r;
	(*output) += 2;
}

static inline void RGB_FLOAT_to_RGB565(unsigned char *(*output), float *input)
{
	unsigned char r, g, b;
	r = (unsigned char)(CLIP(input[0], 0, 1) * 0x1f);
	g = (unsigned char)(CLIP(input[1], 0, 1) * 0x3f);
	b = (unsigned char)(CLIP(input[2], 0, 1) * 0x1f);

	*(uint16_t*)(*output) = (r << 11) |
		(g << 5) |
		b;
	(*output) += 2;
}

static inline void RGB_FLOAT_to_BGR888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = b;
	*(*output)++ = g;
	*(*output)++ = r;
}

static inline void RGB_FLOAT_to_RGB888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void RGB_FLOAT_to_RGBA8888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = 0xff;
}

static inline void RGB_FLOAT_to_ARGB8888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = 0xff;
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void RGB_FLOAT_to_RGBA_FLOAT(unsigned char *(*output), 
	float *input)
{
    float *(*output2) = (float**)output;
	*(*output2)++ = input[0];
	*(*output2)++ = input[1];
	*(*output2)++ = input[2];
	*(*output2)++ = 1.0;
}

static inline void RGB_FLOAT_to_BGR8888(unsigned char *(*output), 
	float *input)
{
	unsigned char r = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	unsigned char g = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	unsigned char b = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}

static inline void RGB_FLOAT_to_YUV888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
}

static inline void RGB_FLOAT_to_YUVA8888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b;

	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
	*(*output)++ = 255;
}


static inline void RGB_FLOAT_to_YUV101010(unsigned char *(*output), 
	float *input)
{
	int r, g, b;
	int y, u, v;

	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);
	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}


static inline void RGB_FLOAT_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	float *input,
	int output_column)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

// printf("RGB_FLOAT_to_YUV420P_YUV422P %d output_y=%p output_u=%p output_v=%p\n", 
// output_column,
// output_y,
// output_u,
// output_v);
 	output_y[output_column] = y >> 8;
 	output_u[output_column / 2] = u >> 8;
 	output_v[output_column / 2] = v >> 8;
}

static inline void RGB_FLOAT_to_YUV422(unsigned char *(*output),
	float *input,
	int j)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);
	if(!(j & 1))
	{
		(*output)[1] = u >> 8;
		(*output)[3] = v >> 8;
		(*output)[0] = y >> 8;
	}
	else
	{
		(*output)[2] = y >> 8;
		(*output) += 4;
	}
}

static inline void RGB_FLOAT_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	float *input,
	int output_column)
{
	int y, u, v, r, g, b;
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	output_y[output_column] = y >> 8;
	output_u[output_column] = u >> 8;
	output_v[output_column] = v >> 8;
}



// ****************************** RGBA FLOAT -> *********************************

static inline void RGBA_FLOAT_to_RGB8(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0x10000);
	r = (uint32_t)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * 0xff * a);

	*(*output) = (unsigned char)(((r & 0xc00000) >> 16) + 
				((g & 0xe00000) >> 18) + 
				((b & 0xe00000) >> 21));
	(*output)++;
}

static inline void RGBA_FLOAT_to_BGR565(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0x10000);
	r = (uint32_t)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * 0xff * a);

	*(uint16_t*)(*output) = (uint16_t)(((b & 0xf80000) >> 8) + 
				((g & 0xfc0000) >> 13) + 
				((r & 0xf80000) >> 19));
	(*output) += 2;
}

static inline void RGBA_FLOAT_to_RGB565(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0x10000);
	r = (uint32_t)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * 0xff * a);

	*(uint16_t*)(*output) = (uint16_t)(((r & 0xf80000) >> 8) + 
				((g & 0xfc0000) >> 13) + 
				((b & 0xf80000) >> 19));
	(*output) += 2;
}

static inline void RGBA_FLOAT_to_BGR888(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0xff);
	r = (uint32_t)(CLIP(input[0], 0, 1) * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * a);

	*(*output)++ = (unsigned char)b;
	*(*output)++ = (unsigned char)g;
	*(*output)++ = (unsigned char)r;
}

static inline void RGBA_FLOAT_to_RGB888(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0xff);
	r = (uint32_t)(CLIP(input[0], 0, 1) * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * a);

	*(*output)++ = (unsigned char)r;
	*(*output)++ = (unsigned char)g;
	*(*output)++ = (unsigned char)b;
}

static inline void RGBA_FLOAT_to_RGB_FLOAT(unsigned char *(*output), 
	float *input)
{
	float a;
	a = input[3];

    float *(*output2) = (float**)output;
	*(*output2)++ = input[0] * a;
	*(*output2)++ = input[1] * a;
	*(*output2)++ = input[2] * a;
}

static inline void RGBA_FLOAT_to_RGBA_FLOAT(unsigned char *(*output), 
	float *input)
{
    float *(*output2) = (float**)output;
	*(*output2)++ = input[0];
	*(*output2)++ = input[1];
	*(*output2)++ = input[2];
	*(*output2)++ = input[3];
}


static inline void RGBA_FLOAT_to_RGBA8888(unsigned char *(*output), 
	float *input)
{
	*(*output)++ = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[3], 0, 1) * 0xff);
}

static inline void RGBA_FLOAT_to_ARGB8888(unsigned char *(*output), 
	float *input)
{
	*(*output)++ = (unsigned char)(CLIP(input[3], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[0], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[1], 0, 1) * 0xff);
	*(*output)++ = (unsigned char)(CLIP(input[2], 0, 1) * 0xff);
}


static inline void RGBA_FLOAT_to_BGR8888(unsigned char *(*output), 
	float *input)
{
	uint32_t r, g, b, a;
	a = (uint32_t)(CLIP(input[3], 0, 1) * 0xff);
	r = (uint32_t)(CLIP(input[0], 0, 1) * a);
	g = (uint32_t)(CLIP(input[1], 0, 1) * a);
	b = (uint32_t)(CLIP(input[2], 0, 1) * a);

	*(*output)++ = (unsigned char)(b);
	*(*output)++ = (unsigned char)(g);
	*(*output)++ = (unsigned char)(r);
	*(*output)++;
}

static inline void RGBA_FLOAT_to_YUV888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b, a;
	a = (int)(CLIP(input[3], 0, 1) * 0x101);
	r = (int)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (int)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (int)(CLIP(input[2], 0, 1) * 0xff * a);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
}

static inline void RGBA_FLOAT_to_YUVA8888(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b, a;

	a = (int)(CLIP(input[3], 0, 1) * 0xff);
	r = (int)(CLIP(input[0], 0, 1) * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);

	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
	*(*output)++ = a;
}


static inline void RGBA_FLOAT_to_YUV101010(unsigned char *(*output), 
	float *input)
{
	int y, u, v, r, g, b, a;

	a = (int)(CLIP(input[3], 0, 1) * 0x101);
	r = (int)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (int)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (int)(CLIP(input[2], 0, 1) * 0xff * a);

	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}


static inline void RGBA_FLOAT_to_YUV420P_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	float *input,
	int output_column)
{
	int y, u, v, r, g, b, a;
	a = (int)(CLIP(input[3], 0, 1) * 0x101);
	r = (int)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (int)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (int)(CLIP(input[2], 0, 1) * 0xff * a);

	RGB_TO_YUV16(y, u, v, r, g, b);

	output_y[output_column] = y >> 8;
	output_u[output_column / 2] = u >> 8;
	output_v[output_column / 2] = v >> 8;
}

static inline void RGBA_FLOAT_to_YUV422(unsigned char *(*output),
	float *input,
	int j)
{
	int y, u, v, r, g, b;
	float a = CLIP(input[3], 0, 1);
	r = (int)(CLIP(input[0], 0, 1) * a * 0xffff);
	g = (int)(CLIP(input[1], 0, 1) * a * 0xffff);
	b = (int)(CLIP(input[2], 0, 1) * a * 0xffff);

	RGB_TO_YUV16(y, u, v, r, g, b);
	if(!(j & 1))
	{
		(*output)[1] = u >> 8;
		(*output)[3] = v >> 8;
		(*output)[0] = y >> 8;
	}
	else
	{
		(*output)[2] = y >> 8;
		(*output) += 4;
	}
}


static inline void RGBA_FLOAT_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	float *input,
	int output_column)
{
	int y, u, v, r, g, b, a;
	a = (int)(CLIP(input[3], 0, 1) * 0x101);
	r = (int)(CLIP(input[0], 0, 1) * 0xff * a);
	g = (int)(CLIP(input[1], 0, 1) * 0xff * a);
	b = (int)(CLIP(input[2], 0, 1) * 0xff * a);

	RGB_TO_YUV16(y, u, v, r, g, b);

	output_y[output_column] = y >> 8;
	output_u[output_column] = u >> 8;
	output_v[output_column] = v >> 8;
}



void cmodel_init_float()
{
    SCALE_FLOAT(BC_RGB_FLOAT, BC_RGB8, RGB_FLOAT_to_RGB8)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_BGR565, RGB_FLOAT_to_BGR565)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_RGB565, RGB_FLOAT_to_RGB565)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_BGR888, RGB_FLOAT_to_BGR888)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_BGR8888, RGB_FLOAT_to_BGR8888)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_RGB888, RGB_FLOAT_to_RGB888)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_RGBA8888, RGB_FLOAT_to_RGBA8888)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_ARGB8888, RGB_FLOAT_to_ARGB8888)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_RGBA_FLOAT, RGB_FLOAT_to_RGBA_FLOAT)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_YUV888, RGB_FLOAT_to_YUV888)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_YUVA8888, RGB_FLOAT_to_YUVA8888)
    SCALE_FLOAT(BC_RGB_FLOAT, BC_YUV101010, RGB_FLOAT_to_YUV101010)
    SCALE_422_OUT(BC_RGB_FLOAT, BC_YUV422, RGB_FLOAT_to_YUV422)
    SCALE_420P_OUT(BC_RGB_FLOAT, BC_YUV420P, RGB_FLOAT_to_YUV420P_YUV422P)
    SCALE_422P_OUT(BC_RGB_FLOAT, BC_YUV422P, RGB_FLOAT_to_YUV420P_YUV422P)
    SCALE_444P_OUT(BC_RGB_FLOAT, BC_YUV444P, RGB_FLOAT_to_YUV444P)


    SCALE_FLOAT(BC_RGBA_FLOAT, BC_RGB8, RGBA_FLOAT_to_RGB8)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_BGR565, RGBA_FLOAT_to_BGR565)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_RGB565, RGBA_FLOAT_to_RGB565)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_BGR888, RGBA_FLOAT_to_BGR888)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_BGR8888, RGBA_FLOAT_to_BGR8888)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_RGB888, RGBA_FLOAT_to_RGB888)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_RGB_FLOAT, RGBA_FLOAT_to_RGB_FLOAT)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_RGBA8888, RGBA_FLOAT_to_RGBA8888)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_RGBA_FLOAT, RGBA_FLOAT_to_RGBA_FLOAT)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_ARGB8888, RGBA_FLOAT_to_ARGB8888)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_YUV888, RGBA_FLOAT_to_YUV888)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_YUVA8888, RGBA_FLOAT_to_YUVA8888)
    SCALE_FLOAT(BC_RGBA_FLOAT, BC_YUV101010, RGBA_FLOAT_to_YUV101010)
    SCALE_422_OUT(BC_RGBA_FLOAT, BC_YUV422, RGBA_FLOAT_to_YUV422)
    SCALE_420P_OUT(BC_RGBA_FLOAT, BC_YUV420P, RGBA_FLOAT_to_YUV420P_YUV422P)
    SCALE_422P_OUT(BC_RGBA_FLOAT, BC_YUV422P, RGBA_FLOAT_to_YUV420P_YUV422P)
    SCALE_444P_OUT(BC_RGBA_FLOAT, BC_YUV444P, RGBA_FLOAT_to_YUV444P)

}
