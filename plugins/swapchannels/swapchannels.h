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

#ifndef SWAPCHANNELS_H
#define SWAPCHANNELS_H

#include "bchash.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "vframe.inc"





class SwapMain;


class SwapConfig
{
public:
	SwapConfig();

	int equivalent(SwapConfig &that);
	void copy_from(SwapConfig &that);

	int r_channel;
	int g_channel;
	int b_channel;
	int a_channel;
	int r_layer;
	int g_layer;
	int b_layer;
	int a_layer;
};

// source track of the channel
class SwapLayerMenu;
class SwapLayerItem : public BC_MenuItem
{
public:
	SwapLayerItem(SwapLayerMenu *menu, const char *title);

	int handle_event();

	SwapLayerMenu *menu;
	char *title;
};

class SwapLayerMenu : public BC_PopupMenu
{
public:
    SwapLayerMenu(SwapMain *plugin,
        int *output,
        int x,
        int y,
        int w);

    void create_objects();
    int handle_event();

    SwapMain *plugin;
    int *output;
};


// source channel
class SwapChannelMenu : public BC_PopupMenu
{
public:
	SwapChannelMenu(SwapMain *client, 
        int *output, 
        int x, 
        int y, 
        int w);


	int handle_event();
	void create_objects();

	SwapMain *client;
	int *output;
};

class SwapItem : public BC_MenuItem
{
public:
	SwapItem(SwapChannelMenu *menu, const char *title);

	int handle_event();

	SwapChannelMenu *menu;
	char *title;
};

class SwapWindow : public PluginClientWindow
{
public:
	SwapWindow(SwapMain *plugin);
	~SwapWindow();


	void create_objects();

	SwapMain *plugin;
	SwapChannelMenu *r_channel;
	SwapChannelMenu *g_channel;
	SwapChannelMenu *b_channel;
	SwapChannelMenu *a_channel;
    SwapLayerMenu *r_track;
    SwapLayerMenu *g_track;
    SwapLayerMenu *b_track;
    SwapLayerMenu *a_track;
};







class SwapMain : public PluginVClient
{
public:
	SwapMain(PluginServer *server);
	~SwapMain();

	PLUGIN_CLASS_MEMBERS(SwapConfig)
// required for all realtime plugins
	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int is_synthesis();
    int is_multichannel();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int handle_opengl();
    void color_switch(char *output_frag, 
        int src_channel, 
        int src_layer, 
        const char *dst_channel);







	void reset();




// parameters needed for processor
	static const char* output_to_text(int value);
    const char* output_to_track(int number);
	int text_to_output(const char *text);

	VFrame *temp;
    int input_track;
};


#endif
