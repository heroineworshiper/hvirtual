
/*
 * CINELERRA
 * Copyright (C) 2008-2025 Adam Williams <broadcast at earthling dot net>
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

#ifndef BC_NEWFOLDER_H
#define BC_NEWFOLDER_H


#include "bcdialog.h"
#include "bcfilebox.inc"
#include "bcwindow.h"


class BC_NewFolder : public BC_Window
{
public:
	BC_NewFolder(int x, int y, BC_FileBox *filebox);
	~BC_NewFolder();

	void create_objects();
	const char* get_text();

private:
	BC_TextBox *textbox;
};

class BC_NewFolderThread : public BC_DialogThread
{
public:
	BC_NewFolderThread(BC_FileBox *filebox);

	void handle_done_event(int result);
	BC_Window* new_gui();

private:
	BC_FileBox *filebox;
};





#endif
