
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

#include "filexml.h"
#include "clip.h"
#include "colorbalance.h"
#include "bchash.h"
#include "language.h"
#include "picon_png.h"
#include "playback3d.h"

#include "aggregated.h"
#include "../interpolate/aggregated.h"
#include "../gamma/aggregated.h"

#include <stdio.h>
#include <string.h>

// 1000 corresponds to (1.0 + MAX_COLOR) * input
#define MAX_COLOR 1.0

REGISTER_PLUGIN(ColorBalanceMain)



ColorBalanceConfig::ColorBalanceConfig()
{
	cyan = 0;
	magenta = 0;
	yellow = 0;
	lock_params = 0;
    preserve = 0;
}

int ColorBalanceConfig::equivalent(ColorBalanceConfig &that)
{
	return (cyan == that.cyan && 
		magenta == that.magenta && 
		yellow == that.yellow && 
		lock_params == that.lock_params && 
    	preserve == that.preserve);
}

void ColorBalanceConfig::copy_from(ColorBalanceConfig &that)
{
	cyan = that.cyan;
	magenta = that.magenta;
	yellow = that.yellow;
	lock_params = that.lock_params;
    preserve = that.preserve;
}

void ColorBalanceConfig::interpolate(ColorBalanceConfig &prev, 
	ColorBalanceConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->cyan = prev.cyan * prev_scale + next.cyan * next_scale;
	this->magenta = prev.magenta * prev_scale + next.magenta * next_scale;
	this->yellow = prev.yellow * prev_scale + next.yellow * next_scale;
	this->preserve = prev.preserve;
	this->lock_params = prev.lock_params;
}










ColorBalanceEngine::ColorBalanceEngine(ColorBalanceMain *plugin)
 : Thread()
{
	this->plugin = plugin;
	last_frame = 0;
	set_synchronous(1);
}

ColorBalanceEngine::~ColorBalanceEngine()
{
	last_frame = 1;
	input_lock.unlock();
	Thread::join();
}


int ColorBalanceEngine::start_process_frame(VFrame *output, VFrame *input, int row_start, int row_end)
{
	this->output = output;
	this->input = input;
	this->row_start = row_start;
	this->row_end = row_end;
	input_lock.unlock();
	return 0;
}


int ColorBalanceEngine::wait_process_frame()
{
	output_lock.lock("ColorBalanceEngine::wait_process_frame");
	return 0;
}

void ColorBalanceEngine::run()
{
	while(1)
	{
		input_lock.lock("ColorBalanceEngine::run");
		if(last_frame)
		{
			output_lock.unlock();
			return;
		}

#define PROCESS(yuvtorgb,  \
	rgbtoyuv,  \
	r_lookup,  \
	g_lookup,  \
	b_lookup,  \
	type,  \
	max,  \
	components,  \
	do_yuv) \
{ \
	int i, j, k; \
	int y, cb, cr, r, g, b, r_n, g_n, b_n; \
    float h, s, v, h_old, s_old, r_f, g_f, b_f; \
	type **input_rows, **output_rows; \
	input_rows = (type**)input->get_rows(); \
	output_rows = (type**)output->get_rows(); \
 \
	for(j = row_start; j < row_end; j++) \
	{ \
		for(k = 0; k < input->get_w() * components; k += components) \
		{ \
			if(do_yuv) \
			{ \
				y = input_rows[j][k]; \
				cb = input_rows[j][k + 1]; \
				cr = input_rows[j][k + 2]; \
				yuvtorgb(r, g, b, y, cb, cr); \
			} \
			else \
			{ \
            	r = input_rows[j][k]; \
            	g = input_rows[j][k + 1]; \
            	b = input_rows[j][k + 2]; \
			} \
 \
            r_n = plugin->r_lookup[r]; \
            g_n = plugin->g_lookup[g]; \
            b_n = plugin->b_lookup[b]; \
 \
			if(plugin->config.preserve) \
            { \
				HSV::rgb_to_hsv((float)r_n, (float)g_n, (float)b_n, h, s, v); \
				HSV::rgb_to_hsv((float)r, (float)g, (float)b, h_old, s_old, v); \
                HSV::hsv_to_rgb(r_f, g_f, b_f, h, s, v); \
                r = (type)r_f; \
                g = (type)g_f; \
                b = (type)b_f; \
			} \
            else \
            { \
                r = r_n; \
                g = g_n; \
                b = b_n; \
			} \
 \
			if(do_yuv) \
			{ \
				rgbtoyuv(CLAMP(r, 0, max), CLAMP(g, 0, max), CLAMP(b, 0, max), y, cb, cr); \
                output_rows[j][k] = y; \
                output_rows[j][k + 1] = cb; \
                output_rows[j][k + 2] = cr; \
			} \
			else \
			{ \
                output_rows[j][k] = CLAMP(r, 0, max); \
                output_rows[j][k + 1] = CLAMP(g, 0, max); \
                output_rows[j][k + 2] = CLAMP(b, 0, max); \
			} \
		} \
	} \
}

#define PROCESS_F(components) \
{ \
	int i, j, k; \
	float y, cb, cr, r, g, b, r_n, g_n, b_n; \
    float h, s, v, h_old, s_old, r_f, g_f, b_f; \
	float **input_rows, **output_rows; \
	input_rows = (float**)input->get_rows(); \
	output_rows = (float**)output->get_rows(); \
	cyan_f = plugin->calculate_transfer(plugin->config.cyan); \
	magenta_f = plugin->calculate_transfer(plugin->config.magenta); \
	yellow_f = plugin->calculate_transfer(plugin->config.yellow); \
 \
/* printf("PROCESS_F %f\n", cyan_f); */ \
	for(j = row_start; j < row_end; j++) \
	{ \
		for(k = 0; k < input->get_w() * components; k += components) \
		{ \
            r = input_rows[j][k]; \
            g = input_rows[j][k + 1]; \
            b = input_rows[j][k + 2]; \
 \
            r_n = r * cyan_f; \
            g_n = g * magenta_f; \
            b_n = b * yellow_f; \
 \
			if(plugin->config.preserve) \
            { \
				HSV::rgb_to_hsv(r_n, g_n, b_n, h, s, v); \
				HSV::rgb_to_hsv(r, g, b, h_old, s_old, v); \
                HSV::hsv_to_rgb(r_f, g_f, b_f, h, s, v); \
                r = (float)r_f; \
                g = (float)g_f; \
                b = (float)b_f; \
			} \
            else \
            { \
                r = r_n; \
                g = g_n; \
                b = b_n; \
			} \
 \
            output_rows[j][k] = r; \
            output_rows[j][k + 1] = g; \
            output_rows[j][k + 2] = b; \
		} \
	} \
}

		switch(input->get_color_model())
		{
			case BC_RGB888:
				PROCESS(yuv.yuv_to_rgb_8, 
					yuv.rgb_to_yuv_8, 
					r_lookup_8, 
					g_lookup_8, 
					b_lookup_8, 
					unsigned char, 
					0xff, 
					3,
					0);
				break;

			case BC_RGB_FLOAT:
				PROCESS_F(3);
				break;

			case BC_YUV888:
				PROCESS(yuv.yuv_to_rgb_8, 
					yuv.rgb_to_yuv_8, 
					r_lookup_8, 
					g_lookup_8, 
					b_lookup_8, 
					unsigned char, 
					0xff, 
					3,
					1);
				break;
			
			case BC_RGBA_FLOAT:
				PROCESS_F(4);
				break;

			case BC_RGBA8888:
				PROCESS(yuv.yuv_to_rgb_8, 
					yuv.rgb_to_yuv_8, 
					r_lookup_8, 
					g_lookup_8, 
					b_lookup_8, 
					unsigned char, 
					0xff, 
					4,
					0);
				break;

			case BC_YUVA8888:
				PROCESS(yuv.yuv_to_rgb_8, 
					yuv.rgb_to_yuv_8, 
					r_lookup_8, 
					g_lookup_8, 
					b_lookup_8, 
					unsigned char, 
					0xff, 
					4,
					1);
				break;
			
			case BC_YUV161616:
				PROCESS(yuv.yuv_to_rgb_16, 
					yuv.rgb_to_yuv_16, 
					r_lookup_16, 
					g_lookup_16, 
					b_lookup_16, 
					u_int16_t, 
					0xffff, 
					3,
					1);
				break;

			case BC_YUVA16161616:
				PROCESS(yuv.yuv_to_rgb_16, 
					yuv.rgb_to_yuv_16, 
					r_lookup_16, 
					g_lookup_16, 
					b_lookup_16, 
					u_int16_t, 
					0xffff, 
					4,
					1);
				break;
		}

		
		
		output_lock.unlock();
	}
}




ColorBalanceMain::ColorBalanceMain(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
	
}

ColorBalanceMain::~ColorBalanceMain()
{
	


	if(engine)
	{
		for(int i = 0; i < total_engines; i++)
		{
			delete engine[i];
		}
		delete [] engine;
	}
}

const char* ColorBalanceMain::plugin_title() { return N_("Color Balance"); }
int ColorBalanceMain::is_realtime() { return 1; }


int ColorBalanceMain::reconfigure()
{
	int r_n, g_n, b_n;
	float r_scale = calculate_transfer(config.cyan);
	float g_scale = calculate_transfer(config.magenta);
	float b_scale = calculate_transfer(config.yellow);



#define RECONFIGURE(r_lookup, g_lookup, b_lookup, max) \
	for(int i = 0; i <= max; i++) \
    { \
	    r_lookup[i] = CLIP((int)(r_scale * i), 0, max); \
	    g_lookup[i] = CLIP((int)(g_scale * i), 0, max); \
	    b_lookup[i] = CLIP((int)(b_scale * i), 0, max); \
    }

	RECONFIGURE(r_lookup_8, g_lookup_8, b_lookup_8, 0xff);
	RECONFIGURE(r_lookup_16, g_lookup_16, b_lookup_16, 0xffff);
	
	return 0;
}

int64_t ColorBalanceMain::calculate_slider(float in)
{
	if(in < 1.0)
	{
		return (int64_t)(in * 1000 - 1000.0);
	}
	else
	if(in > 1.0)
	{
		return (int64_t)(1000 * (in - 1.0) / MAX_COLOR);
	}
	else
		return 0;
}

float ColorBalanceMain::calculate_transfer(float in)
{
	if(in < 0)
	{
		return (1000.0 + in) / 1000.0;
	}
	else
	if(in > 0)
	{
		return 1.0 + in / 1000.0 * MAX_COLOR;
	}
	else
		return 1.0;
}




int ColorBalanceMain::test_boundary(float &value)
{

	if(value < -1000) value = -1000;
    if(value > 1000) value = 1000;
	return 0;
}

int ColorBalanceMain::synchronize_params(ColorBalanceSlider *slider, float difference)
{
	if(thread && config.lock_params)
    {
	    if(slider != ((ColorBalanceWindow*)thread->window)->cyan)
        {
        	config.cyan += difference;
            test_boundary(config.cyan);
        	((ColorBalanceWindow*)thread->window)->cyan->update((int64_t)config.cyan);
        }
	    if(slider != ((ColorBalanceWindow*)thread->window)->magenta)
        {
        	config.magenta += difference;
            test_boundary(config.magenta);
        	((ColorBalanceWindow*)thread->window)->magenta->update((int64_t)config.magenta);
        }
	    if(slider != ((ColorBalanceWindow*)thread->window)->yellow)
        {
        	config.yellow += difference;
            test_boundary(config.yellow);
        	((ColorBalanceWindow*)thread->window)->yellow->update((int64_t)config.yellow);
        }
    }
	return 0;
}






NEW_PICON_MACRO(ColorBalanceMain)
LOAD_CONFIGURATION_MACRO(ColorBalanceMain, ColorBalanceConfig)
NEW_WINDOW_MACRO(ColorBalanceMain, ColorBalanceWindow)





int ColorBalanceMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	need_reconfigure |= load_configuration();

//printf("ColorBalanceMain::process_realtime 1 %d\n", need_reconfigure);
	if(need_reconfigure)
	{
		int i;

		if(!engine)
		{
			total_engines = PluginClient::smp + 1;
			engine = new ColorBalanceEngine*[total_engines];
			for(int i = 0; i < total_engines; i++)
			{
				engine[i] = new ColorBalanceEngine(this);
				engine[i]->start();
			}
		}

		reconfigure();
		need_reconfigure = 0;
	}

	frame->get_params()->update("COLORBALANCE_PRESERVE", config.preserve);
	frame->get_params()->update("COLORBALANCE_CYAN", calculate_transfer(config.cyan));
	frame->get_params()->update("COLORBALANCE_MAGENTA", calculate_transfer(config.magenta));
	frame->get_params()->update("COLORBALANCE_YELLOW", calculate_transfer(config.yellow));


	read_frame(frame,
		0,
		get_source_position(),
		get_framerate(),
		get_use_opengl());

	int aggregate_interpolate = 0;
	int aggregate_gamma = 0;
	get_aggregation(&aggregate_interpolate,
		&aggregate_gamma);

	if(!EQUIV(config.cyan, 0) || 
		!EQUIV(config.magenta, 0) || 
		!EQUIV(config.yellow, 0) ||
		(get_use_opengl() &&
			(aggregate_interpolate ||
			aggregate_gamma)))
	{
		if(get_use_opengl())
		{
get_output()->dump_stacks();
// Aggregate
			if(next_effect_is("Histogram")) return 0;
			return run_opengl();
		}
	
		for(int i = 0; i < total_engines; i++)
		{
			engine[i]->start_process_frame(frame, 
				frame, 
				frame->get_h() * i / total_engines, 
				frame->get_h() * (i + 1) / total_engines);
		}

		for(int i = 0; i < total_engines; i++)
		{
			engine[i]->wait_process_frame();
		}
	}


	return 0;
}


void ColorBalanceMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		((ColorBalanceWindow*)thread->window)->lock_window("ColorBalanceMain::update_gui");
		((ColorBalanceWindow*)thread->window)->cyan->update((int64_t)config.cyan);
		((ColorBalanceWindow*)thread->window)->magenta->update((int64_t)config.magenta);
		((ColorBalanceWindow*)thread->window)->yellow->update((int64_t)config.yellow);
		((ColorBalanceWindow*)thread->window)->preserve->update(config.preserve);
		((ColorBalanceWindow*)thread->window)->lock_params->update(config.lock_params);
		((ColorBalanceWindow*)thread->window)->unlock_window();
	}
}




void ColorBalanceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("COLORBALANCE");
	output.tag.set_property("CYAN", config.cyan);
	output.tag.set_property("MAGENTA",  config.magenta);
	output.tag.set_property("YELLOW",  config.yellow);
	output.tag.set_property("PRESERVELUMINOSITY",  config.preserve);
	output.tag.set_property("LOCKPARAMS",  config.lock_params);
	output.append_tag();
	output.terminate_string();
}

void ColorBalanceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("COLORBALANCE"))
			{
				config.cyan = input.tag.get_property("CYAN", config.cyan);
				config.magenta = input.tag.get_property("MAGENTA", config.magenta);
				config.yellow = input.tag.get_property("YELLOW", config.yellow);
				config.preserve = input.tag.get_property("PRESERVELUMINOSITY", config.preserve);
				config.lock_params = input.tag.get_property("LOCKPARAMS", config.lock_params);
			}
		}
	}
}

void ColorBalanceMain::get_aggregation(int *aggregate_interpolate,
	int *aggregate_gamma)
{
// 	if(!strcmp(get_output()->get_prev_effect(1), "Interpolate Pixels") &&
// 		!strcmp(get_output()->get_prev_effect(0), "Gamma"))
// 	{
// 		*aggregate_interpolate = 1;
// 		*aggregate_gamma = 1;
// 	}
// 	else
// 	if(!strcmp(get_output()->get_prev_effect(0), "Interpolate Pixels"))
// 	{
// 		*aggregate_interpolate = 1;
// 	}
// 	else
	if(!strcmp(get_output()->get_prev_effect(0), "Gamma"))
	{
		*aggregate_gamma = 1;
	}
}

int ColorBalanceMain::handle_opengl()
{
#ifdef HAVE_GL

	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int shader = 0;
	const char *shader_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int current_shader = 0;
	int aggregate_interpolate = 0;
	int aggregate_gamma = 0;

	get_aggregation(&aggregate_interpolate,
		&aggregate_gamma);

//printf("ColorBalanceMain::handle_opengl %d %d\n", aggregate_interpolate, aggregate_gamma);
	if(aggregate_interpolate)
		INTERPOLATE_COMPILE(shader_stack, current_shader)

	if(aggregate_gamma)
		GAMMA_COMPILE(shader_stack, current_shader, aggregate_interpolate)

	COLORBALANCE_COMPILE(shader_stack, 
		current_shader, 
		aggregate_gamma || aggregate_interpolate)

	shader = VFrame::make_shader(0, 
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

		if(aggregate_interpolate) INTERPOLATE_UNIFORMS(shader);
		if(aggregate_gamma) GAMMA_UNIFORMS(shader);

		COLORBALANCE_UNIFORMS(shader);

	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
    return 0;
}







