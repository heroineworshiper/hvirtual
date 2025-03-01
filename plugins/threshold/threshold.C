
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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
#include "bchash.h"
#include "filexml.h"
#include "histogramengine.h"
#include "language.h"
#include "cicolors.h"
#include "threshold.h"
#include "thresholdwindow.h"
#include "vframe.h"

#include <string.h>


ThresholdConfig::ThresholdConfig()
{
	reset();
}

int ThresholdConfig::equivalent(ThresholdConfig &that)
{
	return EQUIV(min, that.min) &&
		EQUIV(max, that.max) &&
		plot == that.plot;
}

void ThresholdConfig::copy_from(ThresholdConfig &that)
{
	min = that.min;
	max = that.max;
	plot = that.plot;
}

void ThresholdConfig::interpolate(ThresholdConfig &prev,
	ThresholdConfig &next,
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / 
		(next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / 
		(next_frame - prev_frame);

	min = prev.min * prev_scale + next.min * next_scale;
	max = prev.max * prev_scale + next.max * next_scale;
	plot = prev.plot;
}

void ThresholdConfig::reset()
{
	min = 0.0;
	max = 1.0;
	plot = 1;
}

void ThresholdConfig::boundaries()
{
	CLAMP(min, HISTOGRAM_MIN, max);
	CLAMP(max, min, HISTOGRAM_MAX);
}








REGISTER_PLUGIN(ThresholdMain)

ThresholdMain::ThresholdMain(PluginServer *server)
 : PluginVClient(server)
{
	
	engine = 0;
	threshold_engine = 0;
}

ThresholdMain::~ThresholdMain()
{
	
	delete engine;
	delete threshold_engine;
}

int ThresholdMain::is_realtime()
{
	return 1;
}

const char* ThresholdMain::plugin_title() 
{ 
	return N_("Threshold"); 
}


#include "picon_png.h"
NEW_PICON_MACRO(ThresholdMain)

NEW_WINDOW_MACRO(ThresholdMain, ThresholdWindow)

LOAD_CONFIGURATION_MACRO(ThresholdMain, ThresholdConfig)







int ThresholdMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	int use_opengl = get_use_opengl() &&
		(!config.plot || !gui_open());

	read_frame(frame,
		0,
		get_source_position(),
		get_framerate(),
		use_opengl);

	if(use_opengl) return run_opengl();

	send_render_gui(frame, 1);

	if(!threshold_engine)
		threshold_engine = new ThresholdEngine(this);
	threshold_engine->process_packages(frame);
	
	return 0;
}


void ThresholdMain::save_data(KeyFrame *keyframe)
{
	FileXML file;
	file.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	file.tag.set_title("THRESHOLD");
	file.tag.set_property("MIN", config.min);
	file.tag.set_property("MAX", config.max);
	file.tag.set_property("PLOT", config.plot);
	file.append_tag();
	file.terminate_string();
}

void ThresholdMain::read_data(KeyFrame *keyframe)
{
	FileXML file;
	file.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));
	int result = 0;
	while(!result)
	{
		result = file.read_tag();
		if(!result)
		{
			config.min = file.tag.get_property("MIN", config.min);
			config.max = file.tag.get_property("MAX", config.max);
			config.plot = file.tag.get_property("PLOT", config.plot);
		}
	}
	config.boundaries();
}

void ThresholdMain::update_gui()
{
	if(thread)
	{
		thread->window->lock_window("ThresholdMain::update_gui");
		if(load_configuration())
		{
			((ThresholdWindow*)thread->window)->min->update(config.min);
			((ThresholdWindow*)thread->window)->max->update(config.max);
			((ThresholdWindow*)thread->window)->plot->update(config.plot);
		}
		thread->window->unlock_window();
	}
}

void ThresholdMain::render_gui(void *data, int size)
{
	if(thread)
	{
		calculate_histogram((VFrame*)data);
		thread->window->lock_window("ThresholdMain::render_gui");
		((ThresholdWindow*)thread->window)->canvas->draw();
		thread->window->unlock_window();
	}
}

void ThresholdMain::calculate_histogram(VFrame *frame)
{
	if(!engine) engine = new HistogramEngine(get_project_smp() + 1,
		get_project_smp() + 1);
	engine->process_packages(frame);
}

int ThresholdMain::handle_opengl()
{
#ifdef HAVE_GL
	static const char *rgb_shader = 
		"uniform sampler2D tex;\n"
		"uniform float min;\n"
		"uniform float max;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
		"	float v = dot(pixel.rgb, vec3(0.299, 0.587, 0.114));\n"
		"	if(v >= min && v < max)\n"
		"		pixel.rgb = vec3(1.0, 1.0, 1.0);\n"
		"	else\n"
		"		pixel.rgb = vec3(0.0, 0.0, 0.0);\n"
		"	gl_FragColor = pixel;\n"
		"}\n";

	static const char *yuv_shader = 
		"uniform sampler2D tex;\n"
		"uniform float min;\n"
		"uniform float max;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
		"	if(pixel.r >= min && pixel.r < max)\n"
		"		pixel.rgb = vec3(1.0, 0.5, 0.5);\n"
		"	else\n"
		"		pixel.rgb = vec3(0.0, 0.5, 0.5);\n"
		"	gl_FragColor = pixel;\n"
		"}\n";

	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int shader = 0;
	if(cmodel_is_yuv(get_output()->get_color_model()))
		shader = VFrame::make_shader(0, yuv_shader, 0);
	else
		shader = VFrame::make_shader(0, rgb_shader, 0);

	if(shader > 0)
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		glUniform1f(glGetUniformLocation(shader, "min"), config.min);
		glUniform1f(glGetUniformLocation(shader, "max"), config.max);
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
    return 0;
}























ThresholdPackage::ThresholdPackage()
 : LoadPackage()
{
	start = end = 0;
}











ThresholdUnit::ThresholdUnit(ThresholdEngine *server)
 : LoadClient(server)
{
	this->server = server;
}

void ThresholdUnit::process_package(LoadPackage *package)
{
	ThresholdPackage *pkg = (ThresholdPackage*)package;
	VFrame *data = server->data;
	int min = (int)(server->plugin->config.min * 0xffff);
	int max = (int)(server->plugin->config.max * 0xffff);
	int r, g, b, a, y, u, v;
	int w = server->data->get_w();
	int h = server->data->get_h();

#define THRESHOLD_HEAD(type) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *in_row = (type*)data->get_rows()[i]; \
		type *out_row = in_row; \
		for(int j = 0; j < w; j++) \
		{

#define THRESHOLD_TAIL(components, r_on, g_on, b_on, a_on, r_off, g_off, b_off, a_off) \
			v = (r * 76 + g * 150 + b * 29) >> 8; \
			if(v >= min && v < max) \
			{ \
				*out_row++ = r_on; \
				*out_row++ = g_on; \
				*out_row++ = b_on; \
				if(components == 4) *out_row++ = a_on; \
			} \
			else \
			{ \
				*out_row++ = r_off; \
				*out_row++ = g_off; \
				*out_row++ = b_off; \
				if(components == 4) *out_row++ = a_off; \
			} \
			in_row += components; \
		} \
	} \
}


	switch(data->get_color_model())
	{
		case BC_RGB888:
			THRESHOLD_HEAD(unsigned char)
			r = (in_row[0] << 8) | in_row[0];
			g = (in_row[1] << 8) | in_row[1];
			b = (in_row[2] << 8) | in_row[2];
			THRESHOLD_TAIL(3, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff);
			break;
		case BC_RGB_FLOAT:
			THRESHOLD_HEAD(float)
			r = (int)(in_row[0] * 0xffff);
			g = (int)(in_row[1] * 0xffff);
			b = (int)(in_row[2] * 0xffff);
			THRESHOLD_TAIL(3, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0);
			break;
		case BC_RGBA8888:
			THRESHOLD_HEAD(unsigned char)
			r = (in_row[0] << 8) | in_row[0];
			g = (in_row[1] << 8) | in_row[1];
			b = (in_row[2] << 8) | in_row[2];
			THRESHOLD_TAIL(4, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff);
			break;
		case BC_RGBA_FLOAT:
			THRESHOLD_HEAD(float)
			r = (int)(in_row[0] * 0xffff);
			g = (int)(in_row[1] * 0xffff);
			b = (int)(in_row[2] * 0xffff);
			THRESHOLD_TAIL(4, 1.0, 1.0, 1.0, 1.0, 0, 0, 0, 1.0);
			break;
		case BC_YUV888:
			THRESHOLD_HEAD(unsigned char)
			y = (in_row[0] << 8) | in_row[0];
			u = (in_row[1] << 8) | in_row[1];
			v = (in_row[2] << 8) | in_row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			THRESHOLD_TAIL(3, 0xff, 0x80, 0x80, 0xff, 0x0, 0x80, 0x80, 0xff)
			break;
		case BC_YUVA8888:
			THRESHOLD_HEAD(unsigned char)
			y = (in_row[0] << 8) | in_row[0];
			u = (in_row[1] << 8) | in_row[1];
			v = (in_row[2] << 8) | in_row[2];
			server->yuv->yuv_to_rgb_16(r, g, b, y, u, v);
			THRESHOLD_TAIL(4, 0xff, 0x80, 0x80, 0xff, 0x0, 0x80, 0x80, 0xff)
			break;
	}
}











ThresholdEngine::ThresholdEngine(ThresholdMain *plugin)
 : LoadServer(plugin->get_project_smp() + 1,
 	plugin->get_project_smp() + 1)
{
	this->plugin = plugin;
	yuv = new YUV;
}

ThresholdEngine::~ThresholdEngine()
{
	delete yuv;
}

void ThresholdEngine::process_packages(VFrame *data)
{
	this->data = data;
	LoadServer::process_packages();
}

void ThresholdEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		ThresholdPackage *package = (ThresholdPackage*)get_package(i);
		package->start = data->get_h() * i / get_total_packages();
		package->end = data->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* ThresholdEngine::new_client()
{
	return (LoadClient*)new ThresholdUnit(this);
}

LoadPackage* ThresholdEngine::new_package()
{
	return (LoadPackage*)new HistogramPackage;
}






