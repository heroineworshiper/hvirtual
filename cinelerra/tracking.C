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

#include "arender.h"
#include "condition.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "tracking.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "renderengine.h"
#include "mainsession.h"
#include "trackcanvas.h"



// States
#define PLAYING 0
#define DONE 1



Tracking::Tracking(PlaybackEngine *playback_engine)
 : Thread(1, 0, 0)
{
    this->playback_engine = playback_engine;
	follow_loop = 0; 
	visible = 0;
	pixel = 0;
    playback_engine = 0;
	state = DONE;
	startup_lock = new Condition(0, "Tracking::startup_lock");
}

Tracking::~Tracking()
{
	if(state == PLAYING)
	{
// Stop loop
		state = DONE;
// Not working in NPTL for some reason
//		Thread::cancel();
		Thread::join();
	}


	delete startup_lock;
}

int Tracking::start_playback(double new_position)
{
	if(state != PLAYING)
	{
		last_position = new_position;
		state = PLAYING;
		Thread::start();
		startup_lock->lock("Tracking::start_playback");
	}
	return 0;
}

int Tracking::stop_playback()
{
	if(state != DONE)
	{
// Stop loop
		state = DONE;
// Not working in NPTL for some reason
//		Thread::cancel();
		Thread::join();

// Final position is updated continuously during playback
// Get final position
		double position = playback_engine->get_tracking_position();
// Update cursor
		playback_engine->update_tracker(position);
	
		state = DONE;
	}
	return 0;
}







void Tracking::run()
{
	startup_lock->unlock();

	double position;
	while(state != DONE)
	{
		Thread::enable_cancel();
		timer.delay(1000 / TRACKING_RATE);
		Thread::disable_cancel();

		if(state != DONE)
		{

// can be stopped during wait
			if(playback_engine->tracking_active)
			{
// Get position of cursor
				position = playback_engine->get_tracking_position();

// Update cursor
				playback_engine->update_tracker(position);
			}
		}
	}
}





