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

#include "cplayback.h"
#include "ctimebar.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "localsession.h"
#include "mainclock.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "playtransport.h"
#include "timelinepane.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "zoombar.h"

// Playback engine for composite window

CPlayback::CPlayback(CWindow *cwindow, Canvas *output)
 : PlaybackEngine()
{
	this->cwindow = cwindow;
    set_canvas(output);
// We update the plugin GUIs
    set_use_gui(1);
}

void CPlayback::init_cursor()
{
	MWindow::instance->gui->set_playing_back(1);
	MWindow::instance->gui->lock_window("CPlayback::init_cursor");
	MWindow::instance->gui->deactivate_timeline();
	MWindow::instance->gui->unlock_window();

// stop the plugin GUIs
    MWindow::instance->stop_plugin_guis();
//	cwindow->playback_cursor->start_playback(tracking_position);
}

void CPlayback::stop_cursor()
{
    MWindowGUI *gui = MWindow::instance->gui;
	gui->set_playing_back(0);
#ifdef USE_METERS
	cwindow->gui->lock_window("Tracking::stop_meters 1");
	cwindow->gui->meters->stop_meters();
	cwindow->gui->unlock_window();
#endif


	gui->lock_window("Tracking::stop_meters 2");
	gui->stop_meters();
	gui->unlock_window();

	MWindow::instance->lwindow->gui->lock_window("Tracking::stop_meters 3");
	MWindow::instance->lwindow->gui->panel->stop_meters();
	MWindow::instance->lwindow->gui->unlock_window();
}


int CPlayback::brender_available(long position)
{
	return MWindow::instance->brender_available(position);
}



#define SCROLL_THRESHOLD .5


int CPlayback::update_scroll(double position)
{
	int updated_scroll = 0;
    MWindowGUI *gui = MWindow::instance->gui;
    EDL *edl = MWindow::instance->edl;

	if(MWindow::preferences->view_follows_playback)
	{
		TimelinePane *pane = gui->get_focused_pane();
		double seconds_per_pixel = (double)edl->local_session->zoom_sample / 
			edl->session->sample_rate;
		double half_canvas = seconds_per_pixel * 
			pane->canvas->get_w() / 2;
		double midpoint = edl->local_session->view_start[pane->number] * 
			seconds_per_pixel +
			half_canvas;

		if(command->get_direction() == PLAY_FORWARD)
		{
			double left_boundary = midpoint + SCROLL_THRESHOLD * half_canvas;
			double right_boundary = midpoint + half_canvas;

			if(position > left_boundary &&
				position < right_boundary)
			{
				int pixels = Units::to_int64((position - left_boundary) * 
						edl->session->sample_rate /
						edl->local_session->zoom_sample);
				if(pixels) 
				{
					MWindow::instance->move_right(pixels);
//printf("CTracking::update_scroll 1 %d\n", pixels);
					updated_scroll = 1;
				}
			}
		}
		else
		{
			double right_boundary = midpoint - SCROLL_THRESHOLD * half_canvas;
			double left_boundary = midpoint - half_canvas;

			if(position < right_boundary &&
				position > left_boundary && 
				edl->local_session->view_start[pane->number] > 0)
			{
				int pixels = Units::to_int64((right_boundary - position) * 
						edl->session->sample_rate /
						edl->local_session->zoom_sample);
				if(pixels) 
				{
					MWindow::instance->move_left(pixels);
					updated_scroll = 1;
				}
			}
		}
	}

	return updated_scroll;
}

void CPlayback::update_tracker(double position)
{
	int updated_scroll = 0;
    MWindowGUI *gui = MWindow::instance->gui;
    EDL *edl = MWindow::instance->edl;

// Update cwindow slider
	cwindow->gui->lock_window("CTracking::update_tracker 1");

#ifdef USE_SLIDER
	cwindow->gui->slider->update(position);
#endif

// This is going to boost the latency but we need to update the timebar
	cwindow->gui->timebar->update(1);
//	cwindow->gui->timebar->flash();
	cwindow->gui->unlock_window();

// Update mwindow cursor
	gui->lock_window("CTracking::update_tracker 2");

	edl->local_session->set_selectionstart(position);
	edl->local_session->set_selectionend(position);

	updated_scroll = update_scroll(position);

	gui->mainclock->update(position);
	gui->update_patchbay();
	gui->update_timebar(0);

	if(!updated_scroll)
	{
		gui->update_cursor();
		gui->zoombar->update();

		for(int i = 0; i < TOTAL_PANES; i++)
			if(gui->pane[i])
				gui->pane[i]->canvas->flash(0);
	}

	gui->flush();
	gui->unlock_window();

// Plugin GUI's hold lock on mwindow->gui here during user interface handlers.
	MWindow::instance->update_plugin_guis();


	update_meters((int64_t)(position * MWindow::instance->edl->session->sample_rate));
}


void CPlayback::update_meters(int64_t position)
{
	double output_levels[MAXCHANNELS];
	int do_audio = get_output_levels(output_levels, position);

	if(do_audio)
	{
		module_levels.remove_all();
		get_module_levels(&module_levels, position);

#ifdef USE_METERS
		MWindow::instance->cwindow->gui->lock_window("Tracking::update_meters 1");
		MWindow::instance->cwindow->gui->meters->update(output_levels);
		MWindow::instance->cwindow->gui->unlock_window();
#endif

		MWindow::instance->lwindow->gui->lock_window("Tracking::update_meters 2");
		MWindow::instance->lwindow->gui->panel->update(output_levels);
		MWindow::instance->lwindow->gui->unlock_window();

		MWindow::instance->gui->lock_window("Tracking::update_meters 3");
		MWindow::instance->gui->update_meters(&module_levels);
		MWindow::instance->gui->unlock_window();

	}
}
