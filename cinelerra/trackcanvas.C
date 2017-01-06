
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
#include "asset.h"
#include "autoconf.h"
#include "automation.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "clip.h"
#include "colors.h"
#include "cplayback.h"
#include "cursors.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "edithandles.h"
#include "editpopup.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "indexstate.h"
#include "intauto.h"
#include "intautos.h"
#include "keyframe.h"
#include "keyframepopup.h"
#include "keyframes.h"
#include "keys.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mainsession.h"
#include "mainundo.h"
#include "maskautos.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "panautos.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "plugin.h"
#include "pluginpopup.h"
#include "pluginserver.h"
#include "pluginset.h"
#include "plugintoggles.h"
#include "preferences.h"
#include "resourcepixmap.h"
#include "resourcethread.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracking.h"
#include "tracks.h"
#include "transition.h"
#include "transitionhandles.h"
#include "transitionpopup.h"
#include "transportque.h"
#include "vframe.h"
#include "vpatchgui.inc"
#include "zoombar.h"

#include <string.h>

//#define PIXMAP_AGE -5
#define PIXMAP_AGE -32

TrackCanvas::TrackCanvas(MWindow *mwindow, 
	TimelinePane *pane, 
	int x, 
	int y, 
	int w, 
	int h)
 : BC_SubWindow(x,
 	y,
	w,
	h)
{
	this->mwindow = mwindow;
	this->gui = mwindow->gui;
	this->pane = pane;
	
	selection_midpoint = 0;
	drag_scroll = 0;
	active = 0;
	temp_picon = 0;
	resource_timer = new Timer;
	hourglass_enabled = 0;
	timebar_position = -1;
}

TrackCanvas::~TrackCanvas()
{
//	delete transition_handles;
	delete edit_handles;
	delete keyframe_pixmap;
	delete camerakeyframe_pixmap;
	delete modekeyframe_pixmap;
	delete pankeyframe_pixmap;
	delete projectorkeyframe_pixmap;
	delete maskkeyframe_pixmap;
	delete background_pixmap;
	if(temp_picon) delete temp_picon;
	delete resource_timer;
}

void TrackCanvas::create_objects()
{
	background_pixmap = new BC_Pixmap(this, get_w(), get_h());
//	transition_handles = new TransitionHandles(mwindow, this);
	edit_handles = new EditHandles(mwindow, this);
	keyframe_pixmap = new BC_Pixmap(this, mwindow->theme->keyframe_data, PIXMAP_ALPHA);
	camerakeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->camerakeyframe_data, PIXMAP_ALPHA);
	modekeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->modekeyframe_data, PIXMAP_ALPHA);
	pankeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->pankeyframe_data, PIXMAP_ALPHA);
	projectorkeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->projectorkeyframe_data, PIXMAP_ALPHA);
	maskkeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->maskkeyframe_data, PIXMAP_ALPHA);
	draw();
	update_cursor(0);
	flash(0);
}

void TrackCanvas::resize_event()
{
//printf("TrackCanvas::resize_event 1\n");
	draw(0, 0);
	flash(0);
//printf("TrackCanvas::resize_event 2\n");
}

int TrackCanvas::keypress_event()
{
	int result = 0;


	return result;
}


int TrackCanvas::drag_stop_event()
{
	return gui->drag_stop();
}


int TrackCanvas::drag_motion_event()
{
	return gui->drag_motion();
}

int TrackCanvas::drag_motion(Track **over_track,
	Edit **over_edit,
	PluginSet **over_pluginset,
	Plugin **over_plugin)
{
	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();


	if(get_cursor_over_window() &&
		cursor_x >= 0 && 
		cursor_y >= 0 && 
		cursor_x < get_w() && 
		cursor_y < get_h())
	{
//printf("TrackCanvas::drag_motion %d %d\n", __LINE__, pane->number);
// Find the edit and track the cursor is over
		for(Track *track = mwindow->edl->tracks->first; track; track = track->next)
		{
			int64_t track_x, track_y, track_w, track_h;
			track_dimensions(track, track_x, track_y, track_w, track_h);

			if(cursor_y >= track_y && 
				cursor_y < track_y + track_h)
			{
				*over_track = track;
				for(Edit *edit = track->edits->first; edit; edit = edit->next)
				{
					int64_t edit_x, edit_y, edit_w, edit_h;
					edit_dimensions(edit, edit_x, edit_y, edit_w, edit_h);

					if(cursor_x >= edit_x && 
						cursor_y >= edit_y && 
						cursor_x < edit_x + edit_w && 
						cursor_y < edit_y + edit_h)
					{
						*over_edit = edit;
						break;
					}
				}

				for(int i = 0; i < track->plugin_set.total; i++)
				{
					PluginSet *pluginset = track->plugin_set.values[i];
					


					for(Plugin *plugin = (Plugin*)pluginset->first;
						plugin;
						plugin = (Plugin*)plugin->next)
					{
						int64_t plugin_x, plugin_y, plugin_w, plugin_h;
						plugin_dimensions(plugin, plugin_x, plugin_y, plugin_w, plugin_h);
						
						if(cursor_y >= plugin_y &&
							cursor_y < plugin_y + plugin_h)
						{
							*over_pluginset = plugin->plugin_set;
						
							if(cursor_x >= plugin_x &&
								cursor_x < plugin_x + plugin_w)
							{
								*over_plugin = plugin;
								break;
							}
						}
					}
				}
				break;
			}
		}
	}

	return 0;
}

int TrackCanvas::drag_stop(int *redraw)
{
	int result = 0;
	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();


	if(get_cursor_over_window() &&
		cursor_x >= 0 && 
		cursor_y >= 0 && 
		cursor_x < get_w() && 
		cursor_y < get_h())
	{
		switch(mwindow->session->current_operation)
		{
			case DRAG_VTRANSITION:
			case DRAG_ATRANSITION:
				if(mwindow->session->edit_highlighted)
				{
					if((mwindow->session->current_operation == DRAG_ATRANSITION &&
						mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
						(mwindow->session->current_operation == DRAG_VTRANSITION &&
						mwindow->session->track_highlighted->data_type == TRACK_VIDEO))
					{
						mwindow->session->current_operation = NO_OPERATION;
						mwindow->paste_transition();
 						result = 1;
					}
				}
				*redraw = 1;
				break;




	// Behavior for dragged plugins is limited by the fact that a shared plugin
	// can only refer to a standalone plugin that exists in the same position in
	// time.  Dragging a plugin from one point in time to another can't produce
	// a shared plugin to the original plugin.  In this case we relocate the
	// plugin instead of sharing it.
			case DRAG_AEFFECT_COPY:
			case DRAG_VEFFECT_COPY:
				if(mwindow->session->track_highlighted &&
					((mwindow->session->current_operation == DRAG_AEFFECT_COPY &&
						mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
						(mwindow->session->current_operation == DRAG_VEFFECT_COPY &&
						mwindow->session->track_highlighted->data_type == TRACK_VIDEO)))
				{
					mwindow->session->current_operation = NO_OPERATION;

	// Insert shared plugin in source
					if(mwindow->session->track_highlighted != mwindow->session->drag_plugin->track &&
						!mwindow->session->plugin_highlighted &&
						!mwindow->session->pluginset_highlighted)
					{
	// Move plugin if different startproject

						mwindow->move_effect(mwindow->session->drag_plugin,
							0,
							mwindow->session->track_highlighted,
							0);
						result = 1;
					}
					else
	// Move source to different location
					if(mwindow->session->pluginset_highlighted)
					{
						if(mwindow->session->plugin_highlighted)
						{
							if(mwindow->session->plugin_highlighted !=
								mwindow->session->drag_plugin)
							{
								mwindow->move_effect(mwindow->session->drag_plugin,
									mwindow->session->plugin_highlighted->plugin_set,
									0,
									mwindow->session->plugin_highlighted->startproject);
							}
						}
						else
						{
							mwindow->move_effect(mwindow->session->drag_plugin,
								mwindow->session->pluginset_highlighted,
								0,
								mwindow->session->pluginset_highlighted->length());
						}
						result = 1;
					}
					else
	// Move to a new plugin set between two edits
					if(mwindow->session->edit_highlighted)
					{
						mwindow->move_effect(mwindow->session->drag_plugin,
							0,
							mwindow->session->track_highlighted,
							mwindow->session->edit_highlighted->startproject);
						result = 1;
					}
					else
	// Move to a new plugin set
					if(mwindow->session->track_highlighted)
					{
						mwindow->move_effect(mwindow->session->drag_plugin,
							0,
							mwindow->session->track_highlighted,
							0);
						result = 1;
					}
				}
				break;

			case DRAG_AEFFECT:
			case DRAG_VEFFECT:
				if(mwindow->session->track_highlighted && 
					((mwindow->session->current_operation == DRAG_AEFFECT &&
					mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
					(mwindow->session->current_operation == DRAG_VEFFECT &&
					mwindow->session->track_highlighted->data_type == TRACK_VIDEO)))
				{
// Drop all the effects
					PluginSet *plugin_set = mwindow->session->pluginset_highlighted;
					Track *track = mwindow->session->track_highlighted;
					double start = 0;
					double length = track->get_length();

					if(mwindow->session->plugin_highlighted)
					{
						start = track->from_units(mwindow->session->plugin_highlighted->startproject);
						length = track->from_units(mwindow->session->plugin_highlighted->length);
						if(length <= 0) length = track->get_length();
					}
					else
					if(mwindow->session->pluginset_highlighted)
					{
						start = track->from_units(plugin_set->length());
						length = track->get_length() - start;
						if(length <= 0) length = track->get_length();
					}
					else
					if(mwindow->edl->local_session->get_selectionend() > 
						mwindow->edl->local_session->get_selectionstart())
					{
						start = mwindow->edl->local_session->get_selectionstart();
						length = mwindow->edl->local_session->get_selectionend() - 
							mwindow->edl->local_session->get_selectionstart();
					}
	// Move to a point between two edits
	// 				else
	// 				if(mwindow->session->edit_highlighted)
	// 				{
	// 					start = mwindow->session->track_highlighted->from_units(
	// 						mwindow->session->edit_highlighted->startproject);
	// 					length = mwindow->session->track_highlighted->from_units(
	// 						mwindow->session->edit_highlighted->length);
	// 				}

					mwindow->insert_effects_canvas(start, length);
					*redraw = 1;
				}
				if (mwindow->session->track_highlighted)
					result = 1;  // we have to cleanup
				break;

			case DRAG_ASSET:
				if(mwindow->session->track_highlighted)
				{
					int64_t position = mwindow->session->edit_highlighted ?
						mwindow->session->edit_highlighted->startproject :
						mwindow->session->track_highlighted->edits->length();
					double position_f = mwindow->session->track_highlighted->from_units(position);
					Track *track = mwindow->session->track_highlighted;
					mwindow->paste_assets(position_f, track);
					result = 1;    // need to be one no matter what, since we have track highlited so we have to cleanup....
				}
				break;

			case DRAG_EDIT:
				mwindow->session->current_operation = NO_OPERATION;
				if(mwindow->session->track_highlighted)
				{
					if(mwindow->session->track_highlighted->data_type == mwindow->session->drag_edit->track->data_type)
					{
						int64_t position;
						double position_f = 0;
						if(mwindow->session->free_drag)
						{
							position_f = (double)(cursor_x + 
								mwindow->edl->local_session->view_start[pane->number]) *
								mwindow->edl->local_session->zoom_sample /
								mwindow->edl->session->sample_rate;
						}
						else
						{
							position = mwindow->session->edit_highlighted ?
								mwindow->session->edit_highlighted->startproject :
								mwindow->session->track_highlighted->edits->length();
							position_f = mwindow->session->track_highlighted->from_units(position);
						}
						Track *track = mwindow->session->track_highlighted;
						mwindow->move_edits(mwindow->session->drag_edits,
							track,
							position_f);
					}

					result = 1;
				}
				break;
		}
	}
	
	return result;
}


int TrackCanvas::drag_start_event()
{
	int result = 0;
	int redraw = 0;
	int rerender = 0;
	int new_cursor, update_cursor;

	if(mwindow->session->current_operation != NO_OPERATION) return 0;

	if(is_event_win())
	{

		if(do_plugins(get_drag_x(), 
			get_drag_y(), 
			1,
			0,
			redraw,
			rerender))
		{
			result = 1;
		}
		else
		if(do_edits(get_drag_x(),
			get_drag_y(),
			0,
			1,
			redraw,
			rerender,
			new_cursor,
			update_cursor))
		{
			result = 1;
		}
	}

	if(result) mwindow->session->free_drag = ctrl_down();

	return result;
}

int TrackCanvas::cursor_leave_event()
{
// Because drag motion calls get_cursor_over_window we can be sure that
// all highlights get deleted now.
// This ended up blocking keyboard input from the drag operations.
	if(timebar_position >= 0)
	{
		timebar_position = -1;
		if(pane->timebar) pane->timebar->update(1);
	}
	
	return 0;
//	return drag_motion();
}




void TrackCanvas::draw(int mode, int hide_cursor)
{
	const int debug = 0;


// Swap pixmap layers
 	if(get_w() != background_pixmap->get_w() ||
 		get_h() != background_pixmap->get_h())
 	{
 		delete background_pixmap;
 		background_pixmap = new BC_Pixmap(this, get_w(), get_h());
 	}

// Cursor disappears after resize when this is called.
// Cursor doesn't redraw after editing when this isn't called.
	if(pane->cursor && hide_cursor) pane->cursor->hide();
	draw_top_background(get_parent(), 0, 0, get_w(), get_h(), background_pixmap);

	if(debug) PRINT_TRACE
	draw_resources(mode);

	if(debug) PRINT_TRACE
	draw_overlays();
	if(debug) PRINT_TRACE
}

void TrackCanvas::update_cursor(int flush)
{
	switch(mwindow->edl->session->editing_mode)
	{
		case EDITING_ARROW: set_cursor(ARROW_CURSOR, 0, flush); break;
		case EDITING_IBEAM: set_cursor(IBEAM_CURSOR, 0, flush); break;
	}
}


void TrackCanvas::test_timer()
{
	if(resource_timer->get_difference() > 1000 && 
		!hourglass_enabled)
	{
		start_hourglass();
		hourglass_enabled = 1;
	}
}


void TrackCanvas::draw_indexes(Indexable *indexable)
{
// Don't redraw raw samples
	IndexState *index_state = 0;
	index_state = indexable->index_state;


	if(index_state->index_zoom > mwindow->edl->local_session->zoom_sample)
		return;


	draw_resources(0, 1, indexable);

	draw_overlays();
	flash(0);
}

void TrackCanvas::draw_resources(int mode, 
	int indexes_only, 
	Indexable *indexable)
{
	const int debug = 0;
	
	if(debug) PRINT_TRACE

	if(!mwindow->edl->session->show_assets) return;


// can't stop thread here, because this is called for every pane
//	if(mode != IGNORE_THREAD && !indexes_only)
//		gui->resource_thread->stop_draw(!indexes_only);

	if(mode != IGNORE_THREAD && 
		!indexes_only &&
		!gui->resource_thread->interrupted)
	{
		printf("TrackCanvas::draw_resources %d: called without stopping ResourceThread\n", 
			__LINE__);
		
		BC_Signals::dump_stack();
	}
	
	if(debug) PRINT_TRACE

	resource_timer->update();

// Age resource pixmaps for deletion
	if(!indexes_only)
		for(int i = 0; i < gui->resource_pixmaps.total; i++)
			gui->resource_pixmaps.values[i]->visible--;

	if(mode == FORCE_REDRAW)
		gui->resource_pixmaps.remove_all_objects();

	if(debug) PRINT_TRACE

// Search every edit
	for(Track *current = mwindow->edl->tracks->first;
		current;
		current = NEXT)
	{
		if(debug) PRINT_TRACE
		for(Edit *edit = current->edits->first; edit; edit = edit->next)
		{
			if(debug) PRINT_TRACE
			if(!edit->asset && !edit->nested_edl) continue;
			if(indexes_only)
			{
				if(edit->track->data_type != TRACK_AUDIO) continue;

				if(edit->nested_edl && 
					strcmp(indexable->path, edit->nested_edl->path)) continue;
					
				if(edit->asset &&
					strcmp(indexable->path, edit->asset->path)) continue;
			}

			if(debug) PRINT_TRACE

			int64_t edit_x, edit_y, edit_w, edit_h;
			edit_dimensions(edit, edit_x, edit_y, edit_w, edit_h);

// Edit is visible
			if(MWindowGUI::visible(edit_x, edit_x + edit_w, 0, get_w()) &&
				MWindowGUI::visible(edit_y, edit_y + edit_h, 0, get_h()))
			{
				int64_t pixmap_x, pixmap_w, pixmap_h;
				if(debug) PRINT_TRACE

// Search for existing pixmap containing edit
				for(int i = 0; i < gui->resource_pixmaps.total; i++)
				{
					ResourcePixmap* pixmap = gui->resource_pixmaps.values[i];
// Same pointer can be different edit if editing took place
					if(pixmap->edit_id == edit->id &&
						pixmap->pane_number == pane->number)
					{
						pixmap->visible = 1;
						break;
					}
				}
				if(debug) PRINT_TRACE

// Get new size, offset of pixmap needed
				get_pixmap_size(edit, 
					edit_x, 
					edit_w, 
					pixmap_x, 
					pixmap_w, 
					pixmap_h);
				if(debug) PRINT_TRACE

// Draw new data
				if(pixmap_w && pixmap_h)
				{
// Create pixmap if it doesn't exist
					ResourcePixmap* pixmap = create_pixmap(edit, 
						edit_x, 
						pixmap_x, 
						pixmap_w, 
						pixmap_h);
// Resize it if it's bigger
					if(pixmap_w > pixmap->pixmap_w ||
						pixmap_h > pixmap->pixmap_h)
						pixmap->resize(pixmap_w, pixmap_h);
					pixmap->draw_data(this,
						edit,
						edit_x, 
						edit_w, 
						pixmap_x, 
						pixmap_w, 
						pixmap_h, 
						mode,
						indexes_only);
// Resize it if it's smaller
					if(pixmap_w < pixmap->pixmap_w ||
						pixmap_h < pixmap->pixmap_h)
						pixmap->resize(pixmap_w, pixmap_h);

// Copy pixmap to background canvas
					background_pixmap->draw_pixmap(pixmap, 
						pixmap->pixmap_x, 
						current->y_pixel - mwindow->edl->local_session->track_start[pane->number],
						pixmap->pixmap_w,
						edit_h);
				}
				if(debug) PRINT_TRACE

			}
		}
	}


// Delete unused pixmaps
	if(debug) PRINT_TRACE
	if(!indexes_only)
		for(int i = gui->resource_pixmaps.total - 1; i >= 0; i--)
			if(gui->resource_pixmaps.values[i]->visible < PIXMAP_AGE)
			{
//printf("TrackCanvas::draw_resources %d\n", __LINE__);
				delete gui->resource_pixmaps.values[i];
				gui->resource_pixmaps.remove(gui->resource_pixmaps.values[i]);
			}
	if(debug) PRINT_TRACE

	if(hourglass_enabled) 
	{
		stop_hourglass();
		hourglass_enabled = 0;
	}
	if(debug) PRINT_TRACE

// can't stop thread here, because this is called for every pane
//	if(mode != IGNORE_THREAD && !indexes_only)
//		gui->resource_thread->start_draw();
	if(debug) PRINT_TRACE


}

ResourcePixmap* TrackCanvas::create_pixmap(Edit *edit, 
	int64_t edit_x, 
	int64_t pixmap_x, 
	int64_t pixmap_w, 
	int64_t pixmap_h)
{
	ResourcePixmap *result = 0;

	for(int i = 0; i < gui->resource_pixmaps.total; i++)
	{
//printf("TrackCanvas::create_pixmap 1 %d %d\n", edit->id, resource_pixmaps.values[i]->edit->id);
		if(gui->resource_pixmaps.values[i]->edit_id == edit->id &&
			gui->resource_pixmaps.values[i]->pane_number == pane->number) 
		{
			result = gui->resource_pixmaps.values[i];
			break;
		}
	}

	if(!result)
	{
//SET_TRACE
		result = new ResourcePixmap(mwindow, 
			gui, 
			edit,
			pane->number, 
			pixmap_w, 
			pixmap_h);
//SET_TRACE
		gui->resource_pixmaps.append(result);
	}

//	result->resize(pixmap_w, pixmap_h);
	return result;
}

void TrackCanvas::get_pixmap_size(Edit *edit, 
	int64_t edit_x, 
	int64_t edit_w, 
	int64_t &pixmap_x, 
	int64_t &pixmap_w,
	int64_t &pixmap_h)
{

// Align x on frame boundaries


// 	switch(edit->edits->track->data_type)
// 	{
// 		case TRACK_AUDIO:

			pixmap_x = edit_x;
			pixmap_w = edit_w;
			if(pixmap_x < 0)
			{
				pixmap_w -= -edit_x;
				pixmap_x = 0;
			}

			if(pixmap_x + pixmap_w > get_w())
			{
				pixmap_w = get_w() - pixmap_x;
			}

// 			break;
// 
// 		case TRACK_VIDEO:
// 		{
// 			int64_t picon_w = (int64_t)(edit->picon_w() + 0.5);
// 			int64_t frame_w = (int64_t)(edit->frame_w() + 0.5);
// 			int64_t pixel_increment = MAX(picon_w, frame_w);
// 			int64_t pixmap_x1 = edit_x;
// 			int64_t pixmap_x2 = edit_x + edit_w;
// 
// 			if(pixmap_x1 < 0)
// 			{
// 				pixmap_x1 = (int64_t)((double)-edit_x / pixel_increment) * 
// 					pixel_increment + 
// 					edit_x;
// 			}
// 
// 			if(pixmap_x2 > get_w())
// 			{
// 				pixmap_x2 = (int64_t)((double)(get_w() - edit_x) / pixel_increment + 1) * 
// 					pixel_increment + 
// 					edit_x;
// 			}
// 			pixmap_x = pixmap_x1;
// 			pixmap_w = pixmap_x2 - pixmap_x1;
// 			break;
// 		}
// 	}

	pixmap_h = mwindow->edl->local_session->zoom_track;
	if(mwindow->edl->session->show_titles) pixmap_h += mwindow->theme->get_image("title_bg_data")->get_h();
//printf("get_pixmap_size %d %d %d %d\n", edit_x, edit_w, pixmap_x, pixmap_w);
}

void TrackCanvas::edit_dimensions(Edit *edit, 
	int64_t &x, 
	int64_t &y, 
	int64_t &w, 
	int64_t &h)
{
//	w = Units::round(edit->track->from_units(edit->length) * 
//		mwindow->edl->session->sample_rate / 
//		mwindow->edl->local_session->zoom_sample);

	h = resource_h();

	x = Units::round(edit->track->from_units(edit->startproject) * 
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start[pane->number]);

// Method for calculating w so when edits are together we never get off by one error due to rounding
	int64_t x_next = Units::round(edit->track->from_units(edit->startproject + edit->length) * 
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start[pane->number]);
	w = x_next - x;

	y = edit->edits->track->y_pixel - mwindow->edl->local_session->track_start[pane->number];

	if(mwindow->edl->session->show_titles) 
		h += mwindow->theme->get_image("title_bg_data")->get_h();
}

void TrackCanvas::track_dimensions(Track *track, int64_t &x, int64_t &y, int64_t &w, int64_t &h)
{
	x = 0;
	w = get_w();
	y = track->y_pixel - mwindow->edl->local_session->track_start[pane->number];
	h = track->vertical_span(mwindow->theme);
}


void TrackCanvas::draw_paste_destination()
{
	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();
	int current_atrack = 0;
	int current_vtrack = 0;
	int current_aedit = 0;
	int current_vedit = 0;
	int64_t w = 0;
	int64_t x;
	double position;

//if(pane->number == BOTTOM_RIGHT_PANE)
//printf("TrackCanvas::draw_paste_destination %d %p\n", __LINE__, mwindow->session->track_highlighted);

	if((mwindow->session->current_operation == DRAG_ASSET &&
			(mwindow->session->drag_assets->total ||
			mwindow->session->drag_clips->total)) ||
		(mwindow->session->current_operation == DRAG_EDIT &&
			mwindow->session->drag_edits->total))
	{

		Indexable *indexable = 0;
		EDL *clip = 0;
		int draw_box = 0;

		if(mwindow->session->current_operation == DRAG_ASSET &&
			mwindow->session->drag_assets->size())
			indexable = mwindow->session->drag_assets->get(0);

		if(mwindow->session->current_operation == DRAG_ASSET &&
			mwindow->session->drag_clips->size())
			clip = mwindow->session->drag_clips->get(0);

// Get destination track
		for(Track *dest = mwindow->session->track_highlighted; 
			dest; 
			dest = dest->next)
		{
			if(dest->record)
			{
// Get source width in pixels
				w = 0;

// Use current cursor position
				if(mwindow->session->free_drag)
				{
					position = (double)(cursor_x + 
						mwindow->edl->local_session->view_start[pane->number]) *
						mwindow->edl->local_session->zoom_sample /
						mwindow->edl->session->sample_rate;
				}
				else
// Use start of highlighted edit
				if(mwindow->session->edit_highlighted)
				{
					position = mwindow->session->track_highlighted->from_units(
						mwindow->session->edit_highlighted->startproject);
				}
				else
// Use end of highlighted track, disregarding effects
				{
					position = mwindow->session->track_highlighted->from_units(
						mwindow->session->track_highlighted->edits->length());
				}

// Get the x coordinate
				x = Units::to_int64(position * 
					mwindow->edl->session->sample_rate /
					mwindow->edl->local_session->zoom_sample) - 
					mwindow->edl->local_session->view_start[pane->number];

				if(dest->data_type == TRACK_AUDIO)
				{
					if(indexable && current_atrack < indexable->get_audio_channels())
					{
//printf("TrackCanvas::draw_paste_destination %d %d\n", __LINE__, current_atrack);
						w = Units::to_int64((double)indexable->get_audio_samples() /
							indexable->get_sample_rate() *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample);
						current_atrack++;
						draw_box = 1;
					}
					else
					if(clip && current_atrack < clip->tracks->total_audio_tracks())
					{
						w = Units::to_int64((double)clip->tracks->total_length() *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample);
//printf("draw_paste_destination %d\n", x);
						current_atrack++;
						draw_box = 1;
					}
					else
					if(mwindow->session->current_operation == DRAG_EDIT &&
						current_aedit < mwindow->session->drag_edits->total)
					{
						Edit *edit;
						while(current_aedit < mwindow->session->drag_edits->total &&
							mwindow->session->drag_edits->values[current_aedit]->track->data_type != TRACK_AUDIO)
							current_aedit++;

						if(current_aedit < mwindow->session->drag_edits->total)
						{
							edit = mwindow->session->drag_edits->values[current_aedit];
							w = Units::to_int64(edit->length / mwindow->edl->local_session->zoom_sample);

							current_aedit++;
							draw_box = 1;
						}
					}
				}
				else
				if(dest->data_type == TRACK_VIDEO)
				{
//printf("draw_paste_destination 1\n");
					if(indexable && current_vtrack < indexable->get_video_layers())
					{
						w = Units::to_int64((double)indexable->get_video_frames() / 
							indexable->get_frame_rate() *
							mwindow->edl->session->sample_rate /
							mwindow->edl->local_session->zoom_sample);
						current_vtrack++;
						draw_box = 1;
					}
					else
					if(clip && current_vtrack < clip->tracks->total_video_tracks())
					{
						w = Units::to_int64(clip->tracks->total_length() *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample);
						current_vtrack++;
						draw_box = 1;
					}
					else
					if(mwindow->session->current_operation == DRAG_EDIT &&
						current_vedit < mwindow->session->drag_edits->total)
					{
						Edit *edit;
						while(current_vedit < mwindow->session->drag_edits->total &&
							mwindow->session->drag_edits->values[current_vedit]->track->data_type != TRACK_VIDEO)
							current_vedit++;

						if(current_vedit < mwindow->session->drag_edits->total)
						{
							edit = mwindow->session->drag_edits->values[current_vedit];
							w = Units::to_int64(edit->track->from_units(edit->length) *
								mwindow->edl->session->sample_rate / 
								mwindow->edl->local_session->zoom_sample);

							current_vedit++;
							draw_box = 1;
						}
					}
				}

				if(w)
				{
					int y = dest->y_pixel - mwindow->edl->local_session->track_start[pane->number];
					int h = dest->vertical_span(mwindow->theme);


//printf("TrackCanvas::draw_paste_destination 2 %d %d %d %d\n", x, y, w, h);
					if(x < -BC_INFINITY)
					{
						w -= -BC_INFINITY - x;
						x += -BC_INFINITY - x;
					}
					w = MIN(65535, w);
// if(pane->number == TOP_RIGHT_PANE)
// printf("TrackCanvas::draw_paste_destination %d %d %d %d %d\n",
// __LINE__,
// x,
// y,
// w,
// h);
					draw_highlight_rectangle(x, y, w, h);
				}
			}
		}
	}
}

void TrackCanvas::plugin_dimensions(Plugin *plugin, int64_t &x, int64_t &y, int64_t &w, int64_t &h)
{
	x = Units::round(plugin->track->from_units(plugin->startproject) *
		mwindow->edl->session->sample_rate / 
		mwindow->edl->local_session->zoom_sample - 
		mwindow->edl->local_session->view_start[pane->number]);
	w = Units::round(plugin->track->from_units(plugin->length) *
		mwindow->edl->session->sample_rate / 
		mwindow->edl->local_session->zoom_sample);
	y = plugin->track->y_pixel - 
		mwindow->edl->local_session->track_start[pane->number] + 
		mwindow->edl->local_session->zoom_track +
		plugin->plugin_set->get_number() * 
		mwindow->theme->get_image("plugin_bg_data")->get_h();
	if(mwindow->edl->session->show_titles)
		y += mwindow->theme->get_image("title_bg_data")->get_h();
	h = mwindow->theme->get_image("plugin_bg_data")->get_h();
}

int TrackCanvas::resource_h()
{
	return mwindow->edl->local_session->zoom_track;
}

void TrackCanvas::draw_highlight_rectangle(int x, int y, int w, int h)
{
	if(x < -10)
	{
		w += x - -10;
		x = -10;
	}

	if(y < -10)
	{
		h += y - -10;
		y = -10;
	}

	w = MIN(w, get_w() + 20);
	h = MIN(h, get_h() + 20);
	if(w > 0 && h > 0)
	{
		set_color(WHITE);
		set_inverse();
		draw_rectangle(x, y, w, h);
		draw_rectangle(x + 1, y + 1, w - 2, h - 2);
		draw_rectangle(x + 2, y + 2, w - 4, h - 4);
		set_opaque();
	}
//if(pane->number == TOP_RIGHT_PANE) 
//printf("TrackCanvas::draw_highlight_rectangle %d %d %d %d %d\n", __LINE__, x, y, w, h);
}

void TrackCanvas::draw_playback_cursor()
{
// Called before playback_cursor exists
// 	if(mwindow->playback_cursor && mwindow->playback_cursor->visible)
// 	{
// 		mwindow->playback_cursor->visible = 0;
// 		mwindow->playback_cursor->draw();
// 	}
}

void TrackCanvas::get_handle_coords(Edit *edit, int64_t &x, int64_t &y, int64_t &w, int64_t &h, int side)
{
	int handle_w = mwindow->theme->edithandlein_data[0]->get_w();
	int handle_h = mwindow->theme->edithandlein_data[0]->get_h();

	edit_dimensions(edit, x, y, w, h);

	if(mwindow->edl->session->show_titles)
	{
		y += mwindow->theme->get_image("title_bg_data")->get_h();
	}
	else
	{
		y = 0;
	}

	if(side == EDIT_OUT)
	{
		x += w - handle_w;
	}

	h = handle_h;
	w = handle_w;
}

void TrackCanvas::get_transition_coords(int64_t &x, int64_t &y, int64_t &w, int64_t &h)
{
//printf("TrackCanvas::get_transition_coords 1\n");
// 	int transition_w = mwindow->theme->transitionhandle_data[0]->get_w();
// 	int transition_h = mwindow->theme->transitionhandle_data[0]->get_h();
	int transition_w = 30;
	int transition_h = 30;
//printf("TrackCanvas::get_transition_coords 1\n");

	if(mwindow->edl->session->show_titles)
		y += mwindow->theme->get_image("title_bg_data")->get_h();
//printf("TrackCanvas::get_transition_coords 2\n");

	y += (h - mwindow->theme->get_image("title_bg_data")->get_h()) / 2 - transition_h / 2;
	x -= transition_w / 2;

	h = transition_h;
	w = transition_w;
}

void TrackCanvas::draw_highlighting()
{
	int64_t x, y, w, h;
	int draw_box = 0;




	switch(mwindow->session->current_operation)
	{
		case DRAG_ATRANSITION:
		case DRAG_VTRANSITION:
//printf("TrackCanvas::draw_highlighting 1 %p %p\n", 
//	mwindow->session->track_highlighted, mwindow->session->edit_highlighted);
			if(mwindow->session->edit_highlighted)
			{
//printf("TrackCanvas::draw_highlighting 2\n");
				if((mwindow->session->current_operation == DRAG_ATRANSITION && 
					mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
					(mwindow->session->current_operation == DRAG_VTRANSITION && 
					mwindow->session->track_highlighted->data_type == TRACK_VIDEO))
				{
//printf("TrackCanvas::draw_highlighting 2\n");
					edit_dimensions(mwindow->session->edit_highlighted, x, y, w, h);
//printf("TrackCanvas::draw_highlighting 2\n");

					if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
						MWindowGUI::visible(y, y + h, 0, get_h()))
					{
						draw_box = 1;
						get_transition_coords(x, y, w, h);
					}
//printf("TrackCanvas::draw_highlighting 3\n");
				}
			}
			break;



// Dragging a new effect from the Resource window
		case DRAG_AEFFECT:
		case DRAG_VEFFECT:
			if(mwindow->session->track_highlighted &&
				((mwindow->session->current_operation == DRAG_AEFFECT && mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
					(mwindow->session->current_operation == DRAG_VEFFECT && mwindow->session->track_highlighted->data_type == TRACK_VIDEO)))
			{
// Put it before another plugin
				if(mwindow->session->plugin_highlighted)
				{
					plugin_dimensions(mwindow->session->plugin_highlighted, 
						x, 
						y, 
						w, 
						h);
//printf("TrackCanvas::draw_highlighting 1 %d %d\n", x, w);
				}
				else
// Put it after a plugin set
				if(mwindow->session->pluginset_highlighted &&
					mwindow->session->pluginset_highlighted->last)
				{
					plugin_dimensions((Plugin*)mwindow->session->pluginset_highlighted->last, 
						x, 
						y, 
						w, 
						h);
//printf("TrackCanvas::draw_highlighting 1 %d %d\n", x, w);
					int64_t track_x, track_y, track_w, track_h;
					track_dimensions(mwindow->session->track_highlighted, 
						track_x, 
						track_y, 
						track_w, 
						track_h);

					x += w;
					w = Units::round(
							mwindow->session->track_highlighted->get_length() *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample - 
							mwindow->edl->local_session->view_start[pane->number]) -
						x;
//printf("TrackCanvas::draw_highlighting 2 %d\n", w);
					if(w <= 0) w = track_w;
				}
				else
				{
					track_dimensions(mwindow->session->track_highlighted, 
						x, 
						y, 
						w, 
						h);

//printf("TrackCanvas::draw_highlighting 1 %d %d %d %d\n", x, y, w, h);
// Put it in a new plugin set determined by the selected range
					if(mwindow->edl->local_session->get_selectionend() > 
						mwindow->edl->local_session->get_selectionstart())
					{
						x = Units::to_int64(mwindow->edl->local_session->get_selectionstart() *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample -
							mwindow->edl->local_session->view_start[pane->number]);
						w = Units::to_int64((mwindow->edl->local_session->get_selectionend() - 
							mwindow->edl->local_session->get_selectionstart()) *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample);
					}
// Put it in a new plugin set determined by an edit boundary
//					else
// 					if(mwindow->session->edit_highlighted)
// 					{
// 						int64_t temp_y, temp_h;
// 						edit_dimensions(mwindow->session->edit_highlighted, 
// 							x, 
// 							temp_y, 
// 							w, 
// 							temp_h);
// 					}
// Put it at the beginning of the track in a new plugin set
				}

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
//printf("TrackCanvas::draw_highlighting 1\n");
					draw_box = 1;
				}
			}
			break;
		
		case DRAG_ASSET:
			if(mwindow->session->track_highlighted)
			{
//				track_dimensions(mwindow->session->track_highlighted, x, y, w, h);

//				if(MWindowGUI::visible(y, y + h, 0, get_h()))
//				{
					draw_paste_destination();
//				}
			}
			break;

		case DRAG_EDIT:
			if(mwindow->session->track_highlighted)
			{
//				track_dimensions(mwindow->session->track_highlighted, x, y, w, h);
//
//				if(MWindowGUI::visible(y, y + h, 0, get_h()))
//				{
					draw_paste_destination();
//				}
			}
			break;

// Dragging an effect from the timeline
		case DRAG_AEFFECT_COPY:
		case DRAG_VEFFECT_COPY:
			if((mwindow->session->plugin_highlighted || mwindow->session->track_highlighted) &&
				((mwindow->session->current_operation == DRAG_AEFFECT_COPY && mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
				(mwindow->session->current_operation == DRAG_VEFFECT_COPY && mwindow->session->track_highlighted->data_type == TRACK_VIDEO)))
			{
// Put it before another plugin
				if(mwindow->session->plugin_highlighted)
					plugin_dimensions(mwindow->session->plugin_highlighted, x, y, w, h);
				else
// Put it after a plugin set
				if(mwindow->session->pluginset_highlighted &&
					mwindow->session->pluginset_highlighted->last)
				{
					plugin_dimensions((Plugin*)mwindow->session->pluginset_highlighted->last, x, y, w, h);
					x += w;
				}
				else
				if(mwindow->session->track_highlighted)
				{
					track_dimensions(mwindow->session->track_highlighted, x, y, w, h);

// Put it in a new plugin set determined by an edit boundary
					if(mwindow->session->edit_highlighted)
					{
						int64_t temp_y, temp_h;
						edit_dimensions(mwindow->session->edit_highlighted, 
							x, 
							temp_y, 
							w, 
							temp_h);
					}
// Put it in a new plugin set at the start of the track
				}

// Calculate length of plugin based on data type of track and units
				if(mwindow->session->track_highlighted->data_type == TRACK_VIDEO)
				{
					w = (int64_t)((double)mwindow->session->drag_plugin->length / 
						mwindow->edl->session->frame_rate *
						mwindow->edl->session->sample_rate /
						mwindow->edl->local_session->zoom_sample);
				}
				else
				{
					w = (int64_t)mwindow->session->drag_plugin->length /
						mwindow->edl->local_session->zoom_sample;
				}

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					draw_box = 1;
				}
			}
			break;

		case DRAG_PLUGINKEY:
			if(mwindow->session->plugin_highlighted && 
			   mwindow->session->current_operation == DRAG_PLUGINKEY)
			{
// Just highlight the plugin
				plugin_dimensions(mwindow->session->plugin_highlighted, x, y, w, h);

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					draw_box = 1;
				}
			}
			break;

	}


	if(draw_box)
	{
		draw_highlight_rectangle(x, y, w, h);
	}
}

void TrackCanvas::draw_plugins()
{
	char string[BCTEXTLEN];
	int current_on = 0;
	int current_show = 0;


//	if(!mwindow->edl->session->show_assets) goto done;

	for(int i = 0; i < plugin_on_toggles.total; i++)
		plugin_on_toggles.values[i]->in_use = 0;
	for(int i = 0; i < plugin_show_toggles.total; i++)
		plugin_show_toggles.values[i]->in_use = 0;


	for(Track *track = mwindow->edl->tracks->first;
		track;
		track = track->next)
	{
		if(track->expand_view)
		{
			for(int i = 0; i < track->plugin_set.total; i++)
			{
				PluginSet *pluginset = track->plugin_set.values[i];

				for(Plugin *plugin = (Plugin*)pluginset->first; plugin; plugin = (Plugin*)plugin->next)
				{
					int64_t total_x, y, total_w, h;
					plugin_dimensions(plugin, total_x, y, total_w, h);
					
					if(MWindowGUI::visible(total_x, total_x + total_w, 0, get_w()) &&
						MWindowGUI::visible(y, y + h, 0, get_h()) &&
						plugin->plugin_type != PLUGIN_NONE)
					{
						int x = total_x, w = total_w, left_margin = 5;
						int right_margin = 5;
						if(x < 0)
						{
							w -= -x;
							x = 0;
						}
						if(w + x > get_w()) w -= (w + x) - get_w();

						draw_3segmenth(x, 
							y, 
							w, 
							total_x,
							total_w,
							mwindow->theme->get_image("plugin_bg_data"),
							0);
						set_color(mwindow->theme->title_color);
						set_font(mwindow->theme->title_font);
						plugin->calculate_title(string, 0);

// Truncate string to int64_test visible in background
						int len = strlen(string), j;
						for(j = len; j >= 0; j--)
						{
							if(left_margin + get_text_width(mwindow->theme->title_font, string) > w)
							{
								string[j] = 0;
							}
							else
								break;
						}

// Justify the text on the left boundary of the edit if it is visible.
// Otherwise justify it on the left side of the screen.
						int64_t text_x = total_x + left_margin;
						int64_t text_w = get_text_width(mwindow->theme->title_font, string, strlen(string));
						text_x = MAX(left_margin, text_x);
						draw_text(text_x, 
							y + get_text_ascent(mwindow->theme->title_font) + 2, 
							string,
							strlen(string),
							0);
						int64_t min_x = total_x + text_w;


// Update plugin toggles
						int toggle_x = total_x + total_w;
						int toggle_y = y;
						toggle_x = MIN(get_w() - right_margin, toggle_x);

// On toggle
						toggle_x -= PluginOn::calculate_w(mwindow) + 10;
						if(toggle_x > min_x)
						{
							if(current_on >= plugin_on_toggles.total)
							{
								PluginOn *plugin_on = new PluginOn(mwindow, toggle_x, toggle_y, plugin);
								add_subwindow(plugin_on);
								plugin_on_toggles.append(plugin_on);
							}
							else
							{
								plugin_on_toggles.values[current_on]->update(toggle_x, toggle_y, plugin);
							}
							current_on++;
						}

// Toggles for standalone plugins only
						if(plugin->plugin_type == PLUGIN_STANDALONE)
						{
// Show
							toggle_x -= PluginShow::calculate_w(mwindow) + 10;
							if(toggle_x > min_x)
							{
								if(current_show >= plugin_show_toggles.total)
								{
									PluginShow *plugin_show = new PluginShow(mwindow, toggle_x, toggle_y, plugin);
									add_subwindow(plugin_show);
									plugin_show_toggles.append(plugin_show);
								}
								else
								{
									plugin_show_toggles.values[current_show]->update(toggle_x, toggle_y, plugin);
								}
								current_show++;
							}


							
						}
					}
				}
			}
		}
	}

// Remove unused toggles
done:
	while(current_show < plugin_show_toggles.total)
	{
		plugin_show_toggles.remove_object_number(current_show);
	}

	while(current_on < plugin_on_toggles.total)
	{
		plugin_on_toggles.remove_object_number(current_on);
	}

}

void TrackCanvas::refresh_plugintoggles()
{
	for(int i = 0; i < plugin_on_toggles.total; i++)
	{
		PluginOn *on = plugin_on_toggles.values[i];
		on->reposition_window(on->get_x(), on->get_y());
	}
	for(int i = 0; i < plugin_show_toggles.total; i++)
	{
		PluginShow *show = plugin_show_toggles.values[i];
		show->reposition_window(show->get_x(), show->get_y());
	}
}

void TrackCanvas::draw_inout_points()
{
}


void TrackCanvas::draw_drag_handle()
{
	if(mwindow->session->current_operation == DRAG_EDITHANDLE2 ||
		mwindow->session->current_operation == DRAG_PLUGINHANDLE2)
	{
//printf("TrackCanvas::draw_drag_handle 1 %ld %ld\n", mwindow->session->drag_sample, mwindow->edl->local_session->view_start);
		int64_t pixel1 = Units::round(mwindow->session->drag_position * 
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start[pane->number]);
//printf("TrackCanvas::draw_drag_handle 2 %d %jd\n", pane->number, pixel1);
		set_color(GREEN);
		set_inverse();
//printf("TrackCanvas::draw_drag_handle 3\n");
		draw_line(pixel1, 0, pixel1, get_h());
		set_opaque();
//printf("TrackCanvas::draw_drag_handle 4\n");
	}
}


void TrackCanvas::draw_transitions()
{
	int64_t x, y, w, h;

//	if(!mwindow->edl->session->show_assets) return;

	for(Track *track = mwindow->edl->tracks->first;
		track;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit;
			edit = edit->next)
		{
			if(edit->transition)
			{
				edit_dimensions(edit, x, y, w, h);
				get_transition_coords(x, y, w, h);

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					PluginServer *server = mwindow->scan_plugindb(edit->transition->title,
						track->data_type);
					if(!server->picon)
					{
						server->open_plugin(1, mwindow->preferences, 0, 0, -1);
						server->close_plugin();
					}

					if(server->picon)
					{
						draw_vframe(server->picon, 
							x, 
							y, 
							w, 
							h, 
							0, 
							0, 
							server->picon->get_w(), 
							server->picon->get_h());
					}
				}
			}
		}
	}
}

void TrackCanvas::draw_loop_points()
{
//printf("TrackCanvas::draw_loop_points 1\n");
	if(mwindow->edl->local_session->loop_playback)
	{
//printf("TrackCanvas::draw_loop_points 2\n");
		int64_t x = Units::round(mwindow->edl->local_session->loop_start *
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start[pane->number]);
//printf("TrackCanvas::draw_loop_points 3\n");

		if(MWindowGUI::visible(x, x + 1, 0, get_w()))
		{
			set_color(GREEN);
			draw_line(x, 0, x, get_h());
		}
//printf("TrackCanvas::draw_loop_points 4\n");

		x = Units::round(mwindow->edl->local_session->loop_end *
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start[pane->number]);
//printf("TrackCanvas::draw_loop_points 5\n");

		if(MWindowGUI::visible(x, x + 1, 0, get_w()))
		{
			set_color(GREEN);
			draw_line(x, 0, x, get_h());
		}
//printf("TrackCanvas::draw_loop_points 6\n");
	}
//printf("TrackCanvas::draw_loop_points 7\n");
}

void TrackCanvas::draw_brender_range()
{
	if(mwindow->preferences->use_brender)
	{
		int64_t x1 = Units::round(mwindow->edl->session->brender_start *
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start[pane->number]);
		int64_t x2 = Units::round(mwindow->edl->session->brender_end *
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start[pane->number]);

		if(MWindowGUI::visible(x2, x2 + 1, 0, get_w()))
		{
			set_color(RED);
			draw_line(x2, 0, x2, get_h());
		}
		if(MWindowGUI::visible(x1, x1 + 1, 0, get_w()))
		{
			set_color(RED);
			draw_line(x1, 0, x1, get_h());
		}
	}
}

static int auto_colors[] = 
{
	BLUE,
	RED,
	GREEN,
	BLUE,
	RED,
	GREEN,
	BLUE,
	WHITE,
	0,
	0,
	0,
	WHITE
};

// The operations which correspond to each automation type
static int auto_operations[] = 
{
	DRAG_MUTE,
	DRAG_CAMERA_X,
	DRAG_CAMERA_Y,
	DRAG_CAMERA_Z,
	DRAG_PROJECTOR_X,
	DRAG_PROJECTOR_Y,
	DRAG_PROJECTOR_Z,
	DRAG_FADE,
	DRAG_PAN,
	DRAG_MODE,
	DRAG_MASK,
	DRAG_SPEED
};

// The buttonpress operations, so nothing changes unless the mouse moves
// a certain amount.  This allows the keyframe to be used to position the
// insertion point without moving itself.
static int pre_auto_operations[] =
{
	DRAG_MUTE,
	DRAG_CAMERA_X,
	DRAG_CAMERA_Y,
	DRAG_CAMERA_Z,
	DRAG_PROJECTOR_X,
	DRAG_PROJECTOR_Y,
	DRAG_PROJECTOR_Z,
	DRAG_FADE,
	DRAG_PAN_PRE,
	DRAG_MODE_PRE,
	DRAG_MASK_PRE,
	DRAG_SPEED
};


int TrackCanvas::do_keyframes(int cursor_x, 
	int cursor_y, 
	int draw, 
	int buttonpress, 
	int &new_cursor,
	int &update_cursor,
	int &rerender)
{
// Note: button 3 (right mouse button) is not eaten to allow
// track context menu to appear
	int current_tool = 0;
	int result = 0;
	EDLSession *session = mwindow->edl->session;



	BC_Pixmap *auto_pixmaps[] = 
	{
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		pankeyframe_pixmap,
		modekeyframe_pixmap,
		maskkeyframe_pixmap,
		0,
	};



	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
        Auto *auto_keyframe;
		Automation *automation = track->automation;


// Handle keyframes in reverse drawing order if a button press
		int start = 0;
		int end = AUTOMATION_TOTAL;
		int step = 1;
		if(buttonpress)
		{
			start = AUTOMATION_TOTAL - 1;
			end = -1;
			step = -1;
		}
		for(int i = start; i != end && !result; i += step)
		{
// Event not trapped and automation visible
			Autos *autos = automation->autos[i];
			if(!result && session->auto_conf->autos[i] && autos)
			{
				switch(i)
				{
					case AUTOMATION_MODE:
					case AUTOMATION_PAN:
					case AUTOMATION_MASK:
						result = do_autos(track, 
							automation->autos[i],
							cursor_x, 
							cursor_y, 
							draw, 
							buttonpress,
							auto_pixmaps[i],
                            auto_keyframe,
							rerender);
						break;

					default:
						switch(autos->get_type())
						{
							case Autos::AUTOMATION_TYPE_FLOAT:
// Do dropshadow
								if(draw)
									result = do_float_autos(track, 
										autos,
										cursor_x, 
										cursor_y, 
										draw, 
										buttonpress, 
										1,
										1,
										BLACK,
										auto_keyframe);

								result = do_float_autos(track, 
									autos,
									cursor_x, 
									cursor_y, 
									draw, 
									buttonpress, 
									0,
									0,
									auto_colors[i],
									auto_keyframe);
								break;

							case Autos::AUTOMATION_TYPE_INT:
// Do dropshadow
								if(draw)
									result = do_int_autos(track, 
										autos,
										cursor_x, 
										cursor_y, 
										draw, 
										buttonpress,
										1,
										1,
										BLACK,
										auto_keyframe);
								result = do_int_autos(track, 
									autos,
									cursor_x, 
									cursor_y, 
									draw, 
									buttonpress,
									0,
									0,
									auto_colors[i],
									auto_keyframe);
								break;
						}
						break;
				}
			


				if(result)
				{
					if(mwindow->session->current_operation == auto_operations[i])
						rerender = 1;

// printf("TrackCanvas::do_keyframes %d %d %d\n", 
// __LINE__, 
// mwindow->session->current_operation,
// auto_operations[i]);
					if(buttonpress)
					{
                        if (buttonpress != 3)
                        {
							if(i == AUTOMATION_FADE) 
								synchronize_autos(0, 
									track, 
									(FloatAuto*)mwindow->session->drag_auto, 
									1);
							mwindow->session->current_operation = pre_auto_operations[i];
							update_drag_caption();
						}
						else
						{
                            gui->keyframe_menu->update(automation, 
								autos, 
								auto_keyframe);
                            gui->keyframe_menu->activate_menu();
                            rerender = 1; // the position changes
						}
					}
				}
			}
		}




		if(!result && 
			session->auto_conf->plugins /* &&
			mwindow->edl->session->show_assets */)
		{
			Plugin *plugin;
			KeyFrame *keyframe;
			result = do_plugin_autos(track,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				plugin,
				keyframe);
			if(result && mwindow->session->current_operation == DRAG_PLUGINKEY)
			{
				rerender = 1;
			}
			if(result && (buttonpress == 1))
			{
				mwindow->session->current_operation = DRAG_PLUGINKEY_PRE;
				update_drag_caption();
				rerender = 1;
			} else
			if (result && (buttonpress == 3))
			{
				gui->keyframe_menu->update(plugin, keyframe);
				gui->keyframe_menu->activate_menu();
				rerender = 1; // the position changes
			}
		}
	}

// Final pass to trap event
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(mwindow->session->current_operation == pre_auto_operations[i] ||
			mwindow->session->current_operation == auto_operations[i])
		{
			result = 1;
			break;
		}
	}

	if(mwindow->session->current_operation == DRAG_PLUGINKEY ||
		mwindow->session->current_operation == DRAG_PLUGINKEY_PRE)
	{
	 	result = 1;
	}

	update_cursor = 1;
	if(result)
	{
		new_cursor = UPRIGHT_ARROW_CURSOR;
	}


	return result;
}

void TrackCanvas::draw_auto(Auto *current, 
	int x, 
	int y, 
	int center_pixel, 
	int zoom_track)
{
	int x1, y1, x2, y2;
	char string[BCTEXTLEN];

	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	if(y1 < center_pixel + -zoom_track / 2) y1 = center_pixel + -zoom_track / 2;
	if(y2 > center_pixel + zoom_track / 2) y2 = center_pixel + zoom_track / 2;

	draw_box(x1, y1, x2 - x1, y2 - y1);
}








// This draws lines for bezier in & out controls
void TrackCanvas::draw_cropped_line(int x1, 
	int y1, 
	int x2, 
	int y2, 
	int min_y,
	int max_y)
{


// Don't care about x since it is clipped by the window.
// Put y coords in ascending order
	if(y2 < y1)
	{
		y2 ^= y1;
		y1 ^= y2;
		y2 ^= y1;
		x2 ^= x1;
		x1 ^= x2;
		x2 ^= x1;
	}



	double slope = (double)(x2 - x1) / (y2 - y1);
//printf("TrackCanvas::draw_cropped_line %d %d %d %d %d\n", __LINE__, x1, y1, x2, y2);
	if(y1 < min_y)
	{
		x1 = (int)(x1 + (min_y - y1) * slope);
		y1 = min_y;
	}
	else
	if(y1 >= max_y)
	{
		x1 = (int)(x1 + (max_y - 1 - y1) * slope);
		y1 = max_y - 1;
	}

	if(y2 >= max_y)
	{
		x2 = (int)(x2 + (max_y - 1 - y2) * slope);
		y2 = max_y - 1;
	}
	else
	if(y2 < min_y)
	{
		x2 = (int)(x2 + (min_y - y2) * slope);
		y1 = min_y;
	}


//printf("TrackCanvas::draw_cropped_line %d %d %d %d %d\n", __LINE__, x1, y1, x2, y2);
	if(y1 >= min_y && 
		y1 < max_y &&
		y2 >= min_y &&
		y2 < max_y)
		draw_line(x1, y1, x2, y2);
}




void TrackCanvas::draw_floatauto(Auto *current, 
	int x, 
	int y, 
	int in_x, 
	int in_y, 
	int out_x, 
	int out_y, 
	int center_pixel, 
	int zoom_track)
{
	int x1, y1, x2, y2;
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	int visible;
	char string[BCTEXTLEN];

// Center extents
	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	CLAMP(y1, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(y2, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);

	if(y2 - 1 > y1)
	{
		if(((FloatAuto*)current)->mode == Auto::LINEAR)
		{
			draw_box(x1, y1, x2 - x1, y2 - y1);
		}
		else
		{
			ArrayList<int> polygon_x;
			ArrayList<int> polygon_y;
			polygon_x.append((x1 + x2) / 2 + 1);
			polygon_y.append(y1 + 1);
			polygon_x.append(x2 + 1);
			polygon_y.append((y1 + y2) / 2 + 1);
			polygon_x.append((x1 + x2) / 2 + 1);
			polygon_y.append(y2 + 1);
			polygon_x.append(x1 + 1);
			polygon_y.append((y1 + y2) / 2 + 1);
			fill_polygon(&polygon_x, &polygon_y);
		}
	}

// In handle
	if(current->mode == Auto::BEZIER)
	{
		in_x1 = in_x - HANDLE_W / 2;
		in_x2 = in_x + HANDLE_W / 2;
		in_y1 = center_pixel + in_y - HANDLE_W / 2;
		in_y2 = center_pixel + in_y + HANDLE_W / 2;

	// Draw line
		draw_cropped_line(x, 
			center_pixel + y, 
			in_x, 
			center_pixel + in_y, 
			center_pixel + -zoom_track / 2,
			center_pixel + zoom_track / 2);

		CLAMP(in_y1, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
		CLAMP(in_y2, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
		CLAMP(in_y, -zoom_track / 2, zoom_track / 2);

//     Draw handle
//  	if(in_y2 > in_y1)
//  	{
//  		set_color(BLACK);
//  		draw_box(in_x1 + 1, in_y1 + 1, in_x2 - in_x1, in_y2 - in_y1);
//  		set_color(color);
//  		draw_box(in_x1, in_y1, in_x2 - in_x1, in_y2 - in_y1);
//  	}
//     

	// Out handle
		out_x1 = out_x - HANDLE_W / 2;
		out_x2 = out_x + HANDLE_W / 2;
		out_y1 = center_pixel + out_y - HANDLE_W / 2;
		out_y2 = center_pixel + out_y + HANDLE_W / 2;

	// Draw line
		draw_cropped_line(x, 
			center_pixel + y, 
			out_x, 
			center_pixel + out_y, 
			center_pixel + -zoom_track / 2,
			center_pixel + zoom_track / 2);

		CLAMP(out_y1, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
		CLAMP(out_y2, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
		CLAMP(out_y, -zoom_track / 2, zoom_track / 2);

//  	if(out_y2 > out_y1)
//  	{
//  		set_color(BLACK);
//  		draw_box(out_x1 + 1, out_y1 + 1, out_x2 - out_x1, out_y2 - out_y1);
//  		set_color(color);
//  		draw_box(out_x1, out_y1, out_x2 - out_x1, out_y2 - out_y1);
//  	}
	}
}

int TrackCanvas::test_auto(Auto *current, 
	int x, 
	int y, 
	int center_pixel, 
	int zoom_track, 
	int cursor_x, 
	int cursor_y, 
	int buttonpress)
{
	int x1, y1, x2, y2;
	char string[BCTEXTLEN];
	int result = 0;

	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	if(y1 < center_pixel + -zoom_track / 2) y1 = center_pixel + -zoom_track / 2;
	if(y2 > center_pixel + zoom_track / 2) y2 = center_pixel + zoom_track / 2;

	if(cursor_x >= x1 && cursor_x < x2 && cursor_y >= y1 && cursor_y < y2)
	{
		if(buttonpress && buttonpress != 3)
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = current->value_to_percentage();
			mwindow->session->drag_start_position = current->position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
		}
		result = 1;
	}

	if(buttonpress && buttonpress != 3 && result)
	{
//printf("TrackCanvas::test_auto %d\n", __LINE__);
		mwindow->undo->update_undo_before();
	}

	return result;
}

int TrackCanvas::test_floatauto(Auto *current, 
	int x, 
	int y, 
	int in_x,
	int in_y,
	int out_x,
	int out_y,
	int center_pixel, 
	int zoom_track, 
	int cursor_x, 
	int cursor_y, 
	int buttonpress)
{
	int x1, y1, x2, y2;
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	char string[BCTEXTLEN];
	int result = 0;

	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	if(y1 < center_pixel + -zoom_track / 2) y1 = center_pixel + -zoom_track / 2;
	if(y2 > center_pixel + zoom_track / 2) y2 = center_pixel + zoom_track / 2;

	in_x1 = in_x;
	in_x2 = x;

// Compute in handle extents from x position
	if(x > in_x)
	{
		in_y1 = center_pixel + 
			in_y + 
			(cursor_x - in_x) * 
			(y - in_y) / 
			(x - in_x) - 
			HANDLE_W / 2;
	}
	else
	{
		in_y1 = in_y - HANDLE_W / 2;
	}
	in_y2 = in_y1 + HANDLE_W;

	if(in_y1 < center_pixel + -zoom_track / 2) in_y1 = center_pixel + -zoom_track / 2;
	if(in_y2 > center_pixel + zoom_track / 2) in_y2 = center_pixel + zoom_track / 2;

	out_x1 = x;
	out_x2 = out_x;
	if(x < out_x)
	{
		out_y1 = center_pixel +
			y +
			(cursor_x - x) *
			(out_y - y) /
			(out_x - x) -
			HANDLE_W / 2;
	}
	else
		out_y1 = out_y - HANDLE_W / 2;

	out_y2 = out_y1 + HANDLE_W;

	if(out_y1 < center_pixel + -zoom_track / 2) out_y1 = center_pixel + -zoom_track / 2;
	if(out_y2 > center_pixel + zoom_track / 2) out_y2 = center_pixel + zoom_track / 2;


//if(ctrl_down())
//printf("TrackCanvas::test_floatauto %d cursor_x=%d cursor_y=%d in_x1=%d in_x2=%d in_y=%d in_y1=%d in_y2=%d position=%lld\n", 
//__LINE__, cursor_x, cursor_y, in_x1, in_x2, in_y, in_y1, in_y2, current->position);
//if(ctrl_down())
//printf("TrackCanvas::test_floatauto %d cursor_x=%d cursor_y=%d out_x1=%d out_x2=%d out_y=%d out_y1=%d out_y2=%d\n", 
//__LINE__, cursor_x, cursor_y, out_x1, out_x2, out_y, out_y1, out_y2);

//printf("TrackCanvas::test_floatauto %d %d %d %d %d %d\n", cursor_x, cursor_y, x1, x2, y1, y2);
// Test value
	if(!ctrl_down() &&
		cursor_x >= x1 && 
		cursor_x < x2 && 
		cursor_y >= y1 && 
		cursor_y < y2)
	{
		if(buttonpress && (buttonpress != 3))
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = current->value_to_percentage();
			mwindow->session->drag_start_position = current->position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
			mwindow->session->drag_handle = 0;
		}
		result = 1;
	}
	else
// Test in control line
	if(ctrl_down() &&
		cursor_x >= in_x1 && 
		cursor_x < in_x2 && 
		cursor_y >= in_y1 && 
		cursor_y < in_y2 &&
		current->position > 0 &&
		current->mode == Auto::BEZIER)
	{
		if(buttonpress && (buttonpress != 3))
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = 
				current->invalue_to_percentage();
//			mwindow->session->drag_start_position = 
//				((FloatAuto*)current)->control_in_position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
			mwindow->session->drag_handle = 1;
		}
		result = 1;
	}
	else
// Test out control
	if(ctrl_down() &&
		cursor_x >= out_x1 && 
		cursor_x < out_x2 && 
		cursor_y >= out_y1 && 
		cursor_y < out_y2 &&
		current->mode == Auto::BEZIER)
	{
		if(buttonpress && (buttonpress != 3))
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = 
				current->outvalue_to_percentage();
//			mwindow->session->drag_start_position = 
//				((FloatAuto*)current)->control_out_position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
			mwindow->session->drag_handle = 2;
		}
		result = 1;
	}

// if(buttonpress)
// printf("TrackCanvas::test_floatauto 2 drag_handle=%d ctrl_down=%d cursor_x=%d cursor_y=%d x1=%d x2=%d y1=%d y2=%d\n", 
// mwindow->session->drag_handle,
// ctrl_down(),
// cursor_x,
// cursor_y,
// x1, x2, y1, y2);
	if(buttonpress && (buttonpress != 3) && result)
	{
		mwindow->undo->update_undo_before();
	}

	return result;
}


// Get the float value & y for position x on the canvas
#define X_TO_FLOATLINE(x) \
	int64_t position1 = (int64_t)(unit_start + x * zoom_units); \
	int64_t position2 = (int64_t)(unit_start + x * zoom_units) + 1; \
/* Call by reference fails for some reason here */ \
	float value1 = autos->get_value(position1, PLAY_FORWARD, previous1, next1); \
	float value2 = autos->get_value(position2, PLAY_FORWARD, previous1, next1); \
	double position = unit_start + x * zoom_units; \
	double value = 0; \
	if(position2 > position1) \
	{ \
		value = value1 + \
			(value2 - value1) * \
			(position - position1) / \
			(position2 - position1); \
	} \
	else \
	{ \
		value = value1; \
	} \
	int y = center_pixel + \
		(int)(((value - automation_min) / automation_range - 0.5) * -yscale);


void TrackCanvas::draw_floatline(int center_pixel, 
	FloatAuto *previous,
	FloatAuto *next,
	FloatAutos *autos,
	double unit_start,
	double zoom_units,
	double yscale,
	int x1,
	int y1,
	int x2,
	int y2,
	int *prev_y)
{
// Solve bezier equation for either every pixel or a certain large number of
// points.



// Not using slope intercept
	x1 = MAX(0, x1);




// Call by reference fails for some reason here
	FloatAuto *previous1 = previous, *next1 = next;
	float automation_min = mwindow->edl->local_session->automation_min;
	float automation_max = mwindow->edl->local_session->automation_max;
	float automation_range = automation_max - automation_min;

	for(int x = x1; x < x2; x++)
	{
// Interpolate value between frames
		X_TO_FLOATLINE(x)

		if(*prev_y == 0x7fffffff) *prev_y = y;
		if(/* x > x1 && */
			y >= center_pixel - yscale / 2 && 
			y < center_pixel + yscale / 2 - 1)
		{
// printf("TrackCanvas::draw_floatline y=%d min=%d max=%d\n",
// y,
// (int)(center_pixel - yscale / 2),
// (int)(center_pixel + yscale / 2 - 1));

 			draw_line(x - 1, *prev_y, x, y);
		}
		*prev_y = y;
	}


}





int TrackCanvas::test_floatline(int center_pixel, 
		FloatAutos *autos,
		double unit_start,
		double zoom_units,
		double yscale,
		int x1,
		int x2,
		int cursor_x, 
		int cursor_y, 
		int buttonpress)
{
	int result = 0;


	float automation_min = mwindow->edl->local_session->automation_min;
	float automation_max = mwindow->edl->local_session->automation_max;
	float automation_range = automation_max - automation_min;
	FloatAuto *previous1 = 0, *next1 = 0;
	X_TO_FLOATLINE(cursor_x);

	if(cursor_x >= x1 && 
		cursor_x < x2 &&
		cursor_y >= y - HANDLE_W / 2 && 
		cursor_y < y + HANDLE_W / 2 &&
		!ctrl_down())
	{
		result = 1;

// Menu
		if(buttonpress == 3)
		{
		}
		else
// Create keyframe
		if(buttonpress)
		{
			Auto *current;
			mwindow->undo->update_undo_before();
			current = mwindow->session->drag_auto = autos->insert_auto(position1);
			((FloatAuto*)current)->value = value;
			mwindow->session->drag_start_percentage = current->value_to_percentage();
			mwindow->session->drag_start_position = current->position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
			mwindow->session->drag_handle = 0;
		}
	}


	return result;
}


void TrackCanvas::synchronize_autos(float change, 
	Track *skip, 
	FloatAuto *fauto, 
	int fill_gangs)
{
// fill mwindow->session->drag_auto_gang
	if (fill_gangs == 1 && skip->gang)
	{
		for(Track *current = mwindow->edl->tracks->first;
			current;
			current = NEXT)
		{
			if(current->data_type == skip->data_type &&
				current->gang && 
				current->record && 
				current != skip)
			{
				FloatAutos *fade_autos = (FloatAutos*)current->automation->autos[AUTOMATION_FADE];
				double position = skip->from_units(fauto->position);
				FloatAuto *previous = 0, *next = 0;

				float init_value = fade_autos->get_value(fauto->position, PLAY_FORWARD, previous, next);
				FloatAuto *keyframe;
				keyframe = (FloatAuto*)fade_autos->get_auto_at_position(position);
				
				if (!keyframe)
				{
// create keyframe at exactly this point in time
					keyframe = (FloatAuto*)fade_autos->insert_auto(fauto->position);
					keyframe->value = init_value;
				} 
				else
				{ 
// keyframe exists, just change it
					keyframe->value += change;		
				} 
				
				keyframe->position = fauto->position;
//				keyframe->control_out_position = fauto->control_out_position;
//				keyframe->control_in_position = fauto->control_in_position;
				keyframe->control_out_value = fauto->control_out_value;
				keyframe->control_in_value = fauto->control_in_value;

				mwindow->session->drag_auto_gang->append((Auto *)keyframe);
			}
		}
	} else 
// move the gangs
	if (fill_gangs == 0)      
	{
// Move the gang!
		for (int i = 0; i < mwindow->session->drag_auto_gang->total; i++)
		{
			FloatAuto *keyframe = (FloatAuto *)mwindow->session->drag_auto_gang->values[i];
			
			keyframe->value += change;
			keyframe->position = fauto->position;
			if(skip->data_type == TRACK_AUDIO)
				CLAMP(keyframe->value, INFINITYGAIN, MAX_AUDIO_FADE);
			else
				CLAMP(keyframe->value, 0, MAX_VIDEO_FADE);
//			keyframe->control_out_position = fauto->control_out_position;
//			keyframe->control_in_position = fauto->control_in_position;
			keyframe->control_out_value = fauto->control_out_value;
			keyframe->control_in_value = fauto->control_in_value;
		} 

	} 
	else
// remove the gangs
	if (fill_gangs == -1)      
	{
		for (int i = 0; i < mwindow->session->drag_auto_gang->total; i++)
		{
			FloatAuto *keyframe = (FloatAuto *)mwindow->session->drag_auto_gang->values[i];
			keyframe->autos->remove_nonsequential(
					keyframe);
		} 
		mwindow->session->drag_auto_gang->remove_all();
	}
}


void TrackCanvas::draw_toggleline(int center_pixel, 
	int x1,
	int y1,
	int x2,
	int y2)
{
	draw_line(x1, center_pixel + y1, x2, center_pixel + y1);

	if(y2 != y1)
	{
		draw_line(x2, center_pixel + y1, x2, center_pixel + y2);
	}
}

int TrackCanvas::test_toggleline(Autos *autos,
	int center_pixel, 
	int x1,
	int y1,
	int x2,
	int y2, 
	int cursor_x, 
	int cursor_y, 
	int buttonpress)
{
	int result = 0;
	if(cursor_x >= x1 && cursor_x < x2)
	{
		int miny = center_pixel + y1 - HANDLE_W / 2;
		int maxy = center_pixel + y1 + HANDLE_W / 2;
		if(cursor_y >= miny && cursor_y < maxy) 
		{
			result = 1;

// Menu
			if(buttonpress == 3)
			{
			}
			else
// Insert keyframe
			if(buttonpress)
			{
				Auto *current;
				double position = (double)(cursor_x +
						mwindow->edl->local_session->view_start[pane->number]) * 
					mwindow->edl->local_session->zoom_sample / 
					mwindow->edl->session->sample_rate;
				int64_t unit_position = autos->track->to_units(position, 0);
				int new_value = (int)((IntAutos*)autos)->get_automation_constant(unit_position, unit_position);

				mwindow->undo->update_undo_before();

				current = mwindow->session->drag_auto = autos->insert_auto(unit_position);
				((IntAuto*)current)->value = new_value;
				mwindow->session->drag_start_percentage = current->value_to_percentage();
				mwindow->session->drag_start_position = current->position;
				mwindow->session->drag_origin_x = cursor_x;
				mwindow->session->drag_origin_y = cursor_y;
			}
		}
	};

	return result;
}

void TrackCanvas::calculate_viewport(Track *track, 
	double &view_start,   // Seconds
	double &unit_start,
	double &view_end,     // Seconds
	double &unit_end,
	double &yscale,
	int &center_pixel,
	double &zoom_sample,
	double &zoom_units)
{
	view_start = (double)mwindow->edl->local_session->view_start[pane->number] * 
		mwindow->edl->local_session->zoom_sample /
		mwindow->edl->session->sample_rate;
	unit_start = track->to_doubleunits(view_start);
	view_end = (double)(mwindow->edl->local_session->view_start[pane->number] + 
		get_w()) * 
		mwindow->edl->local_session->zoom_sample / 
		mwindow->edl->session->sample_rate;
	unit_end = track->to_doubleunits(view_end);
	yscale = mwindow->edl->local_session->zoom_track;
//printf("TrackCanvas::calculate_viewport yscale=%.0f\n", yscale);
	center_pixel = (int)(track->y_pixel - 
			mwindow->edl->local_session->track_start[pane->number] + 
			yscale / 2) + 
		(mwindow->edl->session->show_titles ? 
			mwindow->theme->get_image("title_bg_data")->get_h() : 
			0);
	zoom_sample = mwindow->edl->local_session->zoom_sample;

	zoom_units = track->to_doubleunits(zoom_sample / mwindow->edl->session->sample_rate);
}

float TrackCanvas::percentage_to_value(float percentage, 
	int is_toggle,
	Auto *reference)
{
	float result;
	if(is_toggle)
	{
		if(percentage > 0.5) 
			result = 1;
		else
			result = 0;
	}
	else
	{
		float automation_min = mwindow->edl->local_session->automation_min;
		float automation_max = mwindow->edl->local_session->automation_max;
		float automation_range = automation_max - automation_min;

		result = percentage * automation_range + automation_min;
		if(reference)
		{
			FloatAuto *ptr = (FloatAuto*)reference;
			result -= ptr->value;
		}
//printf("TrackCanvas::percentage_to_value %d %f\n", __LINE__, result);
	}
	return result;
}


void TrackCanvas::calculate_auto_position(double *x, 
	double *y,
	double *in_x,
	double *in_y,
	double *out_x,
	double *out_y,
	Auto *current,
	double unit_start,
	double zoom_units,
	double yscale)
{
	float automation_min = mwindow->edl->local_session->automation_min;
	float automation_max = mwindow->edl->local_session->automation_max;
	float automation_range = automation_max - automation_min;
	FloatAuto *ptr = (FloatAuto*)current;
	*x = (double)(ptr->position - unit_start) / zoom_units;
	*y = ((ptr->value - automation_min) /
		automation_range - 0.5) * 
		-yscale;

	if(in_x)
	{
//		if(!EQUIV(ptr->control_in_value, 0.0))
			*in_x = *x - mwindow->theme->control_pixels;
//		else
//			*in_x = *x;

// 		*in_x = (double)(ptr->position + 
// 			ptr->control_in_position - 
// 			unit_start) /
// 			zoom_units;
	}

	if(in_y)
	{
		*in_y = (((ptr->value + ptr->control_in_value) -
			automation_min) /
			automation_range - 0.5) *
			-yscale;
	}

	if(out_x)
	{
//		if(!EQUIV(ptr->control_out_value, 0.0))
			*out_x = *x + mwindow->theme->control_pixels;
//		else
//			*out_x = *x;

// 		*out_x = (double)(ptr->position + 
// 			ptr->control_out_position - 
// 			unit_start) /
// 			zoom_units;
	}

	if(out_y)
	{
		*out_y = (((ptr->value + ptr->control_out_value) -
			automation_min) /
			automation_range - 0.5) *
			-yscale;
	}
}





int TrackCanvas::do_float_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		int x_offset,
		int y_offset,
		int color,
		Auto* &auto_instance)
{
	int result = 0;

	double view_start;
	double unit_start;
	double view_end;
	double unit_end;
	double yscale;
	int center_pixel;
	double zoom_sample;
	double zoom_units;
	double ax, ay, ax2, ay2;
	double in_x2, in_y2, out_x2, out_y2;
	int draw_auto;
	double slope;
	int skip = 0;
	int prev_y1 = 0x7fffffff;
	int prev_y2 = 0x7fffffff;
	
	auto_instance = 0;

	if(draw) set_color(color);

	calculate_viewport(track, 
		view_start,
		unit_start,
		view_end,
		unit_end,
		yscale,
		center_pixel,
		zoom_sample,
		zoom_units);



// Get first auto before start
	Auto *current = 0;
	Auto *previous = 0;
	for(current = autos->last; 
		current && current->position >= unit_start; 
		current = PREVIOUS)
		;

	if(current)
	{
		calculate_auto_position(&ax, 
			&ay,
			0,
			0,
			0,
			0,
			current,
			unit_start,
			zoom_units,
			yscale);
		current = NEXT;
	}
	else
	{
		current = autos->first ? autos->first : autos->default_auto;
		if(current)
		{
			calculate_auto_position(&ax, 
				&ay,
				0,
				0,
				0,
				0,
				current,
				unit_start,
				zoom_units,
				yscale);
			ax = 0;
		}
		else
		{
			ax = 0;
			ay = 0;
		}
	}





	do
	{
		skip = 0;
		draw_auto = 1;

		if(current)
		{
			calculate_auto_position(&ax2, 
				&ay2,
				&in_x2,
				&in_y2,
				&out_x2,
				&out_y2,
				current,
				unit_start,
				zoom_units,
				yscale);
		}
		else
		{
			ax2 = get_w();
			ay2 = ay;
			skip = 1;
		}

		slope = (ay2 - ay) / (ax2 - ax);

		if(ax2 > get_w())
		{
			draw_auto = 0;
			ax2 = get_w();
			ay2 = ay + slope * (get_w() - ax);
		}
		
		if(ax < 0)
		{
			ay = ay + slope * (0 - ax);
			ax = 0;
		}














// Draw handle
		if(current && !result)
		{
			if(current != autos->default_auto)
			{
				if(!draw)
				{
					if(track->record)
						result = test_floatauto(current, 
							(int)ax2, 
							(int)ay2, 
							(int)in_x2,
							(int)in_y2,
							(int)out_x2,
							(int)out_y2,
							(int)center_pixel, 
							(int)yscale, 
							cursor_x, 
							cursor_y, 
							buttonpress);
					if (result) 
						auto_instance = current;
				}
				else
				if(draw_auto)
					draw_floatauto(current, 
						(int)ax2 + x_offset, 
						(int)ay2, 
						(int)in_x2 + x_offset,
						(int)in_y2,
						(int)out_x2 + x_offset,
						(int)out_y2,
						(int)center_pixel + y_offset, 
						(int)yscale);
			}
		}





// Draw joining line
		if(!draw)
		{
			if(!result)
			{
				if(track->record /* && buttonpress != 3 */)
				{
					result = test_floatline(center_pixel, 
						(FloatAutos*)autos,
						unit_start,
						zoom_units,
						yscale,
						(int)ax,
// Exclude auto coverage from the end of the line.  The auto overlaps
						(int)ax2 - HANDLE_W / 2,
						cursor_x, 
						cursor_y, 
						buttonpress);
				}
			}
		}
		else
			draw_floatline(center_pixel + y_offset,
				(FloatAuto*)previous,
				(FloatAuto*)current,
				(FloatAutos*)autos,
				unit_start,
				zoom_units,
				yscale,
				(int)ax, 
				(int)ay, 
				(int)ax2, 
				(int)ay2,
				&prev_y1);







		if(current)
		{
			previous = current;
			current = NEXT;
		}



		ax = ax2;
		ay = ay2;
	}while(current && 
		current->position <= unit_end && 
		!result);









	if(ax < get_w() && !result)
	{
		ax2 = get_w();
		ay2 = ay;
		if(!draw)
		{
			if(track->record /* && buttonpress != 3 */)
			{
				result = test_floatline(center_pixel, 
					(FloatAutos*)autos,
					unit_start,
					zoom_units,
					yscale,
					(int)ax,
					(int)ax2,
					cursor_x, 
					cursor_y, 
					buttonpress);
			}
		}
		else
			draw_floatline(center_pixel + y_offset, 
				(FloatAuto*)previous,
				(FloatAuto*)current,
				(FloatAutos*)autos,
				unit_start,
				zoom_units,
				yscale,
				(int)ax, 
				(int)ay, 
				(int)ax2, 
				(int)ay2,
				&prev_y2);
	}








	return result;
}


int TrackCanvas::do_int_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		int x_offset,
		int y_offset,
		int color,
		Auto * &auto_instance)
{
	int result = 0;
	double view_start;
	double unit_start;
	double view_end;
	double unit_end;
	double yscale;
	int center_pixel;
	double zoom_sample;
	double zoom_units;
	double ax, ay, ax2, ay2;

	auto_instance = 0;

	if(draw) set_color(color);

	calculate_viewport(track, 
		view_start,
		unit_start,
		view_end,
		unit_end,
		yscale,
		center_pixel,
		zoom_sample,
		zoom_units);


	double high = -yscale * 0.8 / 2;
	double low = yscale * 0.8 / 2;

// Get first auto before start
	Auto *current;
	for(current = autos->last; current && current->position >= unit_start; current = PREVIOUS)
		;

	if(current)
	{
		ax = 0;
		ay = ((IntAuto*)current)->value > 0 ? high : low;
		current = NEXT;
	}
	else
	{
		current = autos->first ? autos->first : autos->default_auto;
		if(current)
		{
			ax = 0;
			ay = ((IntAuto*)current)->value > 0 ? high : low;
		}
		else
		{
			ax = 0;
			ay = yscale;
		}
	}

	do
	{
		if(current)
		{
			ax2 = (double)(current->position - unit_start) / zoom_units;
			ay2 = ((IntAuto*)current)->value > 0 ? high : low;
		}
		else
		{
			ax2 = get_w();
			ay2 = ay;
		}

		if(ax2 > get_w()) ax2 = get_w();

	    if(current && !result) 
		{
			if(current != autos->default_auto)
			{
				if(!draw)
				{
					if(track->record)
					{
						result = test_auto(current, 
							(int)ax2, 
							(int)ay2, 
							(int)center_pixel, 
							(int)yscale, 
							cursor_x, 
							cursor_y, 
							buttonpress);
						if (result)
							auto_instance = current;
					}
				}
				else
					draw_auto(current, 
						(int)ax2 + x_offset, 
						(int)ay2 + y_offset, 
						(int)center_pixel, 
						(int)yscale);
			}

			current = NEXT;
		}

		if(!draw)
		{
			if(!result)
			{
				if(track->record /* && buttonpress != 3 */)
				{
					result = test_toggleline(autos, 
						center_pixel, 
						(int)ax, 
						(int)ay, 
						(int)ax2, 
						(int)ay2,
						cursor_x, 
						cursor_y, 
						buttonpress);
				}
			}
		}
		else
			draw_toggleline(center_pixel + y_offset, 
				(int)ax, 
				(int)ay, 
				(int)ax2, 
				(int)ay2);

		ax = ax2;
		ay = ay2;
	}while(current && current->position <= unit_end && !result);

	if(ax < get_w() && !result)
	{
		ax2 = get_w();
		ay2 = ay;
		if(!draw)
		{
			if(track->record /* && buttonpress != 3 */)
			{
				result = test_toggleline(autos,
					center_pixel, 
					(int)ax, 
					(int)ay, 
					(int)ax2, 
					(int)ay2,
					cursor_x, 
					cursor_y, 
					buttonpress);
			}
		}
		else
			draw_toggleline(center_pixel + y_offset, 
				(int)ax, 
				(int)ay, 
				(int)ax2, 
				(int)ay2);
	}
	return result;
}

int TrackCanvas::do_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		BC_Pixmap *pixmap,
		Auto * &auto_instance,
		int &rerender)
{
	int result = 0;

	double view_start;
	double unit_start;
	double view_end;
	double unit_end;
	double yscale;
	int center_pixel;
	double zoom_sample;
	double zoom_units;

	calculate_viewport(track, 
		view_start,
		unit_start,
		view_end,
		unit_end,
		yscale,
		center_pixel,
		zoom_sample,
		zoom_units);

	Auto *current;
	auto_instance = 0;

	for(current = autos->first; current && !result; current = NEXT)
	{
		if(current->position >= unit_start && current->position < unit_end)
		{
			int64_t x, y;
			x = (int64_t)((double)(current->position - unit_start) / 
				zoom_units - (pixmap->get_w() / 2 + 0.5));
			y = center_pixel - pixmap->get_h() / 2;

			if(!draw)
			{
				if(cursor_x >= x && cursor_y >= y &&
					cursor_x < x + pixmap->get_w() &&
					cursor_y < y + pixmap->get_h())
				{
					result = 1;
					auto_instance = current;

					if(buttonpress && (buttonpress != 3))
					{
						mwindow->session->drag_auto = current;
						mwindow->session->drag_start_position = current->position;
						mwindow->session->drag_origin_x = cursor_x;
						mwindow->session->drag_origin_y = cursor_y;

						double position = autos->track->from_units(current->position);
						double center = (mwindow->edl->local_session->get_selectionstart(1) +
							mwindow->edl->local_session->get_selectionend(1)) / 
							2;

						if(!shift_down())
						{
							mwindow->edl->local_session->set_selectionstart(position);
							mwindow->edl->local_session->set_selectionend(position);
						}
						else
						if(position < center)
						{
							mwindow->edl->local_session->set_selectionstart(position);
						}
						else
							mwindow->edl->local_session->set_selectionend(position);

						rerender = 1;
					}
				}
			}
			else
				draw_pixmap(pixmap, x, y);
		}
	}


	return result;
}

// so this means it is always >0 when keyframe is found 
int TrackCanvas::do_plugin_autos(Track *track, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		Plugin* &keyframe_plugin,
		KeyFrame* &keyframe_instance)
{
	int result = 0;

	double view_start;
	double unit_start;
	double view_end;
	double unit_end;
	double yscale;
	int center_pixel;
	double zoom_sample;
	double zoom_units;

	if(!track->expand_view) return 0;

	calculate_viewport(track, 
		view_start,
		unit_start,
		view_end,
		unit_end,
		yscale,
		center_pixel,
		zoom_sample,
		zoom_units);



	for(int i = 0; i < track->plugin_set.total && !result; i++)
	{
		PluginSet *plugin_set = track->plugin_set.values[i];
		int center_pixel = (int)(track->y_pixel - 
			mwindow->edl->local_session->track_start[pane->number] + 
			mwindow->edl->local_session->zoom_track +
			(i + 0.5) * mwindow->theme->get_image("plugin_bg_data")->get_h() + 
			(mwindow->edl->session->show_titles ? mwindow->theme->get_image("title_bg_data")->get_h() : 0));

		for(Plugin *plugin = (Plugin*)plugin_set->first; 
			plugin && !result; 
			plugin = (Plugin*)plugin->next)
		{
			for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first; 
				keyframe && !result; 
				keyframe = (KeyFrame*)keyframe->next)
			{
//printf("TrackCanvas::draw_plugin_autos 3 %d\n", keyframe->position);
				if(keyframe->position >= unit_start && keyframe->position < unit_end)
				{
					int64_t x = (int64_t)((keyframe->position - unit_start) / zoom_units);
					int y = center_pixel - keyframe_pixmap->get_h() / 2;

//printf("TrackCanvas::draw_plugin_autos 4 %d %d\n", x, center_pixel);
					if(!draw)
					{
						if(cursor_x >= x && cursor_y >= y &&
							cursor_x < x + keyframe_pixmap->get_w() &&
							cursor_y < y + keyframe_pixmap->get_h())
						{
							result = 1;
							keyframe_plugin = plugin;
							keyframe_instance = keyframe;

							if(buttonpress)
							{
								mwindow->session->drag_auto = keyframe;
								mwindow->session->drag_start_position = keyframe->position;
								mwindow->session->drag_origin_x = cursor_x;
								mwindow->session->drag_origin_y = cursor_y;

								double position = track->from_units(keyframe->position);
								double center = (mwindow->edl->local_session->get_selectionstart(1) +
									mwindow->edl->local_session->get_selectionend(1)) / 
									2;

								if(!shift_down())
								{
									mwindow->edl->local_session->set_selectionstart(position);
									mwindow->edl->local_session->set_selectionend(position);
								}
								else
								if(position < center)
								{
									mwindow->edl->local_session->set_selectionstart(position);
								}
								else
									mwindow->edl->local_session->set_selectionend(position);
							}
						}
					}
					else
						draw_pixmap(keyframe_pixmap, 
							x, 
							y);
				}
			}
		}
	}



// 	if(buttonpress && buttonpress != 3 && result)
// 	{
// 		mwindow->undo->update_undo_before();
// 	}

	return result;
}

void TrackCanvas::draw_overlays()
{
	int new_cursor, update_cursor, rerender;

// Move background pixmap to foreground pixmap
	draw_pixmap(background_pixmap, 
		0, 
		0,
		get_w(),
		get_h(),
		0,
		0);

// In/Out points
	draw_inout_points();

// Transitions
	if(mwindow->edl->session->auto_conf->transitions) draw_transitions();

// Plugins
	draw_plugins();

// Loop points
	draw_loop_points();
	draw_brender_range();

// Highlighted areas
	draw_highlighting();

// Automation
	do_keyframes(0, 
		0, 
		1, 
		0, 
		new_cursor, 
		update_cursor,
		rerender);

// Selection cursor
	if(pane->cursor) pane->cursor->restore(1);

// Handle dragging
	draw_drag_handle();

// Playback cursor
	draw_playback_cursor();

	show_window(0);
}

int TrackCanvas::activate()
{
	if(!active)
	{
//printf("TrackCanvas::activate %d %d\n", __LINE__, pane->number);
//BC_Signals::dump_stack();
		get_top_level()->deactivate();
		active = 1;
		set_active_subwindow(this);
		pane->cursor->activate();
		gui->focused_pane = pane->number;
	}
	return 0;
}

int TrackCanvas::deactivate()
{
	if(active)
	{
		active = 0;
		pane->cursor->deactivate();
	}
	return 0;
}


void TrackCanvas::update_drag_handle()
{
	double new_position;

	new_position = 
		(double)(get_cursor_x() + 
		mwindow->edl->local_session->view_start[pane->number]) *
		mwindow->edl->local_session->zoom_sample /
		mwindow->edl->session->sample_rate;
	new_position = 
		mwindow->edl->align_to_frame(new_position, 0);


	if(new_position != mwindow->session->drag_position)
	{
		mwindow->session->drag_position = new_position;
		gui->mainclock->update(new_position);
		
		
		timebar_position = new_position;
		gui->update_timebar(0);
// Que the CWindow.  Doesn't do anything if selectionstart and selection end 
// aren't changed.
//		mwindow->cwindow->update(1, 0, 0);
	}
}

int TrackCanvas::update_drag_edit()
{
	int result = 0;
	
	
	
	return result;
}

int TrackCanvas::get_drag_values(float *percentage, 
	int64_t *position,
	int do_clamp,
	int cursor_x,
	int cursor_y,
	Auto *current)
{
	int x = cursor_x - mwindow->session->drag_origin_x;
	int y = cursor_y - mwindow->session->drag_origin_y;
	*percentage = 0;
	*position = 0;

	if(!current->autos->track->record) return 1;
	double view_start;
	double unit_start;
	double view_end;
	double unit_end;
	double yscale;
	int center_pixel;
	double zoom_sample;
	double zoom_units;

	calculate_viewport(current->autos->track, 
		view_start,
		unit_start,
		view_end,
		unit_end,
		yscale,
		center_pixel,
		zoom_sample,
		zoom_units);

	*percentage = (float)(mwindow->session->drag_origin_y - cursor_y) /
		yscale + 
		mwindow->session->drag_start_percentage;
	if(do_clamp) CLAMP(*percentage, 0, 1);

	*position = Units::to_int64(zoom_units *
		(cursor_x - mwindow->session->drag_origin_x) +
		mwindow->session->drag_start_position + 0.5);

	if((do_clamp) && *position < 0) *position = 0;
	return 0;
}










int TrackCanvas::update_drag_floatauto(int cursor_x, int cursor_y)
{
	FloatAuto *current = (FloatAuto*)mwindow->session->drag_auto;
	float value;
	float old_value;
	float percentage;
	int64_t position;
	int result = 0;

	if(get_drag_values(&percentage, 
		&position,
		mwindow->session->drag_handle == 0,
		cursor_x,
		cursor_y,
		current)) return 0;

//printf("TrackCanvas::update_drag_floatauto %d %f\n", __LINE__, percentage);


	switch(mwindow->session->drag_handle)
	{
// Center
		case 0:
// Snap to nearby values
			old_value = current->value;
			if(shift_down())
			{
				double value1;
				double distance1;
				double value2;
				double distance2;
				value = percentage_to_value(percentage, 0, 0);

				if(current->previous)
				{
					value1 = ((FloatAuto*)current->previous)->value;
					distance1 = fabs(value - value1);
					current->value = value1;
				}

				if(current->next)
				{
					value2 = ((FloatAuto*)current->next)->value;
					distance2 = fabs(value - value2);
					if(!current->previous || distance2 < distance1)
					{
						current->value = value2;
					}
				}

				if(!current->previous && !current->next)
				{
					current->value = ((FloatAutos*)current->autos)->default_;
				}
				value = current->value;
			}
			else
				value = percentage_to_value(percentage, 0, 0);

			if(value != old_value || position != current->position)
			{
				result = 1;
				float change = value - old_value;		
				current->value = value;
				current->position = position;
				synchronize_autos(change, current->autos->track, current, 0);

				char string[BCTEXTLEN], string2[BCTEXTLEN];
				Units::totext(string2, 
					current->autos->track->from_units(current->position),
					mwindow->edl->session->time_format,
					mwindow->edl->session->sample_rate,
					mwindow->edl->session->frame_rate,
					mwindow->edl->session->frames_per_foot);
				sprintf(string, "%s, %.2f", string2, current->value);
				gui->show_message(string);
			}
			break;

// In control
		case 1:
			value = percentage_to_value(percentage, 0, current);
			position = MIN(0, position);
			if(value != current->control_in_value)
			{
				result = 1;
				current->control_in_value = value;
				synchronize_autos(0, current->autos->track, current, 0);

				char string[BCTEXTLEN], string2[BCTEXTLEN];
				sprintf(string, "%.2f", current->control_in_value);
				gui->show_message(string);
			}
			break;

// Out control
		case 2:
			value = percentage_to_value(percentage, 0, current);
			position = MAX(0, position);
			if(value != current->control_out_value)
			{
				result = 1;
				current->control_out_value = value;
				synchronize_autos(0, current->autos->track, current, 0);

				char string[BCTEXTLEN], string2[BCTEXTLEN];
 				sprintf(string, "%.2f", 
 					((FloatAuto*)current)->control_out_value);
				gui->show_message(string);
			}
			break;
	}

	return result;
}

int TrackCanvas::update_drag_toggleauto(int cursor_x, int cursor_y)
{
	IntAuto *current = (IntAuto*)mwindow->session->drag_auto;
	float percentage;
	int64_t position;
	int result = 0;

	if(get_drag_values(&percentage, 
		&position,
		1,
		cursor_x,
		cursor_y,
		current)) return 0;


	int value = (int)percentage_to_value(percentage, 1, 0);

	if(value != current->value || position != current->position)
	{
		result = 1;
		current->value = value;
		current->position = position;

		char string[BCTEXTLEN], string2[BCTEXTLEN];
		Units::totext(string2, 
			current->autos->track->from_units(current->position),
			mwindow->edl->session->time_format,
			mwindow->edl->session->sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
		sprintf(string, "%s, %d", string2, current->value);
		gui->show_message(string);
	}

	return result;
}

// Autos which can't change value through dragging.

int TrackCanvas::update_drag_auto(int cursor_x, int cursor_y)
{
	Auto *current = (Auto*)mwindow->session->drag_auto;
	float percentage;
	int64_t position;
	int result = 0;

	if(get_drag_values(&percentage, 
		&position,
		1,
		cursor_x,
		cursor_y,
		current)) return 0;


	if(position != current->position)
	{
		result = 1;
		current->position = position;

		char string[BCTEXTLEN];
		Units::totext(string, 
			current->autos->track->from_units(current->position),
			mwindow->edl->session->time_format,
			mwindow->edl->session->sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
		gui->show_message(string);

		double position_f = current->autos->track->from_units(current->position);
		double center_f = (mwindow->edl->local_session->get_selectionstart(1) +
			mwindow->edl->local_session->get_selectionend(1)) / 
			2;
		if(!shift_down())
		{
			mwindow->edl->local_session->set_selectionstart(position_f);
			mwindow->edl->local_session->set_selectionend(position_f);
		}
		else
		if(position_f < center_f)
		{
			mwindow->edl->local_session->set_selectionstart(position_f);
		}
		else
			mwindow->edl->local_session->set_selectionend(position_f);
	}


	return result;
}

int TrackCanvas::update_drag_pluginauto(int cursor_x, int cursor_y)
{
	KeyFrame *current = (KeyFrame*)mwindow->session->drag_auto;
	float percentage;
	int64_t position;
	int result = 0;

	if(get_drag_values(&percentage, 
		&position,
		1,
		cursor_x,
		cursor_y,
		current)) return 0;

	if(position != current->position)
	{
//	printf("uida: autos: %p, track: %p ta: %p\n", current->autos, current->autos->track, current->autos->track->automation);
		Track *track = current->autos->track;
		PluginAutos *pluginautos = (PluginAutos *)current->autos;
		PluginSet *pluginset;
		Plugin *plugin;
// figure out the correct pluginset & correct plugin 
		int found = 0;
		for(int i = 0; i < track->plugin_set.total; i++)
		{
			pluginset = track->plugin_set.values[i];
			for(plugin = (Plugin *)pluginset->first; plugin; plugin = (Plugin *)plugin->next)
			{
				KeyFrames *keyframes = plugin->keyframes;
				for(KeyFrame *currentkeyframe = (KeyFrame *)keyframes->first; currentkeyframe; currentkeyframe = (KeyFrame *) currentkeyframe->next)
				{
					if (currentkeyframe == current) 
					{
						found = 1;
						break;
					}
 
				}
				if (found) break;			
			}
			if (found) break;			
		}	
	
		mwindow->session->plugin_highlighted = plugin;
		mwindow->session->track_highlighted = track;
		result = 1;
		current->position = position;

		char string[BCTEXTLEN];
		Units::totext(string, 
			current->autos->track->from_units(current->position),
			mwindow->edl->session->time_format,
			mwindow->edl->session->sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
		gui->show_message(string);

		double position_f = current->autos->track->from_units(current->position);
		double center_f = (mwindow->edl->local_session->get_selectionstart(1) +
			mwindow->edl->local_session->get_selectionend(1)) / 
			2;
		if(!shift_down())
		{
			mwindow->edl->local_session->set_selectionstart(position_f);
 			mwindow->edl->local_session->set_selectionend(position_f);
		}
		else
		if(position_f < center_f)
		{
			mwindow->edl->local_session->set_selectionstart(position_f);
		}
		else
			mwindow->edl->local_session->set_selectionend(position_f);
	}


	return result;
}

void TrackCanvas::update_drag_caption()
{
	switch(mwindow->session->current_operation)
	{
		case DRAG_FADE:
			
			break;
	}
}



int TrackCanvas::cursor_motion_event()
{
	int result, cursor_x, cursor_y;
	int update_clock = 0;
	int update_zoom = 0;
	int update_scroll = 0;
	int update_overlay = 0;
	int update_cursor = 0;
	int new_cursor = 0;
	int rerender = 0;
	double position = 0;
//printf("TrackCanvas::cursor_motion_event %d\n", __LINE__);
	result = 0;

// Default cursor
	switch(mwindow->edl->session->editing_mode)
	{
		case EDITING_ARROW: new_cursor = ARROW_CURSOR; break;
		case EDITING_IBEAM: new_cursor = IBEAM_CURSOR; break;
	}

	switch(mwindow->session->current_operation)
	{
		case DRAG_EDITHANDLE1:
// Outside threshold.  Upgrade status
			if(active)
			{
				if(labs(get_cursor_x() - mwindow->session->drag_origin_x) > HANDLE_W)
				{
					mwindow->session->current_operation = DRAG_EDITHANDLE2;
					update_overlay = 1;
				}
			}
			break;

		case DRAG_EDITHANDLE2:
			if(active)
			{
				update_drag_handle();
				update_overlay = 1;
			}
			break;

		case DRAG_PLUGINHANDLE1:
			if(active)
			{
				if(labs(get_cursor_x() - mwindow->session->drag_origin_x) > HANDLE_W)
				{
					mwindow->session->current_operation = DRAG_PLUGINHANDLE2;
					update_overlay = 1;
				}
			}
			break;

		case DRAG_PLUGINHANDLE2:
			if(active)
			{
				update_drag_handle();
				update_overlay = 1;
			}
			break;

// Rubber band curves
		case DRAG_FADE:
		case DRAG_SPEED:
		case DRAG_CZOOM:
		case DRAG_PZOOM:
		case DRAG_CAMERA_X:
		case DRAG_CAMERA_Y:
		case DRAG_CAMERA_Z:
		case DRAG_PROJECTOR_X:
		case DRAG_PROJECTOR_Y:
		case DRAG_PROJECTOR_Z:
			if(active) rerender = 
				update_overlay =
				update_drag_floatauto(get_cursor_x(), get_cursor_y());
			break;

		case DRAG_PLAY:
			if(active) rerender = 
				update_overlay =
				update_drag_toggleauto(get_cursor_x(), get_cursor_y());
			break;

		case DRAG_MUTE:
			if(active) rerender = 
				update_overlay = 
				update_drag_toggleauto(get_cursor_x(), get_cursor_y());
			break;

// Keyframe icons are sticky
		case DRAG_PAN_PRE:
		case DRAG_MASK_PRE:
		case DRAG_MODE_PRE:
		case DRAG_PLUGINKEY_PRE:
			if(active) 
			{
				if(labs(get_cursor_x() - mwindow->session->drag_origin_x) > HANDLE_W)
				{
					mwindow->session->current_operation++;
					update_overlay = 1;

					mwindow->undo->update_undo_before();
				}
			}
			break;

		case DRAG_PAN:
		case DRAG_MASK:
		case DRAG_MODE:
			if(active) rerender = 
				update_overlay = 
				update_drag_auto(get_cursor_x(), get_cursor_y());
			break;

 		case DRAG_PLUGINKEY:
 			if(active) rerender = 
				update_overlay = 
 				update_drag_pluginauto(get_cursor_x(), get_cursor_y());
 			break;

		case SELECT_REGION:
			if(active) 
			{
				cursor_x = get_cursor_x();
				cursor_y = get_cursor_y();
				position = (double)(cursor_x + mwindow->edl->local_session->view_start[pane->number]) * 
					mwindow->edl->local_session->zoom_sample /
					mwindow->edl->session->sample_rate;

				position = mwindow->edl->align_to_frame(position, 0);
				position = MAX(position, 0);

				if(position < selection_midpoint)
				{
					mwindow->edl->local_session->set_selectionend(selection_midpoint);
					mwindow->edl->local_session->set_selectionstart(position);
	// Que the CWindow
					gui->unlock_window();
					mwindow->cwindow->update(1, 0, 0, 0, 1);
					gui->lock_window("TrackCanvas::cursor_motion_event 1");
	// Update the faders
					mwindow->update_plugin_guis();
					gui->update_patchbay();
				}
				else
				{
					mwindow->edl->local_session->set_selectionstart(selection_midpoint);
					mwindow->edl->local_session->set_selectionend(position);
	// Don't que the CWindow
				}

				timebar_position = mwindow->edl->local_session->get_selectionend(1);

				gui->hide_cursor(0);
				gui->draw_cursor(1);
				gui->update_timebar(0);
				gui->flash_canvas(1);
				result = 1;
				update_clock = 1;
				update_zoom = 1;
				update_scroll = 1;
			}
			break;

		default:
			if(is_event_win() && cursor_inside())
			{
// Update clocks
				cursor_x = get_cursor_x();
				position = (double)cursor_x * 
					(double)mwindow->edl->local_session->zoom_sample / 
					(double)mwindow->edl->session->sample_rate + 
					(double)mwindow->edl->local_session->view_start[pane->number] * 
					(double)mwindow->edl->local_session->zoom_sample / 
					(double)mwindow->edl->session->sample_rate;
				position = mwindow->edl->align_to_frame(position, 0);
				update_clock = 1;

// set all timebars
				for(int i = 0; i < TOTAL_PANES; i++)
					if(gui->pane[i]) gui->pane[i]->canvas->timebar_position = position;

//printf("TrackCanvas::cursor_motion_event %d %d %p %p\n", __LINE__, pane->number, pane, pane->timebar);
				gui->update_timebar(0);

// Update cursor
				if(do_transitions(get_cursor_x(), 
						get_cursor_y(), 
						0, 
						new_cursor, 
						update_cursor))
				{
					break;
				}
				else
// Update cursor
				if(do_keyframes(get_cursor_x(), 
					get_cursor_y(), 
					0, 
					0, 
					new_cursor,
					update_cursor,
					rerender))
				{
					break;
				}
				else
// Edit boundaries
				if(do_edit_handles(get_cursor_x(), 
					get_cursor_y(), 
					0, 
					new_cursor,
					update_cursor))
				{
					break;
				}
				else
// Plugin boundaries
				if(do_plugin_handles(get_cursor_x(), 
					get_cursor_y(), 
					0, 
					new_cursor,
					update_cursor))
				{
					break;
				}
				else
				if(do_edits(get_cursor_x(), 
					get_cursor_y(), 
					0, 
					0, 
					update_overlay, 
					rerender,
					new_cursor,
					update_cursor))
				{
					break;
				}
			}
			break;
	}

//printf("TrackCanvas::cursor_motion_event 1\n");
	if(update_cursor && new_cursor != get_cursor())
	{
		set_cursor(new_cursor, 0, 1);
	}

//printf("TrackCanvas::cursor_motion_event 1 %d\n", rerender);
	if(rerender)
	{
		mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_PARAMS);
		mwindow->update_plugin_guis();
		gui->unlock_window();
		mwindow->cwindow->update(1, 0, 0, 0, 1);
		gui->lock_window("TrackCanvas::cursor_motion_event 2");
// Update faders
		gui->update_patchbay();
	}


	if(update_clock)
	{
		if(!mwindow->cwindow->playback_engine->is_playing_back)
			gui->mainclock->update(position);
	}

	if(update_zoom)
	{
		gui->zoombar->update();
	}

	if(update_scroll)
	{
		if(!drag_scroll && 
			(cursor_x >= get_w() || cursor_x < 0 || cursor_y >= get_h() || cursor_y < 0))
			start_dragscroll();
		else
		if(drag_scroll &&
			(cursor_x < get_w() && cursor_x >= 0 && cursor_y < get_h() && cursor_y >= 0))
			stop_dragscroll();
	}

	if(update_overlay)
	{
		gui->draw_overlays(1);
	}


//printf("TrackCanvas::cursor_motion_event %d\n", __LINE__);
	return result;
}

void TrackCanvas::start_dragscroll()
{
	if(!drag_scroll)
	{
		drag_scroll = 1;
		set_repeat(BC_WindowBase::get_resources()->scroll_repeat);
//printf("TrackCanvas::start_dragscroll 1\n");
	}
}

void TrackCanvas::stop_dragscroll()
{
	if(drag_scroll)
	{
		drag_scroll = 0;
		unset_repeat(BC_WindowBase::get_resources()->scroll_repeat);
//printf("TrackCanvas::stop_dragscroll 1\n");
	}
}

int TrackCanvas::repeat_event(int64_t duration)
{
	if(!drag_scroll) return 0;
	if(duration != BC_WindowBase::get_resources()->scroll_repeat) return 0;

	int sample_movement = 0;
	int track_movement = 0;
	int64_t x_distance = 0;
	int64_t y_distance = 0;
	double position = 0;
	int result = 0;

	switch(mwindow->session->current_operation)
	{
		case SELECT_REGION:
//printf("TrackCanvas::repeat_event 1 %d\n", mwindow->edl->local_session->view_start);
			if(get_cursor_x() > get_w())
			{
				x_distance = get_cursor_x() - get_w();
				sample_movement = 1;
			}
			else
			if(get_cursor_x() < 0)
			{
				x_distance = get_cursor_x();
				sample_movement = 1;
			}

			if(get_cursor_y() > get_h())
			{
				y_distance = get_cursor_y() - get_h();
				track_movement = 1;
			}
			else
			if(get_cursor_y() < 0)
			{
				y_distance = get_cursor_y();
				track_movement = 1;
			}
			result = 1;
			break;
	}


	if(sample_movement)
	{
		position = (double)(get_cursor_x() + 
			mwindow->edl->local_session->view_start[pane->number] + 
			x_distance) * 
			mwindow->edl->local_session->zoom_sample /
			mwindow->edl->session->sample_rate;
		position = mwindow->edl->align_to_frame(position, 0);
		position = MAX(position, 0);

//printf("TrackCanvas::repeat_event 1 %f\n", position);
		switch(mwindow->session->current_operation)
		{
			case SELECT_REGION:
				if(position < selection_midpoint)
				{
					mwindow->edl->local_session->set_selectionend(selection_midpoint);
					mwindow->edl->local_session->set_selectionstart(position);
// Que the CWindow
					gui->unlock_window();
					mwindow->cwindow->update(1, 0, 0);
					gui->lock_window("TrackCanvas::repeat_event");
// Update the faders
					mwindow->update_plugin_guis();
					gui->update_patchbay();
				}
				else
				{
					mwindow->edl->local_session->set_selectionstart(selection_midpoint);
					mwindow->edl->local_session->set_selectionend(position);
// Don't que the CWindow
				}
				break;
		}

		mwindow->samplemovement(
			mwindow->edl->local_session->view_start[pane->number] + x_distance,
			pane->number);

	}

	if(track_movement)
	{
		mwindow->trackmovement(y_distance, pane->number);
	}

	return result;
}

int TrackCanvas::button_release_event()
{
	int redraw = 0, update_overlay = 0, result = 0;

// printf("TrackCanvas::button_release_event %d\n", 
// mwindow->session->current_operation);
	if(active)
	{
		switch(mwindow->session->current_operation)
		{
			case DRAG_EDITHANDLE2:
				mwindow->session->current_operation = NO_OPERATION;
				drag_scroll = 0;
				result = 1;

				end_edithandle_selection();
				break;

			case DRAG_EDITHANDLE1:
				mwindow->session->current_operation = NO_OPERATION;
				drag_scroll = 0;
				result = 1;
				break;

			case DRAG_PLUGINHANDLE2:
				mwindow->session->current_operation = NO_OPERATION;
				drag_scroll = 0;
				result = 1;

				end_pluginhandle_selection();
				break;

			case DRAG_PLUGINHANDLE1:
				mwindow->session->current_operation = NO_OPERATION;
				drag_scroll = 0;
				result = 1;
				break;

			case DRAG_FADE:
			case DRAG_SPEED:
	// delete the drag_auto_gang first and remove out of order keys
				synchronize_autos(0, 0, 0, -1); 
			case DRAG_CZOOM:
			case DRAG_PZOOM:
			case DRAG_PLAY:
			case DRAG_MUTE:
			case DRAG_MASK:
			case DRAG_MODE:
			case DRAG_PAN:
			case DRAG_CAMERA_X:
			case DRAG_CAMERA_Y:
			case DRAG_CAMERA_Z:
			case DRAG_PROJECTOR_X:
			case DRAG_PROJECTOR_Y:
			case DRAG_PROJECTOR_Z:
			case DRAG_PLUGINKEY:
				mwindow->session->current_operation = NO_OPERATION;
				mwindow->session->drag_handle = 0;
	// Remove any out-of-order keyframe
				if(mwindow->session->drag_auto)
				{
					mwindow->session->drag_auto->autos->remove_nonsequential(
						mwindow->session->drag_auto);
	//				mwindow->session->drag_auto->autos->optimize();
					update_overlay = 1;
				}


				mwindow->undo->update_undo_after(_("keyframe"), LOAD_AUTOMATION);
				result = 1;
				break;

			case DRAG_EDIT:
			case DRAG_AEFFECT_COPY:
			case DRAG_VEFFECT_COPY:
	// Trap in drag stop

				break;


			default:
				if(mwindow->session->current_operation)
				{
	//				if(mwindow->session->current_operation == SELECT_REGION)
	//				{
	//					mwindow->undo->update_undo_after(_("select"), LOAD_SESSION, 0, 0);
	//				}

					mwindow->session->current_operation = NO_OPERATION;
					drag_scroll = 0;
	// Traps button release events
	//				result = 1;
				}
				break;
		}
	}
	
	if (result) 
		cursor_motion_event();

	if(update_overlay)
	{
		gui->draw_overlays(1);
	}
	if(redraw)
	{
		gui->draw_canvas(NORMAL_DRAW, 0);
	}
	return result;
}

int TrackCanvas::do_edit_handles(int cursor_x, 
	int cursor_y, 
	int button_press, 
	int &new_cursor,
	int &update_cursor)
{
	Edit *edit_result = 0;
	int handle_result = 0;
	int result = 0;

	if(!mwindow->edl->session->show_assets) return 0;

	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit && !result;
			edit = edit->next)
		{
			int64_t edit_x, edit_y, edit_w, edit_h;
			edit_dimensions(edit, edit_x, edit_y, edit_w, edit_h);

			if(cursor_x >= edit_x && cursor_x <= edit_x + edit_w &&
				cursor_y >= edit_y && cursor_y < edit_y + edit_h)
			{
				if(cursor_x < edit_x + HANDLE_W)
				{
					edit_result = edit;
					handle_result = 0;
					result = 1;
				}
				else
				if(cursor_x >= edit_x + edit_w - HANDLE_W)
				{
					edit_result = edit;
					handle_result = 1;
					result = 1;
				}
				else
				{
					result = 0;
				}
			}
		}
	}

	update_cursor = 1;
	if(result)
	{
		double position;
		if(handle_result == 0)
		{
			position = edit_result->track->from_units(edit_result->startproject);
			new_cursor = LEFT_CURSOR;
		}
		else
		if(handle_result == 1)
		{
			position = edit_result->track->from_units(edit_result->startproject + edit_result->length);
			new_cursor = RIGHT_CURSOR;
		}

// Reposition cursor
		if(button_press)
		{
			mwindow->session->drag_edit = edit_result;
			mwindow->session->drag_handle = handle_result;
			mwindow->session->drag_button = get_buttonpress() - 1;
			mwindow->session->drag_position = position;
			mwindow->session->current_operation = DRAG_EDITHANDLE1;
			mwindow->session->drag_origin_x = get_cursor_x();
			mwindow->session->drag_origin_y = get_cursor_y();
			mwindow->session->drag_start = position;

			int rerender = start_selection(position);
			if(rerender)
			{
				gui->unlock_window();
				mwindow->cwindow->update(1, 0, 0);
				gui->lock_window("TrackCanvas::do_edit_handles");
			}
			gui->update_timebar(0);
			gui->zoombar->update();
			gui->hide_cursor(0);
			gui->draw_cursor(1);
			draw_overlays();
			flash(0);
		}
	}

	return result;
}

int TrackCanvas::do_plugin_handles(int cursor_x, 
	int cursor_y, 
	int button_press,
	int &new_cursor,
	int &update_cursor)
{
	Plugin *plugin_result = 0;
	int handle_result = 0;
	int result = 0;

//	if(!mwindow->edl->session->show_assets) return 0;

	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(int i = 0; i < track->plugin_set.total && !result; i++)
		{
			PluginSet *plugin_set = track->plugin_set.values[i];
			for(Plugin *plugin = (Plugin*)plugin_set->first; 
				plugin && !result; 
				plugin = (Plugin*)plugin->next)
			{
				int64_t plugin_x, plugin_y, plugin_w, plugin_h;
				plugin_dimensions(plugin, plugin_x, plugin_y, plugin_w, plugin_h);

				if(cursor_x >= plugin_x && cursor_x <= plugin_x + plugin_w &&
					cursor_y >= plugin_y && cursor_y < plugin_y + plugin_h)
				{
					if(cursor_x < plugin_x + HANDLE_W)
					{
						plugin_result = plugin;
						handle_result = 0;
						result = 1;
					}
					else
					if(cursor_x >= plugin_x + plugin_w - HANDLE_W)
					{
						plugin_result = plugin;
						handle_result = 1;
						result = 1;
					}
				}
			}

			if(result && shift_down())
				mwindow->session->trim_edits = plugin_set;
		}
	}

	update_cursor = 1;
	if(result)
	{
		double position;
		if(handle_result == 0)
		{
			position = plugin_result->track->from_units(plugin_result->startproject);
			new_cursor = LEFT_CURSOR;
		}
		else
		if(handle_result == 1)
		{
			position = plugin_result->track->from_units(plugin_result->startproject + plugin_result->length);
			new_cursor = RIGHT_CURSOR;
		}
		
		if(button_press)
		{
			mwindow->session->drag_plugin = plugin_result;
			mwindow->session->drag_handle = handle_result;
			mwindow->session->drag_button = get_buttonpress() - 1;
			mwindow->session->drag_position = position;
			mwindow->session->current_operation = DRAG_PLUGINHANDLE1;
			mwindow->session->drag_origin_x = get_cursor_x();
			mwindow->session->drag_origin_y = get_cursor_y();
			mwindow->session->drag_start = position;

			int rerender = start_selection(position);
			if(rerender) 
			{
				gui->unlock_window();
				mwindow->cwindow->update(1, 0, 0);
				gui->lock_window("TrackCanvas::do_plugin_handles");
			}
			gui->update_timebar(0);
			gui->zoombar->update();
			gui->hide_cursor(0);
			gui->draw_cursor(1);
			gui->draw_overlays(0);
			gui->flash_canvas(1);
		}
	}
	
	return result;
}


int TrackCanvas::do_tracks(int cursor_x, 
		int cursor_y,
		int button_press)
{
	int result = 0;


//	if(!mwindow->edl->session->show_assets) return 0;


	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		int64_t track_x, track_y, track_w, track_h;
		track_dimensions(track, track_x, track_y, track_w, track_h);

		if(button_press && 
			get_buttonpress() == RIGHT_BUTTON &&
			cursor_y >= track_y && 
			cursor_y < track_y + track_h)
		{
			gui->edit_menu->update(track, 0);
			gui->edit_menu->activate_menu();
			result = 1;
		}
	}

	return result;
}

int TrackCanvas::do_edits(int cursor_x, 
	int cursor_y, 
	int button_press,
	int drag_start,
	int &redraw,
	int &rerender,
	int &new_cursor,
	int &update_cursor)
{
	int result = 0;
	int over_edit_handle = 0;

	if(!mwindow->edl->session->show_assets) return 0;

	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit && !result;
			edit = edit->next)
		{
			int64_t edit_x, edit_y, edit_w, edit_h;
			edit_dimensions(edit, edit_x, edit_y, edit_w, edit_h);

// Cursor inside a track
// Cursor inside an edit
			if(cursor_x >= edit_x && cursor_x < edit_x + edit_w &&
				cursor_y >= edit_y && cursor_y < edit_y + edit_h)
			{
// Select duration of edit
				if(button_press)
				{
					if(get_double_click() && !drag_start)
					{
						mwindow->edl->local_session->set_selectionstart(edit->track->from_units(edit->startproject));
						mwindow->edl->local_session->set_selectionend(edit->track->from_units(edit->startproject) + 
							edit->track->from_units(edit->length));
						if(mwindow->edl->session->cursor_on_frames) 
						{
							mwindow->edl->local_session->set_selectionstart(
								mwindow->edl->align_to_frame(mwindow->edl->local_session->get_selectionstart(1), 0));
							mwindow->edl->local_session->set_selectionend(
								mwindow->edl->align_to_frame(mwindow->edl->local_session->get_selectionend(1), 1));
						}
						redraw = 1;
						rerender = 1;
						result = 1;
					}
				}
				else
				if(drag_start && track->record)
				{
					if(mwindow->edl->session->editing_mode == EDITING_ARROW)
					{
// Need to create drag window
						mwindow->session->current_operation = DRAG_EDIT;
						mwindow->session->drag_edit = edit;
//printf("TrackCanvas::do_edits 2\n");

// Drag only one edit if ctrl is initially down
						if(ctrl_down())
						{
							mwindow->session->drag_edits->remove_all();
							mwindow->session->drag_edits->append(edit);
						}
						else
// Construct list of all affected edits
						{
							mwindow->edl->tracks->get_affected_edits(
								mwindow->session->drag_edits, 
								edit->track->from_units(edit->startproject),
								edit->track);
						}
						mwindow->session->drag_origin_x = cursor_x;
						mwindow->session->drag_origin_y = cursor_y;

						gui->drag_popup = new BC_DragWindow(gui, 
							mwindow->theme->get_image("clip_icon") /*, 
							get_abs_cursor_x(0) - mwindow->theme->get_image("clip_icon")->get_w() / 2,
							get_abs_cursor_y(0) - mwindow->theme->get_image("clip_icon")->get_h() / 2 */);

						result = 1;
					}
				}
			}
		}
	}
	return result;
}


int TrackCanvas::test_resources(int cursor_x, int cursor_y)
{
	return 0;
}

int TrackCanvas::do_plugins(int cursor_x, 
	int cursor_y, 
	int drag_start,
	int button_press,
	int &redraw,
	int &rerender)
{
	Plugin *plugin = 0;
	int result = 0;
	int done = 0;
	int64_t x, y, w, h;
	Track *track = 0;


//	if(!mwindow->edl->session->show_assets) return 0;


	for(track = mwindow->edl->tracks->first;
		track && !done;
		track = track->next)
	{
		if(!track->expand_view) continue;


		for(int i = 0; i < track->plugin_set.total && !done; i++)
		{
			PluginSet *plugin_set = track->plugin_set.values[i];
			for(plugin = (Plugin*)plugin_set->first;
				plugin && !done;
				plugin = (Plugin*)plugin->next)
			{
				plugin_dimensions(plugin, x, y, w, h);
				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					if(cursor_x >= x && cursor_x < x + w &&
						cursor_y >= y && cursor_y < y + h)
					{
						done = 1;
						break;
					}
				}
			}
		}
	}

	if(plugin)
	{
// Start plugin popup
		if(button_press)
		{
			if(get_buttonpress() == 3)
			{
				gui->plugin_menu->update(plugin);
				gui->plugin_menu->activate_menu();
				result = 1;
			} 
			else
// Select range of plugin on doubleclick over plugin
			if (get_double_click() && !drag_start)
			{
				mwindow->edl->local_session->set_selectionstart(plugin->track->from_units(plugin->startproject));
				mwindow->edl->local_session->set_selectionend(plugin->track->from_units(plugin->startproject) + 
					plugin->track->from_units(plugin->length));
				if(mwindow->edl->session->cursor_on_frames) 
				{
					mwindow->edl->local_session->set_selectionstart(
						mwindow->edl->align_to_frame(mwindow->edl->local_session->get_selectionstart(1), 0));
					mwindow->edl->local_session->set_selectionend(
						mwindow->edl->align_to_frame(mwindow->edl->local_session->get_selectionend(1), 1));
				}
				rerender = 1;
				redraw = 1;
				result = 1;
			}
		}
		else
// Move plugin
		if(drag_start && plugin->track->record)
		{
			if(mwindow->edl->session->editing_mode == EDITING_ARROW)
			{
				if(plugin->track->data_type == TRACK_AUDIO)
					mwindow->session->current_operation = DRAG_AEFFECT_COPY;
				else
				if(plugin->track->data_type == TRACK_VIDEO)
					mwindow->session->current_operation = DRAG_VEFFECT_COPY;

				mwindow->session->drag_plugin = plugin;





// Create picon
				switch(plugin->plugin_type)
				{
					case PLUGIN_STANDALONE:
					{
						PluginServer *server = mwindow->scan_plugindb(
							plugin->title,
							plugin->track->data_type);
						VFrame *frame = server->picon;

						if(!frame)
						{
							if(plugin->track->data_type == TRACK_AUDIO)
							{
								frame = mwindow->theme->get_image("aeffect_icon");
							}
							else
							{
								frame = mwindow->theme->get_image("veffect_icon");
							}
						}

						gui->drag_popup = new BC_DragWindow(gui, 
							frame /*, 
							get_abs_cursor_x(0) - frame->get_w() / 2,
							get_abs_cursor_y(0) - frame->get_h() / 2 */);
						break;
					}
					
					case PLUGIN_SHAREDPLUGIN:
					case PLUGIN_SHAREDMODULE:
						gui->drag_popup = new BC_DragWindow(gui, 
							mwindow->theme->get_image("clip_icon") /*, 
							get_abs_cursor_x(0) - mwindow->theme->get_image("clip_icon")->get_w() / 2,
							get_abs_cursor_y(0) - mwindow->theme->get_image("clip_icon")->get_h() / 2 */);
						break;
				}


				result = 1;
			}
		}
	}

	return result;
}

int TrackCanvas::do_transitions(int cursor_x, 
	int cursor_y, 
	int button_press,
	int &new_cursor,
	int &update_cursor)
{
	Transition *transition = 0;
	int result = 0;
	int64_t x, y, w, h;



	if(/* !mwindow->edl->session->show_assets || */
		!mwindow->edl->session->auto_conf->transitions) return 0;
					


	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit;
			edit = edit->next)
		{
			if(edit->transition)
			{
				edit_dimensions(edit, x, y, w, h);
				get_transition_coords(x, y, w, h);

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					if(cursor_x >= x && cursor_x < x + w &&
						cursor_y >= y && cursor_y < y + h)
					{
						transition = edit->transition;
						result = 1;
						break;
					}
				}
			}
		}
	}
	
	update_cursor = 1;
	if(transition)
	{
		if(!button_press)
		{
			new_cursor = UPRIGHT_ARROW_CURSOR;
		}
		else
		if(get_buttonpress() == 3)
		{
			gui->transition_menu->update(transition);
			gui->transition_menu->activate_menu();
		}
	}

	return result;
}

int TrackCanvas::button_press_event()
{
	int result = 0;
	int cursor_x, cursor_y;
	int new_cursor, update_cursor;

	cursor_x = get_cursor_x();
	cursor_y = get_cursor_y();
	mwindow->session->trim_edits = 0;

	if(is_event_win() && cursor_inside())
	{
		if(!active)
		{
			activate();
		}

		if(get_buttonpress() == LEFT_BUTTON)
		{
			gui->unlock_window();
			gui->mbuttons->transport->handle_transport(STOP, 1, 0, 0);
			gui->lock_window("TrackCanvas::button_press_event");
		}

		int update_overlay = 0, update_cursor = 0, rerender = 0;

		if(get_buttonpress() == WHEEL_UP)
		{
			if(shift_down())
				mwindow->expand_sample();
			else
				mwindow->move_up(get_h() / 10);
			result = 1;
		}
		else
		if(get_buttonpress() == WHEEL_DOWN)
		{
			if(shift_down())
				mwindow->zoom_in_sample();
			else
				mwindow->move_down(get_h() / 10);
			result = 1;
		}
		else
		switch(mwindow->edl->session->editing_mode)
		{
// Test handles and resource boundaries and highlight a track
			case EDITING_ARROW:
			{
				Edit *edit;
				int handle;
				if(mwindow->edl->session->auto_conf->transitions && 
					do_transitions(cursor_x, 
						cursor_y, 
						1, 
						new_cursor, 
						update_cursor))
				{
					break;
				}
				else
				if(do_keyframes(cursor_x, 
					cursor_y, 
					0, 
					get_buttonpress(), 
					new_cursor, 
					update_cursor,
					rerender))
				{
					break;
				}
				else
// Test edit boundaries
				if(do_edit_handles(cursor_x, 
					cursor_y, 
					1, 
					new_cursor, 
					update_cursor))
				{
					break;
				}
				else
// Test plugin boundaries
				if(do_plugin_handles(cursor_x, 
					cursor_y, 
					1, 
					new_cursor, 
					update_cursor))
				{
					break;
				}
				else
				if(do_edits(cursor_x, cursor_y, 1, 0, update_cursor, rerender, new_cursor, update_cursor))
				{
					break;
				}
				else
				if(do_plugins(cursor_x, cursor_y, 0, 1, update_cursor, rerender))
				{
					break;
				}
				else
				if(test_resources(cursor_x, cursor_y))
				{
					break;
				}
				else
				if(do_tracks(cursor_x, cursor_y, 1))
				{
					break;
				}
				break;
			}

// Test handles only and select a region
			case EDITING_IBEAM:
			{
				double position = (double)cursor_x * 
					mwindow->edl->local_session->zoom_sample /
					mwindow->edl->session->sample_rate + 
					(double)mwindow->edl->local_session->view_start[pane->number] * 
					mwindow->edl->local_session->zoom_sample /
					mwindow->edl->session->sample_rate;
//printf("TrackCanvas::button_press_event %d\n", position);

				if(mwindow->edl->session->auto_conf->transitions && 
					do_transitions(cursor_x, 
						cursor_y, 
						1, 
						new_cursor, 
						update_cursor))
				{
					break;
				}
				else
				if(do_keyframes(cursor_x, 
					cursor_y, 
					0, 
					get_buttonpress(), 
					new_cursor, 
					update_cursor,
					rerender))
				{
					update_overlay = 1;
					break;
				}
				else
// Test edit boundaries
				if(do_edit_handles(cursor_x, 
					cursor_y, 
					1, 
					new_cursor, 
					update_cursor))
				{
					break;
				}
				else
// Test plugin boundaries
				if(do_plugin_handles(cursor_x, 
					cursor_y, 
					1, 
					new_cursor, 
					update_cursor))
				{
					break;
				}
				else
				if(do_edits(cursor_x, 
					cursor_y, 
					1, 
					0, 
					update_cursor, 
					rerender, 
					new_cursor, 
					update_cursor))
				{
					break;
				}
				else
				if(do_plugins(cursor_x, 
					cursor_y, 
					0, 
					1, 
					update_cursor, 
					rerender))
				{
					break;
				}
				else
				if(do_tracks(cursor_x, cursor_y, 1))
				{
					break;
				}
// Highlight selection
				else
				{
					rerender = start_selection(position);
					mwindow->session->current_operation = SELECT_REGION;
					update_cursor = 1;
				}

				break;
			}
		}


		if(rerender)
		{
			gui->unlock_window();
			mwindow->cwindow->update(1, 0, 0, 0, 1);

			gui->lock_window("TrackCanvas::button_press_event 2");
// Update faders
			mwindow->update_plugin_guis();
			gui->update_patchbay();
		}

		if(update_overlay)
		{
			gui->draw_overlays(1);
		}

		if(update_cursor)
		{
			gui->update_timebar(0);
			gui->hide_cursor(0);
			gui->show_cursor(1);
			gui->zoombar->update();
			gui->flash_canvas(1);
			result = 1;
		}



	}
	return result;
}

int TrackCanvas::start_selection(double position)
{
	int rerender = 0;
	position = mwindow->edl->align_to_frame(position, 0);


// Extend a border
	if(shift_down())
	{
		double midpoint = (mwindow->edl->local_session->get_selectionstart(1) + 
			mwindow->edl->local_session->get_selectionend(1)) / 2;

		if(position < midpoint)
		{
			mwindow->edl->local_session->set_selectionstart(position);
			selection_midpoint = mwindow->edl->local_session->get_selectionend(1);
// Que the CWindow
			rerender = 1;
		}
		else
		{
			mwindow->edl->local_session->set_selectionend(position);
			selection_midpoint = mwindow->edl->local_session->get_selectionstart(1);
// Don't que the CWindow for the end
		}
	}
	else
// Start a new selection
	{
//printf("TrackCanvas::start_selection %f\n", position);
		mwindow->edl->local_session->set_selectionstart(position);
		mwindow->edl->local_session->set_selectionend(position);
		selection_midpoint = position;
// Que the CWindow
		rerender = 1;
	}
	
	return rerender;
}

void TrackCanvas::end_edithandle_selection()
{
	mwindow->modify_edithandles();
}

void TrackCanvas::end_pluginhandle_selection()
{
	mwindow->modify_pluginhandles();
}


double TrackCanvas::time_visible()
{
	return (double)get_w() * 
		mwindow->edl->local_session->zoom_sample / 
		mwindow->edl->session->sample_rate;
}

// Patchbay* TrackCanvas::get_patchbay()
// {
// 	if(pane->patchbay) return pane->patchbay;
// 	if(gui->total_panes() == 2 &&
// 		gui->pane[TOP_LEFT_PANE] &&
// 		gui->pane[TOP_RIGHT_PANE])
// 		return gui->pane[TOP_LEFT_PANE]->patchbay;
// 	if(gui->total_panes() == 4)
// 	{
// 		if(pane->number == TOP_RIGHT_PANE)
// 			return gui->pane[TOP_LEFT_PANE]->patchbay;
// 		else
// 			return gui->pane[BOTTOM_LEFT_PANE]->patchbay;
// 	}
// 
// 	return 0;
// }









































