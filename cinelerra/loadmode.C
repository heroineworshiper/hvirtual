
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#include "clip.h"
#include "language.h"
#include "loadmode.h"
#include "mwindow.h"
#include "theme.h"

// Must match macros
static const char *mode_images[] = 
{
	"loadmode_none",
	"loadmode_new",
	"loadmode_newcat",
	"loadmode_newtracks",
	"loadmode_cat",
	"loadmode_paste",
	"loadmode_resource",
	"loadmode_nested"
};
static const char *tooltips[] = 
{
	"Insert nothing",
	"Replace current project",
	"Replace current project and concatenate tracks",
	"Append in new tracks",
	"Concatenate to existing tracks",
	"Paste at insertion point",
	"Create new resources only",
	"Nest sequence"
};




LoadMode::LoadMode(MWindow *mwindow,
	BC_WindowBase *window, 
	int x, 
	int y, 
	int *output, 
	int use_nothing,
	int use_nested)
{
	this->mwindow = mwindow;
	this->window = window;
	this->x = x;
	this->y = y;
	this->output = output;
	this->use_nothing = use_nothing;
	this->use_nested = use_nested;
	for(int i = 0; i < TOTAL_LOADMODES; i++)
		mode[i] = 0;
}

LoadMode::~LoadMode()
{
	delete title;
	for(int i = 0; i < TOTAL_LOADMODES; i++)
		if(mode[i]) delete mode[i];
}

int LoadMode::calculate_h(BC_WindowBase *gui, Theme *theme)
{
	int text_line;
	int w;
	int h;
	int toggle_x;
	int toggle_y;
	int text_x;
	int text_y;
	int text_w;
	int text_h;
	BC_Toggle::calculate_extents(gui, 
		theme->get_image_set(mode_images[0]),
		0,
		&text_line,
		&w,
		&h,
		&toggle_x,
		&toggle_y,
		&text_x,
		&text_y, 
		&text_w,
		&text_h, 
		0);
	return h;
}

int LoadMode::calculate_w(BC_WindowBase *gui, 
	Theme *theme, 
	int use_none, 
	int use_nested)
{
	int total = gui->get_text_width(MEDIUMFONT, _("Insertion strategy:")) + 10;
	for(int i = 0; i < TOTAL_LOADMODES; i++)
	{
		if((i != LOADMODE_NOTHING || use_none) &&
			(i != LOADMODE_NESTED || use_nested))
		{
			int text_line;
			int w;
			int h;
			int toggle_x;
			int toggle_y;
			int text_x;
			int text_y;
			int text_w;
			int text_h;
			BC_Toggle::calculate_extents(gui, 
				theme->get_image_set(mode_images[i]),
				0,
				&text_line,
				&w,
				&h,
				&toggle_x,
				&toggle_y,
				&text_x,
				&text_y, 
				&text_w,
				&text_h, 
				0);
			total += w + 10;
		}
	}
	return total;
}

int LoadMode::get_h()
{
	int result = 0;
	result = MAX(result, title->get_h());
	result = MAX(result, mode[1]->get_h());
	return result;
}

void LoadMode::create_objects()
{
	int x = this->x, y = this->y;

	window->add_subwindow(title = new BC_Title(x, y, _("Insertion strategy:")));
	x += title->get_w() + 10;
	int x1 = x;
	for(int i = 0; i < TOTAL_LOADMODES; i++)
	{
		if((i != LOADMODE_NOTHING || use_nothing) &&
			(i != LOADMODE_NESTED || use_nested))
		{
			VFrame **images = mwindow->theme->get_image_set(mode_images[i]);
			if(x + images[0]->get_w() > window->get_w())
			{
				x = x1;
				y += images[0]->get_h() + 5;
			}
			window->add_subwindow(mode[i] = new LoadModeToggle(x, 
				y, 
				this, 
				i, 
				mode_images[i],
				tooltips[i]));
			x += mode[i]->get_w() + 10;
		}
	}


}

int LoadMode::get_x()
{
	return x;
}

int LoadMode::get_y()
{
	return y;
}

int LoadMode::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	title->reposition_window(x, y);
	x += title->get_w() + 10;
	int x1 = x;
	for(int i = 0; i < TOTAL_LOADMODES; i++)
	{
		if(mode[i])
		{
			VFrame **images = mwindow->theme->get_image_set(mode_images[i]);
			if(x + images[0]->get_w() > window->get_w())
			{
				x = x1;
				y += images[0]->get_h() + 5;
			}
			mode[i]->reposition_window(x, y);
			x += mode[i]->get_w() + 10;
		}
	}

	return 0;
}

void LoadMode::update()
{
	for(int i = 0; i < TOTAL_LOADMODES; i++)
	{
		if(mode[i])
		{
			mode[i]->set_value(*output == i);
		}
	}
}








LoadModeToggle::LoadModeToggle(int x, 
	int y, 
	LoadMode *window, 
	int value, 
	const char *images,
	const char *tooltip)
 : BC_Toggle(x, 
 	y, 
	window->mwindow->theme->get_image_set(images),
	*window->output == value)
{
	this->window = window;
	this->value = value;
	set_tooltip(_(tooltip));
}

int LoadModeToggle::handle_event()
{
	*window->output = value;
	window->update();
	return 1;
}


