/*
 * CINELERRA
 * Copyright (C) 2010-2025 Adam Williams <broadcast at earthling dot net>
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

#include "condition.h"
#include "bcfilebox.h"
#include "bcrename.h"
#include "bcresources.h"
#include "bctheme.h"
#include "bctitle.h"
#include "filesystem.h"
#include "language.h"
#include "mutex.h"
#include <string.h>

#include <sys/stat.h>





#define WINDOW_W DP(320)
#define WINDOW_H DP(150)



BC_Rename::BC_Rename(BC_RenameThread *thread, int x, int y, BC_FileBox *filebox)
 : BC_Window(filebox->get_rename_title(), 
 	x, 
	y, 
	WINDOW_W, 
	WINDOW_H, 
	0, 
	0, 
	0, 
	0, 
	1)
{
	this->thread = thread;
}

BC_Rename::~BC_Rename()
{
}


void BC_Rename::create_objects()
{
    int margin = BC_Resources::theme->widget_border;
	int x = margin, y = margin;
    BC_Title *text;
	lock_window("BC_Rename::create_objects");
	add_tool(text = new BC_Title(x, y, _("Enter a new name for the file:")));
	y += text->get_h() + margin;
	add_subwindow(textbox = new BC_TextBox(x, 
        y, 
        get_w() - margin * 2, 
        1, 
        thread->orig_name));
	y += textbox->get_h() + margin;
	add_subwindow(new BC_OKButton(x, y));
	x = get_w() - BC_CancelButton::calculate_w() - margin;
	add_subwindow(new BC_CancelButton(x, y));
	show_window();
	unlock_window();
}

const char* BC_Rename::get_text()
{
	return textbox->get_text();
}


BC_RenameThread::BC_RenameThread(BC_FileBox *filebox)
 : BC_DialogThread()
{
	this->filebox = filebox;
}


BC_Window* BC_RenameThread::new_gui()
{
	int x = filebox->get_abs_cursor_x(1);
	int y = filebox->get_abs_cursor_y(1);
	char string[BCTEXTLEN];
	FileSystem fs;
	strcpy(string, filebox->get_current_path());

	if(fs.is_dir(string))
	{
// Extract last directory from path to rename it
		strcpy(orig_path, string);
		char *ptr = orig_path + strlen(orig_path) - 1;
// Scan for end of path
		while(*ptr == '/' && ptr > orig_path) ptr--;
// Scan for previous separator
		while(*ptr != '/' && ptr > orig_path) ptr--;
// Get final path segment
		if(*ptr == '/') ptr++;
		strcpy(orig_name, ptr);
// Terminate original path
		*ptr = 0;
	}
	else
	{
// Extract filename from path to rename it
		fs.extract_dir(orig_path, string);
		fs.extract_name(orig_name, string);
	}

	BC_Rename *window = new BC_Rename(this,
		x, 
		y,
		filebox);
	window->create_objects();
    return window;
}

void BC_RenameThread::handle_done_event(int result)
{
	if(!result)
	{
		char old_name[BCTEXTLEN];
		char new_name[BCTEXTLEN];
        BC_Rename *window = (BC_Rename*)get_gui();
		filebox->fs->join_names(old_name, orig_path, orig_name);
		filebox->fs->join_names(new_name, orig_path, window->get_text());

printf("BC_RenameThread::run %d %s -> %s\n", 
__LINE__,
old_name, 
new_name);
		rename(old_name, new_name);
		

		filebox->lock_window("BC_RenameThread::run");
		filebox->refresh(0, 0);
		filebox->unlock_window();
	}
}


