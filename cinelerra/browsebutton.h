
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

#ifndef BROWSEBUTTON_H
#define BROWSEBUTTON_H

#include "bcdialog.h"
#include "guicast.h"
#include "mutex.inc"
#include "theme.inc"

class BrowseButtonWindow;

class BrowseButton : public BC_Button, public BC_DialogThread
{
public:
	BrowseButton(Theme *theme, 
		BC_WindowBase *parent_window, 
		BC_TextBox *textbox, 
		int x, 
		int y, 
		const char *init_directory, 
		const char *title, 
		const char *caption, 
		int want_directory = 0);
	~BrowseButton();
	
	int handle_event();
    
    BC_Window* new_gui();
	void handle_done_event(int result);
	void handle_close_event(int result);


	int want_directory;
	char result[BCTEXTLEN];
	const char *title;
	const char *caption;
	const char *init_directory;
	BC_TextBox *textbox;
	Theme *theme;
	BC_WindowBase *parent_window;
	int x, y;
};

class BrowseButtonWindow : public BC_FileBox
{
public:
	BrowseButtonWindow(Theme *theme,
		BrowseButton *button,
		BC_WindowBase *parent_window, 
		const char *init_directory, 
		const char *title, 
		const char *caption, 
		int want_directory);
	~BrowseButtonWindow();
};





#endif
