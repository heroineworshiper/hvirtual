
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

#ifndef GWINDOWGUI_H
#define GWINDOWGUI_H

#include "automation.inc"
#include "guicast.h"
#include "mwindow.inc"

class GWindowToggle;

#define ASSETS 0
#define TITLES 1
#define TRANSITIONS 2
#define PLUGIN_AUTOS 3
#define OTHER_TOGGLES 4

class GWindowGUI : public BC_Window
{
public:
	GWindowGUI(MWindow *mwindow, int w, int h);
	static void calculate_extents(BC_WindowBase *gui, int *w, int *h, int *x2);
	void create_objects();
	int translation_event();
	int close_event();
	int keypress_event();
	void update_toggles(int use_lock);
	void update_mwindow();
	int cursor_motion_event();

	int drag_operation;
	int new_status;

	MWindow *mwindow;
	GWindowToggle *other[OTHER_TOGGLES];
	GWindowToggle *auto_toggle[AUTOMATION_TOTAL];
};

class GWindowToggle : public BC_CheckBox
{
public:
	GWindowToggle(MWindow *mwindow, 
		GWindowGUI *gui, 
		int x, 
		int y, 
		int subscript,
		int other, 
		const char *text);
	int handle_event();
	void update();

	static int* get_main_value(MWindow *mwindow, int subscript, int other);

	int button_press_event();
	int button_release_event();

	MWindow *mwindow;
	GWindowGUI *gui;
	int subscript;
	int other;
};

#endif
