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

#include "datatype.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlfactory.h"
#include "edlsession.h"
#include "filepreviewer.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindialog.h"
#include "pluginserver.h"
#include "preferences.h"
#include "theme.h"
#include "tracks.h"
#include "transition.h"
#include "transitiondialog.h"
#include "transportque.h"



#include <string.h>



TransitionDialogThread::TransitionDialogThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
}

void TransitionDialogThread::start(int data_type, 
    Edit *dst_edit) // set based on what's calling it
{
    if(!is_running())
    {
	    this->data_type = data_type;
        this->dst_edit = dst_edit;

	    transition_names.remove_all_objects();
// Construct listbox names	
	    ArrayList<PluginServer*> plugindb;
	    MWindow::search_plugindb(data_type == TRACK_AUDIO, 
		    data_type == TRACK_VIDEO, 
		    0, 
		    1,
		    0,
		    plugindb);
	    for(int i = 0; i < plugindb.total; i++)
		    transition_names.append(new BC_ListBoxItem(_(plugindb.get(i)->title)));

	    if(data_type == TRACK_AUDIO)
		    strcpy(transition_title, mwindow->edl->session->default_atransition);
	    else
		    strcpy(transition_title, mwindow->edl->session->default_vtransition);

	    mwindow->gui->unlock_window();
	    BC_DialogThread::start();
	    mwindow->gui->lock_window("TransitionDialogThread::start");
    }
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
        mwindow->paste_transitions(data_type, transition_title, dst_edit);
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
	DP(320), 
	DP(240),
	1,
	0,
	1)
{
// printf("TransitionDialog::TransitionDialog %d %d %d %d %d\n", 
// __LINE__,
// x,
// y,
// mwindow->session->transitiondialog_w,
// mwindow->session->transitiondialog_h);
	this->mwindow = mwindow;
	this->thread = thread;
}

TransitionDialog::~TransitionDialog()
{
    TransitionPreviewer::instance.clear_preview();
    TransitionPreviewer::instance.set_gui(0);
}


void TransitionDialog::create_objects()
{
	BC_Resources *resources = BC_WindowBase::get_resources();
    int widget_border = MWindow::theme->widget_border;
    int window_border = MWindow::theme->window_border;
	int x = window_border;
	int y = window_border;
    compute_sizes(get_w(), get_h());

	lock_window("TransitionDialog::create_objects");
	add_subwindow(name_title = new BC_Title(x, y, _("Select transition from list")));
	y += name_title->get_h() + widget_border;
	add_subwindow(name_list = new TransitionDialogName(thread, 
		&thread->transition_names, 
		list_x, 
		list_y,
		list_w,
		list_h));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
    
    TransitionPreviewer::instance.set_gui(this);
}



int TransitionDialog::resize_event(int w, int h)
{
	BC_Resources *resources = BC_WindowBase::get_resources();
    int widget_border = MWindow::theme->widget_border;
    int window_border = MWindow::theme->window_border;
	int x = window_border;
	int y = window_border;
    compute_sizes(w, h);

	name_title->reposition_window(x, y);
	y += name_title->get_h() + widget_border;
	name_list->reposition_window(list_x, 
		list_y,
		list_w,
		list_h);
    TransitionPreviewer::instance.handle_resize(w, h);
	return 1;
}

void TransitionDialog::compute_sizes(int w, int h)
{
	BC_Resources *resources = BC_WindowBase::get_resources();
    int widget_border = MWindow::theme->widget_border;
    int window_border = MWindow::theme->window_border;
	int x = window_border;
	int y = window_border;
    y += BC_Title::calculate_h(this, "Xj");
    preview_w = resources->filebox_preview_w;
    preview_x = w - preview_w;
    list_x = 0;
    list_y = y;
    list_w = preview_x;
    list_h = h - y - BC_OKButton::calculate_h() - window_border - widget_border;
    preview_center_y = list_y + list_h / 2;
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
    const char *title = thread->transition_names.values[number]->get_text();
	strcpy(thread->transition_title, 
		title);
    TransitionPreviewer::instance.submit_transition(title, 
        thread->data_type);
	return 1;
}



TransitionPreviewer TransitionPreviewer::instance;


TransitionPreviewer::TransitionPreviewer() : Previewer()
{
}

TransitionPreviewer::~TransitionPreviewer()
{
}


void TransitionPreviewer::initialize()
{
    Previewer::initialize();
}

void TransitionPreviewer::handle_resize(int w, int h)
{
    TransitionDialog *gui = (TransitionDialog*)this->gui;
    previewer_lock->lock("TransitionPreviewer::handle_resize");
    int x = gui->preview_x /* + margin */;
    int y = gui->preview_center_y - canvas_h / 2;
    if(canvas)
    {
        canvas->reposition_window(x,
            y,
            canvas_w,
            canvas_h);
        y += canvas_h;
    }
    if(scroll)
    {
        scroll->reposition_window(x,
            y,
            canvas_w);
        y += scroll->get_h();
    }
    if(rewind)
    {
        rewind->reposition_window(x,
            y);
        x += rewind->get_w();
    }
    if(play)
    {
        play->reposition_window(x,
            y);
    }
    previewer_lock->unlock();
}

// Sets up playback in the foreground, since there is no file format detection
void TransitionPreviewer::submit_transition(const char *title, int data_type)
{
    interrupt_playback();
    
    previewer_lock->lock("TransitionPreviewer::submit_transition");
    if(edl) 
    {
        edl->remove_user();
        edl = 0;
    }

// load a transition template EDL
    FileXML xml_file;
    char path[BCTEXTLEN];
    if(data_type == TRACK_AUDIO)
        sprintf(path, "%s/previews/atransition.xml", MWindow::preferences->plugin_dir);
    else
        sprintf(path, "%s/previews/vtransition.xml", MWindow::preferences->plugin_dir);
    if(!xml_file.read_from_file(path))
    {
        edl = new EDL;
        edl->create_objects();
        if(!edl->load_xml(&xml_file, LOAD_ALL))
        {
            Track *track = edl->tracks->first;
            if(track)
            {
                Edit *edit = track->edits->last;
                if(edit)
                {
                    Transition *transition = edit->transition;
                    if(transition)
                    {
// replace the transition title & force it to use its default keyframe
                        strcpy(transition->title, title);
                        transition->get_keyframe()->set_data("");
                    }
                }
            }
        }
        play_position = 0;
    }

    previewer_lock->unlock();
    create_preview();
}

void TransitionPreviewer::create_preview()
{
    TransitionDialog *gui = (TransitionDialog*)this->gui;
    previewer_lock->lock("TransitionPreviewer::create_preview");

    if(edl)
    {
        canvas_w = gui->preview_w;
        canvas_h = canvas_w;
// create widgets for video
        if(edl->tracks->playable_video_tracks())
        {
            canvas_h = canvas_w * edl->session->output_h / edl->session->output_w;
        }
        else
// create widgets for audio
        {
            canvas_h = canvas_w * 
                speaker_image->get_h() / 
                speaker_image->get_w();
        }
        int x = gui->preview_x /* + margin */;
        int y = gui->preview_center_y - canvas_h / 2;

        if(canvas)
            canvas->reposition_window(x,
                y,
                canvas_w,
                canvas_h);
        else
            gui->add_subwindow(canvas = new PreviewCanvas(this, 
                x,
                y,
                canvas_w,
                canvas_h));
        canvas->show_window(0);
        y += canvas_h;
         
        if(scroll)
        {
            double total = edl->tracks->total_playable_length();
            scroll->reposition_window(x,
                y,
                canvas_w);
            scroll->update(canvas_w,
                play_position * 
                    scroll->get_length() / 
                    total,
                0,
                scroll->get_length());
        }
        else
            gui->add_subwindow(scroll = new PreviewerScroll(this, 
                x,
                y,
                canvas_w));
        y += scroll->get_h();

        if(rewind)
            rewind->reposition_window(x,
                y);
        else
            gui->add_subwindow(rewind = new PreviewerRewind(this, 
                x,
                y));
        x += rewind->get_w();

        if(play)
            play->reposition_window(x,
                y);
        else
            gui->add_subwindow(play = new PreviewerPlay(this, 
                x,
                y));
        
        if(!edl->tracks->playable_video_tracks())
            canvas->draw_vframe(speaker_image,
                0,
                0,
                canvas->get_w(),
                canvas->get_h());
            canvas->flash(0);
        
        gui->flush();
        playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_ALL,
			edl,
			1);
    }

    previewer_lock->unlock();
}


















