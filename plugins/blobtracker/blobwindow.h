/*
 * CINELERRA
 * Copyright (C) 2026 Adam Williams <broadcast at earthling dot net>
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

#ifndef BLOBWINDOW_H
#define BLOBWINDOW_H

#include "guicast.h"
#include "blobtracker.inc"

class BlobTrackMode : public BC_PopupMenu
{
public:
	BlobTrackMode(int *output, 
        PluginClient *plugin, 
        BC_WindowBase *gui, 
        int x, 
        int y);
	int handle_event();
	void create_objects();
	static int calculate_w(BC_WindowBase *gui);
	static int from_text(const char *text);
	static const char* to_text(int mode);
	PluginClient *plugin;
    int *output;
};


class BlobLayer : public BC_PopupMenu
{
public:
	BlobLayer(int *output, PluginClient *plugin, BC_WindowBase *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(BC_WindowBase *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	PluginClient *plugin;
    int *output;
};

class BlobToggle : public BC_CheckBox
{
public:
	BlobToggle(PluginClient *plugin, 
		int *output, 
		int x, 
		int y,
		const char *text);
	int handle_event();

	PluginClient *plugin;
	int *output;
};


class BlobSlider : public BC_ISlider
{
public:
    BlobSlider(BlobTracker *plugin, int x, int y, int min, int max, int *output);
    int handle_event();
    BlobTracker *plugin;
    int *output;
};

class BlobWindow : public PluginClientWindow
{
public:
	BlobWindow(BlobTracker *plugin);
	~BlobWindow();

    void create_objects();
    void update_mode();

    BlobTracker *plugin;
    BlobTrackMode *tracking_type;
    BlobLayer *ref_layer;
    BlobLayer *targ_layer;
    BlobToggle *draw_guides;
    BlobToggle *stabilize;
    BlobSlider *min_size;
    BC_Title *detected_size;
};




#endif



