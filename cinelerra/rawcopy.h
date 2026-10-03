/*
 * CINELERRA
 * Copyright (C) 1997-2026 Adam Williams <broadcast at earthling dot net>
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

// copy media directly from source files to destination, neglecting
// timing accuracy
// This ended up being another save option to save an ffmpeg compatible EDL.
// Then the user has to run ffmpeg with the EDL to splice the raw data, but it's
// overall a better experience.

#ifndef RAWCOPY_H
#define RAWCOPY_H

#include "commandtools.h"
#include "guicast.h"


class RawCopyItem : public BC_MenuItem
{
public:
	RawCopyItem();
	int handle_event();
};


class RawCopyThread : public BC_DialogThread
{
public:
	RawCopyThread();
	~RawCopyThread();

	BC_Window* new_gui();
// parameters for the raw copy
    Asset *asset;
};

class RawCopyWindow : public CommandTools
{
public:
    RawCopyWindow();
    ~RawCopyWindow();
    
	void create_objects2(int x, int y);
    void update();
};






#endif



