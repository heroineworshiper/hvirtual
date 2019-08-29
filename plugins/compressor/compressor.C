
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
#include "bcsignals.h"
#include "clip.h"
#include "compressor.h"
#include "cursors.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "samples.h"
#include "theme.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>





REGISTER_PLUGIN(CompressorEffect)






// More potential compressor algorithms:
// Use single readahead time parameter.  Negative readahead time uses 
// readahead.  Positive readahead time uses slope.

// Smooth input stage if readahead.
// Determine slope from current smoothed sample to every sample in readahead area.
// Once highest slope is found, count of number of samples remaining until it is
// reached.  Only search after this count for the next highest slope.
// Use highest slope to determine smoothed value.

// Smooth input stage if not readahead.
// For every sample, calculate slope needed to reach current sample from 
// current smoothed value in the readahead time.  If higher than current slope,
// make it the current slope and count number of samples remaining until it is
// reached.  If this count is met and no higher slopes are found, base slope
// on current sample when count is met.

// Gain stage.
// For every sample, calculate gain from smoothed input value.





CompressorEffect::CompressorEffect(PluginServer *server)
 : PluginAClient(server)
{
	input_buffer = 0;
	input_size = 0;
	input_allocated = 0;
	input_start = 0;
    
	next_target = 1.0;
	prev_target = 1.0;
	target_samples = 0;
	target_current_sample = 0;
	need_reconfigure = 1;
}

CompressorEffect::~CompressorEffect()
{
	if(input_buffer)
	{
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
        {
			delete input_buffer[i];
        }
		delete [] input_buffer;
	}



	input_buffer = 0;
	input_size = 0;
	input_allocated = 0;

	levels.remove_all();
}



void CompressorEffect::allocate_input(int size)
{
    int channels = PluginClient::total_in_buffers;
	if(size > input_allocated)
	{
		Samples **new_input_buffer = new Samples*[channels];
		for(int i = 0; i < channels; i++)
		{
			new_input_buffer[i] = new Samples(size);
			if(input_buffer)
			{
				memcpy(new_input_buffer[i]->get_data(), 
					input_buffer[i]->get_data(), 
					input_size * sizeof(double));
				delete input_buffer[i];
			}
		}
		if(input_buffer) delete [] input_buffer;

		input_allocated = size;
		input_buffer = new_input_buffer;
	}
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
//				band_config->readahead_len = input.tag.get_property("READAHEAD_LEN", band_config->readahead_len);
				band_config->attack_len = input.tag.get_property("ATTACK_LEN", band_config->attack_len);
				band_config->release_len = input.tag.get_property("RELEASE_LEN", band_config->release_len);
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
//	output.tag.set_property("READAHEAD_LEN", band_config->readahead_len);
	output.tag.set_property("ATTACK_LEN", band_config->attack_len);
	output.tag.set_property("RELEASE_LEN", band_config->release_len);
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


LOAD_CONFIGURATION_MACRO(CompressorEffect, CompressorConfig)
NEW_WINDOW_MACRO(CompressorEffect, CompressorWindow)
VFrame* CompressorEffect::new_picon() { return 0; }




int CompressorEffect::process_buffer(int64_t size, 
		Samples **buffer,
		int64_t start_position,
		int sample_rate)
{
    int channels = PluginClient::total_in_buffers;
    BandConfig *band_config = &config.bands[0];

// don't restart after every tweek
	load_configuration();

// restart after seeking
    if(last_position != start_position)
    {
        last_position = start_position;
        input_size = 0;
        target_samples = 0;
        target_current_sample = 0;
    }

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


	int attack_samples = labs(Units::round(band_config->attack_len * sample_rate));
	int release_samples = Units::round(band_config->release_len * sample_rate);
	int trigger = CLIP(config.trigger, 0, PluginAClient::total_in_buffers - 1);

	CLAMP(attack_samples, 1, 1000000);
	CLAMP(release_samples, 1, 1000000);


// 	for(int i = 0; i < channels; i++)
// 	{
// 		read_samples(buffer[i],
// 			i,
// 			sample_rate,
// 			start_position,
// 			size);
// 	}

	double current_slope = (next_target - prev_target) / 
		attack_samples;
	double *trigger_buffer = buffer[trigger]->get_data();
    int preview_samples = MAX(attack_samples, release_samples);
	if(target_current_sample < 0) target_current_sample = target_samples;


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
			for(int i = 0; i < channels; i++)
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
		Samples **new_input_buffer = new Samples*[channels];
		for(int i = 0; i < channels; i++)
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
	if(input_size < size + preview_samples)
	{
		int fragment_size = size + preview_samples - input_size;
		for(int i = 0; i < channels; i++)
		{
			input_buffer[i]->set_offset(input_size);
// 1 read only
			read_samples(input_buffer[i],
				i,
				sample_rate,
				input_start + input_size,
				fragment_size);
			input_buffer[i]->set_offset(0);
		}
		input_size += fragment_size;
	}


// release has to be a constant rate
	for(int i = 0; i < size; i++)
	{
		double current_slope = (next_target - prev_target) /
			target_samples;

#define ADAPTIVE

#ifdef ADAPTIVE
// maximums in the 2 time ranges
        double attack_slope = -0x7fffffff;
        double attack_sample = -1;
        int attack_offset = -1;
        double release_slope = -0x7fffffff;
        double release_sample = -1;
        int release_offset = -1;
        if(target_current_sample >= target_samples)
        {
// start new line segment
            for(int j = 1; j < preview_samples; j++)
            {
                GET_TRIGGER(input_buffer[channel]->get_data(), i + j)
                double new_slope = (sample - current_value) / j;
                if(j < attack_samples && new_slope >= attack_slope)
                {
                    attack_slope = new_slope;
                    attack_sample = sample;
                    attack_offset = j;
                }
                
                if(j < release_samples && 
                    new_slope <= 0 && 
                    new_slope > release_slope)
                {
                    release_slope = new_slope;
                    release_sample = sample;
                    release_offset = j;
                }
            }

			target_current_sample = 0;
            if(attack_slope >= 0)
            {
// attack
				target_samples = attack_offset;
                prev_target = current_value;
                next_target = attack_sample;
                current_slope = attack_slope;
printf("Compressor::process_buffer %d position=%ld slope=%f samples=%d\n", 
__LINE__, start_position + i, current_slope, target_samples);
            }
            else
            {
// release
//                target_samples = release_offset;
                target_samples = release_samples;
                prev_target = current_value;
                next_target = release_sample;
                current_slope = release_slope;
printf("Compressor::process_buffer %d position=%ld slope=%f\n", 
__LINE__, start_position + i, current_slope);
            }
        }
        else
        {
// check for new peak after the line segment
            GET_TRIGGER(input_buffer[channel]->get_data(), i + attack_samples)
            double new_slope = (sample - current_value) /
				attack_samples;
            if(current_slope >= 0)
            {
                if(new_slope > current_slope)
                {
                    target_samples = attack_samples;
                    target_current_sample = 0;
                    prev_target = current_value;
                    next_target = sample;
				    current_slope = new_slope;
printf("Compressor::process_buffer %d position=%ld slope=%f\n", 
__LINE__, start_position + i, current_slope);
                }
            }
            else
            if(current_slope < 0)
            {
                if(sample > next_target)
                {
                    target_samples = attack_samples;
                    target_current_sample = 0;
                    prev_target = current_value;
                    next_target = sample;
				    current_slope = new_slope;
                }
            }
        }


#else // ADAPTIVE


// Get slope from current sample to every sample in readahead_samples.
// Take highest one or first one after target_samples are up.

// For optimization, calculate the first slope we really need.
// Assume every slope up to the end of readahead_samples has been calculated and
// found <= to current slope.
        int first_slope = preview_samples - 1;

// Need new slope immediately
		if(target_current_sample >= target_samples)
		{
            first_slope = 1;
		}

        for(int j = first_slope; 
			j < preview_samples; 
			j++)
		{
            GET_TRIGGER(input_buffer[channel]->get_data(), i + j)

			double new_slope = (sample - current_value) /
				j;
// Got equal or higher slope
			if(new_slope > current_slope && 
				(current_slope >= 0 ||
				new_slope >= 0))
			{
// if(sample > 0) 
// printf("CompressorEffect::process_buffer %d %ld %f\n", 
// __LINE__, 
// start_position + i, 
// sample);

				target_current_sample = 0;
				target_samples = j;
				current_slope = new_slope;
				next_target = sample;
				prev_target = current_value;
			}
// 				else
// 				if(sample > next_target && current_slope < 0)
// 				{
// 					target_current_sample = 0;
// 					target_samples = release_samples;
// 					current_slope = (sample - current_value) /
// 						release_samples;
// 					next_target = sample;
// 					prev_target = current_value;
// 				}
            else
// Hit end of current slope range without finding higher slope
			if(target_current_sample >= target_samples)
			{
				current_slope = sample - current_value;
				target_current_sample = 0;
 				target_samples = release_samples;
				prev_target = current_value;
				next_target = sample;
// printf("CompressorEffect::process_buffer %d position=%ld next_target=%f target_samples=%d\n", 
// __LINE__,
// start_position + i + j,
// next_target,
// target_samples);
			}
		}
#endif // !ADAPTIVE




// Update current value and multiply gain
		target_current_sample++;
        current_value = prev_target +
            (next_target - prev_target) * 
            target_current_sample / 
            target_samples;


		if(config.smoothing_only)
		{
			for(int j = 0; j < channels; j++)
			{
				buffer[j]->get_data()[i] = current_value * 2 - 1;
			}
		}
		else
		{
			double gain = config.calculate_gain(0, current_value);
			for(int j = 0; j < channels; j++)
			{
				buffer[j]->get_data()[i] = input_buffer[j]->get_data()[i] * gain;
			}
		}
	}


    if(get_direction() == PLAY_FORWARD)
    {
        last_position += size;
    }
    else
    {
        last_position -= size;
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
//    delete readahead;
    delete attack;
    delete release;
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
    
    
//	add_subwindow(new BC_Title(x, y, _("Lookahead secs:")));
//	y += title->get_h() + margin;
//	readahead = new CompressorLookAhead(plugin, this, x, y);
//    readahead->create_objects();
//	y += readahead->get_h() + margin;

    
	add_subwindow(new BC_Title(x, y, _("Attack secs:")));
	y += title->get_h() + margin;
	attack = new CompressorAttack(plugin, this, x, y);
    attack->create_objects();
	y += attack->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Release secs:")));
	y += title->get_h() + margin;
	release = new CompressorRelease(plugin, this, x, y);
    release->create_objects();
	y += release->get_h() + margin;


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

//	if(!EQUIV(atof(readahead->get_text()), band_config->readahead_len))
//		readahead->update((float)band_config->readahead_len);
	if(!EQUIV(atof(attack->get_text()), band_config->attack_len))
		attack->update((float)band_config->attack_len);
	if(!EQUIV(atof(release->get_text()), band_config->release_len))
		release->update((float)band_config->release_len);
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






// CompressorLookAhead::CompressorLookAhead(CompressorEffect *plugin, 
//     CompressorWindow *window, 
//     int x, 
//     int y) 
//  : BC_TumbleTextBox(window, 
//     (float)plugin->config.bands[0].readahead_len,
//     (float)MIN_LOOKAHEAD,
//     (float)MAX_LOOKAHEAD,
//     x, 
//     y, 
//     DP(100))
// {
// 	this->plugin = plugin;
//     set_increment(0.1);
//     set_precision(2);
// }
// 
// int CompressorLookAhead::handle_event()
// {
// 	plugin->config.bands[0].readahead_len = atof(get_text());
// 	plugin->send_configure_change();
// 	return 1;
// }
// 
// 




CompressorAttack::CompressorAttack(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y) 
 : BC_TumbleTextBox(window, 
    (float)plugin->config.bands[0].attack_len,
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

int CompressorAttack::handle_event()
{
	plugin->config.bands[0].attack_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}





CompressorRelease::CompressorRelease(CompressorEffect *plugin, 
    CompressorWindow *window, 
    int x, 
    int y) 
 : BC_TumbleTextBox(window,
    (float)plugin->config.bands[0].release_len,
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
int CompressorRelease::handle_event()
{
	plugin->config.bands[0].release_len = atof(get_text());
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




