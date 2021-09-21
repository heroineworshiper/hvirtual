
/*
 * CINELERRA
 * Copyright (C) 2021 Adam Williams <broadcast at earthling dot net>
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

#ifndef SWAPASSET_H
#define SWAPASSET_H

#include "bcdialog.h"
#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "swapasset.inc"

class SwapAssetDialog;

class SwapAssetThread : public BC_DialogThread
{
public:
	SwapAssetThread(MWindow *mwindow);
	~SwapAssetThread();

	void start();
	BC_Window* new_gui();
	void handle_close_event(int result);

    ArrayList <BC_ListBoxItem*> path_listitems;
	MWindow *mwindow;

	string old_path;
	string new_path;
};


class SwapAssetPath : public BC_PopupTextBox
{
public:
    SwapAssetPath(int x, 
        int y, 
        int w,
        int h, 
        string *output,
        SwapAssetThread *thread,
        SwapAssetDialog *gui);
    int handle_event();
    SwapAssetThread *thread;
    SwapAssetDialog *gui;
    string *output;
};

class SwapAssetDialog : public BC_Window
{
public:
	SwapAssetDialog(MWindow *mwindow, 
		SwapAssetThread *thread, 
		int x, 
		int y);
	~SwapAssetDialog();
	
    int resize_event(int w, int h);
	void create_objects();
    void update();
	
	MWindow *mwindow;
	SwapAssetThread *thread;
	SwapAssetPath *old_path;
	SwapAssetPath *new_path;
    int x1;
};


#endif
