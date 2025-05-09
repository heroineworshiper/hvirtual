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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "edl.inc"
#include "filexml.h"
#include "language.h"
#include "overlayframe.h"
#include "slide.h"
#include "theme.h"
#include "vframe.h"


#include <stdint.h>
#include <string.h>



REGISTER_PLUGIN(SlideMain)





SlideLeft::SlideLeft(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->motion_direction == 0, 
		_("Left"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideLeft::handle_event()
{
	update(1);
	plugin->motion_direction = 0;
	window->right->update(0);
	plugin->send_configure_change();
	return 0;
}

SlideRight::SlideRight(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->motion_direction == 1, 
		_("Right"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideRight::handle_event()
{
	update(1);
	plugin->motion_direction = 1;
	window->left->update(0);
	plugin->send_configure_change();
	return 0;
}

SlideIn::SlideIn(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 0, 
		_("In"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideIn::handle_event()
{
	update(1);
	plugin->direction = 0;
	window->out->update(0);
	plugin->send_configure_change();
	return 0;
}

SlideOut::SlideOut(SlideMain *plugin, 
	SlideWindow *window,
	int x,
	int y)
 : BC_Radial(x, 
		y, 
		plugin->direction == 1, 
		_("Out"))
{
	this->plugin = plugin;
	this->window = window;
}

int SlideOut::handle_event()
{
	update(1);
	plugin->direction = 1;
	window->in->update(0);
	plugin->send_configure_change();
	return 0;
}








SlideWindow::SlideWindow(SlideMain *plugin)
 : PluginClientWindow(plugin, 
	DP(320), 
	DP(100), 
	DP(320), 
	DP(100), 
	0)
{
	this->plugin = plugin;
}






void SlideWindow::create_objects()
{
	int widget_border = plugin->get_theme()->widget_border;
	int window_border = plugin->get_theme()->window_border;
	int x = window_border, y = window_border;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Direction:")));
	x += title->get_w() + widget_border;
	add_subwindow(left = new SlideLeft(plugin, 
		this,
		x,
		y));
	x += left->get_w() + widget_border;
	add_subwindow(right = new SlideRight(plugin, 
		this,
		x,
		y));

	y += right->get_h() + widget_border;
	x = window_border;
	add_subwindow(title = new BC_Title(x, y, _("Direction:")));
	x += title->get_w() + widget_border;
	add_subwindow(in = new SlideIn(plugin, 
		this,
		x,
		y));
	x += in->get_w() + widget_border;
	add_subwindow(out = new SlideOut(plugin, 
		this,
		x,
		y));

	show_window();
}











SlideMain::SlideMain(PluginServer *server)
 : PluginVClient(server)
{
	motion_direction = 0;
	direction = 0;
	
}

SlideMain::~SlideMain()
{
	
}

const char* SlideMain::plugin_title() { return N_("Slide"); }
int SlideMain::is_transition() { return 1; }
int SlideMain::uses_gui() { return 1; }

NEW_WINDOW_MACRO(SlideMain, SlideWindow)


void SlideMain::update_gui()
{
	if(thread)
	{
        load_configuration();
        thread->window->lock_window("ShapeWipeMain::update_gui 1");
        ((SlideWindow*)thread->window)->left->update(motion_direction == 0);
        ((SlideWindow*)thread->window)->right->update(motion_direction == 1);
        ((SlideWindow*)thread->window)->in->update(direction == 0);
        ((SlideWindow*)thread->window)->out->update(direction == 1);
        thread->window->unlock_window();
    }
}



void SlideMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("SLIDE");
	output.tag.set_property("MOTION_DIRECTION", motion_direction);
	output.tag.set_property("DIRECTION", direction);
	output.append_tag();
	output.terminate_string();
}

void SlideMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	while(!input.read_tag())
	{
		if(input.tag.title_is("SLIDE"))
		{
			motion_direction = input.tag.get_property("MOTION_DIRECTION", motion_direction);
			direction = input.tag.get_property("DIRECTION", direction);
		}
	}
}

int SlideMain::load_configuration()
{
	read_data(get_prev_keyframe(get_source_position()));
	return 1;
}




#define SLIDE(type, components) \
{ \
	if(direction == 0) \
	{ \
		int in_add, out_add, cpy_len; \
		if(motion_direction == 0) \
		{ \
			int x = w *  \
				PluginClient::get_source_position() /  \
				PluginClient::get_total_len(); \
			out_add = 0; \
			in_add = (w - x) * components * sizeof(type); \
			cpy_len = x * components * sizeof(type); \
		} \
		else \
		{ \
			int x = w - w *  \
				PluginClient::get_source_position() /  \
				PluginClient::get_total_len(); \
			out_add = x * components * sizeof(type); \
			in_add = 0; \
			cpy_len = (w - x) * components * sizeof(type); \
		} \
 \
		for(int j = 0; j < h; j++) \
		{ \
			memcpy( ((char *)outgoing->get_rows()[j]) + out_add, \
				((char *)incoming->get_rows()[j]) + in_add, \
				cpy_len); \
		} \
	} \
	else \
	{ \
		if(motion_direction == 0) \
		{ \
			int x = w - w *  \
				PluginClient::get_source_position() /  \
				PluginClient::get_total_len(); \
			for(int j = 0; j < h; j++) \
			{ \
				char *in_row = (char*)incoming->get_rows()[j]; \
				char *out_row = (char*)outgoing->get_rows()[j]; \
				memmove(out_row + 0, out_row + ((w - x) * components * sizeof(type)), x * components * sizeof(type)); \
				memcpy (out_row + x * components * sizeof(type), in_row + x * components * sizeof (type), (w - x) * components * sizeof(type)); \
			} \
		} \
		else \
		{ \
			int x = w *  \
				PluginClient::get_source_position() /  \
				PluginClient::get_total_len(); \
			for(int j = 0; j < h; j++) \
			{ \
				char *in_row = (char*)incoming->get_rows()[j]; \
				char *out_row = (char*)outgoing->get_rows()[j]; \
	 \
				memmove(out_row + (x * components *sizeof(type)), out_row + 0, (w - x) * components * sizeof(type)); \
				memcpy (out_row + 0, in_row + 0, (x) * components * sizeof(type)); \
			} \
		} \
	} \
}





int SlideMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();

//	struct timeval start_time;
//	gettimeofday(&start_time, 0);

	switch(incoming->get_color_model())
	{
		case BC_RGB_FLOAT:
			SLIDE(float, 3)
			break;
		case BC_RGBA_FLOAT:
			SLIDE(float, 4)
			break;
		case BC_RGB888:
		case BC_YUV888:
			SLIDE(unsigned char, 3)
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			SLIDE(unsigned char, 4)
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			SLIDE(uint16_t, 3)
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			SLIDE(uint16_t, 4)
			break;
	}
	
//	int64_t dif= get_difference(&start_time);
//	printf("diff: %lli\n", dif);

	return 0;
}
