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

#ifndef TRANSITIONDIALOG_H
#define TRANSITIONDIALOG_H


#include "bcdialog.h"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "playbackengine.h"
#include "plugindialog.inc"
#include "previewer.h"

class TransitionDialogName;
class TransitionDialogThread;
class TransitionDialog;
class TransitionPreviewer;



class TransitionDialogThread : public BC_DialogThread
{
public:
	TransitionDialogThread(MWindow *mwindow);
	BC_Window* new_gui();
	void handle_close_event(int result);
	void start(int data_type, 
        Edit *dst_edit); // set based on what's calling it

	char transition_title[BCTEXTLEN];
	MWindow *mwindow;
    Edit *dst_edit;
	int data_type;
	ArrayList<BC_ListBoxItem*> transition_names;
};



class TransitionDialog : public BC_Window
{
public:
	TransitionDialog(MWindow *mwindow, 
		TransitionDialogThread *thread,
		int x,
		int y);
    ~TransitionDialog();

	void create_objects();
	int resize_event(int w, int h);
    void compute_sizes(int w, int h);

	MWindow *mwindow;
	TransitionDialogThread *thread;
	TransitionDialogName *name_list;
	BC_Title *name_title;
    int preview_x, preview_center_y, preview_w;
    int list_x, list_y, list_w, list_h;
};

class TransitionDialogName : public BC_ListBox
{
public:
	TransitionDialogName(TransitionDialogThread *thread, 
		ArrayList<BC_ListBoxItem*> *standalone_data, 
		int x,
		int y, 
		int w, 
		int h);
	int handle_event();
	int selection_changed();
	TransitionDialogThread *thread;
};





class TransitionPreviewer : public Previewer
{
public:
    TransitionPreviewer();
    ~TransitionPreviewer();


    static TransitionPreviewer instance;

    void initialize();
// Sets up playback in the foreground, since there is no file format detection
    void submit_transition(const char *title, int data_type);
// create the widgets
    void create_preview();
    void handle_resize(int w, int h);
    int canvas_w, canvas_h;
};

#endif


