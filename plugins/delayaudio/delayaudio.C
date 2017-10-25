
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
#include "delayaudio.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "samples.h"
#include "theme.h"
#include "vframe.h"

#include <string.h>



PluginClient* new_plugin(PluginServer *server)
{
	return new DelayAudio(server);
}


DelayAudio::DelayAudio(PluginServer *server)
 : PluginAClient(server)
{
	reset();
}

DelayAudio::~DelayAudio()
{

	
	if(buffer) delete buffer;
}



VFrame* DelayAudio::new_picon()
{
	return new VFrame(picon_png);
}

NEW_WINDOW_MACRO(DelayAudio, DelayAudioWindow)

const char* DelayAudio::plugin_title() { return N_("Delay audio"); }
int DelayAudio::is_realtime() { return 1; }


void DelayAudio::reset()
{
	need_reconfigure = 1;
	buffer = 0;
}

int DelayAudio::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	
	DelayAudioConfig old_config;
	old_config.copy_from(config);
 	read_data(prev_keyframe);

 	if(!old_config.equivalent(config))
 	{
// Reconfigure
		need_reconfigure = 1;
		return 1;
	}
	return 0;
}


void DelayAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DELAYAUDIO"))
			{
				config.length = input.tag.get_property("LENGTH", (double)config.length);
			}
		}
	}
}


void DelayAudio::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("DELAYAUDIO");
	output.tag.set_property("LENGTH", (double)config.length);
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void DelayAudio::reconfigure()
{
	input_start = (int64_t)(config.length * PluginAClient::project_sample_rate + 0.5);
	int64_t new_allocation = input_start + PluginClient::in_buffer_size;
	Samples *new_buffer = new Samples(new_allocation);
	bzero(new_buffer->get_data(), sizeof(double) * new_allocation);

// printf("DelayAudio::reconfigure %f %d %d %d\n", 
// config.length, 
// PluginAClient::project_sample_rate, 
// PluginClient::in_buffer_size,
// new_allocation);
// 


	if(buffer)
	{
		int size = MIN(new_allocation, allocation);

		memcpy(new_buffer->get_data(), 
			buffer->get_data(), 
			(size - PluginClient::in_buffer_size) * sizeof(double));
		delete buffer;
	}

	allocation = new_allocation;
	buffer = new_buffer;
	allocation = new_allocation;
	need_reconfigure = 0;
}

int DelayAudio::process_realtime(int64_t size, Samples *input_ptr, Samples *output_ptr)
{

	load_configuration();
	if(need_reconfigure) reconfigure();

// printf("DelayAudio::process_realtime %d %d\n",
// input_start, size);



	memcpy(buffer->get_data() + input_start, input_ptr->get_data(), size * sizeof(double));
	memcpy(output_ptr->get_data(), buffer->get_data(), size * sizeof(double));

	for(int i = size, j = 0; i < allocation; i++, j++)
	{
		buffer->get_data()[j] = buffer->get_data()[i];
	}

	return 0;
}




void DelayAudio::update_gui()
{
	if(thread)
	{
		load_configuration();
		((DelayAudioWindow*)thread->window)->lock_window();
		((DelayAudioWindow*)thread->window)->update_gui();
		((DelayAudioWindow*)thread->window)->unlock_window();
	}
}
















DelayAudioWindow::DelayAudioWindow(DelayAudio *plugin)
 : PluginClientWindow(plugin, 
	DP(200), 
	DP(80), 
	DP(200), 
	DP(80), 
	0)
{
	this->plugin = plugin;
}

DelayAudioWindow::~DelayAudioWindow()
{
}

void DelayAudioWindow::create_objects()
{
	BC_Title *title;
	int margin = client->get_theme()->widget_border;
	int y = margin;

	add_subwindow(title = new BC_Title(margin, y, _("Delay seconds:")));
	y += title->get_h() + margin;
	length = new DelayAudioTextBox(
		plugin, 
		this,
		margin, 
		y);
	length->create_objects();
	update_gui();
	show_window();
}

void DelayAudioWindow::update_gui()
{
	char string[BCTEXTLEN];
	sprintf(string, "%.04f", plugin->config.length);
	length->update(string);
}












DelayAudioTextBox::DelayAudioTextBox(
	DelayAudio *plugin, 
	DelayAudioWindow *window,
	int x, 
	int y)
 : BC_TumbleTextBox(window,
 	(float)plugin->config.length,
	(float)0,
	(float)10,
 	x, 
	y, 
	DP(100))
{
	this->plugin = plugin;
	set_increment(0.01);
}

DelayAudioTextBox::~DelayAudioTextBox()
{
}

int DelayAudioTextBox::handle_event()
{
	plugin->config.length = atof(get_text());
	if(plugin->config.length < 0) plugin->config.length = 0;
	plugin->send_configure_change();
	return 1;
}








DelayAudioConfig::DelayAudioConfig()
{
	length = 1;
}
	
int DelayAudioConfig::equivalent(DelayAudioConfig &that)
{
	return(EQUIV(this->length, that.length));
}

void DelayAudioConfig::copy_from(DelayAudioConfig &that)
{
	this->length = that.length;
}





