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

#ifndef TRACKING_H
#define TRACKING_H

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "condition.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "playbackengine.inc"
#include "thread.h"
#include "bctimer.h"

class Tracking : public Thread
{
public:
	Tracking(PlaybackEngine *playback_engine);
	virtual ~Tracking();

	int start_playback(double new_position);
	int stop_playback();




	void run();

	int state;
    PlaybackEngine *playback_engine;









	void show_playback_cursor(int64_t position);
	int view_follows_playback;
// Delay until startup
	Condition *startup_lock;
	double last_position;
	int follow_loop;
	int64_t current_offset;
	int reverse;
	int double_speed;
	Timer timer;
// Pixel of last drawn cursor
	int pixel;
// Cursor is visible
	int visible;
};

#endif
