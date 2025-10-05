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
#include "edgeengine.h"
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
	output.set_shared_string(keyframe->get_data());
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

	input.set_shared_string(keyframe->get_data());

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
		engine = new EdgeEngine(
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
		frame_rate,
		0);
	engine->process(temp, frame, config.amount);
	frame->copy_from(temp);

	return 0;
}

