
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

#include "apatchgui.h"
#include "automation.h"
#include "floatauto.h"
#include "floatautos.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "intauto.h"
#include "intautos.h"
#include "language.h"
#include "localsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "patchgui.h"
#include "mainsession.h"
#include "theme.h"
#include "timelinepane.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "vpatchgui.h"






NudgePopup::NudgePopup(MWindow *mwindow, PatchBay *patchbay)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->patchbay = patchbay;
}

NudgePopup::~NudgePopup()
{
}


void NudgePopup::create_objects()
{
	add_item(seconds_item = new NudgePopupSeconds(this));
	add_item(native_item = new NudgePopupNative(this));
	add_item(new NudgeCut(this));
	add_item(new NudgeCopy(this));
	add_item(new NudgePaste(this));
}

void NudgePopup::activate_menu(PatchGUI *gui)
{
    this->gui = gui;
// Set checked status
	seconds_item->set_checked(mwindow->edl->session->nudge_seconds ? 1 : 0);
	native_item->set_checked(mwindow->edl->session->nudge_seconds ? 0 : 1);

// Set native units to track format
	native_item->set_text(gui->track->data_type == TRACK_AUDIO ? 
		(char*)"Samples" : 
		(char*)"Frames");

// Show it
	BC_PopupMenu::activate_menu();
}



NudgePopupSeconds::NudgePopupSeconds(NudgePopup *popup)
 : BC_MenuItem("Seconds")
{
	this->popup = popup;
}

int NudgePopupSeconds::handle_event()
{
	popup->mwindow->edl->session->nudge_seconds = 1;
	popup->mwindow->gui->update_patchbay();
	return 1;
}





NudgePopupNative::NudgePopupNative(NudgePopup *popup)
 : BC_MenuItem("")
{
	this->popup = popup;
}

int NudgePopupNative::handle_event()
{
	popup->mwindow->edl->session->nudge_seconds = 0;
	popup->mwindow->gui->update_patchbay();
	return 1;
}



NudgeCut::NudgeCut(NudgePopup *popup)
 : BC_MenuItem("Cut")
{
	this->popup = popup;
}

int NudgeCut::handle_event()
{
    if(popup->gui->nudge)
    {
    	popup->gui->nudge->cut(1);
    }
	return 1;
}



NudgeCopy::NudgeCopy(NudgePopup *popup)
 : BC_MenuItem("Copy")
{
	this->popup = popup;
}

int NudgeCopy::handle_event()
{
    if(popup->gui->nudge)
    {
    	popup->gui->nudge->copy(1);
    }
	return 1;
}




NudgePaste::NudgePaste(NudgePopup *popup)
 : BC_MenuItem("Paste")
{
	this->popup = popup;
}

int NudgePaste::handle_event()
{
    if(popup->gui->nudge)
    {
    	popup->gui->nudge->paste(1);
    }
	return 1;
}









PatchBay::PatchBay(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->patchbay_x,
 	mwindow->theme->patchbay_y,
	mwindow->theme->patchbay_w,
	mwindow->theme->patchbay_h)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->pane = 0;
	drag_operation = Tracks::NONE;
}

PatchBay::PatchBay(MWindow *mwindow, 
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
	drag_operation = Tracks::NONE;
// printf("PatchBay::PatchBay %d %d %d %d %d\n", 
// __LINE__,
// x,
// y,
// w,
// h);
}

PatchBay::~PatchBay() 
{
}


int PatchBay::delete_all_patches()
{
    patches.remove_all_objects();
    return 0;
}

void PatchBay::create_objects()
{
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	flash(0);

// Create icons for mode types
	mode_icons[TRANSFER_NORMAL] = new BC_Pixmap(this, 
		mwindow->theme->get_image("mode_normal"),
		PIXMAP_ALPHA);
	mode_icons[TRANSFER_ADDITION] = new BC_Pixmap(this, 
		mwindow->theme->get_image("mode_add"),
		PIXMAP_ALPHA);
	mode_icons[TRANSFER_SUBTRACT] = new BC_Pixmap(this, 
		mwindow->theme->get_image("mode_subtract"),
		PIXMAP_ALPHA);
	mode_icons[TRANSFER_MULTIPLY] = new BC_Pixmap(this, 
		mwindow->theme->get_image("mode_multiply"),
		PIXMAP_ALPHA);
	mode_icons[TRANSFER_DIVIDE] = new BC_Pixmap(this, 
		mwindow->theme->get_image("mode_divide"),
		PIXMAP_ALPHA);
	mode_icons[TRANSFER_REPLACE] = new BC_Pixmap(this, 
		mwindow->theme->get_image("mode_replace"),
		PIXMAP_ALPHA);
	mode_icons[TRANSFER_MAX] = new BC_Pixmap(this, 
		mwindow->theme->get_image("mode_max"),
		PIXMAP_ALPHA);
	mode_icons[TRANSFER_MIN] = new BC_Pixmap(this, 
		mwindow->theme->get_image("mode_min"),
		PIXMAP_ALPHA);

	add_subwindow(nudge_popup = new NudgePopup(mwindow, this));
	nudge_popup->create_objects();
	update();
}

BC_Pixmap* PatchBay::mode_to_icon(int mode)
{
	return mode_icons[mode];
}

int PatchBay::icon_to_mode(BC_Pixmap *icon)
{
	for(int i = 0; i < TRANSFER_TYPES; i++)
		if(icon == mode_icons[i]) return i;
	return TRANSFER_NORMAL;
}

void PatchBay::resize_event()
{
	reposition_window(mwindow->theme->patchbay_x,
		mwindow->theme->patchbay_y,
		mwindow->theme->patchbay_w,
		mwindow->theme->patchbay_h);
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	update();
	flash(0);
}

void PatchBay::resize_event(int x, int y, int w, int h)
{
	reposition_window(x,
		y,
		w,
		h);
	draw_top_background(get_parent(), 0, 0, w, h);
	update();
	flash(0);
}

int PatchBay::button_press_event()
{
	int result = 0;
// Too much junk to support the wheel
	return result;
}

int PatchBay::cursor_motion_event()
{
	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();
	int update_gui = 0;

//printf("PatchBay::cursor_motion_event %d %d %d\n", 
//__LINE__, drag_operation, get_button_down());
	if(drag_operation != Tracks::NONE)
	{
		if(cursor_y >= 0 &&
			cursor_y < get_h())
		{
// Get track we're inside of
			for(Track *track = mwindow->edl->tracks->first;
				track;
				track = track->next)
			{
				int y = track->y_pixel - mwindow->edl->local_session->track_start[pane->number];
				int h = track->vertical_span(mwindow->theme);
				if(cursor_y >= y && cursor_y < y + h)
				{
					switch(drag_operation)
					{
						case Tracks::PLAY:
							if(track->play != new_status)
							{
								track->play = new_status;
								mwindow->gui->unlock_window();
								mwindow->restart_brender();
								mwindow->sync_parameters(CHANGE_EDL);
								mwindow->gui->lock_window();
								update_gui = 1;
							}
							break;
						case Tracks::RECORD:
							if(track->record != new_status)
							{
								track->record = new_status;
								update_gui = 1;
							}
							break;
						case Tracks::GANG:
							if(track->gang != new_status)
							{
								track->gang = new_status;
								update_gui = 1;
							}
							break;
						case Tracks::DRAW:
							if(track->draw != new_status)
							{
								track->draw = new_status;
								update_gui = 1;
							}
							break;
						case Tracks::EXPAND:
							if(track->expand_view != new_status)
							{
								track->expand_view = new_status;
								mwindow->edl->tracks->update_y_pixels(mwindow->theme);
								gui->draw_trackmovement();
								update_gui = 0;
							}
							break;
						case Tracks::MUTE:
						{
							IntAuto *current = 0;
							Auto *keyframe = 0;
							double position = mwindow->edl->local_session->get_selectionstart(1);
							Autos *mute_autos = track->automation->autos[AUTOMATION_MUTE];

							current = (IntAuto*)mute_autos->get_prev_auto(PLAY_FORWARD, 
								keyframe);

							if(current->value != new_status)
							{

//								mwindow->undo->update_undo_before(_("keyframe"), this);
								current = (IntAuto*)mute_autos->get_auto_for_editing(position);

								current->value = new_status;

//								mwindow->undo->update_undo_after(_("keyframe"), LOAD_AUTOMATION);

								mwindow->gui->unlock_window();
								mwindow->restart_brender();
								mwindow->sync_parameters(CHANGE_PARAMS);
								mwindow->gui->lock_window();

								if(mwindow->edl->session->auto_conf->autos[AUTOMATION_MUTE])
								{
									gui->draw_overlays(1, 1);
								}
								update_gui = 1;
							}
							break;
						}
					}
				}
			}
		}
	}

	if(update_gui)
	{
//printf("PatchBay::cursor_motion_event %d\n", __LINE__);
		gui->update_patchbay();
// redraw visible assets
        if(drag_operation == Tracks::DRAW)
        {
            gui->update(0, 1, 0, 0, 0, 0, 0);
        }
	}

	return 0;
}

void PatchBay::set_meter_format(int mode, int min, int max)
{
	for(int i = 0; i < patches.total; i++)
	{
		PatchGUI *patchgui = patches.values[i];
		if(patchgui->data_type == TRACK_AUDIO)
		{
			APatchGUI *apatchgui = (APatchGUI*)patchgui;
			if(apatchgui->meter)
			{
				apatchgui->meter->change_format(mode, min, max);
			}
		}
	}
}

void PatchBay::update_meters(ArrayList<double> *module_levels)
{
	for(int level_number = 0, patch_number = 0;
		patch_number < patches.total && level_number < module_levels->total;
		patch_number++)
	{
		APatchGUI *patchgui = (APatchGUI*)patches.values[patch_number];

		if(patchgui->data_type == TRACK_AUDIO)
		{
			if(patchgui->meter)
			{
				double level = module_levels->values[level_number];
				patchgui->meter->update(level, level > 1);
			}

			level_number++;
		}
	}
}

void PatchBay::reset_meters()
{
	for(int patch_number = 0;
		patch_number < patches.total;
		patch_number++)
	{
		APatchGUI *patchgui = (APatchGUI*)patches.values[patch_number];
		if(patchgui->data_type == TRACK_AUDIO && patchgui->meter)
		{
			patchgui->meter->reset_over();
		}
	}
}

void PatchBay::stop_meters()
{
	for(int patch_number = 0;
		patch_number < patches.total;
		patch_number++)
	{
		APatchGUI *patchgui = (APatchGUI*)patches.values[patch_number];
		if(patchgui->data_type == TRACK_AUDIO && patchgui->meter)
		{
			patchgui->meter->reset();
		}
	}
}


#define PATCH_X 3

int PatchBay::update()
{
	int patch_count = 0;

// Every patch has a GUI regardless of whether or not it is visible.
// Make sure GUI's are allocated for every patch and deleted for non-existant
// patches.
	for(Track *current = mwindow->edl->tracks->first;
		current;
		current = NEXT, patch_count++)
	{
		PatchGUI *patchgui;
		int y = current->y_pixel - mwindow->edl->local_session->track_start[pane->number];

//printf("PatchBay::update %d %d\n", __LINE__, y);
		if(patches.total > patch_count)
		{
			if(patches.values[patch_count]->track_id != current->get_id())
			{
				delete patches.values[patch_count];

				switch(current->data_type)
				{
					case TRACK_AUDIO:
						patchgui = patches.values[patch_count] = new APatchGUI(mwindow, this, (ATrack*)current, PATCH_X, y);
						break;
					case TRACK_VIDEO:
						patchgui = patches.values[patch_count] = new VPatchGUI(mwindow, this, (VTrack*)current, PATCH_X, y);
						break;
				}
				patchgui->create_objects();
			}
			else
			{
				patches.values[patch_count]->update(PATCH_X, y);
			}
		}
		else
		{
			switch(current->data_type)
			{
				case TRACK_AUDIO:
					patchgui = new APatchGUI(mwindow, this, (ATrack*)current, PATCH_X, y);
					break;
				case TRACK_VIDEO:
					patchgui = new VPatchGUI(mwindow, this, (VTrack*)current, PATCH_X, y);
					break;
			}
			patches.append(patchgui);
			patchgui->create_objects();
		}
	}

	while(patches.total > patch_count)
	{
		delete patches.values[patches.total - 1];
		patches.remove_number(patches.total - 1);
	}

	show_window(0);
	return 0;
}

void PatchBay::synchronize_faders(float change, int data_type, Track *skip)
{
	for(Track *current = mwindow->edl->tracks->first;
		current;
		current = NEXT)
	{
		if(current->data_type == data_type &&
			current->gang && 
			current->record && 
			current != skip)
		{
			FloatAutos *fade_autos = (FloatAutos*)current->automation->autos[AUTOMATION_FADE];
			double position = mwindow->edl->local_session->get_selectionstart(1);


			FloatAuto *keyframe = (FloatAuto*)fade_autos->get_auto_for_editing(position);

			keyframe->value += change;
			if(data_type == TRACK_AUDIO)
				CLAMP(keyframe->value, INFINITYGAIN, MAX_AUDIO_FADE);
			else
				CLAMP(keyframe->value, 0, MAX_VIDEO_FADE);


			PatchGUI *patch = get_patch_of(current);
			if(patch) patch->update(patch->x, patch->y);
		}
	}
}

void PatchBay::synchronize_nudge(int64_t value, Track *skip)
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
			current->nudge = value;
			PatchGUI *patch = get_patch_of(current);
			if(patch) patch->update(patch->x, patch->y);
		}
	}
}

PatchGUI* PatchBay::get_patch_of(Track *track)
{
	for(int i = 0; i < patches.total; i++)
	{
		if(patches.values[i]->track == track)
			return patches.values[i];
	}
	return 0;
}

int PatchBay::resize_event(int top, int bottom)
{
	reposition_window(mwindow->theme->patchbay_x,
 		mwindow->theme->patchbay_y,
		mwindow->theme->patchbay_w,
		mwindow->theme->patchbay_h);
	return 0;
}


