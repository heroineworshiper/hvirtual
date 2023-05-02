
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

#ifndef TREMOLO_H
#define TREMOLO_H

class Tremolo;

#include "pluginaclient.h"


#define SINE 0
#define SAWTOOTH 1
#define SAWTOOTH2 2
#define SQUARE 3
#define TRIANGLE 4
#define TOTAL_WAVEFORMS 5

class TremoloConfig
{
public:
	TremoloConfig();


	int equivalent(TremoloConfig &that);
	void copy_from(TremoloConfig &that);
	void interpolate(TremoloConfig &prev, 
		TremoloConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();


// starting phase offset in ms
	float offset;
// how much the phase oscillates in ms
	float depth;
// rate of phase oscillation in Hz
	float rate;
	int waveform;
};




class Tremolo : public PluginAClient
{
public:
	Tremolo(PluginServer *server);
	~Tremolo();

	void update_gui();



// required for all realtime/multichannel plugins
	PLUGIN_CLASS_MEMBERS(TremoloConfig);
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

    int table_size;
    double *table;
    int table_offset;
// detect seeking
    int64_t last_position;
	int need_reconfigure;
};



class TremoloWaveForm;
class TremoloWindow : public PluginClientWindow
{
public:
	TremoloWindow(Tremolo *plugin);
	~TremoloWindow();
	
	void create_objects();
    void update();
    static char* waveform_to_text(char *text, int waveform);
    void param_updated();

	Tremolo *plugin;
    PluginParam *offset;
    PluginParam *depth;
    PluginParam *rate;
    TremoloWaveForm *waveform;
};


class TremoloWaveForm : public BC_PopupMenu
{
public:
	TremoloWaveForm(Tremolo *plugin, int x, int y, char *text);
	~TremoloWaveForm();

	void create_objects();
	Tremolo *plugin;
};

class TremoloWaveFormItem : public BC_MenuItem
{
public:
	TremoloWaveFormItem(Tremolo *plugin, char *text, int value);
	~TremoloWaveFormItem();
	
	int handle_event();
	
	int value;
	Tremolo *plugin;
};




#endif



