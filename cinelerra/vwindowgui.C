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

#include "asset.h"
#include "assets.h"
#include "awindowgui.h"
#include "awindow.h"
#include "canvas.h"
#include "clip.h"
#include "clipedit.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "fonts.h"
#include "keys.h"
#include "labels.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "meterpanel.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "playtransport.h"
#include "preferences.h"
#include "theme.h"
#include "timebar.h"
#include "tracks.h"
#include "vframe.h"
#include "vplayback.h"
#include "vtimebar.h"
#include "vwindowgui.h"
#include "vwindow.h"




VWindowGUI::VWindowGUI(MWindow *mwindow, VWindow *vwindow)
 : BC_Window(PROGRAM_NAME ": Viewer",
 	mwindow->session->vwindow_x,
	mwindow->session->vwindow_y,
	mwindow->session->vwindow_w,
	mwindow->session->vwindow_h,
	100,
	100,
	1,
	1,
	0) // Hide it
{
	this->mwindow = mwindow;
	this->vwindow = vwindow;
}

VWindowGUI::~VWindowGUI()
{
printf("VWindowGUI::~VWindowGUI %d\n", __LINE__);
	delete canvas;
	delete transport;
}

void VWindowGUI::change_source(EDL *edl, const char *title)
{
//printf("VWindowGUI::change_source %d\n", __LINE__);

	update_sources(title);
	char string[BCTEXTLEN];

	if(title[0]) 
		sprintf(string, PROGRAM_NAME ": %s", title);
	else
		sprintf(string, PROGRAM_NAME ": Viewer");

	lock_window("VWindowGUI::change_source");
#ifdef USE_SLIDER
	slider->set_position();
#endif
	timebar->update(0);
	set_title(string);
    show_window(0);
	unlock_window();
}


// Get source list from master EDL
void VWindowGUI::update_sources(const char *title)
{
	lock_window("VWindowGUI::update_sources");

//printf("VWindowGUI::update_sources 1\n");
	sources.remove_all_objects();
//printf("VWindowGUI::update_sources 2\n");



	for(int i = 0;
		i < mwindow->edl->clips.total;
		i++)
	{
		char *clip_title = mwindow->edl->clips.values[i]->local_session->clip_title;
		int exists = 0;

		for(int j = 0; j < sources.total; j++)
		{
			if(!strcasecmp(sources.values[j]->get_text(), clip_title))
			{
				exists = 1;
			}
		}

		if(!exists)
		{
			sources.append(new BC_ListBoxItem(clip_title));
		}
	}
//printf("VWindowGUI::update_sources 3\n");

	FileSystem fs;
	for(Asset *current = mwindow->edl->assets->first; 
		current;
		current = NEXT)
	{
		char clip_title[BCTEXTLEN];
		fs.extract_name(clip_title, current->path);
		int exists = 0;
		
		for(int j = 0; j < sources.total; j++)
		{
			if(!strcasecmp(sources.values[j]->get_text(), clip_title))
			{
				exists = 1;
			}
		}
		
		if(!exists)
		{
			sources.append(new BC_ListBoxItem(clip_title));
		}
	}

//printf("VWindowGUI::update_sources 4\n");

//	source->update_list(&sources);
//	source->update(title);
	unlock_window();
}

void VWindowGUI::create_objects()
{
	in_point = 0;
	out_point = 0;
	lock_window("VWindowGUI::create_objects");
	set_icon(mwindow->theme->get_image("vwindow_icon"));

//printf("VWindowGUI::create_objects %d\n", __LINE__);
	mwindow->theme->get_vwindow_sizes(this);
	mwindow->theme->draw_vwindow_bg(this);
	flash(0);

#ifdef USE_METERS
	meters = new VWindowMeters(mwindow, 
		this,
		mwindow->theme->vmeter_x,
		mwindow->theme->vmeter_y,
		mwindow->theme->vmeter_h);
	meters->create_objects();
#endif

//printf("VWindowGUI::create_objects 1\n");
// Requires meters to build
	edit_panel = new VWindowEditing(mwindow, vwindow);
#ifdef USE_METERS
	edit_panel->set_meters(meters);
#endif
	edit_panel->create_objects();

//printf("VWindowGUI::create_objects 1\n");
#ifdef USE_SLIDER
	add_subwindow(slider = new VWindowSlider(mwindow, 
    	vwindow, 
		this, 
        mwindow->theme->vslider_x, 
        mwindow->theme->vslider_y, 
        mwindow->theme->vslider_w));
#endif
//printf("VWindowGUI::create_objects 1\n");
	transport = new VWindowTransport(mwindow, 
		this, 
		mwindow->theme->vtransport_x, 
		mwindow->theme->vtransport_y);
    transport->create_objects();
#ifdef USE_SLIDER
	transport->set_slider(slider);
#endif

//printf("VWindowGUI::create_objects 1\n");
//	add_subwindow(fps_title = new BC_Title(mwindow->theme->vedit_x, y, ""));
    add_subwindow(clock = new MainClock(mwindow,
		mwindow->theme->vtime_x, 
		mwindow->theme->vtime_y,
		mwindow->theme->vtime_w));

	canvas = new VWindowCanvas(mwindow, this);
	canvas->create_objects(mwindow->edl);



//printf("VWindowGUI::create_objects 1\n");
	add_subwindow(timebar = new VTimeBar(mwindow, 
		this,
		mwindow->theme->vtimebar_x,
		mwindow->theme->vtimebar_y,
		mwindow->theme->vtimebar_w, 
		mwindow->theme->vtimebar_h));
	timebar->create_objects();
//printf("VWindowGUI::create_objects 2\n");


//printf("VWindowGUI::create_objects 1\n");
// 	source = new VWindowSource(mwindow, 
// 		this, 
// 		mwindow->theme->vsource_x,
// 		mwindow->theme->vsource_y);
// 	source->create_objects();	
	update_sources(_("None"));

//printf("VWindowGUI::create_objects 2\n");
	deactivate();
#ifdef USE_SLIDER
	slider->activate();
#endif

	show_window();
	unlock_window();
}

int VWindowGUI::resize_event(int w, int h)
{
	mwindow->session->vwindow_x = get_x();
	mwindow->session->vwindow_y = get_y();
	mwindow->session->vwindow_w = w;
	mwindow->session->vwindow_h = h;

	mwindow->theme->get_vwindow_sizes(this);
	mwindow->theme->draw_vwindow_bg(this);
	flash(0);

//printf("VWindowGUI::resize_event %d %d\n", __LINE__, mwindow->theme->vedit_y);
	edit_panel->reposition_buttons(mwindow->theme->vedit_x, 
		mwindow->theme->vedit_y);



#ifdef USE_SLIDER
	slider->reposition_window(mwindow->theme->vslider_x, 
        mwindow->theme->vslider_y, 
        mwindow->theme->vslider_w);
// Recalibrate pointer motion range
	slider->set_position();
#endif

	timebar->resize_event();
	transport->reposition_buttons(mwindow->theme->vtransport_x, 
		mwindow->theme->vtransport_y);
	clock->reposition_window(mwindow->theme->vtime_x, 
		mwindow->theme->vtime_y,
		mwindow->theme->vtime_w,
		clock->get_h());
	canvas->reposition_window(0,
		mwindow->theme->vcanvas_x, 
		mwindow->theme->vcanvas_y, 
		mwindow->theme->vcanvas_w, 
		mwindow->theme->vcanvas_h);
//printf("VWindowGUI::resize_event %d %d\n", __LINE__, mwindow->theme->vcanvas_x);
// 	source->reposition_window(mwindow->theme->vsource_x,
// 		mwindow->theme->vsource_y);
#ifdef USE_METERS
	meters->reposition_window(mwindow->theme->vmeter_x,
		mwindow->theme->vmeter_y,
		-1,
		mwindow->theme->vmeter_h);
#endif


	BC_WindowBase::resize_event(w, h);
	return 1;
}





int VWindowGUI::translation_event()
{
	mwindow->session->vwindow_x = get_x();
	mwindow->session->vwindow_y = get_y();
	return 0;
}

// hide it but don't stop the thread
int VWindowGUI::close_event()
{
	unlock_window();
    vwindow->playback_engine->interrupt_playback(1);


	lock_window("VWindowGUI::close_event");
	hide_window();
	int total = 0;
	for(int i = 0; i < mwindow->vwindows.size(); i++)
	{

		if(mwindow->vwindows.get(i)->get_edl()) total++;
	}

// last window closed.  Keep source but cancel menu item
    if(total <= 1)
        mwindow->session->show_vwindow = 0;
    else
    {
// other window closed.  Close the source
        vwindow->delete_source(1, 0);
        canvas->clear();
    }
	unlock_window();

	mwindow->gui->lock_window("VWindowGUI::close_event");
	mwindow->gui->mainmenu->show_vwindow->set_checked(mwindow->session->show_vwindow);
	mwindow->gui->unlock_window();

	lock_window("VWindowGUI::close_event");
	mwindow->save_defaults();
	return 1;
}

int VWindowGUI::keypress_event()
{
	int result = 0;
	switch(get_keypress())
	{
		case 'w':
		case 'W':
			close_event();
			result = 1;
			break;
		case 'z':
			mwindow->undo_entry(this);
			break;
		case 'Z':
			mwindow->redo_entry(this);
			break;
		case 'f':
			unlock_window();
			if(mwindow->session->vwindow_fullscreen)
				canvas->stop_fullscreen();
			else
				canvas->start_fullscreen();
			lock_window("VWindowGUI::keypress_event 1");
			break;
		case ESC:
			unlock_window();
			if(mwindow->session->vwindow_fullscreen)
				canvas->stop_fullscreen();
			lock_window("VWindowGUI::keypress_event 2");
			break;
	}
	if(!result) result = transport->keypress_event();
	
	return result;
}

int VWindowGUI::button_press_event()
{
	if(canvas->get_canvas())
		return canvas->button_press_event_base(canvas->get_canvas());
	return 0;
}

int VWindowGUI::cursor_leave_event()
{
	if(canvas->get_canvas())
		return canvas->cursor_leave_event_base(canvas->get_canvas());
	return 0;
}

int VWindowGUI::cursor_enter_event()
{
	if(canvas->get_canvas())
		return canvas->cursor_enter_event_base(canvas->get_canvas());
	return 0;
}

int VWindowGUI::button_release_event()
{
	if(canvas->get_canvas())
		return canvas->button_release_event();
	return 0;
}

int VWindowGUI::cursor_motion_event()
{
	if(canvas->get_canvas())
	{
		canvas->get_canvas()->unhide_cursor();
		return canvas->cursor_motion_event();
	}
	return 0;
}


void VWindowGUI::drag_motion()
{
// Window hidden
	if(get_hidden()) return;
	if(mwindow->session->current_operation != DRAG_ASSET) return;

	int old_status = mwindow->session->vcanvas_highlighted;

	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();
	
	mwindow->session->vcanvas_highlighted = (get_cursor_over_window() &&
		cursor_x >= canvas->x &&
		cursor_x < canvas->x + canvas->w &&
		cursor_y >= canvas->y &&
		cursor_y < canvas->y + canvas->h);


// printf("VWindowGUI::drag_motion 1 %d %d %d %d %d\n", 
// __LINE__, 
// mwindow->session->vcanvas_highlighted,
// get_cursor_over_window(),
// cursor_x,
// cursor_y);


	if(old_status != mwindow->session->vcanvas_highlighted)
		canvas->draw_refresh();
}

int VWindowGUI::drag_stop()
{
	if(get_hidden()) return 0;

	if(mwindow->session->vcanvas_highlighted &&
		mwindow->session->current_operation == DRAG_ASSET)
	{
		mwindow->session->vcanvas_highlighted = 0;
		canvas->draw_refresh();

		Indexable *indexable = mwindow->session->drag_assets->size() ?
			mwindow->session->drag_assets->get(0) :
			0;
		EDL *edl = mwindow->session->drag_clips->size() ?
			mwindow->session->drag_clips->get(0) :
			0;
		
		if(indexable)
			vwindow->change_source(indexable);
		else
		if(edl)
			vwindow->change_source(edl);
		return 1;
	}

	return 0;
}


void VWindowGUI::update_meters()
{
#ifdef USE_METERS
	if(mwindow->edl->session->vwindow_meter != meters->visible)
	{
		meters->set_meters(meters->meter_count, 
			mwindow->edl->session->vwindow_meter);
		mwindow->theme->get_vwindow_sizes(this);
		resize_event(get_w(), get_h());
	}
#endif
}



#ifdef USE_METERS
VWindowMeters::VWindowMeters(MWindow *mwindow, 
	VWindowGUI *gui, 
	int x, 
	int y, 
	int h)
 : MeterPanel(mwindow, 
		gui,
		x,
		y,
		-1,
		h,
		mwindow->edl->session->audio_channels,
		mwindow->edl->session->vwindow_meter,
		0,
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

VWindowMeters::~VWindowMeters()
{
}

int VWindowMeters::change_status_event(int new_status)
{
	mwindow->edl->session->vwindow_meter = new_status;
	gui->update_meters();
	return 1;
}
#endif







VWindowEditing::VWindowEditing(MWindow *mwindow, VWindow *vwindow)
 : EditPanel(mwindow, 
		vwindow->gui, 
		mwindow->theme->vedit_x, 
		mwindow->theme->vedit_y,
		EDITING_ARROW, 
		0,
		0,
		1, 
		1,
		0,
		0,
		1, 
		0,
		0,
		0,
		1,
		1,
		1,
		0,
		0)
{
	this->mwindow = mwindow;
	this->vwindow = vwindow;
}

VWindowEditing::~VWindowEditing()
{
}

void VWindowEditing::copy_selection()
{
	vwindow->copy();
}

void VWindowEditing::splice_selection()
{
	if(vwindow->get_edl())
	{
		mwindow->gui->lock_window("VWindowEditing::splice_selection");
		mwindow->splice(vwindow->get_edl());
		mwindow->gui->unlock_window();
	}
}

void VWindowEditing::overwrite_selection()
{
	if(vwindow->get_edl())
	{
		mwindow->gui->lock_window("VWindowEditing::overwrite_selection");
		mwindow->overwrite(vwindow->get_edl());
		mwindow->gui->unlock_window();
	}
}

void VWindowEditing::toggle_label()
{
	if(vwindow->get_edl())
	{
		EDL *edl = vwindow->get_edl();
		edl->labels->toggle_label(edl->local_session->get_selectionstart(1),
			edl->local_session->get_selectionend(1));
		vwindow->gui->timebar->update(1);
	}
}

void VWindowEditing::prev_label()
{
	if(vwindow->get_edl())
	{
		EDL *edl = vwindow->get_edl();
		vwindow->gui->unlock_window();
		vwindow->playback_engine->interrupt_playback(1);
		vwindow->gui->lock_window("VWindowEditing::prev_label");

		Label *current = edl->labels->prev_label(
			edl->local_session->get_selectionstart(1));
		

		if(!current)
		{
			edl->local_session->set_selectionstart(0);
			edl->local_session->set_selectionend(0);
			vwindow->update_position(CHANGE_NONE, 0, 1, 0);
			vwindow->gui->timebar->update(1);
		}
		else
		{
			edl->local_session->set_selectionstart(current->position);
			edl->local_session->set_selectionend(current->position);
			vwindow->update_position(CHANGE_NONE, 0, 1, 0);
			vwindow->gui->timebar->update(1);
		}
	}
}

void VWindowEditing::next_label()
{
	if(vwindow->get_edl())
	{
		EDL *edl = vwindow->get_edl();
		Label *current = edl->labels->next_label(
			edl->local_session->get_selectionstart(1));
		if(!current)
		{
			vwindow->gui->unlock_window();
			vwindow->playback_engine->interrupt_playback(1);
			vwindow->gui->lock_window("VWindowEditing::next_label 1");

			double position = edl->tracks->total_length();
			edl->local_session->set_selectionstart(position);
			edl->local_session->set_selectionend(position);
			vwindow->update_position(CHANGE_NONE, 0, 1, 0);
			vwindow->gui->timebar->update(1);
		}
		else
		{
			vwindow->gui->unlock_window();
			vwindow->playback_engine->interrupt_playback(1);
			vwindow->gui->lock_window("VWindowEditing::next_label 2");

			edl->local_session->set_selectionstart(current->position);
			edl->local_session->set_selectionend(current->position);
			vwindow->update_position(CHANGE_NONE, 0, 1, 0);
			vwindow->gui->timebar->update(1);
		}
	}
}

void VWindowEditing::set_inpoint()
{
	vwindow->set_inpoint();
}

void VWindowEditing::set_outpoint()
{
	vwindow->set_outpoint();
}

void VWindowEditing::clear_inpoint()
{
	vwindow->clear_inpoint();
}

void VWindowEditing::clear_outpoint()
{
	vwindow->clear_outpoint();
}

void VWindowEditing::to_clip()
{
	if(vwindow->get_edl())
	{
		FileXML file;
		EDL *edl = vwindow->get_edl();
		double start = edl->local_session->get_selectionstart();
		double end = edl->local_session->get_selectionend();

		if(EQUIV(start, end))
		{
			end = edl->tracks->total_length();
			start = 0;
		}



		edl->copy(start, 
			end, 
			1,
			0,
			0,
			&file,
			"",
			1);




		EDL *new_edl = new EDL(mwindow->edl);
		new_edl->create_objects();
		new_edl->load_xml(&file, LOAD_ALL);
		sprintf(new_edl->local_session->clip_title, _("Clip %d"), mwindow->session->clip_number++);
		new_edl->local_session->set_selectionstart(0);
		new_edl->local_session->set_selectionend(0);


//printf("VWindowEditing::to_clip 1 %s\n", edl->local_session->clip_title);

		vwindow->clip_edit->create_clip(new_edl);
	}
}





#ifdef USE_SLIDER

VWindowSlider::VWindowSlider(MWindow *mwindow, 
	VWindow *vwindow, 
	VWindowGUI *gui,
	int x, 
	int y, 
	int pixels)
 : BC_PercentageSlider(x, 
			y,
			0,
			pixels, 
			pixels, 
			0, 
			1, 
			0)
{
	this->mwindow = mwindow;
	this->vwindow = vwindow;
	this->gui = gui;
	set_precision(0.00001);
}

VWindowSlider::~VWindowSlider()
{
}

int VWindowSlider::handle_event()
{
	vwindow->update_position(get_value());

// 	unlock_window();
// 	vwindow->playback_engine->interrupt_playback(1);
// 	lock_window("VWindowSlider::handle_event");
// 
// 	vwindow->update_position(CHANGE_NONE, 1, 0, 0);

	gui->timebar->update(1);
	return 1;
}

void VWindowSlider::set_position()
{
	EDL *edl = vwindow->get_edl();
	if(edl)
	{
		double new_length = edl->tracks->total_playable_length();

// 		if(EQUIV(edl->local_session->preview_end, 0))
// 			edl->local_session->preview_end = new_length;
// 		if(edl->local_session->preview_end > new_length)
// 			edl->local_session->preview_end = new_length;
// 		if(edl->local_session->preview_start > new_length)
// 			edl->local_session->preview_start = 0;

		update(mwindow->theme->vslider_w, 
			edl->local_session->get_selectionstart(1), 
			0,
			new_length);
//			edl->local_session->preview_start, 
//			edl->local_session->preview_end);
	}
}

#endif









VWindowSource::VWindowSource(MWindow *mwindow, VWindowGUI *vwindow, int x, int y)
 : BC_PopupTextBox(vwindow, 
 	&vwindow->sources, 
	"",
	x, 
	y, 
	200, 
	200)
{
	this->mwindow = mwindow;
	this->vwindow = vwindow;
}

VWindowSource::~VWindowSource()
{
}

int VWindowSource::handle_event()
{
	return 1;
}







VWindowTransport::VWindowTransport(MWindow *mwindow, 
	VWindowGUI *gui, 
	int x, 
	int y)
 : PlayTransport(mwindow, 
	gui, 
	x, 
	y)
{
	this->gui = gui;
}

EDL* VWindowTransport::get_edl()
{
	return gui->vwindow->get_edl();
}


void VWindowTransport::goto_start()
{
	gui->unlock_window();
	handle_transport(REWIND, 1.0, 1, 0, 1);
	gui->lock_window("VWindowTransport::goto_start");
	gui->vwindow->goto_start();
}

void VWindowTransport::goto_end()
{
	gui->unlock_window();
	handle_transport(GOTO_END, 1.0, 1, 0, 1);
	gui->lock_window("VWindowTransport::goto_end");
	gui->vwindow->goto_end();
}




VWindowCanvas::VWindowCanvas(MWindow *mwindow, VWindowGUI *gui)
 : Canvas(mwindow,
 	gui,
 	mwindow->theme->vcanvas_x, 
	mwindow->theme->vcanvas_y, 
	mwindow->theme->vcanvas_w, 
	mwindow->theme->vcanvas_h,
	0,
	0,
	0,
	0,
	0,
	1)
{
//printf("VWindowCanvas::VWindowCanvas %d %d\n", __LINE__, mwindow->theme->vcanvas_x);
	this->mwindow = mwindow;
	this->gui = gui;
}

void VWindowCanvas::zoom_resize_window(float percentage)
{
	EDL *edl = gui->vwindow->get_edl();
	if(!edl) edl = mwindow->edl;

	int canvas_w, canvas_h;
	int new_w, new_h;

// Get required canvas size
	calculate_sizes(edl->get_aspect_ratio(), 
		edl->session->output_w, 
		edl->session->output_h, 
		percentage,
		canvas_w,
		canvas_h);

// Estimate window size from current borders
	new_w = canvas_w + (gui->get_w() - mwindow->theme->vcanvas_w);
	new_h = canvas_h + (gui->get_h() - mwindow->theme->vcanvas_h);

	mwindow->session->vwindow_w = new_w;
	mwindow->session->vwindow_h = new_h;

	mwindow->theme->get_vwindow_sizes(gui);

// Estimate again from new borders
	new_w = canvas_w + (mwindow->session->vwindow_w - mwindow->theme->vcanvas_w);
	new_h = canvas_h + (mwindow->session->vwindow_h - mwindow->theme->vcanvas_h);


	gui->resize_window(new_w, new_h);
	gui->resize_event(new_w, new_h);
}

void VWindowCanvas::close_source()
{
    gui->unlock_window();
    gui->vwindow->playback_engine->interrupt_playback(1);
    gui->lock_window("VWindowCanvas::close_source");
	gui->vwindow->delete_source(1, 1);
    gui->canvas->clear();
}


void VWindowCanvas::draw_refresh(int flush)
{
	EDL *edl = gui->vwindow->get_edl();

//	if(!get_canvas()->get_video_on()) get_canvas()->clear_box(0, 0, get_canvas()->get_w(), get_canvas()->get_h());
	if(!get_canvas()->get_video_on())
	{
		if(refresh_frame && edl)
		{
            Canvas::draw_refresh(flush);
        }
        else
        {
            get_canvas()->clear_box(0, 
				0, 
				get_canvas()->get_w(), 
				get_canvas()->get_h());
        }

		draw_overlays();
		get_canvas()->flash(flush);
	}
}

void VWindowCanvas::draw_overlays()
{
	if(mwindow->session->vcanvas_highlighted)
	{
		get_canvas()->set_color(WHITE);
		get_canvas()->set_inverse();
		get_canvas()->draw_rectangle(0, 0, get_canvas()->get_w(), get_canvas()->get_h());
		get_canvas()->draw_rectangle(1, 1, get_canvas()->get_w() - 2, get_canvas()->get_h() - 2);
		get_canvas()->set_opaque();
	}
}

int VWindowCanvas::get_fullscreen()
{
	return mwindow->session->vwindow_fullscreen;
}

void VWindowCanvas::set_fullscreen(int value)
{
	mwindow->session->vwindow_fullscreen = value;
}





































#if 0
void VWindowGUI::update_points()
{
	EDL *edl = vwindow->get_edl();

//printf("VWindowGUI::update_points 1\n");
	if(!edl) return;

//printf("VWindowGUI::update_points 2\n");
	long pixel = (long)((double)edl->local_session->in_point / 
		edl->tracks->total_playable_length() *
		(mwindow->theme->vtimebar_w - 
			2 * 
			mwindow->theme->in_point[0]->get_w())) + 
		mwindow->theme->in_point[0]->get_w();

//printf("VWindowGUI::update_points 3 %d\n", edl->local_session->in_point);
	if(in_point)
	{
//printf("VWindowGUI::update_points 3.1\n");
		if(edl->local_session->in_point >= 0)
		{
//printf("VWindowGUI::update_points 4\n");
			if(edl->local_session->in_point != in_point->position ||
				in_point->pixel != pixel)
			{
				in_point->pixel = pixel;
				in_point->reposition();
			}

//printf("VWindowGUI::update_points 5\n");
			in_point->position = edl->local_session->in_point;

//printf("VWindowGUI::update_points 6\n");
			if(edl->equivalent(in_point->position, edl->local_session->get_selectionstart(1)) ||
				edl->equivalent(in_point->position, edl->local_session->get_selectionend(1)))
				in_point->update(1);
			else
				in_point->update(0);
//printf("VWindowGUI::update_points 7\n");
		}
		else
		{
			delete in_point;
			in_point = 0;
		}
	}
	else
	if(edl->local_session->in_point >= 0)
	{
//printf("VWindowGUI::update_points 8 %p\n", mwindow->theme->in_point);
		add_subwindow(in_point = 
			new VWindowInPoint(mwindow, 
				0, 
				this,
				pixel, 
				edl->local_session->in_point));
//printf("VWindowGUI::update_points 9\n");
	}
//printf("VWindowGUI::update_points 10\n");

	pixel = (long)((double)edl->local_session->out_point / 
		(edl->tracks->total_playable_length() + 0.5) *
		(mwindow->theme->vtimebar_w - 
			2 * 
			mwindow->theme->in_point[0]->get_w())) + 
		mwindow->theme->in_point[0]->get_w() * 
		2;

	if(out_point)
	{
		if(edl->local_session->out_point >= 0 && pixel >= 0 && pixel <= mwindow->theme->vtimebar_w)
		{
			if(edl->local_session->out_point != out_point->position ||
				out_point->pixel != pixel) 
			{
				out_point->pixel = pixel;
				out_point->reposition();
			}
			out_point->position = edl->local_session->out_point;

			if(edl->equivalent(out_point->position, edl->local_session->get_selectionstart(1)) ||
				edl->equivalent(out_point->position, edl->local_session->get_selectionend(1)))
				out_point->update(1);
			else
				out_point->update(0);
		}
		else
		{
			delete out_point;
			out_point = 0;
		}
	}
	else
	if(edl->local_session->out_point >= 0 && pixel >= 0 && pixel <= mwindow->theme->vtimebar_w)
	{
		add_subwindow(out_point = 
			new VWindowOutPoint(mwindow, 
				0, 
				this, 
				pixel, 
				edl->local_session->out_point));
	}
}


void VWindowGUI::update_labels()
{
	EDL *edl = vwindow->get_edl();
	int output = 0;

	for(Label *current = edl->labels->first;
		current;
		current = NEXT)
	{
		long pixel = (long)((current->position - edl->local_session->view_start) / edl->local_session->zoom_sample);

		if(pixel >= 0 && pixel < mwindow->theme->vtimebar_w)
		{
// Create new label
			if(output >= labels.total)
			{
				LabelGUI *new_label;
				add_subwindow(new_label = new LabelGUI(mwindow, this, pixel, 0, current->position));
				labels.append(new_label);
			}
			else
// Reposition old label
			if(labels.values[output]->pixel != pixel)
			{
				labels.values[output]->pixel = pixel;
				labels.values[output]->position = current->position;
				labels.values[output]->reposition();
			}

			if(mwindow->edl->local_session->get_selectionstart(1) <= current->position &&
				mwindow->edl->local_session->get_selectionend(1) >= current->position)
				labels.values[output]->update(1);
			else
			if(labels.values[output]->get_value())
				labels.values[output]->update(0);
			output++;
		}
	}

// Delete excess labels
	while(labels.total > output)
	{
		labels.remove_object();
	}
}



VWindowInPoint::VWindowInPoint(MWindow *mwindow, 
		TimeBar *timebar, 
		VWindowGUI *gui,
		long pixel, 
		double position)
 : InPointGUI(mwindow, 
		timebar, 
		pixel, 
		position)
{
	this->gui = gui;
}

int VWindowInPoint::handle_event()
{
	EDL *edl = gui->vwindow->get_edl();

	if(edl)
	{
		double position = edl->align_to_frame(this->position, 0);

		edl->local_session->set_selectionstart(position);
		edl->local_session->set_selectionend(position);
		gui->timebar->update(1);

// Que the VWindow
		mwindow->vwindow->update_position(CHANGE_NONE, 0, 1, 0);
	}
	return 1;
}



VWindowOutPoint::VWindowOutPoint(MWindow *mwindow, 
		TimeBar *timebar, 
		VWindowGUI *gui,
		long pixel, 
		double position)
 : OutPointGUI(mwindow, 
		timebar, 
		pixel, 
		position)
{
	this->gui = gui;
}

int VWindowOutPoint::handle_event()
{
	EDL *edl = gui->vwindow->get_edl();

	if(edl)
	{
		double position = edl->align_to_frame(this->position, 0);

		edl->local_session->set_selectionstart(position);
		edl->local_session->set_selectionend(position);
		gui->timebar->update(1);

// Que the VWindow
		mwindow->vwindow->update_position(CHANGE_NONE, 0, 1, 0);
	}

	return 1;
}



#endif

