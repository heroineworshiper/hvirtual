
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
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "theme.h"
#include "transportque.h"

#include <string.h>

class ReframeRT;
class ReframeRTWindow;

class ReframeRTConfig
{
public:
	ReframeRTConfig();
	void boundaries();
	int equivalent(ReframeRTConfig &src);
	void copy_from(ReframeRTConfig &src);
	void interpolate(ReframeRTConfig &prev, 
		ReframeRTConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
// was scale
	double num;
	double denom;
	int stretch;
	int optic_flow;
};


class ReframeRTNum : public BC_TumbleTextBox
{
public:
	ReframeRTNum(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
};

class ReframeRTDenom : public BC_TumbleTextBox
{
public:
	ReframeRTDenom(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
};

class ReframeRTStretch : public BC_Radial
{
public:
	ReframeRTStretch(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};

class ReframeRTDownsample : public BC_Radial
{
public:
	ReframeRTDownsample(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};

class ReframeRTWindow : public PluginClientWindow
{
public:
	ReframeRTWindow(ReframeRT *plugin);
	~ReframeRTWindow();
	void create_objects();
	ReframeRT *plugin;
	ReframeRTNum *num;
	ReframeRTDenom *denom;
	ReframeRTStretch *stretch;
	ReframeRTDownsample *downsample;
};


class ReframeRT : public PluginVClient
{
public:
	ReframeRT(PluginServer *server);
	~ReframeRT();

	PLUGIN_CLASS_MEMBERS(ReframeRTConfig)

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int is_synthesis();
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
};







REGISTER_PLUGIN(ReframeRT);



ReframeRTConfig::ReframeRTConfig()
{
	num = 1.0;
	denom = 1.0;
	stretch = 0;
	optic_flow = 1;
}

int ReframeRTConfig::equivalent(ReframeRTConfig &src)
{
	return fabs(num - src.num) < 0.0001 &&
		fabs(denom - src.denom) < 0.0001 &&
		stretch == src.stretch;
}

void ReframeRTConfig::copy_from(ReframeRTConfig &src)
{
	this->num = src.num;
	this->denom = src.denom;
	this->stretch = src.stretch;
}

void ReframeRTConfig::interpolate(ReframeRTConfig &prev, 
	ReframeRTConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
//	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
//	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

//	this->scale = prev.scale * prev_scale + next.scale * next_scale;
	this->num = prev.num;
	this->denom = prev.denom;
	this->stretch = prev.stretch;
}

void ReframeRTConfig::boundaries()
{
	if(num < 0.0001) num = 0.0001;
	if(denom < 0.0001) denom = 0.0001;
}








ReframeRTWindow::ReframeRTWindow(ReframeRT *plugin)
 : PluginClientWindow(plugin, 
	DP(210), 
	DP(160), 
	DP(200), 
	DP(160), 
	0)
{
	this->plugin = plugin;
}

ReframeRTWindow::~ReframeRTWindow()
{
}

void ReframeRTWindow::create_objects()
{
	int x = plugin->get_theme()->window_border;
	int y = plugin->get_theme()->window_border;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Input frames:")));
	y += title->get_h() + plugin->get_theme()->widget_border;
	num = new ReframeRTNum(plugin, 
		this,
		x, 
		y);
	num->create_objects();
	num->set_increment(1.0);

	y += num->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(title = new BC_Title(x, y, _("Output frames:")));
	y += title->get_h() + plugin->get_theme()->widget_border;
	denom = new ReframeRTDenom(plugin, 
		this,
		x, 
		y);
	denom->create_objects();
	denom->set_increment(1.0);
	
	
	y += denom->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(stretch = new ReframeRTStretch(plugin, 
		this,
		x, 
		y));
	y += stretch->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(downsample = new ReframeRTDownsample(plugin, 
		this,
		x, 
		y));
	show_window();
}








ReframeRTNum::ReframeRTNum(ReframeRT *plugin, 
	ReframeRTWindow *gui,
	int x, 
	int y)
 : BC_TumbleTextBox(gui,
 	(float)plugin->config.num,
	(float)1,
	(float)1000000,
 	x, 
	y, 
	gui->get_w() - plugin->get_theme()->window_border * 3)
{
	this->plugin = plugin;
}

int ReframeRTNum::handle_event()
{
	plugin->config.num = atof(get_text());
	plugin->config.boundaries();
	plugin->send_configure_change();
	return 1;
}







ReframeRTDenom::ReframeRTDenom(ReframeRT *plugin, 
	ReframeRTWindow *gui,
	int x, 
	int y)
 : BC_TumbleTextBox(gui,
 	(float)plugin->config.denom,
	(float)1,
	(float)1000000,
 	x, 
	y, 
	gui->get_w() - plugin->get_theme()->window_border * 3)
{
	this->plugin = plugin;
}

int ReframeRTDenom::handle_event()
{
	plugin->config.denom = atof(get_text());
	plugin->config.boundaries();
	plugin->send_configure_change();
	return 1;
}







ReframeRTStretch::ReframeRTStretch(ReframeRT *plugin,
	ReframeRTWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, plugin->config.stretch, _("Stretch"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ReframeRTStretch::handle_event()
{
	plugin->config.stretch = get_value();
	gui->downsample->update(!get_value());
	plugin->send_configure_change();
	return 1;
}


ReframeRTDownsample::ReframeRTDownsample(ReframeRT *plugin,
	ReframeRTWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, !plugin->config.stretch, _("Downsample"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ReframeRTDownsample::handle_event()
{
	plugin->config.stretch = !get_value();
	gui->stretch->update(!get_value());
	plugin->send_configure_change();
	return 1;
}






ReframeRT::ReframeRT(PluginServer *server)
 : PluginVClient(server)
{
	
}


ReframeRT::~ReframeRT()
{
	
}

const char* ReframeRT::plugin_title() { return N_("ReframeRT"); }
int ReframeRT::is_realtime() { return 1; }
int ReframeRT::is_synthesis() { return 1; }

#include "picon_png.h"
NEW_PICON_MACRO(ReframeRT)

NEW_WINDOW_MACRO(ReframeRT, ReframeRTWindow)

LOAD_CONFIGURATION_MACRO(ReframeRT, ReframeRTConfig)

int ReframeRT::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int64_t input_frame = get_source_start();
	KeyFrame *tmp_keyframe, *next_keyframe = get_prev_keyframe(get_source_start());
	int64_t tmp_position, next_position;
	int64_t segment_len;
	double input_rate = frame_rate;
	int is_current_keyframe;

// if there are no keyframes, the default keyframe is used, and its position is always 0;
// if there are keyframes, the first keyframe can be after the effect start (and it controls settings before it)
// so let's calculate using a fake keyframe with the same settings but position == effect start
	KeyFrame *fake_keyframe = new KeyFrame();
	fake_keyframe->copy_from(next_keyframe);
	fake_keyframe->position = local_to_edl(get_source_start());
	next_keyframe = fake_keyframe;

	// calculate input_frame accounting for all previous keyframes
	do
	{
		tmp_keyframe = next_keyframe;
		next_keyframe = get_next_keyframe(tmp_keyframe->position+1, 0);

		tmp_position = edl_to_local(tmp_keyframe->position);
		next_position = edl_to_local(next_keyframe->position);

		read_data(tmp_keyframe);

		is_current_keyframe =
			next_position > start_position // the next keyframe is after the current position
			|| next_keyframe->position == tmp_keyframe->position // there are no more keyframes
			|| !next_keyframe->position; // there are no keyframes at all

		if (is_current_keyframe)
			next_position = start_position;

		segment_len = next_position - tmp_position;

		input_frame += (int64_t)(segment_len * config.num / config.denom);
	} while (!is_current_keyframe);

// Change rate
// Don't think this is what you want for downsampling.
	if (!config.stretch)
	{
		input_rate *= config.num / config.denom;

	}

// printf("ReframeRT::process_buffer %d %lld %f %lld %f\n", 
// __LINE__, 
// start_position, 
// frame_rate,
// input_frame,
// input_rate);

	read_frame(frame,
		0,
		input_frame,
		input_rate,
		0);

	delete fake_keyframe;

	return 0;
}





void ReframeRT::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("REFRAMERT");
// for backwards compatability, we call num scale
	output.tag.set_property("SCALE", config.num);
	output.tag.set_property("DENOM", config.denom);
	output.tag.set_property("STRETCH", config.stretch);
	output.append_tag();
	output.terminate_string();
}

void ReframeRT::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("REFRAMERT"))
		{
// for backwards compatability, we call num scale
			config.num = input.tag.get_property("SCALE", config.num);
			config.denom = input.tag.get_property("DENOM", config.denom);
			config.stretch = input.tag.get_property("STRETCH", config.stretch);
		}
	}
}

void ReframeRT::update_gui()
{
	if(thread)
	{
		int changed = load_configuration();

		if(changed)
		{
			thread->window->lock_window("ReframeRT::update_gui");
			((ReframeRTWindow*)thread->window)->num->update((float)config.num);
			((ReframeRTWindow*)thread->window)->denom->update((float)config.denom);
			((ReframeRTWindow*)thread->window)->stretch->update(config.stretch);
			((ReframeRTWindow*)thread->window)->downsample->update(!config.stretch);
			thread->window->unlock_window();
		}
	}
}





