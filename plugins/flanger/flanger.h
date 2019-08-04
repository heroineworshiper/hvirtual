
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

#include "flangerwindow.h"
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

// 1 voice for a flange.  More voices for a chorus
    int voices;
// phase offset in seconds
	float offset;
// starting position of oscillation in %
	float starting_phase;
// how much the phase oscillates
	float depth;
// rate of phase oscillation in Hz
	float rate;
// how much of input signal
	float wetness;
};




class Flanger : public PluginAClient
{
public:
	Flanger(PluginServer *server);
	~Flanger();

	void update_gui();



// required for all realtime/multichannel plugins
	PLUGIN_CLASS_MEMBERS(FlangerConfig);
    int process_buffer(int64_t size, 
	    Samples **buffer, 
	    int64_t start_position,
	    int sample_rate);
    double gauss(double sigma, double center, double x);
    void calculate_envelope();
    void reallocate_dsp(int new_dsp_allocated);

	int is_realtime();
	int is_synthesis();
	int is_multichannel();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

// the output all reflections are painted on
	double **dsp_in;
// may have to expand it for fft windows larger than the reflected time
    int dsp_in_allocated;
// total samples written into dsp_in
	int dsp_in_length;

// detect seeking
    int64_t last_position;

	int need_reconfigure;
};



#endif



