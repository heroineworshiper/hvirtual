
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

#include "asset.h"
#include "confirmsave.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"



ConfirmSave::ConfirmSave()
{
}

ConfirmSave::~ConfirmSave()
{
}

int ConfirmSave::test_file(MWindow *mwindow, char *path)
{
	ArrayList<char*> paths;
	paths.append(path);
	int result = test_files(mwindow, &paths);
	paths.remove_all();
	return result;
}

int ConfirmSave::test_files(MWindow *mwindow, ArrayList<char*> *paths)
{
	ArrayList<string*> strings;
	for(int i = 0; i < paths->size(); i++)
	{
		strings.append(new string(paths->get(i)));
	}
	
	int result = test_files(mwindow, &strings);
	strings.remove_all_objects();
	return result;
}

int ConfirmSave::test_files(MWindow *mwindow, 
	ArrayList<string*> *paths)
{
	FILE *file;
	ArrayList<BC_ListBoxItem*> list;
	int result = 0;

	for(int i = 0; i < paths->size(); i++)
	{
		string *path = paths->get(i);
		if(file = fopen(path->c_str(), "r"))
		{
			fclose(file);
			list.append(new BC_ListBoxItem(path->c_str()));
		}
	}

	if(list.total)
	{
		if(mwindow)
		{
			ConfirmSaveWindow window(mwindow, &list);
			window.create_objects();
			window.raise_window();
			result = window.run_window();
		}
		else
		{
			printf("The following files exist:\n");
			for(int i = 0; i < list.total; i++)
			{
				printf("    %s\n", list.values[i]->get_text());
			}
			printf("Won't overwrite existing files.\n");
			result = 1;
		}
		list.remove_all_objects();
		return result;
	}
	else
	{
		list.remove_all_objects();
		return 0;
	}

	return result;
}









ConfirmSaveWindow::ConfirmSaveWindow(MWindow *mwindow, 
	ArrayList<BC_ListBoxItem*> *list)
 : BC_Window(PROGRAM_NAME ": File Exists", 
 		mwindow->gui->get_abs_cursor_x(1) - DP(160), 
		mwindow->gui->get_abs_cursor_y(1) - DP(120), 
		DP(480), 
		DP(320))
{
	this->list = list;
}

ConfirmSaveWindow::~ConfirmSaveWindow()
{
}


void ConfirmSaveWindow::create_objects()
{
    int margin = MWindow::theme->widget_border;
	int x = margin, y = margin;
	lock_window("ConfirmSaveWindow::create_objects");
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	add_subwindow(title = new BC_Title(x, 
		y, 
		_("The following files exist.  Overwrite them?")));
	y += title->get_h() + margin;
	add_subwindow(listbox = new BC_ListBox(x, 
		y, 
		get_w() - x - margin,
		get_h() - y - BC_OKButton::calculate_h() - margin * 2,
		LISTBOX_TEXT,
		list));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window(1);
	unlock_window();
}

int ConfirmSaveWindow::resize_event(int w, int h)
{
	int x = DP(10), y = DP(10);
	title->reposition_window(x, y);
	y += DP(30);
	listbox->reposition_window(x,
		y,
		w - x - DP(10),
		h - y - DP(50));
	return 1;
}






