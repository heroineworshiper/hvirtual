
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef THRESHOLD_H
#define THRESHOLD_H


#include "histogramengine.inc"
#include "loadbalance.h"
#include "thresholdwindow.inc"
#include "cicolors.inc"
#include "pluginvclient.h"


class ThresholdEngine;

class ThresholdConfig
{
public:
	ThresholdConfig();
	int equivalent(ThresholdConfig &that);
	void copy_from(ThresholdConfig &that);
	void interpolate(ThresholdConfig &prev,
		ThresholdConfig &next,
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void reset();
	void boundaries();

	float min;
	float max;
	int plot;
};



class ThresholdMain : public PluginVClient
{
public:
	ThresholdMain(PluginServer *server);
	~ThresholdMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void *data, int size);
	void calculate_histogram(VFrame *frame);
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(ThresholdConfig);
	HistogramEngine *engine;
	ThresholdEngine *threshold_engine;
};






class ThresholdPackage : public LoadPackage
{
public:
	ThresholdPackage();
	int start;
	int end;
};


class ThresholdUnit : public LoadClient
{
public:
	ThresholdUnit(ThresholdEngine *server);
	void process_package(LoadPackage *package);
	
	ThresholdEngine *server;
};


class ThresholdEngine : public LoadServer
{
public:
	ThresholdEngine(ThresholdMain *plugin);
	~ThresholdEngine();

	void process_packages(VFrame *data);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	YUV *yuv;
	ThresholdMain *plugin;
	VFrame *data;
};






#endif
