/*
 * CINELERRA
 * Copyright (C) 1997-2025 Adam Williams <broadcast at earthling dot net>
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
    before = new FloatAuto;
    after = new FloatAuto;
}

void EditKeyframeThread::start(Auto *auto_)
{
    if(!is_running())
    {
        this->auto_ = (FloatAuto*)auto_;
        
        auto_copy->copy_from(auto_, 1);
        before->copy_from(auto_, 1);

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
	mwindow->gui->unlock_window();
	EditKeyframeDialog *window = new EditKeyframeDialog(mwindow, 
		this,
		x, 
		y);
	window->create_objects();
	return window;
}

void EditKeyframeThread::handle_close_event(int result)
{
    if(before->identical(auto_copy)) return;
    if(result)
    {
// revert it
        mwindow->gui->lock_window("EditKeyframeThread::handle_close_event");
        if(mwindow->edl->tracks->keyframe_exists(auto_))
        {
            auto_->copy_from(before, 0);
            apply_common();
        }
        mwindow->gui->unlock_window();
    }
    else
    {
        mwindow->gui->lock_window("EditKeyframeThread::handle_close_event");
        if(mwindow->edl->tracks->keyframe_exists(auto_))
        {
    // single undo update when the window is closed
            after->copy_from(auto_copy, 0);
    // revert to the before value
            auto_->copy_from(before, 0);
            mwindow->undo->update_undo_before(_("edit keyframe"), this);
    // revert to the after value
            auto_->copy_from(after, 0);
            mwindow->undo->update_undo_after(_("edit keyframe"), LOAD_AUTOMATION);
        }
        mwindow->gui->unlock_window();
    }
}

void EditKeyframeThread::apply_common()
{
	mwindow->restart_brender();
    mwindow->gui->draw_overlays(1, 1);
    mwindow->sync_parameters(CHANGE_PARAMS);
}

// copy from the temporary to the EDL keyframe with undo code
void EditKeyframeThread::apply(EditKeyframeDialog *gui,
    float *changed_value)
{
//printf("EditKeyframeThread::apply %d %p\n", __LINE__, auto_);
// convert it to locked bezier
    if(auto_copy->mode == FloatAuto::BEZIER_LOCKED &&
        auto_->mode != FloatAuto::BEZIER_LOCKED)
    {
        auto_copy->to_locked();
        gui->in->update(auto_copy->control_in_value);
        gui->out->update(auto_copy->control_out_value);
    }
    else
// mirror the changed control point
    if(changed_value &&
        auto_copy->mode == FloatAuto::BEZIER_LOCKED)
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
//        mwindow->undo->update_undo_before(_("edit keyframe"), this);

        auto_->copy_from(auto_copy, 0);


//        mwindow->undo->update_undo_after(_("edit keyframe"), LOAD_AUTOMATION);

        apply_common();
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

    lock_window("EditKeyframeDialog::create_objects");
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
    lock_texts();
   
//    BC_OKButton *ok;
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
//    ok->set_esc(1);
	show_window();
    unlock_window();
}

void EditKeyframeDialog::lock_texts()
{
    if(thread->auto_copy->mode == FloatAuto::LINEAR ||
        thread->auto_copy->mode == FloatAuto::BEZIER_TANGENT)
    {
        in->disable();
        out->disable();
    }
    else
    {
        in->enable();
        out->enable();
    }
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
	add_item(new BC_MenuItem(mode_to_text(FloatAuto::LINEAR)));
	add_item(new BC_MenuItem(mode_to_text(FloatAuto::BEZIER_UNLOCKED)));
	add_item(new BC_MenuItem(mode_to_text(FloatAuto::BEZIER_LOCKED)));
//	add_item(new BC_MenuItem(mode_to_text(FloatAuto::BEZIER_TANGENT)));
}
int EditKeyframeMode::handle_event()
{
    gui->thread->auto_copy->mode = text_to_mode(get_text());
    gui->thread->apply(gui, 0);
    gui->lock_texts();
    return 1;
}

char* EditKeyframeMode::mode_to_text(int mode)
{
    switch(mode)
    {
        case FloatAuto::BEZIER_UNLOCKED:
            return _("Unlocked bezier");
            break;
        case FloatAuto::BEZIER_TANGENT:
            return _("Tangent bezier");
            break;
        case FloatAuto::LINEAR:
            return _("Linear");
            break;
        case FloatAuto::BEZIER_LOCKED:
        default:
            return _("Locked bezier");
            break;
    }
}

int EditKeyframeMode::text_to_mode(char *text)
{
    int modes[] = 
    {
        FloatAuto::BEZIER_UNLOCKED,
        FloatAuto::LINEAR,
        FloatAuto::BEZIER_LOCKED,
        FloatAuto::BEZIER_TANGENT
    };
    int total_modes = sizeof(modes) / sizeof(int);
    for(int i = 0; i < total_modes; i++)
        if(!strcmp(mode_to_text(i), text)) return i;
    return FloatAuto::LINEAR;
}







