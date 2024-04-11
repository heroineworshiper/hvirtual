/*
 * CINELERRA
 * Copyright (C) 1997-2024 Adam Williams <broadcast at earthling dot net>
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


#ifndef CURVESWINDOW_H
#define CURVESWINDOW_H


#include "curves.inc"
#include "pluginvclient.h"



class CurvesReset : public BC_GenericButton
{
public:
    CurvesReset(CurvesMain *plugin, 
		int x,
		int y);
	int handle_event();
	CurvesMain *plugin;
};

class CurvesMode : public BC_Radial
{
public:
	CurvesMode(CurvesMain *plugin, 
		int x, 
		int y,
		int value,
		char *text);
	int handle_event();
	CurvesMain *plugin;
	int value;
};


class CurvesToggle : public BC_CheckBox
{
public:
	CurvesToggle(CurvesMain *plugin, 
	    int x, 
	    int y,
        int *output,
        const char *text);
	int handle_event();
	CurvesMain *plugin;
    int *output;
};

class CurvesCanvas : public BC_SubWindow
{
public:
	CurvesCanvas(CurvesMain *plugin,
		CurvesWindow *gui,
		int x,
		int y,
		int w,
		int h);
    void point_to_canvas(int &x, int &y, curve_point_t *point);
    void canvas_to_point(curve_point_t &point, int x, int y);
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	CurvesMain *plugin;
	CurvesWindow *gui;
    int is_drag;
    int x_offset;
    int y_offset;
};

class CurvesText : public BC_TumbleTextBox
{
public:
	CurvesText(CurvesMain *plugin,
		CurvesWindow *gui,
		int x,
		int y,
        int w,
        int min,
        int max,
        int is_x,
        int is_y,
        int is_number);

	CurvesText(CurvesMain *plugin,
		CurvesWindow *gui,
		int x,
		int y,
        int w,
        float min,
        float max,
        int is_x,
        int is_y,
        int is_number);

	int handle_event();

    int is_x;
    int is_y;
    int is_number;
	CurvesMain *plugin;
	CurvesWindow *gui;
};

class CurvesWindow : public PluginClientWindow
{
public:
    CurvesWindow(CurvesMain *plugin);
    ~CurvesWindow();
    void create_objects();
    int resize_event(int w, int h);
    void update(int do_canvas, int do_toggles, int do_text);

    CurvesCanvas *canvas;
    CurvesToggle *plot;
    CurvesToggle *split;
    CurvesMode *mode_v, *mode_r, *mode_g, *mode_b;
    CurvesReset *reset;
    CurvesText *number;
    CurvesText *x;
    CurvesText *y;
    BC_Title *title1;
    BC_Title *title2;
    BC_Title *title3;
    int canvas_x, canvas_y, canvas_w, canvas_h;
    
    CurvesMain *plugin;
};




#endif



