
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
}

int SphereTranslateConfig::equivalent(SphereTranslateConfig &that)
{
	return EQUIV(translate_x, that.translate_x) && 
		EQUIV(translate_y, that.translate_y) && 
		EQUIV(translate_z, that.translate_z) && 
		EQUIV(rotate_x, that.rotate_x) &&
		EQUIV(rotate_y, that.rotate_y) && 
		EQUIV(rotate_z, that.rotate_z);
}

void SphereTranslateConfig::copy_from(SphereTranslateConfig &that)
{
	translate_x = that.translate_x;
	translate_y = that.translate_y;
	translate_z = that.translate_z;
	rotate_x = that.rotate_x;
	rotate_y = that.rotate_y;
	rotate_z = that.rotate_z;
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
			}
		}
	}
}








int SphereTranslateMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	VFrame *input, *output;
	
	
	input = input_ptr;
	output = output_ptr;

	load_configuration();

//printf("SphereTranslateMain::process_realtime 1 %p\n", input);
	if(input->get_rows()[0] == output->get_rows()[0])
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
			((SphereTranslateWin*)thread->window)->translate_x_text->update(config.translate_x);
			((SphereTranslateWin*)thread->window)->translate_y_text->update(config.translate_y);
			((SphereTranslateWin*)thread->window)->translate_z_text->update(config.translate_z);
			((SphereTranslateWin*)thread->window)->rotate_x_text->update(config.rotate_x);
			((SphereTranslateWin*)thread->window)->rotate_y_text->update(config.rotate_y);
			((SphereTranslateWin*)thread->window)->rotate_z_text->update(config.rotate_z);
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

#define PIXEL_TO_ANGLE(x, y) \
	


void SphereTranslateUnit::process_package(LoadPackage *package)
{
	SphereTranslatePackage *pkg = (SphereTranslatePackage*)package;
	VFrame *input = plugin->get_temp();
	VFrame *output = plugin->get_output();
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	int w = input->get_w();
	int h = input->get_h();

// interpolate & accumulate a pixel in the output
#define BLEND_PIXEL(type, components) \
 	if(x_in < 0.0 || x_in >= w - 1 || \
		y_in < 0.0 || y_in >= h - 1) \
	{ \
		out_row += components; \
	} \
	else \
	{ \
		float y1_fraction = y_in - floor(y_in); \
		float y2_fraction = 1.0 - y1_fraction; \
		float x1_fraction = x_in - floor(x_in); \
		float x2_fraction = 1.0 - x1_fraction; \
		type *in_pixel1 = in_rows[(int)y_in] + (int)x_in * components; \
		type *in_pixel2 = in_rows[(int)y_in + 1] + (int)x_in * components; \
		for(int i = 0; i < components; i++) \
		{ \
			float value = in_pixel1[i] * x2_fraction * y2_fraction + \
				in_pixel2[i] * x2_fraction * y1_fraction + \
				in_pixel1[i + components] * x1_fraction * y2_fraction + \
				in_pixel2[i + components] * x1_fraction * y1_fraction; \
			value = *out_row * inv_a + value * a; \
			*out_row++ = (type)value; \
		} \
	}


#define FUNCTION(type, components, chroma) \
{ \
	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
 \
	for(int out_y = row1; out_y < row2; out_y++) \
	{ \
		type *out_row = out_rows[out_y]; \
 \
 		for(int out_x = 0; out_x < w; out_x++) \
		{ \
		} \
	} \
}

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

}









SphereTranslateEngine::SphereTranslateEngine(SphereTranslateMain *plugin)
 : LoadServer(plugin->PluginClient::smp + 1, plugin->PluginClient::smp + 1)
// : LoadServer(1, 1)
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



