
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

#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mainclock.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "vplayback.h"
#include "vtimebar.h"
#include "vtracking.h"
#include "vwindow.h"
#include "vwindowgui.h"


VTracking::VTracking(MWindow *mwindow, VWindow *vwindow)
 : Tracking(mwindow)
{
	this->vwindow = vwindow;
}

VTracking::~VTracking()
{
}

void VTracking::update_tracker(double position)
{
	vwindow->gui->lock_window("VTracking::update_tracker");
	vwindow->get_edl()->local_session->set_selectionstart(position);
	vwindow->get_edl()->local_session->set_selectionend(position);

#ifdef USE_SLIDER
	vwindow->gui->slider->update(position);
#endif

	vwindow->gui->clock->update(position);

// This is going to boost the latency but we need to update the timebar
//	vwindow->gui->timebar->draw_range();
	vwindow->gui->timebar->update(1);
//	vwindow->gui->timebar->flash();

	vwindow->gui->unlock_window();

	update_meters((int64_t)(position * mwindow->edl->session->sample_rate));
}

void VTracking::update_meters(int64_t position)
{
	double output_levels[MAXCHANNELS];

#ifdef USE_METERS
	int do_audio = playback_engine->get_output_levels(output_levels, 
		position);
	if(do_audio)
	{
		vwindow->gui->lock_window("VTracking::update_meters");
		vwindow->gui->meters->update(output_levels);
		vwindow->gui->unlock_window();
	}
#endif

}

void VTracking::stop_meters()
{
#ifdef USE_METERS
	vwindow->gui->lock_window("VTracking::stop_meters");
	vwindow->gui->meters->stop_meters();
	vwindow->gui->unlock_window();
#endif
}

void VTracking::draw()
{
}
