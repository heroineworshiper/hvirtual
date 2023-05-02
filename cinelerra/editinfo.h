/*
 * CINELERRA
 * Copyright (C) 2008-2021 Adam Williams <broadcast at earthling dot net>
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

// Edit fields of an edit


#ifndef EDITINFO_H
#define EDITINFO_H


#include "bcdialog.h"
#include "editinfo.inc"
#include "edit.inc"
#include "mwindow.inc"


class EditInfoThread : public BC_DialogThread
{
public:
    EditInfoThread(MWindow *mwindow);
    ~EditInfoThread();

    void show_edit(Edit *edit);
    BC_Window* new_gui();
    int text_to_format(char *text);
    char* format_to_text(int format);
    void handle_close_event(int result);

// data specific conversions
    double from_units(int64_t x);
    int64_t to_units(double x);


    ArrayList <BC_ListBoxItem*> path_listitems;
    MWindow *mwindow;

// copied over from the edit so the edit can be deleted
    int edit_id;
    string path;
    int64_t startsource;
    int64_t startproject;
    int64_t length;
    int channel;
    int is_silence;

    string orig_path;
    int64_t orig_startsource;
    int64_t orig_startproject;
    int64_t orig_length;
    int orig_channel;

    int data_type;
    double rate;
};


class EditInfoFormat : public BC_PopupMenu
{
public:
    EditInfoFormat(MWindow *mwindow, 
        EditInfoGUI *gui, 
        EditInfoThread *thread,
        int x,
        int y,
        int w);
    ~EditInfoFormat();
    
    int handle_event();
    
    MWindow *mwindow;
    EditInfoGUI *gui;
    EditInfoThread *thread;
};

class EditInfoPath : public BC_PopupTextBox
{
public:
    EditInfoPath(int x, 
        int y, 
        int w,
        int h, 
        EditInfoThread *thread,
        EditInfoGUI *gui);
    int handle_event();
    EditInfoThread *thread;
    EditInfoGUI *gui;
};

class EditInfoNumber : public BC_TextBox
{
public:
    EditInfoNumber(EditInfoGUI *gui, 
        int x, 
        int y, 
        int w, 
        int64_t *output);
    
    int handle_event();
// handle scrollwheel
	int button_press_event();
    void increase();
    void decrease();
    int64_t *output;
    EditInfoGUI *gui;
};

class EditInfoTumbler : public BC_Tumbler
{
public:
    EditInfoTumbler(EditInfoGUI *gui, 
        int x, 
        int y, 
        EditInfoNumber *text);
    
    int handle_up_event();
    int handle_down_event();
    EditInfoNumber *text;
    EditInfoGUI *gui;
};

class EditInfoChannel : public BC_TumbleTextBox
{
public:
    EditInfoChannel(EditInfoGUI *gui,
        int x, 
        int y, 
        int w,
        int *output);
    int handle_event();
    int *output;
    EditInfoGUI *gui;
};

class EditInfoGUI : public BC_Window
{
public:
    EditInfoGUI(MWindow *mwindow, 
        EditInfoThread *thread, 
        int x, 
        int y);
    ~EditInfoGUI();

    void create_objects();
    char* format_to_text(int format);
    void update();
    int resize_event(int w, int h);


    MWindow *mwindow;
    EditInfoThread *thread;
    EditInfoPath *path;
    EditInfoNumber *startsource;
    EditInfoTumbler *startsource2;
    EditInfoNumber *startproject;
    EditInfoTumbler *startproject2;
    EditInfoNumber *length;
    EditInfoTumbler *length2;
    EditInfoChannel *channel;
    EditInfoFormat *format;
    int x1;
};


#endif


