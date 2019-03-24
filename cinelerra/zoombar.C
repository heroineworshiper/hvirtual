
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#include "apatchgui.inc"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "preferences.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "units.h"
#include "vpatchgui.inc"
#include "zoombar.h"




ZoomBar::ZoomBar(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mzoom_x,
 	mwindow->theme->mzoom_y,
	mwindow->theme->mzoom_w,
	mwindow->theme->mzoom_h) 
{
	this->gui = gui;
	this->mwindow = mwindow;
	old_position = 0;
}

ZoomBar::~ZoomBar()
{
	delete sample_zoom;
	delete amp_zoom;
	delete track_zoom;
}

void ZoomBar::create_objects()
{
	int margin = mwindow->theme->widget_border;
	int x = margin;
	int y = get_h() / 2 - 
		mwindow->theme->get_image_set("zoombar_menu", 0)[0]->get_h() / 2;

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	sample_zoom = new SampleZoomPanel(mwindow, this, x, y);
	sample_zoom->set_menu_images(mwindow->theme->get_image_set("zoombar_menu", 0));
	sample_zoom->set_tumbler_images(mwindow->theme->get_image_set("zoombar_tumbler", 0));
	sample_zoom->create_objects();
	x += sample_zoom->get_w();
	amp_zoom = new AmpZoomPanel(mwindow, this, x, y);
	amp_zoom->set_menu_images(mwindow->theme->get_image_set("zoombar_menu", 0));
	amp_zoom->set_tumbler_images(mwindow->theme->get_image_set("zoombar_tumbler", 0));
	amp_zoom->create_objects();
	x += amp_zoom->get_w();
	track_zoom = new TrackZoomPanel(mwindow, this, x, y);
	track_zoom->set_menu_images(mwindow->theme->get_image_set("zoombar_menu", 0));
	track_zoom->set_tumbler_images(mwindow->theme->get_image_set("zoombar_tumbler", 0));
	track_zoom->create_objects();
	x += track_zoom->get_w() + DP(10);

#define DEFAULT_TEXT "000.00 - 000.00"
	add_subwindow(auto_zoom_popup = new AutoZoomPopup(
		mwindow, 
		this, 
		x, 
		y,
		get_text_width(MEDIUMFONT, DEFAULT_TEXT) + 20));
	auto_zoom_popup->create_objects();
	x += auto_zoom_popup->get_w() + margin;
// 	add_subwindow(auto_zoom_text = new BC_Title(
// 		x, 
// 		get_h() / 2 - BC_Title::calculate_h(this, "0") / 2, 
// 		DEFAULT_TEXT));
// 	x += auto_zoom_text->get_w() + margin;
	add_subwindow(auto_zoom = new AutoZoom(mwindow, this, x, y));
	update_autozoom();
	x += auto_zoom->get_w() + margin;

	add_subwindow(from_value = new FromTextBox(mwindow, this, x, 0));
	x += from_value->get_w() + margin;
	add_subwindow(length_value = new LengthTextBox(mwindow, this, x, 0));
	x += length_value->get_w() + margin;
	add_subwindow(to_value = new ToTextBox(mwindow, this, x, 0));
	x += to_value->get_w() + margin;

	update_formatting(from_value);
	update_formatting(length_value);
	update_formatting(to_value);

	add_subwindow(playback_value = new BC_Title(x, DP(100), _("--"), MEDIUMFONT, RED));

	add_subwindow(zoom_value = new BC_Title(x, DP(100), _("--"), MEDIUMFONT, BLACK));
	update();
}


void ZoomBar::update_formatting(BC_TextBox *dst)
{
	dst->set_separators(
		Units::format_to_separators(mwindow->edl->session->time_format));
}

void ZoomBar::resize_event()
{
	hide_window(0);
	reposition_window(mwindow->theme->mzoom_x,
		mwindow->theme->mzoom_y,
		mwindow->theme->mzoom_w,
		mwindow->theme->mzoom_h);

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
// 	int x = 3, y = 1;
// 	sample_zoom->reposition_window(x, y);
// 	x += sample_zoom->get_w();
// 	amp_zoom->reposition_window(x, y);
// 	x += amp_zoom->get_w();
// 	track_zoom->reposition_window(x, y);
	flash(0);
	show_window(0);
}

void ZoomBar::redraw_time_dependancies()
{
// Recalculate sample zoom menu
	sample_zoom->update_menu();
	sample_zoom->update(mwindow->edl->local_session->zoom_sample);
	update_formatting(from_value);
	update_formatting(length_value);
	update_formatting(to_value);
	update_autozoom();
	update_clocks();
}

int ZoomBar::draw()
{
	update();
	return 0;
}

void ZoomBar::update_autozoom()
{
	char string[BCTEXTLEN];
	sprintf(string, "%0.02f - %0.02f\n", 
		mwindow->edl->local_session->automation_min, 
		mwindow->edl->local_session->automation_max);
//	auto_zoom_text->update(string);
	auto_zoom_popup->set_text(string);
}

int ZoomBar::update()
{
	sample_zoom->update(mwindow->edl->local_session->zoom_sample);
	amp_zoom->update(mwindow->edl->local_session->zoom_y);
	track_zoom->update(mwindow->edl->local_session->zoom_track);
	update_autozoom();
	update_clocks();
	return 0;
}

int ZoomBar::update_clocks()
{
	from_value->update_position(mwindow->edl->local_session->get_selectionstart(1));
	length_value->update_position(mwindow->edl->local_session->get_selectionend(1) - 
		mwindow->edl->local_session->get_selectionstart(1));
	to_value->update_position(mwindow->edl->local_session->get_selectionend(1));
	return 0;
}

int ZoomBar::update_playback(int64_t new_position)
{
	if(new_position != old_position)
	{
		Units::totext(string, 
				new_position, 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
		playback_value->update(string);
		old_position = new_position;
	}
	return 0;
}

int ZoomBar::resize_event(int w, int h)
{
// don't change anything but y and width
	reposition_window(0, h - this->get_h(), w, this->get_h());
	return 0;
}


// Values for which_one
#define SET_FROM 1
#define SET_LENGTH 2
#define SET_TO 3


int ZoomBar::set_selection(int which_one)
{
	double start_position = mwindow->edl->local_session->get_selectionstart(1);
	double end_position = mwindow->edl->local_session->get_selectionend(1);
	double length = end_position - start_position;

// Fix bogus results

	switch(which_one)
	{
		case SET_LENGTH:
			start_position = Units::text_to_seconds(from_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			length = Units::text_to_seconds(length_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			end_position = start_position + length;

			if(end_position < start_position)
			{
				start_position = end_position;
				mwindow->edl->local_session->set_selectionend(
					mwindow->edl->local_session->get_selectionstart(1));
			}
			break;

		case SET_FROM:
			start_position = Units::text_to_seconds(from_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			end_position = Units::text_to_seconds(to_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);

			if(end_position < start_position)
			{
				end_position = start_position;
				mwindow->edl->local_session->set_selectionend(
					mwindow->edl->local_session->get_selectionstart(1));
			}
			break;

		case SET_TO:
			start_position = Units::text_to_seconds(from_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			end_position = Units::text_to_seconds(to_value->get_text(), 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);

			if(end_position < start_position)
			{
				start_position = end_position;
				mwindow->edl->local_session->set_selectionend(
					mwindow->edl->local_session->get_selectionstart(1));
			}
			break;
	}

	mwindow->edl->local_session->set_selectionstart(
		mwindow->edl->align_to_frame(start_position, 1));
	mwindow->edl->local_session->set_selectionend(
		mwindow->edl->align_to_frame(end_position, 1));


	mwindow->gui->update_timebar_highlights();
	mwindow->gui->hide_cursor(1);
	mwindow->gui->show_cursor(1);
	update();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->gui->flash_canvas(1);

	return 0;
}












SampleZoomPanel::SampleZoomPanel(MWindow *mwindow, 
	ZoomBar *zoombar, 
	int x, 
	int y)
 : ZoomPanel(mwindow, 
 	zoombar, 
	mwindow->edl->local_session->zoom_sample, 
	x, 
	y, 
	DP(110), 
	MIN_ZOOM_TIME, 
	MAX_ZOOM_TIME, 
//	ZOOM_LONG)
	ZOOM_TIME)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}
int SampleZoomPanel::handle_event()
{
	mwindow->zoom_sample((int64_t)get_value());
	return 1;
}











AmpZoomPanel::AmpZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : ZoomPanel(mwindow, 
 	zoombar, 
	mwindow->edl->local_session->zoom_y, 
	x, 
	y, 
	DP(80),
	MIN_AMP_ZOOM, 
	MAX_AMP_ZOOM, 
	ZOOM_LONG)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}
int AmpZoomPanel::handle_event()
{
	mwindow->zoom_amp((int64_t)get_value());
	return 1;
}

TrackZoomPanel::TrackZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : ZoomPanel(mwindow, 
 	zoombar, 
	mwindow->edl->local_session->zoom_track, 
	x, 
	y, 
	DP(70),
	MIN_TRACK_ZOOM, 
	MAX_TRACK_ZOOM, 
	ZOOM_LONG)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}
int TrackZoomPanel::handle_event()
{
	mwindow->zoom_track((int64_t)get_value());
	zoombar->amp_zoom->update(mwindow->edl->local_session->zoom_y);
	return 1;
}




AutoZoom::AutoZoom(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_Tumbler(x,
 	y,
 	mwindow->theme->get_image_set("zoombar_tumbler"))
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int AutoZoom::handle_up_event()
{
	mwindow->expand_autos();
	return 1;
}

int AutoZoom::handle_down_event()
{
	mwindow->shrink_autos();
	return 1;
}







AutoZoomPopup::AutoZoomPopup(MWindow *mwindow, 
	ZoomBar *zoombar, 
	int x, 
	int y,
	int w)
 : BC_PopupMenu(x,
 	y,
	w,
	"",
	1,
	mwindow->theme->get_image_set("zoombar_menu", 0))
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

void AutoZoomPopup::create_objects()
{
	char string[BCTEXTLEN];

	sprintf(string, "-1 - 1");
	add_item(new BC_MenuItem(string));

	sprintf(string, "0 - %d", MAX_VIDEO_FADE);
	add_item(new BC_MenuItem(string));

	sprintf(string, "%d - %d", INFINITYGAIN, MAX_AUDIO_FADE);
	add_item(new BC_MenuItem(string));
}

int AutoZoomPopup::handle_event()
{
	if(!strcmp(get_text(), get_item(0)->get_text()))
		mwindow->zoom_autos(-1, 1);
	else
	if(!strcmp(get_text(), get_item(1)->get_text()))
		mwindow->zoom_autos(0, MAX_VIDEO_FADE);
	else
	if(!strcmp(get_text(), get_item(2)->get_text()))
		mwindow->zoom_autos(INFINITYGAIN, MAX_AUDIO_FADE);
	return 1;
}










FromTextBox::FromTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, DP(90), 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int FromTextBox::handle_event()
{
	if(get_keypress() == 13)
	{
		zoombar->set_selection(SET_FROM);
		return 1;
	}
	return 0;
}

int FromTextBox::update_position(double new_position)
{
	Units::totext(string, 
		new_position, 
		mwindow->edl->session->time_format, 
		mwindow->edl->session->sample_rate, 
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
//printf("FromTextBox::update_position %f %s\n", new_position, string);
	update(string);
	return 0;
}






LengthTextBox::LengthTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, DP(90), 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int LengthTextBox::handle_event()
{
	if(get_keypress() == 13)
	{
		zoombar->set_selection(SET_LENGTH);
		return 1;
	}
	return 0;
}

int LengthTextBox::update_position(double new_position)
{
	Units::totext(string, 
		new_position, 
		mwindow->edl->session->time_format, 
		mwindow->edl->session->sample_rate, 
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
	update(string);
	return 0;
}





ToTextBox::ToTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, DP(90), 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}

int ToTextBox::handle_event()
{
	if(get_keypress() == 13)
	{
		zoombar->set_selection(SET_TO);
		return 1;
	}
	return 0;
}

int ToTextBox::update_position(double new_position)
{
	Units::totext(string, 
		new_position, 
		mwindow->edl->session->time_format, 
		mwindow->edl->session->sample_rate, 
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
	update(string);
	return 0;
}
