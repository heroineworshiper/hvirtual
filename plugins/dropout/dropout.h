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

// A crummy transition just to test an audio transition with a GUI


#ifndef CROSSFADE_H
#define CROSSFADE_H

class DropoutMain;
class DropoutWindow;

#include "pluginaclient.h"


class DropoutSlider : public BC_ISlider
{
public:
	DropoutSlider(DropoutMain *plugin, 
        DropoutWindow *window,
        int x,
        int y,
        int w,
        int *output);
	int handle_event();
	DropoutMain *plugin;
	DropoutWindow *window;
    int *output;
};

class DropoutWindow : public PluginClientWindow
{
public:
	DropoutWindow(DropoutMain *plugin);
	void create_objects();
	DropoutMain *plugin;
	DropoutSlider *balance;
};




class DropoutMain : public PluginAClient
{
public:
	DropoutMain(PluginServer *server);
	~DropoutMain();

// required for all transition plugins
	int process_realtime(int64_t size, 
		Samples *input_ptr, 
		Samples *output_ptr);
	int uses_gui();
	int is_transition();
	const char* plugin_title();
	PluginClientWindow* new_window();
    void save_data(KeyFrame *keyframe);
    void read_data(KeyFrame *keyframe);
    void update_gui();
    int load_configuration();

    int balance;
};

#endif


