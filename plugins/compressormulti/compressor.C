
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
#include "compressorgui.h"
#include "bchash.h"
#include "eqcanvas.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "samples.h"
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



BandConfig::BandConfig()
{
    freq = 0;
    solo = 0;
    bypass = 0;
}

BandConfig::~BandConfig()
{
    
}

void BandConfig::copy_from(BandConfig *src)
{
	levels.remove_all();
	for(int i = 0; i < src->levels.total; i++)
		levels.append(src->levels.values[i]);

    freq = src->freq;
    solo = src->solo;
    bypass = src->bypass;
}

int BandConfig::equiv(BandConfig *src)
{
    if(levels.total != levels.total ||
        solo != src->solo ||
        bypass != src->bypass ||
        freq != src->freq)
    {
        return 0;
    }


	for(int i = 0; 
		i < levels.total && i < src->levels.total; 
		i++)
	{
		compressor_point_t *this_level = &levels.values[i];
		compressor_point_t *that_level = &src->levels.values[i];
		if(!EQUIV(this_level->x, that_level->x) ||
			!EQUIV(this_level->y, that_level->y))
		{
        	return 0;
        }
	}
    
    return 1;
}



CompressorConfig::CompressorConfig()
{
    q = 1.0;
	reaction_len = 1.0;
	min_db = -80.0;
	min_x = min_db;
	min_y = min_db;
	max_x = 0;
	max_y = 0;
	trigger = 0;
	input = CompressorConfig::TRIGGER;
	smoothing_only = 0;
	decay_len = 1.0;
    window_size = 4096;
    for(int band = 0; band < TOTAL_BANDS; band++)
    {
        bands[band].freq = Freq::tofreq((band + 1) * TOTALFREQS / TOTAL_BANDS);
    }
}

void CompressorConfig::copy_from(CompressorConfig &that)
{
	reaction_len = that.reaction_len;
	decay_len = that.decay_len;
	min_db = that.min_db;
	min_x = that.min_x;
	min_y = that.min_y;
	max_x = that.max_x;
	max_y = that.max_y;
	trigger = that.trigger;
	input = that.input;
	smoothing_only = that.smoothing_only;
    window_size = that.window_size;
    q = that.q;

    for(int band = 0; band < TOTAL_BANDS; band++)
    {
        BandConfig *dst = &bands[band];
        BandConfig *src = &that.bands[band];
        dst->copy_from(src);
    }
}

int CompressorConfig::equivalent(CompressorConfig &that)
{
    for(int band = 0; band < TOTAL_BANDS; band++)
    {
        if(!bands[band].equiv(&that.bands[band]))
        {
            return 0;
        }
    }

	if(!EQUIV(reaction_len, that.reaction_len) ||
		!EQUIV(decay_len, that.decay_len) ||
		!EQUIV(q, that.q) ||
		trigger != that.trigger ||
		input != that.input ||
		smoothing_only != that.smoothing_only ||
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

void CompressorConfig::dump()
{
	printf("CompressorConfig::dump\n");
// 	for(int i = 0; i < levels.total; i++)
// 	{
// 		printf("	%f %f\n", levels.values[i].x, levels.values[i].y);
// 	}
}


double CompressorConfig::get_y(int band, int number)
{
    BandConfig *ptr = &bands[band];
	if(!ptr->levels.total) 
    {
		return 1.0;
	}
    else
	if(number >= ptr->levels.total)
	{
    	return ptr->levels.values[ptr->levels.total - 1].y;
	}
    else
	{
    	return ptr->levels.values[number].y;
    }
}

double CompressorConfig::get_x(int band, int number)
{
    BandConfig *ptr = &bands[band];
	if(!ptr->levels.total)
    {
		return 0.0;
	}
    else
	if(number >= ptr->levels.total)
	{
    	return ptr->levels.values[ptr->levels.total - 1].x;
	}
    else
	{
    	return ptr->levels.values[number].x;
    }
}

double CompressorConfig::calculate_db(int band, double x)
{
// maximum
	if(x > -0.001) return 0.0;

    BandConfig *ptr = &bands[band];
	for(int i = ptr->levels.total - 1; i >= 0; i--)
	{
		if(ptr->levels.values[i].x <= x)
		{
			if(i < ptr->levels.total - 1)
			{
				return ptr->levels.values[i].y + 
					(x - ptr->levels.values[i].x) *
					(ptr->levels.values[i + 1].y - ptr->levels.values[i].y) / 
					(ptr->levels.values[i + 1].x - ptr->levels.values[i].x);
			}
			else
			{
				return ptr->levels.values[i].y +
					(x - ptr->levels.values[i].x) * 
					(max_y - ptr->levels.values[i].y) / 
					(max_x - ptr->levels.values[i].x);
			}
		}
	}

	if(ptr->levels.total)
	{
		return min_y + 
			(x - min_x) * 
			(ptr->levels.values[0].y - min_y) / 
			(ptr->levels.values[0].x - min_x);
	}
	else
    {
// output = input
		return x;
    }
}


int CompressorConfig::set_point(int band, double x, double y)
{
    BandConfig *ptr = &bands[band];
	for(int i = ptr->levels.total - 1; i >= 0; i--)
	{
		if(ptr->levels.values[i].x < x)
		{
			ptr->levels.append();
			i++;
			for(int j = ptr->levels.total - 2; j >= i; j--)
			{
				ptr->levels.values[j + 1] = ptr->levels.values[j];
			}
			ptr->levels.values[i].x = x;
			ptr->levels.values[i].y = y;

			return i;
		}
	}

	ptr->levels.append();
	for(int j = ptr->levels.total - 2; j >= 0; j--)
	{
		ptr->levels.values[j + 1] = ptr->levels.values[j];
	}
	ptr->levels.values[0].x = x;
	ptr->levels.values[0].y = y;
	return 0;
}

void CompressorConfig::remove_point(int band, int number)
{
    BandConfig *ptr = &bands[band];
	for(int j = number; j < ptr->levels.total - 1; j++)
	{
		ptr->levels.values[j] = ptr->levels.values[j + 1];
	}
	ptr->levels.remove();
}










CompressorEffect::CompressorEffect(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	for(int i = 0; i < TOTAL_BANDS; i++)
    {
        engines[i] = new CompressorEngine(this, i);
    }
}

CompressorEffect::~CompressorEffect()
{
   	delete_dsp();
	for(int i = 0; i < TOTAL_BANDS; i++)
    {
        delete engines[i];
    }
}

void CompressorEffect::delete_dsp()
{
	if(input_buffer)
	{
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
			delete input_buffer[i];
		delete [] input_buffer;
	}


	if(fft)
	{
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
			delete fft[i];
		delete [] fft;
	}

    for(int i = 0; i < TOTAL_BANDS; i++)
    {
        engines[i]->delete_dsp();
    }


    filtered_size = 0;
	input_buffer = 0;
	input_size = 0;
    new_input_size = 0;
    fft = 0;
//	input_allocated = 0;
}


void CompressorEffect::reset()
{
    for(int i = 0; i < TOTAL_BANDS; i++)
    {
        engines[i] = 0;
    }


	input_buffer = 0;
	input_size = 0;
    new_input_size = 0;
	input_start = 0;
    filtered_size = 0;
    last_position = 0;
    fft = 0;
    need_reconfigure = 1;
    current_band = 0;
}

const char* CompressorEffect::plugin_title() { return N_("Compressor Multi"); }
int CompressorEffect::is_realtime() { return 1; }
int CompressorEffect::is_multichannel() { return 1; }



void CompressorEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
    char string[BCTEXTLEN];
    for(int i = 0; i < TOTAL_BANDS; i++)
    {
	    config.bands[i].levels.remove_all();
    }
    
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("COMPRESSOR_MULTI"))
			{
				config.reaction_len = input.tag.get_property("REACTION_LEN", config.reaction_len);
				config.decay_len = input.tag.get_property("DECAY_LEN", config.decay_len);
				config.trigger = input.tag.get_property("TRIGGER", config.trigger);
				config.smoothing_only = input.tag.get_property("SMOOTHING_ONLY", config.smoothing_only);
				config.input = input.tag.get_property("INPUT", config.input);
				config.q = input.tag.get_property("Q", config.q);
				config.window_size = input.tag.get_property("WINDOW_SIZE", config.window_size);

                for(int i = 0; i < TOTAL_BANDS; i++)
                {
                    sprintf(string,"FREQ%d", i);
	                config.bands[i].freq = input.tag.get_property(string, config.bands[i].freq);
                    sprintf(string,"BYPASS%d", i);
	                config.bands[i].bypass = input.tag.get_property(string, config.bands[i].bypass);
                    sprintf(string,"SOLO%d", i);
	                config.bands[i].solo = input.tag.get_property(string, config.bands[i].solo);
                }
			}
			else
            {
                for(int i = 0; i < TOTAL_BANDS; i++)
                {
                    sprintf(string, "LEVEL%d", i);
                    if(input.tag.title_is(string))
			        {
				        double x = input.tag.get_property("X", (double)0);
				        double y = input.tag.get_property("Y", (double)0);
				        compressor_point_t point = { x, y };

				        config.bands[i].levels.append(point);
                        break;
			        }
                }
            }
		}
	}
}

void CompressorEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("COMPRESSOR_MULTI");
	output.tag.set_property("TRIGGER", config.trigger);
	output.tag.set_property("REACTION_LEN", config.reaction_len);
	output.tag.set_property("DECAY_LEN", config.decay_len);
	output.tag.set_property("SMOOTHING_ONLY", config.smoothing_only);
	output.tag.set_property("INPUT", config.input);
	output.tag.set_property("Q", config.q);
	output.tag.set_property("WINDOW_SIZE", config.window_size);
    
    char string[BCTEXTLEN];
    for(int band = 0; band < TOTAL_BANDS; band++)
    {
        sprintf(string, "FREQ%d", band);
	    output.tag.set_property(string, config.bands[band].freq);
        sprintf(string, "BYPASS%d", band);
	    output.tag.set_property(string, config.bands[band].bypass);
        sprintf(string, "SOLO%d", band);
	    output.tag.set_property(string, config.bands[band].solo);
	}

    output.append_tag();
	output.append_newline();


    for(int band = 0; band < TOTAL_BANDS; band++)
    {
	    for(int i = 0; i < config.bands[i].levels.total; i++)
	    {
            sprintf(string, "LEVEL%d", band);
		    output.tag.set_title(string);
		    output.tag.set_property("X", config.bands[band].levels.values[i].x);
		    output.tag.set_property("Y", config.bands[band].levels.values[i].y);

		    output.append_tag();
		    output.append_newline();
	    }
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
    int channels = PluginClient::total_in_buffers;
	need_reconfigure |= load_configuration();


    if(need_reconfigure)
    {
//printf("CompressorEffect::process_buffer %d\n", __LINE__);
        need_reconfigure = 0;

	    min_x = DB::fromdb(config.min_db);
	    min_y = DB::fromdb(config.min_db);
	    max_x = 1.0;
	    max_y = 1.0;

        if(fft && fft[0]->window_size != config.window_size)
        {
		    for(int i = 0; i < channels; i++)
		    {
 			    delete fft[i];
		    }
            delete [] fft;
            fft = 0;
        }

        if(!fft)
        {
            fft = new CompressorFFT*[channels];
		    for(int i = 0; i < channels; i++)
		    {
			    fft[i] = new CompressorFFT(this, i);
                fft[i]->initialize(config.window_size, TOTAL_BANDS);
		    }
        }

        for(int i = 0; i < TOTAL_BANDS; i++)
        {
            engines[i]->reconfigure();
        }

    }
    
    
// reset after seeking
    if(last_position != start_position)
    {
//printf("CompressorEffect::process_buffer %d\n", __LINE__);
        if(fft)
        {
		    for(int i = 0; i < channels; i++)
		    {
 			    if(fft[i]) fft[i]->delete_fft();
		    }
        }
        
        input_size = 0;
        filtered_size = 0;
        input_start = start_position;
    }




	int reaction_samples = (int)(config.reaction_len * sample_rate + 0.5);
	int decay_samples = (int)(config.decay_len * sample_rate + 0.5);
	int trigger = CLIP(config.trigger, 0, channels - 1);

	CLAMP(reaction_samples, -1000000, 1000000);
	CLAMP(decay_samples, reaction_samples, 1000000);
	CLAMP(decay_samples, 1, 1000000);
	if(labs(reaction_samples) < 1) reaction_samples = 1;
	if(labs(decay_samples) < 1) decay_samples = 1;

	int total_buffers = get_total_buffers();

// Only read the current size
	if(reaction_samples >= 0)
	{
        for(int band = 0; band < TOTAL_BANDS; band++)
        {
            engines[band]->allocate_filtered(size);
        }


// FFT all the channels & bands
        for(int channel = 0; channel < total_buffers; channel++)
		{
// reset the input size for each channel
            new_input_size = input_size;
// array of filtered buffers for each band
            Samples *filtered_array[TOTAL_BANDS];
            for(int band = 0; band < TOTAL_BANDS; band++)
            {
                new_spectrogram_frames[band] = 0;
                filtered_array[band] = engines[band]->filtered_buffer[channel];
            }

// this puts bandpassed input in the start of filtered_buffer 
// & raw input on the end of input_buffer
            fft[channel]->process_buffer(start_position, 
		        size, 
		        filtered_array,
		        get_direction());

// shift raw input to the output
            memcpy(buffer[channel]->get_data(), 
                input_buffer[channel]->get_data(),
                size * sizeof(double));
            memcpy(input_buffer[channel]->get_data(),
                input_buffer[channel]->get_data() + size,
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


// compression for each band
        for(int band = 0; band < TOTAL_BANDS; band++)
        {
            engines[band]->process_readbehind(size,
                reaction_samples,
                decay_samples,
                trigger);
		}

	}
	else
// read the current size + extra to look ahead
	{
		int64_t preview_samples = -reaction_samples;

        int new_filtered_size = size + preview_samples;
        for(int band = 0; band < TOTAL_BANDS; band++)
        {
            engines[band]->allocate_filtered(new_filtered_size);
        }

// Append data to the buffers to fill the readahead area.
        int remane = new_filtered_size - filtered_size;
		for(int channel = 0; channel < total_buffers; channel++)
		{
            new_input_size = input_size;

// array of filtered buffers for each band
            Samples *filtered_array[TOTAL_BANDS];
            for(int band = 0; band < TOTAL_BANDS; band++)
            {
                new_spectrogram_frames[band] = 0;
                filtered_array[band] = engines[band]->filtered_buffer[channel];
                filtered_array[band]->set_offset(filtered_size);
            }


            
            int64_t start;
            if(get_direction() == PLAY_FORWARD)
            {
                start = input_start + filtered_size;
            }
            else
            {
                start = input_start - filtered_size;
            }
            
            fft[channel]->process_buffer(start, 
		        remane, 
		        filtered_array,
		        get_direction());

            for(int band = 0; band < TOTAL_BANDS; band++)
            {
                filtered_array[band]->set_offset(0);
            }
		}
        
        input_size = new_input_size;
        filtered_size = new_filtered_size;


// send the spectrograms to the plugin.  This consumes the pointers
	    for(int i = 0; i < spectrogram_frames.size(); i++)
        {
            add_gui_frame(spectrogram_frames.get(i));
        }
        spectrogram_frames.remove_all();

// compression for each band
        for(int band = 0; band < TOTAL_BANDS; band++)
        {
            engines[band]->process_readahead(size,
                preview_samples,
                reaction_samples,
                decay_samples,
                trigger);
        }
	}


// Add together filtered buffers + unfiltered buffer.
// Apply the solo here.
    int have_solo = 0;
    for(int band = 0; band < TOTAL_BANDS; band++)
    {
        if(config.bands[band].solo)
        {
            have_solo = 1;
            break;
        }
    }
    
    for(int channel = 0; channel < channels; channel++)
    {
        double *dst = buffer[channel]->get_data();
        bzero(dst, size * sizeof(double));
        
        for(int band = 0; band < TOTAL_BANDS; band++)
        {
            if(!have_solo || config.bands[band].solo)
            {
                double *src = engines[band]->filtered_buffer[channel]->get_data();
                for(int i = 0; i < size; i++)
                {
                    dst[i] += src[i];
                }
            }
        }
    }



    if(reaction_samples < 0)
    {
        for(int band = 0; band < TOTAL_BANDS; band++)
        {

// shift buffers if readahead
            for(int i = 0; i < channels; i++)
            {
                memcpy(engines[band]->filtered_buffer[i]->get_data(),
                    engines[band]->filtered_buffer[i]->get_data() + size,
                    (filtered_size - size) * sizeof(double));
            }
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


double CompressorEffect::calculate_output(int band, double x)
{
	if(x > 0.999) return 1.0;
    
    ArrayList<compressor_point_t> *levels = &config.bands[band].levels;

	for(int i = levels->total - 1; i >= 0; i--)
	{
		if(levels->values[i].x <= x)
		{
			if(i < levels->total - 1)
			{
				return levels->values[i].y + 
					(x - levels->values[i].x) *
					(levels->values[i + 1].y - levels->values[i].y) / 
					(levels->values[i + 1].x - levels->values[i].x);
			}
			else
			{
				return levels->values[i].y +
					(x - levels->values[i].x) * 
					(max_y - levels->values[i].y) / 
					(max_x - levels->values[i].x);
			}
		}
	}

	if(levels->total)
	{
		return min_y + 
			(x - min_x) * 
			(levels->values[0].y - min_y) / 
			(levels->values[0].x - min_x);
	}
	else
    {
		return x;
    }
}


double CompressorEffect::calculate_gain(int band, double input)
{
//  	double x_db = DB::todb(input);
//  	double y_db = config.calculate_db(x_db);
//  	double y_linear = DB::fromdb(y_db);


	double y_linear = calculate_output(band, input);
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
    for(int i = 0; i < TOTAL_BANDS; i++)
    {
        engines[i]->calculate_envelope();
    }
}




CompressorEngine::CompressorEngine(CompressorEffect *plugin, int band)
{
    this->plugin = plugin;
    this->band = band;
    reset();
}

CompressorEngine::~CompressorEngine()
{
    delete_dsp();
}

void CompressorEngine::delete_dsp()
{
	delete [] envelope;
	levels.remove_all();
	if(filtered_buffer)
	{
		for(int i = 0; i < plugin->total_in_buffers; i++)
		{
        	delete filtered_buffer[i];
		}
        delete [] filtered_buffer;
	}
    reset();
}

void CompressorEngine::reset()
{
    envelope = 0;
    envelope_allocated = 0;
    filtered_buffer = 0;

	next_target = 1.0;
	previous_target = 1.0;
	target_samples = 1;
	target_current_sample = -1;
	current_value = 1.0;
}

void CompressorEngine::reconfigure()
{
// Calculate linear transfer from db 
	levels.remove_all();
    
    BandConfig *config = &plugin->config.bands[band];
	for(int i = 0; i < config->levels.total; i++)
	{
		levels.append();
		levels.values[i].x = DB::fromdb(config->levels.values[i].x);
		levels.values[i].y = DB::fromdb(config->levels.values[i].y);
	}

    calculate_envelope();
}


void CompressorEngine::allocate_filtered(int new_size)
{
    if(!filtered_buffer ||
        new_size > filtered_buffer[0]->get_allocated())
    {
        Samples **new_filtered_buffer = new Samples*[plugin->get_total_buffers()];
        for(int i = 0; i < plugin->get_total_buffers(); i++)
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



void CompressorEngine::calculate_envelope()
{
    

// the window size changed
    if(envelope && envelope_allocated < plugin->config.window_size / 2)
    {
        delete [] envelope;
        envelope = 0;
    }

    if(!envelope)
    {
        envelope_allocated = plugin->config.window_size / 2;
        envelope = new double[envelope_allocated];
    }

    int max_freq = Freq::tofreq_f(TOTALFREQS - 1);
    int nyquist = plugin->project_sample_rate / 2;
    int freq = plugin->config.bands[band].freq;
    int low = 0;
    int high = max_freq;

// max frequency of all previous bands is the min
    for(int i = 0; i < band; i++)
    {
        if(plugin->config.bands[i].freq > low)
        {
            low = plugin->config.bands[i].freq;
        }
    }
    
    if(band < TOTAL_BANDS - 1)
    {
        high = freq;
    }


// limit the frequencies
    if(high >= nyquist)
    {
        high = nyquist;
    }

    if(low > high)
    {
        low = high;
    }

//#define LOG_CROSSOVER
// number of slots in the edge
    double edge = (1.0 - plugin->config.q) * TOTALFREQS / 2;
// number of slots to arrive at 1/2 power

#ifndef LOG_CROSSOVER
// linear
    double edge2 = edge / 2;
#else
// log
    double edge2 = edge * 6 / -INFINITYGAIN;
#endif
    
    double low_slot = Freq::fromfreq_f(low);
    double high_slot = Freq::fromfreq_f(high);
// shift slots to allow crossover
    if(band < TOTAL_BANDS - 1)
    {
        high_slot -= edge2;
    }
    
    if(band > 0)
    {
        low_slot += edge2;
    }

    for(int i = 0; i < plugin->config.window_size / 2; i++)
    {
        double freq = i * nyquist / (plugin->config.window_size / 2);
        double slot = Freq::fromfreq_f(freq);

// sum of previous bands
        double prev_sum = 0;
        for(int prev_band = 0; prev_band < band; prev_band++)
        {
            double *prev_envelope = plugin->engines[prev_band]->envelope;
            prev_sum += prev_envelope[i];
        }

        if(slot < high_slot)
        {
// remaneder of previous bands
            envelope[i] = 1.0 - prev_sum;
        }
        else
// next crossover
        if(slot < high_slot + edge)
        {
            double remane = 1.0 - prev_sum;
#ifndef LOG_CROSSOVER
// linear
            envelope[i] = remane - remane * (slot - high_slot) / edge;
#else
// log TODO
            envelope[i] = DB::fromdb((slot - high_slot) * INFINITYGAIN / edge);
#endif

        }
        else
        {
            envelope[i] = 0.0;
        }

    }
}


void CompressorEngine::process_readbehind(int size, 
    int reaction_samples, 
    int decay_samples,
    int trigger)
{
	if(target_current_sample < 0) target_current_sample = reaction_samples;


	double current_slope = (next_target - previous_target) / 
		reaction_samples;
	double *trigger_buffer = filtered_buffer[trigger]->get_data();
    int channels = plugin->get_total_buffers();
	for(int i = 0; i < size; i++)
	{
// Get slope required to reach current sample from smoothed sample over reaction
// length.
		double sample;
		switch(plugin->config.input)
		{
			case CompressorConfig::MAX:
			{
				double max = 0;
				for(int j = 0; j < channels; j++)
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
				for(int j = 0; j < channels; j++)
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

		if(plugin->config.smoothing_only)
		{
			for(int j = 0; j < channels; j++)
            {
				filtered_buffer[j]->get_data()[i] = current_value;
            }
		}
		else
        if(!plugin->config.bands[band].bypass)
        {
			double gain = plugin->calculate_gain(band, current_value);
			for(int j = 0; j < channels; j++)
			{
				filtered_buffer[j]->get_data()[i] *= gain;
			}
		}
    }

}

void CompressorEngine::process_readahead(int size, 
    int preview_samples,
    int reaction_samples, 
    int decay_samples,
    int trigger)
{
	if(target_current_sample < 0) target_current_sample = target_samples;
	double current_slope = (next_target - previous_target) /
		target_samples;
	double *trigger_buffer = filtered_buffer[trigger]->get_data();
    int channels = plugin->get_total_buffers();
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
            int buffer_offset = i + j;
			switch(plugin->config.input)
			{
				case CompressorConfig::MAX:
				{
					double max = 0;
					for(int k = 0; k < channels; k++)
					{
						sample = fabs(filtered_buffer[k]->get_data()[buffer_offset]);
						if(sample > max) max = sample;
					}
					sample = max;
					break;
				}

				case CompressorConfig::TRIGGER:
					sample = fabs(trigger_buffer[buffer_offset]);
					break;

				case CompressorConfig::SUM:
				{
					double max = 0;
					for(int k = 0; k < channels; k++)
					{
						sample = fabs(filtered_buffer[k]->get_data()[buffer_offset]);
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

		if(plugin->config.smoothing_only)
		{
			for(int j = 0; j < channels; j++)
			{
				filtered_buffer[j]->get_data()[i] = current_value;
			}
		}
		else
        if(!plugin->config.bands[band].bypass)
		{
			double gain = plugin->calculate_gain(band, current_value);
			for(int j = 0; j < channels; j++)
			{
				filtered_buffer[j]->get_data()[i] *= gain;
			}
		}
	}
}




CompressorFFT::CompressorFFT(CompressorEffect *plugin, int channel)
{
    this->plugin = plugin;
    this->channel = channel;
}

CompressorFFT::~CompressorFFT()
{
}

int CompressorFFT::signal_process(int band)
{
// Create new spectrogram for updating the GUI
    frame = 0;
    if((plugin->config.input != CompressorConfig::TRIGGER ||
        channel == plugin->config.trigger))
    {
        if(plugin->new_spectrogram_frames[band] >= plugin->spectrogram_frames.size())
        {
            int total_data = TOTAL_BANDS * window_size / 2;
// store all bands in the same GUI frame
	        frame = new PluginClientFrame(total_data, 
                window_size / 2, 
                plugin->PluginAClient::project_sample_rate);
            plugin->spectrogram_frames.append(frame);
            frame->data = new double[total_data];
            bzero(frame->data, sizeof(double) * total_data);
            frame->nyquist = plugin->PluginAClient::project_sample_rate / 2;
        }
        else
        {
            frame = plugin->spectrogram_frames.get(plugin->new_spectrogram_frames[band]);
        }
    }

//printf("CompressorFFT::signal_process %d channel=%d band=%d frame=%p\n", __LINE__, channel, band, frame);
// apply the bandpass filter
    for(int i = 0; i < window_size / 2; i++)
    {
        double mag = sqrt(freq_real[i] * freq_real[i] + 
            freq_imag[i] * freq_imag[i]);

        double mag2 = plugin->engines[band]->envelope[i] * mag;
        double angle = atan2(freq_imag[i], freq_real[i]);

		freq_real[i] = mag2 * cos(angle);
		freq_imag[i] = mag2 * sin(angle);

// update the spectrogram with the output
// neglect the true average & max spectrograms, but always use the trigger
        if(frame)
        {
            int offset = band * window_size / 2 + i;
            frame->data[offset] = MAX(frame->data[offset], mag2);

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

int CompressorFFT::post_process(int band)
{
    if(frame)
    {
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

        plugin->new_spectrogram_frames[band]++;
    }

    return 0;
}


int CompressorFFT::read_samples(int64_t output_sample, 
	int samples, 
	Samples *buffer)
{
// printf("CompressorFFT::read_samples %d channel=%d output_sample=%ld\n", 
// __LINE__, 
// channel, 
// output_sample);
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








