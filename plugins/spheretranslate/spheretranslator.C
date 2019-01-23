
/*
 * CINELERRA
 * Copyright (C) 2017 Adam Williams <broadcast at earthling dot net>
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


// the rotation algorithm is from https://github.com/FoxelSA/libgnomonic.git



#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "spheretranslator.h"
#include "vframe.h"
#include "bcsignals.h"



SphereTranslatePackage::SphereTranslatePackage()
 : LoadPackage() {}





SphereTranslateUnit::SphereTranslateUnit(SphereTranslateEngine *engine)
 : LoadClient(engine)
{
	this->engine = engine;
}


SphereTranslateUnit::~SphereTranslateUnit()
{
}


void SphereTranslateUnit::rotate_to_matrix(float matrix[3][3], 
	float rotate_x, 
	float rotate_y, 
	float rotate_z)
{
    double cos_a = cos(rotate_x * 2 * M_PI / 360);
    double sin_a = sin(rotate_x * 2 * M_PI / 360);
    double cos_e = cos(rotate_y * 2 * M_PI / 360);
    double sin_e = sin(rotate_y * 2 * M_PI / 360);
    double cos_r = cos(rotate_z * 2 * M_PI / 360);
    double sin_r = sin(rotate_z * 2 * M_PI / 360);

    /* Compute matrix entries */
    matrix[0][0] = cos_a * cos_e;
    matrix[0][1] = cos_a * sin_e * sin_r - sin_a * cos_r;
    matrix[0][2] = cos_a * sin_e * cos_r + sin_a * sin_r;
    matrix[1][0] = sin_a * cos_e; 
    matrix[1][1] = sin_a * sin_e * sin_r + cos_a * cos_r;
    matrix[1][2] = sin_a * sin_e * cos_r - cos_a * sin_r;
    matrix[2][0] = -sin_e;
    matrix[2][1] = cos_e * sin_r;
    matrix[2][2] = cos_e * cos_r;
}

void SphereTranslateUnit::multiply_pixel_matrix(float *pvf, float *pvi, float matrix[3][3])
{
    pvf[0] = matrix[0][0] * pvi[0] + matrix[0][1] * pvi[1] + matrix[0][2] * pvi[2];
    pvf[1] = matrix[1][0] * pvi[0] + matrix[1][1] * pvi[1] + matrix[1][2] * pvi[2];
    pvf[2] = matrix[2][0] * pvi[0] + matrix[2][1] * pvi[1] + matrix[2][2] * pvi[2];
}

void SphereTranslateUnit::multiply_matrix_matrix(float dst[3][3], 
	float arg1[3][3], 
	float arg2[3][3])
{
	int i, j, k;


	for(i = 0; i < 3; i++)
	{
		float *dst_row = dst[i];
		float *arg1_row = arg1[i];

		for(j = 0; j < 3; j++)
		{
			double sum = 0;
			for(k = 0; k < 3; k++)
			{
				sum += arg2[k][j] * arg1_row[k];
			}

			dst_row[j] = sum;
		}
	}
}


void SphereTranslateUnit::process_package(LoadPackage *package)
{
	SphereTranslatePackage *pkg = (SphereTranslatePackage*)package;
	VFrame *input = engine->input;
	VFrame *output = engine->output;
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	int w = input->get_w();
	int h = input->get_h();
	float pivot_x = (float)(engine->pivot_x - 50) * w / 100;

// since this is the foundation of the motion effect,
// apply rotation in a way that cancels a motion vector at the pivot point
// matrix which centers the Y pivot point
	float matrix1[3][3];
	rotate_to_matrix(matrix1, 
		0, 
		-(engine->pivot_y - 50) * 180 / 100, 
		0);


// matrix which applies the Y, Z rotation to the point defined by the pivot
	float matrix2[3][3];
	rotate_to_matrix(matrix2, 
		0, 
		engine->rotate_y, 
		engine->rotate_z);


// matrix which undoes the Y pivot & applies X rotation
	float matrix3[3][3];
	rotate_to_matrix(matrix3, 
		engine->rotate_x, 
		(engine->pivot_y - 50) * 180 / 100, 
		0);


// 	float matrix4[3][3];
// // combine the transformations in order
// 	multiply_matrix_matrix(matrix4, 
// 		matrix1, 
// 		matrix2);
// 	multiply_matrix_matrix(matrix1, 
// 		matrix4, 
// 		matrix3);




	float pixel1[3];
	float pixel2[3];
	float pixel3[3];
	float pixel4[3];









// interpolate & accumulate a pixel in the output
#define BLEND_PIXEL(type, components) \
	float x_in2 = x_in + 1; \
	float y_in2 = y_in + 1; \
 \
 	if(x_in < 0.0) \
	{ \
		x_in += w; \
	} \
	else \
	if(x_in > w - 1) \
	{ \
		x_in -= w; \
	} \
 \
	if(y_in < 0.0) \
	{ \
		y_in = 0; \
	} \
	else \
	if(y_in > h - 1) \
	{ \
		y_in = h - 1; \
	} \
 \
 	if(x_in2 < 0.0) \
	{ \
		x_in2 += w; \
	} \
	else \
	if(x_in2 > w - 1) \
	{ \
		x_in2 -= w; \
	} \
 \
	if(y_in2 < 0.0) \
	{ \
		y_in2 = 0; \
	} \
	else \
	if(y_in2 > h - 1) \
	{ \
		y_in2 = h - 1; \
	} \
 \
/* printf("%f %f %f %f\n", x_in, y_in, x_in2, y_in2); */ \
 \
	float y1_fraction = y_in - floor(y_in); \
	float y2_fraction = 1.0 - y1_fraction; \
	float x1_fraction = x_in - floor(x_in); \
	float x2_fraction = 1.0 - x1_fraction; \
	type *in_pixel1 = in_rows[(int)y_in] + (int)x_in * components; \
	type *in_pixel2 = in_rows[(int)y_in2] + (int)x_in * components; \
	type *in_pixel3 = in_rows[(int)y_in] + (int)x_in2 * components; \
	type *in_pixel4 = in_rows[(int)y_in2] + (int)x_in2 * components; \
	for(int i = 0; i < components; i++) \
	{ \
		float value = in_pixel1[i] * x2_fraction * y2_fraction + \
			in_pixel2[i] * x2_fraction * y1_fraction + \
			in_pixel3[i] * x1_fraction * y2_fraction + \
			in_pixel4[i] * x1_fraction * y1_fraction; \
		*out_row++ = (type)value; \
	} \


# define LG_ATN(x,y) ( ( ( x ) >= 0 ) ? ( ( ( y ) >= 0 ) ? atan( ( y ) / ( x ) ) : (M_PI * 2) + atan( ( y ) / ( x ) ) ) : M_PI + atan( ( y ) / ( x ) ) )
# define LG_ASN(x)   ( asin( x ) )


#define FUNCTION(type, components, chroma) \
{ \
	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
 \
	for(int out_y = row1; out_y < row2; out_y++) \
	{ \
		type *out_row = out_rows[out_y]; \
    	float dy = (((float)out_y / h) - 0.5) * M_PI; \
 \
 		for(int out_x = 0; out_x < w; out_x++) \
		{ \
 \
 \
                /* Compute mapping pixel angular coordinates */ \
                float dx = (((float)(out_x - pivot_x) / w) * 2.0) * M_PI; \
 \
                /* Compute pixel position in 3d-frame */ \
                pixel1[0] = cos(dy); \
                pixel1[1] = sin(dx) * pixel1[0]; \
                pixel1[0] = cos(dx) * pixel1[0]; \
                pixel1[2] = sin(dy); \
 \
                /* Compute rotated pixel position in 3d-frame */ \
				multiply_pixel_matrix(pixel2, pixel1, matrix1); \
				multiply_pixel_matrix(pixel3, pixel2, matrix2); \
				multiply_pixel_matrix(pixel4, pixel3, matrix3); \
 \
                /* Retrieve mapping pixel (x,y)-coordinates */ \
                float x_in = w * (LG_ATN(pixel4[0], pixel4[1]) / (M_PI * 2)); \
                float y_in = h * ((LG_ASN(pixel4[2] ) / M_PI) + 0.5); \
				x_in += pivot_x; \
 \
 				if(isnan(x_in) || isnan(y_in)) \
				{ \
					printf("SphereTranslateUnit::process_package %d: out_x=%d out_y=%d x_in=%f y_in=%f\n", \
						__LINE__, out_x, out_y, x_in, y_in); \
				} \
				else \
				{ \
 \
	 				BLEND_PIXEL(type, components) \
				} \
 \
		} \
	} \
}

// printf("SphereTranslateUnit::process_package %d %f %f %f %f %f\n", 
// __LINE__,
// engine->rotate_x, engine->rotate_y, engine->rotate_z,
// engine->pivot_x, engine->pivot_y);

	switch(engine->input->get_color_model())
	{
		case BC_RGB888:
			FUNCTION(unsigned char, 3, 0x0);
			break;
		case BC_RGBA8888:
			FUNCTION(unsigned char, 4, 0x0);
			break;
		case BC_RGB_FLOAT:
			FUNCTION(float, 3, 0.0);
			break;
		case BC_RGBA_FLOAT:
			FUNCTION(float, 4, 0.0);
			break;
		case BC_YUV888:
			FUNCTION(unsigned char, 3, 0x80);
			break;
		case BC_YUVA8888:
			FUNCTION(unsigned char, 4, 0x80);
			break;
	}

//output->clear_frame();
}









SphereTranslateEngine::SphereTranslateEngine(int total_clients,
		int total_packages)
 : LoadServer(total_clients, total_packages)
// : LoadServer(1, 1)
{
}

SphereTranslateEngine::~SphereTranslateEngine()
{
}


void SphereTranslateEngine::set_pivot(float x, float y)
{
	this->pivot_x = x;
	this->pivot_y = y;
}

void SphereTranslateEngine::set_x(float value)
{
	this->rotate_x = value;
}

void SphereTranslateEngine::set_y(float value)
{
	this->rotate_y = value;
}

void SphereTranslateEngine::set_z(float value)
{
	this->rotate_z = value;
}


void SphereTranslateEngine::process(VFrame *output, 
		VFrame *input)
{
	this->input = input;
	this->output = output;
	process_packages();
}



void SphereTranslateEngine::process(VFrame *output, 
		VFrame *input, 
		float rotate_x,
		float rotate_y,
		float rotate_z,
		float pivot_x,
		float pivot_y)
{
	this->input = input;
	this->output = output;
	this->rotate_x = rotate_x;
	this->rotate_y = rotate_y;
	this->rotate_z = rotate_z;
	this->pivot_x = pivot_x;
	this->pivot_y = pivot_y;
	process_packages();
}


void SphereTranslateEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		SphereTranslatePackage *package = (SphereTranslatePackage*)LoadServer::get_package(i);
		package->row1 = input->get_h() * i / LoadServer::get_total_packages();
		package->row2 = input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* SphereTranslateEngine::new_client()
{
	return new SphereTranslateUnit(this);
}

LoadPackage* SphereTranslateEngine::new_package()
{
	return new SphereTranslatePackage;
}








