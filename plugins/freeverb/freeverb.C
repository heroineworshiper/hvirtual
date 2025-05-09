
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
#include "guicast.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "pluginaclient.h"
#include "revmodel.hpp"
#include "samples.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>







class FreeverbEffect;

class FreeverbConfig
{
public:
	FreeverbConfig();


	int equivalent(FreeverbConfig &that);
	void copy_from(FreeverbConfig &that);
	void interpolate(FreeverbConfig &prev, 
		FreeverbConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);


	float gain;
	float roomsize;
	float damp;
	float wet;
	float dry;
	float width;
	float mode;
};


class FreeverbGain : public BC_FPot
{
public:
	FreeverbGain(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbRoomsize : public BC_FPot
{
public:
	FreeverbRoomsize(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbDamp : public BC_FPot
{
public:
	FreeverbDamp(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbWet : public BC_FPot
{
public:
	FreeverbWet(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbDry : public BC_FPot
{
public:
	FreeverbDry(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbWidth : public BC_FPot
{
public:
	FreeverbWidth(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};

class FreeverbMode : public BC_CheckBox
{
public:
	FreeverbMode(FreeverbEffect *plugin, int x, int y);
	int handle_event();
	FreeverbEffect *plugin;
};



class FreeverbWindow : public PluginClientWindow
{
public:
	FreeverbWindow(FreeverbEffect *plugin);
	void create_objects();

	FreeverbEffect *plugin;
	
	FreeverbGain *gain;
	FreeverbRoomsize *roomsize;
	FreeverbDamp *damp;
	FreeverbWet *wet;
	FreeverbDry *dry;
	FreeverbWidth *width;
	FreeverbMode *mode;
};





class FreeverbEffect : public PluginAClient
{
public:
	FreeverbEffect(PluginServer *server);
	~FreeverbEffect();

	PLUGIN_CLASS_MEMBERS(FreeverbConfig)
	int is_realtime();
	int is_multichannel();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(int64_t size, Samples **input_ptr, Samples **output_ptr);




	void update_gui();


	revmodel *engine;
	float **temp;
	float **temp_out;
	int temp_allocated;
};




REGISTER_PLUGIN(FreeverbEffect)










FreeverbGain::FreeverbGain(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.gain, INFINITYGAIN, 6)
{
	this->plugin = plugin;
	set_precision(.1);
}

int FreeverbGain::handle_event()
{
	plugin->config.gain = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbRoomsize::FreeverbRoomsize(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.roomsize, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbRoomsize::handle_event()
{
	plugin->config.roomsize = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbDamp::FreeverbDamp(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.damp, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbDamp::handle_event()
{
	plugin->config.damp = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbWet::FreeverbWet(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.wet, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbWet::handle_event()
{
	plugin->config.wet = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbDry::FreeverbDry(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.dry, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbDry::handle_event()
{
	plugin->config.dry = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbWidth::FreeverbWidth(FreeverbEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.width, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	set_precision(.01);
}

int FreeverbWidth::handle_event()
{
	plugin->config.width = get_value();
	plugin->send_configure_change();
	return 1;
}

FreeverbMode::FreeverbMode(FreeverbEffect *plugin, int x, int y)
 : BC_CheckBox(x, y, (int)plugin->config.mode, _("Freeze"))
{
	this->plugin = plugin;
}

int FreeverbMode::handle_event()
{
	plugin->config.mode = get_value();
	plugin->send_configure_change();
	return 1;
}











FreeverbWindow::FreeverbWindow(FreeverbEffect *plugin)
 : PluginClientWindow(plugin, 
	DP(180), 
	DP(250), 
	DP(180), 
	DP(250),
	0)
{
	this->plugin = plugin;
}

void FreeverbWindow::create_objects()
{
	int x1 = DP(10), x2 = DP(100), x3 = DP(135), y1 = DP(10), y2 = DP(20), margin = DP(30);

	add_subwindow(new BC_Title(x1, y2, _("Gain:")));
	add_subwindow(gain = new FreeverbGain(plugin, x3, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Roomsize:")));
	add_subwindow(roomsize = new FreeverbRoomsize(plugin, x2, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Damp:")));
	add_subwindow(damp = new FreeverbDamp(plugin, x3, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Wet:")));
	add_subwindow(wet = new FreeverbWet(plugin, x2, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Dry:")));
	add_subwindow(dry = new FreeverbDry(plugin, x3, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(new BC_Title(x1, y2, _("Width:")));
	add_subwindow(width = new FreeverbWidth(plugin, x2, y1));
	y1 += margin;
	y2 += margin;
	add_subwindow(mode = new FreeverbMode(plugin, x1, y2));
	show_window();
}













FreeverbConfig::FreeverbConfig()
{
	gain = -6.0;
	wet = -6.0;
	dry = 0;
	roomsize = -6.0;
	damp = 0;
	width = 0;
	mode = 0;
}

int FreeverbConfig::equivalent(FreeverbConfig &that)
{
	return EQUIV(gain, that.gain) &&
		EQUIV(wet, that.wet) &&
		EQUIV(roomsize, that.roomsize) &&
		EQUIV(dry, that.dry) &&
		EQUIV(damp, that.damp) &&
		EQUIV(width, that.width) &&
		EQUIV(mode, that.mode);
}

void FreeverbConfig::copy_from(FreeverbConfig &that)
{
	gain = that.gain;
	wet = that.wet;
	roomsize = that.roomsize;
	dry = that.dry;
	damp = that.damp;
	width = that.width;
	mode = that.mode;
}

void FreeverbConfig::interpolate(FreeverbConfig &prev, 
	FreeverbConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	gain = prev.gain * prev_scale + next.gain * next_scale;
	wet = prev.wet * prev_scale + next.wet * next_scale;
	roomsize = prev.roomsize * prev_scale + next.roomsize * next_scale;
	dry = prev.dry * prev_scale + next.dry * next_scale;
	damp = prev.damp * prev_scale + next.damp * next_scale;
	width = prev.width * prev_scale + next.width * next_scale;
	mode = prev.mode;
}





























FreeverbEffect::FreeverbEffect(PluginServer *server)
 : PluginAClient(server)
{
	engine = 0;
	temp = 0;
	temp_out = 0;
	temp_allocated = 0;
	
}

FreeverbEffect::~FreeverbEffect()
{
	if(engine) delete engine;
	if(temp)
	{
		for(int i = 0; i < total_in_buffers; i++)
		{
			delete [] temp[i];
			delete [] temp_out[i];
		}
		delete [] temp;
		delete [] temp_out;
	}
	
}

NEW_PICON_MACRO(FreeverbEffect)
NEW_WINDOW_MACRO(FreeverbEffect, FreeverbWindow)


const char* FreeverbEffect::plugin_title() { return N_("Freeverb"); }
int FreeverbEffect::is_realtime() { return 1; }
int FreeverbEffect::is_multichannel() { return 1; }



void FreeverbEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("FREEVERB"))
			{
				config.gain = input.tag.get_property("GAIN", config.gain);
				config.roomsize = input.tag.get_property("ROOMSIZE", config.roomsize);
				config.damp = input.tag.get_property("DAMP", config.damp);
				config.wet = input.tag.get_property("WET", config.wet);
				config.dry = input.tag.get_property("DRY", config.dry);
				config.width = input.tag.get_property("WIDTH", config.width);
				config.mode = input.tag.get_property("MODE", config.mode);
			}
		}
	}
}

void FreeverbEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("FREEVERB");
	output.tag.set_property("GAIN", config.gain);
	output.tag.set_property("ROOMSIZE", config.roomsize);
	output.tag.set_property("DAMP", config.damp);
	output.tag.set_property("WET", config.wet);
	output.tag.set_property("DRY", config.dry);
	output.tag.set_property("WIDTH", config.width);
	output.tag.set_property("MODE", config.mode);
	output.append_tag();
	output.append_newline();

	output.terminate_string();
}


LOAD_CONFIGURATION_MACRO(FreeverbEffect, FreeverbConfig)

void FreeverbEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((FreeverbWindow*)thread->window)->gain->update(config.gain);
		((FreeverbWindow*)thread->window)->roomsize->update(config.roomsize);
		((FreeverbWindow*)thread->window)->damp->update(config.damp);
		((FreeverbWindow*)thread->window)->wet->update(config.wet);
		((FreeverbWindow*)thread->window)->dry->update(config.dry);
		((FreeverbWindow*)thread->window)->width->update(config.width);
		((FreeverbWindow*)thread->window)->mode->update((int)config.mode);
		thread->window->unlock_window();
	}
}

int FreeverbEffect::process_realtime(int64_t size, 
	Samples **input_ptr, 
	Samples **output_ptr)
{
	load_configuration();
	if(!engine) engine = new revmodel;

	engine->setroomsize(DB::fromdb(config.roomsize));
	engine->setdamp(DB::fromdb(config.damp));
	engine->setwet(DB::fromdb(config.wet));
	engine->setdry(DB::fromdb(config.dry));
	engine->setwidth(DB::fromdb(config.width));
	engine->setmode(config.mode);

// printf("FreeverbEffect::process_realtime %d %f %f %f %f %f %d\n",
// __LINE__,
// DB::fromdb(config.roomsize),
// DB::fromdb(config.damp),
// DB::fromdb(config.wet),
// DB::fromdb(config.dry),
// DB::fromdb(config.width),
// (int)config.mode);

	float gain_f = DB::fromdb(config.gain);

	if(size > temp_allocated)
	{
		if(temp)
		{
			for(int i = 0; i < total_in_buffers; i++)
			{
				delete [] temp[i];
				delete [] temp_out[i];
			}
			delete [] temp;
			delete [] temp_out;
		}
		temp = 0;
		temp_out = 0;
	}

	if(!temp)
	{
		temp_allocated = size * 2;
		temp = new float*[total_in_buffers];
		temp_out = new float*[total_in_buffers];
		for(int i = 0; i < total_in_buffers; i++)
		{
			temp[i] = new float[temp_allocated];
			temp_out[i] = new float[temp_allocated];
		}
	}

	for(int i = 0; i < 2 && i < total_in_buffers; i++)
	{
		float *out = temp[i];
		double *in = input_ptr[i]->get_data();
		for(int j = 0; j < size; j++)
		{
			out[j] = in[j];
		}
	}

	if(total_in_buffers < 2)
	{
		engine->processreplace(temp[0], 
			temp[0], 
			temp_out[0], 
			temp_out[0], 
			size, 
			1);
	}
	else
	{
// 2 channels max
		engine->processreplace(temp[0], 
			temp[1], 
			temp_out[0], 
			temp_out[1], 
			size, 
			1);
	}

	for(int i = 0; i < 2 && i < total_in_buffers; i++)
	{
		double *out = output_ptr[i]->get_data();
		float *in = temp_out[i];
		for(int j = 0; j < size; j++)
		{
			out[j] = gain_f * in[j];
		}
	}

	return 0;
}


