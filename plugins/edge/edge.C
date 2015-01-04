/*
 * CINELERRA
 * Copyright (C) 1997-2015 Adam Williams <broadcast at earthling dot net>
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

#include "affine.h"
#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "edge.h"
#include "edgewindow.h"
#include "language.h"
#include "transportque.inc"
#include <string.h>

// Edge detection from the Gimp

REGISTER_PLUGIN(Edge)

EdgeConfig::EdgeConfig()
{
	amount = 8;
}

int EdgeConfig::equivalent(EdgeConfig &that)
{
	if(this->amount != that.amount) return 0;
	return 1;
}

void EdgeConfig::copy_from(EdgeConfig &that)
{
	this->amount = that.amount;
}

void EdgeConfig::interpolate(
	EdgeConfig &prev, 
	EdgeConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	copy_from(next);
}

void EdgeConfig::limits()
{
	CLAMP(amount, 0, 10);
}


Edge::Edge(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	temp = 0;
}

Edge::~Edge()
{
	if(engine) delete engine;
	if(temp) delete temp;
}

const char* Edge::plugin_title() { return N_("Edge"); }
int Edge::is_realtime() { return 1; }

NEW_WINDOW_MACRO(Edge, EdgeWindow);
LOAD_CONFIGURATION_MACRO(Edge, EdgeConfig)

void Edge::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("EDGE");
	output.tag.set_property("AMOUNT", config.amount);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/EDGE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Edge::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("EDGE"))
			{
				config.amount = input.tag.get_property("AMOUNT", config.amount);
				config.limits();
			
			}
			else
			if(input.tag.title_is("/EDGE"))
			{
				result = 1;
			}
		}
	}

}

void Edge::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("Edge::update_gui");
			EdgeWindow *window = (EdgeWindow*)thread->window;
			thread->window->unlock_window();
		}
	}
}



int Edge::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{

	int need_reconfigure = load_configuration();
	int w = frame->get_w();
	int h = frame->get_h();
	int color_model = frame->get_color_model();

// initialize everything
	if(!temp)
	{
		engine = new EdgeEngine(this,
			PluginClient::get_project_smp() + 1,
			PluginClient::get_project_smp() + 1);
		
		temp = new VFrame(0,
			-1,
			w,
			h,
			color_model,
			-1);
		
	}
	
	read_frame(frame, 
		0, 
		start_position, 
		frame_rate);
	engine->process(temp, frame);
	frame->copy_from(temp);

	return 0;
}




EdgePackage::EdgePackage()
 : LoadPackage()
{
}

EdgeUnit::EdgeUnit(EdgeEngine *server) : LoadClient(server)
{
	this->server = server;
}

EdgeUnit::~EdgeUnit() 
{
}


float EdgeUnit::edge_detect(float *data, float max, int do_max)
{
	const float v_kernel[9] = { 0,  0,  0,
                               0,  2, -2,
                               0,  2, -2 };
	const float h_kernel[9] = { 0,  0,  0,
                               0, -2, -2,
                               0,  2,  2 };
	int i;
	float v_grad, h_grad;
	float amount = server->plugin->config.amount;

	for (i = 0, v_grad = 0, h_grad = 0; i < 9; i++)
    {
    	v_grad += v_kernel[i] * data[i];
    	h_grad += h_kernel[i] * data[i];
    }

	float result = sqrt (v_grad * v_grad * amount +
            	 h_grad * h_grad * amount);
	if(do_max)
		CLAMP(result, 0, max);
	return result;
}

#define EDGE_MACRO(type, max, components) \
{ \
	type **input_rows = (type**)server->src->get_rows(); \
	type **output_rows = (type**)server->dst->get_rows(); \
	int comps = MIN(components, 3); \
	for(int y = pkg->y1; y < pkg->y2; y++) \
	{ \
		for(int x = 0; x < w; x++) \
		{ \
/* kernel is in bounds */ \
			if(y > 0 && x > 0 && y < h - 2 && x < w - 2) \
			{ \
				for(int chan = 0; chan < comps; chan++) \
				{ \
/* load kernel */ \
					for(int kernel_y = 0; kernel_y < 3; kernel_y++) \
					{ \
						for(int kernel_x = 0; kernel_x < 3; kernel_x++) \
						{ \
							kernel[3 * kernel_y + kernel_x] = \
								(type)input_rows[y - 1 + kernel_y][(x - 1 + kernel_x) * components + chan]; \
						} \
					} \
/* do the business */ \
					output_rows[y][x * components + chan] = edge_detect(kernel, max, sizeof(type) < 4); \
				} \
				if(components == 4) output_rows[y][x * components + 3] = \
					input_rows[y][x * components + 3]; \
			} \
			else \
			{ \
				for(int chan = 0; chan < comps; chan++) \
				{ \
/* load kernel */ \
					for(int kernel_y = 0; kernel_y < 3; kernel_y++) \
					{ \
						for(int kernel_x = 0; kernel_x < 3; kernel_x++) \
						{ \
							int in_y = y - 1 + kernel_y; \
							int in_x = x - 1 + kernel_x; \
							CLAMP(in_y, 0, h - 1); \
							CLAMP(in_x, 0, w - 1); \
							kernel[3 * kernel_y + kernel_x] = \
								(type)input_rows[in_y][in_x * components + chan]; \
						} \
					} \
/* do the business */ \
					output_rows[y][x * components + chan] = edge_detect(kernel, max, sizeof(type) < 4); \
				} \
				if(components == 4) output_rows[y][x * components + 3] = \
					input_rows[y][x * components + 3]; \
			} \
		} \
	} \
}


void EdgeUnit::process_package(LoadPackage *package)
{
	EdgePackage *pkg = (EdgePackage*)package;
	int w = server->src->get_w();
	int h = server->src->get_h();
	float kernel[9];
	
	switch(server->src->get_color_model())
	{
		case BC_RGB_FLOAT:
			EDGE_MACRO(float, 1, 3);
			break;
		case BC_RGBA_FLOAT:
			EDGE_MACRO(float, 1, 4);
			break;
		case BC_RGB888:
		case BC_YUV888:
			EDGE_MACRO(unsigned char, 0xff, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			EDGE_MACRO(unsigned char, 0xff, 4);
			break;		
	}

}


EdgeEngine::EdgeEngine(Edge *plugin,
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

EdgeEngine::~EdgeEngine()
{
}


void EdgeEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		EdgePackage *pkg = (EdgePackage*)get_package(i);
		pkg->y1 = plugin->get_input(0)->get_h() * i / LoadServer::get_total_packages();
		pkg->y2 = plugin->get_input(0)->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

void EdgeEngine::process(VFrame *dst, VFrame *src)
{
	this->dst = dst;
	this->src = src;
	process_packages();
}


LoadClient* EdgeEngine::new_client()
{
	return new EdgeUnit(this);
}

LoadPackage* EdgeEngine::new_package()
{
	return new EdgePackage;
}


