
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

#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "edl.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vwindow.h"
#include "vwindowgui.h"




ClipEdit::ClipEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->awindow = awindow;
	this->vwindow = vwindow;
	this->clip = 0;
	this->create_it = 0;
}

ClipEdit::~ClipEdit()
{
}

// After the window is closed and deleted, this is called.
void ClipEdit::handle_close_event(int result)
{
	if(!result)
	{
		EDL *new_edl = 0;
// Add to EDL
		if(create_it)
			new_edl = mwindow->edl->add_clip(clip);

// Copy clip to existing clip in EDL
		if(!create_it)
			original->copy_session(clip);


//			mwindow->vwindow->gui->update_sources(mwindow->vwindow->gui->source->get_text());


		mwindow->awindow->gui->lock_window();
		mwindow->awindow->gui->update_assets();
		mwindow->awindow->gui->flush();
		mwindow->awindow->gui->unlock_window();

// Change VWindow to it if vwindow was called
// But this doesn't let you easily create a lot of clips.
		if(vwindow && create_it)
		{
//				vwindow->change_source(new_edl);
		}
	}
	else
	{
		mwindow->session->clip_number--;
	}



// For creating new clips, the original was copied in add_clip.
// For editing old clips, the original was transferred to another variable.
	if(!create_it) clip->remove_user();
	original = 0;
	clip = 0;
	create_it = 0;
}


// User creates the window and initializes it here.
BC_Window* ClipEdit::new_gui()
{
	original = clip;

	if(!create_it)
	{
		this->clip = new EDL(mwindow->edl);
		clip->create_objects();
		clip->copy_all(original);
	}


	window = new ClipEditWindow(mwindow, this);
	window->create_objects();
	return window;
}



void ClipEdit::edit_clip(EDL *clip)
{
// Allow more than one window so we don't have to delete the clip in handle_event
	if(!this->clip)
	{
		this->clip = clip;
		this->create_it = 0;
		start();
	}
}

void ClipEdit::create_clip(EDL *clip)
{
// Allow more than one window so we don't have to delete the clip in handle_event
	if(!this->clip)
	{
		this->clip = clip;
		this->create_it = 1;
		start();
	}
}








ClipEditWindow::ClipEditWindow(MWindow *mwindow, ClipEdit *thread)
 : BC_Window(PROGRAM_NAME ": Clip Info", 
 	mwindow->gui->get_abs_cursor_x(1) - 400 / 2,
	mwindow->gui->get_abs_cursor_y(1) - 350 / 2,
	400, 
	350,
	400,
	430,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

ClipEditWindow::~ClipEditWindow()
{
}

	
void ClipEditWindow::create_objects()
{
	lock_window("ClipEditWindow::create_objects");
	this->create_it = thread->create_it;

	int x = 10, y = 10;
	int x1 = x;
	BC_TextBox *textbox;
	BC_TextBox *titlebox;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x1, y, _("Title:")));
	y += title->get_h() + 5;
	add_subwindow(titlebox = new ClipEditTitle(this, x1, y, get_w() - x1 * 2));
	y += titlebox->get_h() + 10;
	add_subwindow(title = new BC_Title(x1, y, _("Comments:")));
	y += title->get_h() + 5;
	add_subwindow(textbox = new ClipEditComments(this, 
		x1, 
		y, 
		get_w() - x1 * 2, 
		BC_TextBox::pixels_to_rows(this, 
			MEDIUMFONT, 
			get_h() - 10 - BC_OKButton::calculate_h() - y)));



	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	titlebox->activate();
	unlock_window();
}





ClipEditTitle::ClipEditTitle(ClipEditWindow *window, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, window->thread->clip->local_session->clip_title)
{
	this->window = window;
}

int ClipEditTitle::handle_event()
{
	strcpy(window->thread->clip->local_session->clip_title, get_text());
	return 1;
}





ClipEditComments::ClipEditComments(ClipEditWindow *window, int x, int y, int w, int rows)
 : BC_TextBox(x, y, w, rows, window->thread->clip->local_session->clip_notes)
{
	this->window = window;
}

int ClipEditComments::handle_event()
{
	strcpy(window->thread->clip->local_session->clip_notes, get_text());
	return 1;
}
