
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

#ifndef FLANGER_H
#define FLANGER_H

class Flanger;

#include "pluginaclient.h"


class FlangerConfig
{
public:
	FlangerConfig();


	int equivalent(FlangerConfig &that);
	void copy_from(FlangerConfig &that);
	void interpolate(FlangerConfig &prev, 
		FlangerConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();

// phase offset in ms
	float offset;
// starting position of oscillation in %
	float starting_phase;
// how much the phase oscillates in ms
	float depth;
// rate of phase oscillation in Hz
	float rate;
// how much of input signal
	float wetness;
};


// state of a single voice
class Voice
{
public:
    Voice();

// position in the waveform table
    int table_offset;
};

// each sample in the flanging waveform
typedef struct 
{
    double input_sample;
    double input_period;
} flange_sample_t;

class Flanger : public PluginAClient
{
public:
	Flanger(PluginServer *server);
	~Flanger();

	void update_gui();



// required for all realtime/multichannel plugins
	PLUGIN_CLASS_MEMBERS(FlangerConfig);
    int process_buffer(int64_t size, 
	    Samples *buffer, 
	    int64_t start_position,
	    int sample_rate);
    void reallocate_dsp(int new_dsp_allocated);
    void reallocate_history(int new_allocation);

	int is_realtime();
	int is_synthesis();
	int is_multichannel();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	double *history_buffer;
// Number of samples in the history buffer 
	int64_t history_size;

// the temporary all voices are painted on
	double *dsp_in;
    int dsp_in_allocated;

    Voice voice;
// flanging table is a whole number of samples that repeats
// always an even number
    int table_size;
    flange_sample_t *flanging_table;

// detect seeking
    int64_t last_position;

	int need_reconfigure;
};





class FlangerWindow : public PluginClientWindow
{
public:
	FlangerWindow(Flanger *plugin);
	~FlangerWindow();
	
	void create_objects();
    void update();
    void param_updated();

	Flanger *plugin;
    PluginParam *offset;
    PluginParam *starting_phase;
    PluginParam *depth;
    PluginParam *rate;
    PluginParam *wetness;
};




#endif



