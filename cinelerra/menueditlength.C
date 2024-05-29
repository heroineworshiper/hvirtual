/*
 * CINELERRA
 * Copyright (C) 2010-2024 Adam Williams <broadcast at earthling dot net>
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

#include "editlength.h"
#include "edl.h"
#include "edlsession.h"
#include "guicast.h"
#include "language.h"
#include "menueditlength.h"
#include "mwindow.h"
#include "plugindialog.inc"
#include "preferences.h"
#include "swapasset.h"
#include "theme.h"

class EditAlignThread : public BC_DialogThread
{
public:
    EditAlignThread();
    ~EditAlignThread();
    void start();
    BC_Window* new_gui();
    void handle_close_event(int result);
};

class EditAlignDialog : public BC_Window
{
public:
    EditAlignDialog(int x,
	    int y);
    ~EditAlignDialog();
    void create_objects();
    int close_event();
};

class AlignCheckbox : public BC_CheckBox
{
public:
    AlignCheckbox(int x, 
        int y, 
        int *output, 
        const char *text);
    int handle_event();
    int *output;
};

EditAlignThread::EditAlignThread()
 : BC_DialogThread()
{
}

EditAlignThread::~EditAlignThread()
{
}

void EditAlignThread::start()
{
	BC_DialogThread::start();
}

BC_Window* EditAlignThread::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x() - DP(150);
	int y = display_info.get_abs_cursor_y() - DP(50);
	EditAlignDialog *gui = new EditAlignDialog(x, y);
	gui->create_objects();
	return gui;
}

void EditAlignThread::handle_close_event(int result)
{
    MWindow::instance->save_defaults();
	if(!result)
	{
        MWindow::instance->align_edits();
	}
}









EditAlignDialog::EditAlignDialog(int x, int y)
 : BC_Window(PROGRAM_NAME ": Align edits", 
	x,
	y,
	DP(350), 
	DP(150), 
	-1, 
	-1, 
	0,
	0, 
	1)
{
}

EditAlignDialog::~EditAlignDialog()
{
}

	
void EditAlignDialog::create_objects()
{
	lock_window("EditAlignDialog::create_objects");
    int margin = MWindow::theme->widget_border;
    int x = margin;
    int y = margin;
    AlignCheckbox *checkbox;
    add_subwindow(checkbox = new AlignCheckbox(x, 
        y, 
        &MWindow::preferences->align_deglitch,
        _("Delete glitch edits")));
    y += checkbox->get_h() + margin;

    add_subwindow(checkbox = new AlignCheckbox(x, 
        y, 
        &MWindow::preferences->align_synchronize,
        _("Synchronize source positions")));
    y += checkbox->get_h() + margin;

    add_subwindow(checkbox = new AlignCheckbox(x, 
        y, 
        &MWindow::preferences->align_extend,
        _("Extend edits to fill gaps")));
    y += checkbox->get_h() + margin;

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}

int EditAlignDialog::close_event()
{
	set_done(0);
	return 1;
}





AlignCheckbox::AlignCheckbox(int x, 
    int y, 
    int *output, 
    const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
}

int AlignCheckbox::handle_event()
{
	*output = get_value();
	return 0;
}











MenuEditLength::MenuEditLength(MWindow *mwindow)
 : BC_MenuItem(_("Edit Length..."))
{
	this->mwindow = mwindow;
	thread = new EditLengthThread(mwindow);
}



int MenuEditLength::handle_event()
{
	thread->start(0);
	return 1;
}



MenuEditShuffle::MenuEditShuffle(MWindow *mwindow)
 : BC_MenuItem(_("Shuffle Edits"))
{
	this->mwindow = mwindow;
}



int MenuEditShuffle::handle_event()
{
	mwindow->shuffle_edits();
	return 1;
}


MenuEditReverse::MenuEditReverse(MWindow *mwindow)
 : BC_MenuItem(_("Reverse Edits"))
{
	this->mwindow = mwindow;
}



int MenuEditReverse::handle_event()
{
	mwindow->reverse_edits();
	return 1;
}





MenuEditAlign::MenuEditAlign(MWindow *mwindow)
 : BC_MenuItem(_("Align Edits..."))
{
	this->mwindow = mwindow;
	thread = new EditAlignThread;
}



int MenuEditAlign::handle_event()
{
	thread->start();
//	mwindow->align_edits();
	return 1;
}





MenuSwapAsset::MenuSwapAsset(MWindow *mwindow)
 : BC_MenuItem(_("Swap assets..."))
{
	this->mwindow = mwindow;
	thread = new SwapAssetThread(mwindow);
}



int MenuSwapAsset::handle_event()
{
	thread->start();
	return 1;
}

