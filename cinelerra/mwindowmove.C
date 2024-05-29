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

#include "clip.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "labels.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "plugin.h"
#include "resourcethread.h"
#include "samplescroll.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "zoombar.h"


void MWindow::update_plugins()
{
// Show plugins which are visible and hide plugins which aren't
// Update plugin pointers in plugin servers
}


int MWindow::expand_sample()
{
	if(gui)
	{
		if(edl->local_session->zoom_sample < 0x100000)
		{
            int64_t prev_zoom_sample = edl->local_session->zoom_sample;
			edl->local_session->zoom_sample *= 2;
			gui->zoombar->sample_zoom->update(edl->local_session->zoom_sample);
			zoom_sample(prev_zoom_sample, edl->local_session->zoom_sample);
		}
	}
	return 0;
}

int MWindow::zoom_in_sample()
{
	if(gui)
	{
		if(edl->local_session->zoom_sample > 1)
		{
            int64_t prev_zoom_sample = edl->local_session->zoom_sample;
			edl->local_session->zoom_sample /= 2;
			gui->zoombar->sample_zoom->update(edl->local_session->zoom_sample);
			zoom_sample(prev_zoom_sample, edl->local_session->zoom_sample);
		}
	}
	return 0;
}

int MWindow::zoom_sample(int64_t prev_zoom_sample, int64_t zoom_sample)
{
	CLIP(zoom_sample, 1, 0x100000);
	TimelinePane *focused_pane = gui->get_focused_pane();

	edl->local_session->zoom_sample = zoom_sample;
// center the active pane on the cursor
	find_cursor();

// center the inactive panes on their center sample
    int top_pane = -1;
    int bottom_pane = -1;
	if(focused_pane->number == TOP_LEFT_PANE ||
		focused_pane->number == BOTTOM_LEFT_PANE)
	{
        top_pane = TOP_RIGHT_PANE;
        bottom_pane = BOTTOM_RIGHT_PANE;
    }
    else
    {
        top_pane = TOP_LEFT_PANE;
        bottom_pane = BOTTOM_LEFT_PANE;
    }

//printf("MWindow::zoom_sample %d %p %p\n", __LINE__, gui->pane[top_pane], gui->pane[bottom_pane]);

    TimelinePane *pane = gui->pane[top_pane];
    if(pane)
    {
        int64_t prev_view_start = edl->local_session->view_start[top_pane];
        int w = pane->canvas->get_w();
        int64_t prev_center_sample = 
            prev_zoom_sample * (prev_view_start + w / 2);
        int64_t view_start = (prev_center_sample - 
            zoom_sample * w / 2) / 
            zoom_sample;

        if(view_start < 0)
        {
            view_start = 0;
        }

        edl->local_session->view_start[top_pane] =
		    edl->local_session->view_start[bottom_pane] =
		    view_start;
// printf("MWindow::zoom_sample %d prev_view_start=%ld prev_zoom_sample=%ld w=%d prev_center_sample=%ld\n", 
// __LINE__, 
// prev_view_start,
// prev_zoom_sample,
// w,
// prev_center_sample);
    }


	samplemovement(edl->local_session->view_start[focused_pane->number], 
        focused_pane->number);


	return 0;
}

void MWindow::find_cursor()
{
	TimelinePane *pane = gui->get_focused_pane();
	edl->local_session->view_start[pane->number] = 
		Units::round((edl->local_session->get_selectionend(1) + 
		    edl->local_session->get_selectionstart(1)) / 
		    2 *
		    edl->session->sample_rate /
		    edl->local_session->zoom_sample - 
		    (double)pane->canvas->get_w() / 
		    2);

	if(edl->local_session->view_start[pane->number] < 0) 
		edl->local_session->view_start[pane->number] = 0;
}


void MWindow::fit_selection()
{
    int64_t prev_zoom_sample = edl->local_session->zoom_sample;
	if(EQUIV(edl->local_session->get_selectionstart(1),
		edl->local_session->get_selectionend(1)))
	{
		double total_samples = edl->tracks->total_length() * 
			edl->session->sample_rate;
		TimelinePane *pane = gui->get_focused_pane();
		for(edl->local_session->zoom_sample = 1; 
			pane->canvas->get_w() * edl->local_session->zoom_sample < total_samples; 
			edl->local_session->zoom_sample *= 2)
			;
	}
	else
	{
		double total_samples = (edl->local_session->get_selectionend(1) - 
			edl->local_session->get_selectionstart(1)) * 
			edl->session->sample_rate;
		TimelinePane *pane = gui->get_focused_pane();
		for(edl->local_session->zoom_sample = 1; 
			pane->canvas->get_w() * edl->local_session->zoom_sample < total_samples; 
			edl->local_session->zoom_sample *= 2)
			;
	}

	edl->local_session->zoom_sample = MIN(0x100000, 
		edl->local_session->zoom_sample);
	zoom_sample(prev_zoom_sample, edl->local_session->zoom_sample);
}


void MWindow::fit_autos()
{
	float min = 0, max = 0;
	double start, end;

// Test all autos
	if(EQUIV(edl->local_session->get_selectionstart(1),
		edl->local_session->get_selectionend(1)))
	{
		start = 0;
		end = edl->tracks->total_length();
	}
	else
// Test autos in highlighting only
	{
		start = edl->local_session->get_selectionstart(1);
		end = edl->local_session->get_selectionend(1);
	}

// Adjust min and max
	edl->tracks->get_automation_extents(&min, &max, start, end);
//printf("MWindow::fit_autos %f %f\n", min, max);

// Pad
	float range = max - min;
// No automation visible
	if(range < 0.001)
	{
		min -= 1;
		max += 1;
	}
	float pad = range * 0.33;
	min -= pad;
	max += pad;
	edl->local_session->automation_min = min;
	edl->local_session->automation_max = max;

// Show range in zoombar
	gui->zoombar->update();

// Draw
	gui->draw_overlays(1);
}


void MWindow::expand_autos()
{
	float range = edl->local_session->automation_max - 
		edl->local_session->automation_min;
	float center = range / 2 + 
		edl->local_session->automation_min;
	if(EQUIV(range, 0)) range = 0.002;
	edl->local_session->automation_min = center - range;
	edl->local_session->automation_max = center + range;
	gui->zoombar->update_autozoom();
	gui->draw_overlays(1);
}

void MWindow::shrink_autos()
{
	float range = edl->local_session->automation_max - 
		edl->local_session->automation_min;
	float center = range / 2 + 
		edl->local_session->automation_min;
	float new_range = range / 4;
	edl->local_session->automation_min = center - new_range;
	edl->local_session->automation_max = center + new_range;
	gui->zoombar->update_autozoom();
	gui->draw_overlays(1);
}


void MWindow::zoom_autos(float min, float max)
{
	edl->local_session->automation_min = min;
	edl->local_session->automation_max = max;
	gui->zoombar->update_autozoom();
	gui->draw_overlays(1);
}


void MWindow::zoom_amp(int64_t zoom_amp)
{
	edl->local_session->zoom_y = zoom_amp;
	gui->draw_canvas(0, 0);
	gui->flash_canvas(0);
	gui->update_patchbay();
	gui->flush();
}

void MWindow::zoom_track(int64_t zoom_track)
{
// scale waveforms
	edl->local_session->zoom_y = (int64_t)((float)edl->local_session->zoom_y * 
		zoom_track / 
		edl->local_session->zoom_track);
	CLAMP(edl->local_session->zoom_y, MIN_AMP_ZOOM, MAX_AMP_ZOOM);

// scale tracks
	double scale = (double)zoom_track / edl->local_session->zoom_track;
	edl->local_session->zoom_track = zoom_track;

// shift row position
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		edl->local_session->track_start[i] *= scale;
	}
	edl->tracks->update_y_pixels(theme);
	gui->draw_trackmovement();
//printf("MWindow::zoom_track %d %d\n", edl->local_session->zoom_y, edl->local_session->zoom_track);
}

void MWindow::trackmovement(int offset, int pane_number)
{
	edl->local_session->track_start[pane_number] += offset;
	if(edl->local_session->track_start[pane_number] < 0) 
		edl->local_session->track_start[pane_number] = 0;

	if(pane_number == TOP_RIGHT_PANE ||
		pane_number == TOP_LEFT_PANE)
	{
		edl->local_session->track_start[TOP_LEFT_PANE] = 
			edl->local_session->track_start[TOP_RIGHT_PANE] = 
			edl->local_session->track_start[pane_number];
	}
	else
	if(pane_number == BOTTOM_RIGHT_PANE ||
		pane_number == BOTTOM_LEFT_PANE)
	{
		edl->local_session->track_start[BOTTOM_LEFT_PANE] = 
			edl->local_session->track_start[BOTTOM_RIGHT_PANE] = 
			edl->local_session->track_start[pane_number];
	}


	edl->tracks->update_y_pixels(theme);
	gui->draw_trackmovement();
}

void MWindow::move_up(int64_t distance)
{
	TimelinePane *pane = gui->get_focused_pane();
	if(distance == 0) distance = edl->local_session->zoom_track;

	trackmovement(-distance, pane->number);
}

void MWindow::move_down(int64_t distance)
{
	TimelinePane *pane = gui->get_focused_pane();
	if(distance == 0) distance = edl->local_session->zoom_track;

	trackmovement(distance, pane->number);
}

int MWindow::goto_end()
{
	TimelinePane *pane = gui->get_focused_pane();
	int64_t old_view_start = edl->local_session->view_start[pane->number];
	
	if(edl->tracks->total_length() > (double)pane->canvas->get_w() * 
		edl->local_session->zoom_sample / 
		edl->session->sample_rate)
	{
		edl->local_session->view_start[pane->number] = 
			Units::round(edl->tracks->total_length() * 
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				pane->canvas->get_w() / 
				2);
	}
	else
	{
		edl->local_session->view_start[pane->number] = 0;
	}

	if(gui->shift_down())
	{
		edl->local_session->set_selectionend(edl->tracks->total_length());
	}
	else
	{
		edl->local_session->set_selectionstart(edl->tracks->total_length());
		edl->local_session->set_selectionend(edl->tracks->total_length());
	}

	if(edl->local_session->view_start[pane->number] != old_view_start)
	{
		samplemovement(edl->local_session->view_start[pane->number], pane->number);
		gui->draw_samplemovement();
	}

	update_plugin_guis();
	
	gui->update_patchbay();
	gui->update_cursor();
	gui->activate_timeline();
	gui->zoombar->update();
	gui->update_timebar(1);
	cwindow->update(1, 0, 0, 0, 1);
	return 0;
}

int MWindow::goto_start()
{
	TimelinePane *pane = gui->get_focused_pane();
	int64_t old_view_start = edl->local_session->view_start[pane->number];

	edl->local_session->view_start[pane->number] = 0;
	if(gui->shift_down())
	{
		edl->local_session->set_selectionstart(0);
	}
	else
	{
		edl->local_session->set_selectionstart(0);
		edl->local_session->set_selectionend(0);
	}

	if(edl->local_session->view_start[pane->number] != old_view_start)
	{
		samplemovement(edl->local_session->view_start[pane->number], pane->number);
		gui->draw_samplemovement();
	}

	update_plugin_guis();
	gui->update_patchbay();
	gui->update_cursor();
	gui->activate_timeline();
	gui->zoombar->update();
	gui->update_timebar(1);
	cwindow->update(1, 0, 0, 0, 1);
	return 0;
}


int MWindow::samplemovement(int64_t view_start, int pane_number)
{
	edl->local_session->view_start[pane_number] = view_start;
	if(edl->local_session->view_start[pane_number] < 0) 
		edl->local_session->view_start[pane_number] = 0;

// copy the view start to the neighboring panes
	if(pane_number == TOP_LEFT_PANE ||
		pane_number == BOTTOM_LEFT_PANE)
	{
		edl->local_session->view_start[TOP_LEFT_PANE] =
			edl->local_session->view_start[BOTTOM_LEFT_PANE] =
			edl->local_session->view_start[pane_number];
	}
	else
	{
		edl->local_session->view_start[TOP_RIGHT_PANE] =
			edl->local_session->view_start[BOTTOM_RIGHT_PANE] =
			edl->local_session->view_start[pane_number];
	}
	
	gui->draw_samplemovement();

	return 0;
}

int MWindow::move_left(int64_t distance)
{
	TimelinePane *pane = gui->get_focused_pane();
	if(!distance) 
		distance = pane->canvas->get_w() / 
			10;
	edl->local_session->view_start[pane->number] -= distance;
	samplemovement(edl->local_session->view_start[pane->number],
		pane->number);
	return 0;
}

int MWindow::move_right(int64_t distance)
{
	TimelinePane *pane = gui->get_focused_pane();
	if(!distance) 
		distance = pane->canvas->get_w() / 
			10;
	edl->local_session->view_start[pane->number] += distance;
	samplemovement(edl->local_session->view_start[pane->number],
		pane->number);
	return 0;
}

void MWindow::select_all()
{
	edl->local_session->set_selectionstart(0);
	edl->local_session->set_selectionend(edl->tracks->total_length());
	gui->update(0, 1, 1, 1, 0, 1, 0);
	gui->activate_timeline();
	cwindow->update(1, 0, 0, 0, 1);
	update_plugin_guis();
}

int MWindow::next_label(int shift_down)
{
	Label *current;
	Labels *labels = edl->labels;

// Test for label under cursor position
	for(current = labels->first; 
		current && !edl->equivalent(current->position, 
			edl->local_session->get_selectionend(1)); 
		current = NEXT)
		;

// Test for label before cursor position
	if(!current)
		for(current = labels->last;
			current && current->position > edl->local_session->get_selectionend(1);
			current = PREVIOUS)
			;

// Test for label after cursor position
	if(!current)
		current = labels->first;
	else
// Get next label
		current = NEXT;

	if(current)
	{

		edl->local_session->set_selectionend(current->position);
		if(!shift_down) 
			edl->local_session->set_selectionstart(
				edl->local_session->get_selectionend(1));

		update_plugin_guis();
		TimelinePane *pane = gui->get_focused_pane();
		if(edl->local_session->get_selectionend(1) >= 
			(double)edl->local_session->view_start[pane->number] *
				edl->local_session->zoom_sample /
				edl->session->sample_rate + 
				pane->canvas->time_visible() ||
			edl->local_session->get_selectionend(1) < (double)edl->local_session->view_start[pane->number] *
				edl->local_session->zoom_sample /
				edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->get_selectionend(1) *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				pane->canvas->get_w() / 
				2),
				pane->number);
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
		{
			gui->update_patchbay();
			gui->update_timebar(0);
			gui->hide_cursor(0);
			gui->draw_cursor(0);
			gui->zoombar->update();
			gui->flash_canvas(1);
			cwindow->update(1, 0, 0, 0, 1);
		}
	}
	else
	{
		goto_end();
	}
	return 0;
}

int MWindow::prev_label(int shift_down)
{
	Label *current;
	Labels *labels = edl->labels;

// Test for label under cursor position
	for(current = labels->first; 
		current && !edl->equivalent(current->position, 
			edl->local_session->get_selectionstart(1)); 
		current = NEXT)
		;

// Test for label after cursor position
	if(!current)
		for(current = labels->first;
			current && 
				current->position < edl->local_session->get_selectionstart(1);
			current = NEXT)
			;

// Test for label before cursor position
	if(!current) 
		current = labels->last;
	else
// Get previous label
		current = PREVIOUS;

	if(current)
	{
		edl->local_session->set_selectionstart(current->position);
		if(!shift_down) 
			edl->local_session->set_selectionend(edl->local_session->get_selectionstart(1));

		update_plugin_guis();
// Scroll the display
		TimelinePane *pane = gui->get_focused_pane();
		if(edl->local_session->get_selectionstart(1) >= edl->local_session->view_start[pane->number] *
			edl->local_session->zoom_sample /
			edl->session->sample_rate + 
			pane->canvas->time_visible() 
		||
			edl->local_session->get_selectionstart(1) < edl->local_session->view_start[pane->number] *
			edl->local_session->zoom_sample /
			edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->get_selectionstart(1) *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				pane->canvas->get_w() / 
				2),
				pane->number);
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
// Don't scroll the display
		{
			gui->update_patchbay();
			gui->update_timebar(0);
			gui->hide_cursor(0);
			gui->draw_cursor(0);
			gui->zoombar->update();
			gui->flash_canvas(1);
			cwindow->update(1, 0, 0, 0, 1);
		}
	}
	else
	{
		goto_start();
	}
	return 0;
}






int MWindow::next_edit_handle(int shift_down)
{
	double position = edl->local_session->get_selectionend(1);
	Units::fix_double(&position);
	double new_position = INFINITY;
// Test for edit handles after cursor position
	for (Track *track = edl->tracks->first; track; track = track->next)
	{
		if (track->record)
		{
			for (Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				double edit_end = track->from_units(edit->startproject + edit->length);
				Units::fix_double(&edit_end);
				if (edit_end > position && edit_end < new_position)
					new_position = edit_end;
			}
		}
	}

	if(new_position != INFINITY)
	{

		edl->local_session->set_selectionend(new_position);
//printf("MWindow::next_edit_handle %d\n", shift_down);
		if(!shift_down) 
			edl->local_session->set_selectionstart(
				edl->local_session->get_selectionend(1));

		update_plugin_guis();
		TimelinePane *pane = gui->get_focused_pane();
		if(edl->local_session->get_selectionend(1) >= 
			(double)edl->local_session->view_start[pane->number] *
			edl->local_session->zoom_sample /
			edl->session->sample_rate + 
			pane->canvas->time_visible() ||
			edl->local_session->get_selectionend(1) < (double)edl->local_session->view_start[pane->number] *
			edl->local_session->zoom_sample /
			edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->get_selectionend(1) *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				pane->canvas->get_w() / 
				2),
				pane->number);
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
		{
			gui->update_patchbay();
			gui->update_timebar(0);
			gui->hide_cursor(0);
			gui->draw_cursor(0);
			gui->zoombar->update();
			gui->flash_canvas(1);
			cwindow->update(1, 0, 0, 0, 1);
		}
	}
	else
	{
		goto_end();
	}
	return 0;
}

int MWindow::prev_edit_handle(int shift_down)
{
	double position = edl->local_session->get_selectionstart(1);
	double new_position = -1;
	Units::fix_double(&position);
// Test for edit handles before cursor position
	for (Track *track = edl->tracks->first; track; track = track->next)
	{
		if (track->record)
		{
			for (Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				double edit_end = track->from_units(edit->startproject);
				Units::fix_double(&edit_end);
				if (edit_end < position && edit_end > new_position)
					new_position = edit_end;
			}
		}
	}

	if(new_position != -1)
	{

		edl->local_session->set_selectionstart(new_position);
//printf("MWindow::next_edit_handle %d\n", shift_down);
		if(!shift_down) 
			edl->local_session->set_selectionend(edl->local_session->get_selectionstart(1));

		update_plugin_guis();
// Scroll the display
		TimelinePane *pane = gui->get_focused_pane();
		if(edl->local_session->get_selectionstart(1) >= edl->local_session->view_start[pane->number] *
			edl->local_session->zoom_sample /
			edl->session->sample_rate + 
			pane->canvas->time_visible() 
		||
			edl->local_session->get_selectionstart(1) < edl->local_session->view_start[pane->number] *
			edl->local_session->zoom_sample /
			edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->get_selectionstart(1) *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				pane->canvas->get_w() / 
				2),
				pane->number);
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
// Don't scroll the display
		{
			gui->update_patchbay();
			gui->update_timebar(0);
			gui->hide_cursor(0);
			gui->draw_cursor(0);
			gui->zoombar->update();
			gui->flash_canvas(1);
			cwindow->update(1, 0, 0, 0, 1);
		}
	}
	else
	{
		goto_start();
	}
	return 0;
}








int MWindow::expand_y()
{
	int result = edl->local_session->zoom_y * 2;
	result = MIN(result, MAX_AMP_ZOOM);
	zoom_amp(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::zoom_in_y()
{
	int result = edl->local_session->zoom_y / 2;
	result = MAX(result, MIN_AMP_ZOOM);
	zoom_amp(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::expand_t()
{
	int result = edl->local_session->zoom_track * 2;
	result = MIN(result, MAX_TRACK_ZOOM);
	zoom_track(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::zoom_in_t()
{
	int result = edl->local_session->zoom_track / 2;
	result = MAX(result, MIN_TRACK_ZOOM);
	zoom_track(result);
	gui->zoombar->update();
	return 0;
}

void MWindow::split_x()
{
	gui->resource_thread->stop_draw(1);

	if(gui->pane[TOP_RIGHT_PANE])
	{
		gui->delete_x_pane(theme->mcanvas_w);
		edl->local_session->x_pane = -1;
	}
	else
	{
		gui->create_x_pane(theme->mcanvas_w / 2);
		edl->local_session->x_pane = theme->mcanvas_w / 2;
	}

	gui->mainmenu->update_toggles(0);
	gui->update_pane_dividers();
	gui->update_cursor();
// required to get new widgets to appear
	gui->show_window();
	
	gui->resource_thread->start_draw();
}

void MWindow::split_y()
{
	gui->resource_thread->stop_draw(1);
	if(gui->pane[BOTTOM_LEFT_PANE])
	{
		gui->delete_y_pane(theme->mcanvas_h);
		edl->local_session->y_pane = -1;
	}
	else
	{
		gui->create_y_pane(theme->mcanvas_h / 2);
		edl->local_session->y_pane = theme->mcanvas_h / 2;
	}

	gui->mainmenu->update_toggles(0);
	gui->update_pane_dividers();
	gui->update_cursor();
// required to get new widgets to appear
	gui->show_window();
	gui->resource_thread->start_draw();
}



