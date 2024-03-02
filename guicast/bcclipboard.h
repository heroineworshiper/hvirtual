/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
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

#ifndef BCCLIPBOARD_H
#define BCCLIPBOARD_H

#include "bcwindowbase.h"
#include "thread.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

// clipboard mask bits
// The primary buffer is filled by highlighting a region
// This goes to command line programs
#define PRIMARY_SELECTION 0x1
// The secondary buffer is filled by copying
// this goes to windowed programs
#define SECONDARY_SELECTION 0x2

// Storage for guicast only
// The other buffers were never reliable either in Cinelerra
// or anything else so this is a guaranteed solution for any data not
// intended for use outside Cinelerra.
#define BC_PRIMARY_SELECTION 0x4

#define ALL_SELECTIONS (PRIMARY_SELECTION | SECONDARY_SELECTION | BC_PRIMARY_SELECTION)

// the number of buffers
#define TOTAL_SELECTIONS 3



class BC_Clipboard : public Thread
{
public:
	BC_Clipboard(const char *display_name);
	~BC_Clipboard();

	int start_clipboard();
	void run();
	int stop_clipboard();
// this stores in every clipboard with a 1 bit
	int to_clipboard(const char *data, int len, uint32_t clipboard_mask);
// these take 1 mask bit
	int clipboard_len(uint32_t clipboard_mask);
	int from_clipboard(char *data, int maxlen, uint32_t clipboard_mask);
	int from_clipboard(char *data, int maxlen, int *len_return, uint32_t clipboard_mask);


private:
	Display *in_display, *out_display;
	Atom completion_atom, primary, secondary, utf8_target, targets, string_target;
	Window in_win, out_win;
	static char *g_data[TOTAL_SELECTIONS];
	static int g_length[TOTAL_SELECTIONS];
    static Mutex *g_lock;
	char display_name[BCTEXTLEN];

// convert mask bit to array index
    int mask_to_buffer(int32_t clipboard_mask);
// process a single mask bit
    void to_1clipboard(const char *data, 
        int len, 
        uint32_t clipboard_mask);

};

#endif
