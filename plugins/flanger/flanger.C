
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
#include "bcdisplayinfo.h"
#include "bchash.h"
#include "bcsignals.h"
#include "filexml.h"
#include "flanger.h"
#include "guicast.h"
#include "language.h"
#include "samples.h"
#include "theme.h"
#include "transportque.inc"
#include "units.h"


#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#define MIN_RATE 0.1
#define MAX_RATE 10.0
#define MIN_OFFSET 0.0
#define MAX_OFFSET 100.0
#define MIN_DEPTH 0.0
#define MAX_DEPTH 100.0
#define MIN_STARTING_PHASE 0
#define MAX_STARTING_PHASE 100


PluginClient* new_plugin(PluginServer *server)
{
	return new Flanger(server);
}



Flanger::Flanger(PluginServer *server)
 : PluginAClient(server)
{
	need_reconfigure = 1;
    dsp_in = 0;
    dsp_in_allocated = 0;
    last_position = -1;
    flanging_table = 0;
    table_size = 0;
    history_buffer = 0;
    history_size = 0;
}

Flanger::~Flanger()
{
    if(dsp_in)
    {
        delete [] dsp_in;
    }

    if(history_buffer)
    {
        delete [] history_buffer;
    }

    delete [] flanging_table;
}

const char* Flanger::plugin_title() { return N_("Flanger"); }
int Flanger::is_realtime() { return 1; }
int Flanger::is_multichannel() { return 0; }
int Flanger::is_synthesis() { return 0; }
VFrame* Flanger::new_picon() { return 0; }


int Flanger::process_buffer(int64_t size, 
	Samples *buffer, 
	int64_t start_position,
	int sample_rate)
{
    need_reconfigure |= load_configuration();
// printf("Flanger::process_buffer %d start_position=%ld size=%ld\n",
// __LINE__,
// start_position, 
// size);



// reset after seeking & configuring
    if(last_position != start_position || need_reconfigure)
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



// compute the phase position from the keyframe position & the phase offset
		int64_t prev_position = edl_to_local(
			get_prev_keyframe(
				get_source_position())->position);

		if(prev_position == 0)
		{
			prev_position = get_source_start();
		}

        voice.table_offset = (int64_t)(start_position - 
            prev_position + 
            config.starting_phase * table_size / 100) % table_size;
// printf("Flanger::process_buffer %d start_position=%ld table_offset=%d table_size=%d\n", 
// __LINE__,
// start_position,
// voice.table_offset,
// table_size);
    }

    int starting_offset = (int)(config.offset * sample_rate / 1000);
    int depth_offset = (int)(config.depth * sample_rate / 1000);
    reallocate_dsp(size);
//    reallocate_history(starting_offset + depth_offset + 1);
// always use the maximum history, in case of keyframes
    reallocate_history((MAX_OFFSET + MAX_DEPTH) * sample_rate / 1000 + 1);

// read the input
	read_samples(buffer,
		0,
		sample_rate,
		start_position,
		size);



// paint the voices
    double wetness = DB::fromdb(config.wetness);
    if(config.wetness <= INFINITYGAIN)
    {
        wetness = 0;
    }

    double *output = dsp_in;
    double *input = buffer->get_data();

// input signal
    for(int j = 0; j < size; j++)
    {
        output[j] = input[j] * wetness;
    }


// delayed signal
    int table_offset = voice.table_offset;
    for(int j = 0; j < size; j++)
    {
        flange_sample_t *table = &flanging_table[table_offset];
        double input_sample = j - starting_offset + table->input_sample;
        double input_period = table->input_period;

// if(j == 0)
// printf("Flanger::process_buffer %d input_sample=%f\n", 
// __LINE__,
// input_sample);

// values to interpolate
        double sample1;
        double sample2;
        int input_sample1 = (int)(input_sample);
        int input_sample2 = (int)(input_sample + 1);
        double fraction1 = (double)((int)(input_sample + 1)) - input_sample;
        double fraction2 = 1.0 - fraction1;
        if(input_sample1 < 0)
        {
            sample1 = history_buffer[history_size + input_sample1];
        }
        else
        {
            sample1 = input[input_sample1];
        }

        if(input_sample2 < 0)
        {
            sample2 = history_buffer[history_size + input_sample2];
        }
        else
        {
            sample2 = input[input_sample2];
        }
        output[j] += sample1 * fraction1 + sample2 * fraction2;

        table_offset++;
        table_offset %= table_size;
    }
    voice.table_offset = table_offset;

// history is bigger than input buffer.  Copy entire input buffer.
    if(history_size > size)
    {
        memcpy(history_buffer, 
            history_buffer + size,
            (history_size - size) * sizeof(double));
        memcpy(history_buffer + (history_size - size),
            buffer->get_data(),
            size * sizeof(double));
    }
    else
    {
// input is bigger than history buffer.  Copy only history size
       memcpy(history_buffer,
            buffer->get_data() + size - history_size,
            history_size * sizeof(double));
    }
//printf("Flanger::process_buffer %d\n", 
//__LINE__);


// copy the DSP buffer to the output
    memcpy(buffer->get_data(), dsp_in, size * sizeof(double));


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
        if(dsp_in)
        {
            delete [] dsp_in;
        }
        dsp_in = new double[new_dsp_allocated];
        dsp_in_allocated = new_dsp_allocated;
    }
}

void Flanger::reallocate_history(int new_size)
{
    if(new_size != history_size)
    {
// copy samples already read into the new buffers
        double *new_history = new double[new_size];
        bzero(new_history, sizeof(double) * new_size);
        if(history_buffer)
        {
            int copy_size = MIN(new_size, history_size);
            memcpy(new_history, 
                history_buffer + history_size - copy_size, 
                sizeof(double) * copy_size);
            delete [] history_buffer;
        }
        history_buffer = new_history;
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
	offset = 0.00;
	starting_phase = 0;
	depth = 10.0;
	rate = 0.20;
	wetness = 0;
}

int FlangerConfig::equivalent(FlangerConfig &that)
{
	return EQUIV(offset, that.offset) &&
		EQUIV(starting_phase, that.starting_phase) &&
		EQUIV(depth, that.depth) &&
		EQUIV(rate, that.rate) &&
		EQUIV(wetness, that.wetness);
}

void FlangerConfig::copy_from(FlangerConfig &that)
{
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
	CLAMP(offset, MIN_OFFSET, MAX_OFFSET);
	CLAMP(starting_phase, MIN_STARTING_PHASE, MAX_STARTING_PHASE);
	CLAMP(depth, MIN_DEPTH, MAX_DEPTH);
	CLAMP(rate, MIN_RATE, MAX_RATE);
	CLAMP(wetness, INFINITYGAIN, 0.0);
}








#define WINDOW_W DP(400)
#define WINDOW_H DP(200)

FlangerWindow::FlangerWindow(Flanger *plugin)
 : PluginClientWindow(plugin, 
	WINDOW_W, 
	WINDOW_H, 
	WINDOW_W, 
	WINDOW_H, 
	0)
{ 
	this->plugin = plugin; 
}

FlangerWindow::~FlangerWindow()
{
    delete offset;
    delete starting_phase;
    delete depth;
    delete rate;
    delete wetness;
}

void FlangerWindow::create_objects()
{
	int margin = client->get_theme()->widget_border;
    int x1 = margin;
	int x2 = DP(200), y = margin;
    int x3 = x2 + BC_Pot::calculate_w() + margin;
    int x4 = x3 + BC_Pot::calculate_w() + margin;
    int text_w = get_w() - margin - x4;
    int height = BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1) + margin;


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
    offset->set_precision(3);
    offset->initialize();
    y += height;


    starting_phase = new PluginParam(plugin,
        this,
        x1, 
        x2,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.starting_phase, // output_f
        0, // output_q
        "Starting phase (%):",
        MIN_STARTING_PHASE, // min
        MAX_STARTING_PHASE); // max
    starting_phase->set_precision(3);
    starting_phase->initialize();
    y += height;



    depth = new PluginParam(plugin,
        this,
        x1, 
        x3,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.depth, // output_f
        0, // output_q
        "Depth (ms):",
        MIN_DEPTH, // min
        MAX_DEPTH); // max
    depth->set_precision(3);
    depth->initialize();
    y += height;



    rate = new PluginParam(plugin,
        this,
        x1, 
        x2,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.rate, // output_f
        0, // output_q
        "Rate (Hz):",
        MIN_RATE, // min
        MAX_RATE); // max
    rate->set_precision(3);
    rate->initialize();
    y += height;



    wetness = new PluginParam(plugin,
        this,
        x1, 
        x3,
        x4,
        y, 
        text_w,
        0,  // output_i
        &plugin->config.wetness, // output_f
        0, // output_q
        "Wetness (db):",
        INFINITYGAIN, // min
        0); // max
    wetness->set_precision(3);
    wetness->initialize();
    y += height;

	show_window();
}

void FlangerWindow::update()
{
    offset->update(0, 0);
    starting_phase->update(0, 0);
    depth->update(0, 0);
    rate->update(0, 0);
    wetness->update(0, 0);
}

void FlangerWindow::param_updated()
{
}

