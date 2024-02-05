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


#define YUV420P_TO_PACKED_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_y = in_y_plane + row_table[i] * in_rowspan; \
		unsigned char *input_u = in_u_plane + (row_table[i] / 2) * (in_rowspan / 2); \
		unsigned char *input_v = in_v_plane + (row_table[i] / 2) * (in_rowspan / 2); \
		for(j = 0; j < out_w; j++) \
		{

#define PLANAR_TAIL \
		} \
	}

#define SCALE_YUV420P_TO_PACKED(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV420P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + column_table[j], \
                input_u + column_table[j] / 2, \
                input_v + column_table[j] / 2); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV420P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + j, \
                input_u + j / 2, \
                input_v + j / 2); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}



#define YUV411P_TO_PACKED_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_y = in_y_plane + row_table[i] * in_rowspan; \
		unsigned char *input_u = in_u_plane + row_table[i] * (in_rowspan / 4); \
		unsigned char *input_v = in_v_plane + row_table[i] * (in_rowspan / 4); \
		for(j = 0; j < out_w; j++) \
		{

#define PLANAR_TAIL \
		} \
	}

#define SCALE_YUV411P_TO_PACKED(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV411P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + column_table[j], \
                input_u + column_table[j] / 4, \
                input_v + column_table[j] / 4); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV411P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + j, \
                input_u + j / 4, \
                input_v + j / 4); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}

#define YUV422P_TO_PACKED_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_y = in_y_plane + row_table[i] * in_rowspan; \
		unsigned char *input_u = in_u_plane + row_table[i] * (in_rowspan / 2); \
		unsigned char *input_v = in_v_plane + row_table[i] * (in_rowspan / 2); \
		for(j = 0; j < out_w; j++) \
		{

#define SCALE_YUV422P_TO_PACKED(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV422P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + column_table[j], \
                input_u + column_table[j] / 2, \
                input_v + column_table[j] / 2); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV422P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + j, \
                input_u + j / 2, \
                input_v + j / 2); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}


#define YUV444P_TO_PACKED_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_y = in_y_plane + row_table[i] * in_rowspan; \
		unsigned char *input_u = in_u_plane + row_table[i] * in_rowspan; \
		unsigned char *input_v = in_v_plane + row_table[i] * in_rowspan; \
		for(j = 0; j < out_w; j++) \
		{

#define SCALE_YUV444P_TO_PACKED(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV444P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + column_table[j], \
                input_u + column_table[j], \
                input_v + column_table[j]); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV444P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + j, \
                input_u + j, \
                input_v + j); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}


// pack YUV422
#define SCALE_YUV420P_YUV422(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV420P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + column_table[j], \
                input_u + column_table[j] / 2, \
                input_v + column_table[j] / 2, \
                j); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV420P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + j, \
                input_u + j / 2, \
                input_v + j / 2, \
                j); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}


#define SCALE_YUV420P10LE(in, out, function) \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV420P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + column_table[j] * 2, \
                input_u + (column_table[j] & ~0x1), \
                input_v + (column_table[j] & ~0x1)); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV420P_TO_PACKED_HEAD \
            function(&output_row, \
                input_y + j * 2, \
                input_u + (j & ~0x1), \
                input_v + (j & ~0x1)); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_);

#define NV12_IN_HEAD \
    for(i = 0; i < out_h; i++) \
    { \
        unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
        unsigned char *input_y = in_y_plane + row_table[i] * in_rowspan; \
        unsigned char *input_uv = in_u_plane + (row_table[i] / 2) * in_rowspan; \
        for(j = 0; j < out_w; j++) \
        {

#define SCALE_NV12(in, out, function) \
    void function##__(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            NV12_IN_HEAD \
            function(&output_row, \
                input_y + column_table[j], \
                input_uv + (column_table[j] / 2) * 2, \
                input_uv + (column_table[j] / 2) * 2 + 1); \
            PLANAR_TAIL \
        } \
        else \
        { \
            NV12_IN_HEAD \
            function(&output_row, \
                input_y + j, \
                input_uv + (j / 2) * 2, \
                input_uv + (j / 2) * 2 + 1); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##__);



#define YUV420P_TO_YUV420_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_y = out_y_plane + i * out_rowspan; \
		unsigned char *output_u = out_u_plane + i / 2 * out_rowspan / 2; \
		unsigned char *output_v = out_v_plane + i / 2 * out_rowspan / 2; \
		unsigned char *input_y = in_y_plane + row_table[i] * in_rowspan; \
		unsigned char *input_u = in_u_plane + row_table[i] / 2 * in_rowspan / 2; \
		unsigned char *input_v = in_v_plane + row_table[i] / 2 * in_rowspan / 2; \
		for(j = 0; j < out_w; j++) \
		{



#define SCALE_YUV420P_TO_YUV420P(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV420P_TO_YUV420_HEAD \
            function( \
                input_y + column_table[j], \
                input_u + column_table[j] / 2, \
                input_v + column_table[j] / 2, \
                output_y, \
                output_u, \
                output_v, \
                j); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV420P_TO_YUV420_HEAD \
            function( \
                input_y + j, \
                input_u + j / 2, \
                input_v + j / 2, \
                output_y, \
                output_u, \
                output_v, \
                j); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}


#define YUV420P_TO_YUV422P_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_y = out_y_plane + i * out_rowspan; \
		unsigned char *output_u = out_u_plane + i * out_rowspan / 2; \
		unsigned char *output_v = out_v_plane + i * out_rowspan / 2; \
		unsigned char *input_y = in_y_plane + row_table[i] * in_rowspan; \
		unsigned char *input_u = in_u_plane + row_table[i] / 2 * in_rowspan / 2; \
		unsigned char *input_v = in_v_plane + row_table[i] / 2 * in_rowspan / 2; \
		for(j = 0; j < out_w; j++) \
		{

#define SCALE_YUV420P_TO_YUV422P(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV420P_TO_YUV422P_HEAD \
            function( \
                input_y + column_table[j], \
                input_u + column_table[j] / 2, \
                input_v + column_table[j] / 2, \
                output_y, \
                output_u, \
                output_v, \
                j); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV420P_TO_YUV422P_HEAD \
            function( \
                input_y + j, \
                input_u + j / 2, \
                input_v + j / 2, \
                output_y, \
                output_u, \
                output_v, \
                j); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}


#define YUV420P_TO_YUV444P_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_y = out_y_plane + i * out_rowspan; \
		unsigned char *output_u = out_u_plane + i * out_rowspan; \
		unsigned char *output_v = out_v_plane + i * out_rowspan; \
		unsigned char *input_y = in_y_plane + row_table[i] * in_rowspan; \
		unsigned char *input_u = in_u_plane + row_table[i] / 2 * in_rowspan / 2; \
		unsigned char *input_v = in_v_plane + row_table[i] / 2 * in_rowspan / 2; \
		for(j = 0; j < out_w; j++) \
		{

#define SCALE_YUV420P_TO_YUV444P(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV420P_TO_YUV444P_HEAD \
            function( \
                input_y + column_table[j], \
                input_u + column_table[j] / 2, \
                input_v + column_table[j] / 2, \
                output_y, \
                output_u, \
                output_v, \
                j); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV420P_TO_YUV444P_HEAD \
            function( \
                input_y + j, \
                input_u + j / 2, \
                input_v + j / 2, \
                output_y, \
                output_u, \
                output_v, \
                j); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}


#define YUV9P_HEAD \
	for(i = 0; i < out_h; i++) \
	{ \
		unsigned char *output_row = output_rows[i + out_y] + out_x * out_pixelsize; \
		unsigned char *input_y = in_y_plane + row_table[i] * in_rowspan; \
		unsigned char *input_u = in_u_plane + (row_table[i] / 4) * (in_rowspan / 4); \
		unsigned char *input_v = in_v_plane + (row_table[i] / 4) * (in_rowspan / 4); \
		for(j = 0; j < out_w; j++) \
		{


#define SCALE_YUV9P(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV9P_HEAD \
            function(&output_row, \
                input_y + column_table[j], \
                input_u + column_table[j] / 4, \
                input_v + column_table[j] / 4); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV9P_HEAD \
            function(&output_row, \
                input_y + j, \
                input_u + j / 4, \
                input_v + j / 4); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}


// pack YUV422
#define SCALE_YUV9P_TO_YUV422(in, out, function) \
{ \
    void function##_(const cmodel_args_t *args) \
    { \
        ARGS_TO_LOCALS \
        if(scale) \
        { \
            YUV9P_HEAD \
            function(&output_row, \
                input_y + column_table[j], \
                input_u + column_table[j] / 4, \
                input_v + column_table[j] / 4, \
                j); \
            PLANAR_TAIL \
        } \
        else \
        { \
            YUV9P_HEAD \
            function(&output_row, \
                input_y + j, \
                input_u + j / 4, \
                input_v + j / 4, \
                j); \
            PLANAR_TAIL \
        } \
    } \
    register_cmodel_function(in, out, 0, function##_); \
}
// *******************************************************************************

static inline void YUV_PLANAR_to_RGB8(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v, r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	*(*output) = (unsigned char)((r & 0xc0) +
			    			 ((g & 0xe0) >> 2) +
		 	    			 ((b & 0xe0) >> 5));
	(*output)++;
}

static inline void YUV_PLANAR_to_BGR565(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((b & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((r & 0xf8) >> 3);
	(*output) += 2;
}

static inline void YUV_PLANAR_to_RGB565(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	*(uint16_t*)(*output) = ((r & 0xf8) << 8)
			 + ((g & 0xfc) << 3)
			 + ((b & 0xf8) >> 3);
	(*output) += 2;
}


static inline void YUV_PLANAR_to_RGB888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v, r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void YUV_PLANAR_to_ARGB8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v, r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = 0xff;
	(*output)[1] = r;
	(*output)[2] = g;
	(*output)[3] = b;
	(*output) += 4;
}

static inline void YUV_PLANAR_to_ABGR8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v, r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = 0xff;
	(*output)[3] = r;
	(*output)[2] = g;
	(*output)[1] = b;
	(*output) += 4;
}

static inline void YUV_PLANAR_to_RGBA8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	int y, u, v;
	int r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void YUV_PLANAR_to_YUV888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = *input_y;
	(*output)[1] = *input_u;
	(*output)[2] = *input_v;
	(*output) += 3;
}

static inline void YUV_PLANAR_to_YUVA8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	(*output)[0] = *input_y;
	(*output)[1] = *input_u;
	(*output)[2] = *input_v;
	(*output)[3] = 0xff;
	(*output) += 4;
}


static inline void YUV_PLANAR_to_RGB_FLOAT(unsigned char* *output, 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	float y = (float)*input_y / 0xff;
	int u, v;
	float r, g, b;
	u = *input_u;
	v = *input_v;
	YUV_TO_FLOAT(y, u, v, r, g, b)
// optimization error here

    float *(*output2) = (float**)output;
	(*output2)[0] = r;
	(*output2)[1] = g;
	(*output2)[2] = b;
	(*output2) += 3;
}


static inline void YUV_PLANAR_to_RGBA_FLOAT(unsigned char* *output, 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
// Signedness is important
	float y = (float)*input_y / 0xff;
	int u, v;
	float r, g, b;
	u = *input_u;
	v = *input_v;
	YUV_TO_FLOAT(y, u, v, r, g, b)

// optimization error here
    float *(*output2) = (float**)output;
	(*output2)[0] = r;
	(*output2)[1] = g;
	(*output2)[2] = b;
	(*output2)[3] = 1.0;
	(*output2) += 4;
}


static inline void YUV_PLANAR_to_BGR888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;
	
	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 3;
}

static inline void YUV_PLANAR_to_BGR8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	int r, g, b;

	y = (*input_y << 16) | (*input_y << 8) | *input_y;
	u = *input_u;
	v = *input_v;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}


// YUV10P_PLANAR ------------------------------------------------------------

// YUV planar 10 bits per channel
static inline void YUV10P_PLANAR_to_BGR8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	y = (input_y[1] << 22) | (input_y[0] << 14) | (input_y[0] << 6);
	u = (input_u[1] << 6) | (input_u[0] >> 2);
	v = (input_v[1] << 6) | (input_v[0] >> 2);
	
	int r, g, b;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = b;
	(*output)[1] = g;
	(*output)[2] = r;
	(*output) += 4;
}


static inline void YUV10P_PLANAR_to_RGB888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
	y = (input_y[1] << 22) | (input_y[0] << 14);
	u = (input_u[1] << 6) | (input_u[0] >> 2);
	v = (input_v[1] << 6) | (input_v[0] >> 2);
	
	int r, g, b;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output) += 3;
}

static inline void YUV10P_PLANAR_to_YUV_FLOAT(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
    int y_i, u_i, v_i;
	y_i = (input_y[1] << 8) | input_y[0];
	u_i = (input_u[1] << 8) | input_u[0];
	v_i = (input_v[1] << 8) | input_v[0];


    float *(*output2) = (float**)output;
	(*output2)[0] = (float)y_i / 0x3ff;
	(*output2)[1] = (float)u_i / 0x3ff;
	(*output2)[2] = (float)v_i / 0x3ff;
	(*output2) += 3;
}

static inline void YUV10P_PLANAR_to_RGB_FLOAT(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	float y;
    int y_i;
    int u, v;
	y_i = (input_y[1] << 22) | (input_y[0] << 14);
	u = (input_u[1] << 14) | (input_u[0] << 6);
	v = (input_v[1] << 14) | (input_v[0] << 6);

// fill missing bits
    y_i |= y_i >> 10;
    y_i |= y_i >> 20;
    y = (float)y_i / 0xffffff;
    u |= u >> 10;
    v |= v >> 10;

	float r, g, b;
	YUV16_TO_RGB_FLOAT(y, u, v, r, g, b)

    float *(*output2) = (float**)output;
	(*output2)[0] = r;
	(*output2)[1] = g;
	(*output2)[2] = b;
	(*output2) += 3;
}

static inline void YUV10P_PLANAR_to_RGBA_FLOAT(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	float y;
    int y_i;
    int u, v;
	y_i = (input_y[1] << 22) | (input_y[0] << 14);
	u = (input_u[1] << 14) | (input_u[0] << 6);
	v = (input_v[1] << 14) | (input_v[0] << 6);

// fill missing bits
    y_i |= y_i >> 10;
    y_i |= y_i >> 20;
    y = (float)y_i / 0xffffff;
    u |= u >> 10;
    v |= v >> 10;

	float r, g, b;
	YUV16_TO_RGB_FLOAT(y, u, v, r, g, b)

    float *(*output2) = (float**)output;
	(*output2)[0] = r;
	(*output2)[1] = g;
	(*output2)[2] = b;
	(*output2)[3] = 1;
	(*output2) += 4;
}

static inline void YUV10P_PLANAR_to_YUV888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
// throw away 2 bits
	y = (input_y[1] << 6) | (input_y[0] >> 2);
	u = (input_u[1] << 6) | (input_u[0] >> 2);
	v = (input_v[1] << 6) | (input_v[0] >> 2);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output) += 3;
}

static inline void YUV10P_PLANAR_to_YUVA8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
// throw away 2 bits
	y = (input_y[1] << 6) | (input_y[0] >> 2);
	u = (input_u[1] << 6) | (input_u[0] >> 2);
	v = (input_v[1] << 6) | (input_v[0] >> 2);

	(*output)[0] = y;
	(*output)[1] = u;
	(*output)[2] = v;
	(*output)[3] = 0xff;
	(*output) += 4;
}

static inline void YUV10P_PLANAR_to_RGBA8888(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v)
{
	int y, u, v;
// throw away 2 bits
	y = (input_y[1] << 22) | (input_y[0] << 14) | (input_y[0] << 6);
	u = (input_u[1] << 6) | (input_u[0] >> 2);
	v = (input_v[1] << 6) | (input_v[0] >> 2);
	
	int r, g, b;
	YUV_TO_RGB(y, u, v, r, g, b)

	(*output)[0] = r;
	(*output)[1] = g;
	(*output)[2] = b;
	(*output)[3] = 0xff;
	(*output) += 4;
}



static inline void YUV_PLANAR_to_YUV422(unsigned char *(*output), 
	unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	int j)
{
// Store U and V for even pixels only
	if(!(j & 1))
	{
		(*output)[1] = *input_u;
		(*output)[3] = *input_v;
		(*output)[0] = *input_y;
	}
	else
// Store Y and advance output for odd pixels only
	{
		(*output)[2] = *input_y;
		(*output) += 4;
	}
}


static inline void YUV_PLANAR_to_YUV420P(unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	unsigned char *output_y,
	unsigned char *output_u,
	unsigned char *output_v,
	int j)
{
	output_y[j] = *input_y;
	output_u[j / 2] = *input_u;
	output_v[j / 2] = *input_v;
}


static inline void YUV_PLANAR_to_YUV444P(unsigned char *input_y,
	unsigned char *input_u,
	unsigned char *input_v,
	unsigned char *output_y,
	unsigned char *output_u,
	unsigned char *output_v,
	int j)
{
	output_y[j] = *input_y;
	output_u[j] = *input_u;
	output_v[j] = *input_v;
}

void cmodel_init_planar()
{
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_RGB8, YUV_PLANAR_to_RGB8)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_BGR565, YUV_PLANAR_to_BGR565)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_RGB565, YUV_PLANAR_to_RGB565)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_BGR888, YUV_PLANAR_to_BGR888)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_BGR8888, YUV_PLANAR_to_BGR8888)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_RGB888, YUV_PLANAR_to_RGB888)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_RGBA8888, YUV_PLANAR_to_RGBA8888)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_ARGB8888, YUV_PLANAR_to_ARGB8888)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_ABGR8888, YUV_PLANAR_to_ABGR8888)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_RGB_FLOAT, YUV_PLANAR_to_RGB_FLOAT)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_RGBA_FLOAT, YUV_PLANAR_to_RGBA_FLOAT)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_YUV888, YUV_PLANAR_to_YUV888)
    SCALE_YUV420P_TO_PACKED(BC_YUV420P, BC_YUVA8888, YUV_PLANAR_to_YUVA8888)
    SCALE_YUV420P_YUV422(BC_YUV420P, BC_YUV422, YUV_PLANAR_to_YUV422)

// planar to planar
    SCALE_YUV420P_TO_YUV420P(BC_YUV420P, BC_YUV420P, YUV_PLANAR_to_YUV420P);
    SCALE_YUV420P_TO_YUV422P(BC_YUV420P, BC_YUV422P, YUV_PLANAR_to_YUV420P);
    SCALE_YUV420P_TO_YUV444P(BC_YUV420P, BC_YUV444P, YUV_PLANAR_to_YUV444P);

// nvidia hardware decoding
    SCALE_NV12(BC_NV12, BC_YUV888, YUV_PLANAR_to_YUV888)
    SCALE_NV12(BC_NV12, BC_YUVA8888, YUV_PLANAR_to_YUVA8888)
    SCALE_NV12(BC_NV12, BC_RGB888, YUV_PLANAR_to_RGB888)
    SCALE_NV12(BC_NV12, BC_BGR8888, YUV_PLANAR_to_BGR8888)
    SCALE_NV12(BC_NV12, BC_RGBA8888, YUV_PLANAR_to_RGBA8888)
    SCALE_NV12(BC_NV12, BC_RGB_FLOAT, YUV_PLANAR_to_RGB_FLOAT)
    SCALE_NV12(BC_NV12, BC_RGBA_FLOAT, YUV_PLANAR_to_RGBA_FLOAT)


// 10 bit HEVC
    SCALE_YUV420P10LE(BC_YUV420P10LE, BC_YUV888, YUV10P_PLANAR_to_YUV888)
    SCALE_YUV420P10LE(BC_YUV420P10LE, BC_YUVA8888, YUV10P_PLANAR_to_YUVA8888)
    SCALE_YUV420P10LE(BC_YUV420P10LE, BC_RGB888, YUV10P_PLANAR_to_RGB888)
    SCALE_YUV420P10LE(BC_YUV420P10LE, BC_BGR8888, YUV10P_PLANAR_to_BGR8888)
    SCALE_YUV420P10LE(BC_YUV420P10LE, BC_RGBA8888, YUV10P_PLANAR_to_RGBA8888)
    SCALE_YUV420P10LE(BC_YUV420P10LE, BC_RGB_FLOAT, YUV10P_PLANAR_to_RGB_FLOAT)
    SCALE_YUV420P10LE(BC_YUV420P10LE, BC_RGBA_FLOAT, YUV10P_PLANAR_to_RGBA_FLOAT)
    SCALE_YUV420P10LE(BC_YUV420P10LE, BC_YUV_FLOAT, YUV10P_PLANAR_to_YUV_FLOAT)

// quicktime formats which have never been used
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_RGB8, YUV_PLANAR_to_RGB8)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_BGR565, YUV_PLANAR_to_BGR565)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_RGB565, YUV_PLANAR_to_RGB565)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_BGR888, YUV_PLANAR_to_BGR888)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_BGR8888, YUV_PLANAR_to_BGR8888)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_RGB888, YUV_PLANAR_to_RGB888)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_RGBA8888, YUV_PLANAR_to_RGBA8888)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_ARGB8888, YUV_PLANAR_to_ARGB8888)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_ABGR8888, YUV_PLANAR_to_ABGR8888)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_RGB_FLOAT, YUV_PLANAR_to_RGB_FLOAT)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_RGBA_FLOAT, YUV_PLANAR_to_RGBA_FLOAT)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_YUV888, YUV_PLANAR_to_YUV888)
    SCALE_YUV444P_TO_PACKED(BC_YUV444P, BC_YUVA8888, YUV_PLANAR_to_YUVA8888)

    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_RGB8, YUV_PLANAR_to_RGB8)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_BGR565, YUV_PLANAR_to_BGR565)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_RGB565, YUV_PLANAR_to_RGB565)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_BGR888, YUV_PLANAR_to_BGR888)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_BGR8888, YUV_PLANAR_to_BGR8888)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_RGB888, YUV_PLANAR_to_RGB888)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_RGBA8888, YUV_PLANAR_to_RGBA8888)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_ARGB8888, YUV_PLANAR_to_ARGB8888)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_ABGR8888, YUV_PLANAR_to_ABGR8888)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_RGB_FLOAT, YUV_PLANAR_to_RGB_FLOAT)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_RGBA_FLOAT, YUV_PLANAR_to_RGBA_FLOAT)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_YUV888, YUV_PLANAR_to_YUV888)
    SCALE_YUV411P_TO_PACKED(BC_YUV411P, BC_YUVA8888, YUV_PLANAR_to_YUVA8888)


    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_RGB8, YUV_PLANAR_to_RGB8)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_BGR565, YUV_PLANAR_to_BGR565)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_RGB565, YUV_PLANAR_to_RGB565)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_BGR888, YUV_PLANAR_to_BGR888)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_BGR8888, YUV_PLANAR_to_BGR8888)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_RGB888, YUV_PLANAR_to_RGB888)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_RGBA8888, YUV_PLANAR_to_RGBA8888)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_ARGB8888, YUV_PLANAR_to_ARGB8888)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_ABGR8888, YUV_PLANAR_to_ABGR8888)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_RGB_FLOAT, YUV_PLANAR_to_RGB_FLOAT)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_RGBA_FLOAT, YUV_PLANAR_to_RGBA_FLOAT)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_YUV888, YUV_PLANAR_to_YUV888)
    SCALE_YUV422P_TO_PACKED(BC_YUV422P, BC_YUVA8888, YUV_PLANAR_to_YUVA8888)
//    SCALE_YUV422P_TO_YUV420P(BC_YUV422P, BC_YUV420P, YUV_PLANAR_to_YUVA8888)

// obsolete SVQ1 codec
    SCALE_YUV9P(BC_YUV9P, BC_RGB8, YUV_PLANAR_to_RGB8)
    SCALE_YUV9P(BC_YUV9P, BC_BGR565, YUV_PLANAR_to_BGR565)
    SCALE_YUV9P(BC_YUV9P, BC_RGB565, YUV_PLANAR_to_RGB565)
    SCALE_YUV9P(BC_YUV9P, BC_RGB888, YUV_PLANAR_to_RGB888)
    SCALE_YUV9P(BC_YUV9P, BC_ARGB8888, YUV_PLANAR_to_ARGB8888)
    SCALE_YUV9P(BC_YUV9P, BC_ABGR8888, YUV_PLANAR_to_ABGR8888)
    SCALE_YUV9P(BC_YUV9P, BC_RGBA8888, YUV_PLANAR_to_RGBA8888)
    SCALE_YUV9P(BC_YUV9P, BC_BGR888, YUV_PLANAR_to_BGR888)
    SCALE_YUV9P(BC_YUV9P, BC_BGR8888, YUV_PLANAR_to_BGR8888)
    SCALE_YUV9P(BC_YUV9P, BC_RGB_FLOAT, YUV_PLANAR_to_RGB_FLOAT)
    SCALE_YUV9P(BC_YUV9P, BC_RGBA_FLOAT, YUV_PLANAR_to_RGBA_FLOAT)
    SCALE_YUV9P(BC_YUV9P, BC_YUV888, YUV_PLANAR_to_YUV888)
    SCALE_YUV9P(BC_YUV9P, BC_YUVA8888, YUV_PLANAR_to_YUVA8888)
    SCALE_YUV9P_TO_YUV422(BC_YUV9P, BC_YUV422, YUV_PLANAR_to_YUV422)
// YUV9P_TO_YUV420P
// YUV9P_TO_YUV422P
// YUV9P_TO_YUV444P


}



