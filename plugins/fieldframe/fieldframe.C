
/*
 * CINELERRA
 * Copyright (C) 2008-2017 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "transportque.inc"
#include "vframe.h"

#include <string.h>


#define TOP_FIELD_FIRST 0
#define BOTTOM_FIELD_FIRST 1

class FieldFrame;
class FieldFrameWindow;







class FieldFrameConfig
{
public:
	FieldFrameConfig();
	int equivalent(FieldFrameConfig &src);
	int field_dominance;
	int first_frame;
};




class FieldFrameTop : public BC_Radial
{
public:
	FieldFrameTop(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);
	int handle_event();
	FieldFrame *plugin;
	FieldFrameWindow *gui;
};


class FieldFrameBottom : public BC_Radial
{
public:
	FieldFrameBottom(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);
	int handle_event();
	FieldFrame *plugin;
	FieldFrameWindow *gui;
};

// class FieldFrameFirst : public BC_Radial
// {
// public:
// 	FieldFrameFirst(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);
// 	int handle_event();
// 	FieldFrame *plugin;
// 	FieldFrameWindow *gui;
// };
// 
// class FieldFrameSecond : public BC_Radial
// {
// public:
// 	FieldFrameSecond(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);
// 	int handle_event();
// 	FieldFrame *plugin;
// 	FieldFrameWindow *gui;
// };

class FieldFrameWindow : public PluginClientWindow
{
public:
	FieldFrameWindow(FieldFrame *plugin);
	void create_objects();
	FieldFrame *plugin;
	FieldFrameTop *top;
	FieldFrameBottom *bottom;
//	FieldFrameFirst *first;
//	FieldFrameSecond *second;
};






class FieldFrame : public PluginVClient
{
public:
	FieldFrame(PluginServer *server);
	~FieldFrame();

	PLUGIN_CLASS_MEMBERS(FieldFrameConfig);

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void apply_field(VFrame *output, VFrame *input, int field);


	VFrame *input;
};








REGISTER_PLUGIN(FieldFrame)




FieldFrameConfig::FieldFrameConfig()
{
	field_dominance = TOP_FIELD_FIRST;
	first_frame = 0;
}

int FieldFrameConfig::equivalent(FieldFrameConfig &src)
{
	return src.field_dominance == field_dominance &&
		src.first_frame == first_frame;
}








FieldFrameWindow::FieldFrameWindow(FieldFrame *plugin)
 : PluginClientWindow(plugin, 
	DP(230), 
	DP(100), 
	DP(230), 
	DP(100), 
	0)
{
	this->plugin = plugin;
}

void FieldFrameWindow::create_objects()
{
	int x = DP(10), y = DP(10);
	add_subwindow(top = new FieldFrameTop(plugin, this, x, y));
	y += DP(30);
	add_subwindow(bottom = new FieldFrameBottom(plugin, this, x, y));
// 	y += 30;
// 	add_subwindow(first = new FieldFrameFirst(plugin, this, x, y));
// 	y += 30;
// 	add_subwindow(second = new FieldFrameSecond(plugin, this, x, y));

	show_window();
}













FieldFrameTop::FieldFrameTop(FieldFrame *plugin, 
	FieldFrameWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == TOP_FIELD_FIRST,
	_("Top field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FieldFrameTop::handle_event()
{
	plugin->config.field_dominance = TOP_FIELD_FIRST;
	gui->bottom->update(0);
	plugin->send_configure_change();
	return 1;
}





FieldFrameBottom::FieldFrameBottom(FieldFrame *plugin, 
	FieldFrameWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == BOTTOM_FIELD_FIRST,
	_("Bottom field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FieldFrameBottom::handle_event()
{
	plugin->config.field_dominance = BOTTOM_FIELD_FIRST;
	gui->top->update(0);
	plugin->send_configure_change();
	return 1;
}





// FieldFrameFirst::FieldFrameFirst(FieldFrame *plugin, 
// 	FieldFrameWindow *gui, 
// 	int x, 
// 	int y)
//  : BC_Radial(x, 
// 	y, 
// 	plugin->config.first_frame == 0,
// 	_("First frame is first field"))
// {
// 	this->plugin = plugin;
// 	this->gui = gui;
// }
// 
// int FieldFrameFirst::handle_event()
// {
// 	plugin->config.first_frame = 0;
// 	gui->second->update(0);
// 	plugin->send_configure_change();
// 	return 1;
// }
// 
// 
// 
// 
// FieldFrameSecond::FieldFrameSecond(FieldFrame *plugin, 
// 	FieldFrameWindow *gui, 
// 	int x, 
// 	int y)
//  : BC_Radial(x, 
// 	y, 
// 	plugin->config.first_frame == 1,
// 	_("Second frame is first field"))
// {
// 	this->plugin = plugin;
// 	this->gui = gui;
// }
// 
// int FieldFrameSecond::handle_event()
// {
// 	plugin->config.first_frame = 1;
// 	gui->first->update(0);
// 	plugin->send_configure_change();
// 	return 1;
// }






















FieldFrame::FieldFrame(PluginServer *server)
 : PluginVClient(server)
{
	
	input = 0;
}


FieldFrame::~FieldFrame()
{
	

	if(input) delete input;
}

const char* FieldFrame::plugin_title() { return N_("Fields to frames"); }
int FieldFrame::is_realtime() { return 1; }


NEW_PICON_MACRO(FieldFrame)
NEW_WINDOW_MACRO(FieldFrame, FieldFrameWindow)

int FieldFrame::load_configuration()
{
	KeyFrame *prev_keyframe;
	FieldFrameConfig old_config = config;

	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);

	return !old_config.equivalent(config);
}



void FieldFrame::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("FIELD_FRAME");
	output.tag.set_property("DOMINANCE", config.field_dominance);
	output.tag.set_property("FIRST_FRAME", config.first_frame);
	output.append_tag();
	output.terminate_string();
}

void FieldFrame::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("FIELD_FRAME"))
		{
			config.field_dominance = input.tag.get_property("DOMINANCE", config.field_dominance);
			config.first_frame = input.tag.get_property("FIRST_FRAME", config.first_frame);
		}
	}
}


void FieldFrame::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window();
			((FieldFrameWindow*)thread->window)->top->update(config.field_dominance == TOP_FIELD_FIRST);
			((FieldFrameWindow*)thread->window)->bottom->update(config.field_dominance == BOTTOM_FIELD_FIRST);
//			thread->window->first->update(config.first_frame == 0);
//			thread->window->second->update(config.first_frame == 1);
			thread->window->unlock_window();
		}
	}
}


int FieldFrame::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int result = 0;
	load_configuration();

	if(input && !input->equivalent(frame, 0))
	{
		delete input;
		input = 0;
	}

	if(!input)
	{
		input = new VFrame(0, 
			-1,
			frame->get_w(), 
			frame->get_h(), 
			frame->get_color_model(),
			-1);
	}

// Get input frames
	int64_t field1_position = start_position * 2;
	int64_t field2_position = start_position * 2 + 1;

	if (get_direction() == PLAY_REVERSE)
	{
		field1_position -= 1;
		field2_position -= 1;
	}


// printf("FieldFrame::process_buffer %d %lld %lld\n", 
// config.field_dominance, 
// field1_position, 
// field2_position);
	read_frame(input, 
		0, 
		field1_position,
		frame_rate * 2,
		0);
	apply_field(frame, 
		input, 
		config.field_dominance == TOP_FIELD_FIRST ? 0 : 1);
	read_frame(input, 
		0, 
		field2_position,
		frame_rate * 2,
		0);
	apply_field(frame, 
		input, 
		config.field_dominance == TOP_FIELD_FIRST ? 1 : 0);





	return result;
}


void FieldFrame::apply_field(VFrame *output, VFrame *input, int field)
{
	unsigned char **input_rows = input->get_rows();
	unsigned char **output_rows = output->get_rows();
	int row_size = VFrame::calculate_bytes_per_pixel(output->get_color_model()) * output->get_w();
	for(int i = field; i < output->get_h(); i += 2)
	{
		memcpy(output_rows[i], input_rows[i], row_size);
	}
}
