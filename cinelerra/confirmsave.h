
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

#ifndef CONFIRMSAVE_H
#define CONFIRMSAVE_H

#include "asset.inc"
#include "guicast.h"
#include "mwindow.inc"

class ConfirmSaveOkButton;
class ConfirmSaveCancelButton;

class ConfirmSave
{
public:
	ConfirmSave();
	~ConfirmSave();

// Return values:
// 1 cancel
// 0 replace or doesn't exist yet
	static int test_file(MWindow *mwindow, char *path);
	static int test_files(MWindow *mwindow, ArrayList<char*> *paths);
	static int test_files(MWindow *mwindow, ArrayList<string*> *paths);

};

class ConfirmSaveWindow : public BC_Window
{
public:
	ConfirmSaveWindow(MWindow *mwindow, ArrayList<BC_ListBoxItem*> *list);
	~ConfirmSaveWindow();

	void create_objects();
	int resize_event(int w, int h);

	ArrayList<BC_ListBoxItem*> *list;
	BC_Title *title;
	BC_ListBox *listbox;
};

#endif
