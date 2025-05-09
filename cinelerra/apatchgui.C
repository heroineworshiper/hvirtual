/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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
#include "apatchgui.inc"
#include "atrack.h"
#include "autoconf.h"
#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "language.h"
#include "localsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "panauto.h"
#include "panautos.h"
#include "patchbay.h"
#include "theme.h"
#include "trackcanvas.h"




APatchGUI::APatchGUI(MWindow *mwindow, 
	PatchBay *patchbay, 
	ATrack *track, 
	int x, 
	int y)
 : PatchGUI(mwindow, 
 	patchbay, 
	track, 
	x, 
	y)
{
	data_type = TRACK_AUDIO;
	this->atrack = track;
	meter = 0;
	pan = 0;
	fade = 0;
}

APatchGUI::~APatchGUI()
{
	if(fade) delete fade;
	if(meter) delete meter;
	if(pan) delete pan;
}

void APatchGUI::create_objects()
{
	update(x, y);
}

int APatchGUI::reposition(int x, int y)
{
	int x1 = 0;
	int y1 = PatchGUI::reposition(x, y);

	if(fade) fade->reposition_window(fade->get_x(), 
		y1 + y);
	y1 += mwindow->theme->fade_h;

	if(meter) meter->reposition_window(meter->get_x(),
		y1 + y,
		-1,
		meter->get_w());
	y1 += mwindow->theme->meter_h;

	if(pan) pan->reposition_window(pan->get_x(),
		y1 + y);

	if(nudge) nudge->reposition_window(nudge->get_x(),
		y1 + y);

	y1 += mwindow->theme->pan_h;
	return y1;
}

int APatchGUI::update(int x, int y)
{
	int h = track->vertical_span(mwindow->theme);
	int x1 = 0;
	int y1 = PatchGUI::update(x, y);

	if(fade)
	{
		if(h - y1 < mwindow->theme->fade_h)
		{
			delete fade;
			fade = 0;
		}
		else
		{
			FloatAuto *previous = 0, *next = 0;
			double unit_position = mwindow->edl->local_session->get_selectionstart(1);
			unit_position = mwindow->edl->align_to_frame(unit_position, 0);
			unit_position = atrack->to_units(unit_position, 0);
			FloatAutos *ptr = (FloatAutos*)atrack->automation->autos[AUTOMATION_FADE];
			float value = ptr->get_value(
				(long)unit_position,
				PLAY_FORWARD, 
				previous, 
				next);
			fade->update(value);
		}
	}
	else
	if(h - y1 >= mwindow->theme->fade_h)
	{
		patchbay->add_subwindow(fade = new AFadePatch(mwindow, 
			this, 
			x1 + x, 
			y1 + y, 
			patchbay->get_w() - 10));
	}
	y1 += mwindow->theme->fade_h;

	if(meter)
	{
		if(h - y1 < mwindow->theme->meter_h)
		{
			delete meter;
			meter = 0;
		}
	}
	else
	if(h - y1 >= mwindow->theme->meter_h)
	{
		patchbay->add_subwindow(meter = new AMeterPatch(mwindow,
			this,
			x1 + x, 
			y1 + y));
	}
	y1 += mwindow->theme->meter_h;
	x1 += 10;

	if(pan)
	{
		if(h - y1 < mwindow->theme->pan_h)
		{
			delete pan;
			pan = 0;
			delete nudge;
			nudge = 0;
		}
		else
		{
			if(pan->get_total_values() != mwindow->edl->session->audio_channels)
			{
				pan->change_channels(mwindow->edl->session->audio_channels,
					mwindow->edl->session->achannel_positions);
			}
			else
			{
				int handle_x, handle_y;
				PanAuto *previous = 0, *next = 0;
				double unit_position = mwindow->edl->local_session->get_selectionstart(1);
				unit_position = mwindow->edl->align_to_frame(unit_position, 0);
				unit_position = atrack->to_units(unit_position, 0);
				PanAutos *ptr = (PanAutos*)atrack->automation->autos[AUTOMATION_PAN];
				ptr->get_handle(handle_x,
					handle_y,
					(long)unit_position, 
					PLAY_FORWARD,
					previous,
					next);
				pan->update(handle_x, handle_y);
			}
			nudge->update();
		}
	}
	else
	if(h - y1 >= mwindow->theme->pan_h)
	{
		patchbay->add_subwindow(pan = new APanPatch(mwindow,
			this,
			x1 + x, 
			y1 + y));
		x1 += pan->get_w() + 10;
		patchbay->add_subwindow(nudge = new NudgePatch(mwindow,
			this,
			x1 + x,
			y1 + y,
			patchbay->get_w() - x1 - 10));
	}
	y1 += mwindow->theme->pan_h;

	return y1;
}

void APatchGUI::synchronize_fade(float value_change)
{
	if(fade && !change_source) 
	{
		fade->update(fade->get_value() + value_change);
		fade->update_edl();
	}
}



AFadePatch::AFadePatch(MWindow *mwindow, APatchGUI *patch, int x, int y, int w)
 : BC_FSlider(x, 
			y, 
			0, 
			w, 
			w, 
			(float)INFINITYGAIN, 
			(float)MAX_AUDIO_FADE, 
			get_keyframe(mwindow, patch)->value)
{
	this->mwindow = mwindow;
	this->patch = patch;
}

float AFadePatch::update_edl()
{
	FloatAuto *current;
	double position = mwindow->edl->local_session->get_selectionstart(1);
	Autos *fade_autos = patch->atrack->automation->autos[AUTOMATION_FADE];
	int need_undo = !fade_autos->auto_exists_for_editing(position);

	mwindow->undo->update_undo_before(_("fade"), need_undo ? 0 : this);

	current = (FloatAuto*)fade_autos->get_auto_for_editing(position);

	float result = get_value() - current->value;
	current->value = get_value();

	mwindow->undo->update_undo_after(_("fade"), LOAD_AUTOMATION);

	return result;
}


int AFadePatch::handle_event()
{
	if(shift_down()) 
	{
		update(0.0);
		set_tooltip(get_caption());
	}

	patch->change_source = 1;
	float change = update_edl();
	if(patch->track->gang && patch->track->record) 
		patch->patchbay->synchronize_faders(change, TRACK_AUDIO, patch->track);
	patch->change_source = 0;

	mwindow->sync_parameters(CHANGE_PARAMS);

	if(mwindow->edl->session->auto_conf->autos[AUTOMATION_FADE])
	{
		mwindow->gui->draw_overlays(1, 1);
	}
	return 1;
}

FloatAuto* AFadePatch::get_keyframe(MWindow *mwindow, APatchGUI *patch)
{
	Auto *current = 0;
	double unit_position = mwindow->edl->local_session->get_selectionstart(1);
	unit_position = mwindow->edl->align_to_frame(unit_position, 0);
	unit_position = patch->atrack->to_units(unit_position, 0);

	FloatAutos *ptr = (FloatAutos*)patch->atrack->automation->autos[AUTOMATION_FADE];
	return (FloatAuto*)ptr->get_prev_auto(
		(long)unit_position, 
		PLAY_FORWARD,
		current);
}


APanPatch::APanPatch(MWindow *mwindow, APatchGUI *patch, int x, int y)
 : BC_Pan(x, 
		y, 
		PAN_RADIUS, 
		MAX_PAN, 
		mwindow->edl->session->audio_channels, 
		mwindow->edl->session->achannel_positions, 
		get_keyframe(mwindow, patch)->handle_x, 
		get_keyframe(mwindow, patch)->handle_y,
 		get_keyframe(mwindow, patch)->values)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip("Pan");
}

int APanPatch::handle_event()
{
	PanAuto *current;
	double position = mwindow->edl->local_session->get_selectionstart(1);
	Autos *pan_autos = patch->atrack->automation->autos[AUTOMATION_PAN];
	int need_undo = !pan_autos->auto_exists_for_editing(position);

	mwindow->undo->update_undo_before(_("pan"), need_undo ? 0 : this);
	
	current = (PanAuto*)pan_autos->get_auto_for_editing(position);

	current->handle_x = get_stick_x();
	current->handle_y = get_stick_y();
	memcpy(current->values, get_values(), sizeof(float) * mwindow->edl->session->audio_channels);

	mwindow->undo->update_undo_after(_("pan"), LOAD_AUTOMATION);

	mwindow->sync_parameters(CHANGE_PARAMS);

	if(need_undo && mwindow->edl->session->auto_conf->autos[AUTOMATION_PAN])
	{
		mwindow->gui->draw_overlays(1, 1);
	}
	return 1;
}

PanAuto* APanPatch::get_keyframe(MWindow *mwindow, APatchGUI *patch)
{
	Auto *current = 0;
	double unit_position = mwindow->edl->local_session->get_selectionstart(1);
	unit_position = mwindow->edl->align_to_frame(unit_position, 0);
	unit_position = patch->atrack->to_units(unit_position, 0);

	PanAutos *ptr = (PanAutos*)patch->atrack->automation->autos[AUTOMATION_PAN];
	return (PanAuto*)ptr->get_prev_auto(
		(long)unit_position, 
		PLAY_FORWARD,
		current);
}




AMeterPatch::AMeterPatch(MWindow *mwindow, APatchGUI *patch, int x, int y)
 : BC_Meter(x, 
			y, 
			METER_HORIZ, 
			patch->patchbay->get_w() - 10, 
			mwindow->edl->session->min_meter_db, 
			mwindow->edl->session->max_meter_db, 
			mwindow->edl->session->meter_format, 
			0,
			-1)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_delays(TRACKING_RATE * 10,
			TRACKING_RATE);
}

int AMeterPatch::button_press_event()
{
	if(cursor_inside() && is_event_win() && get_buttonpress() == 1)
	{
		mwindow->reset_meters();
		return 1;
	}

	return 0;
}





