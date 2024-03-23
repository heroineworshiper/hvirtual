/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef SAVE_H
#define SAVE_H

// common functions which save XML files

#include "guicast.h"
#include "save.inc"


class SaveBackupItem : public BC_MenuItem
{
public:
	SaveBackupItem();
	int handle_event();
};

class SaveItem : public BC_MenuItem
{
public:
	SaveItem();
	int handle_event();
	int save_before_quit();
};

class SaveClipItem : public BC_MenuItem
{
public:
	SaveClipItem();
	int handle_event();
};

class SaveAsItem : public BC_MenuItem
{
public:
	SaveAsItem();
	int handle_event();
};


class SaveThread : public BC_DialogThread
{
public:
	SaveThread();
	~SaveThread();

	
	BC_Window* new_gui();
	void handle_done_event(int result);
    void reset_flags();

    int quit_now;
    int do_clip;
	SaveWindow *window;
};


class SaveWindow : public BC_FileBox
{
public:
	SaveWindow(char *init_path, int do_clip);
	~SaveWindow();
};


#endif








