
/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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
#include "edit.h"
#include "editpopup.h"
#include "edl.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindialog.h"
#include "resizetrackthread.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
#include "trackcanvas.h"


#include <string.h>

EditPopup::EditPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

EditPopup::~EditPopup()
{
    edit_editors.remove_all_objects();
}

void EditPopup::create_objects()
{
	add_item(new EditAttachEffect(mwindow, this));
	add_item(new EditMoveTrackUp(mwindow, this));
	add_item(new EditMoveTrackDown(mwindow, this));
	add_item(new EditPopupDeleteTrack(mwindow, this));
	add_item(new EditPopupAddTrack(mwindow, this));
//	add_item(new EditPopupTitle(mwindow, this));

	resize_option = 0;
	matchsize_option = 0;
    info = 0;
}

int EditPopup::update(Track *track, Edit *edit)
{
	this->edit = edit;
	this->track = track;

// make them always the same order
    if(resize_option)
    {
    	remove_item(resize_option);
    }
    
    if(matchsize_option)
    {
    	remove_item(matchsize_option);
    }
    
    if(info)
    {
        remove_item(info);
    }

	resize_option = 0;
	matchsize_option = 0;
    info = 0;


    if(edit && !info)
    {
        add_item(info = new EditInfo(mwindow, this));
    }



	if(track->data_type == TRACK_VIDEO && !resize_option)
	{
		add_item(resize_option = new EditPopupResize(mwindow, this));
		add_item(matchsize_option = new EditPopupMatchSize(mwindow, this));
	}
	return 0;
}









EditAttachEffect::EditAttachEffect(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Attach effect..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new PluginDialogThread(mwindow);
}

EditAttachEffect::~EditAttachEffect()
{
	delete dialog_thread;
}

int EditAttachEffect::handle_event()
{
	dialog_thread->start_window(popup->track,
		0, 
		PROGRAM_NAME ": Attach Effect",
		0,
		popup->track->data_type);
	return 1;
}


EditMoveTrackUp::EditMoveTrackUp(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Move up"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
EditMoveTrackUp::~EditMoveTrackUp()
{
}
int EditMoveTrackUp::handle_event()
{
	mwindow->move_track_up(popup->track);
	return 1;
}



EditMoveTrackDown::EditMoveTrackDown(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Move down"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
EditMoveTrackDown::~EditMoveTrackDown()
{
}
int EditMoveTrackDown::handle_event()
{
	mwindow->move_track_down(popup->track);
	return 1;
}


EditInfo::EditInfo(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Edit info..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
EditInfo::~EditInfo()
{
}
int EditInfo::handle_event()
{
    int got_it = 0;
    if(!popup->edit)
    {
        return 0;
    }

    for(int i = 0; i < popup->edit_editors.size(); i++)
    {
        EditInfoThread *thread = popup->edit_editors.get(i);
        if(!thread->running())
        {
            thread->show_edit(popup->edit);
            got_it = 1;
        }
    }
    
    if(!got_it)
    {
//printf("EditInfo::handle_event %d edit=%p\n", __LINE__, popup->edit);
        EditInfoThread *thread = new EditInfoThread(mwindow);
        popup->edit_editors.append(thread);
        thread->show_edit(popup->edit);
    }
	return 1;
}


EditInfoThread::EditInfoThread(MWindow *mwindow)
 : BC_DialogThread()
{
    this->mwindow = mwindow;
}

EditInfoThread::~EditInfoThread()
{
}

void EditInfoThread::show_edit(Edit *edit)
{
    this->is_silence = edit->silence();
    Indexable *edit_source = 0;
    if(edit->asset)
    {
        edit_source = edit->asset;
    }
    else
    {
        edit_source = edit->nested_edl;
    }
    
    
    if(this->is_silence)
    {
        path.assign("SILENCE");
    }
    else
    {
        this->path.assign(edit_source->path);
    }
    
    this->data_type = edit->track->data_type;
    this->startsource = edit->startsource;
    this->startproject = edit->startproject;
    this->length = edit->length;
    int edl_rate = 1;
    int source_rate = 1;
    if(this->data_type == TRACK_AUDIO)
    {
        edl_rate = edit->edl->get_sample_rate();
        source_rate = edit_source->get_sample_rate();
    }
    else
    {
        edl_rate = edit->edl->get_frame_rate();
        source_rate = edit_source->get_frame_rate();
    }
    
    this->startsource_s = (double)edit->startsource / 
        edl_rate;
    this->startproject_s = (double)edit->startproject / 
        source_rate;
    this->length_s = (double)edit->length / 
        edl_rate;
    this->channel = edit->channel;
    mwindow->gui->unlock_window();
    BC_DialogThread::start();
    mwindow->gui->lock_window("EditInfoThread::show_edit");
}

BC_Window* EditInfoThread::new_gui()
{
	mwindow->gui->lock_window("EditInfoThread::new_gui");
	int x = mwindow->gui->get_abs_cursor_x(0) - 
		mwindow->session->plugindialog_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) - 
		mwindow->session->plugindialog_h / 2;
    EditInfoGUI *gui = new EditInfoGUI(mwindow, this, x, y);
    gui->create_objects();
	mwindow->gui->unlock_window();
    return gui;
}

char* EditInfoThread::format_to_text(int format)
{
    switch(format)
    {
        case EDIT_INFO_HHMMSS:
            return TIME_HMS_TEXT;
        default:
            if(data_type == TRACK_AUDIO)
            {
                return TIME_SAMPLES_TEXT;
            }
            else
            {
                return TIME_FRAMES_TEXT;
            }
    }
    return (char*)"";
}

int EditInfoThread::text_to_format(char *text)
{
    if(!strcmp(text, TIME_HMS_TEXT))
    {
        return EDIT_INFO_HHMMSS;
    }
    else
    {
        return EDIT_INFO_FRAMES;
    }
}






EditInfoFormat::EditInfoFormat(MWindow *mwindow, 
    EditInfoGUI *gui, 
    EditInfoThread *thread,
    int x,
    int y)
 : BC_PopupMenu(x,
    y,
    DP(200),
    thread->format_to_text(mwindow->session->edit_info_format),
    1)
{
    this->mwindow = mwindow;
    this->gui = gui;
    this->thread = thread;
}

EditInfoFormat::~EditInfoFormat()
{
}


int EditInfoFormat::handle_event()
{
    mwindow->session->edit_info_format = thread->text_to_format(get_text());
    gui->update();
//    printf("EditInfoFormat::handle_event %d %d\n", __LINE__, mwindow->session->edit_info_format);
    return 1;
}







#define WINDOW_W DP(400)
#define WINDOW_H DP(200)

EditInfoGUI::EditInfoGUI(MWindow *mwindow, EditInfoThread *thread, int x, int y)
 : BC_Window(PROGRAM_NAME ": Edit Info", 
 	x, 
	y, 
	WINDOW_W, 
	WINDOW_H,
	WINDOW_W,
	WINDOW_H,
	0,
	0,
	1)
{
    this->mwindow = mwindow;
    this->thread = thread;
}


EditInfoGUI::~EditInfoGUI()
{
}


void EditInfoGUI::create_objects()
{
    int margin = mwindow->theme->widget_border;
    int x = margin;
    int x1 = x;
    int y = margin;
    BC_Title *title;
    BC_TextBox *text;
    
    add_subwindow(title = new BC_Title(x, y, _("Path:")));
    x1 = x + title->get_w() + margin;
    add_subwindow(text = new BC_TextBox(x1, 
        y,
        get_w() - margin - x1,
        1,
        &thread->path));
    text->set_read_only(1);

    y += text->get_h() + margin;
    add_subwindow(title = new BC_Title(x, y, _("Source Start:")));
    x1 = x + title->get_w() + margin;
    add_subwindow(startsource = new BC_TextBox(x1, 
        y,
        get_w() - margin - x1,
        1,
        thread->startsource));
    startsource->set_read_only(1);

    y += startsource->get_h() + margin;
    add_subwindow(title = new BC_Title(x, y, _("Project Start:")));
    x1 = x + title->get_w() + margin;
    add_subwindow(startproject = new BC_TextBox(x1, 
        y,
        get_w() - margin - x1,
        1,
        thread->startproject));
    startproject->set_read_only(1);

    y += startproject->get_h() + margin;
    add_subwindow(title = new BC_Title(x, y, _("Length:")));
    x1 = x + title->get_w() + margin;
    add_subwindow(length = new BC_TextBox(x1, 
        y,
        get_w() - margin - x1,
        1,
        thread->length));
    length->set_read_only(1);

    y += length->get_h() + margin;
    add_subwindow(title = new BC_Title(x, y, _("Channel:")));
    x1 = x + title->get_w() + margin;
    add_subwindow(text = new BC_TextBox(x1, 
        y,
        get_w() - margin - x1,
        1,
        thread->channel));
    text->set_read_only(1);

    y += text->get_h() + margin;
    add_subwindow(title = new BC_Title(x, y, _("Format:")));
    x1 = x + title->get_w() + margin;
    EditInfoFormat *format;
    add_subwindow(format = new EditInfoFormat(mwindow, 
        this, 
        thread,
        x1,
        y));
    format->add_item(new BC_MenuItem(thread->format_to_text(EDIT_INFO_FRAMES)));
    format->add_item(new BC_MenuItem(thread->format_to_text(EDIT_INFO_HHMMSS)));

    add_subwindow(new BC_CancelButton(this));
// print the titles in the right format
    update();

	show_window();
}

void EditInfoGUI::update()
{
    if(mwindow->session->edit_info_format == EDIT_INFO_FRAMES)
    {
        startsource->update(thread->startsource);
        startproject->update(thread->startproject);
        length->update(thread->length);
    }
    else
    {
        char string[BCTEXTLEN];
        Units::totext(string, 
				thread->startsource_s, 
				TIME_HMS);
        startsource->update(string);
        Units::totext(string, 
				thread->startproject_s, 
				TIME_HMS);
        startproject->update(string);
        Units::totext(string, 
				thread->length_s, 
				TIME_HMS);
        length->update(string);
    }
}







EditPopupResize::EditPopupResize(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Resize track..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new ResizeTrackThread(mwindow, 
		popup->track->tracks->number_of(popup->track));
}
EditPopupResize::~EditPopupResize()
{
	delete dialog_thread;
}

int EditPopupResize::handle_event()
{
	dialog_thread->start_window(popup->track, popup->track->tracks->number_of(popup->track));
	return 1;
}






EditPopupMatchSize::EditPopupMatchSize(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Match output size"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
EditPopupMatchSize::~EditPopupMatchSize()
{
}

int EditPopupMatchSize::handle_event()
{
	mwindow->match_output_size(popup->track);
	return 1;
}







EditPopupDeleteTrack::EditPopupDeleteTrack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
int EditPopupDeleteTrack::handle_event()
{
	mwindow->delete_track(popup->track);
	return 1;
}






EditPopupAddTrack::EditPopupAddTrack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Add track"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int EditPopupAddTrack::handle_event()
{
	if(popup->track->data_type == TRACK_AUDIO)
		mwindow->add_audio_track_entry(1, popup->track);
	else
		mwindow->add_video_track_entry(popup->track);
	return 1;
}






// EditPopupTitle::EditPopupTitle(MWindow *mwindow, EditPopup *popup)
//  : BC_MenuItem(_("User title..."))
// {
// 	this->mwindow = mwindow;
// 	this->popup = popup;
// 	window = 0;
// }
// 
// EditPopupTitle::~EditPopupTitle()
// {
// 	delete popup;
// }
// 
// int EditPopupTitle::handle_event()
// {
// 	int result;
// 
// 	Track *trc = mwindow->session->track_highlighted;
// 
// 	if (trc && trc->record)
// 	{
// 		Edit *edt = mwindow->session->edit_highlighted;
// 		if(!edt) return 1;
// 
// 		window = new EditPopupTitleWindow (mwindow, popup);
// 		window->create_objects();
// 		result = window->run_window();
// 
// 
// 		if(!result && edt)
// 		{
// 			strcpy(edt->user_title, window->title_text->get_text());
// 		}
// 
// 		delete window;
// 		window = 0;
// 	}
// 
// 	return 1;
// }
// 
// 
// EditPopupTitleWindow::EditPopupTitleWindow (MWindow *mwindow, EditPopup *popup)
//  : BC_Window (PROGRAM_NAME ": Set edit title",
// 	mwindow->gui->get_abs_cursor_x(0) - 400 / 2,
// 	mwindow->gui->get_abs_cursor_y(0) - 500 / 2,
// 	300,
// 	100,
// 	300,
// 	100,
// 	0,
// 	0,
// 	1)
// {
// 	this->mwindow = mwindow;
// 	this->popup = popup;
// 	this->edt = this->mwindow->session->edit_highlighted;
// 	if(this->edt)
// 	{
// 		strcpy(new_text, this->edt->user_title);
// 	}
// }
// 
// EditPopupTitleWindow::~EditPopupTitleWindow()
// {
// }
// 
// int EditPopupTitleWindow::close_event()
// {
// 	set_done(1);
// 	return 1;
// }
// 
// void EditPopupTitleWindow::create_objects()
// {
// 	int x = 5;
// 	int y = 10;
// 
// 	add_subwindow (new BC_Title (x, y, _("User title")));
// 	add_subwindow (title_text = new EditPopupTitleText (this,
// 		mwindow, x, y + 20));
// 	add_tool(new BC_OKButton(this));
// 	add_tool(new BC_CancelButton(this));
// 
// 
// 	show_window();
// 	flush();
// }
// 
// 
// EditPopupTitleText::EditPopupTitleText (EditPopupTitleWindow *window, 
// 	MWindow *mwindow, int x, int y)
//  : BC_TextBox(x, y, 250, 1, (char*)(window->edt ? window->edt->user_title : ""))
// {
// 	this->window = window;
// 	this->mwindow = mwindow;
// }
// 
// EditPopupTitleText::~EditPopupTitleText() 
// { 
// }
//  
// int EditPopupTitleText::handle_event()
// {
// 	return 1;
// }







