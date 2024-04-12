/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "leveleffect.h"
#include "picon_png.h"
#include "samples.h"
#include "theme.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"

#include <errno.h>
#include <math.h>
#include <string.h>
#include <unistd.h>










REGISTER_PLUGIN(SoundLevelEffect)









SoundLevelConfig::SoundLevelConfig()
{
	duration = 1.0;
}

void SoundLevelConfig::copy_from(SoundLevelConfig &that)
{
	duration = that.duration;
}

int SoundLevelConfig::equivalent(SoundLevelConfig &that)
{
	return EQUIV(duration, that.duration);
}

void SoundLevelConfig::interpolate(SoundLevelConfig &prev, 
	SoundLevelConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	duration = prev.duration;
}















SoundLevelDuration::SoundLevelDuration(SoundLevelEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, DP(180), DP(180), 0.0, 10.0, plugin->config.duration)
{
	this->plugin = plugin;
	set_precision(0.1);
}

int SoundLevelDuration::handle_event()
{
	plugin->config.duration = get_value();
	plugin->send_configure_change();
	return 1;
}



SoundLevelWindow::SoundLevelWindow(SoundLevelEffect *plugin)
 : PluginClientWindow(plugin, 
	DP(370), 
	DP(120), 
	DP(370), 
	DP(120),
	0)
{
	this->plugin = plugin;
}

void SoundLevelWindow::create_objects()
{
//printf("SoundLevelWindow::create_objects 1\n");
	int margin = plugin->get_theme()->widget_border;
	int x = margin, y = margin;


	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Duration (seconds):")));
	add_subwindow(duration = new SoundLevelDuration(plugin, x + title->get_w() + margin, y));
	y += DP(35);
	add_subwindow(title = new BC_Title(x, y, _("Max soundlevel (dB):")));
	add_subwindow(soundlevel_max = new BC_Title(x + title->get_w() + margin, y, "0.0"));
	y += DP(35);
	add_subwindow(title = new BC_Title(x, y, _("RMS soundlevel (dB):")));
	add_subwindow(soundlevel_rms = new BC_Title(x + title->get_w() + margin, y, "0.0"));

	show_window();
//printf("SoundLevelWindow::create_objects 2\n");
}





























SoundLevelEffect::SoundLevelEffect(PluginServer *server)
 : PluginAClient(server)
{
	
	reset();
}

SoundLevelEffect::~SoundLevelEffect()
{
	
}

NEW_PICON_MACRO(SoundLevelEffect)

LOAD_CONFIGURATION_MACRO(SoundLevelEffect, SoundLevelConfig)

NEW_WINDOW_MACRO(SoundLevelEffect, SoundLevelWindow)



void SoundLevelEffect::reset()
{
	rms_accum = 0;
	max_accum = 0;
	accum_size = 0;
    last_position = 0;
}

const char* SoundLevelEffect::plugin_title() { return N_("SoundLevel"); }
int SoundLevelEffect::is_realtime() { return 1; }


void SoundLevelEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SOUNDLEVEL"))
			{
				config.duration = input.tag.get_property("DURATION", config.duration);
			}
		}
	}
}

void SoundLevelEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("SOUNDLEVEL");
	output.tag.set_property("DURATION", config.duration);
	output.append_tag();
	output.append_newline();

	output.terminate_string();
}


void SoundLevelEffect::update_gui()
{
//printf("SoundLevelEffect::update_gui 1\n");
	if(thread)
	{
		int reconfigure = load_configuration();
        int total_frames = pending_gui_frames();
        
        if(reconfigure || total_frames)
        {
    		thread->window->lock_window();
        }
        
        if(reconfigure)
        {
    		((SoundLevelWindow*)thread->window)->duration->update(config.duration);
        }

        if(total_frames)
        {
            PluginClientFrame *frame = 0;
            for(int i = 0; i < total_frames; i++)
            {
                frame = get_gui_frame();
                if(i < total_frames - 1) delete frame;
            }
            
		    char string[BCTEXTLEN];
		    sprintf(string, "%.2f", DB::todb(frame->data[0]));
		    ((SoundLevelWindow*)thread->window)->soundlevel_max->update(string);
		    sprintf(string, "%.2f", DB::todb(frame->data[1]));
		    ((SoundLevelWindow*)thread->window)->soundlevel_rms->update(string);
		    thread->window->flush();
            delete frame;
        }
        
		thread->window->unlock_window();
	}
}

int SoundLevelEffect::process_realtime(int64_t size, 
    Samples *input_ptr, 
    Samples *output_ptr)
{
	load_configuration();
	if(last_position != get_source_position())
    {
        send_reset_gui_frames();
    }
    
	accum_size += size;
	for(int i = 0; i < size; i++)
	{
		double value = fabs(input_ptr->get_data()[i]);
		if(value > max_accum) max_accum = value;
		rms_accum += value * value;
	}

	if(accum_size > config.duration * PluginAClient::project_sample_rate)
	{
//printf("SoundLevelEffect::process_realtime 1 %f %d\n", rms_accum, accum_size);
		rms_accum = sqrt(rms_accum / accum_size);
        
        PluginClientFrame *frame = new PluginClientFrame();
        frame->data = new double[2];
        frame->data_size = 2;
		double arg[2];
		frame->data[0] = max_accum;
		frame->data[1] = rms_accum;
        frame->edl_position = get_playhead_position();
		add_gui_frame(frame);
        
		rms_accum = 0;
		max_accum = 0;
		accum_size = 0;
	}


    if(get_direction() == PLAY_FORWARD)
    {
        last_position = get_source_position() + size;
    }
    else
    {
        last_position = get_source_position() - size;
    }
	return 0;
}
