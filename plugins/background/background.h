
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

#ifndef BACKGROUND_H
#define BACKGROUND_H

class BackgroundMain;
class BackgroundEngine;
class BackgroundThread;
class BackgroundWindow;
class BackgroundServer;


#define MAXRADIUS 10000

#include "colorpicker.h"
#include "bchash.inc"
#include "filexml.inc"
#include "guicast.h"
#include "loadbalance.h"
#include "overlayframe.inc"
#include "cicolors.h"
#include "pluginvclient.h"
#include "thread.h"
#include "vframe.inc"

class BackgroundConfig
{
public:
	BackgroundConfig();

	int equivalent(BackgroundConfig &that);
	void copy_from(BackgroundConfig &that);
	void interpolate(BackgroundConfig &prev, 
		BackgroundConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
// Int to hex triplet conversion
	int get_in_color();
	int get_out_color();

	int r, g, b, a;
};




class BackgroundWindow : public PluginClientWindow
{
public:
	BackgroundWindow(BackgroundMain *plugin);
	~BackgroundWindow();
	
	void create_objects();
	void update_in_color();
	void update_out_color();
	void update_shape();

	BackgroundMain *plugin;
	BC_Title *angle_title;
	BackgroundAngle *angle;
	BackgroundInRadius *in_radius;
	BackgroundOutRadius *out_radius;
	BackgroundInColorButton *in_color;
	BackgroundOutColorButton *out_color;
	BackgroundInColorThread *in_color_thread;
	BackgroundOutColorThread *out_color_thread;
	BackgroundShape *shape;
	BC_Title *shape_title;
	BackgroundCenterX *center_x;
	BC_Title *center_x_title;
	BC_Title *center_y_title;
	BackgroundCenterY *center_y;
	BackgroundRate *rate;
	int in_color_x, in_color_y;
	int out_color_x, out_color_y;
	int shape_x, shape_y;
};







class BackgroundMain : public PluginVClient
{
public:
	BackgroundMain(PluginServer *server);
	~BackgroundMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_synthesis();
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(BackgroundConfig)
};



#endif
