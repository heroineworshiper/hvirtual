/*
 * CINELERRA
 * Copyright (C) 2021 Adam Williams <broadcast at earthling dot net>
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
#include "assets.h"
#include "clip.h"
#include "edl.h"
#include "indexable.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "nestededls.h"
#include "swapasset.h"
#include "theme.h"






SwapAssetPath::SwapAssetPath(int x, 
    int y, 
    int w,
    int h,
    string *output,
    SwapAssetThread *thread,
    SwapAssetDialog *gui)
 : BC_PopupTextBox(gui,
    &thread->path_listitems,
    output->c_str(),
    x,
    y,
    w,
    h)
{
    this->thread = thread;
    this->gui = gui;
    this->output = output;
}

int SwapAssetPath::handle_event()
{
	int result = 0;
	if(get_textbox()->get_keypress() != RETURN)
	{
		result = get_textbox()->calculate_suggestions(&thread->path_listitems, 1);
	}
    output->assign(get_text());
	return result;
}







SwapAssetDialog::SwapAssetDialog(MWindow *mwindow, 
	SwapAssetThread *thread, 
	int x, 
	int y)
 : BC_Window(PROGRAM_NAME ": Swap Asset", 
 	x, 
	y, 
	mwindow->session->swap_asset_w, 
	mwindow->session->swap_asset_h,
	100, 
	100,
	1,
	0,
	1)
{
    this->mwindow = mwindow;
    this->thread = thread;
}

SwapAssetDialog::~SwapAssetDialog()
{
}


void SwapAssetDialog::create_objects()
{
    int margin = mwindow->theme->widget_border;
    int x = margin;
    int y = margin;
    int text_h = margin + BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1);
    BC_Title *title;
    x1 = 0;
    add_subwindow(title = new BC_Title(x, 
        y, 
        _("Replace all of 1 asset with another in the recordable tracks, in the highlighted range."),
        MEDIUMFONT,
        -1, // color
        0, // centered
        get_w() - x - margin,
        1));
    y += title->get_h() + margin;
    int y1 = y;
    add_subwindow(title = new BC_Title(x, y, _("Orig source:")));
    x1 = MAX(x1, x + title->get_w());
    y += text_h;
    add_subwindow(title = new BC_Title(x, y, _("New source:")));
    x1 = MAX(x1, x + title->get_w());
    y += text_h;

    y = y1;
    x1 += margin;

    old_path = new SwapAssetPath(x1, 
        y,
        get_w() - margin - x1 - BC_PopupTextBox::calculate_w(),
        DP(300),
        &thread->old_path,
        thread,
        this);
    old_path->create_objects();
    y += text_h;

    new_path = new SwapAssetPath(x1, 
        y,
        get_w() - margin - x1 - BC_PopupTextBox::calculate_w(),
        DP(300),
        &thread->new_path,
        thread,
        this);
    new_path->create_objects();
    y += text_h;

    add_subwindow(new BC_OKButton(this));
    add_subwindow(new BC_CancelButton(this));

	show_window();
}


int SwapAssetDialog::resize_event(int w, int h)
{
    int margin = mwindow->theme->widget_border;
    old_path->reposition_window(old_path->get_x(),
        old_path->get_y(),
        w - margin - x1 - BC_PopupTextBox::calculate_w());
    new_path->reposition_window(new_path->get_x(),
        new_path->get_y(),
        w - margin - x1 - BC_PopupTextBox::calculate_w());
    mwindow->session->swap_asset_w = w;
    mwindow->session->swap_asset_h = h;
	flush();
	return 1;
}





SwapAssetThread::SwapAssetThread(MWindow *mwindow)
 : BC_DialogThread()
{
    this->mwindow = mwindow;
}

SwapAssetThread::~SwapAssetThread()
{
}


void SwapAssetThread::start()
{
// construct the path substitution list
// TODO: merge with editinfo
    path_listitems.remove_all_objects();
    EDL *edl = mwindow->edl;
    if(edl)
    {
        for(Asset *current = edl->assets->first; 
	        current; 
	        current = NEXT)
        {
            path_listitems.append(new BC_ListBoxItem(current->path));
        }

	    for(int i = 0; i < edl->nested_edls->size(); i++)
	    {
		    Indexable *indexable = edl->nested_edls->get(i);
            path_listitems.append(new BC_ListBoxItem(indexable->path));
        }

        path_listitems.append(new BC_ListBoxItem(SILENCE_TEXT));

// crummy sort
	    int done = 0;
	    while(!done)
	    {
		    done = 1;
		    for(int i = 0; i < path_listitems.size() - 1; i++)
		    {
			    BC_ListBoxItem *item1 = path_listitems.get(i);
			    BC_ListBoxItem *item2 = path_listitems.get(i + 1);
			    if(strcmp(item1->get_text(), item2->get_text()) > 0)
			    {
				    path_listitems.set(i + 1, item1);
				    path_listitems.set(i, item2);
				    done = 0;
			    }
		    }
	    }
        
        
        old_path.assign(path_listitems.get(0)->get_text());
        new_path.assign(path_listitems.get(0)->get_text());
    }
    

    mwindow->gui->unlock_window();
    BC_DialogThread::start();
    mwindow->gui->lock_window("SwapAssetThread::start");
}

BC_Window* SwapAssetThread::new_gui()
{
	mwindow->gui->lock_window("SwapAssetThread::new_gui");
	int x = mwindow->gui->get_abs_cursor_x(0) - 
		mwindow->session->swap_asset_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) - 
		mwindow->session->swap_asset_h / 2;
    SwapAssetDialog *gui = new SwapAssetDialog(mwindow, this, x, y);
    gui->create_objects();
	mwindow->gui->unlock_window();
    return gui;
}

void SwapAssetThread::handle_close_event(int result)
{
    if(!result)
    {
        if(old_path.compare(new_path))
        {
            int old_is_silence = 0;
            int new_is_silence = 0;
            if(!old_path.compare(SILENCE_TEXT))
            {
                old_is_silence = 1;
            }
            if(!new_path.compare(SILENCE_TEXT))
            {
                new_is_silence = 1;
            }

            mwindow->swap_asset(&old_path, 
                &new_path, 
                old_is_silence,
                new_is_silence);
        }
    }
}



