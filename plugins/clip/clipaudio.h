/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef CLIPAUDIO_H
#define CLIPAUDIO_H


// clip audio at a certain level

class Clip;

#include "pluginaclient.h"

class ClipConfig
{
public:
	ClipConfig();
	int equivalent(ClipConfig &that);
	void copy_from(ClipConfig &that);
	void interpolate(ClipConfig &prev, 
		ClipConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	double level;
};


class ClipLevel : public BC_FSlider
{
public:
	ClipLevel(Clip *plugin, int x, int y);
	int handle_event();
	Clip *plugin;
};

class ClipWindow : public PluginClientWindow
{
public:
	ClipWindow(Clip *gain);
	~ClipWindow();
	
	void create_objects();

	Clip *plugin;
	ClipLevel *level;
};




class Clip : public PluginAClient
{
public:
	Clip(PluginServer *server);
	~Clip();

	int process_realtime(int64_t size, Samples *input_ptr, Samples *output_ptr);

	PLUGIN_CLASS_MEMBERS(ClipConfig)
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();


	DB db;
};

#endif
