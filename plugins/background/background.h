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

#ifndef BACKGROUND_H
#define BACKGROUND_H

class BackgroundMain;
class BackgroundThread;
class BackgroundWindow;


#include "colorpicker.h"
#include "bchash.inc"
#include "filexml.inc"
#include "guicast.h"
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
	int get_color();

	int r, g, b, a;
    int type;
#define COLOR 0
#define LUT 1
};

class BackgroundColorObjects : public ColorObjects
{
public:
    BackgroundColorObjects(BackgroundWindow *window, 
        BackgroundMain *plugin,
        int x,
        int y);
    void handle_event();
    
    BackgroundWindow *window;
    BackgroundMain *plugin;
};

class BackgroundType : public BC_PopupMenu
{
public:
    BackgroundType(BackgroundWindow *window, 
        BackgroundMain *plugin, 
        int x,
        int y,
        int w);
    int handle_event();
    static int text_to_format(char *text);
    static char* format_to_text(int format);
    BackgroundWindow *window;
    BackgroundMain *plugin;
};

class BackgroundWindow : public PluginClientWindow
{
public:
	BackgroundWindow(BackgroundMain *plugin);
	~BackgroundWindow();
	
	void create_objects();
	void update();

	BackgroundMain *plugin;
	BackgroundColorObjects *color_objs;
    BackgroundType *type;
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
    

	PLUGIN_CLASS_MEMBERS(BackgroundConfig)
    YUV yuv;
};



#endif
