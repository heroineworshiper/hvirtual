
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

#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainsession.h"
#include "menuattachtransition.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindialog.h"
#include "pluginserver.h"



#include <string.h>



MenuAttachTransition::MenuAttachTransition(MWindow *mwindow, int data_type)
 : BC_MenuItem(_("Attach Transition..."))
{
	this->mwindow = mwindow;
	this->data_type = data_type;
	thread = new TransitionDialogThread(mwindow, data_type);
}

int MenuAttachTransition::handle_event()
{
	thread->start();
	return 1;
}


TransitionDialogThread::TransitionDialogThread(MWindow *mwindow, int data_type)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->data_type = data_type;
}

void TransitionDialogThread::start()
{
	if(!transition_names.total)
	{
// Construct listbox names	
		ArrayList<PluginServer*> plugindb;
		mwindow->search_plugindb(data_type == TRACK_AUDIO, 
			data_type == TRACK_VIDEO, 
			0, 
			1,
			0,
			plugindb);
		for(int i = 0; i < plugindb.total; i++)
			transition_names.append(new BC_ListBoxItem(_(plugindb.values[i]->title)));
	}

	if(data_type == TRACK_AUDIO)
		strcpy(transition_title, mwindow->edl->session->default_atransition);
	else
		strcpy(transition_title, mwindow->edl->session->default_vtransition);

	mwindow->gui->unlock_window();
	BC_DialogThread::start();
	mwindow->gui->lock_window("TransitionDialogThread::start");
}



BC_Window* TransitionDialogThread::new_gui()
{
	mwindow->gui->lock_window("TransitionDialogThread::new_gui");
	int x = mwindow->gui->get_abs_cursor_x(0) -
		mwindow->session->transitiondialog_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) -
		mwindow->session->transitiondialog_h / 2;
	TransitionDialog *window = new TransitionDialog(mwindow, 
		this,
		x, 
		y);
	window->create_objects();
	mwindow->gui->unlock_window();
	return window;
}

void TransitionDialogThread::handle_close_event(int result)
{
	if(!result)
	{
		mwindow->paste_transitions(data_type, transition_title);
	}
}



TransitionDialog::TransitionDialog(MWindow *mwindow, 
	TransitionDialogThread *thread,
	int x,
	int y)
 : BC_Window(_("Attach Transition"), 
 	x,
	y,
	mwindow->session->transitiondialog_w,
	mwindow->session->transitiondialog_h, 
	320, 
	240,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void TransitionDialog::create_objects()
{
	int x = 10;
	int y = 10;
	
	lock_window("TransitionDialog::create_objects");
	add_subwindow(name_title = new BC_Title(x, y, _("Select transition from list")));
	y += name_title->get_h() + 5;
	add_subwindow(name_list = new TransitionDialogName(thread, 
		&thread->transition_names, 
		x, 
		y,
		get_w() - x - x,
		get_h() - y - BC_OKButton::calculate_h() - 10));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	flush();
	unlock_window();
}


int TransitionDialog::resize_event(int w, int h)
{
	int x = 10;
	int y = 10;

	name_title->reposition_window(x, y);
	y += name_title->get_h() + 5;
	name_list->reposition_window(x, 
		y,
		w - x - x,
		h - y - BC_OKButton::calculate_h() - 10);

	return 1;
}



TransitionDialogName::TransitionDialogName(TransitionDialogThread *thread, 
	ArrayList<BC_ListBoxItem*> *standalone_data, 
	int x,
	int y, 
	int w, 
	int h)
 : BC_ListBox(x, 
 	y, 
	w, 
	h, 
	LISTBOX_TEXT,
	standalone_data)
{
	this->thread = thread;
}

int TransitionDialogName::handle_event()
{
	set_done(0);
	return 1;
}

int TransitionDialogName::selection_changed()
{
	int number = get_selection_number(0, 0);
	strcpy(thread->transition_title, 
		thread->transition_names.values[number]->get_text());
	return 1;
}




