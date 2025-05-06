/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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



#ifndef COLORMODELS2_H
#define COLORMODELS2_H

// Colormodels
#define BC_TRANSPARENCY 0
#define BC_COMPRESSED   1
#define BC_RGB8         2
#define BC_RGB565       3
#define BC_BGR565       4
#define BC_BGR888       5
#define BC_BGR8888      6
// Working bitmaps are packed to simplify processing
#define BC_RGB888       9
#define BC_RGBA8888     10
#define BC_ARGB8888     20
#define BC_ABGR8888     21
#define BC_RGB161616    11
#define BC_RGBA16161616 12
#define BC_YUV888       13
#define BC_YUVA8888     14
#define BC_YUV161616    15
#define BC_YUVA16161616 16
#define BC_YUV422       19
#define BC_A8           22
#define BC_A16          23
#define BC_A_FLOAT      31
#define BC_YUV101010    24
#define BC_VYU888       25
#define BC_UYVA8888     26
#define BC_RGB_FLOAT    29
#define BC_RGBA_FLOAT   30
#define BC_YUV_FLOAT    35 // test.  YUV to RGB float conversion is negligible
// Planar
#define BC_YUV420P      7
#define BC_YUVA420P     36
#define BC_YUV422P      17
#define BC_YUV444P      27
#define BC_YUVA444P     37
#define BC_YUV411P      18
#define BC_YUV9P        28 // Disastrous cmodel from Sorenson
#define BC_YUV420P10LE  32
#define BC_NV12         33 // Y plane, interleaved UV plane
#define BC_NV21         34 // Y plane, interleaved VU plane

// Colormodels purely used by Quicktime are done in Quicktime.

// For communication with the X Server
#define FOURCC_YV12 0x32315659  /* YV12   YUV420P */
#define FOURCC_YUV2 0x32595559  /* YUV2   YUV422 */
#define FOURCC_I420 0x30323449  /* I420   Intel Indeo 4 */

#undef CLAMP
#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))

#undef CLIP
#define CLIP(x, y, z) ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))

// All variables are unsigned
// y -> 24 bits u, v, -> 8 bits r, g, b -> 8 bits
#define YUV_TO_RGB(y, u, v, r, g, b) \
{ \
	(r) = ((y + cmodel_yuv_table->vtor_tab[v]) >> 16); \
	(g) = ((y + cmodel_yuv_table->utog_tab[u] + cmodel_yuv_table->vtog_tab[v]) >> 16); \
	(b) = ((y + cmodel_yuv_table->utob_tab[u]) >> 16); \
	CLAMP(r, 0, 0xff); \
	CLAMP(g, 0, 0xff); \
	CLAMP(b, 0, 0xff); \
}

// y -> 0 - 1 float
// u, v, -> 8 bits
// r, g, b -> float
#define YUV_TO_FLOAT(y, u, v, r, g, b) \
{ \
	(r) = y + cmodel_yuv_table->vtor_float_tab[v]; \
	(g) = y + cmodel_yuv_table->utog_float_tab[u] + cmodel_yuv_table->vtog_float_tab[v]; \
	(b) = y + cmodel_yuv_table->utob_float_tab[u]; \
}

// y -> 0 - 1 float
// u, v, -> 16 bits
// r, g, b -> float
#define YUV16_TO_RGB_FLOAT(y, u, v, r, g, b) \
{ \
	(r) = y + cmodel_yuv_table->v16tor_float_tab[v]; \
	(g) = y + cmodel_yuv_table->u16tog_float_tab[u] + cmodel_yuv_table->v16tog_float_tab[v]; \
	(b) = y + cmodel_yuv_table->u16tob_float_tab[u]; \
}

// y -> 24 bits   u, v-> 16 bits
#define YUV_TO_RGB16(y, u, v, r, g, b) \
{ \
	(r) = ((y + cmodel_yuv_table->vtor_tab16[v]) >> 8); \
	(g) = ((y + cmodel_yuv_table->utog_tab16[u] + cmodel_yuv_table->vtog_tab16[v]) >> 8); \
	(b) = ((y + cmodel_yuv_table->utob_tab16[u]) >> 8); \
	CLAMP(r, 0, 0xffff); \
	CLAMP(g, 0, 0xffff); \
	CLAMP(b, 0, 0xffff); \
}




#define RGB_TO_YUV(y, u, v, r, g, b) \
{ \
	y = ((cmodel_yuv_table->rtoy_tab[r] + cmodel_yuv_table->gtoy_tab[g] + cmodel_yuv_table->btoy_tab[b]) >> 16); \
	u = ((cmodel_yuv_table->rtou_tab[r] + cmodel_yuv_table->gtou_tab[g] + cmodel_yuv_table->btou_tab[b]) >> 16); \
	v = ((cmodel_yuv_table->rtov_tab[r] + cmodel_yuv_table->gtov_tab[g] + cmodel_yuv_table->btov_tab[b]) >> 16); \
	CLAMP(y, 0, 0xff); \
	CLAMP(u, 0, 0xff); \
	CLAMP(v, 0, 0xff); \
}

// r, g, b -> 16 bits
#define RGB_TO_YUV16(y, u, v, r, g, b) \
{ \
	y = ((cmodel_yuv_table->rtoy_tab16[r] + cmodel_yuv_table->gtoy_tab16[g] + cmodel_yuv_table->btoy_tab16[b]) >> 8); \
	u = ((cmodel_yuv_table->rtou_tab16[r] + cmodel_yuv_table->gtou_tab16[g] + cmodel_yuv_table->btou_tab16[b]) >> 8); \
	v = ((cmodel_yuv_table->rtov_tab16[r] + cmodel_yuv_table->gtov_tab16[g] + cmodel_yuv_table->btov_tab16[b]) >> 8); \
	CLAMP(y, 0, 0xffff); \
	CLAMP(u, 0, 0xffff); \
	CLAMP(v, 0, 0xffff); \
}


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int rtoy_tab[0x100], gtoy_tab[0x100], btoy_tab[0x100];
	int rtou_tab[0x100], gtou_tab[0x100], btou_tab[0x100];
	int rtov_tab[0x100], gtov_tab[0x100], btov_tab[0x100];

	int vtor_tab[0x100], vtog_tab[0x100];
	int utog_tab[0x100], utob_tab[0x100];
// Used by init_yuv only
	int *vtor, *vtog, *utog, *utob;

	float vtor_float_tab[0x100], vtog_float_tab[0x100];
	float utog_float_tab[0x100], utob_float_tab[0x100];
	float *vtor_float, *vtog_float, *utog_float, *utob_float;

	int rtoy_tab16[0x10000], gtoy_tab16[0x10000], btoy_tab16[0x10000];
	int rtou_tab16[0x10000], gtou_tab16[0x10000], btou_tab16[0x10000];
	int rtov_tab16[0x10000], gtov_tab16[0x10000], btov_tab16[0x10000];

	int vtor_tab16[0x10000], vtog_tab16[0x10000];
	int utog_tab16[0x10000], utob_tab16[0x10000];
	int *vtor16, *vtog16, *utog16, *utob16;

	float v16tor_float_tab[0x10000], v16tog_float_tab[0x10000];
	float u16tog_float_tab[0x10000], u16tob_float_tab[0x10000];
	float *v16tor_float, *v16tog_float, *u16tog_float, *u16tob_float;
} cmodel_yuv_t;

extern cmodel_yuv_t *cmodel_yuv_table;

void cmodel_init();

int cmodel_calculate_pixelsize(int colormodel);
int cmodel_calculate_datasize(int w, 
    int h, 
    int bytes_per_line, 
    int color_model,
    int with_pad);
int cmodel_calculate_max(int colormodel);
int cmodel_components(int colormodel);
int cmodel_is_yuv(int colormodel);
int cmodel_has_alpha(int colormodel);
int cmodel_is_float(int colormodel);

// Tell when to use plane arguments or row pointer arguments to functions
int cmodel_is_planar(int color_model);
const char* cmodel_to_text(char *string, int cmodel);
int cmodel_from_text(const char *text);


// the big colorspace converter
void cmodel_transfer(unsigned char **output_rows, /* Leave NULL if non existent */
	unsigned char **input_rows,
	unsigned char *out_y_plane, /* Leave NULL if non existent */
	unsigned char *out_u_plane,
	unsigned char *out_v_plane,
	unsigned char *out_a_plane,
	unsigned char *in_y_plane, /* Leave NULL if non existent */
	unsigned char *in_u_plane,
	unsigned char *in_v_plane,
	unsigned char *in_a_plane,
	int in_x,        /* Dimensions to capture from input frame */
	int in_y, 
	int in_w, 
	int in_h,
	int out_x,       /* Dimensions to project on output frame */
	int out_y, 
	int out_w, 
	int out_h,
	int in_colormodel, 
	int out_colormodel,
	int bg_color,         /* When transferring BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
	int in_rowspan,       /* For planar use the luma rowspan */
	int out_rowspan);     /* For planar use the luma rowspan */

int cmodel_bc_to_x(int color_model);

// transfer limited colormodels with alpha checkerboard to BC_BGR8888
void cmodel_transfer_alpha(unsigned char **output_rows, /* Leave NULL if non existent */
	unsigned char **input_rows,
    int in_x,        /* Dimensions to capture from input frame */
	int in_y, 
	int in_w, 
	int in_h,
	int out_x,       /* Dimensions to project on output frame */
	int out_y, 
	int out_w, 
	int out_h,
    int in_colormodel, 
	int out_colormodel,
    int in_rowspan,    // bytes per line
	int out_rowspan,
    int checker_w,
    int checker_h);


#ifdef __cplusplus
}
#endif

#endif
