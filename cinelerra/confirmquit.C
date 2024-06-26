
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

#include "confirmquit.h"
#include "keys.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"




ConfirmQuitWindow::ConfirmQuitWindow(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Confirm Quit", 
 	mwindow->gui->get_abs_cursor_x(1), 
	mwindow->gui->get_abs_cursor_y(1), 
	DP(375), 
	DP(160))
{
	this->mwindow = mwindow;
}

ConfirmQuitWindow::~ConfirmQuitWindow()
{
}

void ConfirmQuitWindow::create_objects(char *string)
{
    int margin = MWindow::theme->widget_border;
	int x = margin, y = margin;
	BC_Title *title;

	lock_window("ConfirmQuitWindow::create_objects");
	add_subwindow(title = new BC_Title(x, y, string));
	y += title->get_h();
	add_subwindow(title = new BC_Title(x, y, _("( Answering ""No"" will destroy changes )")));

	add_subwindow(new ConfirmQuitYesButton(mwindow, this));
	add_subwindow(new ConfirmQuitNoButton(mwindow, this));
	add_subwindow(new ConfirmQuitCancelButton(mwindow, this));
	show_window(1);
	unlock_window();
}

ConfirmQuitYesButton::ConfirmQuitYesButton(MWindow *mwindow, 
	ConfirmQuitWindow *gui)
 : BC_GenericButton(DP(10), 
 	gui->get_h() - BC_GenericButton::calculate_h() - 10, 
	_("Yes"))
{
	set_underline(0);
}

int ConfirmQuitYesButton::handle_event()
{
	set_done(2);
	return 1;
}

int ConfirmQuitYesButton::keypress_event()
{;
	if(get_keypress() == 'y') return handle_event();
	return 0;
}

ConfirmQuitNoButton::ConfirmQuitNoButton(MWindow *mwindow, 
	ConfirmQuitWindow *gui)
 : BC_GenericButton(gui->get_w() / 2 - BC_GenericButton::calculate_w(gui, _("No")) / 2, 
 	gui->get_h() - BC_GenericButton::calculate_h() - 10, 
	_("No"))
{
	set_underline(0);
}

int ConfirmQuitNoButton::handle_event()
{
	set_done(0);
	return 1;
}

int ConfirmQuitNoButton::keypress_event()
{
	if(get_keypress() == 'n') return handle_event(); 
	return 0;
}

ConfirmQuitCancelButton::ConfirmQuitCancelButton(MWindow *mwindow, 
	ConfirmQuitWindow *gui)
 : BC_GenericButton(gui->get_w() - BC_GenericButton::calculate_w(gui, _("Cancel")) - DP(10), 
 	gui->get_h() - BC_GenericButton::calculate_h() - 10, 
	_("Cancel"))
{
}

int ConfirmQuitCancelButton::handle_event()
{
	set_done(1);
	return 1;
}

int ConfirmQuitCancelButton::keypress_event()
{
	if(get_keypress() == ESC) return handle_event();
	return 0;
}

