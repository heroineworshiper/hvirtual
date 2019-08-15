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


#define MIN_ATTACK -10
#define MAX_ATTACK 10
#define MIN_DECAY 0
#define MAX_DECAY 255
#define MIN_TRIGGER 0
#define MAX_TRIGGER 255



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

// upper frequency in Hz
    int freq;
};

class CompressorConfigBase
{
public:
    CompressorConfigBase(int total_bands);
    virtual ~CompressorConfigBase();

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







#endif




