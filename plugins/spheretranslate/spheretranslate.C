
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
#include "spheretranslator.h"

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


	if(!engine) engine = new SphereTranslateEngine(PluginClient::get_project_smp() + 1,
		PluginClient::get_project_smp() + 1);
	engine->process(output_ptr, 
		input, 
		config.rotate_x,
		config.rotate_y,
		config.rotate_z,
		config.pivot_x,
		config.pivot_y);

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


