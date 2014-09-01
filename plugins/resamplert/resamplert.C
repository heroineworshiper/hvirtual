
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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
#include "language.h"
#include "resamplert.h"
#include "theme.h"
#include "transportque.h"

#include <string.h>







REGISTER_PLUGIN(ResampleRT);



ResampleRTConfig::ResampleRTConfig()
{
	scale = 1;
}


int ResampleRTConfig::equivalent(ResampleRTConfig &src)
{
	return fabs(scale - src.scale) < 0.0001;
}

void ResampleRTConfig::copy_from(ResampleRTConfig &src)
{
	this->scale = src.scale;
}

void ResampleRTConfig::interpolate(ResampleRTConfig &prev, 
	ResampleRTConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	this->scale = prev.scale;
}

void ResampleRTConfig::boundaries()
{
	if(fabs(scale) < 0.0001) scale = 0.0001;
}




ResampleRTWindow::ResampleRTWindow(ResampleRT *plugin)
 : PluginClientWindow(plugin, 
	210, 
	160, 
	200, 
	160, 
	0)
{
	this->plugin = plugin;
}

ResampleRTWindow::~ResampleRTWindow()
{
}

void ResampleRTWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, "Scale by amount:"));
	y += title->get_h() + plugin->get_theme()->widget_border;

	scale = new ResampleRTScale(this,
		plugin, 
		x, 
		y);
	scale->create_objects();
	show_window();
}






ResampleRTScale::ResampleRTScale(ResampleRTWindow *window,
	ResampleRT *plugin, 
	int x, 
	int y)
 : BC_TumbleTextBox(window,
 	plugin->config.scale,
	(float)0.0001,
	(float)1000,
 	x, 
	y, 
	100)
{
	this->plugin = plugin;
	set_increment(0.001);
}

int ResampleRTScale::handle_event()
{
	plugin->config.scale = atof(get_text());
	plugin->send_configure_change();
	return 1;
}






ResampleRTResample::ResampleRTResample(ResampleRT *plugin)
 : Resample()
{
	this->plugin = plugin;
}

// To get the keyframes to work, resampling is always done in the forward
// direction with the plugin converting to reverse.
int ResampleRTResample::read_samples(Samples *buffer, 
	int64_t start, 
	int64_t len)
{
	int result = plugin->read_samples(buffer,
		0,
		plugin->get_samplerate(),
		plugin->source_start,
		len);

//printf("ResampleRTResample::read_samples %lld %lld %lld %d\n", start, plugin->source_start, len, result);
	if(plugin->get_direction() == PLAY_FORWARD)
		plugin->source_start += len;
	else
		plugin->source_start -= len;
	
	return result;
}






ResampleRT::ResampleRT(PluginServer *server)
 : PluginAClient(server)
{
	resample = 0;
	need_reconfigure = 1;
	prev_scale = 0;
	dest_start = -1;
}


ResampleRT::~ResampleRT()
{
	delete resample;
}

const char* ResampleRT::plugin_title() { return N_("ResampleRT"); }
int ResampleRT::is_realtime() { return 1; }
int ResampleRT::is_synthesis() { return 1; }

#include "picon_png.h"
NEW_PICON_MACRO(ResampleRT)

NEW_WINDOW_MACRO(ResampleRT, ResampleRTWindow)

LOAD_CONFIGURATION_MACRO(ResampleRT, ResampleRTConfig)


int ResampleRT::process_buffer(int64_t size, 
	Samples *buffer,
	int64_t start_position,
	int sample_rate)
{
	if(!resample) resample = new ResampleRTResample(this);

	need_reconfigure |= load_configuration();
	
	
	if(start_position != dest_start) need_reconfigure = 1;
	dest_start = start_position;

// Get start position of the input.
// Sample 0 is the keyframe position
	if(need_reconfigure)
	{
		int64_t prev_position = edl_to_local(
			get_prev_keyframe(
				get_source_position())->position);

		if(prev_position == 0)
		{
			prev_position = get_source_start();
		}

		source_start = (int64_t)((start_position - prev_position) * 
			config.scale) + prev_position;

		resample->reset();
		need_reconfigure = 0;
	}

	resample->resample(buffer,
		size,
		(int)1000000,
		(int)(1000000 / config.scale),
		start_position,
		get_direction());	

	if(get_direction() == PLAY_FORWARD)
		dest_start += size;
	else
		dest_start -= size;

	return 0;
}

void ResampleRT::render_stop()
{
	need_reconfigure = 1;
}




void ResampleRT::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("RESAMPLERT");
	output.tag.set_property("SCALE", config.scale);
	output.append_tag();
	output.terminate_string();
}

void ResampleRT::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("RESAMPLERT"))
		{
			config.scale = input.tag.get_property("SCALE", config.scale);
		}
	}
}

void ResampleRT::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("ResampleRT::update_gui");
			((ResampleRTWindow*)thread->window)->scale->update((float)config.scale);
			thread->window->unlock_window();
		}
	}
}





