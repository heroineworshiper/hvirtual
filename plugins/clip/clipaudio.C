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

#include "clipaudio.h"
#include "confirmsave.h"
#include "bchash.h"
#include "errorbox.h"
#include "filexml.h"
#include "language.h"
#include "clip.h"
#include "samples.h"
#include "theme.h"
#include "vframe.h"

#include <string.h>


REGISTER_PLUGIN(Clip)


ClipConfig::ClipConfig()
{
	level = 0.0;
}

int ClipConfig::equivalent(ClipConfig &that)
{
	return EQUIV(level, that.level);
}

void ClipConfig::copy_from(ClipConfig &that)
{
	this->level = that.level;
}

void ClipConfig::interpolate(ClipConfig &prev, 
	ClipConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	level = prev.level * prev_scale + next.level * next_scale;
}





#define WINDOW_W DP(230)
#define WINDOW_H DP(60)


ClipWindow::ClipWindow(Clip *plugin)
 : PluginClientWindow(plugin, 
	WINDOW_W, 
	WINDOW_H, 
	WINDOW_W, 
	WINDOW_H, 
	0)
{
	this->plugin = plugin;
}

ClipWindow::~ClipWindow()
{
}

void ClipWindow::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
	int x = margin, y = margin;
    BC_Title *title;
	add_tool(title = new BC_Title(x, y, _("Level:")));
	y += margin + title->get_h();
	add_tool(level = new ClipLevel(plugin, x, y));
	show_window();
}








ClipLevel::ClipLevel(Clip *plugin, int x, int y)
 : BC_FSlider(x, 
 	y, 
	0,
	WINDOW_W - plugin->get_theme()->widget_border * 2,
	WINDOW_W - plugin->get_theme()->widget_border * 2,
	INFINITYGAIN, 
	0.0,
	plugin->config.level)
{
	this->plugin = plugin;
}
int ClipLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}






Clip::Clip(PluginServer *server)
 : PluginAClient(server)
{
	
}

Clip::~Clip()
{
	
}

const char* Clip::plugin_title() { return N_("Clip"); }
int Clip::is_realtime() { return 1; }


NEW_WINDOW_MACRO(Clip, ClipWindow)
LOAD_CONFIGURATION_MACRO(Clip, ClipConfig)

VFrame* Clip::new_picon()
{
    return 0;
}

int Clip::process_realtime(int64_t size, Samples *input_ptr, Samples *output_ptr)
{
	load_configuration();

	double level = db.fromdb(config.level);

    double *src = input_ptr->get_data();
    double *dst = output_ptr->get_data();
	for(int64_t i = 0; i < size; i++)
	{
        double sample = src[i];
        if(sample > level) sample = level;
        else
        if(sample < -level) sample = -level;
		dst[i] = sample;
	}

	return 0;
}




void Clip::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("CLIP");
	output.tag.set_property("LEVEL", config.level);
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Clip::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("CLIP"))
		{
			config.level = input.tag.get_property("LEVEL", config.level);
		}
	}
}

void Clip::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((ClipWindow*)thread->window)->level->update(config.level);
		thread->window->unlock_window();
	}
}


