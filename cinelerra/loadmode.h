
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

#ifndef LOADMODE_H
#define LOADMODE_H

#include "guicast.h"
#include "loadmode.inc"
#include "mwindow.inc"
#include "theme.inc"

class LoadModeListBox;

class LoadModeItem : public BC_ListBoxItem
{
public:
	LoadModeItem(char *text, int value);
	int value;
};

class LoadModeToggle : public BC_Toggle
{
public:
	LoadModeToggle(int x, 
		int y, 
		LoadMode *window, 
		int value, 
		const char *images,
		const char *tooltip);
	int handle_event();
	LoadMode *window;
	int value;
};



class LoadMode
{
public:
	LoadMode(MWindow *mwindow,
		BC_WindowBase *window, 
		int x, 
		int y, 
		int *output,
		int use_nothing,
		int use_nested);
	~LoadMode();
	
	void create_objects();
	int reposition_window(int x, int y);
    void set_conform(BC_CheckBox *conform);
	static int calculate_h(BC_WindowBase *gui, Theme *theme);
	static int calculate_w(BC_WindowBase *gui, 
		Theme *theme, 
		int use_none, 
		int use_nested);
	int get_h();
	int get_x();
	int get_y();

	char* mode_to_text();
	void update();
// update a conform checkbox based on the load mode
    void update_conform();

	BC_Title *title;
	MWindow *mwindow;
	BC_WindowBase *window;
	int x;
	int y;
	int *output;
	int use_nothing;
	int use_nested;
	LoadModeToggle *mode[TOTAL_LOADMODES];
// Disable a conform widget for certain modes if it exists
    BC_CheckBox *conform;
//	BC_TextBox *textbox;
//	ArrayList<LoadModeItem*> load_modes;
//	LoadModeListBox *listbox;
};

class LoadModeListBox : public BC_ListBox
{
public:
	LoadModeListBox(BC_WindowBase *window, LoadMode *loadmode, int x, int y);
	~LoadModeListBox();

	int handle_event();

	BC_WindowBase *window;
	LoadMode *loadmode;
};

#endif
