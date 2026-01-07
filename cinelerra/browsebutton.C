
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

#include "bcsignals.h"
#include "browsebutton.h"
#include "filepreviewer.h"
#include "language.h"
#include "mutex.h"
#include "theme.h"




BrowseButton::BrowseButton(Theme *theme, 
	BC_WindowBase *parent_window, 
	BC_TextBox *textbox, 
	int x, 
	int y, 
	const char *init_directory, 
	const char *title, 
	const char *caption, 
	int want_directory)
 : BC_Button(x, y, theme->get_image_set("magnify_button")), 
   BC_DialogThread()
{
	this->parent_window = parent_window;
	this->want_directory = want_directory;
	this->title = title;
	this->caption = caption;
	this->init_directory = init_directory;
	this->textbox = textbox;
	this->theme = theme;
	set_tooltip(_("Look for file"));
}

BrowseButton::~BrowseButton()
{
}

int BrowseButton::handle_event()
{
	x = parent_window->get_abs_cursor_x(0);
	y = parent_window->get_abs_cursor_y(0);
    start();
    return 1;
}


BC_Window* BrowseButton::new_gui()
{
	BrowseButtonWindow *browsewindow = new BrowseButtonWindow(theme,
		this,
		parent_window, 
		textbox->get_text(), 
		title, 
		caption, 
		want_directory);

	browsewindow->create_objects();
	return browsewindow;
}

void BrowseButton::handle_done_event(int result)
{
	if(!result)
	{
// 		if(want_directory)
// 		{
// 			textbox->update(browsewindow.get_directory());
// 		}
// 		else
// 		{
// 			textbox->update(browsewindow.get_filename());
// 		}

        BrowseButtonWindow *browsewindow = (BrowseButtonWindow*)get_gui();
		parent_window->lock_window("BrowseButton::handle_close_event");
//printf("BrowseButton::handle_close_event %d %p %p %p\n", 
//__LINE__, textbox, browsewindow, browsewindow->get_submitted_path());
// TODO: don't complete the path
		textbox->update(browsewindow->get_submitted_path());
		parent_window->flush();
		textbox->handle_event();
		parent_window->unlock_window();
	}
}

void BrowseButton::handle_close_event(int result)
{
}






BrowseButtonWindow::BrowseButtonWindow(Theme *theme, 
	BrowseButton *button,
	BC_WindowBase *parent_window, 
	const char *init_directory, 
	const char *title, 
	const char *caption, 
	int want_directory)
 : BC_FileBox(button->x - 
 		BC_WindowBase::get_resources()->filebox_w / 2, 
 	button->y - 
		BC_WindowBase::get_resources()->filebox_h / 2,
	init_directory,
	title,
	caption,
// Set to 1 to get hidden files. 
	want_directory,
// Want only directories
	want_directory,
	0,
	theme->browse_pad)
{
    set_previewer(&FilePreviewer::instance);
}

BrowseButtonWindow::~BrowseButtonWindow() 
{
}
