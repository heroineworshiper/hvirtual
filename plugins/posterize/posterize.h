
/*
 * CINELERRA
 * Copyright (C) 2008-2020 Adam Williams <broadcast at earthling dot net>
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

#ifndef POSTERIZE_H
#define POSTERIZE_H

class PosterizeMain;
class PosterizeWindow;

#include "bchash.h"
#include "mutex.h"
#include "pluginvclient.h"
#include <sys/types.h>

class PosterizeConfig
{
public:
	PosterizeConfig();
	int equivalent(PosterizeConfig &that);
	void copy_from(PosterizeConfig &that);
	void interpolate(PosterizeConfig &prev, 
		PosterizeConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();
	int steps;
};

class PosterizeMain : public PluginVClient
{
public:
	PosterizeMain(PluginServer *server);
	~PosterizeMain();

// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS(PosterizeConfig);
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
};



class PosterizeSlider : public BC_ISlider
{
public:
	PosterizeSlider(PosterizeMain *plugin, 
        PosterizeWindow *gui,
		int x, 
		int y, 
        int w,
		int *output,
		int min,
		int max);
	int handle_event();
	PosterizeMain *plugin;
    PosterizeWindow *gui;
	int *output;
};

class PosterizeText : public BC_TextBox
{
public:
    PosterizeText(PosterizeMain *plugin, 
        PosterizeWindow *gui,
        int x, 
        int y, 
        int w, 
        int *output);
    int handle_event();
	PosterizeMain *plugin;
    PosterizeWindow *gui;
	int *output;
};



class PosterizeWindow : public PluginClientWindow
{
public:
	PosterizeWindow(PosterizeMain *plugin);
	~PosterizeWindow();
	
	void create_objects();

	void update(int do_slider, int do_text);

	PosterizeMain *plugin;
	PosterizeText *text;
    PosterizeSlider *slider;
};


#endif
