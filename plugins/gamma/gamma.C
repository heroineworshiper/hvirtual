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

#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "gamma.h"
#include "bchash.h"
#include "language.h"
#include "picon_png.h"
#include "cicolors.h"
#include "../interpolate/aggregated.h"
#include "playback3d.h"
#include "workarounds.h"


#include <stdio.h>
#include <string.h>

#include "aggregated.h"


REGISTER_PLUGIN(GammaMain)



GammaConfig::GammaConfig()
{
	max = 1;
	gamma = 0.6;
	automatic = 1;
	plot = 1;
}

int GammaConfig::equivalent(GammaConfig &that)
{
	return (EQUIV(max, that.max) && 
		EQUIV(gamma, that.gamma) &&
		automatic == that.automatic) &&
		plot == that.plot;
}

void GammaConfig::copy_from(GammaConfig &that)
{
	max = that.max;
	gamma = that.gamma;
	automatic = that.automatic;
	plot = that.plot;
}

void GammaConfig::interpolate(GammaConfig &prev, 
	GammaConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->max = prev.max * prev_scale + next.max * next_scale;
	this->gamma = prev.gamma * prev_scale + next.gamma * next_scale;
	this->automatic = prev.automatic;
	this->plot = prev.plot;
}








GammaPackage::GammaPackage()
 : LoadPackage()
{
	start = end = 0;
}










GammaUnit::GammaUnit(GammaMain *plugin)
{
	this->plugin = plugin;
}

	
void GammaUnit::process_package(LoadPackage *package)
{
	GammaPackage *pkg = (GammaPackage*)package;
	GammaEngine *engine = (GammaEngine*)get_server();
	VFrame *data = engine->data;
	int w = data->get_w();
	float r, g, b, y, u, v;

// The same algorithm used by dcraw
	if(engine->operation == GammaEngine::HISTOGRAM)
	{
#define HISTOGRAM_HEAD(type) \
		for(int i = pkg->start; i < pkg->end; i++) \
		{ \
			type *row = (type*)data->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{

#define HISTOGRAM_TAIL(components) \
				int slot; \
				slot = (int)(r * HISTOGRAM_SIZE); \
				accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++; \
				slot = (int)(g * HISTOGRAM_SIZE); \
				accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++; \
				slot = (int)(b * HISTOGRAM_SIZE); \
				accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++; \
				row += components; \
			} \
		}


		switch(data->get_color_model())
		{
			case BC_RGB888:
				HISTOGRAM_HEAD(unsigned char)
				r = (float)row[0] / 0xff;
				g = (float)row[1] / 0xff;
				b = (float)row[2] / 0xff;
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGBA8888:
				HISTOGRAM_HEAD(unsigned char)
				r = (float)row[0] / 0xff;
				g = (float)row[1] / 0xff;
				b = (float)row[2] / 0xff;
				HISTOGRAM_TAIL(4)
				break;
			case BC_RGB_FLOAT:
				HISTOGRAM_HEAD(float)
				r = row[0];
				g = row[1];
				b = row[2];
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGBA_FLOAT:
				HISTOGRAM_HEAD(float)
				r = row[0];
				g = row[1];
				b = row[2];
				HISTOGRAM_TAIL(4)
				break;
			case BC_YUV888:
				HISTOGRAM_HEAD(unsigned char)
				y = row[0];
				u = row[1];
				v = row[2];
				y /= 0xff;
				u = (float)((u - 0x80) / 0xff);
				v = (float)((v - 0x80) / 0xff);
				YUV::yuv_to_rgb_f(r, g, b, y, u, v);
				HISTOGRAM_TAIL(3)
				break;
			case BC_YUVA8888:
				HISTOGRAM_HEAD(unsigned char)
				y = row[0];
				u = row[1];
				v = row[2];
				y /= 0xff;
				u = (float)((u - 0x80) / 0xff);
				v = (float)((v - 0x80) / 0xff);
				YUV::yuv_to_rgb_f(r, g, b, y, u, v);
				HISTOGRAM_TAIL(4)
				break;
		}
	}
	else
	{
		float max = plugin->config.max * plugin->config.gamma;
		float scale = 1.0 / max;
		float gamma = plugin->config.gamma - 1.0;

#define GAMMA_HEAD(type) \
		for(int i = pkg->start; i < pkg->end; i++) \
		{ \
			type *row = (type*)data->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{

// powf errors don't show up until later in the pipeline, which makes
// this very hard to isolate.
#define MY_POW(x, y) ((x > 0.0) ? powf(x * 2 / max, y) : 0.0)

#define GAMMA_MID \
				r = r * scale * MY_POW(r, gamma); \
				g = g * scale * MY_POW(g, gamma); \
				b = b * scale * MY_POW(b, gamma); \

#define GAMMA_TAIL(components) \
				row += components; \
			} \
		}


		switch(data->get_color_model())
		{
			case BC_RGB888:
				GAMMA_HEAD(unsigned char)
				r = (float)row[0] / 0xff;
				g = (float)row[1] / 0xff;
				b = (float)row[2] / 0xff;
				GAMMA_MID
				row[0] = (int)CLIP(r * 0xff, 0, 0xff);
				row[1] = (int)CLIP(g * 0xff, 0, 0xff);
				row[2] = (int)CLIP(b * 0xff, 0, 0xff);
				GAMMA_TAIL(3)
				break;
			case BC_RGBA8888:
				GAMMA_HEAD(unsigned char)
				r = (float)row[0] / 0xff;
				g = (float)row[1] / 0xff;
				b = (float)row[2] / 0xff;
				GAMMA_MID
				row[0] = (int)CLIP(r * 0xff, 0, 0xff);
				row[1] = (int)CLIP(g * 0xff, 0, 0xff);
				row[2] = (int)CLIP(b * 0xff, 0, 0xff);
				GAMMA_TAIL(4)
				break;
			case BC_RGB_FLOAT:
				GAMMA_HEAD(float)
				r = row[0];
				g = row[1];
				b = row[2];
				GAMMA_MID
				row[0] = r;
				row[1] = g;
				row[2] = b;
				GAMMA_TAIL(3)
				break;
			case BC_RGBA_FLOAT:
				GAMMA_HEAD(float)
				r = row[0];
				g = row[1];
				b = row[2];
				GAMMA_MID
				row[0] = r;
				row[1] = g;
				row[2] = b;
				GAMMA_TAIL(4)
				break;
			case BC_YUV888:
				GAMMA_HEAD(unsigned char)
				y = row[0];
				u = row[1];
				v = row[2];
				y /= 0xff;
				u = (float)((u - 0x80) / 0xff);
				v = (float)((v - 0x80) / 0xff);
				YUV::yuv_to_rgb_f(r, g, b, y, u, v);
				GAMMA_MID
				YUV::rgb_to_yuv_f(r, g, b, y, u, v);
				y *= 0xff;
				u = u * 0xff + 0x80;
				v = v * 0xff + 0x80;
				row[0] = (int)CLIP(y, 0, 0xff);
				row[1] = (int)CLIP(u, 0, 0xff);
				row[2] = (int)CLIP(v, 0, 0xff);
				GAMMA_TAIL(3)
				break;
			case BC_YUVA8888:
				GAMMA_HEAD(unsigned char)
				y = row[0];
				u = row[1];
				v = row[2];
				y /= 0xff;
				u = (float)((u - 0x80) / 0xff);
				v = (float)((v - 0x80) / 0xff);
				YUV::yuv_to_rgb_f(r, g, b, y, u, v);
				GAMMA_MID
				YUV::rgb_to_yuv_f(r, g, b, y, u, v);
				y *= 0xff;
				u = u * 0xff + 0x80;
				v = v * 0xff + 0x80;
				row[0] = (int)CLIP(y, 0, 0xff);
				row[1] = (int)CLIP(u, 0, 0xff);
				row[2] = (int)CLIP(v, 0, 0xff);
				GAMMA_TAIL(4)
				break;
		}
	}
}











GammaEngine::GammaEngine(GammaMain *plugin)
 : LoadServer(plugin->get_project_smp() + 1, 
 	plugin->get_project_smp() + 1)
{
	this->plugin = plugin;
}

void GammaEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		GammaPackage *package = (GammaPackage*)get_package(i);
		package->start = data->get_h() * i / get_total_packages();
		package->end = data->get_h() * (i + 1) / get_total_packages();
	}

// Initialize clients here in case some don't get run.
	for(int i = 0; i < get_total_clients(); i++)
	{
		GammaUnit *unit = (GammaUnit*)get_client(i);
		bzero(unit->accum, sizeof(int) * HISTOGRAM_SIZE);
	}
	bzero(accum, sizeof(int) * HISTOGRAM_SIZE);
}

LoadClient* GammaEngine::new_client()
{
	return new GammaUnit(plugin);
}

LoadPackage* GammaEngine::new_package()
{
	return new GammaPackage;
}

void GammaEngine::process_packages(int operation, VFrame *data)
{
	this->data = data;
	this->operation = operation;
	LoadServer::process_packages();
	for(int i = 0; i < get_total_clients(); i++)
	{
		GammaUnit *unit = (GammaUnit*)get_client(i);
		for(int j = 0; j < HISTOGRAM_SIZE; j++)
		{
			accum[j] += unit->accum[j];
		}
	}
}
















GammaMain::GammaMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	
}

GammaMain::~GammaMain()
{
	

	delete engine;
}

const char* GammaMain::plugin_title() { return N_("Gamma"); }
int GammaMain::is_realtime() { return 1; }





NEW_PICON_MACRO(GammaMain)
NEW_WINDOW_MACRO(GammaMain, GammaWindow)
LOAD_CONFIGURATION_MACRO(GammaMain, GammaConfig)





int GammaMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	this->frame = frame;
	load_configuration();

	frame->get_params()->update("GAMMA_GAMMA", config.gamma);
	frame->get_params()->update("GAMMA_MAX", config.max);

	int use_opengl = get_use_opengl() &&
		!config.automatic && 
		(!config.plot || !gui_open());

	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		use_opengl);

	if(use_opengl)
	{
// Aggregate
		if(next_effect_is("Histogram"))
			return 0;
		if(next_effect_is("Color Balance"))
			return 0;

	
		return run_opengl();
	}
	else
	if(config.automatic)
	{
		calculate_max(frame);
// Always plot to set the slider
		send_render_gui(this, 1);
	}
	else
	if(config.plot) 
	{
		send_render_gui(this, 1);
	}

	if(!engine) engine = new GammaEngine(this);
	engine->process_packages(GammaEngine::APPLY, frame);
	return 0;
}

void GammaMain::calculate_max(VFrame *frame)
{
	if(!engine) engine = new GammaEngine(this);
	engine->process_packages(GammaEngine::HISTOGRAM, frame);
	int total_pixels = frame->get_w() * frame->get_h() * 3;
	int max_fraction = (int)((int64_t)total_pixels * 99 / 100);
	int current = 0;
	config.max = 1;
	for(int i = 0; i < HISTOGRAM_SIZE; i++)
	{
		current += engine->accum[i];
		if(current > max_fraction)
		{
			config.max = (float)i / HISTOGRAM_SIZE;
			break;
		}
	}
}


void GammaMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("GammaMain::update_gui");
			((GammaWindow*)thread->window)->update();
			thread->window->unlock_window();
		}
	}
}

void GammaMain::render_gui(void *data, int size)
{
	GammaMain *ptr = (GammaMain*)data;
	config.max = ptr->config.max;

	if(!engine) engine = new GammaEngine(this);
	if(ptr->engine && ptr->config.automatic)
	{
		thread->window->lock_window("GammaMain::render_gui");
		memcpy(engine->accum, 
			ptr->engine->accum, 
			sizeof(int) * HISTOGRAM_SIZE);
		((GammaWindow*)thread->window)->update();
		thread->window->unlock_window();
	}
	else
	{
		thread->window->lock_window("GammaMain::render_gui");
		engine->process_packages(GammaEngine::HISTOGRAM, 
			ptr->frame);
		((GammaWindow*)thread->window)->update_histogram();
		thread->window->unlock_window();
	}


}

void GammaMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("GAMMA");
	output.tag.set_property("MAX", config.max);
	output.tag.set_property("GAMMA", config.gamma);
	output.tag.set_property("AUTOMATIC",  config.automatic);
	output.tag.set_property("PLOT",  config.plot);
	output.append_tag();
	output.terminate_string();
}

void GammaMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("GAMMA"))
			{
				config.max = input.tag.get_property("MAX", config.max);
				config.gamma = input.tag.get_property("GAMMA", config.gamma);
				config.automatic = input.tag.get_property("AUTOMATIC", config.automatic);
				config.plot = input.tag.get_property("PLOT", config.plot);
//printf("GammaMain::read_data %f\n", config.max);
			}
		}
	}
}

int GammaMain::handle_opengl()
{
#ifdef HAVE_GL
//printf("GammaMain::handle_opengl 1\n");

	get_output()->to_texture();
	get_output()->enable_opengl();


	const char *shader_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int current_shader = 0;


// Aggregate with interpolate
	int aggregate = 0;
// 	if(prev_effect_is("Interpolate Pixels"))
// 	{
// 		aggregate = 1;
// 		INTERPOLATE_COMPILE(shader_stack, current_shader)
// 	}

	GAMMA_COMPILE(shader_stack, current_shader, aggregate);

	unsigned int shader = VFrame::make_shader(0,
				shader_stack[0],
				shader_stack[1],
				shader_stack[2],
				shader_stack[3],
				shader_stack[4],
				shader_stack[5],
				shader_stack[6],
				shader_stack[7],
				0);


	if(shader > 0) 
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);

		if(aggregate)
		{
			INTERPOLATE_UNIFORMS(shader)
		}
		GAMMA_UNIFORMS(shader)
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
    return 0;
}









