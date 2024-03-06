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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "transition.h"
#include "track.h"
#include "tracks.h"
#include "transitiondialog.h"
#include "transitionpopup.h"


TransitionLengthThread::TransitionLengthThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
}

TransitionLengthThread::~TransitionLengthThread()
{
}

void TransitionLengthThread::start(Transition *transition, 
	double length)
{
	this->transition = transition;
	this->length = this->orig_length = length;
	BC_DialogThread::start();
}

BC_Window* TransitionLengthThread::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x() - 150;
	int y = display_info.get_abs_cursor_y() - 50;
	TransitionLengthDialog *gui = new TransitionLengthDialog(mwindow, 
		this,
		x,
		y);
	gui->create_objects();
	return gui;
}

void TransitionLengthThread::handle_close_event(int result)
{
	if(!result)
	{
		if(transition)
		{
			mwindow->set_transition_length(transition, length);
		}
		else
		{
			mwindow->set_transition_length(length);
		}
	}
}









TransitionLengthDialog::TransitionLengthDialog(MWindow *mwindow, 
	TransitionLengthThread *thread,
	int x,
	int y)
 : BC_Window(PROGRAM_NAME ": Transition length", 
	x,
	y,
	DP(300), 
	DP(100), 
	-1, 
	-1, 
	0,
	0, 
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

TransitionLengthDialog::~TransitionLengthDialog()
{
}

	
void TransitionLengthDialog::create_objects()
{
	lock_window("TransitionLengthDialog::create_objects");
	add_subwindow(new BC_Title(DP(10), DP(10), _("Seconds:")));
	text = new TransitionLengthText(mwindow, this, DP(100), DP(10));
	text->create_objects();
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}

int TransitionLengthDialog::close_event()
{
	set_done(0);
	return 1;
}






TransitionLengthText::TransitionLengthText(MWindow *mwindow, 
	TransitionLengthDialog *gui,
	int x, 
	int y)
 : BC_TumbleTextBox(gui, 
 	(float)gui->thread->length,
	(float)0, 
	(float)100, 
	x,
	y,
	DP(100))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int TransitionLengthText::handle_event()
{
	double result = atof(get_text());
	if(!EQUIV(result, gui->thread->length))
	{
		gui->thread->length = result;
	}

	return 1;
}











TransitionPopup::TransitionPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

TransitionPopup::~TransitionPopup()
{
//	delete dialog_thread;
}


void TransitionPopup::create_objects()
{
	length_thread = new TransitionLengthThread(mwindow);
	add_item(attach = new TransitionPopupAttach(mwindow, this));
//	add_item(attach_default = new TransitionPopupDefault(mwindow));
	add_item(detach = new TransitionPopupDetach(mwindow, this));
    add_item(new BC_MenuItem("-"));
	add_item(show = new TransitionPopupShow(mwindow, this));
	add_item(on = new TransitionPopupOn(mwindow, this));
	add_item(copy = new TransitionCopy(mwindow));
	add_item(paste = new TransitionPaste(mwindow));
	add_item(length_item = new TransitionPopupLength(mwindow, this));
}

int TransitionPopup::update(Edit *edit, Transition *transition)
{
    this->edit = edit;
	this->transition = transition;
	this->length = transition->edit->track->from_units(transition->length);
	show->set_checked(transition->show);
	on->set_checked(transition->on);
    paste->transition = transition;
    copy->transition = transition;
	return 0;
}





TransitionPopupAttach::TransitionPopupAttach(MWindow *mwindow,
    TransitionPopup *popup)
 : BC_MenuItem(_("Change transition..."))
{
	this->mwindow = mwindow;
    this->popup = popup;
}

int TransitionPopupAttach::handle_event()
{
    mwindow->attach_transition->start(popup->edit->track->data_type, 
        popup->edit);
    return 0;
}



// TransitionPopupDefault::TransitionPopupDefault(MWindow *mwindow)
//  : BC_MenuItem(_("Attach default"))
// {
// 	this->mwindow = mwindow;
// }
// 
// int TransitionPopupDefault::handle_event()
// {
//     return 0;
// }





TransitionPaste::TransitionPaste(MWindow *mwindow)
 : BC_MenuItem(_("Paste settings"))
{
	this->mwindow = mwindow;
}

int TransitionPaste::handle_event()
{
    int64_t len = mwindow->gui->get_clipboard()->clipboard_len(BC_PRIMARY_SELECTION);
    char *string = new char[len + 1];
    mwindow->gui->get_clipboard()->from_clipboard(string, 
		len, 
		SECONDARY_SELECTION);
    FileXML file;
	file.read_from_string(string);
    int result = file.read_tag();

    if(!result && file.tag.title_is("TRANSITION"))
    {
        mwindow->undo->update_undo_before();
        transition->load_xml(&file);
    
        mwindow->save_backup();
        mwindow->undo->update_undo_after(_("paste settings"), LOAD_AUTOMATION);

        if(transition->track->data_type == TRACK_VIDEO)
            mwindow->restart_brender();
        mwindow->update_plugin_guis();
        mwindow->gui->update(0, 1, 0, 0, 0, 0, 0);
        mwindow->sync_parameters(CHANGE_ALL);
    }

    delete [] string;
	return 1;
}


TransitionCopy::TransitionCopy(MWindow *mwindow)
 : BC_MenuItem(_("Copy settings"))
{
	this->mwindow = mwindow;
}

int TransitionCopy::handle_event()
{
    FileXML file;
    transition->save_xml(&file);
    mwindow->gui->get_clipboard()->to_clipboard(file.string, 
		strlen(file.string), 
		ALL_SELECTIONS);
	return 1;
}








TransitionPopupDetach::TransitionPopupDetach(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Detach transition"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupDetach::~TransitionPopupDetach()
{
}

int TransitionPopupDetach::handle_event()
{
	mwindow->detach_transition(popup->transition);
	return 1;
}


TransitionPopupOn::TransitionPopupOn(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("On"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupOn::~TransitionPopupOn()
{
}

int TransitionPopupOn::handle_event()
{
	popup->transition->on = !get_checked();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}






TransitionPopupShow::TransitionPopupShow(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Show"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupShow::~TransitionPopupShow()
{
}

int TransitionPopupShow::handle_event()
{
	mwindow->show_plugin(popup->transition);
	return 1;
}








TransitionPopupLength::TransitionPopupLength(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Length"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupLength::~TransitionPopupLength()
{
}

int TransitionPopupLength::handle_event()
{
	popup->length_thread->start(popup->transition,
		popup->length);
	return 1;
}





