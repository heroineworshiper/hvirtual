
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
#include "eqcanvas.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "samples.h"
#include "theme.h"
#include "transportque.inc"
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
	delete [] envelope;
    
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

	if(filtered_buffer)
	{
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
			delete filtered_buffer[i];
		delete [] filtered_buffer;
	}


	if(fft)
	{
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
			delete fft[i];
		delete [] fft;
	}



    filtered_buffer = 0;
    filtered_size = 0;
	input_buffer = 0;
	input_size = 0;
    new_input_size = 0;
    fft = 0;
//	input_allocated = 0;
}


void CompressorEffect::reset()
{
    filtered_buffer = 0;
    filtered_size = 0;
	input_buffer = 0;
	input_size = 0;
    new_input_size = 0;
	input_start = 0;

    last_position = 0;


	next_target = 1.0;
	previous_target = 1.0;
	target_samples = 1;
	target_current_sample = -1;
	current_value = 1.0;

    envelope = 0;
    envelope_allocated = 0;
    fft = 0;
    need_reconfigure = 1;
}

const char* CompressorEffect::plugin_title() { return N_("Compressor"); }
int CompressorEffect::is_realtime() { return 1; }
int CompressorEffect::is_multichannel() { return 1; }



void CompressorEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	config.levels.remove_all();
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("COMPRESSOR"))
			{
				config.reaction_len = input.tag.get_property("REACTION_LEN", config.reaction_len);
				config.decay_len = input.tag.get_property("DECAY_LEN", config.decay_len);
				config.trigger = input.tag.get_property("TRIGGER", config.trigger);
				config.smoothing_only = input.tag.get_property("SMOOTHING_ONLY", config.smoothing_only);
				config.pass_trigger = input.tag.get_property("PASS_TRIGGER", config.pass_trigger);
				config.input = input.tag.get_property("INPUT", config.input);
	            config.low = input.tag.get_property("LOW", config.low);
	            config.high = input.tag.get_property("HIGH", config.high);
	            config.q = input.tag.get_property("Q", config.q);
			}
			else
			if(input.tag.title_is("LEVEL"))
			{
				double x = input.tag.get_property("X", (double)0);
				double y = input.tag.get_property("Y", (double)0);
				compressor_point_t point = { x, y };

				config.levels.append(point);
			}
		}
	}
}

void CompressorEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("COMPRESSOR");
	output.tag.set_property("TRIGGER", config.trigger);
	output.tag.set_property("REACTION_LEN", config.reaction_len);
	output.tag.set_property("DECAY_LEN", config.decay_len);
	output.tag.set_property("SMOOTHING_ONLY", config.smoothing_only);
	output.tag.set_property("PASS_TRIGGER", config.pass_trigger);
	output.tag.set_property("INPUT", config.input);
	output.tag.set_property("LOW", config.low);
	output.tag.set_property("HIGH", config.high);
	output.tag.set_property("Q", config.q);
	output.append_tag();
	output.append_newline();


	for(int i = 0; i < config.levels.total; i++)
	{
		output.tag.set_title("LEVEL");
		output.tag.set_property("X", config.levels.values[i].x);
		output.tag.set_property("Y", config.levels.values[i].y);

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
			thread->window->lock_window("CompressorEffect::update_gui 1");
			((CompressorWindow*)thread->window)->update();
			thread->window->unlock_window();
		}
        else
        {
            int total_frames = get_gui_update_frames();
			if(total_frames)
			{
				thread->window->lock_window("CompressorEffect::update_gui 2");
				((CompressorWindow*)thread->window)->update_eqcanvas();
				thread->window->unlock_window();
			}
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
	need_reconfigure |= load_configuration();


    if(need_reconfigure)
    {
//printf("CompressorEffect::process_buffer %d\n", __LINE__);
        need_reconfigure = 0;
    
        if(fft && fft[0]->window_size != config.window_size)
        {
		    for(int i = 0; i < PluginClient::total_in_buffers; i++)
		    {
 			    delete fft[i];
		    }
            delete [] fft;
            fft = 0;
        }

        if(!fft)
        {
            fft = new CompressorFFT*[PluginClient::total_in_buffers];
		    for(int i = 0; i < PluginClient::total_in_buffers; i++)
		    {
			    fft[i] = new CompressorFFT(this, i);
                fft[i]->initialize(config.window_size);
		    }
        }

// Calculate linear transfer from db 
	    levels.remove_all();
	    for(int i = 0; i < config.levels.total; i++)
	    {
		    levels.append();
		    levels.values[i].x = DB::fromdb(config.levels.values[i].x);
		    levels.values[i].y = DB::fromdb(config.levels.values[i].y);
	    }


        calculate_envelope();
    }
    
    
// reset after seeking
    if(last_position != start_position)
    {
//printf("CompressorEffect::process_buffer %d\n", __LINE__);
        if(fft)
        {
		    for(int i = 0; i < PluginClient::total_in_buffers; i++)
		    {
 			    if(fft[i]) fft[i]->delete_fft();
		    }
        }
        
        input_size = 0;
        filtered_size = 0;
        input_start = start_position;
    }



	min_x = DB::fromdb(config.min_db);
	min_y = DB::fromdb(config.min_db);
	max_x = 1.0;
	max_y = 1.0;


	int reaction_samples = (int)(config.reaction_len * sample_rate + 0.5);
	int decay_samples = (int)(config.decay_len * sample_rate + 0.5);
	int trigger = CLIP(config.trigger, 0, PluginAClient::total_in_buffers - 1);

	CLAMP(reaction_samples, -1000000, 1000000);
	CLAMP(decay_samples, reaction_samples, 1000000);
	CLAMP(decay_samples, 1, 1000000);
	if(labs(reaction_samples) < 1) reaction_samples = 1;
	if(labs(decay_samples) < 1) decay_samples = 1;

	int total_buffers = get_total_buffers();

// Only read the current size
	if(reaction_samples >= 0)
	{
		if(target_current_sample < 0) target_current_sample = reaction_samples;
		
        
        allocate_filtered(size);
        for(int i = 0; i < total_buffers; i++)
		{
            new_spectrogram_frames = 0;
// reset the input size for each channel
            new_input_size = input_size;
// this puts bandpassed input in the start of filtered_buffer 
// & raw input on the end of input_buffer
            fft[i]->process_buffer(start_position, 
		        size, 
		        filtered_buffer[i],
		        get_direction());

// shift raw input to the output
            memcpy(buffer[i]->get_data(), 
                input_buffer[i]->get_data(),
                size * sizeof(double));
            memcpy(input_buffer[i]->get_data(),
                input_buffer[i]->get_data() + size,
                (new_input_size - size) * sizeof(double));
            new_input_size -= size;
		}
        input_size = new_input_size;

// send the spectrograms to the plugin.  This consumes the pointers
	    for(int i = 0; i < spectrogram_frames.size(); i++)
        {
            add_gui_frame(spectrogram_frames.get(i));
        }
        spectrogram_frames.remove_all();

		double current_slope = (next_target - previous_target) / 
			reaction_samples;
		double *trigger_buffer = filtered_buffer[trigger]->get_data();
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
						sample = fabs(filtered_buffer[j]->get_data()[i]);
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
						sample = fabs(filtered_buffer[j]->get_data()[i]);
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
                {
					buffer[j]->get_data()[i] = current_value;
                }
			}
			else
			{
				double gain = calculate_gain(current_value);
				for(int j = 0; j < total_buffers; j++)
				{
					buffer[j]->get_data()[i] *= gain;
				}
			}
		}
        
// pass filtered trigger through
        if(config.pass_trigger)
        {
            for(int i = 0; i < total_buffers; i++)
            {
                memcpy(buffer[i]->get_data(), filtered_buffer[i]->get_data(), size * sizeof(double));
            }
        }

	}
	else
// read the current size + extra to look ahead
	{
		if(target_current_sample < 0) target_current_sample = target_samples;
		int64_t preview_samples = -reaction_samples;

        int new_filtered_size = size + preview_samples;
        allocate_filtered(new_filtered_size);

// Append data to the buffers to fill the readahead area.
        int remane = new_filtered_size - filtered_size;
		for(int i = 0; i < total_buffers; i++)
		{
            new_spectrogram_frames = 0;
            new_input_size = input_size;
            filtered_buffer[i]->set_offset(filtered_size);
            
            int64_t start;
            if(get_direction() == PLAY_FORWARD)
            {
                start = input_start + filtered_size;
            }
            else
            {
                start = input_start - filtered_size;
            }
            
            fft[i]->process_buffer(start, 
		        remane, 
		        filtered_buffer[i],
		        get_direction());
            filtered_buffer[i]->set_offset(0);
		}
        input_size = new_input_size;
        filtered_size = new_filtered_size;


// send the spectrograms to the plugin.  This consumes the pointers
	    for(int i = 0; i < spectrogram_frames.size(); i++)
        {
            add_gui_frame(spectrogram_frames.get(i));
        }
        spectrogram_frames.remove_all();

		double current_slope = (next_target - previous_target) /
			target_samples;
		double *trigger_buffer = filtered_buffer[trigger]->get_data();
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
			{
            	first_slope = 1;
			}
            
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
							sample = fabs(filtered_buffer[k]->get_data()[i + j]);
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
							sample = fabs(filtered_buffer[k]->get_data()[i + j]);
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
				double gain = calculate_gain(current_value);
				for(int j = 0; j < total_buffers; j++)
				{
					buffer[j]->get_data()[i] = input_buffer[j]->get_data()[i] * gain;
				}
			}
		}


// shift buffers
        for(int i = 0; i < total_buffers; i++)
        {
// pass filtered trigger through
            if(config.pass_trigger)
            {
                memcpy(buffer[i]->get_data(), filtered_buffer[i]->get_data(), size * sizeof(double));
            }

            memcpy(input_buffer[i]->get_data(),
                input_buffer[i]->get_data() + size,
                (input_size - size) * sizeof(double));
            memcpy(filtered_buffer[i]->get_data(),
                filtered_buffer[i]->get_data() + size,
                (filtered_size - size) * sizeof(double));
        }
        input_size -= size;
        filtered_size -= size;

        if(get_direction() == PLAY_FORWARD)
        {
            input_start += size;
        }
        else
        {
            input_start -= size;
        }
	}



    if(get_direction() == PLAY_FORWARD)
    {
        last_position = start_position + size;
    }
    else
    {
        last_position = start_position - size;
    }

    

	return 0;
}

void CompressorEffect::allocate_input(int new_size)
{
	if(!input_buffer ||
        new_size > input_buffer[0]->get_allocated())
	{
		Samples **new_input_buffer = new Samples*[get_total_buffers()];

		for(int i = 0; i < get_total_buffers(); i++)
		{
			new_input_buffer[i] = new Samples(new_size);
            
			if(input_buffer)
			{
				memcpy(new_input_buffer[i]->get_data(), 
					input_buffer[i]->get_data(), 
					input_buffer[i]->get_allocated() * sizeof(double));
				delete input_buffer[i];
			}
            
		}
        
		if(input_buffer) delete [] input_buffer;
		
		input_buffer = new_input_buffer;
   	}
}

void CompressorEffect::allocate_filtered(int new_size)
{
    if(!filtered_buffer ||
        new_size > filtered_buffer[0]->get_allocated())
    {
        Samples **new_filtered_buffer = new Samples*[get_total_buffers()];
        for(int i = 0; i < get_total_buffers(); i++)
		{
            new_filtered_buffer[i] = new Samples(new_size);
            
            if(filtered_buffer)
            {
				memcpy(new_filtered_buffer[i]->get_data(), 
					filtered_buffer[i]->get_data(), 
					filtered_buffer[i]->get_allocated() * sizeof(double));
				delete filtered_buffer[i];
            }
        }
        
        if(filtered_buffer) delete [] filtered_buffer;
        filtered_buffer = new_filtered_buffer;
    }
}




double CompressorEffect::calculate_output(double x)
{
	if(x > 0.999) return 1.0;

	for(int i = levels.total - 1; i >= 0; i--)
	{
		if(levels.values[i].x <= x)
		{
			if(i < levels.total - 1)
			{
				return levels.values[i].y + 
					(x - levels.values[i].x) *
					(levels.values[i + 1].y - levels.values[i].y) / 
					(levels.values[i + 1].x - levels.values[i].x);
			}
			else
			{
				return levels.values[i].y +
					(x - levels.values[i].x) * 
					(max_y - levels.values[i].y) / 
					(max_x - levels.values[i].x);
			}
		}
	}

	if(levels.total)
	{
		return min_y + 
			(x - min_x) * 
			(levels.values[0].y - min_y) / 
			(levels.values[0].x - min_x);
	}
	else
		return x;
}


double CompressorEffect::calculate_gain(double input)
{
//  	double x_db = DB::todb(input);
//  	double y_db = config.calculate_db(x_db);
//  	double y_linear = DB::fromdb(y_db);


	double y_linear = calculate_output(input);
	double gain;
	if(fabs(input - 0.0) > 0.000001)
	{
    	gain = y_linear / input;
	}
    else
    {
		gain = 100000;
    }

// if(isinf(gain) || isnan(gain))
// {
// printf("CompressorEffect::process_buffer %d y_linear=%f input=%f gain=%f\n", 
// __LINE__, 
// y_linear,
// input,
// gain);
// }


	return gain;
}


void CompressorEffect::calculate_envelope()
{
// assume the window size changed
    if(envelope && envelope_allocated < config.window_size / 2)
    {
        delete [] envelope;
        envelope = 0;
    }

    if(!envelope)
    {
        envelope_allocated = config.window_size / 2;
        envelope = new double[envelope_allocated];
    }

    int max_freq = Freq::tofreq_f(TOTALFREQS - 1);
    int nyquist = PluginAClient::project_sample_rate / 2;
    int low = config.low;
    int high = config.high;


// limit the frequencies
    if(high >= max_freq)
    {
        high = nyquist;
    }

    if(low > high)
    {
        low = high;
    }
    
    double edge = (1.0 - config.q) * TOTALFREQS / 2;
    double low_slot = Freq::fromfreq_f(low);
    double high_slot = Freq::fromfreq_f(high);
    for(int i = 0; i < config.window_size / 2; i++)
    {
        double freq = i * nyquist / (config.window_size / 2);
        double slot = Freq::fromfreq_f(freq);

        if(slot < low_slot - edge)
        {
            envelope[i] = 0.0;
        }
        else
        if(slot < low_slot)
        {
            envelope[i] = DB::fromdb((low_slot - slot) * INFINITYGAIN / edge);
        }
        else
        if(slot < high_slot)
        {
            envelope[i] = 1.0;
        }
        else
        if(slot < high_slot + edge)
        {
            envelope[i] = DB::fromdb((slot - high_slot) * INFINITYGAIN / edge);
        }
        else
        {
            envelope[i] = 0.0;
        }

    }
//printf("CompressorEffect::calculate_envelope %d\n", __LINE__);
}




CompressorFFT::CompressorFFT(CompressorEffect *plugin, int channel)
{
    this->plugin = plugin;
    this->channel = channel;
}

CompressorFFT::~CompressorFFT()
{
}

int CompressorFFT::signal_process()
{
// Create new spectrogram for updating the GUI
    PluginClientFrame *frame = 0;
    if(plugin->config.input != CompressorConfig::TRIGGER ||
        channel == plugin->config.trigger)
    {
        if(plugin->new_spectrogram_frames >= plugin->spectrogram_frames.size())
        {
	        frame = new PluginClientFrame(window_size / 2, 
                window_size / 2, 
                plugin->PluginAClient::project_sample_rate);
            plugin->spectrogram_frames.append(frame);
            frame->data = new double[window_size / 2];
            bzero(frame->data, sizeof(double) * window_size / 2);
            frame->nyquist = plugin->PluginAClient::project_sample_rate / 2;
        }
        else
        {
            frame = plugin->spectrogram_frames.get(plugin->new_spectrogram_frames);
        }
    }

// apply the bandpass filter
    for(int i = 0; i < window_size / 2; i++)
    {
        double mag = sqrt(freq_real[i] * freq_real[i] + 
            freq_imag[i] * freq_imag[i]);

        double mag2 = plugin->envelope[i] * mag;
        double angle = atan2(freq_imag[i], freq_real[i]);

		freq_real[i] = mag2 * cos(angle);
		freq_imag[i] = mag2 * sin(angle);

// update the spectrogram with the output
// neglect the true average & max spectrograms, but always use the trigger
        if(frame)
        {
            frame->data[i] = MAX(frame->data[i], mag2);

// get the maximum output in the frequency domane
            if(mag2 > frame->freq_max)
            {
                frame->freq_max = mag2;
            }
        }
    }

    symmetry(window_size, freq_real, freq_imag);
    return 0;
}

int CompressorFFT::post_process()
{
    if(plugin->config.input != CompressorConfig::TRIGGER ||
        channel == plugin->config.trigger)
    {
        PluginClientFrame *frame = plugin->spectrogram_frames.get(plugin->new_spectrogram_frames);
// get the maximum output in the time domane
	    double time_max = 0;
	    for(int i = 0; i < window_size; i++)
	    {
		    if(fabs(output_real[i]) > time_max) time_max = fabs(output_real[i]);
	    }

        if(time_max > frame->time_max)
        {
    	    frame->time_max = time_max;
        }

// printf("CompressorFFT::post_process %d frame=%p data=%p freq_max=%f time_max=%f\n",
// __LINE__,
// frame,
// frame->data,
// frame->freq_max,
// frame->time_max);

        plugin->new_spectrogram_frames++;
    }

    return 0;
}


int CompressorFFT::read_samples(int64_t output_sample, 
	int samples, 
	Samples *buffer)
{
printf("CompressorFFT::read_samples %d channel=%d output_sample=%ld\n", 
__LINE__, 
channel, 
output_sample);
	int result = plugin->read_samples(buffer,
		channel,
		plugin->get_samplerate(),
		output_sample,
		samples);

// printf("CompressorFFT::read_samples %d output_sample=%ld dsp_in_length=%d samples=%d\n",
// __LINE__,
// output_sample,
// plugin->new_dsp_length,
// samples);

// append unprocessed samples to the input_buffer
    int new_input_size = plugin->new_input_size + samples;
    plugin->allocate_input(new_input_size);
    memcpy(plugin->input_buffer[channel]->get_data() + plugin->new_input_size,
        buffer->get_data(),
        samples * sizeof(double));


    plugin->new_input_size = new_input_size;
    

    return result;
}





CompressorConfig::CompressorConfig()
{
	reaction_len = 1.0;
	min_db = -80.0;
	min_x = min_db;
	min_y = min_db;
	max_x = 0;
	max_y = 0;
	trigger = 0;
	input = CompressorConfig::TRIGGER;
	smoothing_only = 0;
    pass_trigger = 0;
	decay_len = 1.0;
    high = Freq::tofreq(TOTALFREQS);
    low = Freq::tofreq(0);
    q = 1.0;
    window_size = 4096;
}

void CompressorConfig::copy_from(CompressorConfig &that)
{
	this->reaction_len = that.reaction_len;
	this->decay_len = that.decay_len;
	this->min_db = that.min_db;
	this->min_x = that.min_x;
	this->min_y = that.min_y;
	this->max_x = that.max_x;
	this->max_y = that.max_y;
	this->trigger = that.trigger;
	this->input = that.input;
	this->smoothing_only = that.smoothing_only;
	this->pass_trigger = that.pass_trigger;
	levels.remove_all();
	for(int i = 0; i < that.levels.total; i++)
		this->levels.append(that.levels.values[i]);
    
    high = that.high;
    low = that.low;
    q = that.q;
    window_size = that.window_size;
}

int CompressorConfig::equivalent(CompressorConfig &that)
{
	if(!EQUIV(this->reaction_len, that.reaction_len) ||
		!EQUIV(this->decay_len, that.decay_len) ||
		this->trigger != that.trigger ||
		this->input != that.input ||
		this->smoothing_only != that.smoothing_only ||
		this->pass_trigger != that.pass_trigger ||
        this->levels.total != that.levels.total)
	{
    	return 0;
	}

	for(int i = 0; 
		i < this->levels.total && i < that.levels.total; 
		i++)
	{
		compressor_point_t *this_level = &this->levels.values[i];
		compressor_point_t *that_level = &that.levels.values[i];
		if(!EQUIV(this_level->x, that_level->x) ||
			!EQUIV(this_level->y, that_level->y))
			return 0;
	}
    
    if(high != that.high ||
        low != that.low ||
        !EQUIV(q, that.q) ||
        window_size != that.window_size)
    {
        return 0;
    }
    
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

int CompressorConfig::total_points()
{
	if(!levels.total) 
		return 1;
	else
		return levels.total;
}

void CompressorConfig::dump()
{
	printf("CompressorConfig::dump\n");
	for(int i = 0; i < levels.total; i++)
	{
		printf("	%f %f\n", levels.values[i].x, levels.values[i].y);
	}
}


double CompressorConfig::get_y(int number)
{
	if(!levels.total) 
		return 1.0;
	else
	if(number >= levels.total)
		return levels.values[levels.total - 1].y;
	else
		return levels.values[number].y;
}

double CompressorConfig::get_x(int number)
{
	if(!levels.total)
		return 0.0;
	else
	if(number >= levels.total)
		return levels.values[levels.total - 1].x;
	else
		return levels.values[number].x;
}

double CompressorConfig::calculate_db(double x)
{
	if(x > -0.001) return 0.0;

	for(int i = levels.total - 1; i >= 0; i--)
	{
		if(levels.values[i].x <= x)
		{
			if(i < levels.total - 1)
			{
				return levels.values[i].y + 
					(x - levels.values[i].x) *
					(levels.values[i + 1].y - levels.values[i].y) / 
					(levels.values[i + 1].x - levels.values[i].x);
			}
			else
			{
				return levels.values[i].y +
					(x - levels.values[i].x) * 
					(max_y - levels.values[i].y) / 
					(max_x - levels.values[i].x);
			}
		}
	}

	if(levels.total)
	{
		return min_y + 
			(x - min_x) * 
			(levels.values[0].y - min_y) / 
			(levels.values[0].x - min_x);
	}
	else
		return x;
}


int CompressorConfig::set_point(double x, double y)
{
	for(int i = levels.total - 1; i >= 0; i--)
	{
		if(levels.values[i].x < x)
		{
			levels.append();
			i++;
			for(int j = levels.total - 2; j >= i; j--)
			{
				levels.values[j + 1] = levels.values[j];
			}
			levels.values[i].x = x;
			levels.values[i].y = y;

			return i;
		}
	}

	levels.append();
	for(int j = levels.total - 2; j >= 0; j--)
	{
		levels.values[j + 1] = levels.values[j];
	}
	levels.values[0].x = x;
	levels.values[0].y = y;
	return 0;
}

void CompressorConfig::remove_point(int number)
{
	for(int j = number; j < levels.total - 1; j++)
	{
		levels.values[j] = levels.values[j + 1];
	}
	levels.remove();
}

void CompressorConfig::optimize()
{
	int done = 0;
	
	while(!done)
	{
		done = 1;
		
		
		for(int i = 0; i < levels.total - 1; i++)
		{
			if(levels.values[i].x >= levels.values[i + 1].x)
			{
				done = 0;
				for(int j = i + 1; j < levels.total - 1; j++)
				{
					levels.values[j] = levels.values[j + 1];
				}
				levels.remove();
			}
		}
		
	}
}



























CompressorWindow::CompressorWindow(CompressorEffect *plugin)
 : PluginClientWindow(plugin,
	DP(650), 
	DP(560), 
	DP(650), 
	DP(560),
	0)
{
	this->plugin = plugin;
}

CompressorWindow::~CompressorWindow()
{
    delete eqcanvas;
}


void CompressorWindow::create_objects()
{
    int margin = client->get_theme()->widget_border;
	int x = DP(35), y = margin;
	int control_margin = DP(150);
    BC_Title *title;

    add_subwindow(title = new BC_Title(x, y, _("Sound level:")));
    y += title->get_h() + 1;
	add_subwindow(canvas = new CompressorCanvas(plugin, 
		x, 
		y, 
		get_w() - x - control_margin - DP(10), 
		get_h() * 2 / 3 - y));
	canvas->set_cursor(CROSS_CURSOR, 0, 0);
    y += canvas->get_h() + DP(30);
    
    add_subwindow(title = new BC_Title(x, y, _("Trigger bandwidth:")));
    y += title->get_h();
    eqcanvas = new EQCanvas(this,
        margin,
        y,
        canvas->get_w() + x - margin,
        get_h() - y - margin,
        INFINITYGAIN,
        0.0);
    eqcanvas->freq_divisions = 10;
    eqcanvas->initialize();
    
    
	x = get_w() - control_margin;
    y = margin;
	add_subwindow(new BC_Title(x, y, _("Reaction secs:")));
	y += DP(20);
	add_subwindow(reaction = new CompressorReaction(plugin, x, y));
	y += DP(30);
	add_subwindow(new BC_Title(x, y, _("Decay secs:")));
	y += DP(20);
	add_subwindow(decay = new CompressorDecay(plugin, x, y));
	y += DP(30);
	add_subwindow(new BC_Title(x, y, _("Trigger Type:")));
	y += DP(20);
	add_subwindow(input = new CompressorInput(plugin, x, y));
	input->create_objects();
	y += DP(30);
	add_subwindow(new BC_Title(x, y, _("Trigger:")));
	y += DP(20);
	add_subwindow(trigger = new CompressorTrigger(plugin, x, y));
	if(plugin->config.input != CompressorConfig::TRIGGER) trigger->disable();
	y += DP(30);


	add_subwindow(smooth = new CompressorSmooth(plugin, x, y));
    y += smooth->get_h() + margin;
	add_subwindow(pass_trigger = new CompressorPassTrigger(plugin, x, y));
    y += pass_trigger->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Output:")));
    y += title->get_h();
	add_subwindow(y_text = new CompressorY(plugin, x, y));
    y += y_text->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Input:")));
    y += title->get_h();
	add_subwindow(x_text = new CompressorX(plugin, x, y));
    y += x_text->get_h() + margin;

    add_subwindow(title = new BC_Title(x, y, _("Low Freq:")));
    add_subwindow(low = new CompressorQPot(this, 
        plugin, 
        get_w() - margin - BC_Pot::calculate_w(), 
        y, 
        &plugin->config.low));
    y += low->get_h() + margin;
    
    add_subwindow(title = new BC_Title(x, y, _("High Freq:")));
    add_subwindow(high = new CompressorQPot(this, 
        plugin, 
        get_w() - margin - BC_Pot::calculate_w(), 
        y, 
        &plugin->config.high));
        
    y += high->get_h() + margin;
    add_subwindow(title = new BC_Title(x, y, _("Steepness:")));
    add_subwindow(q = new CompressorFPot(this, 
        plugin, 
        get_w() - margin - BC_Pot::calculate_w(), 
        y, 
        &plugin->config.q,
        0,
        1));
    y += q->get_h() + margin;
    
    add_subwindow(title = new BC_Title(x, y, _("Window size:")));
    y += title->get_h();
    add_subwindow(size = new CompressorSize(this,
        plugin,
        x,
        y));
    size->create_objects();
    size->update(plugin->config.window_size);
    y += size->get_h() + margin;
    
	add_subwindow(clear = new CompressorClear(plugin, x, y));

    

    plugin->calculate_envelope();

	draw_scales();
	update_canvas();
    update_eqcanvas();
	show_window();
}

void CompressorWindow::draw_scales()
{
	draw_3d_border(canvas->get_x() - 2, 
		canvas->get_y() - 2, 
		canvas->get_w() + 4, 
		canvas->get_h() + 4, 
		get_bg_color(),
		BLACK,
		MDGREY, 
		get_bg_color());


	set_font(SMALLFONT);
	set_color(get_resources()->default_text_color);

#define DIVISIONS 8
// output divisions
	for(int i = 0; i <= DIVISIONS; i++)
	{
		int y = canvas->get_y() + DP(10) + canvas->get_h() / DIVISIONS * i;
		int x = canvas->get_x() - DP(30);
		char string[BCTEXTLEN];
		
		sprintf(string, "%.0f", (float)i / DIVISIONS * plugin->config.min_db);
		draw_text(x, y, string);
		
		int y1 = canvas->get_y() + canvas->get_h() / DIVISIONS * i;
		int y2 = canvas->get_y() + canvas->get_h() / DIVISIONS * (i + 1);
		for(int j = 0; j < 10; j++)
		{
			y = y1 + (y2 - y1) * j / 10;
			if(j == 0)
			{
				draw_line(canvas->get_x() - DP(10), y, canvas->get_x(), y);
			}
			else
			if(i < DIVISIONS)
			{
				draw_line(canvas->get_x() - DP(5), y, canvas->get_x(), y);
			}
		}
	}

// input divisions
	for(int i = 0; i <= DIVISIONS; i++)
	{
		int y = canvas->get_y() + canvas->get_h();
		int x = canvas->get_x() + (canvas->get_w() - 10) / DIVISIONS * i;
        int y1 = y + get_text_ascent(SMALLFONT);
		char string[BCTEXTLEN];

		sprintf(string, 
            "%.0f", 
            (1.0 - (float)i / DIVISIONS) * plugin->config.min_db);
		draw_text(x, y1 + DP(10), string);

		int x1 = canvas->get_x() + canvas->get_w() / DIVISIONS * i;
		int x2 = canvas->get_x() + canvas->get_w() / DIVISIONS * (i + 1);
		for(int j = 0; j < 10; j++)
		{
			x = x1 + (x2 - x1) * j / 10;
			if(j == 0)
			{
				draw_line(x, 
                    y, 
                    x, 
                    y + DP(10));
			}
			else
			if(i < DIVISIONS)
			{
				draw_line(x, 
                    y, 
                    x, 
                    y + DP(5));
			}
		}
	}



	flash();
}

void CompressorWindow::update()
{
	update_textboxes();
    low->update(plugin->config.low);
    high->update(plugin->config.high);
    q->update(plugin->config.q);
    size->update(plugin->config.window_size);
	update_canvas();
    update_eqcanvas();
}

void CompressorWindow::update_textboxes()
{
	if(atol(trigger->get_text()) != plugin->config.trigger)
		trigger->update((int64_t)plugin->config.trigger);
	if(strcmp(input->get_text(), CompressorInput::value_to_text(plugin->config.input)))
		input->set_text(CompressorInput::value_to_text(plugin->config.input));

	if(plugin->config.input != CompressorConfig::TRIGGER && trigger->get_enabled())
		trigger->disable();
	else
	if(plugin->config.input == CompressorConfig::TRIGGER && !trigger->get_enabled())
		trigger->enable();

	if(!EQUIV(atof(reaction->get_text()), plugin->config.reaction_len))
		reaction->update((float)plugin->config.reaction_len);
	if(!EQUIV(atof(decay->get_text()), plugin->config.decay_len))
		decay->update((float)plugin->config.decay_len);
	smooth->update(plugin->config.smoothing_only);
	pass_trigger->update(plugin->config.pass_trigger);
	if(canvas->current_operation == CompressorCanvas::DRAG)
	{
		x_text->update((float)plugin->config.levels.values[canvas->current_point].x);
		y_text->update((float)plugin->config.levels.values[canvas->current_point].y);
	}
    
    
    
}

#define POINT_W DP(10)
void CompressorWindow::update_canvas()
{
	int y1, y2;


	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	canvas->set_line_dashes(1);
	canvas->set_color(GREEN);
	
	for(int i = 1; i < DIVISIONS; i++)
	{
		int y = canvas->get_h() * i / DIVISIONS;
		canvas->draw_line(0, y, canvas->get_w(), y);
		
		int x = canvas->get_w() * i / DIVISIONS;
		canvas->draw_line(x, 0, x, canvas->get_h());
	}
	canvas->set_line_dashes(0);


	canvas->set_font(MEDIUMFONT);
	canvas->draw_text(plugin->get_theme()->widget_border, 
		canvas->get_h() / 2, 
		_("Output"));
	canvas->draw_text(canvas->get_w() / 2 - canvas->get_text_width(MEDIUMFONT, _("Input")) / 2, 
		canvas->get_h() - plugin->get_theme()->widget_border, 
		_("Input"));


	canvas->set_color(WHITE);
	canvas->set_line_width(2);
	for(int i = 0; i < canvas->get_w(); i++)
	{
		double x_db = ((double)1 - (double)i / canvas->get_w()) * plugin->config.min_db;
		double y_db = plugin->config.calculate_db(x_db);
		y2 = (int)(y_db / plugin->config.min_db * canvas->get_h());

		if(i > 0)
		{
			canvas->draw_line(i - 1, y1, i, y2);
		}

		y1 = y2;
	}
	canvas->set_line_width(1);

	int total = plugin->config.levels.total ? plugin->config.levels.total : 1;
	for(int i = 0; i < plugin->config.levels.total; i++)
	{
		double x_db = plugin->config.get_x(i);
		double y_db = plugin->config.get_y(i);

		int x = (int)(((double)1 - x_db / plugin->config.min_db) * canvas->get_w());
		int y = (int)(y_db / plugin->config.min_db * canvas->get_h());
		
		canvas->draw_box(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
	}
	
	canvas->flash();
}


void CompressorWindow::update_eqcanvas()
{
    plugin->calculate_envelope();
    eqcanvas->update_spectrogram(plugin);
    
    eqcanvas->draw_envelope(plugin->envelope,
        plugin->PluginAClient::project_sample_rate,
        plugin->config.window_size);
    
}

int CompressorWindow::resize_event(int w, int h)
{
	return 1;
}





CompressorFPot::CompressorFPot(CompressorWindow *gui, 
    CompressorEffect *plugin, 
    int x, 
    int y, 
    double *output,
    double min,
    double max)
 : BC_FPot(x,
    y,
    *output,
    min,
    max)
{
    this->gui = gui;
    this->plugin = plugin;
    this->output = output;
    set_precision(0.01);
}



int CompressorFPot::handle_event()
{
    *output = get_value();
    plugin->send_configure_change();
    gui->update_eqcanvas();
    return 1;
}






CompressorQPot::CompressorQPot(CompressorWindow *gui, 
    CompressorEffect *plugin, 
    int x, 
    int y, 
    int *output)
 : BC_QPot(x,
    y,
    *output)
{
    this->gui = gui;
    this->plugin = plugin;
    this->output = output;
}


int CompressorQPot::handle_event()
{
    *output = get_value();
    plugin->send_configure_change();
    gui->update_eqcanvas();
    return 1;
}







CompressorSize::CompressorSize(CompressorWindow *gui, 
    CompressorEffect *plugin, 
    int x, 
    int y)
 : BC_PopupMenu(x, 
    y, 
    DP(100), 
    "4096", 
    1)
{
    this->gui = gui;
    this->plugin = plugin;
}

int CompressorSize::handle_event()
{
    plugin->config.window_size = atoi(get_text());
    plugin->send_configure_change();
    gui->update_eqcanvas();
    return 1;
}


void CompressorSize::create_objects()
{
	add_item(new BC_MenuItem("2048"));
	add_item(new BC_MenuItem("4096"));
	add_item(new BC_MenuItem("8192"));
	add_item(new BC_MenuItem("16384"));
	add_item(new BC_MenuItem("32768"));
	add_item(new BC_MenuItem("65536"));
	add_item(new BC_MenuItem("131072"));
	add_item(new BC_MenuItem("262144"));
}


void CompressorSize::update(int size)
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", size);
	set_text(string);
}










CompressorCanvas::CompressorCanvas(CompressorEffect *plugin, int x, int y, int w, int h) 
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
	current_operation = NONE;
}

int CompressorCanvas::button_press_event()
{
// Check existing points
	if(is_event_win() && cursor_inside())
	{
		for(int i = 0; i < plugin->config.levels.total; i++)
		{
			double x_db = plugin->config.get_x(i);
			double y_db = plugin->config.get_y(i);

			int x = (int)(((double)1 - x_db / plugin->config.min_db) * get_w());
			int y = (int)(y_db / plugin->config.min_db * get_h());

			if(get_cursor_x() <= x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() <= y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			{
				current_operation = DRAG;
				current_point = i;
				return 1;
			}
		}



// Create new point
		double x_db = (double)(1 - (double)get_cursor_x() / get_w()) * plugin->config.min_db;
		double y_db = (double)get_cursor_y() / get_h() * plugin->config.min_db;

		current_point = plugin->config.set_point(x_db, y_db);
		current_operation = DRAG;
		((CompressorWindow*)plugin->thread->window)->update();
		plugin->send_configure_change();
		return 1;
	}
	return 0;
//plugin->config.dump();
}

int CompressorCanvas::button_release_event()
{
	if(current_operation == DRAG)
	{
		if(current_point > 0)
		{
			if(plugin->config.levels.values[current_point].x <
				plugin->config.levels.values[current_point - 1].x)
				plugin->config.remove_point(current_point);
		}

		if(current_point < plugin->config.levels.total - 1)
		{
			if(plugin->config.levels.values[current_point].x >=
				plugin->config.levels.values[current_point + 1].x)
				plugin->config.remove_point(current_point);
		}

		((CompressorWindow*)plugin->thread->window)->update();
		plugin->send_configure_change();
		current_operation = NONE;
		return 1;
	}

	return 0;
}

int CompressorCanvas::cursor_motion_event()
{
	if(current_operation == DRAG)
	{
		int x = get_cursor_x();
		int y = get_cursor_y();
		CLAMP(x, 0, get_w());
		CLAMP(y, 0, get_h());
		double x_db = (double)(1 - (double)x / get_w()) * plugin->config.min_db;
		double y_db = (double)y / get_h() * plugin->config.min_db;
		plugin->config.levels.values[current_point].x = x_db;
		plugin->config.levels.values[current_point].y = y_db;
		((CompressorWindow*)plugin->thread->window)->update();
		plugin->send_configure_change();
		return 1;
//plugin->config.dump();
	}
	else
// Change cursor over points
	if(is_event_win() && cursor_inside())
	{
		int new_cursor = CROSS_CURSOR;

		for(int i = 0; i < plugin->config.levels.total; i++)
		{
			double x_db = plugin->config.get_x(i);
			double y_db = plugin->config.get_y(i);

			int x = (int)(((double)1 - x_db / plugin->config.min_db) * get_w());
			int y = (int)(y_db / plugin->config.min_db * get_h());

			if(get_cursor_x() <= x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() <= y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			{
				new_cursor = UPRIGHT_ARROW_CURSOR;
				break;
			}
		}


		if(new_cursor != get_cursor())
		{
			set_cursor(new_cursor, 0, 1);
		}
	}
	return 0;
}





CompressorReaction::CompressorReaction(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, (float)plugin->config.reaction_len)
{
	this->plugin = plugin;
}

int CompressorReaction::handle_event()
{
	plugin->config.reaction_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}

int CompressorReaction::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			plugin->config.reaction_len += 0.1;
		}
		else
		if(get_buttonpress() == 5)
		{
			plugin->config.reaction_len -= 0.1;
		}
		update((float)plugin->config.reaction_len);
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}

CompressorDecay::CompressorDecay(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, (float)plugin->config.decay_len)
{
	this->plugin = plugin;
}
int CompressorDecay::handle_event()
{
	plugin->config.decay_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}

int CompressorDecay::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			plugin->config.decay_len += 0.1;
		}
		else
		if(get_buttonpress() == 5)
		{
			plugin->config.decay_len -= 0.1;
		}
		update((float)plugin->config.decay_len);
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}



CompressorX::CompressorX(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, "")
{
	this->plugin = plugin;
}
int CompressorX::handle_event()
{
	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < plugin->config.levels.total)
	{
		plugin->config.levels.values[current_point].x = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->update_canvas();
		plugin->send_configure_change();
	}
	return 1;
}



CompressorY::CompressorY(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, "")
{
	this->plugin = plugin;
}
int CompressorY::handle_event()
{
	int current_point = ((CompressorWindow*)plugin->thread->window)->canvas->current_point;
	if(current_point < plugin->config.levels.total)
	{
		plugin->config.levels.values[current_point].y = atof(get_text());
		((CompressorWindow*)plugin->thread->window)->update_canvas();
		plugin->send_configure_change();
	}
	return 1;
}





CompressorTrigger::CompressorTrigger(CompressorEffect *plugin, int x, int y) 
 : BC_TextBox(x, y, DP(100), 1, (int64_t)plugin->config.trigger)
{
	this->plugin = plugin;
}
int CompressorTrigger::handle_event()
{
	plugin->config.trigger = atol(get_text());
	plugin->send_configure_change();
	return 1;
}

int CompressorTrigger::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4) return BC_TextBox::button_press_event();
		if(get_buttonpress() == 4)
		{
			plugin->config.trigger++;
		}
		else
		if(get_buttonpress() == 5)
		{
			plugin->config.trigger--;
		}
		update((int64_t)plugin->config.trigger);
		plugin->send_configure_change();
		return 1;
	}
	return 0;
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
	plugin->config.levels.remove_all();
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


CompressorPassTrigger::CompressorPassTrigger(CompressorEffect *plugin, int x, int y) 
 : BC_CheckBox(x, y, plugin->config.pass_trigger, _("Pass trigger"))
{
	this->plugin = plugin;
}

int CompressorPassTrigger::handle_event()
{
	plugin->config.pass_trigger = get_value();
	plugin->send_configure_change();
	return 1;
}




