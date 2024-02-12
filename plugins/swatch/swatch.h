/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef SWATCH_H
#define SWATCH_H

class SwatchMain;
class SwatchEngine;
class SwatchWindow;
class SwatchServer;



#include "guicast.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "thread.h"
#include "vframe.inc"

class SwatchConfig
{
public:
	SwatchConfig();

	int equivalent(SwatchConfig &that);
	void copy_from(SwatchConfig &that);
	void interpolate(SwatchConfig &prev, 
		SwatchConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

    int brightness;
    int saturation;
    int fix_brightness;
    int angle;
};


class SwatchSlider : public BC_ISlider
{
public:
	SwatchSlider(SwatchMain *plugin, 
        SwatchWindow *gui, 
        int x, 
        int y, 
        int min,
        int max,
        int *output);
	int handle_event();
	SwatchMain *plugin;
    int *output;
};

class SwatchOption : public BC_Radial
{
public:
    SwatchOption(SwatchMain *plugin, 
        SwatchWindow *gui, 
        int x, 
        int y, 
        const char *text,
        int fix_brightness);
	int handle_event();
	SwatchMain *plugin;
    SwatchWindow *gui;
    int fix_brightness;
};


class SwatchWindow : public PluginClientWindow
{
public:
	SwatchWindow(SwatchMain *plugin);
	~SwatchWindow();
	
	void create_objects();
    void update_fixed();

	SwatchMain *plugin;
    SwatchSlider *brightness;
    SwatchSlider *saturation;
    SwatchSlider *angle;
    SwatchOption *fix_brightness;
    SwatchOption *fix_saturation;
    BC_Title *brightness_title;
    BC_Title *saturation_title;
};







class SwatchMain : public PluginVClient
{
public:
	SwatchMain(PluginServer *server);
	~SwatchMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_synthesis();
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(SwatchConfig)

	int need_reconfigure;

	VFrame *temp;
	SwatchServer *engine;
};

class SwatchPackage : public LoadPackage
{
public:
	SwatchPackage();
	int y1;
	int y2;
};

class SwatchUnit : public LoadClient
{
public:
	SwatchUnit(SwatchServer *server, SwatchMain *plugin);
	void process_package(LoadPackage *package);
	SwatchServer *server;
	SwatchMain *plugin;
	YUV yuv;
};

class SwatchServer : public LoadServer
{
public:
	SwatchServer(SwatchMain *plugin, int total_clients, int total_packages);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	SwatchMain *plugin;
};



#endif
