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


#include "asset.h"
#include "assets.h"
#include "clip.h"
#include "datatype.h"
#include "edit.h"
#include "editinfo.h"
#include "edl.h"
#include "indexable.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "nestededls.h"
#include "theme.h"
#include "track.h"



EditInfoGUI::EditInfoGUI(MWindow *mwindow, EditInfoThread *thread, int x, int y)
 : BC_Window(PROGRAM_NAME ": Edit Info", 
 	x, 
	y, 
	mwindow->session->edit_info_w, 
	mwindow->session->edit_info_h,
	100, 
	100,
	1,
	0,
	1)
{
    this->mwindow = mwindow;
    this->thread = thread;
}


EditInfoGUI::~EditInfoGUI()
{
    delete path;
}


void EditInfoGUI::create_objects()
{
    int margin = mwindow->theme->widget_border;
    int x = margin;
    int y = margin;
    int text_h = margin + BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1);
    BC_Title *title;
    x1 = 0;

    if(mwindow->session->edit_info_format != TIME_FRAMES &&
        mwindow->session->edit_info_format != TIME_HMSF &&
        mwindow->session->edit_info_format != TIME_SAMPLES &&
        mwindow->session->edit_info_format != TIME_HMS)
        mwindow->session->edit_info_format = TIME_HMS;

    add_subwindow(title = new BC_Title(x, y, _("Type:")));
    x1 = MAX(x1, x + title->get_w());
    y += text_h;
    add_subwindow(title = new BC_Title(x, y, _("Path:")));
    x1 = MAX(x1, x + title->get_w());
    y += text_h;
    add_subwindow(title = new BC_Title(x, y, _("Source Start:")));
    x1 = MAX(x1, x + title->get_w());
    y += text_h;
    add_subwindow(title = new BC_Title(x, y, _("Project Start:")));
    x1 = MAX(x1, x + title->get_w());
    y += text_h;
    add_subwindow(title = new BC_Title(x, y, _("Length:")));
    x1 = MAX(x1, x + title->get_w());
    y += text_h;
    add_subwindow(title = new BC_Title(x, y, _("Channel:")));
    x1 = MAX(x1, x + title->get_w());
    y += text_h;
    add_subwindow(title = new BC_Title(x, y, _("Units:")));
    x1 = MAX(x1, x + title->get_w());
    y += text_h;

    y = margin;
    x1 += margin;
    if(thread->data_type == TRACK_AUDIO)
    {
        add_subwindow(title = new BC_Title(x1, y, _("Audio")));
    }
    else
    {
        add_subwindow(title = new BC_Title(x1, y, _("Video")));
    }
    
    y += text_h;
    path = new EditInfoPath(x1, 
        y,
        get_w() - margin - x1 - BC_PopupTextBox::calculate_w(),
        DP(300),
        thread,
        this);
    path->create_objects();
    path->set_read_only(1);

    y += text_h;
    add_subwindow(startsource = new EditInfoNumber(this,
        x1, 
        y,
        get_w() - margin - x1 - BC_Tumbler::calculate_w(),
        &thread->startsource));
    add_subwindow(startsource2 = new EditInfoTumbler(this, 
        x1 + startsource->get_w(), 
        y, 
        startsource));
    y += MAX(text_h, startsource2->get_h());
    add_subwindow(startproject = new EditInfoNumber(this,
        x1, 
        y,
        get_w() - margin - x1 - BC_Tumbler::calculate_w(),
        &thread->startproject));
    add_subwindow(startproject2 = new EditInfoTumbler(this, 
        x1 + startproject->get_w(), 
        y, 
        startproject));
    y += MAX(text_h, startproject2->get_h());
    add_subwindow(length = new EditInfoNumber(this,
        x1, 
        y,
        get_w() - margin - x1 - BC_Tumbler::calculate_w(),
        &thread->length));
    add_subwindow(length2 = new EditInfoTumbler(this, 
        x1 + length->get_w(), 
        y, 
        length));
    y += MAX(text_h, length2->get_h());
    channel = new EditInfoChannel(this,
        x1, 
        y,
        get_w() - margin - x1 - BC_TumbleTextBox::calculate_w(),
        &thread->channel);
    channel->create_objects();
//    channel->set_read_only(1);
    y += text_h;
    int w_argument = get_w() - x1 - margin;
// this adds a button to its w argument
    add_subwindow(format = new EditInfoFormat(mwindow, 
        this, 
        thread,
        x1,
        y,
        w_argument - (BC_PopupMenu::calculate_w(w_argument) - w_argument)));
    format->add_item(new BC_MenuItem(thread->format_to_text(TIME_FRAMES)));
    format->add_item(new BC_MenuItem(thread->format_to_text(TIME_SAMPLES)));
    format->add_item(new BC_MenuItem(thread->format_to_text(TIME_HMSF)));
    format->add_item(new BC_MenuItem(thread->format_to_text(TIME_HMS)));




    add_subwindow(new BC_OKButton(this));
    add_subwindow(new BC_CancelButton(this));
// print the titles in the right format
    lock_window("EditInfoGUI::create_objects");
    update();
    unlock_window();

	show_window();
}

int EditInfoGUI::resize_event(int w, int h)
{
    int margin = mwindow->theme->widget_border;
    path->reposition_window(path->get_x(),
        path->get_y(),
        w - margin - x1 - BC_PopupTextBox::calculate_w());
    startsource->reposition_window(startsource->get_x(),
        startsource->get_y(),
        w - margin - x1 - BC_Tumbler::calculate_w(),
        1);
    startsource2->reposition_window(startsource->get_x() + startsource->get_w(),
        startsource2->get_y());
        
    startproject->reposition_window(startproject->get_x(),
        startproject->get_y(),
        w - margin - x1 - BC_Tumbler::calculate_w(),
        1);
    startproject2->reposition_window(startproject->get_x() + startproject->get_w(),
        startproject2->get_y());


    length->reposition_window(length->get_x(),
        length->get_y(),
        w - margin - x1 - BC_Tumbler::calculate_w(),
        1);
    length2->reposition_window(length->get_x() + length->get_w(),
        length2->get_y());

    channel->reposition_window(channel->get_x(),
        channel->get_y(),
        w - margin - x1 - BC_TumbleTextBox::calculate_w());
    format->reposition_window(format->get_x(),
        format->get_y(),
        w - x1 - margin);
    
    mwindow->session->edit_info_w = w;
    mwindow->session->edit_info_h = h;
	flush();
	return 1;
}


void EditInfoGUI::update()
{
// direct copy these formats
    if((thread->data_type == TRACK_VIDEO &&
        MWindow::session->edit_info_format == TIME_FRAMES) ||
        (thread->data_type == TRACK_AUDIO &&
        MWindow::session->edit_info_format == TIME_SAMPLES))
    {
        startsource->update(thread->startsource);
        startproject->update(thread->startproject);
        length->update(thread->length);
    }
    else
    {
// convert a seconds intermediate
        char string[BCTEXTLEN];
        Units::totext(string, 
			thread->from_units(thread->startsource), 
			mwindow->session->edit_info_format,
            mwindow->edl->get_sample_rate(),
            mwindow->edl->get_frame_rate());
        startsource->update(string);
//printf("EditInfoGUI::update %d %f\n", __LINE__, thread->from_units(thread->startsource));


        Units::totext(string, 
			thread->from_units(thread->startproject), 
			mwindow->session->edit_info_format,
            mwindow->edl->get_sample_rate(),
            mwindow->edl->get_frame_rate());
        startproject->update(string);

        Units::totext(string, 
			thread->from_units(thread->length), 
			mwindow->session->edit_info_format,
            mwindow->edl->get_sample_rate(),
            mwindow->edl->get_frame_rate());
        length->update(string);
    }
}







EditInfoPath::EditInfoPath(int x, 
    int y, 
    int w,
    int h,
    EditInfoThread *thread,
    EditInfoGUI *gui)
 : BC_PopupTextBox(gui,
    &thread->path_listitems,
    thread->path.c_str(),
    x,
    y,
    w,
    h)
{
    this->thread = thread;
    this->gui = gui;
}

int EditInfoPath::handle_event()
{
	int result = 0;
	if(get_textbox()->get_keypress() != RETURN)
	{
		result = get_textbox()->calculate_suggestions(&thread->path_listitems, 1);
	}
    thread->path.assign(get_text());
	return result;
}




EditInfoNumber::EditInfoNumber(EditInfoGUI *gui,
    int x, 
    int y, 
    int w, 
    int64_t *output)
 : BC_TextBox(x, y, w, 1, *output)
{
    this->output = output;
    this->gui = gui;
}

int EditInfoNumber::handle_event()
{
// direct copy these formats
    if((gui->thread->data_type == TRACK_VIDEO &&
        MWindow::session->edit_info_format == TIME_FRAMES) ||
        (gui->thread->data_type == TRACK_AUDIO &&
        MWindow::session->edit_info_format == TIME_SAMPLES))
    {
        *output = atol(get_text());
    }
    else
    {
// convert a seconds intermediate
        double seconds = Units::text_to_seconds(
            get_text(), 
			gui->mwindow->edl->get_sample_rate(), 
			MWindow::session->edit_info_format, 
			gui->mwindow->edl->get_frame_rate(), 
			1000);  // frames_per_foot
        *output = gui->thread->to_units(seconds);
    }
//             printf("EditInfoNumber::handle_event %d seconds=%f output=%ld\n", 
//                 __LINE__, 
//                 seconds,
//                 *output);
    return 0;
}

int EditInfoNumber::button_press_event()
{
	if(is_event_win())
	{
		if(get_buttonpress() < 4)
        {
            return BC_TextBox::button_press_event();
        }

		if(get_buttonpress() == 4)
		{
			increase();
		}
		else
		if(get_buttonpress() == 5)
		{
			decrease();
		}
		return 1;
	}
	return 0;
}


void EditInfoNumber::increase()
{
// step in the track units
    (*output)++;
// direct copy these formats
    if((gui->thread->data_type == TRACK_VIDEO &&
        MWindow::session->edit_info_format == TIME_FRAMES) ||
        (gui->thread->data_type == TRACK_AUDIO &&
        MWindow::session->edit_info_format == TIME_SAMPLES))
    {
        update(*output);
    }
    else
    {
// convert a seconds intermediate
        char string[BCTEXTLEN];
        Units::totext(string, 
			gui->thread->from_units(*output), 
			MWindow::session->edit_info_format,
            gui->mwindow->edl->get_sample_rate(),
            gui->mwindow->edl->get_frame_rate());
        update(string);
    }
}

void EditInfoNumber::decrease()
{
// step in the track units
    if(*output > 0)
        (*output)--;
// direct copy these formats
    if((gui->thread->data_type == TRACK_VIDEO &&
        MWindow::session->edit_info_format == TIME_FRAMES) ||
        (gui->thread->data_type == TRACK_AUDIO &&
        MWindow::session->edit_info_format == TIME_SAMPLES))
    {
        update(*output);
    }
    else
    {
// convert a seconds intermediate
        char string[BCTEXTLEN];
        Units::totext(string, 
			gui->thread->from_units(*output), 
			MWindow::session->edit_info_format,
            gui->mwindow->edl->get_sample_rate(),
            gui->mwindow->edl->get_frame_rate());
        update(string);
    }
}




EditInfoTumbler::EditInfoTumbler(EditInfoGUI *gui, 
    int x, 
    int y, 
    EditInfoNumber *text)
 : BC_Tumbler(x, y)
{
    this->gui = gui;
    this->text = text;
}

int EditInfoTumbler::handle_up_event()
{
    text->increase();
    return 1;
}


int EditInfoTumbler::handle_down_event()
{
    text->decrease();
    return 1;
}




EditInfoChannel::EditInfoChannel(EditInfoGUI *gui,
    int x, 
    int y, 
    int w,
    int *output)
 : BC_TumbleTextBox(gui,
    *output,
    0, // min
    255, // max
    x, 
    y, 
    w)
{
    this->output = output;
    this->gui = gui;
}
int EditInfoChannel::handle_event()
{
    gui->thread->channel = atoi(get_text());
    return 0;
}


















EditInfoThread::EditInfoThread(MWindow *mwindow)
 : BC_DialogThread()
{
    this->mwindow = mwindow;
}

EditInfoThread::~EditInfoThread()
{
    path_listitems.remove_all_objects();
}

void EditInfoThread::show_edit(Edit *edit)
{
// construct the path substitution list
// TODO: merge with swapasset
    path_listitems.remove_all_objects();
    EDL *edl = edit->edl;
    if(edl)
    {
        for(Asset *current = mwindow->edl->assets->first; 
	        current; 
	        current = NEXT)
        {
            path_listitems.append(new BC_ListBoxItem(current->path));
        }

	    for(int i = 0; i < mwindow->edl->nested_edls->size(); i++)
	    {
		    Indexable *indexable = mwindow->edl->nested_edls->get(i);
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
    }

//printf("EditInfoThread::show_edit %d this=%p edit=%p\n", __LINE__, this, edit);
    this->edit_id = edit->id;
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
        this->path.assign(SILENCE_TEXT);
    }
    else
    {
        this->path.assign(edit_source->path);
    }

    this->data_type = edit->track->data_type;
    this->startsource = edit->startsource;
    this->startproject = edit->startproject;
    this->length = edit->length;
    if(this->data_type == TRACK_AUDIO)
    {
        this->rate = edit->edl->get_sample_rate();
    }
    else
    {
        this->rate = edit->edl->get_frame_rate();
    }

    this->channel = edit->channel;
    
    
    this->orig_path.assign(this->path);
    this->orig_startsource = this->startsource;
    this->orig_startproject = this->startproject;
    this->orig_length = this->length;
    this->orig_channel = this->channel;
    
    
    mwindow->gui->unlock_window();
    BC_DialogThread::start();
    mwindow->gui->lock_window("EditInfoThread::show_edit");
}

BC_Window* EditInfoThread::new_gui()
{
	mwindow->gui->lock_window("EditInfoThread::new_gui");
	int x = mwindow->gui->get_abs_cursor_x(0) - 
		mwindow->session->edit_info_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) - 
		mwindow->session->edit_info_h / 2;
    EditInfoGUI *gui = new EditInfoGUI(mwindow, this, x, y);
    gui->create_objects();
	mwindow->gui->unlock_window();
    return gui;
}

void EditInfoThread::handle_close_event(int result)
{
//    printf("EditInfoThread::handle_close_event %d %d\n", __LINE__, result);


    if(!result &&
// something changed
        (orig_path.compare(path) ||
        orig_startsource != startsource ||
        orig_startproject != startproject ||
        orig_length != length ||
        orig_channel != channel))
    {
        if(!path.compare(SILENCE_TEXT))
        {
            is_silence = 1;
        }
        else
        {
            is_silence = 0;
        }

        mwindow->update_edit(edit_id,
            path,
            startsource,
            startproject,
            length,
            channel,
            is_silence);
    }
}

char* EditInfoThread::format_to_text(int format)
{
    static char string[BCTEXTLEN];
    string[0] = 0;
    Units::print_time_format(format, string);
    return string;
//     switch(format)
//     {
//         case EDIT_INFO_HHMMSS:
//             return TIME_HMS_TEXT;
//         case EDIT_INFO_HHMMSSFF:
//             return TIME_HMSF_TEXT;
//         default:
//             if(data_type == TRACK_AUDIO)
//             {
//                 return TIME_SAMPLES_TEXT;
//             }
//             else
//             {
//                 return TIME_FRAMES_TEXT;
//             }
//     }
//     return (char*)"";
}

int EditInfoThread::text_to_format(char *text)
{
    return Units::text_to_format(text);
//     if(!strcmp(text, TIME_HMS_TEXT))
//     {
//         return EDIT_INFO_HHMMSS;
//     }
//     else
//     if(!strcmp(text, TIME_HMSF_TEXT))
//     {
//         return EDIT_INFO_HHMMSSFF;
//     }
//     else
//     {
//         return EDIT_INFO_FRAMES;
//     }
}

// don't have access to the track
double EditInfoThread::from_units(int64_t x)
{
	return (double)x / rate;
}

int64_t EditInfoThread::to_units(double x)
{
	return Units::round(x * rate);
}











