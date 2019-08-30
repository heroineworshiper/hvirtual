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


// Base classes for compressors

#ifndef COMPRESSORTOOLS_H
#define COMPRESSORTOOLS_H


#include "guicast.h"
#include "pluginclient.inc"
#include "samples.inc"


#define MIN_ATTACK -10
#define MAX_ATTACK 10
#define MIN_DECAY 0
#define MAX_DECAY 255
#define MIN_TRIGGER 0
#define MAX_TRIGGER 255




// get sample from trigger buffer
#define GET_TRIGGER(buffer, offset) \
double sample = 0; \
switch(config->input) \
{ \
	case CompressorConfigBase::MAX: \
	{ \
		double max = 0; \
		for(int channel = 0; channel < channels; channel++) \
		{ \
			sample = fabs((buffer)[offset]); \
			if(sample > max) max = sample; \
		} \
		sample = max; \
		break; \
	} \
 \
	case CompressorConfigBase::TRIGGER: \
		sample = fabs(trigger_buffer[offset]); \
		break; \
 \
	case CompressorConfigBase::SUM: \
	{ \
		double max = 0; \
		for(int channel = 0; channel < channels; channel++) \
		{ \
			sample = fabs((buffer)[offset]); \
			max += sample; \
		} \
		sample = max; \
		break; \
	} \
}





typedef struct
{
// DB from min_db - 0
	double x, y;
} compressor_point_t;

class BandConfig
{
public:
    BandConfig();
    virtual ~BandConfig();

    void copy_from(BandConfig *src);
    int equiv(BandConfig *src);

	ArrayList<compressor_point_t> levels;
    int solo;
    int bypass;
// units of seconds
//	double readahead_len;
    double attack_len;
	double release_len;

// upper frequency in Hz
    int freq;
};

class CompressorConfigBase
{
public:
    CompressorConfigBase(int total_bands);
    virtual ~CompressorConfigBase();

    virtual void copy_from(CompressorConfigBase &that);
    virtual int equivalent(CompressorConfigBase &that);

    
    void remove_point(int band, int number);
    int set_point(int band, double x, double y);
    double calculate_db(int band, double x);
    double get_x(int band, int number);
    double get_y(int band, int number);
	double calculate_gain(int band, double input);

// Calculate linear output from linear input
	double calculate_output(int band, double x);

// min DB of the graph
	double min_db;
// max DB of the graph
    double max_db;

    BandConfig *bands;
    int total_bands;
    int current_band;
	int trigger;
	int input;
	enum
	{
		TRIGGER,
		MAX,
		SUM
	};
	double min_x, min_y;
	double max_x, max_y;
	int smoothing_only;
};


class CompressorCanvasBase : public BC_SubWindow
{
public:
	CompressorCanvasBase(CompressorConfigBase *config, 
        PluginClient *plugin,
        PluginClientWindow *window,
        int x, 
        int y, 
        int w, 
        int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
    void create_objects();
	void draw_scales();
    void update();
    int x_to_y(int band, int x);
    int db_to_x(double db);
    int db_to_y(double db);
    double x_to_db(int x);
    double y_to_db(int y);

    virtual void update_window();

	enum
	{
		NONE,
		DRAG
	};

// clickable area of canvas
    int graph_x;
    int graph_y;
    int graph_w;
    int graph_h;
	int current_point;
	int current_operation;
    int divisions;
    int subdivisions;
	CompressorConfigBase *config;
    PluginClient *plugin;
    PluginClientWindow *window;
};





class CompressorEngine
{
public:
    CompressorEngine(CompressorConfigBase *config,
        int band);
    ~CompressorEngine();
    
    void reset();
    void calculate_ranges(int *attack_samples,
        int *release_samples,
        int *preview_samples,
        int sample_rate);
    void process(Samples **output_buffer,
        Samples **input_buffer,
        int size,
        int sample_rate,
        int channels,
        int64_t start_position);

    CompressorConfigBase *config;
    int band;
// the current line segment defining the smooth signal level
// starting input value of line segment
	double slope_value1;
// ending input value of line segment
	double slope_value2;
// samples comprising the line segment
	int slope_samples;
// samples from the start of the line to the peak that determined the slope
    int peak_samples;
// current sample from 0 to slope_samples
	int slope_current_sample;
// current value in the line segment
	double current_value;
};



#endif




