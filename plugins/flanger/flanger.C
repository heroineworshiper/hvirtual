
/*
 * CINELERRA
 * Copyright (C) 2017-2019 Adam Williams <broadcast at earthling dot net>
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

#include "clip.h"
#include "bchash.h"
#include "bcsignals.h"
#include "filexml.h"
#include "flanger.h"
#include "language.h"
#include "samples.h"
#include "transportque.inc"
#include "units.h"


#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>





PluginClient* new_plugin(PluginServer *server)
{
	return new Flanger(server);
}



Flanger::Flanger(PluginServer *server)
 : PluginAClient(server)
{
	need_reconfigure = 1;
    dsp_in = 0;
    voices = 0;
    last_position = -1;
    flanging_table = 0;
    history_buffer = 0;
    history_size = 0;
    history_size = 0;
}

Flanger::~Flanger()
{
    if(dsp_in)
    {
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
			delete [] dsp_in[i];
        }
        delete [] dsp_in;
    }
    
    if(history_buffer)
    {
        for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
			delete [] history_buffer[i];
        }
        delete [] history_buffer;
    }
    
    delete [] voices;
    delete [] flanging_table;
}

const char* Flanger::plugin_title() { return N_("Flanger"); }
int Flanger::is_realtime() { return 1; }
int Flanger::is_multichannel() { return 1; }
int Flanger::is_synthesis() { return 0; }
VFrame* Flanger::new_picon() { return 0; }


int Flanger::process_buffer(int64_t size, 
	Samples **buffer, 
	int64_t start_position,
	int sample_rate)
{
    need_reconfigure |= load_configuration();

PRINT_TRACE

    if(!voices)
    {
        voices = new Voice[PluginClient::total_in_buffers];
    }
PRINT_TRACE

    if(!dsp_in)
    {
        dsp_in = new double*[PluginClient::total_in_buffers];
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
 			dsp_in[i] = 0;
        }
    }

// reset after seeking
    if(last_position != start_position)
    {
        need_reconfigure = 1;
    }

PRINT_TRACE

    if(need_reconfigure)
    {
        need_reconfigure = 0;

        if(flanging_table)
        {
            delete [] flanging_table;
        }

// flanging waveform is a whole number of samples that repeats
        table_size = (int)((double)sample_rate / config.rate);
        if(table_size % 2)
        {
            table_size++;
        }

        flanging_table = new flange_sample_t[table_size];
        double depth_samples = config.depth * 
            sample_rate / 1000;
// read behind so the flange can work in realtime
        double ratio = (double)depth_samples /
            (table_size / 2);
        double offset = config.offset * sample_rate / 1000;
// printf("Flanger::process_buffer %d %f %f\n", 
// __LINE__, 
// depth_samples,
// sample_rate / 2 - depth_samples);
        for(int i = 0; i <= table_size / 2; i++)
        {
            double input_sample = -i * ratio;
// printf("Flanger::process_buffer %d i=%d input_sample=%f ratio=%f\n", 
// __LINE__, 
// i, 
// input_sample,
// ratio);
            flanging_table[i].input_sample = input_sample;
            flanging_table[i].input_period = ratio;
        }
        
        for(int i = table_size / 2 + 1; i < table_size; i++)
        {
            double input_sample = -ratio * (table_size - i);
            flanging_table[i].input_sample = input_sample;
            flanging_table[i].input_period = ratio;
// printf("Flanger::process_buffer %d i=%d input_sample=%f ratio=%f\n", 
// __LINE__, 
// i, 
// input_sample,
// ratio);
        }


        if(!history_buffer)
        {
            history_buffer = new double*[PluginClient::total_in_buffers];
            for(int i = 0; i < PluginClient::total_in_buffers; i++)
            {
                history_buffer[i] = 0;
            }
        }
        history_size = 0;
        Voice *voice = &voices[0];

// keyframe position
		int64_t prev_position = edl_to_local(
			get_prev_keyframe(
				get_source_position())->position);

		if(prev_position == 0)
		{
			prev_position = get_source_start();
		}

        voice->table_offset = (int64_t)(start_position - 
            prev_position + 
            config.starting_phase * table_size / 100) % table_size;
    }
PRINT_TRACE

    reallocate_dsp(size);
PRINT_TRACE
    reallocate_history((int)((config.offset + config.depth) * 
        sample_rate / 
        1000 + 1));
PRINT_TRACE

// read the input
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
	{
		read_samples(buffer[i],
			i,
			sample_rate,
			start_position,
			size);
	}


PRINT_TRACE

// paint the voices
    double wetness = DB::fromdb(config.wetness);
    if(config.wetness <= INFINITYGAIN)
    {
        wetness = 0;
    }
PRINT_TRACE

    Voice *voice = &voices[0];
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
	{
PRINT_TRACE
        double *output = dsp_in[i];
PRINT_TRACE
        double *input = buffer[i]->get_data();
PRINT_TRACE

// input signal
//printf("Flanger::process_buffer %d wetness=%f\n", __LINE__, wetness);
        for(int j = 0; j < size; j++)
        {
            output[j] = input[j] * wetness;
        }

printf("Flanger::process_buffer %d start_position=%ld table_offset=%d table_size=%d size=%ld\n",
__LINE__,
start_position, 
voice->table_offset,
table_size,
size);

// delayed signal
        int table_offset = voice->table_offset;
        for(int j = 0; j < size; j++)
        {
            flange_sample_t *table = &flanging_table[table_offset];
            double input_sample = j + table->input_sample;
            double input_period = table->input_period;

// values to interpolate
            double sample1;
            double sample2;
            int input_sample1 = (int)(input_sample);
            int input_sample2 = (int)(input_sample + 1);
            double fraction1 = (double)((int)(input_sample + 1)) - input_sample;
            double fraction2 = 1.0 - fraction1;
            if(input_sample1 < 0)
            {
                sample1 = history_buffer[i][history_size + input_sample1];
            }
            else
            {
                sample1 = input[input_sample1];
            }

            if(input_sample2 < 0)
            {
                sample2 = history_buffer[i][history_size + input_sample2];
            }
            else
            {
                sample2 = input[input_sample2];
            }
            output[j] += sample1 * fraction1 + sample2 * fraction2;
//            output[j] += sample1;
            
            table_offset++;
            table_offset %= table_size;
        }
        voice->table_offset = table_offset;
    }

PRINT_TRACE

    for(int i = 0; i < PluginClient::total_in_buffers; i++)
	{
// history is bigger than input buffer.  Copy entire input buffer.
        if(history_size > size)
        {
            memcpy(history_buffer[i], 
                history_buffer[i] + size,
                (history_size - size) * sizeof(double));
            memcpy(history_buffer[i] + (history_size - size),
                buffer[i]->get_data(),
                size * sizeof(double));
        }
        else
        {
// input is bigger than history buffer.  Copy only history size
           memcpy(history_buffer[i],
                buffer[i]->get_data() + size - history_size,
                history_size * sizeof(double));
        }
    }

PRINT_TRACE

// copy the DSP buffer to the output
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
    {
        memcpy(buffer[i]->get_data(), dsp_in[i], size * sizeof(double));
    }

PRINT_TRACE

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


void Flanger::reallocate_dsp(int new_dsp_allocated)
{
    if(new_dsp_allocated > dsp_in_allocated)
    {
// copy samples already read into the new buffers
        for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
            double *old_dsp = dsp_in[i];
            double *new_dsp = new double[new_dsp_allocated];
            if(old_dsp)
            {
                delete [] old_dsp;
            }
            dsp_in[i] = new_dsp;
        }
        dsp_in_allocated = new_dsp_allocated;
//printf("Reverb::reallocate_dsp %d dsp_in_allocated=%d\n", __LINE__, dsp_in_allocated);
    }
}

void Flanger::reallocate_history(int new_size)
{
    if(new_size != history_size)
    {
// copy samples already read into the new buffers
        for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
            double *old_history = 0;
            
            if(history_buffer)
            {
                old_history = history_buffer[i];
            }
            double *new_history = new double[new_size];
            bzero(new_history, sizeof(double) * new_size);
            if(old_history)
            {
                int copy_size = MIN(new_size, history_size);
                memcpy(new_history, 
                    old_history + history_size - copy_size, 
                    sizeof(double) * copy_size);
                delete [] old_history;
            }
            history_buffer[i] = new_history;
        }
        history_size = new_size;
    }
}


NEW_WINDOW_MACRO(Flanger, FlangerWindow)
LOAD_CONFIGURATION_MACRO(Flanger, FlangerConfig)


void Flanger::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("FLANGER");
	output.tag.set_property("VOICES", config.voices);
	output.tag.set_property("OFFSET", config.offset);
	output.tag.set_property("STARTING_PHASE", config.starting_phase);
	output.tag.set_property("DEPTH", config.depth);
	output.tag.set_property("RATE", config.rate);
	output.tag.set_property("WETNESS", config.wetness);
	output.append_tag();
	output.append_newline();
	
	
	
	output.terminate_string();
}

void Flanger::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("FLANGER"))
		{
			config.voices = input.tag.get_property("VOICES", config.voices);
			config.offset = input.tag.get_property("OFFSET", config.offset);
			config.starting_phase = input.tag.get_property("STARTING_PHASE", config.starting_phase);
			config.depth = input.tag.get_property("DEPTH", config.depth);
			config.rate = input.tag.get_property("RATE", config.rate);
			config.wetness = input.tag.get_property("WETNESS", config.wetness);
		}
	}

	config.boundaries();
}

void Flanger::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("Flanger::update_gui 1");
            ((FlangerWindow*)thread->window)->update();
			thread->window->unlock_window();
		}
	}
}




Voice::Voice()
{
}






FlangerConfig::FlangerConfig()
{
	voices = 1;
	offset = 0.005;
	starting_phase = 0;
	depth = 0.001;
	rate = 1.0;
	wetness = 0;
}

int FlangerConfig::equivalent(FlangerConfig &that)
{
	return (voices == that.voices) &&
		EQUIV(offset, that.offset) &&
		EQUIV(starting_phase, that.starting_phase) &&
		EQUIV(depth, that.depth) &&
		EQUIV(rate, that.rate) &&
		EQUIV(wetness, that.wetness);
}

void FlangerConfig::copy_from(FlangerConfig &that)
{
	voices = that.voices;
	offset = that.offset;
	starting_phase = that.starting_phase;
	depth = that.depth;
	rate = that.rate;
	wetness = that.wetness;
}

void FlangerConfig::interpolate(FlangerConfig &prev, 
	FlangerConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	copy_from(prev);
}

void FlangerConfig::boundaries()
{
	CLAMP(voices, MIN_VOICES, MAX_VOICES);
	CLAMP(offset, MIN_OFFSET, MAX_OFFSET);
	CLAMP(starting_phase, MIN_STARTING_PHASE, MAX_STARTING_PHASE);
	CLAMP(depth, MIN_DEPTH, MAX_DEPTH);
	CLAMP(rate, MIN_RATE, MAX_RATE);
	CLAMP(wetness, INFINITYGAIN, 0.0);
}


