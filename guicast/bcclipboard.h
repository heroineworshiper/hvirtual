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
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

// The primary selection is filled by highlighting a region
#define PRIMARY_SELECTION 0
// The secondary selection is filled by copying
#define SECONDARY_SELECTION 1
#define TOTAL_SELECTIONS 2

// Storage for guicast only
// The secondary selection has never been reliable either in Cinelerra
// or anything else.  We just use the guaranteed solution for any data not
// intended for use outside Cinelerra.
#define BC_PRIMARY_SELECTION 2



class BC_Clipboard : public Thread
{
public:
	BC_Clipboard(const char *display_name);
	~BC_Clipboard();

	int start_clipboard();
	void run();
	int stop_clipboard();
	int clipboard_len(int clipboard_num);
	int to_clipboard(const char *data, int len, int clipboard_num);
	int from_clipboard(char *data, int maxlen, int clipboard_num);
	int from_clipboard(char *data, int maxlen, int *len_return, int clipboard_num);

	Display *in_display, *out_display;
	Atom completion_atom, primary, secondary, utf8_target, targets, string_target;
	Window in_win, out_win;
	static char *g_data[TOTAL_SELECTIONS];
	static int g_length[TOTAL_SELECTIONS];
    static Mutex *g_lock;
	char display_name[BCTEXTLEN];
};

#endif
