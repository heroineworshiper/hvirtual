
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


#include "bcsignals.h"
#include "language.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "statusbar.h"
#include "theme.h"
#include "vframe.h"



StatusBar::StatusBar(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mstatus_x, 
 	mwindow->theme->mstatus_y, 
	mwindow->theme->mstatus_w, 
	mwindow->theme->mstatus_h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

StatusBar::~StatusBar()
{
}



void StatusBar::create_objects()
{
//printf("StatusBar::create_objects 1\n");
	int margin = mwindow->theme->widget_border;
	int x = margin, y = margin;
//printf("StatusBar::create_objects 1\n");
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	add_subwindow(status_text = new BC_Title(
		mwindow->theme->mstatus_message_x, 
		mwindow->theme->mstatus_message_y, 
		"",
		MEDIUMFONT,
		mwindow->theme->message_normal,
		0,
		mwindow->theme->mstatus_progress_x - 
			margin - 
			mwindow->theme->mstatus_message_x));
	x = mwindow->theme->mstatus_progress_x - margin;
// printf("StatusBar::create_objects %d: 0x%08x\n", 
// __LINE__, mwindow->theme->message_normal);
	add_subwindow(main_progress = 
		new BC_ProgressBar(mwindow->theme->mstatus_progress_x, 
			mwindow->theme->mstatus_progress_y, 
			mwindow->theme->mstatus_progress_w, 
			mwindow->theme->mstatus_progress_w));
	x += main_progress->get_w() + margin;
//printf("StatusBar::create_objects 1\n");
	add_subwindow(main_progress_cancel = 
		new StatusBarCancel(mwindow, 
			mwindow->theme->mstatus_cancel_x, 
			mwindow->theme->mstatus_cancel_y));
//printf("StatusBar::create_objects 1\n");
	default_message();
	flash();
}

void StatusBar::resize_event()
{
	int margin = mwindow->theme->widget_border;
	int x = margin, y = 1;


	reposition_window(mwindow->theme->mstatus_x,
		mwindow->theme->mstatus_y,
		mwindow->theme->mstatus_w,
		mwindow->theme->mstatus_h);

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());


	status_text->reposition(mwindow->theme->mstatus_message_x, 
		mwindow->theme->mstatus_message_y,
		mwindow->theme->mstatus_progress_x - 
			margin - 
			mwindow->theme->mstatus_message_x);

	x = mwindow->theme->mstatus_progress_x - margin;
	main_progress->reposition_window(mwindow->theme->mstatus_progress_x, 
		mwindow->theme->mstatus_progress_y);

	x += main_progress->get_w() + margin;
	main_progress_cancel->reposition_window(mwindow->theme->mstatus_cancel_x, 
		mwindow->theme->mstatus_cancel_y);

	flash(0);
}

void StatusBar::set_message(char *text)
{
	status_text->update(text);
}

void StatusBar::default_message()
{
	status_text->set_color(mwindow->theme->message_normal);
	status_text->update(_("Welcome to Cinelerra."));
}


StatusBarCancel::StatusBarCancel(MWindow *mwindow, int x, int y)
 : BC_Button(x, y, mwindow->theme->statusbar_cancel_data)
{
	this->mwindow = mwindow;
	set_tooltip(_("Cancel operation"));
}
int StatusBarCancel::handle_event()
{
	mwindow->mainprogress->cancelled = 1;
	return 1;
}
