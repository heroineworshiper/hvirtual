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

#ifndef CMODEL_PRIV_H
#define CMODEL_PRIV_H

// low level function arguments
typedef struct
{
	unsigned char **output_rows;
	unsigned char **input_rows;
	unsigned char *out_y_plane;
	unsigned char *out_u_plane;
	unsigned char *out_v_plane;
	unsigned char *out_a_plane;
	unsigned char *in_y_plane;
	unsigned char *in_u_plane;
	unsigned char *in_v_plane;
	unsigned char *in_a_plane;
	int in_x;
	int in_y;
	int in_w;
	int in_h;
	int out_x;
	int out_y;
	int out_w;
	int out_h;
	int in_colormodel;
	int out_colormodel;
	int bg_color;
	int in_rowspan;
	int out_rowspan;
	int scale;
	int out_pixelsize;
	int in_pixelsize;
	int *row_table;
	int *column_table;
	int bg_r;
	int bg_g;
	int bg_b;
} cmodel_args_t;

typedef struct
{
    int in_colormodel;
    int out_colormodel;
    int has_bg;
    void (*convert)(const cmodel_args_t*);
} cmodel_function_t;

// the big tables
extern cmodel_function_t *cmodel_functions;
extern int total_cmodel_functions;

// initializers
void register_cmodel_function(int in_colormodel,
    int out_colormodel,
    int has_bg,
    void (*convert)(const cmodel_args_t*));



// gcc famously doesn't copy the struct members to local variables so it 
// is faster to use locals.
#define ARGS_TO_LOCALS \
    int i, j; \
	unsigned char **output_rows = args->output_rows; \
	unsigned char **input_rows = args->input_rows; \
	unsigned char *out_y_plane = args->out_y_plane; \
	unsigned char *out_u_plane = args->out_u_plane; \
	unsigned char *out_v_plane = args->out_v_plane; \
	unsigned char *out_a_plane = args->out_a_plane; \
	unsigned char *in_y_plane = args->in_y_plane; \
	unsigned char *in_u_plane = args->in_u_plane; \
	unsigned char *in_v_plane = args->in_v_plane; \
	unsigned char *in_a_plane = args->in_a_plane; \
	int in_x = args->in_x; \
	int in_y = args->in_y; \
	int in_w = args->in_w; \
	int in_h = args->in_h; \
	int out_x = args->out_x; \
	int out_y = args->out_y; \
	int out_w = args->out_w; \
	int out_h = args->out_h; \
	int in_colormodel = args->in_colormodel; \
	int out_colormodel = args->out_colormodel; \
	int bg_color = args->bg_color; \
	int in_rowspan = args->in_rowspan; \
	int out_rowspan = args->out_rowspan; \
	int scale = args->scale; \
	int out_pixelsize = args->out_pixelsize; \
	int in_pixelsize = args->in_pixelsize; \
	int *row_table = args->row_table; \
	int *column_table = args->column_table; \
	int bg_r = args->bg_r; \
	int bg_g = args->bg_g; \
	int bg_b = args->bg_b;




#define DEFAULT_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_row = input_rows[row_table[i]]; \
		int bit_counter = 7; \
		for(j = 0; j < out_w; j++) \
		{

#define DEFAULT_TAIL \
		} \
	}

#define YUV420P_OUT_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *input_row = input_rows[row_table[i]]; \
		unsigned char *output_y = out_y_plane + i * out_rowspan + out_x; \
		unsigned char *output_u = out_u_plane + (i / 2) * (out_rowspan / 2) + (out_x / 2); \
		unsigned char *output_v = out_v_plane + (i / 2) * (out_rowspan / 2) + (out_x / 2); \
		unsigned char *output_a = out_a_plane + i * out_rowspan + out_x; \
		for(j = 0; j < out_w; j++) \
		{

#define YUV422P_OUT_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *input_row = input_rows[row_table[i]]; \
		unsigned char *output_y = out_y_plane + i * out_rowspan + out_x; \
		unsigned char *output_u = out_u_plane + i * (out_rowspan / 2) + (out_x / 2); \
		unsigned char *output_v = out_v_plane + i * (out_rowspan / 2) + (out_x / 2); \
		for(j = 0; j < out_w; j++) \
		{

#define YUV444P_OUT_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *input_row = input_rows[row_table[i]]; \
		unsigned char *output_y = out_y_plane + i * out_rowspan + out_x; \
		unsigned char *output_u = out_u_plane + i * out_rowspan + out_x; \
		unsigned char *output_v = out_v_plane + i * out_rowspan + out_x; \
		unsigned char *output_a = out_a_plane + i * out_rowspan + out_x; \
		for(j = 0; j < out_w; j++) \
		{




#define WRITE_YUV101010(y, u, v) \
{ \
	uint32_t output_i = ((y & 0xffc0) << 16) | \
		((u & 0xffc0) << 6) | \
		((v & 0xffc0) >> 4); \
	*(*output)++ = (output_i & 0xff); \
	*(*output)++ = (output_i & 0xff00) >> 8; \
	*(*output)++ = (output_i & 0xff0000) >> 16; \
	*(*output)++ = (output_i & 0xff000000) >> 24; \
}


#endif




