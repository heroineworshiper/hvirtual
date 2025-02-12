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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


cmodel_yuv_t *cmodel_yuv_table = 0;
cmodel_function_t *cmodel_functions = 0;
int total_cmodel_functions = 0;
static pthread_mutex_t cmodel_lock = PTHREAD_MUTEX_INITIALIZER;

// Compression coefficients straight out of jpeglib
#define R_TO_Y    0.29900
#define G_TO_Y    0.58700
#define B_TO_Y    0.11400

#define R_TO_U    -0.16874
#define G_TO_U    -0.33126
#define B_TO_U    0.50000

#define R_TO_V    0.50000
#define G_TO_V    -0.41869
#define B_TO_V    -0.08131

// Decompression coefficients straight out of jpeglib
#define V_TO_R    1.40200
#define V_TO_G    -0.71414

#define U_TO_G    -0.34414
#define U_TO_B    1.77200


void cmodel_init_default();
void cmodel_init_planar();
void cmodel_init_float();


void cmodel_init_yuv(cmodel_yuv_t *yuv_table)
{
	int i;

/* compression */
	for(i = 0; i < 0x100; i++)
	{
		yuv_table->rtoy_tab[i] = (int)(R_TO_Y * 0x10000 * i);
		yuv_table->rtou_tab[i] = (int)(R_TO_U * 0x10000 * i);
		yuv_table->rtov_tab[i] = (int)(R_TO_V * 0x10000 * i);

		yuv_table->gtoy_tab[i] = (int)(G_TO_Y * 0x10000 * i);
		yuv_table->gtou_tab[i] = (int)(G_TO_U * 0x10000 * i);
		yuv_table->gtov_tab[i] = (int)(G_TO_V * 0x10000 * i);

		yuv_table->btoy_tab[i] = (int)(B_TO_Y * 0x10000 * i);
		yuv_table->btou_tab[i] = (int)(B_TO_U * 0x10000 * i) + 0x800000;
		yuv_table->btov_tab[i] = (int)(B_TO_V * 0x10000 * i) + 0x800000;
	}

/* compression */
	for(i = 0; i < 0x10000; i++)
	{
		yuv_table->rtoy_tab16[i] = (int)(R_TO_Y * 0x100 * i);
		yuv_table->rtou_tab16[i] = (int)(R_TO_U * 0x100 * i);
		yuv_table->rtov_tab16[i] = (int)(R_TO_V * 0x100 * i);

		yuv_table->gtoy_tab16[i] = (int)(G_TO_Y * 0x100 * i);
		yuv_table->gtou_tab16[i] = (int)(G_TO_U * 0x100 * i);
		yuv_table->gtov_tab16[i] = (int)(G_TO_V * 0x100 * i);

		yuv_table->btoy_tab16[i] = (int)(B_TO_Y * 0x100 * i);
		yuv_table->btou_tab16[i] = (int)(B_TO_U * 0x100 * i) + 0x800000;
		yuv_table->btov_tab16[i] = (int)(B_TO_V * 0x100 * i) + 0x800000;
	}




/* decompression */
	yuv_table->vtor = &(yuv_table->vtor_tab[0x80]);
	yuv_table->vtog = &(yuv_table->vtog_tab[0x80]);
	yuv_table->utog = &(yuv_table->utog_tab[0x80]);
	yuv_table->utob = &(yuv_table->utob_tab[0x80]);
	for(i = -0x80; i < 0x80; i++)
	{
		yuv_table->vtor[i] = (int)(V_TO_R * 0x10000 * i);
		yuv_table->vtog[i] = (int)(V_TO_G * 0x10000 * i);

		yuv_table->utog[i] = (int)(U_TO_G * 0x10000 * i);
		yuv_table->utob[i] = (int)(U_TO_B * 0x10000 * i);
	}


/* decompression */
	yuv_table->vtor_float = &(yuv_table->vtor_float_tab[0x80]);
	yuv_table->vtog_float = &(yuv_table->vtog_float_tab[0x80]);
	yuv_table->utog_float = &(yuv_table->utog_float_tab[0x80]);
	yuv_table->utob_float = &(yuv_table->utob_float_tab[0x80]);
	for(i = -0x80; i < 0x80; i++)
	{
		yuv_table->vtor_float[i] = V_TO_R * i / 0xff;
		yuv_table->vtog_float[i] = V_TO_G * i / 0xff;

		yuv_table->utog_float[i] = U_TO_G * i / 0xff;
		yuv_table->utob_float[i] = U_TO_B * i / 0xff;
	}


/* decompression */
	yuv_table->vtor16 = &(yuv_table->vtor_tab16[0x8000]);
	yuv_table->vtog16 = &(yuv_table->vtog_tab16[0x8000]);
	yuv_table->utog16 = &(yuv_table->utog_tab16[0x8000]);
	yuv_table->utob16 = &(yuv_table->utob_tab16[0x8000]);
	for(i = -0x8000; i < 0x8000; i++)
	{
		yuv_table->vtor16[i] = (int)(V_TO_R * 0x100 * i);
		yuv_table->vtog16[i] = (int)(V_TO_G * 0x100 * i);

		yuv_table->utog16[i] = (int)(U_TO_G * 0x100 * i);
		yuv_table->utob16[i] = (int)(U_TO_B * 0x100 * i);
	}


/* decompression */
	yuv_table->v16tor_float = &(yuv_table->v16tor_float_tab[0x8000]);
	yuv_table->v16tog_float = &(yuv_table->v16tog_float_tab[0x8000]);
	yuv_table->u16tog_float = &(yuv_table->u16tog_float_tab[0x8000]);
	yuv_table->u16tob_float = &(yuv_table->u16tob_float_tab[0x8000]);
	for(i = -0x8000; i < 0x8000; i++)
	{
		yuv_table->v16tor_float[i] = V_TO_R * i / 0xffff;
		yuv_table->v16tog_float[i] = V_TO_G * i / 0xffff;

		yuv_table->u16tog_float[i] = U_TO_G * i / 0xffff;
		yuv_table->u16tob_float[i] = U_TO_B * i / 0xffff;
	}
}

void register_cmodel_function(int in_colormodel,
    int out_colormodel,
    int has_bg,
    void (*convert)(const cmodel_args_t*))
{
    cmodel_functions = (cmodel_function_t*)realloc(cmodel_functions,
        sizeof(cmodel_function_t) * (total_cmodel_functions + 1));
    cmodel_functions[total_cmodel_functions].in_colormodel = in_colormodel;
    cmodel_functions[total_cmodel_functions].out_colormodel = out_colormodel;
    cmodel_functions[total_cmodel_functions].has_bg = has_bg;
    cmodel_functions[total_cmodel_functions].convert = convert;
    total_cmodel_functions++;
}


void cmodel_init()
{
    pthread_mutex_lock(&cmodel_lock);
	if(cmodel_yuv_table == 0)
	{
		cmodel_yuv_table = calloc(1, sizeof(cmodel_yuv_t));
		cmodel_init_yuv(cmodel_yuv_table);
	}

    if(!cmodel_functions)
    {    
// init function tables
        cmodel_init_default();
        cmodel_init_planar();
        cmodel_init_float();
    }
    pthread_mutex_unlock(&cmodel_lock);
}



int cmodel_is_planar(int colormodel)
{
	switch(colormodel)
	{
		case BC_YUV420P10LE:
		case BC_YUVA420P:
		case BC_YUV420P:
		case BC_YUV422P:
		case BC_YUVA444P:
		case BC_YUV444P:
		case BC_YUV411P:
		case BC_YUV9P:
			return 1;
			break;
	}
	return 0;
}

int cmodel_components(int colormodel)
{
	switch(colormodel)
	{
		case BC_A8:           return 1; break;
		case BC_A16:          return 1; break;
		case BC_A_FLOAT:      return 1; break;
		case BC_RGB888:       return 3; break;
		case BC_RGBA8888:     return 4; break;
		case BC_RGB161616:    return 3; break;
		case BC_RGBA16161616: return 4; break;
		case BC_YUV888:       return 3; break;
		case BC_YUVA8888:     return 4; break;
		case BC_YUV161616:    return 3; break;
		case BC_YUVA16161616: return 4; break;
		case BC_YUV101010:    return 3; break;
		case BC_RGB_FLOAT:    return 3; break;
		case BC_YUV_FLOAT:    return 3; break;
		case BC_RGBA_FLOAT:   return 4; break;
		case BC_YUV420P10LE:  return 3; break;
		case BC_YUV420P:      return 3; break;
		case BC_YUVA420P:     return 4; break;
		case BC_YUV422P:      return 3; break;
		case BC_YUV444P:      return 3; break;
		case BC_YUVA444P:     return 4; break;
		case BC_YUV9P:        return 3; break;
	}
    return 3;
}

int cmodel_calculate_pixelsize(int colormodel)
{
	switch(colormodel)
	{
		case BC_A8:           return 1; break;
		case BC_A16:          return 2; break;
		case BC_A_FLOAT:      return 4; break;
		case BC_TRANSPARENCY: return 1; break;
		case BC_COMPRESSED:   return 1; break;
		case BC_RGB8:         return 1; break;
		case BC_RGB565:       return 2; break;
		case BC_BGR565:       return 2; break;
		case BC_BGR888:       return 3; break;
		case BC_BGR8888:      return 4; break;
// Working bitmaps are packed to simplify processing
		case BC_RGB888:       return 3; break;
		case BC_ARGB8888:     return 4; break;
		case BC_ABGR8888:     return 4; break;
		case BC_RGBA8888:     return 4; break;
		case BC_RGB161616:    return 6; break;
		case BC_RGBA16161616: return 8; break;
		case BC_YUV888:       return 3; break;
		case BC_YUVA8888:     return 4; break;
		case BC_YUV161616:    return 6; break;
		case BC_YUVA16161616: return 8; break;
		case BC_YUV101010:    return 4; break;
		case BC_VYU888:       return 3; break;
		case BC_UYVA8888:     return 4; break;
		case BC_YUV_FLOAT:    return 12; break;
		case BC_RGB_FLOAT:    return 12; break;
		case BC_RGBA_FLOAT:   return 16; break;
// Planar
		case BC_YUV420P:      return 1; break;
		case BC_YUVA420P:     return 1; break;
		case BC_YUV422P:      return 1; break;
		case BC_YUV444P:      return 1; break;
		case BC_YUVA444P:     return 1; break;
		case BC_YUV422:       return 2; break;
		case BC_YUV411P:      return 1; break;
		case BC_YUV9P:        return 1; break;
		case BC_YUV420P10LE:  return 2; break;
	}
	return 0;
}

int cmodel_calculate_max(int colormodel)
{
	switch(colormodel)
	{
// Working bitmaps are packed to simplify processing
		case BC_A8:           return 0xff; break;
		case BC_A16:          return 0xffff; break;
		case BC_A_FLOAT:      return 1; break;
		case BC_RGB888:       return 0xff; break;
		case BC_RGBA8888:     return 0xff; break;
		case BC_RGB161616:    return 0xffff; break;
		case BC_RGBA16161616: return 0xffff; break;
		case BC_YUV888:       return 0xff; break;
		case BC_YUVA8888:     return 0xff; break;
		case BC_YUV161616:    return 0xffff; break;
		case BC_YUVA16161616: return 0xffff; break;
		case BC_YUV_FLOAT:    return 1; break;
		case BC_RGB_FLOAT:    return 1; break;
		case BC_RGBA_FLOAT:   return 1; break;
	}
	return 0;
}

int cmodel_calculate_datasize(int w, 
    int h, 
    int bytes_per_line, 
    int color_model,
    int with_pad)
{
	if(bytes_per_line < 0) bytes_per_line = w * 
		cmodel_calculate_pixelsize(color_model);
    int pad = 0;
    if(with_pad)
    {
// Pad the planar models with an extra row of RGBA 8 bit texture width
// for glTexSubImage2D to get all the rows.  
// Since the texture width is aligned, add 2 rows.
        pad = w * 4 * 2;
// Ffmpeg expects 1 more row of bytes_per_line for odd numbered heights
        if(bytes_per_line > pad) pad = bytes_per_line;
    }

    int result = 0;
	switch(color_model)
	{
		case BC_YUV420P:
		case BC_YUV411P:
        case BC_NV12:
			result = bytes_per_line * h + 
                2 * (bytes_per_line / 2) * (h / 2);
			break;

		case BC_YUVA420P:
			result = bytes_per_line * h * 2 + 
                2 * (bytes_per_line / 2) * (h / 2);
            break;

		case BC_YUV422P:
			result = bytes_per_line * h * 2;
			break;

		case BC_YUV444P:
			result = bytes_per_line * h * 3;
			break;

		case BC_YUVA444P:
			result = bytes_per_line * h * 4;
			break;

        case BC_YUV420P10LE:
            result = h * bytes_per_line  + 
                (bytes_per_line / 2) * (h / 2) * 2;
            break;

        case BC_YUV9P:
            result = h * bytes_per_line + 
                (bytes_per_line / 4) * (h / 4) * 2;
            break;

		default:
			result = h * bytes_per_line;
			break;
	}
    
    return result + pad;
}


static void get_scale_tables(int **column_table, 
	int **row_table, 
	int in_x1, 
	int in_y1, 
	int in_x2, 
	int in_y2,
	int out_x1, 
	int out_y1, 
	int out_x2, 
	int out_y2)
{
	int y_out, i;
	float w_in = in_x2 - in_x1;
	float h_in = in_y2 - in_y1;
	int w_out = out_x2 - out_x1;
	int h_out = out_y2 - out_y1;

	float hscale = w_in / w_out;
	float vscale = h_in / h_out;

/* + 1 so we don't overflow when calculating in advance */
	(*column_table) = malloc(sizeof(int) * (w_out + 1));
	(*row_table) = malloc(sizeof(int) * h_out);
	for(i = 0; i < w_out; i++)
	{
		(*column_table)[i] = (int)(hscale * i) + in_x1;
	}

	for(i = 0; i < h_out; i++)
	{
		(*row_table)[i] = (int)(vscale * i) + in_y1;
//printf("get_scale_tables %d %d\n", (*row_table)[i], i);
	}
}

void cmodel_transfer(unsigned char **output_rows, 
	unsigned char **input_rows,
	unsigned char *out_y_plane,
	unsigned char *out_u_plane,
	unsigned char *out_v_plane,
	unsigned char *out_a_plane,
	unsigned char *in_y_plane,
	unsigned char *in_u_plane,
	unsigned char *in_v_plane,
	unsigned char *in_a_plane,
	int in_x, 
	int in_y, 
	int in_w, 
	int in_h,
	int out_x, 
	int out_y, 
	int out_w, 
	int out_h,
	int in_colormodel, 
	int out_colormodel,
	int bg_color,
	int in_rowspan,
	int out_rowspan)
{
	int *column_table;
	int *row_table;
	int scale;
	int bg_r, bg_g, bg_b;
	int in_pixelsize = cmodel_calculate_pixelsize(in_colormodel);
	int out_pixelsize = cmodel_calculate_pixelsize(out_colormodel);
    int i;

	bg_r = (bg_color & 0xff0000) >> 16;
	bg_g = (bg_color & 0xff00) >> 8;
	bg_b = (bg_color & 0xff);

// Initialize tables
    cmodel_init();

// Get scaling
	scale = (out_w != in_w) || (in_x != 0);
	get_scale_tables(&column_table, &row_table, 
		in_x, in_y, in_x + in_w, in_y + in_h,
		out_x, out_y, out_x + out_w, out_y + out_h);

// printf("cmodel_transfer %d %p %p\n", __LINE__, column_table, row_table);
// printf("cmodel_transfer %d %d->%d out_a_plane=%p\n", 
// __LINE__,
// in_colormodel, 
// out_colormodel,
// out_a_plane);
// printf("cmodel_transfer %d %d %d %d,%d %d,%d %d,%d %d,%d\n", 
// __LINE__,
// in_colormodel, 
// out_colormodel, 
// out_x, 
// out_y, 
// out_w, 
// out_h, 
// in_x, 
// in_y, 
// in_w, 
// in_h);


    cmodel_args_t args;
    int got_it = 0;
	args.output_rows = output_rows;
	args.input_rows = input_rows;
	args.out_y_plane = out_y_plane;
	args.out_u_plane = out_u_plane;
	args.out_v_plane = out_v_plane;
	args.out_a_plane = out_a_plane;
	args.in_y_plane = in_y_plane;
	args.in_u_plane = in_u_plane;
	args.in_v_plane = in_v_plane;
	args.in_a_plane = in_a_plane;
	args.in_x = in_x;
	args.in_y = in_y;
	args.in_w = in_w;
	args.in_h = in_h;
	args.out_x = out_x;
	args.out_y = out_y;
	args.out_w = out_w;
	args.out_h = out_h;
	args.in_colormodel = in_colormodel;
	args.out_colormodel = out_colormodel;
	args.bg_color = bg_color;
	args.in_rowspan = in_rowspan;
	args.out_rowspan = out_rowspan;
	args.scale = scale;
	args.out_pixelsize = out_pixelsize;
	args.in_pixelsize = in_pixelsize;
	args.row_table = row_table;
	args.column_table = column_table;
	args.bg_r = bg_r;
	args.bg_g = bg_g;
	args.bg_b = bg_b;

// ignore has_bg in certain cases
    if(out_colormodel == BC_TRANSPARENCY ||
        !cmodel_has_alpha(in_colormodel))
    {
        args.bg_color = 0;
    }

    for(i == 0; i < total_cmodel_functions; i++)
    {
        if(cmodel_functions[i].in_colormodel == in_colormodel &&
            cmodel_functions[i].out_colormodel == out_colormodel &&
                ((cmodel_functions[i].has_bg && args.bg_color) || 
                (!cmodel_functions[i].has_bg && !args.bg_color)))
        {
            cmodel_functions[i].convert(&args);
            got_it = 1;
            break;
        }
    }

    if(!got_it)
    {
        printf("cmodel_transfer %d unsupported conversion %d -> %d bg_color=%08x\n",
            __LINE__,
            in_colormodel,
            out_colormodel,
            bg_color);
        printf("cmodel_transfer %d out=%d,%d %d,%d in=%d,%d %d,%d\n", 
            __LINE__,
            out_x, 
            out_y, 
            out_w, 
            out_h, 
            in_x, 
            in_y, 
            in_w, 
            in_h);
    }



	free(column_table);
	free(row_table);
//printf("cmodel_transfer %d\n", 
//__LINE__);
}

int cmodel_bc_to_x(int color_model)
{
	switch(color_model)
	{
		case BC_YUV420P:
			return FOURCC_YV12;
			break;
		case BC_YUV422:
			return FOURCC_YUV2;
			break;
	}
	return -1;
}

const char* cmodel_to_text(char *string, int cmodel)
{
	switch(cmodel)
	{
        case BC_YUVA420P: strcpy(string, "YUVA420 Planar");   break;
        case BC_YUV420P: strcpy(string, "YUV420 Planar");   break;
        case BC_YUV422P: strcpy(string, "YUV422 Planar");   break;
        case BC_YUVA444P: strcpy(string, "YUVA444 Planar");   break;
        case BC_YUV444P: strcpy(string, "YUV444 Planar");   break;
		case BC_RGB888:       strcpy(string, "RGB-8 Bit");   break;
		case BC_BGR8888:       strcpy(string, "BGRX-8 Bit");   break;
		case BC_RGBA8888:     strcpy(string, "RGBA-8 Bit");  break;
		case BC_YUV888:       strcpy(string, "YUV-8 Bit");   break;
		case BC_YUVA8888:     strcpy(string, "YUVA-8 Bit");  break;
		case BC_YUV_FLOAT:    strcpy(string, "YUV-FLOAT");   break;
		case BC_RGB_FLOAT:    strcpy(string, "RGB-FLOAT");   break;
		case BC_RGBA_FLOAT:   strcpy(string, "RGBA-FLOAT");  break;
		default: strcpy(string, "Unknown"); break;
	}
    return string;
}

int cmodel_from_text(const char *text)
{
	if(!strcasecmp(text, "YUV420 Planar")) return BC_YUV420P;
	if(!strcasecmp(text, "YUVA420 Planar")) return BC_YUVA420P;
	if(!strcasecmp(text, "YUV422 Planar")) return BC_YUV422P;
	if(!strcasecmp(text, "YUV444 Planar")) return BC_YUV444P;
	if(!strcasecmp(text, "YUVA444 Planar")) return BC_YUVA444P;
	if(!strcasecmp(text, "RGB-8 Bit"))   return BC_RGB888;
	if(!strcasecmp(text, "RGBA-8 Bit"))  return BC_RGBA8888;
	if(!strcasecmp(text, "YUV-FLOAT"))   return BC_YUV_FLOAT;
	if(!strcasecmp(text, "RGB-FLOAT"))   return BC_RGB_FLOAT;
	if(!strcasecmp(text, "RGBA-FLOAT"))  return BC_RGBA_FLOAT;
	if(!strcasecmp(text, "YUV-8 Bit"))   return BC_YUV888;
	if(!strcasecmp(text, "YUVA-8 Bit"))  return BC_YUVA8888;
	return BC_RGB888;
}

int cmodel_has_alpha(int colormodel)
{
	switch(colormodel)
	{
		case BC_YUVA8888:
		case BC_RGBA8888:
		case BC_RGBA_FLOAT:
			return 1;
	}
	return 0;
}

int cmodel_is_float(int colormodel)
{
	switch(colormodel)
	{
		case BC_RGB_FLOAT:
		case BC_RGBA_FLOAT:
		case BC_YUV_FLOAT:
			return 1;
	}
	return 0;
}

int cmodel_is_yuv(int colormodel)
{
	switch(colormodel)
	{
		case BC_YUV888:
		case BC_YUVA8888:
		case BC_YUV161616:
		case BC_YUVA16161616:
		case BC_YUV422:
		case BC_YUVA420P:
		case BC_YUV420P:
		case BC_YUV422P:
		case BC_YUVA444P:
		case BC_YUV444P:
		case BC_YUV411P:
		case BC_YUV_FLOAT:
			return 1;
			break;
		
		default:
			return 0;
			break;
	}
}



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
        int checker_h)
{
	int *column_table;
	int *row_table;
	get_scale_tables(&column_table, 
        &row_table, 
		in_x, 
        in_y, 
        in_x + in_w, 
        in_y + in_h,
		out_x, 
        out_y, 
        out_x + out_w, 
        out_y + out_h);


#define HEAD(type, temp, max) \
    for(int i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = (unsigned char*)output_rows[i + out_y] + out_x * 4; \
		type *input_row = (type*)input_rows[row_table[i]]; \
        int color1 = (i / checker_h) % 2; \
 \
        for(int j = 0; j < out_w; j++) \
		{ \
            type *input = input_row + column_table[j] * 4; \
            int color2 = (color1 + j / checker_w) % 2; \
            temp bg_r, bg_g, bg_b; \
	        temp a, anti_a; \
	        a = input[3]; \
	        anti_a = max - a; \
 \
            if(color2) \
            { \
                bg_r = bg_g = bg_b = (temp)(0.6 * max); \
            } \
            else \
            { \
                bg_r = bg_g = bg_b = (temp)(0.4 * max); \
            }

#define TAIL \
	        *output_row++ = (unsigned char)b; \
	        *output_row++ = (unsigned char)g; \
	        *output_row++ = (unsigned char)r; \
	        output_row++; \
        } \
    }



    switch(in_colormodel)
    {
        case BC_RGBA8888:
// printf("cmodel_transfer_alpha %d %d -> %d\n",
// __LINE__,
// in_colormodel,
// out_colormodel);
            HEAD(unsigned char, unsigned int, 0xff)
	        int r = ((int)input[0] * a + bg_r * anti_a) / 255;
	        int g = ((int)input[1] * a + bg_g * anti_a) / 255;
	        int b = ((int)input[2] * a + bg_b * anti_a) / 255;
            TAIL
            break;
        
        case BC_RGBA_FLOAT:
            HEAD(float, float, 1.0)
            float r = 255 * (input[0] * a + bg_r * anti_a);
            float g = 255 * (input[1] * a + bg_g * anti_a);
            float b = 255 * (input[2] * a + bg_b * anti_a);
            CLAMP(r, 0, 255);
            CLAMP(g, 0, 255);
            CLAMP(b, 0, 255);
            TAIL
            break;
        
        case BC_YUVA8888:
            HEAD(unsigned char, unsigned int, 0xff)
            int y = (input[0] << 16) | (input[0] << 8) | input[0];
            int u = input[1];
            int v = input[2];
            int r, g, b;
            YUV_TO_RGB(y, u, v, r, g, b)
	        r = ((unsigned int)r * a + bg_r * anti_a) / 255;
	        g = ((unsigned int)g * a + bg_g * anti_a) / 255;
	        b = ((unsigned int)b * a + bg_b * anti_a) / 255;
            TAIL
            break;

        default:
            printf("cmodel_transfer_alpha %d unsupported transfer %d -> %d\n",
                __LINE__,
                in_colormodel,
                out_colormodel);
            break;
    }
}





