/*
 * CINELERRA
 * Copyright (C) 1997-2025 Adam Williams <broadcast at earthling dot net>
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


#ifndef EDITKEYFRAME_H
#define EDITKEYFRAME_H

#include "auto.inc"
#include "autos.inc"
#include "bcdialog.h"
#include "floatauto.inc"
#include "mwindow.inc"

class EditKeyframeDialog;

class EditKeyframeThread : public BC_DialogThread
{
public:
	EditKeyframeThread(MWindow *mwindow);
	BC_Window* new_gui();
	void handle_close_event(int result);
	void start(Auto *auto_);
    void apply(EditKeyframeDialog *gui,
        float *changed_value);
    void apply_common();

	MWindow *mwindow;
    Autos *autos;
// pointer to the EDL keyframe
    FloatAuto *auto_;
// copy being edited
    FloatAuto *auto_copy;
// before & after values
    FloatAuto *before;
    FloatAuto *after;
};



class EditKeyframeText : public BC_TumbleTextBox
{
public:
	EditKeyframeText(MWindow *mwindow, 
        EditKeyframeDialog *gui, 
        int x, 
        int y,
        float *value);
	int handle_event();
	MWindow *mwindow;
	EditKeyframeDialog *gui;
    float *value;
};


class EditKeyframeMode : public BC_PopupMenu
{
public:
	EditKeyframeMode(MWindow *mwindow, 
        EditKeyframeDialog *gui, 
        int x, 
        int y, 
        const char *text);
	void create_objects();
	int handle_event();
	static char* mode_to_text(int mode);
	static int text_to_mode(char *text);

	MWindow *mwindow;
	EditKeyframeDialog *gui;
};

class EditKeyframeDialog : public BC_Window
{
public:
	EditKeyframeDialog(MWindow *mwindow, 
		EditKeyframeThread *thread,
		int x,
		int y);
    ~EditKeyframeDialog();

	void create_objects();
    void lock_texts();


	MWindow *mwindow;
	EditKeyframeThread *thread;
    EditKeyframeText *in;
    EditKeyframeText *out;
    EditKeyframeText *value;
    EditKeyframeMode *mode;
    
};


#endif

