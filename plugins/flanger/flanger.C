
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
    flanging_waveform = 0;
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
    
    delete [] voices;
    delete [] flanging_waveform;
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


    if(!voices)
    {
        voices = new Voice[PluginClient::total_in_buffers];
    }

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
        dsp_in_length = 0;
        if(voices)
        {
// reset the accumulators
            for(int i = 0; i < PluginClient::total_in_buffers; i++)
		    {
 			    voices[i].accum = 0;
            }
        }
    }


    if(need_reconfigure)
    {
        need_reconfigure = 0;

        if(flanging_waveform)
        {
            delete [] flanging_waveform;
        }

// flanging waveform is a whole number of samples that repeats
        flanging_period = (int)((double)sample_rate / rate);
        flanging_waveform = new flange_sample_t[flanging_period];
        double offset = 0;
        for(int i = 0; i < flanging_period; i++)
        {
            double current_period = 1.0;
            
            offset += current_period;
        }
    }
    
// how much buffer to allocate
    int new_allocation = size + (int)((config.offset + config.depth) * 
        sample_rate / 
        1000 + 1);
    reallocate_dsp(size);


// paint each voice
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
	{
        
    }





// copy the DSP buffer to the output
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
    {
        memcpy(buffer[i]->get_data(), dsp_in[i], size * sizeof(double));
    }

// shift the DSP buffer forward
    int remane = dsp_in_allocated - size;
    for(int i = 0; i < PluginClient::total_in_buffers; i++)
    {
        memcpy(dsp_in[i], dsp_in[i] + size, remane * sizeof(double));
        bzero(dsp_in[i] + remane, size * sizeof(double));
    }

    dsp_in_length -= size;
    
    
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
                memcpy(new_dsp, old_dsp, sizeof(double) * dsp_in_length);
                delete [] old_dsp;
            }
            bzero(new_dsp + dsp_in_allocated, 
                sizeof(double) * (new_dsp_allocated - dsp_in_allocated));
            dsp_in[i] = new_dsp;
        }
        dsp_in_allocated = new_dsp_allocated;
//printf("Reverb::reallocate_dsp %d dsp_in_allocated=%d\n", __LINE__, dsp_in_allocated);
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
    accum = 0;
    phase = 0;
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


