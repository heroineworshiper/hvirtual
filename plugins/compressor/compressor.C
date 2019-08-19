
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "clip.h"
#include "compressor.h"
#include "cursors.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "samples.h"
#include "theme.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>





REGISTER_PLUGIN(CompressorEffect)






// More potential compressor algorithms:
// Use single reaction time parameter.  Negative reaction time uses 
// readahead.  Positive reaction time uses slope.

// Smooth input stage if readahead.
// Determine slope from current smoothed sample to every sample in readahead area.
// Once highest slope is found, count of number of samples remaining until it is
// reached.  Only search after this count for the next highest slope.
// Use highest slope to determine smoothed value.

// Smooth input stage if not readahead.
// For every sample, calculate slope needed to reach current sample from 
// current smoothed value in the reaction time.  If higher than current slope,
// make it the current slope and count number of samples remaining until it is
// reached.  If this count is met and no higher slopes are found, base slope
// on current sample when count is met.

// Gain stage.
// For every sample, calculate gain from smoothed input value.





CompressorEffect::CompressorEffect(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	
}

CompressorEffect::~CompressorEffect()
{
	
	delete_dsp();
	levels.remove_all();
}

void CompressorEffect::delete_dsp()
{
	if(input_buffer)
	{
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
			delete input_buffer[i];
		delete [] input_buffer;
	}


	input_buffer = 0;
	input_size = 0;
	input_allocated = 0;
}


void CompressorEffect::reset()
{
	input_buffer = 0;
	input_size = 0;
	input_allocated = 0;
	input_start = 0;

	next_target = 1.0;
	previous_target = 1.0;
	target_samples = 1;
	target_current_sample = -1;
	current_value = 1.0;
}

const char* CompressorEffect::plugin_title() { return N_("Compressor"); }
int CompressorEffect::is_realtime() { return 1; }
int CompressorEffect::is_multichannel() { return 1; }



void CompressorEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
    BandConfig *band_config = &config.bands[0];
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	band_config->levels.remove_all();
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("COMPRESSOR"))
			{
				band_config->reaction_len = input.tag.get_property("REACTION_LEN", band_config->reaction_len);
				band_config->decay_len = input.tag.get_property("DECAY_LEN", band_config->decay_len);
				config.trigger = input.tag.get_property("TRIGGER", config.trigger);
				config.smoothing_only = input.tag.get_property("SMOOTHING_ONLY", config.smoothing_only);
				config.input = input.tag.get_property("INPUT", config.input);
			}
			else
			if(input.tag.title_is("LEVEL2"))
			{
				double x = input.tag.get_property("X", (double)0);
				double y = input.tag.get_property("Y", (double)0);
				compressor_point_t point = { x, y };

				band_config->levels.append(point);
			}
		}
	}
}

void CompressorEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
    BandConfig *band_config = &config.bands[0];
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("COMPRESSOR");
	output.tag.set_property("TRIGGER", config.trigger);
	output.tag.set_property("SMOOTHING_ONLY", config.smoothing_only);
	output.tag.set_property("INPUT", config.input);
	output.tag.set_property("REACTION_LEN", band_config->reaction_len);
	output.tag.set_property("DECAY_LEN", band_config->decay_len);
	output.append_tag();
	output.append_newline();


	for(int i = 0; i < band_config->levels.total; i++)
	{
		output.tag.set_title("LEVEL2");
		output.tag.set_property("X", band_config->levels.values[i].x);
		output.tag.set_property("Y", band_config->levels.values[i].y);

		output.append_tag();
		output.append_newline();
	}

	output.terminate_string();
}


void CompressorEffect::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("CompressorEffect::update_gui");
			((CompressorWindow*)thread->window)->update();
			thread->window->unlock_window();
		}
	}
}


NEW_PICON_MACRO(CompressorEffect)
LOAD_CONFIGURATION_MACRO(CompressorEffect, CompressorConfig)
NEW_WINDOW_MACRO(CompressorEffect, CompressorWindow)




int CompressorEffect::process_buffer(int64_t size, 
		Samples **buffer,
		int64_t start_position,
		int sample_rate)
{
    BandConfig *band_config = &config.bands[0];
	load_configuration();

// Calculate linear transfer from db 
	levels.remove_all();
	for(int i = 0; i < band_config->levels.total; i++)
	{
		levels.append();
		levels.values[i].x = DB::fromdb(band_config->levels.values[i].x);
		levels.values[i].y = DB::fromdb(band_config->levels.values[i].y);
	}
	min_x = DB::fromdb(config.min_db);
	min_y = DB::fromdb(config.min_db);
	max_x = 1.0;
	max_y = 1.0;


	int reaction_samples = (int)(band_config->reaction_len * sample_rate + 0.5);
	int decay_samples = (int)(band_config->decay_len * sample_rate + 0.5);
	int trigger = CLIP(config.trigger, 0, PluginAClient::total_in_buffers - 1);

	CLAMP(reaction_samples, -1000000, 1000000);
	CLAMP(decay_samples, reaction_samples, 1000000);
	CLAMP(decay_samples, 1, 1000000);
	if(labs(reaction_samples) < 1) reaction_samples = 1;
	if(labs(decay_samples) < 1) decay_samples = 1;

	int total_buffers = get_total_buffers();
	if(reaction_samples >= 0)
	{
		if(target_current_sample < 0) target_current_sample = reaction_samples;
		for(int i = 0; i < total_buffers; i++)
		{
			read_samples(buffer[i],
				i,
				sample_rate,
				start_position,
				size);
		}

		double current_slope = (next_target - previous_target) / 
			reaction_samples;
		double *trigger_buffer = buffer[trigger]->get_data();
		for(int i = 0; i < size; i++)
		{
// Get slope required to reach current sample from smoothed sample over reaction
// length.
			double sample;
			switch(config.input)
			{
				case CompressorConfig::MAX:
				{
					double max = 0;
					for(int j = 0; j < total_buffers; j++)
					{
						sample = fabs(buffer[j]->get_data()[i]);
						if(sample > max) max = sample;
					}
					sample = max;
					break;
				}

				case CompressorConfig::TRIGGER:
					sample = fabs(trigger_buffer[i]);
					break;
				
				case CompressorConfig::SUM:
				{
					double max = 0;
					for(int j = 0; j < total_buffers; j++)
					{
						sample = fabs(buffer[j]->get_data()[i]);
						max += sample;
					}
					sample = max;
					break;
				}
			}

			double new_slope = (sample - current_value) /
				reaction_samples;

// Slope greater than current slope
			if(new_slope >= current_slope && 
				(current_slope >= 0 ||
				new_slope >= 0))
			{
				next_target = sample;
				previous_target = current_value;
				target_current_sample = 0;
				target_samples = reaction_samples;
				current_slope = new_slope;
			}
			else
			if(sample > next_target && current_slope < 0)
			{
				next_target = sample;
				previous_target = current_value;
				target_current_sample = 0;
				target_samples = decay_samples;
				current_slope = (sample - current_value) / decay_samples;
			}
// Current smoothed sample came up without finding higher slope
			if(target_current_sample >= target_samples)
			{
				next_target = sample;
				previous_target = current_value;
				target_current_sample = 0;
				target_samples = decay_samples;
				current_slope = (sample - current_value) / decay_samples;
			}

// Update current value and store gain
			current_value = (next_target * target_current_sample + 
				previous_target * (target_samples - target_current_sample)) /
				target_samples;

			target_current_sample++;

			if(config.smoothing_only)
			{
				for(int j = 0; j < total_buffers; j++)
					buffer[j]->get_data()[i] = current_value;
			}
			else
			{
				double gain = config.calculate_gain(0, current_value);
				for(int j = 0; j < total_buffers; j++)
				{
					buffer[j]->get_data()[i] *= gain;
				}
			}
		}
	}
	else
	{
		if(target_current_sample < 0) target_current_sample = target_samples;
		int64_t preview_samples = -reaction_samples;

// Start of new buffer is outside the current buffer.  Start buffer over.
		if(start_position < input_start ||
			start_position >= input_start + input_size)
		{
			input_size = 0;
			input_start = start_position;
		}
		else
// Shift current buffer so the buffer starts on start_position
		if(start_position > input_start &&
			start_position < input_start + input_size)
		{
			if(input_buffer)
			{
				int len = input_start + input_size - start_position;
				for(int i = 0; i < total_buffers; i++)
				{
					memcpy(input_buffer[i]->get_data(),
						input_buffer[i]->get_data() + (start_position - input_start),
						len * sizeof(double));
				}
				input_size = len;
				input_start = start_position;
			}
		}

// Expand buffer to handle preview size
		if(size + preview_samples > input_allocated)
		{
			Samples **new_input_buffer = new Samples*[total_buffers];
			for(int i = 0; i < total_buffers; i++)
			{
				new_input_buffer[i] = new Samples(size + preview_samples);
				if(input_buffer)
				{
					memcpy(new_input_buffer[i]->get_data(), 
						input_buffer[i]->get_data(), 
						input_size * sizeof(double));
					delete input_buffer[i];
				}
			}
			if(input_buffer) delete [] input_buffer;

			input_allocated = size + preview_samples;
			input_buffer = new_input_buffer;
		}

// Append data to input buffer to construct readahead area.
#define MAX_FRAGMENT_SIZE 131072
		while(input_size < size + preview_samples)
		{
			int fragment_size = MAX_FRAGMENT_SIZE;
			if(fragment_size + input_size > size + preview_samples)
				fragment_size = size + preview_samples - input_size;
			for(int i = 0; i < total_buffers; i++)
			{
				input_buffer[i]->set_offset(input_size);
//printf("CompressorEffect::process_buffer %d %p %d\n", __LINE__, input_buffer[i], input_size);
				read_samples(input_buffer[i],
					i,
					sample_rate,
					input_start + input_size,
					fragment_size);
				input_buffer[i]->set_offset(0);
			}
			input_size += fragment_size;
		}


		double current_slope = (next_target - previous_target) /
			target_samples;
		double *trigger_buffer = input_buffer[trigger]->get_data();
		for(int i = 0; i < size; i++)
		{
// Get slope from current sample to every sample in preview_samples.
// Take highest one or first one after target_samples are up.

// For optimization, calculate the first slope we really need.
// Assume every slope up to the end of preview_samples has been calculated and
// found <= to current slope.
            int first_slope = preview_samples - 1;
// Need new slope immediately
			if(target_current_sample >= target_samples)
				first_slope = 1;
			for(int j = first_slope; 
				j < preview_samples; 
				j++)
			{
				double sample;
				switch(config.input)
				{
					case CompressorConfig::MAX:
					{
						double max = 0;
						for(int k = 0; k < total_buffers; k++)
						{
							sample = fabs(input_buffer[k]->get_data()[i + j]);
							if(sample > max) max = sample;
						}
						sample = max;
						break;
					}

					case CompressorConfig::TRIGGER:
						sample = fabs(trigger_buffer[i + j]);
						break;

					case CompressorConfig::SUM:
					{
						double max = 0;
						for(int k = 0; k < total_buffers; k++)
						{
							sample = fabs(input_buffer[k]->get_data()[i + j]);
							max += sample;
						}
						sample = max;
						break;
					}
				}






				double new_slope = (sample - current_value) /
					j;
// Got equal or higher slope
				if(new_slope >= current_slope && 
					(current_slope >= 0 ||
					new_slope >= 0))
				{
					target_current_sample = 0;
					target_samples = j;
					current_slope = new_slope;
					next_target = sample;
					previous_target = current_value;
				}
				else
				if(sample > next_target && current_slope < 0)
				{
					target_current_sample = 0;
					target_samples = decay_samples;
					current_slope = (sample - current_value) /
						decay_samples;
					next_target = sample;
					previous_target = current_value;
				}

// Hit end of current slope range without finding higher slope
				if(target_current_sample >= target_samples)
				{
					target_current_sample = 0;
					target_samples = decay_samples;
					current_slope = (sample - current_value) / decay_samples;
					next_target = sample;
					previous_target = current_value;
				}
			}

// Update current value and multiply gain
			current_value = (next_target * target_current_sample +
				previous_target * (target_samples - target_current_sample)) /
				target_samples;

			target_current_sample++;

			if(config.smoothing_only)
			{
				for(int j = 0; j < total_buffers; j++)
				{
					buffer[j]->get_data()[i] = current_value;
				}
			}
			else
			{
				double gain = config.calculate_gain(0, current_value);
				for(int j = 0; j < total_buffers; j++)
				{
					buffer[j]->get_data()[i] = input_buffer[j]->get_data()[i] * gain;
				}
			}
		}



	}





	return 0;
}










CompressorConfig::CompressorConfig()
 : CompressorConfigBase(1)
{
}

void CompressorConfig::copy_from(CompressorConfig &that)
{
    CompressorConfigBase::copy_from(that);
}

int CompressorConfig::equivalent(CompressorConfig &that)
{
    return CompressorConfigBase::equivalent(that);
	return 1;
}

void CompressorConfig::interpolate(CompressorConfig &prev, 
	CompressorConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	copy_from(prev);
}



























CompressorWindow::CompressorWindow(CompressorEffect *plugin)
 : PluginClientWindow(plugin,
	DP(650), 
	DP(480), 
	DP(650), 
	DP(480),
	0)
{
	this->plugin = plugin;
}

CompressorWindow::~CompressorWindow()
{
    delete reaction;
    delete decay;
    delete trigger;
    delete x_text;
    delete y_text;
}

void CompressorWindow::create_objects()
{
    int margin = client->get_theme()->widget_border;
	int x = DP(35), y = margin;
	int control_margin = DP(130);
    BC_Title *title;

    add_subwindow(title = new BC_Title(margin, y, _("Sound level (Press shift to snap to grid):")));
    y += title->get_h() + 1;
	add_subwindow(canvas = new CompressorCanvas(plugin, 
        this,
		x, 
		y, 
		get_w() - x - control_margin - DP(10), 
		get_h() - y - DP(70)));
	x = get_w() - control_margin;
	add_subwindow(new BC_Title(x, y, _("Attack secs:")));
	y += title->get_h() + margin;
	reaction = new CompressorReaction(plugin, this, x, y);
    reaction->create_objects();
	y += reaction->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Release secs:")));
	y += title->get_h() + margin;
	decay = new CompressorDecay(plugin, this, x, y);
    decay->create_objects();
	y += decay->get_h() + margin;


	add_subwindow(title = new BC_Title(x, y, _("Trigger Type:")));
	y += title->get_h() + margin;
	add_subwindow(input = new CompressorInput(plugin, x, y));
	input->create_objects();
	y += input->get_h() + margin;


	add_subwindow(title = new BC_Title(x, y, _("Trigger:")));
	y += title->get_h() + margin;
	trigger = new CompressorTrigger(plugin, this, x, y);
    trigger->create_objects();
	if(plugin->config.input != CompressorConfig::TRIGGER) trigger->disable();
	y += trigger->get_h() + margin;
    
    
	add_subwindow(smooth = new CompressorSmooth(plugin, x, y));
	y += smooth->get_h() + margin;
    
	add_subwindow(title = new BC_Title(x, y, _("Output:")));
	y += title->get_h();
	y_text = new CompressorY(plugin, this, x, y);
    y_text->create_objects();
	y += y_text->get_h() + margin;
    
    
	add_subwindow(title = new BC_Title(x, y, _("Input:")));
	y += title->get_h();
	x_text = new CompressorX(plugin, this, x, y);
    x_text->create_objects();
	y += x_text->get_h() + margin;
    
    
	add_subwindow(clear = new CompressorClear(plugin, x, y));
	x = DP(10);
	y = get_h() - DP(40);

    canvas->create_objects();
	canvas->update();
	show_window();
}


void CompressorWindow::update()
{
	update_textboxes();
	canvas->update();
}

void CompressorWindow::update_textboxes()
{
    BandConfig *band_config = &plugin->config.bands[0];

	if(atol(trigger->get_text()) != plugin->config.trigger)
		trigger->update((int64_t)plugin->config.trigger);
	if(strcmp(input->get_text(), CompressorInput::value_to_text(plugin->config.input)))
		input->set_text(CompressorInput::value_to_text(plugin->config.input));

	if(plugin->config.input != CompressorConfig::TRIGGER && trigger->get_enabled())
		trigger->disable();
	else
	if(plugin->config.input == CompressorConfig::TRIGGER && !trigger->get_enabled())
		trigger->enable();

	if(!EQUIV(atof(reaction->get_text()), band_config->reaction_len))
		reaction->update((float)band_config->reaction_len);
	if(!EQUIV(atof(decay->get_text()), band_config->decay_len))
		decay->update((float)band_config->decay_len);
	smooth->update(plugin->config.smoothing_only);
	if(canvas->current_operation == CompressorCanvas::DRAG)
	{
		x_text->update((float)band_config->levels.values[canvas->current_point].x);
		y_text->update((float)band_config->levels.values[canvas->current_point].y);
	}
}

int CompressorWindow::resize_event(int w, int h)
{
	return 1;
}








CompressorCanvas::CompressorCanvas(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y, 
    int w, 
    int h) 
 : CompressorCanvasBase(&plugin->config,
    plugin,
    window,
    x, 
    y, 
    w, 
    h)
{
}


void CompressorCanvas::update_window()
{
    ((CompressorWindow*)window)->update();
}






CompressorReaction::CompressorReaction(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y) 
 : BC_TumbleTextBox(window, 
    (float)plugin->config.bands[0].reaction_len,
    (float)MIN_ATTACK,
    (float)MAX_ATTACK,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
    set_increment(0.1);
    set_precision(2);
}

int CompressorReaction::handle_event()
{
	plugin->config.bands[0].reaction_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}





CompressorDecay::CompressorDecay(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
    (float)plugin->config.bands[0].decay_len,
    (float)MIN_DECAY,
    (float)MAX_DECAY,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
    set_increment(0.1);
    set_precision(2);
}
int CompressorDecay::handle_event()
{
	plugin->config.bands[0].decay_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}





CompressorX::CompressorX(CompressorEffect *plugin, 
    CompressorWindow *window,
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
    (float)0.0,
    plugin->config.min_db,
    plugin->config.max_db,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
    set_increment(0.1);
    set_precision(2);
}
int CompressorX::handle_event()
{
    BandConfig *band_config = &plugin->config.bands[0];
	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < band_config->levels.total)
	{
		band_config->levels.values[current_point].x = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->canvas->update();
		plugin->send_configure_change();
	}
	return 1;
}



CompressorY::CompressorY(CompressorEffect *plugin, 
    CompressorWindow *window,
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
    (float)0.0,
    plugin->config.min_db,
    plugin->config.max_db,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
    set_increment(0.1);
    set_precision(2);
}
int CompressorY::handle_event()
{
    BandConfig *band_config = &plugin->config.bands[0];
	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < band_config->levels.total)
	{
		band_config->levels.values[current_point].y = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->canvas->update();
		plugin->send_configure_change();
	}
	return 1;
}





CompressorTrigger::CompressorTrigger(CompressorEffect *plugin, 
    CompressorWindow *window,
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
    (int)plugin->config.trigger,
    MIN_TRIGGER,
    MAX_TRIGGER,
    x, 
    y, 
    DP(100))
{
	this->plugin = plugin;
    set_increment(1);
}
int CompressorTrigger::handle_event()
{
	plugin->config.trigger = atol(get_text());
	plugin->send_configure_change();
	return 1;
}







CompressorInput::CompressorInput(CompressorEffect *plugin, int x, int y) 
 : BC_PopupMenu(x, 
	y, 
	DP(100), 
	CompressorInput::value_to_text(plugin->config.input), 
	1)
{
	this->plugin = plugin;
}
int CompressorInput::handle_event()
{
	plugin->config.input = text_to_value(get_text());
	((CompressorWindow*)plugin->thread->window)->update();
	plugin->send_configure_change();
	return 1;
}

void CompressorInput::create_objects()
{
	for(int i = 0; i < 3; i++)
	{
		add_item(new BC_MenuItem(value_to_text(i)));
	}
}

const char* CompressorInput::value_to_text(int value)
{
	switch(value)
	{
		case CompressorConfig::TRIGGER: return "Trigger";
		case CompressorConfig::MAX: return "Maximum";
		case CompressorConfig::SUM: return "Total";
	}

	return "Trigger";
}

int CompressorInput::text_to_value(char *text)
{
	for(int i = 0; i < 3; i++)
	{
		if(!strcmp(value_to_text(i), text)) return i;
	}

	return CompressorConfig::TRIGGER;
}






CompressorClear::CompressorClear(CompressorEffect *plugin, int x, int y) 
 : BC_GenericButton(x, y, _("Clear"))
{
	this->plugin = plugin;
}

int CompressorClear::handle_event()
{
    BandConfig *band_config = &plugin->config.bands[0];
	band_config->levels.remove_all();
//plugin->config.dump();
	((CompressorWindow*)plugin->thread->window)->update();
	plugin->send_configure_change();
	return 1;
}



CompressorSmooth::CompressorSmooth(CompressorEffect *plugin, int x, int y) 
 : BC_CheckBox(x, y, plugin->config.smoothing_only, _("Smooth only"))
{
	this->plugin = plugin;
}

int CompressorSmooth::handle_event()
{
	plugin->config.smoothing_only = get_value();
	plugin->send_configure_change();
	return 1;
}




