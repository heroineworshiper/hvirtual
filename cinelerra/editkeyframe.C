/*
 * CINELERRA
 * Copyright (C) 1997-2024 Adam Williams <broadcast at earthling dot net>
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

#include "auto.h"
#include "clip.h"
#include "editkeyframe.h"
#include "edl.h"
#include "floatauto.h"
#include "keys.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "tracks.h"

#include <string.h>

EditKeyframeThread::EditKeyframeThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
    auto_copy = new FloatAuto;
}

void EditKeyframeThread::start(Auto *auto_)
{
    if(!is_running())
    {
        this->auto_ = (FloatAuto*)auto_;
        
        auto_copy->copy_from(auto_);
        auto_copy->is_default = auto_->is_default;

//printf("EditKeyframeThread::start %d %d\n", __LINE__, auto_->is_default);
        mwindow->gui->unlock_window();
	    BC_DialogThread::start();
        mwindow->gui->lock_window("EditKeyframeThread::start");
    }
}



BC_Window* EditKeyframeThread::new_gui()
{
	mwindow->gui->lock_window("EditKeyframeThread::new_gui");
	int x = mwindow->gui->get_abs_cursor_x(0);
	int y = mwindow->gui->get_abs_cursor_y(0);
	EditKeyframeDialog *window = new EditKeyframeDialog(mwindow, 
		this,
		x, 
		y);
	window->create_objects();
	mwindow->gui->unlock_window();
	return window;
}

void EditKeyframeThread::handle_close_event(int result)
{
}

// copy from the temporary to the EDL keyframe with undo code
void EditKeyframeThread::apply(EditKeyframeDialog *gui,
    float *changed_value)
{
//printf("EditKeyframeThread::apply %d %p\n", __LINE__, auto_);
// convert it to locked bezier
    if(auto_copy->mode == Auto::BEZIER_LOCKED &&
        auto_->mode != Auto::BEZIER_LOCKED)
    {
        auto_copy->to_locked();
        gui->in->update(auto_copy->control_in_value);
        gui->out->update(auto_copy->control_out_value);
    }
    else
// mirror the changed control point
    if(changed_value &&
        auto_copy->mode == Auto::BEZIER_LOCKED)
    {
        if(changed_value == &auto_copy->control_in_value)
        {
            auto_copy->control_out_value = -auto_copy->control_in_value;
            gui->out->update(auto_copy->control_out_value);
        }
        else
        if(changed_value == &auto_copy->control_out_value)
        {
            auto_copy->control_in_value = -auto_copy->control_out_value;
            gui->in->update(auto_copy->control_in_value);
        }
    }


    mwindow->gui->lock_window("EditKeyframeThread::apply");

    if(mwindow->edl->tracks->keyframe_exists(auto_))
    {
        mwindow->undo->update_undo_before();

        auto_->copy_from(auto_copy);


        mwindow->undo->update_undo_after(_("edit keyframe"), LOAD_AUTOMATION);

	    mwindow->restart_brender();
        mwindow->gui->draw_overlays(1, 1);
        mwindow->sync_parameters(CHANGE_PARAMS);
    }
    else
    {
        printf("EditKeyframeThread::apply %d: keyframe was deleted\n", __LINE__);
    }
    mwindow->gui->unlock_window();
}


EditKeyframeDialog::EditKeyframeDialog(MWindow *mwindow, 
	EditKeyframeThread *thread,
	int x,
	int y)
 : BC_Window(_("Edit Keyframe"), 
 	x,
	y,
	DP(400),
	DP(160), 
	DP(400), 
	DP(160),
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

EditKeyframeDialog::~EditKeyframeDialog()
{

}


void EditKeyframeDialog::create_objects()
{
	BC_Resources *resources = BC_WindowBase::get_resources();
    int widget_border = MWindow::theme->widget_border;
    int window_border = MWindow::theme->window_border;
	int x = window_border;
	int y = window_border;

    BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("In control:")));
	int y1 = y + title->get_h() + widget_border;
	in = new EditKeyframeText(mwindow, 
        this, 
        x, 
        y1,
        &thread->auto_copy->control_in_value);
    in->set_precision(3);
    in->set_increment(1);
    in->create_objects();
    x += MAX(title->get_w(), in->get_w()) + widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Value:")));
	value = new EditKeyframeText(mwindow, 
        this, 
        x, 
        y1,
        &thread->auto_copy->value);
    value->set_precision(3);
    value->set_increment(1);
    value->create_objects();
    x += MAX(title->get_w(), value->get_w()) + widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Out control:")));
	out = new EditKeyframeText(mwindow, 
        this, 
        x, 
        y1,
        &thread->auto_copy->control_out_value);
    out->set_precision(3);
    out->set_increment(1);
    out->create_objects();
    x = window_border;
    y = y1 + out->get_h() + widget_border;
    
 	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
    x += title->get_w() + widget_border;
	add_subwindow(mode = new EditKeyframeMode(mwindow, 
		this, 
		x, 
		y,
		EditKeyframeMode::mode_to_text(thread->auto_copy->mode)));
	mode->create_objects();

   
    BC_OKButton *ok;
	add_subwindow(ok = new BC_OKButton(this));
    ok->set_esc(1);
	show_window();
}



EditKeyframeText::EditKeyframeText(MWindow *mwindow, 
    EditKeyframeDialog *gui, 
    int x, 
    int y,
    float *value)
 : BC_TumbleTextBox(gui, 
	*value,
	(float)-65536,
	(float)65536,
	x, 
	y, 
	DP(100))
{
	this->mwindow = mwindow;
	this->gui = gui;
    this->value = value;
}
int EditKeyframeText::handle_event()
{
    *value = atof(get_text());
    gui->thread->apply(gui, value);
    return 1;
}



EditKeyframeMode::EditKeyframeMode(MWindow *mwindow, 
    EditKeyframeDialog *gui, 
    int x, 
    int y, 
    const char *text)
 : BC_PopupMenu(x,
 	y,
	DP(200),
	text,
	1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
void EditKeyframeMode::create_objects()
{
	add_item(new BC_MenuItem(mode_to_text(Auto::LINEAR)));
	add_item(new BC_MenuItem(mode_to_text(Auto::BEZIER_UNLOCKED)));
	add_item(new BC_MenuItem(mode_to_text(Auto::BEZIER_LOCKED)));
}
int EditKeyframeMode::handle_event()
{
    gui->thread->auto_copy->mode = text_to_mode(get_text());
    gui->thread->apply(gui, 0);
    return 1;
}

char* EditKeyframeMode::mode_to_text(int mode)
{
    switch(mode)
    {
        case Auto::BEZIER_UNLOCKED:
            return _("Unlocked bezier");
            break;
        case Auto::LINEAR:
            return _("Linear");
            break;
        case Auto::BEZIER_LOCKED:
        default:
            return _("Locked bezier");
            break;
    }
}

int EditKeyframeMode::text_to_mode(char *text)
{
    int modes[] = 
    {
        Auto::BEZIER_UNLOCKED,
        Auto::LINEAR,
        Auto::BEZIER_LOCKED
    };
    int total_modes = sizeof(modes) / sizeof(int);
    for(int i = 0; i < total_modes; i++)
        if(!strcmp(mode_to_text(i), text)) return i;
    return Auto::LINEAR;
}







