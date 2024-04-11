/*
 * CINELERRA
 * Copyright (C) 1997-2024 Adam Williams <broadcast at earthling dot net>
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


#ifndef CURVES_H
#define CURVES_H

#include "cicolors.h"
#include "curves.inc"
#include "histogramtools.h"
#include "loadbalance.h"
#include "pluginvclient.h"


class CurvesConfig
{
public:
    CurvesConfig();
	int equivalent(CurvesConfig &that);
	void copy_from(CurvesConfig &that);
	void interpolate(CurvesConfig &prev, 
		CurvesConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
// Used by constructor
	void reset(int mode, int defaults);
	void boundaries();
    int set_point(int mode, curve_point_t &point);
    
    ArrayList<curve_point_t> points[HISTOGRAM_MODES];
    int plot;
    int split;
};


class CurvesPackage : public LoadPackage
{
public:
	CurvesPackage();
	int start, end;
};

class CurvesUnit : public LoadClient
{
public:
	CurvesUnit(CurvesEngine *server, CurvesMain *plugin);
	~CurvesUnit();
	void process_package(LoadPackage *package);
	CurvesEngine *server;
	CurvesMain *plugin;
};

class CurvesEngine : public LoadServer
{
public:
	CurvesEngine(CurvesMain *plugin, 
		int total_clients, 
		int total_packages);
	void process_packages(VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	CurvesMain *plugin;
	int total_size;
	VFrame *data;
};

class CurveTools : public HistogramTools
{
public:
    CurveTools(CurvesMain *plugin);
    float calculate_level(float input, int mode, int do_value);
    CurvesMain *plugin;
};

class CurvesMain : public PluginVClient
{
public:
	CurvesMain(PluginServer *server);
	~CurvesMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void *data, int size);
	int calculate_use_opengl();
	int handle_opengl();
	void calculate_histogram(VFrame *data, int do_value);
	float calculate_level(float input, int mode, int do_value);
    float calculate_bezier(curve_point_t *p1, 
        curve_point_t *p2, 
        curve_point_t *p3, 
        curve_point_t *p4, 
        float x);

	PLUGIN_CLASS_MEMBERS2(CurvesConfig)


    CurvesEngine *engine;
    CurveTools *tools;
// Current channel being viewed
	int mode;
	int w, h;
    int current_point;
    YUV yuv;
};





#endif




