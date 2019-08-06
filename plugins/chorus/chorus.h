
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

#ifndef CHORUS_H
#define CHORUS_H

class Chorus;

#include "pluginaclient.h"

class ChorusConfig
{
public:
	ChorusConfig();


	int equivalent(ChorusConfig &that);
	void copy_from(ChorusConfig &that);
	void interpolate(ChorusConfig &prev, 
		ChorusConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();


// number of voices per channel to be rendered
    int voices;
// starting phase offset in ms
	float offset;
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
// source channel
    int src_channel;
// destination channel
    int dst_channel;
};


// each sample in the flanging waveform
typedef struct 
{
    double input_sample;
    double input_period;
} flange_sample_t;


class Chorus : public PluginAClient
{
public:
	Chorus(PluginServer *server);
	~Chorus();

	void update_gui();



// required for all realtime/multichannel plugins
	PLUGIN_CLASS_MEMBERS(ChorusConfig);
    int process_buffer(int64_t size, 
	    Samples **buffer, 
	    int64_t start_position,
	    int sample_rate);
    void reallocate_dsp(int new_dsp_allocated);
    void reallocate_history(int new_allocation);
    int total_voices();

	int is_realtime();
	int is_synthesis();
	int is_multichannel();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	double **history_buffer;
// Number of samples in the history buffer 
	int64_t history_size;

// the temporary all voices are painted on
	double **dsp_in;
    int dsp_in_allocated;

    Voice *voices;
// flanging table is a whole number of samples that repeats
// always an even number
    int table_size;
    flange_sample_t *flanging_table;

// detect seeking
    int64_t last_position;

	int need_reconfigure;
};




class ChorusWindow : public PluginClientWindow
{
public:
	ChorusWindow(Chorus *plugin);
	~ChorusWindow();
	
	void create_objects();
    void update();
    void param_updated();

	Chorus *plugin;
    PluginParam *voices;
    PluginParam *offset;
    PluginParam *depth;
    PluginParam *rate;
    PluginParam *wetness;
};




#endif



