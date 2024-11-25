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

#include "asset.h"
#include "clip.h"
#include "language.h"
#include "mwindow.h"
#include "record.h"
#include "recordmonitor.h"
#include "screencapedit.h"
#include "theme.h"


ScreenCapEdit::ScreenCapEdit(RecordMonitorGUI *gui, int x, int y)
 : BC_Button(x, 
 	y, 
	MWindow::theme->get_image_set("wrench"))
{
    this->gui = gui;
	thread = new ScreenCapEditThread(gui->thread);
	set_tooltip(_("Screencap options"));
}


ScreenCapEdit::~ScreenCapEdit()
{
    delete thread;
}

int ScreenCapEdit::handle_event()
{
	unlock_window();
	thread->start();
	lock_window("ScreenCapEdit::handle_event");
	return 1;
}








ScreenCapEditThread::ScreenCapEditThread(RecordMonitor *record_monitor)
 : BC_DialogThread()
{
    this->record_monitor = record_monitor;
}

ScreenCapEditThread::~ScreenCapEditThread()
{
}

void ScreenCapEditThread::handle_done_event(int result)
{
}

BC_Window* ScreenCapEditThread::new_gui()
{
    gui = new ScreenCapEditGUI(record_monitor);
    gui->create_objects();
    return gui;
}




ScreenCapEditGUI::ScreenCapEditGUI(RecordMonitor *record_monitor)
 : BC_Window(PROGRAM_NAME ": Screencap", 
 	record_monitor->window->get_abs_cursor_x(1), 
	record_monitor->window->get_abs_cursor_y(1), 
 	DP(320), 
	DP(240), 
	DP(320), 
	DP(240))
{
    this->record_monitor = record_monitor;
}


ScreenCapEditGUI::~ScreenCapEditGUI()
{
}

void ScreenCapEditGUI::create_objects()
{
    int margin = BC_Resources::theme->widget_border;
	int x = margin, y = margin, i;
    add_subwindow(cursor_toggle = new BC_CheckBox(x, 
        y, 
        &record_monitor->record->do_cursor,
        _("Record cursor")));
    y += cursor_toggle->get_h() + margin;
    add_subwindow(big_cursor_toggle = new BC_CheckBox(x, 
        y, 
        &record_monitor->record->do_big_cursor,
        _("Big cursor")));
    y += big_cursor_toggle->get_h() + margin;
    add_subwindow(keypresses = new BC_CheckBox(x, 
        y, 
        &record_monitor->record->do_keypresses,
        _("Draw keypresses")));
    int x1 = x;
    y += keypresses->get_h() + margin;
    BC_Title *title;
    add_subwindow(title = new BC_Title(x, y, _("Text size:")));
    x += title->get_w() + margin;
    keypress_size = new ScreenCapSize(this, x, y, DP(100));
    keypress_size->create_objects();
    x = x1;
    y += keypress_size->get_h() + margin;
    int y1 = y;
    x1 = x;
    add_subwindow(top_left = new ScreenCapTranslation(this, 
        x, 
        y, 
        TOP_LEFT,
        "Top left"));
    y += top_left->get_h() + margin;
    add_subwindow(bottom_left = new ScreenCapTranslation(this, 
        x, 
        y, 
        BOTTOM_LEFT,
        "Bottom left"));
    x += MAX(top_left->get_w(), bottom_left->get_w()) + margin;
    y = y1;
    add_subwindow(top_right = new ScreenCapTranslation(this, 
        x, 
        y, 
        TOP_RIGHT,
        "Top right"));
    y += top_right->get_h() + margin;
    add_subwindow(bottom_right = new ScreenCapTranslation(this, 
        x, 
        y, 
        BOTTOM_RIGHT,
        "Bottom right"));
    x = x1;
    y += bottom_right->get_h() + margin;

	add_subwindow(new BC_OKButton(this));
    show_window(1);
}



ScreenCapSize::ScreenCapSize(ScreenCapEditGUI *gui, int x, int y, int w)
 : BC_TumbleTextBox(gui,
    (int64_t)gui->record_monitor->record->keypress_size,
    (int64_t)8,
    (int64_t)256,
    x,
    y,
    w)
{
    this->gui = gui;
}

int ScreenCapSize::handle_event()
{
    Record *record = gui->record_monitor->record;
    record->keypress_size = atoi(get_text());
    return 1;
}



ScreenCapTranslation::ScreenCapTranslation(ScreenCapEditGUI *gui, 
    int x,
    int y,
    int value, 
    const char *text)
 : BC_GenericButton(x, y, _(text))
{
    this->gui = gui;
    this->value = value;
}

int ScreenCapTranslation::handle_event()
{
    Record *record = gui->record_monitor->record;
    int root_w = gui->get_root_w(1);
    int root_h = gui->get_root_h(0);
    int asset_w = record->default_asset->width;
    int asset_h = record->default_asset->height;
    switch(value)
    {
        case TOP_LEFT:
	        record->set_translation(0, 0);
            break;
        case TOP_RIGHT:
	        record->set_translation(root_w - asset_w, 
                0);
            break;
        case BOTTOM_LEFT:
	        record->set_translation(0, 
                root_h - asset_h);
            break;
        case BOTTOM_RIGHT:
	        record->set_translation(root_w - asset_w, 
                root_h - asset_h);
            break;
    }
    return 1;
}









