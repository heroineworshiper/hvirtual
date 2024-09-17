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


#define SCALE_DEFAULT(in, out, function) \
{ /* scope allows reuse of the function name */ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + column_table[j] * in_pixelsize); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + j * in_pixelsize); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}

// alpha multiplies a bg color
#define SCALE_DEFAULT_BG(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + column_table[j] * in_pixelsize, bg_r, bg_g, bg_b); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + j * in_pixelsize, bg_r, bg_g, bg_b); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 1, function##_);

// pack YUV422
#define SCALE_422_OUT(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + column_table[j] * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + j * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_);


// unpack YUV422
#define SCALE_422_IN(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + ((column_table[j] * in_pixelsize) & 0xfffffffc), column_table[j]); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + ((j * in_pixelsize) & 0xfffffffc), j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}

#define YUV422_TO_420P_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *input_row = input_rows[row_table[i]]; \
		unsigned char *output_y = out_y_plane + i * out_rowspan + out_x; \
		unsigned char *output_u = out_u_plane + (i / 2) * (out_rowspan / 2) + (out_x / 2); \
		unsigned char *output_v = out_v_plane + (i / 2) * (out_rowspan / 2) + (out_x / 2); \
		for(j = 0; j < out_w; j++) \
		{

#define SCALE_422_TO_420P(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV422_TO_420P_HEAD \
            function(output_y, output_u, output_v, input_row + ((column_table[j] * in_pixelsize) & 0xfffffffc), j, i); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV422_TO_420P_HEAD \
            function(output_y, output_u, output_v, input_row + ((j * in_pixelsize) & 0xfffffffc), j, i); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}

#define YUV422_TO_422P_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *input_row = input_rows[row_table[i]]; \
		unsigned char *output_y = out_y_plane + i * out_rowspan + out_x; \
		unsigned char *output_u = out_u_plane + i * (out_rowspan / 2) + out_x / 2; \
		unsigned char *output_v = out_v_plane + i * (out_rowspan / 2) + out_x / 2; \
		for(j = 0; j < out_w; j++) \
		{

#define SCALE_422_TO_422P(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV422_TO_422P_HEAD \
            function(output_y, output_u, output_v, input_row + ((column_table[j] * in_pixelsize) & 0xfffffffc), j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV422_TO_422P_HEAD \
            function(output_y, output_u, output_v, input_row + ((j * in_pixelsize) & 0xfffffffc), j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}




#define SCALE_420P(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV420P_OUT_HEAD \
            function(output_y, output_u, output_v, input_row + column_table[j] * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV420P_OUT_HEAD \
            function(output_y, output_u, output_v, input_row + j * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_);

#define SCALE_A420P(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV420P_OUT_HEAD \
            function(output_y, output_u, output_v, output_a, input_row + column_table[j] * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV420P_OUT_HEAD \
            function(output_y, output_u, output_v, output_a, input_row + j * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_);

#define SCALE_422P(in, out, function) \
    void function##__(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV422P_OUT_HEAD \
            function(output_y, output_u, output_v, input_row + column_table[j] * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV422P_OUT_HEAD \
            function(output_y, output_u, output_v, input_row + j * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##__);

#define SCALE_444P(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV444P_OUT_HEAD \
            function(output_y, output_u, output_v, input_row + column_table[j] * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV444P_OUT_HEAD \
            function(output_y, output_u, output_v, input_row + j * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_);

#define SCALE_A444P(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV444P_OUT_HEAD \
            function(output_y, output_u, output_v, output_a, input_row + column_table[j] * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            YUV444P_OUT_HEAD \
            function(output_y, output_u, output_v, output_a, input_row + j * in_pixelsize, j); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_);

// converts A to transparency
#define SCALE_TRANSPARENCY(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + column_table[j] * in_pixelsize, &bit_counter); \
            DEFAULT_TAIL \
        } \
        else \
        { \
            DEFAULT_HEAD \
            function(&output_row, input_row + j * in_pixelsize, &bit_counter); \
            DEFAULT_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_);



// ******************************** RGB888 -> *********************************



static inline void RGB888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	*(*output) = (unsigned char)((input[0] & 0xc0) +
			    			 ((input[1] & 0xe0) >> 2) +
		 	    			 ((input[2] & 0xe0) >> 5));
	(*output)++;
}

static inline void RGB888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	uint16_t r, g, b;
	uint16_t r_s, g_s, b_s;
	r = *input++;
	g = *input++;
	b = *input;
	
	r_s  = (r & 0x01) << 7;
	r_s |= (r & 0x02) << 5;
	r_s |= (r & 0x04) << 3;
	r_s |= (r & 0x08) << 1;
	r_s |= (r & 0x10) >> 1;
	r_s |= (r & 0x20) >> 3;
	r_s |= (r & 0x40) >> 5;
	r_s |= (r & 0x80) >> 7;

	g_s  = (g & 0x01) << 7;
	g_s |= (g & 0x02) << 5;
	g_s |= (g & 0x04) << 3;
	g_s |= (g & 0x08) << 1;
	g_s |= (g & 0x10) >> 1;
	g_s |= (g & 0x20) >> 3;
	g_s |= (g & 0x40) >> 5;
	g_s |= (g & 0x80) >> 7;

	b_s  = (b & 0x01) << 7;
	b_s |= (b & 0x02) << 5;
	b_s |= (b & 0x04) << 3;
	b_s |= (b & 0x08) << 1;
	b_s |= (b & 0x10) >> 1;
	b_s |= (b & 0x20) >> 3;
	b_s |= (b & 0x40) >> 5;
	b_s |= (b & 0x80) >> 7;

	*(uint16_t*)(*output) = ((b_s & 0xf8) << 8)
			 + ((g_s & 0xfc) << 3)
			 + ((r_s & 0xf8) >> 3);
	(*output) += 2;
}

static inline void RGB888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	uint16_t r, g, b;
	r = *input++;
	g = *input++;
	b = *input;
	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}


static inline void RGB888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = *input++;
	*(*output)++ = *input++;
	*(*output)++ = *input;
}

static inline void RGB888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = input[2];
	*(*output)++ = input[1];
	*(*output)++ = input[0];
}


static inline void RGB888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = input[2];
	*(*output)++ = input[1];
	*(*output)++ = input[0];
	(*output)++;
}

static inline void RGB888_to_RGB_FLOAT(unsigned char *(*output), unsigned char *input)
{
    float *(*output2) = (float**)output;
	*(*output2)++ = (float)*input++ / 0xff;
	*(*output2)++ = (float)*input++ / 0xff;
	*(*output2)++ = (float)*input / 0xff;
}

static inline void RGB888_to_RGBA_FLOAT(unsigned char *(*output), unsigned char *input)
{
    float *(*output2) = (float**)output;
	*(*output2)++ = (float)*input++ / 0xff;
	*(*output2)++ = (float)*input++ / 0xff;
	*(*output2)++ = (float)*input / 0xff;
	*(*output2)++ = 1.0;
}

static inline void RGB888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = *input++;
	*(*output)++ = *input++;
	*(*output)++ = *input;
	*(*output)++ = 0xff;
}

static inline void RGB888_to_ARGB8888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = 0xff;
	*(*output)++ = *input++;
	*(*output)++ = *input++;
	*(*output)++ = *input;
}

static inline void RGB888_to_ABGR8888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = 0xff;
	*(*output)++ = input[2];
	*(*output)++ = input[1];
	*(*output)++ = input[0];
}


static inline void RGB888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	*(*output)++ = y;
	*(*output)++ = u;
	*(*output)++ = v;
}


static inline void RGB888_to_YUV101010(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;
	int y, u, v;

	r = ((uint16_t)input[0]) << 8;
	g = ((uint16_t)input[1]) << 8;
	b = ((uint16_t)input[2]) << 8;
	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}

static inline void RGB888_to_VYU888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	*(*output)++ = v;
	*(*output)++ = y;
	*(*output)++ = u;
}

static inline void RGB888_to_UYVA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	*(*output)++ = u;
	*(*output)++ = y;
	*(*output)++ = v;
	*(*output)++ = 0xff;
}



static inline void RGB888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	*(*output)++ = y;
	*(*output)++ = u;
	*(*output)++ = v;
	*(*output)++ = 255;
}


static inline void RGB888_to_YUV420P_422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	output_y[output_column] = y;
	output_u[output_column / 2] = u;
	output_v[output_column / 2] = v;
}

static inline void RGB888_to_YUVA420P_422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *output_a, 
	unsigned char *input,
	int output_column)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	output_y[output_column] = y;
	output_u[output_column / 2] = u;
	output_v[output_column / 2] = v;
	output_a[output_column] = 0xff;
}

static inline void BGR8888_to_YUV420P_422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[2], input[1], input[0]);

	output_y[output_column] = y;
	output_u[output_column / 2] = u;
	output_v[output_column / 2] = v;
}

static inline void RGB888_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	output_y[output_column] = y;
	output_u[output_column] = u;
	output_v[output_column] = v;
}

static inline void RGB888_to_YUVA444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *output_a, 
	unsigned char *input,
	int output_column)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	output_y[output_column] = y;
	output_u[output_column] = u;
	output_v[output_column] = v;
    output_a[output_column] = 0xff;
}

static inline void RGB888_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	if(!(j & 1))
	{ 
// Store U and V for even pixels only
		 (*output)[1] = u;
		 (*output)[3] = v;
		 (*output)[0] = y;
	}
	else
	{ 
// Store Y and advance output for odd pixels only
		 (*output)[2] = y;
		 (*output) += 4;
	}

}


// *************************** RGBA8888 -> ************************************

static inline void RGBA8888_to_TRANSPARENCY(unsigned char *(*output), unsigned char *input, int (*bit_counter))
{
	if((*bit_counter) == 7) *(*output) = 0;

	if(input[3] < 127) 
	{
		*(*output) |= (unsigned char)1 << (7 - (*bit_counter));
	}

	if((*bit_counter) == 0)
	{
		(*output)++;
		(*bit_counter) = 7;
	}
	else
		(*bit_counter)--;
}


// These routines blend in a background color since they should be
// exclusively used for widgets.

static inline void RGBA8888_to_RGB8bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	*(*output) = (unsigned char)((r & 0xc0) + 
				((g & 0xe0) >> 2) + 
				((b & 0xe0) >> 5));
	(*output)++;
}

static inline void RGBA8888_to_BGR565bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	*(uint16_t*)(*output) = (uint16_t)(((b & 0xf8) << 8) + 
				((g & 0xfc) << 3) + 
				((r & 0xf8) >> 3));
	(*output) += 2;
}

static inline void RGBA8888_to_RGB565bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	*(uint16_t*)(*output) = (uint16_t)(((r & 0xf8) << 8)+ 
				((g & 0xfc) << 3) + 
				((b & 0xf8) >> 3));
	(*output) += 2;
}

static inline void RGBA8888_to_RGB888bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void RGBA8888_to_BGR8888bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;

	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;

	*(*output)++ = b;
	*(*output)++ = g;
	*(*output)++ = r;
	(*output)++;
}



static inline void RGBA8888_to_BGR888bg(unsigned char *(*output), unsigned char *input, int bg_r, int bg_g, int bg_b)
{
	unsigned int r, g, b, a, anti_a;
	a = input[3];
	anti_a = 255 - a;
	r = ((unsigned int)input[0] * a + bg_r * anti_a) / 0xff;
	g = ((unsigned int)input[1] * a + bg_g * anti_a) / 0xff;
	b = ((unsigned int)input[2] * a + bg_b * anti_a) / 0xff;
	*(*output)++ = b;
	*(*output)++ = g;
	*(*output)++ = r;
}

// These routines blend in a black background

static inline void RGBA8888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = (unsigned int)input[0] * a;
	g = (unsigned int)input[1] * a;
	b = (unsigned int)input[2] * a;
	*(*output) = (unsigned char)(((r & 0xc000) >> 8) + 
				((g & 0xe000) >> 10) + 
				((b & 0xe000) >> 13));
	(*output)++;
}

static inline void RGBA8888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;
	*(uint16_t*)(*output) = (uint16_t)(((b & 0xf8) << 8) + 
				((g & 0xfc) << 3) + 
				((r & 0xf8) >> 3));
	(*output) += 2;
}

static inline void RGBA8888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;


	*(uint16_t*)(*output) = (uint16_t)(((r & 0xf8) << 8) + 
				((g & 0xfc) << 3) + 
				((b & 0xf8) >> 3));
	(*output) += 2;
}


static inline void RGBA8888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;
	*(*output)++ = b;
	*(*output)++ = g;
	*(*output)++ = r;
}

static inline void RGBA8888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void RGBA8888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output)[3] = input[3];
	(*output) += 4;
}

static inline void RGBA8888_to_ARGB8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[3];
	(*output)[1] = input[0];
	(*output)[2] = input[1];
	(*output)[3] = input[2];
	(*output) += 4;
}

static inline void RGBA8888_to_RGB_FLOAT(unsigned char *(*output), unsigned char *input)
{
	float opacity = (float)input[3];
    float *(*output2) = (float**)output;
	*(*output2)++ = (float)*input++ * opacity / 0xff / 0xff;
	*(*output2)++ = (float)*input++ * opacity / 0xff / 0xff;
	*(*output2)++ = (float)*input * opacity / 0xff / 0xff;
}

static inline void RGBA8888_to_RGBA_FLOAT(unsigned char *(*output), unsigned char *input)
{
    float *(*output2) = (float**)output;
	*(*output2)++ = (float)*input++ / 0xff;
	*(*output2)++ = (float)*input++ / 0xff;
	*(*output2)++ = (float)*input++ / 0xff;
	*(*output2)++ = (float)*input / 0xff;
}

static inline void RGBA8888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	unsigned int r, g, b, a;
	a = input[3];
	r = ((unsigned int)input[0] * a) / 0xff;
	g = ((unsigned int)input[1] * a) / 0xff;
	b = ((unsigned int)input[2] * a) / 0xff;
	*(*output)++ = b;
	*(*output)++ = g;
	*(*output)++ = r;
	(*output)++;
}

static inline void RGBA8888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = (input[0] * a) / 0xff;
	g = (input[1] * a) / 0xff;
	b = (input[2] * a) / 0xff;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	*(*output)++ = y;
	*(*output)++ = u;
	*(*output)++ = v;
}

static inline void RGBA8888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	*(*output)++ = y;
	*(*output)++ = u;
	*(*output)++ = v;
	*(*output)++ = input[3];
}


static inline void RGBA8888_to_YUV101010(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;
	int y, u, v;

	r = ((uint16_t)input[0] * input[3]) + 0x1fe;
	g = ((uint16_t)input[1] * input[3]) + 0x1fe;
	b = ((uint16_t)input[2] * input[3]) + 0x1fe;
	RGB_TO_YUV16(y, u, v, r, g, b);
	WRITE_YUV101010(y, u, v);
}

static inline void RGBA8888_to_VYU888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = ((input[0] * a) >> 8) + 1;
	g = ((input[1] * a) >> 8) + 1;
	b = ((input[2] * a) >> 8) + 1;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	*(*output)++ = v;
	*(*output)++ = y;
	*(*output)++ = u;
}

static inline void RGBA8888_to_UYVA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;

	RGB_TO_YUV(y, u, v, input[0], input[1], input[2]);

	*(*output)++ = u;
	*(*output)++ = y;
	*(*output)++ = v;
	*(*output)++ = input[3];
}

static inline void RGBA8888_to_YUV420P_422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = (input[0] * a) / 0xff;
	g = (input[1] * a) / 0xff;
	b = (input[2] * a) / 0xff;

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column / 2] = u;
	output_v[output_column / 2] = v;
}

static inline void RGBA8888_to_YUVA420P_422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *output_a, 
	unsigned char *input,
	int output_column)
{
	int y, u, v, a, r, g, b;
	
	r = input[0];
	g = input[1];
	b = input[2];
	a = input[3];

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column / 2] = u;
	output_v[output_column / 2] = v;
	output_a[output_column] = a;
}

static inline void RGBA8888_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = (input[0] * a) / 0xff;
	g = (input[1] * a) / 0xff;
	b = (input[2] * a) / 0xff;

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column] = u;
	output_v[output_column] = v;
}

static inline void RGBA8888_to_YUVA444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *output_a, 
	unsigned char *input,
	int output_column)
{
	int y, u, v, a, r, g, b;
	
	r = input[0];
	g = input[1];
	b = input[2];
	a = input[3];

	RGB_TO_YUV(y, u, v, r, g, b);

	output_y[output_column] = y;
	output_u[output_column] = u;
	output_v[output_column] = v;
	output_a[output_column] = a;
}

static inline void RGBA8888_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
	int y, u, v, a, r, g, b;
	
	a = input[3];
	r = (input[0] * a) / 0xff;
	g = (input[1] * a) / 0xff;
	b = (input[2] * a) / 0xff;

	RGB_TO_YUV(y, u, v, r, g, b);

	if(!(j & 1))
	{ 
// Store U and V for even pixels only
		 (*output)[1] = u;
		 (*output)[3] = v;
		 (*output)[0] = y;
	}
	else
	{ 
// Store Y and advance output for odd pixels only
		 (*output)[2] = y;
		 (*output) += 4;
	}

}




// ******************************** YUV888 -> *********************************


static inline void YUV888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}


static inline void YUV888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void YUV888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void YUV888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = (int)input[1];
	v = (int)input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void YUV888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = 0xff;
}

static inline void YUV888_to_ARGB8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(*output)++ = 0xff;
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void YUV888_to_RGB_FLOAT(unsigned char *(*output), unsigned char *input)
{
	float y = (float)input[0] / 0xff;
	int u = input[1];
	int v = input[2];
	float r, g, b;
	
	YUV_TO_FLOAT(y, u, v, r, g, b);

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
}

static inline void YUV888_to_RGBA_FLOAT(unsigned char *(*output), unsigned char *input)
{
	float y = (float)input[0] / 0xff;
	int u = input[1];
	int v = input[2];
	float r, g, b;
	
	YUV_TO_FLOAT(y, u, v, r, g, b);

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
	*(*output2)++ = 1.0;
}

static inline void YUV888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = (int)input[0];
	*(*output)++ = input[1];
	*(*output)++ = input[2];
	*(*output)++ = 0xff;
}


static inline void YUV888_to_VYU888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[2];
	(*output)[1] = input[0];
	(*output)[2] = input[1];
	(*output) += 3;
}


static inline void YUV888_to_UYVA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[0];
	(*output)[2] = input[2];
	(*output)[3] = 0xff;
	(*output) += 4;
}


static inline void YUV888_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[1] = input[1];
		(*output)[3] = input[2];
		(*output)[0] = input[0];
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = input[0];
		(*output) += 4;
	}
}

static inline void YUV888_to_YUV101010(unsigned char *(*output), unsigned char *input)
{
	uint16_t y_i = ((uint16_t)input[0]) << 8;
	uint16_t u_i = ((uint16_t)input[1]) << 8;
	uint16_t v_i = ((uint16_t)input[2]) << 8;
	WRITE_YUV101010(y_i, u_i, v_i);
}

static inline void YUV888_to_YUV420P_422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	output_y[output_column] = input[0];
	output_u[output_column / 2] = input[1];
	output_v[output_column / 2] = input[2];
}

static inline void YUV888_to_YUVA420P_422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *output_a, 
	unsigned char *input,
	int output_column)
{
	output_y[output_column] = input[0];
	output_u[output_column / 2] = input[1];
	output_v[output_column / 2] = input[2];
	output_a[output_column] = input[3];
}


static inline void YUV888_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	output_y[output_column] = input[0];
	output_u[output_column] = input[1];
	output_v[output_column] = input[2];
}

static inline void YUV888_to_YUVA444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *output_a, 
	unsigned char *input,
	int output_column)
{
	output_y[output_column] = input[0];
	output_u[output_column] = input[1];
	output_v[output_column] = input[2];
	output_a[output_column] = input[3];
}

static inline void YUV888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = *input++;
	*(*output)++ = *input++;
	*(*output)++ = *input;
}


static inline void YUV888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	*(*output)++ = b;
	*(*output)++ = g;
	*(*output)++ = r;
}

static inline void YUV888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(*output)++ = b;
	*(*output)++ = g;
	*(*output)++ = r;
	(*output)++;
}





// ******************************** YUVA8888 -> *******************************




static inline void YUVA8888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	
	r *= a;
	g *= a;
	b *= a;

	*(*output) = (unsigned char)(((r & 0xc000) >> 8) + 
				((g & 0xe000) >> 10) + 
				((b & 0xe000) >> 13));
	(*output)++;
}

static inline void YUVA8888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	*(uint16_t*)(*output) = (uint16_t)((b & 0xf800) + 
				((g & 0xfc00) >> 5) + 
				((r & 0xf800) >> 11));
	(*output) += 2;
}

static inline void YUVA8888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	*(uint16_t*)(*output) = (uint16_t)((r & 0xf800) + 
				((g & 0xfc00) >> 5) + 
				((b & 0xf800) >> 11));
	(*output) += 2;
}

static inline void YUVA8888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];

	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	*(*output)++ = b / 0xff;
	*(*output)++ = g / 0xff;
	*(*output)++ = r / 0xff;
}


static inline void YUVA8888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;

	a = input[3];
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];

	YUV_TO_RGB(y, u, v, r, g, b);
	
	r *= a;
	g *= a;
	b *= a;
	*(*output)++ = b / 0xff;
	*(*output)++ = g / 0xff;
	*(*output)++ = r / 0xff;
	(*output)++;
}

static inline void YUVA8888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;
	
	a = input[3];
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];

	YUV_TO_RGB(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

	*(*output)++ = r / 0xff;
	*(*output)++ = g / 0xff;
	*(*output)++ = b / 0xff;
}

static inline void YUVA8888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;

	a = input[3];
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];

	YUV_TO_RGB(y, u, v, r, g, b);
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
	*(*output)++ = a;
}

static inline void YUVA8888_to_ARGB8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a;
	int r, g, b;

	a = input[3];
	y = (input[0] << 16) | (input[0] << 8) | input[0];
	u = input[1];
	v = input[2];

	YUV_TO_RGB(y, u, v, r, g, b);
	*(*output)++ = a;
	*(*output)++ = r;
	*(*output)++ = g;
	*(*output)++ = b;
}

static inline void YUVA8888_to_RGB_FLOAT(unsigned char *(*output), unsigned char *input)
{
	float y, a;
	int u, v;
	float r, g, b;

	a = (float)input[3] / 0xff;
	y = (float)input[0] / 0xff;
	u = input[1];
	v = input[2];

	YUV_TO_FLOAT(y, u, v, r, g, b);
		
	r *= a;
	g *= a;
	b *= a;

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
}

static inline void YUVA8888_to_RGBA_FLOAT(unsigned char *(*output), unsigned char *input)
{
	float y = (float)input[0] / 0xff;
	int u, v;
	float r, g, b, a;

	a = (float)input[3] / 0xff;
	u = input[1];
	v = input[2];

	YUV_TO_FLOAT(y, u, v, r, g, b);

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
	*(*output2)++ = a;
}


static inline void YUVA8888_to_VYU888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a, anti_a;
	a = input[3];
	anti_a = 0xff - a;
	y = ((uint32_t)input[0] * a) / 0xff;
	u = ((uint32_t)input[1] * a + 0x80 * anti_a) / 0xff;
	v = ((uint32_t)input[2] * a + 0x80 * anti_a) / 0xff;
	
	*(*output)++ = v;
	*(*output)++ = y;
	*(*output)++ = u;
}

static inline void YUVA8888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v, a, anti_a;
	a = input[3];
	anti_a = 0xff - a;
	y = ((uint32_t)input[0] * a) / 0xff;
	u = ((uint32_t)input[1] * a + 0x80 * anti_a) / 0xff;
	v = ((uint32_t)input[2] * a + 0x80 * anti_a) / 0xff;
	
	*(*output)++ = y;
	*(*output)++ = u;
	*(*output)++ = v;
}


static inline void YUVA8888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = input[0];
	*(*output)++ = input[1];
	*(*output)++ = input[2];
	*(*output)++ = input[3];
}

static inline void YUVA8888_to_UYVA8888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = input[1];
	*(*output)++ = input[0];
	*(*output)++ = input[2];
	*(*output)++ = input[3];
}

static inline void YUVA8888_to_YUV101010(unsigned char *(*output), unsigned char *input)
{
	uint16_t y_i = ((uint16_t)input[0] * input[3]) + 0x1fe;
	uint16_t u_i = ((uint16_t)input[1] * input[3]) + 0x1fe;
	uint16_t v_i = ((uint16_t)input[2] * input[3]) + 0x1fe;
	WRITE_YUV101010(y_i, u_i, v_i);
}


static inline void YUVA8888_to_YUV420P_422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int opacity = input[3];
	int transparency = 0xff - opacity;

	output_y[output_column] =     ((input[0] * opacity) >> 8) + 1;
	output_u[output_column / 2] = ((input[1] * opacity + 0x80 * transparency) >> 8) + 1;
	output_v[output_column / 2] = ((input[2] * opacity + 0x80 * transparency) >> 8) + 1;
}

static inline void YUVA8888_to_YUVA420P_422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *output_a, 
	unsigned char *input,
	int output_column)
{
	output_y[output_column] =     input[0];
	output_u[output_column / 2] = input[1];
	output_v[output_column / 2] = input[2];
	output_a[output_column] =     input[3];
}

static inline void YUVA8888_to_YUV444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
	int opacity = input[3];
	int transparency = 0xff - opacity;

	output_y[output_column] = ((input[0] * opacity) >> 8) + 1;
	output_u[output_column] = ((input[1] * opacity + 0x80 * transparency) >> 8) + 1;
	output_v[output_column] = ((input[2] * opacity + 0x80 * transparency) >> 8) + 1;
}

static inline void YUVA8888_to_YUVA444P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *output_a, 
	unsigned char *input,
	int output_column)
{
	output_y[output_column] = input[0];
	output_u[output_column] = input[1];
	output_v[output_column] = input[2];
	output_a[output_column] = input[3];
}

static inline void YUVA8888_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
	int opacity = input[3];
	int transparency = 0xff - opacity;
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[0] = ((input[0] * opacity) >> 8) + 1;
		(*output)[1] = ((input[1] * opacity + 0x80 * transparency) >> 8) + 1;
		(*output)[3] = ((input[2] * opacity + 0x80 * transparency) >> 8) + 1;
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = ((input[0] * opacity) >> 8) + 1;
		(*output) += 4;
	}
}


// ********************************* YUV101010 -> *****************************

#define READ_YUV101010 \
	uint64_t y, u, v; \
	uint32_t input_i = input[0] | \
		(input[1] << 8) | \
		(input[2] << 16) | \
		(input[3] << 24); \
 \
	y = ((input_i & 0xffc00000) >> 16) | 0x3f; \
	u = ((input_i & 0x3ff000) >> 6) | 0x3f; \
	v = ((input_i & 0xffc) << 4) | 0x3f;






static inline void YUV101010_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = (unsigned char)(((input[0] & 0xc000) >> 8) +
			    			 ((input[1] & 0xe000) >> 10) +
		 	    			 ((input[2] & 0xe000) >> 13));
}

static inline void YUV101010_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(uint16_t*)(*output) = (b & 0xf800) |
			 ((g & 0xfc00) >> 5) |
			 ((r & 0xf800) >> 11);
	(*output) += 2;
}

static inline void YUV101010_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(uint16_t*)(*output) = (r & 0xf800) |
			 ((g & 0xfc00) >> 5) |
			 ((b & 0xf800) >> 11);
	(*output) += 2;
}

static inline void YUV101010_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = b >> 8;
	*(*output)++ = g >> 8;
	*(*output)++ = r >> 8;
}

static inline void YUV101010_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = b >> 8;
	*(*output)++ = g >> 8;
	*(*output)++ = r >> 8;
	(*output)++;
}

static inline void YUV101010_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	READ_YUV101010
	 
	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
}

static inline void YUV101010_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	READ_YUV101010
	 
	*(*output)++ = y >> 8;
	*(*output)++ = u >> 8;
	*(*output)++ = v >> 8;
	*(*output)++ = 0xff;
}

static inline void YUV101010_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = r >> 8;
	*(*output)++ = g >> 8;
	*(*output)++ = b >> 8;
}

static inline void YUV101010_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int r, g, b;

	READ_YUV101010

	y = (y << 8) | (y >> 8);

	YUV_TO_RGB16(y, u, v, r, g, b);

	*(*output)++ = r >> 8;
	*(*output)++ = g >> 8;
	*(*output)++ = b >> 8;
	*(*output)++ = 0xff;
}


static inline void YUV101010_to_RGB_FLOAT(unsigned char *(*output), 
	unsigned char *input)
{
	float r, g, b;
	float y_f;

	READ_YUV101010

	y_f = (float)y / 0xffff;

	YUV16_TO_RGB_FLOAT(y_f, u, v, r, g, b);

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
}

static inline void YUV101010_to_RGBA_FLOAT(unsigned char *(*output), 
	unsigned char *input)
{
	float r, g, b;
	float y_f;

	READ_YUV101010

	y_f = (float)y / 0xffff;

	YUV16_TO_RGB_FLOAT(y_f, u, v, r, g, b);

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
	*(*output2)++ = 1.0;
}






// ******************************** VYU888 -> *********************************


static inline void VYU888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void VYU888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void VYU888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);
	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void VYU888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 3;
}

static inline void VYU888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);
	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 4;
}


static inline void VYU888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void VYU888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[2];
	v = input[0];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}



static inline void VYU888_to_RGB_FLOAT(unsigned char *(*output), unsigned char *input)
{
	float y;
	int u, v;
	float r, g, b;
	
	v = *input++;
	y = (float)*input++ / 0xff;
	u = *input;
	YUV_TO_FLOAT(y, u, v, r, g, b);

//printf("VYU888_to_RGB_FLOAT %d (*output)=%p\n", __LINE__, *output);

    float *(*output2) = (float**)output;
	(*output2)[0] = r;
	(*output2)[1] = g;
	(*output2)[2] = b;
	(*output2) += 3;
}

static inline void VYU888_to_RGBA_FLOAT(unsigned char *(*output), unsigned char *input)
{
	float y;
	int u, v;
	float r, g, b;
	
	v = *input++;
	y = (float)*input++ / 0xff;
	u = *input;
	YUV_TO_FLOAT(y, u, v, r, g, b);

    float *(*output2) = (float**)output;
	(*output2)[0] = r;
	(*output2)[1] = g;
	(*output2)[2] = b;
	(*output2)[3] = 1.0;
	(*output2) += 4;
}


static inline void VYU888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[2];
	(*output)[2] = input[0];
	(*output) += 3;
}

static inline void VYU888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[2];
	(*output)[2] = input[0];
	(*output)[3] = 0xff;
	(*output) += 4;
}



// ******************************** UYVA8888 -> *********************************


static inline void UYVA8888_to_RGB8(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;

	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;
	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void UYVA8888_to_BGR565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;
	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void UYVA8888_to_RGB565(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;
	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void UYVA8888_to_BGR888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;

	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 3;
}

static inline void UYVA8888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;

	(*output)[2] = r;
	(*output)[1] = g;
	(*output)[0] = b;
	(*output) += 4;
}


static inline void UYVA8888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);
	r = r * input[3] / 0xff;
	g = g * input[3] / 0xff;
	b = b * input[3] / 0xff;

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void UYVA8888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	int y, u, v;
	int r, g, b;
	
	y = ((int)input[1]) << 16;
	u = input[0];
	v = input[2];
	YUV_TO_RGB(y, u, v, r, g, b);

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = input[3];
	(*output) += 4;
}


static inline void UYVA8888_to_RGB_FLOAT(unsigned char *(*output), unsigned char *input)
{
	float y, a;
	int u, v;
	float r, g, b;
	
	u = *input++;
	y = (float)*input++ / 0xff;
	v = *input++;
	a = (float)*input / 0xff;
	YUV_TO_FLOAT(y, u, v, r, g, b);

	r = r * a;
	g = g * a;
	b = b * a;

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
}

static inline void UYVA8888_to_RGBA_FLOAT(unsigned char *(*output), unsigned char *input)
{
	float y, a;
	int u, v;
	float r, g, b;
	
	u = *input++;
	y = (float)*input++ / 0xff;
	v = *input++;
	a = (float)*input / 0xff;
	YUV_TO_FLOAT(y, u, v, r, g, b);

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
	*(*output2)++ = a;
}


static inline void UYVA8888_to_YUV888(unsigned char *(*output), unsigned char *input)
{
	int a, anti_a;
	a = input[3];
	anti_a = 0xff - a;

	(*output)[0] = (a * input[1]) / 0xff;
	(*output)[1] = (a * input[0] + anti_a * 0x80) / 0xff;
	(*output)[2] = (a * input[2] + anti_a * 0x80) / 0xff;
	(*output) += 3;
}

static inline void UYVA8888_to_YUVA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[0];
	(*output)[2] = input[2];
	(*output)[3] = input[3];
	(*output) += 4;
}




// ****************************** ARGB8888 -> *********************************

static inline void ARGB8888_to_ARGB8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[1];
	(*output)[2] = input[2];
	(*output)[3] = input[3];
	(*output) += 4;
}

static inline void ARGB8888_to_ABGR8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[0];
	(*output)[1] = input[3];
	(*output)[2] = input[2];
	(*output)[3] = input[1];
	(*output) += 4;
}

static inline void ARGB8888_to_RGBA8888(unsigned char *(*output), unsigned char *input)
{
	(*output)[0] = input[1];
	(*output)[1] = input[2];
	(*output)[2] = input[3];
	(*output)[3] = input[0];
	(*output) += 4;
}


static inline void ARGB8888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	int a = input[0];
	(*output)[0] = input[1] * a / 0xff;
	(*output)[1] = input[2] * a / 0xff;
	(*output)[2] = input[3] * a / 0xff;
	(*output) += 3;
}

static inline void ARGB8888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	int a = input[0];
	(*output)[0] = input[3] * a / 0xff;
	(*output)[1] = input[2] * a / 0xff;
	(*output)[2] = input[1] * a / 0xff;
	(*output) += 3;
}




// ********************************** screen capture *****************************

static inline void BGR8888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = input[2];
	*(*output)++ = input[1];
	*(*output)++ = input[0];
}

static inline void BGR8888_to_BGR8888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = input[0];
	*(*output)++ = input[1];
	*(*output)++ = input[2];
	(*output)++;
}

static inline void BGR888_to_RGB888(unsigned char *(*output), unsigned char *input)
{
	*(*output)++ = input[2];
	*(*output)++ = input[1];
	*(*output)++ = input[0];
}




// ******************************* YUV422 IN ***********************************

static inline void YUV422_to_RGB8(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;

	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void YUV422_to_BGR565(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void YUV422_to_RGB565(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}

static inline void YUV422_to_BGR888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 3;
}

static inline void YUV422_to_RGB888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void YUV422_to_RGBA8888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (input[0] << 16) | (input[0] << 8) | input[0];
	else
// Odd pixel
		y = (input[2] << 16) | (input[2] << 8) | input[2];
	u = input[1];
	v = input[3];
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}


static inline void YUV422_to_RGB_FLOAT(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	float y;
// Signedness is important
	int u, v;
	float r, g, b;

// Even pixel
	if(!(column & 1))
		y = (float)input[0] / 0xff;
	else
// Odd pixel
		y = (float)input[2] / 0xff;
	u = input[1];
	v = input[3];
	YUV_TO_FLOAT(y, u, v, r, g, b)

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
}

static inline void YUV422_to_RGBA_FLOAT(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	float y;
// Signedness is important
	int u, v;
	float r, g, b;

// Even pixel
	if(!(column & 1))
		y = (float)input[0] / 0xff;
	else
// Odd pixel
		y = (float)input[2] / 0xff;
	u = input[1];
	v = input[3];
	YUV_TO_FLOAT(y, u, v, r, g, b)

    float *(*output2) = (float**)output;
	*(*output2)++ = r;
	*(*output2)++ = g;
	*(*output2)++ = b;
	*(*output2)++ = 1.0;
}

static inline void YUV422_to_YUV888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = input[0];
	else
// Odd pixel
		(*output)[0] = input[2];

	(*output)[1] = input[1];
	(*output)[2] = input[3];
	(*output) += 3;
}

static inline void YUV422_to_YUVA8888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
// Even pixel
	if(!(column & 1))
		(*output)[0] = input[0];
	else
// Odd pixel
		(*output)[0] = input[2];

	(*output)[1] = input[1];
	(*output)[2] = input[3];
	(*output)[3] = 255;
	(*output) += 4;
}

static inline void YUV422_to_BGR8888(unsigned char *(*output), 
	unsigned char *input, 
	int column)
{
	int y, u, v;
	int r, g, b;

// Even pixel
	if(!(column & 1))
		y = (int)(input[0]) << 16;
	else
// Odd pixel
		y = (int)(input[2]) << 16;
	u = input[1];
	v = input[3];

	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}


static inline void YUV422_to_YUV422(unsigned char *(*output), 
	unsigned char *input,
	int j)
{
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[0] = input[0];
		(*output)[1] = input[1];
		(*output)[3] = input[3];
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = input[2];
		(*output) += 4;
	}
}


static inline void YUV422_to_YUV422P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column)
{
// Store U and V for even pixels only
	if(!(output_column & 1))
	{
		output_y[output_column] = input[0];
		output_u[output_column / 2] = input[1];
		output_v[output_column / 2] = input[3];
	}
	else
// Store Y and advance output for odd pixels only
	{
		output_y[output_column] = input[2];
	}
}

static inline void YUV422_to_YUV420P(unsigned char *output_y, 
	unsigned char *output_u, 
	unsigned char *output_v, 
	unsigned char *input,
	int output_column,
	int output_row)
{
// Even column
	if(!(output_column & 1))
	{
		output_y[output_column] = input[0];
// Store U and V for even columns and even rows only
		if(!(output_row & 1))
		{
			output_u[output_column / 2] = input[1];
			output_v[output_column / 2] = input[3];
		}
	}
	else
// Odd column
	{
		output_y[output_column] = input[2];
	}
}


void cmodel_init_default()
{
// intermediate formats
    SCALE_DEFAULT(BC_RGB888, BC_RGB8, RGB888_to_RGB8)
    SCALE_DEFAULT(BC_RGB888, BC_BGR565, RGB888_to_BGR565)
    SCALE_DEFAULT(BC_RGB888, BC_RGB565, RGB888_to_RGB565)
    SCALE_DEFAULT(BC_RGB888, BC_RGB888, RGB888_to_RGB888)
    SCALE_DEFAULT(BC_RGB888, BC_BGR888, RGB888_to_BGR888)
    SCALE_DEFAULT(BC_RGB888, BC_BGR8888, RGB888_to_BGR8888)
    SCALE_DEFAULT(BC_RGB888, BC_RGBA8888, RGB888_to_RGBA8888)
    SCALE_DEFAULT(BC_RGB888, BC_ARGB8888, RGB888_to_ARGB8888)
    SCALE_DEFAULT(BC_RGB888, BC_RGB_FLOAT, RGB888_to_RGB_FLOAT)
    SCALE_DEFAULT(BC_RGB888, BC_RGBA_FLOAT, RGB888_to_RGBA_FLOAT)
    SCALE_DEFAULT(BC_RGB888, BC_ABGR8888, RGB888_to_ABGR8888)
    SCALE_DEFAULT(BC_RGB888, BC_YUV888, RGB888_to_YUV888)
    SCALE_DEFAULT(BC_RGB888, BC_YUVA8888, RGB888_to_YUVA8888)
    SCALE_DEFAULT(BC_RGB888, BC_YUV101010, RGB888_to_YUV101010)
    SCALE_422_OUT(BC_RGB888, BC_YUV422, RGB888_to_YUV422)
    SCALE_420P(BC_RGB888, BC_YUV420P, RGB888_to_YUV420P_422P)
    SCALE_A420P(BC_RGB888, BC_YUVA420P, RGB888_to_YUVA420P_422P)
    SCALE_422P(BC_RGB888, BC_YUV422P, RGB888_to_YUV420P_422P)
    SCALE_444P(BC_RGB888, BC_YUV444P, RGB888_to_YUV444P)
    SCALE_A444P(BC_RGB888, BC_YUVA444P, RGB888_to_YUVA444P)


    SCALE_TRANSPARENCY(BC_RGBA8888, BC_TRANSPARENCY, RGBA8888_to_TRANSPARENCY)
    SCALE_DEFAULT(BC_RGBA8888, BC_RGB8, RGBA8888_to_RGB8)
    SCALE_DEFAULT_BG(BC_RGBA8888, BC_RGB8, RGBA8888_to_RGB8bg)
    SCALE_DEFAULT(BC_RGBA8888, BC_BGR565, RGBA8888_to_BGR565)
    SCALE_DEFAULT_BG(BC_RGBA8888, BC_BGR565, RGBA8888_to_BGR565bg)
    SCALE_DEFAULT(BC_RGBA8888, BC_RGB565, RGBA8888_to_RGB565)
    SCALE_DEFAULT_BG(BC_RGBA8888, BC_RGB565, RGBA8888_to_RGB565bg)
    SCALE_DEFAULT(BC_RGBA8888, BC_BGR888, RGBA8888_to_BGR888)
    SCALE_DEFAULT_BG(BC_RGBA8888, BC_BGR888, RGBA8888_to_BGR888bg)
    SCALE_DEFAULT(BC_RGBA8888, BC_BGR8888, RGBA8888_to_BGR8888)
    SCALE_DEFAULT_BG(BC_RGBA8888, BC_BGR8888, RGBA8888_to_BGR8888bg)
    SCALE_DEFAULT(BC_RGBA8888, BC_RGB888, RGBA8888_to_RGB888)
    SCALE_DEFAULT_BG(BC_RGBA8888, BC_RGB888, RGBA8888_to_RGB888bg)
    SCALE_DEFAULT(BC_RGBA8888, BC_RGBA8888, RGBA8888_to_RGBA8888)
    SCALE_DEFAULT(BC_RGBA8888, BC_ARGB8888, RGBA8888_to_ARGB8888)
    SCALE_DEFAULT(BC_RGBA8888, BC_RGB_FLOAT, RGBA8888_to_RGB_FLOAT)
    SCALE_DEFAULT(BC_RGBA8888, BC_RGBA_FLOAT, RGBA8888_to_RGBA_FLOAT)
    SCALE_DEFAULT(BC_RGBA8888, BC_YUV888, RGBA8888_to_YUV888)
    SCALE_DEFAULT(BC_RGBA8888, BC_YUVA8888, RGBA8888_to_YUVA8888)
    SCALE_DEFAULT(BC_RGBA8888, BC_YUV101010, RGBA8888_to_YUV101010)
    SCALE_422_OUT(BC_RGBA8888, BC_YUV422, RGBA8888_to_YUV422)
    SCALE_420P(BC_RGBA8888, BC_YUV420P, RGBA8888_to_YUV420P_422P)
    SCALE_A420P(BC_RGBA8888, BC_YUVA420P, RGBA8888_to_YUVA420P_422P)
    SCALE_422P(BC_RGBA8888, BC_YUV422P, RGBA8888_to_YUV420P_422P)
    SCALE_444P(BC_RGBA8888, BC_YUV444P, RGBA8888_to_YUV444P)
    SCALE_A444P(BC_RGBA8888, BC_YUVA444P, RGBA8888_to_YUVA444P)


    SCALE_DEFAULT(BC_YUV888, BC_RGB8, YUV888_to_RGB8)
    SCALE_DEFAULT(BC_YUV888, BC_BGR565, YUV888_to_BGR565)
    SCALE_DEFAULT(BC_YUV888, BC_RGB565, YUV888_to_RGB565)
    SCALE_DEFAULT(BC_YUV888, BC_RGB888, YUV888_to_RGB888)
    SCALE_DEFAULT(BC_YUV888, BC_RGBA8888, YUV888_to_RGBA8888)
    SCALE_DEFAULT(BC_YUV888, BC_ARGB8888, YUV888_to_ARGB8888)
    SCALE_DEFAULT(BC_YUV888, BC_BGR888, YUV888_to_BGR888)
    SCALE_DEFAULT(BC_YUV888, BC_BGR8888, YUV888_to_BGR8888)
    SCALE_DEFAULT(BC_YUV888, BC_RGB_FLOAT, YUV888_to_RGB_FLOAT)
    SCALE_DEFAULT(BC_YUV888, BC_RGBA_FLOAT, YUV888_to_RGBA_FLOAT)
    SCALE_DEFAULT(BC_YUV888, BC_YUV101010, YUV888_to_YUV101010)
    SCALE_DEFAULT(BC_YUV888, BC_YUV888, YUV888_to_YUV888)
    SCALE_DEFAULT(BC_YUV888, BC_YUVA8888, YUV888_to_YUVA8888)
    SCALE_DEFAULT(BC_YUV888, BC_VYU888, YUV888_to_VYU888)
    SCALE_DEFAULT(BC_YUV888, BC_UYVA8888, YUV888_to_UYVA8888)
    SCALE_422_OUT(BC_YUV888, BC_YUV422, YUV888_to_YUV422)
    SCALE_420P(BC_YUV888, BC_YUV420P, YUV888_to_YUV420P_422P)
    SCALE_A420P(BC_YUV888, BC_YUVA420P, YUV888_to_YUVA420P_422P)
    SCALE_422P(BC_YUV888, BC_YUV422P, YUV888_to_YUV420P_422P)
    SCALE_444P(BC_YUV888, BC_YUV444P, YUV888_to_YUV444P)
    SCALE_A444P(BC_YUV888, BC_YUVA444P, YUV888_to_YUVA444P)

    SCALE_DEFAULT(BC_YUVA8888, BC_RGB8, YUVA8888_to_RGB8)
    SCALE_DEFAULT(BC_YUVA8888, BC_BGR565, YUVA8888_to_BGR565)
    SCALE_DEFAULT(BC_YUVA8888, BC_RGB565, YUVA8888_to_RGB565)
    SCALE_DEFAULT(BC_YUVA8888, BC_BGR888, YUVA8888_to_BGR888)
    SCALE_DEFAULT(BC_YUVA8888, BC_BGR8888, YUVA8888_to_BGR8888)
    SCALE_DEFAULT(BC_YUVA8888, BC_RGB888, YUVA8888_to_RGB888)
    SCALE_DEFAULT(BC_YUVA8888, BC_RGBA8888, YUVA8888_to_RGBA8888)
    SCALE_DEFAULT(BC_YUVA8888, BC_ARGB8888, YUVA8888_to_ARGB8888)
    SCALE_DEFAULT(BC_YUVA8888, BC_RGB_FLOAT, YUVA8888_to_RGB_FLOAT)
    SCALE_DEFAULT(BC_YUVA8888, BC_RGBA_FLOAT, YUVA8888_to_RGBA_FLOAT)
    SCALE_DEFAULT(BC_YUVA8888, BC_VYU888, YUVA8888_to_VYU888)
    SCALE_DEFAULT(BC_YUVA8888, BC_YUV888, YUVA8888_to_YUV888)
    SCALE_DEFAULT(BC_YUVA8888, BC_YUVA8888, YUVA8888_to_YUVA8888)
    SCALE_DEFAULT(BC_YUVA8888, BC_UYVA8888, YUVA8888_to_UYVA8888)
    SCALE_DEFAULT(BC_YUVA8888, BC_YUV101010, YUVA8888_to_YUV101010)
    SCALE_422_OUT(BC_YUVA8888, BC_YUV422, YUVA8888_to_YUV422)
    SCALE_420P(BC_YUVA8888, BC_YUV420P, YUVA8888_to_YUV420P_422P)
    SCALE_A420P(BC_YUVA8888, BC_YUVA420P, YUVA8888_to_YUVA420P_422P)
    SCALE_422P(BC_YUVA8888, BC_YUV422P, YUVA8888_to_YUV420P_422P)
    SCALE_444P(BC_YUVA8888, BC_YUV444P, YUVA8888_to_YUV444P)
    SCALE_A444P(BC_YUVA8888, BC_YUVA444P, YUVA8888_to_YUVA444P)

// screencap formats
    SCALE_DEFAULT(BC_BGR8888, BC_RGB888, BGR8888_to_RGB888)
    SCALE_DEFAULT(BC_BGR8888, BC_BGR8888, BGR8888_to_BGR8888)
    SCALE_420P(BC_BGR8888, BC_YUV420P, BGR8888_to_YUV420P_422P)
    SCALE_DEFAULT(BC_BGR888, BC_RGB888, BGR888_to_RGB888)


// quicktime formats which have never been used
    SCALE_DEFAULT(BC_YUV101010, BC_RGB8, YUV101010_to_RGB8)
    SCALE_DEFAULT(BC_YUV101010, BC_BGR565, YUV101010_to_BGR565)
    SCALE_DEFAULT(BC_YUV101010, BC_RGB565, YUV101010_to_RGB565)
    SCALE_DEFAULT(BC_YUV101010, BC_BGR888, YUV101010_to_BGR888)
    SCALE_DEFAULT(BC_YUV101010, BC_BGR8888, YUV101010_to_BGR8888)
    SCALE_DEFAULT(BC_YUV101010, BC_RGB888, YUV101010_to_RGB888)
    SCALE_DEFAULT(BC_YUV101010, BC_RGBA8888, YUV101010_to_RGBA8888)
    SCALE_DEFAULT(BC_YUV101010, BC_YUV888, YUV101010_to_YUV888)
    SCALE_DEFAULT(BC_YUV101010, BC_YUVA8888, YUV101010_to_YUVA8888)
    SCALE_DEFAULT(BC_YUV101010, BC_RGB_FLOAT, YUV101010_to_RGB_FLOAT)
    SCALE_DEFAULT(BC_YUV101010, BC_RGBA_FLOAT, YUV101010_to_RGBA_FLOAT)

    SCALE_DEFAULT(BC_VYU888, BC_RGB8, VYU888_to_RGB8)
    SCALE_DEFAULT(BC_VYU888, BC_BGR565, VYU888_to_BGR565)
    SCALE_DEFAULT(BC_VYU888, BC_RGB565, VYU888_to_RGB565)
    SCALE_DEFAULT(BC_VYU888, BC_BGR888, VYU888_to_BGR888)
    SCALE_DEFAULT(BC_VYU888, BC_BGR8888, VYU888_to_BGR8888)
    SCALE_DEFAULT(BC_VYU888, BC_RGB888, VYU888_to_RGB888)
    SCALE_DEFAULT(BC_VYU888, BC_RGBA8888, VYU888_to_RGBA8888)
    SCALE_DEFAULT(BC_VYU888, BC_YUV888, VYU888_to_YUV888)
    SCALE_DEFAULT(BC_VYU888, BC_YUVA8888, VYU888_to_YUVA8888)
    SCALE_DEFAULT(BC_VYU888, BC_RGB_FLOAT, VYU888_to_RGB_FLOAT)
    SCALE_DEFAULT(BC_VYU888, BC_RGBA_FLOAT, VYU888_to_RGBA_FLOAT)

    SCALE_DEFAULT(BC_UYVA8888, BC_RGB8, UYVA8888_to_RGB8)
    SCALE_DEFAULT(BC_UYVA8888, BC_BGR565, UYVA8888_to_BGR565)
    SCALE_DEFAULT(BC_UYVA8888, BC_RGB565, UYVA8888_to_RGB565)
    SCALE_DEFAULT(BC_UYVA8888, BC_BGR888, UYVA8888_to_BGR888)
    SCALE_DEFAULT(BC_UYVA8888, BC_BGR8888, UYVA8888_to_BGR8888)
    SCALE_DEFAULT(BC_UYVA8888, BC_RGB888, UYVA8888_to_RGB888)
    SCALE_DEFAULT(BC_UYVA8888, BC_RGBA8888, UYVA8888_to_RGBA8888)
    SCALE_DEFAULT(BC_UYVA8888, BC_YUV888, UYVA8888_to_YUV888)
    SCALE_DEFAULT(BC_UYVA8888, BC_YUVA8888, UYVA8888_to_YUVA8888)
    SCALE_DEFAULT(BC_UYVA8888, BC_RGB_FLOAT, UYVA8888_to_RGB_FLOAT)
    SCALE_DEFAULT(BC_UYVA8888, BC_RGBA_FLOAT, UYVA8888_to_RGBA_FLOAT)


    SCALE_DEFAULT(BC_ARGB8888, BC_ARGB8888, ARGB8888_to_ARGB8888)
    SCALE_DEFAULT(BC_ARGB8888, BC_ABGR8888, ARGB8888_to_ABGR8888)
    SCALE_DEFAULT(BC_ARGB8888, BC_RGBA8888, ARGB8888_to_RGBA8888)
    SCALE_DEFAULT(BC_ARGB8888, BC_RGB888, ARGB8888_to_RGB888)
    SCALE_DEFAULT(BC_ARGB8888, BC_BGR8888, ARGB8888_to_BGR8888)

    SCALE_DEFAULT(BC_ABGR8888, BC_ARGB8888, ARGB8888_to_ARGB8888)
    SCALE_DEFAULT(BC_ABGR8888, BC_ABGR8888, ARGB8888_to_ABGR8888)
    SCALE_DEFAULT(BC_ABGR8888, BC_RGBA8888, ARGB8888_to_RGBA8888)
    SCALE_DEFAULT(BC_ABGR8888, BC_RGB888, ARGB8888_to_RGB888)
    SCALE_DEFAULT(BC_ABGR8888, BC_BGR8888, ARGB8888_to_BGR8888)

// capture from webcam, video4linux
    SCALE_422_IN(BC_YUV422, BC_RGB8, YUV422_to_RGB8)
    SCALE_422_IN(BC_YUV422, BC_RGB565, YUV422_to_RGB565)
    SCALE_422_IN(BC_YUV422, BC_BGR565, YUV422_to_BGR565)
    SCALE_422_IN(BC_YUV422, BC_RGB888, YUV422_to_RGB888)
    SCALE_422_IN(BC_YUV422, BC_RGBA8888, YUV422_to_RGBA8888)
    SCALE_422_IN(BC_YUV422, BC_YUV888, YUV422_to_YUV888)
    SCALE_422_IN(BC_YUV422, BC_YUVA8888, YUV422_to_YUVA8888)
    SCALE_422_IN(BC_YUV422, BC_RGB_FLOAT, YUV422_to_RGB_FLOAT)
    SCALE_422_IN(BC_YUV422, BC_RGBA_FLOAT, YUV422_to_RGBA_FLOAT)
    SCALE_422_IN(BC_YUV422, BC_BGR888, YUV422_to_BGR888)
    SCALE_422_IN(BC_YUV422, BC_BGR8888, YUV422_to_BGR8888)
    SCALE_422_IN(BC_YUV422, BC_YUV422, YUV422_to_YUV422)
    SCALE_422_TO_420P(BC_YUV422, BC_YUV420P, YUV422_to_YUV420P)
    SCALE_422_TO_422P(BC_YUV422, BC_YUV422P, YUV422_to_YUV422P)
    
}






