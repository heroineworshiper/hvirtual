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

#ifndef CPLAYBACK_H
#define CPLAYBACK_H

#include "cwindow.inc"
#include "playbackengine.h"

class CPlayback : public PlaybackEngine
{
public:
	CPlayback(CWindow *cwindow, Canvas *output);

	void init_cursor();
	void stop_cursor();
	int brender_available(long position);
    void update_meters(int64_t position);
    void update_tracker(double position);
    int update_scroll(double position);

	CWindow *cwindow;
// Values to return from playback_engine to update_meter .
// Use ArrayList to simplify module counting
	ArrayList<double> module_levels;
};

#endif
