
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

#include "chorus.h"
#include "clip.h"
#include "confirmsave.h"
#include "bchash.h"
#include "bcsignals.h"
#include "errorbox.h"
#include "filexml.h"
#include "language.h"
#include "samples.h"
#include "theme.h"
#include "transportque.inc"
#include "units.h"

#include "vframe.h"

#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


// min rate for the GUI
#define MIN_RATE 0.0
// min rate to avoid division by zero
#define MIN_RATE2 0.10
#define MAX_RATE 10.0
#define MIN_OFFSET 0.0
#define MAX_OFFSET 100.0
#define MIN_DEPTH 0.0
#define MAX_DEPTH 100.0
#define MIN_VOICES 1
#define MAX_VOICES 256



PluginClient* new_plugin(PluginServer *server)
{
	return new Chorus(server);
}



Chorus::Chorus(PluginServer *server)
 : PluginAClient(server)
{
	srand(time(0));
	need_reconfigure = 1;
    dsp_in = 0;
    dsp_in_allocated = 0;
    voices = 0;
    last_position = -1;
    flanging_table = 0;
    table_size = 0;
    history_buffer = 0;
    history_size = 0;
}

Chorus::~Chorus()
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

const char* Chorus::plugin_title() { return N_("Chorus"); }
int Chorus::is_realtime() { return 1; }
int Chorus::is_multichannel() { return 1; }
int Chorus::is_synthesis() { return 1; }
VFrame* Chorus::new_picon() { return 0; }


int Chorus::process_buffer(int64_t size, 
	Samples **buffer, 
	int64_t start_position,
	int sample_rate)
{
    need_reconfigure |= load_configuration();
// printf("Chorus::process_buffer %d start_position=%ld size=%ld\n",
// __LINE__,
// start_position, 
// size);


    if(!dsp_in)
    {
        dsp_in = new double*[PluginClient::total_in_buffers];
		for(int i = 0; i < PluginClient::total_in_buffers; i++)
		{
 			dsp_in[i] = 0;
        }
    }

// reset after seeking & configuring
    if(last_position != start_position || need_reconfigure)
    {
        need_reconfigure = 0;

        if(flanging_table)
        {
            delete [] flanging_table;
        }

// flanging waveform is a whole number of samples that repeats
        if(config.rate < MIN_RATE2)
        {
            table_size = 256;
        }
        else
        {
            table_size = (int)((double)sample_rate / config.rate);
            if(table_size % 2)
            {
                table_size++;
            }
        }

        flanging_table = new flange_sample_t[table_size];
        double depth_samples = config.depth * 
            sample_rate / 1000;
// read behind so the flange can work in realtime
        double ratio = 0;

        ratio = (double)depth_samples /
            (table_size / 2);

// printf("Chorus::process_buffer %d %f %f\n", 
// __LINE__, 
// depth_samples,
// sample_rate / 2 - depth_samples);
        for(int i = 0; i <= table_size / 2; i++)
        {
            double input_sample = -i * ratio;
// printf("Chorus::process_buffer %d i=%d input_sample=%f ratio=%f\n", 
// __LINE__, 
// i, 
// input_sample,
// ratio);
            flanging_table[i].input_sample = input_sample;
//            flanging_table[i].input_period = ratio;
        }
        
        for(int i = table_size / 2 + 1; i < table_size; i++)
        {
            double input_sample = -ratio * (table_size - i);
            flanging_table[i].input_sample = input_sample;
//            flanging_table[i].input_period = ratio;
// printf("Chorus::process_buffer %d i=%d input_sample=%f ratio=%f\n", 
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

// compute the phase position from the keyframe position & the phase offset
		int64_t prev_position = edl_to_local(
			get_prev_keyframe(
				get_source_position())->position);

		if(prev_position == 0)
		{
			prev_position = get_source_start();
		}

        if(voices)
        {
            delete [] voices;
            voices = 0;
        }
        
        if(!voices)
        {
            voices = new Voice[total_voices()];
        }
        
        for(int i = 0; i < total_voices(); i++)
        {
            Voice *voice = &voices[i];
            voice->src_channel = i / config.voices;
            voice->dst_channel = i % PluginClient::total_in_buffers;

// randomize the starting phase
            voice->table_offset = (int64_t)(start_position - 
                prev_position + 
                (rand() % table_size)) % table_size;
// printf("Chorus::process_buffer %d i=%d src=%d dst=%d\n",
// __LINE__,
// i,
// voice->src_channel,
// voice->dst_channel);
        }
    }

    int starting_offset = (int)(config.offset * sample_rate / 1000);
    int depth_offset = (int)(config.depth * sample_rate / 1000);
    reallocate_dsp(size);
    reallocate_history(starting_offset + depth_offset + 1);

// read the input
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
	{
		read_samples(buffer[i],
			i,
			sample_rate,
			start_position,
			size);
	}



// paint the voices
    double wetness = DB::fromdb(config.wetness);
    if(config.wetness <= INFINITYGAIN)
    {
        wetness = 0;
    }

// input signal
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
    {
        double *output = dsp_in[i];
        double *input = buffer[i]->get_data();
        for(int j = 0; j < size; j++)
        {
            output[j] = input[j] * wetness;
        }
    }

// delayed signals
    for(int i = 0; i < total_voices(); i++)
    {
        Voice *voice = &voices[i];
        double *output = dsp_in[voice->dst_channel];
        double *input = buffer[voice->src_channel]->get_data();
        double *history = history_buffer[voice->src_channel];
// 
// printf("Chorus::process_buffer %d table_offset=%d table=%f\n", 
// __LINE__, 
// voice->table_offset, 
// flanging_table[table_size / 2].input_sample);

        int table_offset = voice->table_offset;
        for(int j = 0; j < size; j++)
        {
            flange_sample_t *table = &flanging_table[table_offset];
            double input_sample = j - starting_offset + table->input_sample;
//            double input_period = table->input_period;

// values to interpolate
            double sample1;
            double sample2;
            int input_sample1 = (int)(input_sample);
            int input_sample2 = (int)(input_sample + 1);
            double fraction1 = (double)((int)(input_sample + 1)) - input_sample;
            double fraction2 = 1.0 - fraction1;
            if(input_sample1 < 0)
            {
                sample1 = history[history_size + input_sample1];
            }
            else
            {
                sample1 = input[input_sample1];
            }

            if(input_sample2 < 0)
            {
                sample2 = history[history_size + input_sample2];
            }
            else
            {
                sample2 = input[input_sample2];
            }
            output[j] += sample1 * fraction1 + sample2 * fraction2;
            
            if(config.rate >= MIN_RATE2)
            {
                table_offset++;
                table_offset %= table_size;
            }
        }
        voice->table_offset = table_offset;
    }


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


// copy the DSP buffer to the output
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
    {
        memcpy(buffer[i]->get_data(), dsp_in[i], size * sizeof(double));
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


int Chorus::total_voices()
{
    return PluginClient::total_in_buffers * config.voices;
}


void Chorus::reallocate_dsp(int new_dsp_allocated)
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
    }
}

void Chorus::reallocate_history(int new_size)
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





NEW_WINDOW_MACRO(Chorus, ChorusWindow)


LOAD_CONFIGURATION_MACRO(Chorus, ChorusConfig)


void Chorus::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause xml file to store data directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("CHORUS");
	output.tag.set_property("VOICES", config.voices);
	output.tag.set_property("OFFSET", config.offset);
	output.tag.set_property("DEPTH", config.depth);
	output.tag.set_property("RATE", config.rate);
	output.tag.set_property("WETNESS", config.wetness);
	output.append_tag();
	output.append_newline();

	output.terminate_string();
}

void Chorus::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause xml file to read directly from text
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));
	int result = 0;

	result = input.read_tag();

	if(!result)
	{
		if(input.tag.title_is("CHORUS"))
		{
			config.voices = input.tag.get_property("VOICES", config.voices);
			config.offset = input.tag.get_property("OFFSET", config.offset);
			config.depth = input.tag.get_property("DEPTH", config.depth);
			config.rate = input.tag.get_property("RATE", config.rate);
			config.wetness = input.tag.get_property("WETNESS", config.wetness);
		}
	}

	config.boundaries();
}

void Chorus::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("Chorus::update_gui 1");
            ((ChorusWindow*)thread->window)->update();
			thread->window->unlock_window();
        }
	}
}








Voice::Voice()
{
}



ChorusConfig::ChorusConfig()
{
	voices = 1;
	offset = 0.00;
	depth = 10.0;
	rate = 0.20;
	wetness = 0;
}

int ChorusConfig::equivalent(ChorusConfig &that)
{
	return (voices == that.voices) &&
		EQUIV(offset, that.offset) &&
		EQUIV(depth, that.depth) &&
		EQUIV(rate, that.rate) &&
		EQUIV(wetness, that.wetness);
}

void ChorusConfig::copy_from(ChorusConfig &that)
{
	voices = that.voices;
	offset = that.offset;
	depth = that.depth;
	rate = that.rate;
	wetness = that.wetness;
}

void ChorusConfig::interpolate(ChorusConfig &prev, 
	ChorusConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	copy_from(prev);
}

void ChorusConfig::boundaries()
{
	CLAMP(voices, MIN_VOICES, MAX_VOICES);
	CLAMP(offset, MIN_OFFSET, MAX_OFFSET);
	CLAMP(depth, MIN_DEPTH, MAX_DEPTH);
	CLAMP(rate, MIN_RATE, MAX_RATE);
	CLAMP(wetness, INFINITYGAIN, 0.0);
}











#define WINDOW_W DP(400)
#define WINDOW_H DP(200)

ChorusWindow::ChorusWindow(Chorus *plugin)
 : PluginClientWindow(plugin, 
	WINDOW_W, 
	WINDOW_H, 
	WINDOW_W, 
	WINDOW_H, 
	0)
{ 
	this->plugin = plugin; 
}

ChorusWindow::~ChorusWindow()
{
    delete voices;
    delete offset;
    delete depth;
    delete rate;
    delete wetness;
}

void ChorusWindow::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
    int x1 = margin;
	int x2 = DP(230), y = margin;
    int x3 = x2 + BC_Pot::calculate_w() + margin;
    int x4 = x3 + BC_Pot::calculate_w() + margin;
    int text_w = get_w() - margin - x4;
    int height = BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1) + margin;


    voices = new PluginParam(plugin,
        this,
        x1, 
        x2,
        x4,
        y, 
        text_w,
        &plugin->config.voices,  // output_i
        0, // output_f
        0, // output_q
        "Voices per channel:",
        MIN_VOICES, // min
        MAX_VOICES); // max
    voices->initialize();
    y += height;

    offset = new PluginParam(plugin,
        this,
        x1, 
        x3,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.offset, // output_f
        0, // output_q
        "Phase offset (ms):",
        MIN_OFFSET, // min
        MAX_OFFSET); // max
    offset->set_precision(2);
    offset->initialize();
    y += height;


    depth = new PluginParam(plugin,
        this,
        x1, 
        x2,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.depth, // output_f
        0, // output_q
        "Depth (ms):",
        MIN_DEPTH, // min
        MAX_DEPTH); // max
    depth->set_precision(2);
    depth->initialize();
    y += height;



    rate = new PluginParam(plugin,
        this,
        x1, 
        x3,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.rate, // output_f
        0, // output_q
        "Rate (Hz):",
        MIN_RATE, // min
        MAX_RATE); // max
    rate->initialize();
    y += height;



    wetness = new PluginParam(plugin,
        this,
        x1, 
        x2,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.wetness, // output_f
        0, // output_q
        "Wetness (db):",
        INFINITYGAIN, // min
        0); // max
    wetness->initialize();
    y += height;

	show_window();
}

void ChorusWindow::update()
{
    voices->update(1, 1);
    offset->update(1, 1);
    depth->update(1, 1);
    rate->update(1, 1);
    wetness->update(1, 1);
}

void ChorusWindow::param_updated()
{
}













