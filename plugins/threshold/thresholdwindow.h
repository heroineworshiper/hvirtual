
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

#ifndef THRESHOLDWINDOW_H
#define THRESHOLDWINDOW_H

#include "guicast.h"
#include "pluginvclient.h"
#include "threshold.inc"
#include "thresholdwindow.inc"

class ThresholdMin : public BC_TumbleTextBox
{
public:
	ThresholdMin(ThresholdMain *plugin,
		ThresholdWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	ThresholdMain *plugin;
	ThresholdWindow *gui;
};

class ThresholdMax : public BC_TumbleTextBox
{
public:
	ThresholdMax(ThresholdMain *plugin,
		ThresholdWindow *gui,
		int x,
		int y,
		int w);
	int handle_event();
	ThresholdMain *plugin;
	ThresholdWindow *gui;
};

class ThresholdPlot : public BC_CheckBox
{
public:
	ThresholdPlot(ThresholdMain *plugin,
		int x,
		int y);
	int handle_event();
	ThresholdMain *plugin;
};

class ThresholdCanvas : public BC_SubWindow
{
public:
	ThresholdCanvas(ThresholdMain *plugin,
		ThresholdWindow *gui,
		int x,
		int y,
		int w,
		int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	void draw();

	ThresholdMain *plugin;
	ThresholdWindow *gui;
	int state;
	enum
	{
		NO_OPERATION,
		DRAG_SELECTION
	};
	int x1;
	int x2;
	int center_x;
};

class ThresholdWindow : public PluginClientWindow
{
public:
	ThresholdWindow(ThresholdMain *plugin);
	~ThresholdWindow();
	
	void create_objects();

	ThresholdMain *plugin;
	ThresholdMin *min;
	ThresholdMax *max;
	ThresholdCanvas *canvas;
	ThresholdPlot *plot;
};



#endif






