/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef SCREENCAPEDIT_H
#define SCREENCAPEDIT_H

#include "bcdialog.h"
#include "recordmonitor.inc"
#include "screencapedit.inc"


class ScreenCapEdit : public BC_Button
{
public:
    ScreenCapEdit(RecordMonitorGUI *gui, int x, int y);
    ~ScreenCapEdit();
	int handle_event();
    RecordMonitorGUI *gui;
    ScreenCapEditThread *thread;
};

class ScreenCapEditThread : public BC_DialogThread
{
public:
    ScreenCapEditThread(RecordMonitor *record_monitor);
    ~ScreenCapEditThread();
    void handle_done_event(int result);
    BC_Window* new_gui();
    RecordMonitor *record_monitor;
    ScreenCapEditGUI *gui;
};


class ScreenCapTranslation : public BC_GenericButton
{
public:
	ScreenCapTranslation(ScreenCapEditGUI *gui, 
        int x,
        int y,
        int value, 
        const char *text);
	int handle_event();
	ScreenCapEditGUI *gui;
    int value;
};

class ScreenCapSize : public BC_TumbleTextBox
{
public:
	ScreenCapSize(ScreenCapEditGUI *gui, int x, int y, int w);
	int handle_event();
	ScreenCapEditGUI *gui;
};

class ScreenCapEditGUI : public BC_Window
{
public:
    ScreenCapEditGUI(RecordMonitor *record_monitor);
    ~ScreenCapEditGUI();
    void create_objects();
    BC_CheckBox *cursor_toggle;
    BC_CheckBox *big_cursor_toggle;
    BC_CheckBox *keypresses;
    ScreenCapSize *keypress_size;
    ScreenCapTranslation *top_left;
    ScreenCapTranslation *bottom_left;
    ScreenCapTranslation *bottom_right;
    ScreenCapTranslation *top_right;

    RecordMonitor *record_monitor;
};



#endif
