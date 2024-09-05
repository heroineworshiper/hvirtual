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

#ifndef LUTEFFECT_H
#define LUTEFFECT_H


#include "browsebutton.inc"
#include "guicast.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include <string>

class LUTConfig
{
public:
    LUTConfig();
	void copy_from(LUTConfig &src);
	int equivalent(LUTConfig &src);
	void interpolate(LUTConfig &prev, 
		LUTConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
    string path;
};

class LUTWindow;
class LUTEffect;
class LutPath : public BC_TextBox
{
public:
	LutPath(LUTWindow *window, int x, int y, int h);
	int handle_event();
	LUTWindow *window;
};


class LUTWindow : public PluginClientWindow
{
public:
    LUTWindow(LUTEffect *client);
    ~LUTWindow();
    void create_objects();
    void update();
    LUTEffect *client;
    BrowseButton *browse;
	LutPath *path;
// Suggestions for the textbox
	ArrayList<BC_ListBoxItem*> *file_entries;
};

class LUTServer : public LoadServer
{
public:
	LUTServer(LUTEffect *plugin);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	LUTEffect *plugin;
};

class LUTPackage : public LoadPackage
{
public:
	LUTPackage();
	int y1, y2;
};

class LUTUnit : public LoadClient
{
public:
	LUTUnit(LUTEffect *plugin, LUTServer *server);
	void process_package(LoadPackage *package);

	LUTEffect *plugin;
};

class LUTEffect : public PluginVClient
{
public:
    LUTEffect(PluginServer *server);
    ~LUTEffect();
    PLUGIN_CLASS_MEMBERS(LUTConfig);
    int process_buffer(VFrame *frame,
	    int64_t start_position,
	    double frame_rate);
    int is_realtime();
    void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
    int handle_opengl();
    LUTServer *engine;
    int reconfigure;
// the LUT after conversion to float
    VFrame *table;
};










#endif




