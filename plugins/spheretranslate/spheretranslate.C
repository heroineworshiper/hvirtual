
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


#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "spheretranslate.h"
#include "spheretranslatewin.h"

#include <string.h>




REGISTER_PLUGIN(SphereTranslateMain)

SphereTranslateConfig::SphereTranslateConfig()
{
	translate_x = translate_y = translate_z = 0;
	rotate_x = rotate_y = rotate_z = 0;
	pivot_x = pivot_y = 50;
	draw_pivot = 0;
}

int SphereTranslateConfig::equivalent(SphereTranslateConfig &that)
{
	return EQUIV(translate_x, that.translate_x) && 
		EQUIV(translate_y, that.translate_y) && 
		EQUIV(translate_z, that.translate_z) && 
		EQUIV(rotate_x, that.rotate_x) &&
		EQUIV(rotate_y, that.rotate_y) && 
		EQUIV(rotate_z, that.rotate_z) &&
		EQUIV(pivot_x, that.pivot_x) && 
		EQUIV(pivot_y, that.pivot_y) &&
		draw_pivot == that.draw_pivot;
}

void SphereTranslateConfig::copy_from(SphereTranslateConfig &that)
{
	translate_x = that.translate_x;
	translate_y = that.translate_y;
	translate_z = that.translate_z;
	rotate_x = that.rotate_x;
	rotate_y = that.rotate_y;
	rotate_z = that.rotate_z;
	pivot_x = that.pivot_x;
	pivot_y = that.pivot_y;
	draw_pivot = that.draw_pivot;
}

void SphereTranslateConfig::interpolate(SphereTranslateConfig &prev, 
	SphereTranslateConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->translate_x = prev.translate_x * prev_scale + next.translate_x * next_scale;
	this->translate_y = prev.translate_y * prev_scale + next.translate_y * next_scale;
	this->translate_z = prev.translate_z * prev_scale + next.translate_z * next_scale;
	this->rotate_x = prev.rotate_x * prev_scale + next.rotate_x * next_scale;
	this->rotate_y = prev.rotate_y * prev_scale + next.rotate_y * next_scale;
	this->rotate_z = prev.rotate_z * prev_scale + next.rotate_z * next_scale;
	this->pivot_x = prev.pivot_x * prev_scale + next.pivot_x * next_scale;
	this->pivot_y = prev.pivot_y * prev_scale + next.pivot_y * next_scale;
	this->draw_pivot = prev.draw_pivot;
	boundaries();
}

void SphereTranslateConfig::boundaries()
{
	CLAMP(translate_x, -1.0, 1.0);
	CLAMP(translate_y, -1.0, 1.0);
	CLAMP(translate_z, -1.0, 1.0);
	CLAMP(rotate_x, -180.0, 180.0);
	CLAMP(rotate_y, -180.0, 180.0);
	CLAMP(rotate_z, -180.0, 180.0);
	CLAMP(pivot_x, 0, 100.0);
	CLAMP(pivot_y, 0, 100.0);
}








SphereTranslateMain::SphereTranslateMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
}

SphereTranslateMain::~SphereTranslateMain()
{
	if(engine) delete engine;
}

const char* SphereTranslateMain::plugin_title() { return N_("Sphere Translate"); }
int SphereTranslateMain::is_realtime() { return 1; }


LOAD_CONFIGURATION_MACRO(SphereTranslateMain, SphereTranslateConfig)

void SphereTranslateMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

// Store data
	output.tag.set_title("SPHERETRANSLATE");
	output.tag.set_property("TRANSLATE_X", config.translate_x);
	output.tag.set_property("TRANSLATE_Y", config.translate_y);
	output.tag.set_property("TRANSLATE_Z", config.translate_z);
	output.tag.set_property("ROTATE_X", config.rotate_x);
	output.tag.set_property("ROTATE_Y", config.rotate_y);
	output.tag.set_property("ROTATE_Z", config.rotate_z);
	output.tag.set_property("PIVOT_X", config.pivot_x);
	output.tag.set_property("PIVOT_Y", config.pivot_y);
	output.tag.set_property("DRAW_PIVOT", config.draw_pivot);
	output.append_tag();

	output.terminate_string();
}

void SphereTranslateMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SPHERETRANSLATE"))
			{
 				config.translate_x = input.tag.get_property("TRANSLATE_X", config.translate_x);
 				config.translate_y = input.tag.get_property("TRANSLATE_Y", config.translate_y);
 				config.translate_z = input.tag.get_property("TRANSLATE_Z", config.translate_z);
 				config.rotate_x = input.tag.get_property("ROTATE_X", config.rotate_x);
 				config.rotate_y = input.tag.get_property("ROTATE_Y", config.rotate_y);
 				config.rotate_z = input.tag.get_property("ROTATE_Z", config.rotate_z);
 				config.pivot_x = input.tag.get_property("PIVOT_X", config.pivot_x);
 				config.pivot_y = input.tag.get_property("PIVOT_Y", config.pivot_y);
 				config.draw_pivot = input.tag.get_property("DRAW_PIVOT", config.draw_pivot);
			}
		}
	}
}








int SphereTranslateMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	
	
	input = input_ptr;

	load_configuration();

//printf("SphereTranslateMain::process_realtime 1 %p\n", input);
	if(input->get_rows()[0] == output_ptr->get_rows()[0])
	{
		new_temp(input_ptr->get_w(), 
				input_ptr->get_h(),
				input->get_color_model());
		temp->copy_from(input);
		input = temp;
	}
//printf("SphereTranslateMain::process_realtime 2 %p\n", input);


	if(!engine) engine = new SphereTranslateEngine(this);
	engine->process_packages();

	if(config.draw_pivot)
	{
		int w = output_ptr->get_w();
		int h = output_ptr->get_h();
		int pivot_x = config.pivot_x * w / 100;
		int pivot_y = config.pivot_y * h / 100;

		output_ptr->draw_line(0, pivot_y, w, pivot_y);
		output_ptr->draw_line(pivot_x, 0, pivot_x, h);
	}
}



NEW_WINDOW_MACRO(SphereTranslateMain, SphereTranslateWin)

void SphereTranslateMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window();
			((SphereTranslateWin*)thread->window)->translate_x->update(config.translate_x);
			((SphereTranslateWin*)thread->window)->translate_y->update(config.translate_y);
			((SphereTranslateWin*)thread->window)->translate_z->update(config.translate_z);
			((SphereTranslateWin*)thread->window)->rotate_x->update(config.rotate_x);
			((SphereTranslateWin*)thread->window)->rotate_y->update(config.rotate_y);
			((SphereTranslateWin*)thread->window)->rotate_z->update(config.rotate_z);
			((SphereTranslateWin*)thread->window)->pivot_x->update(config.pivot_x);
			((SphereTranslateWin*)thread->window)->pivot_y->update(config.pivot_y);

			((SphereTranslateWin*)thread->window)->translate_x_text->update(config.translate_x);
			((SphereTranslateWin*)thread->window)->translate_y_text->update(config.translate_y);
			((SphereTranslateWin*)thread->window)->translate_z_text->update(config.translate_z);
			((SphereTranslateWin*)thread->window)->rotate_x_text->update(config.rotate_x);
			((SphereTranslateWin*)thread->window)->rotate_y_text->update(config.rotate_y);
			((SphereTranslateWin*)thread->window)->rotate_z_text->update(config.rotate_z);
			((SphereTranslateWin*)thread->window)->pivot_x_text->update(config.pivot_x);
			((SphereTranslateWin*)thread->window)->pivot_y_text->update(config.pivot_y);


			((SphereTranslateWin*)thread->window)->draw_pivot->update(config.draw_pivot);
			thread->window->unlock_window();
		}
	}
}






SphereTranslatePackage::SphereTranslatePackage()
 : LoadPackage() {}





SphereTranslateUnit::SphereTranslateUnit(SphereTranslateEngine *engine, SphereTranslateMain *plugin)
 : LoadClient(engine)
{
	this->plugin = plugin;
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
	VFrame *input = plugin->input;
	VFrame *output = plugin->get_output();
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	int w = input->get_w();
	int h = input->get_h();
	float pivot_x = (float)(plugin->config.pivot_x - 50) * w / 100;

// matrix which centers the Y pivot point
	float matrix1[3][3];
	rotate_to_matrix(matrix1, 
		0, 
		-(plugin->config.pivot_y - 50) * 180 / 100, 
		0);


// matrix which applies the Y, Z rotation to the point defined by the pivot
	float matrix2[3][3];
	rotate_to_matrix(matrix2, 
		0, 
		plugin->config.rotate_y, 
		plugin->config.rotate_z);


// matrix which undoes the Y pivot & applies X rotation
	float matrix3[3][3];
	rotate_to_matrix(matrix3, 
		plugin->config.rotate_x, 
		(plugin->config.pivot_y - 50) * 180 / 100, 
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

printf("SphereTranslateUnit::process_package %d %f %f %f %f %f\n", 
__LINE__,
plugin->config.rotate_x, plugin->config.rotate_y, plugin->config.rotate_z,
plugin->config.pivot_x, plugin->config.pivot_y);

	switch(plugin->get_input()->get_color_model())
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
//printf("SphereTranslateUnit::process_package %d\n", __LINE__);

}









SphereTranslateEngine::SphereTranslateEngine(SphereTranslateMain *plugin)
// : LoadServer(plugin->PluginClient::smp + 1, plugin->PluginClient::smp + 1)
 : LoadServer(1, 1)
{
	this->plugin = plugin;
}

SphereTranslateEngine::~SphereTranslateEngine()
{
}

void SphereTranslateEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		SphereTranslatePackage *package = (SphereTranslatePackage*)LoadServer::get_package(i);
		package->row1 = plugin->get_input()->get_h() * i / LoadServer::get_total_packages();
		package->row2 = plugin->get_input()->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* SphereTranslateEngine::new_client()
{
	return new SphereTranslateUnit(this, plugin);
}

LoadPackage* SphereTranslateEngine::new_package()
{
	return new SphereTranslatePackage;
}








