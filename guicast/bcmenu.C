/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

#include "bcmenu.h"
#include "bcmenubar.h"
#include "bcmenuitem.h"
#include "bcmenupopup.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcresources.h"
#include "bcsignals.h"
#include <string.h>



// ==================================== Menu ===================================

BC_Menu::BC_Menu(const char *text)
{
	strcpy(this->text, text);
	menu_bar = 0;
	active = 0;
	highlighted = 0;
}

BC_Menu::~BC_Menu()
{
	delete menu_popup;
}

int BC_Menu::initialize(BC_WindowBase *top_level, 
		BC_MenuBar *menu_bar, 
		int x, 
		int y, 
		int w, 
		int h)
{
	this->x = x; 
	this->y = y; 
	this->w = w; 
	this->h = h;
	this->menu_bar = menu_bar;
	this->top_level = top_level;
	menu_popup = new BC_MenuPopup;
	menu_popup->initialize(top_level, menu_bar, this, 0, 0);
	draw_title(1, 0);
	return 0;
}

int BC_Menu::add_item(BC_MenuItem* menuitem)
{
	menu_popup->add_item(menuitem);
	return 0;
}

int BC_Menu::remove_item(BC_MenuItem *item)
{
	menu_popup->remove_item(item, 0);
	return 0;
}

int BC_Menu::total_menuitems()
{
	return menu_popup->total_menuitems();
}

int BC_Menu::dispatch_button_press()
{
	int result = 0;

// Menu is down so dispatch to popup
	if(active)
	{
		result = menu_popup->dispatch_button_press();
	}

// Try title.
	if(!result)
	{
		if(top_level->event_win == menu_bar->win &&
			top_level->cursor_x >= x && top_level->cursor_x < x + w &&
			top_level->cursor_y >= y && top_level->cursor_y < y + h)
		{
			if(!active)
			{
				menu_bar->deactivate();
				menu_bar->unhighlight();
				menu_bar->button_releases = 0;
				menu_bar->activate();
				activate_menu();
			}
			result = 1;
		}
	}
	return result;
}

int BC_Menu::dispatch_button_release()
{
// try the title
	int result = 0;
	if(top_level->event_win == menu_bar->win &&
		top_level->cursor_x >= x && top_level->cursor_y < x + w &&
		top_level->cursor_y >= y && top_level->cursor_y < y + h)
	{
		if(menu_bar->button_releases >= 2)
		{
			highlighted = 1;
			menu_bar->deactivate();
		}
		result = 1;
	}
	else
// try the popup
		result = menu_popup->dispatch_button_release();
	return result;
}

int BC_Menu::dispatch_keypress()
{
	return menu_popup->dispatch_key_press();
}

int BC_Menu::dispatch_motion_event()
{
	int result = 0;
	int cursor_x, cursor_y;
	Window tempwin;

// try the popup
	if(active)
	{
		result = menu_popup->dispatch_motion_event();
	}

	if(!result)
	{
		top_level->translate_coordinates(top_level->event_win, 
			menu_bar->win,
			top_level->cursor_x,
			top_level->cursor_y,
			&cursor_x,
			&cursor_y);

// change focus from other menu
		if(menu_bar->active && !active &&
			cursor_x >= x && cursor_x < x + w &&
			cursor_y >= y && cursor_y < y + h)
		{
			menu_bar->activate();
			activate_menu();
			result = 1;
		}
		else
// control highlighting
		if(highlighted)
		{
			if(cursor_x < x || cursor_x >= x + w ||
				cursor_y < y || cursor_y >= y + h)
			{
				highlighted = 0;
				draw_title(1, 1);
			}
		}
		else
		{
			if(cursor_x >= x && cursor_x < x + w &&
				cursor_y >= y && cursor_y < y + h)
			{
				menu_bar->unhighlight();
				highlighted = 1;
				draw_title(1, 1);
				result = 1;
			}
		}
	}
	return result;
}

int BC_Menu::dispatch_cursor_leave()
{
	if(active)
	{
		menu_popup->dispatch_cursor_leave();
	}
	unhighlight();
	return 0;
}

int BC_Menu::dispatch_translation_event()
{
	if(active)
	{
		menu_popup->dispatch_translation_event();
	}
	return 0;
}

int BC_Menu::activate_menu()
{
	Window tempwin;
	int new_x, new_y, top_w, top_h;
	if(menu_bar)
	{
		XTranslateCoordinates(top_level->display, 
			menu_bar->win, 
			top_level->rootwin, 
			x, 
			y, 
			&new_x, 
			&new_y, 
			&tempwin);
		menu_popup->activate_menu(new_x, new_y, w, h, 0, 1);
	}
	else
		menu_popup->activate_menu(x, y, w, h, 1, 1);

	active = 1;
	draw_title(1, 1);
	return 0;
}

void BC_Menu::update()
{
    if(menu_popup)
    {
        menu_popup->get_dimensions();
        if(menu_popup->popup)
            menu_popup->popup->reposition_window(
                menu_popup->x,
                menu_popup->y,
                menu_popup->w,
                menu_popup->h);
        menu_popup->draw_items();
    }
}

void BC_Menu::draw_items()
{
	if(active) menu_popup->draw_items();
}

int BC_Menu::set_text(char *text)
{
	strcpy(this->text, text);
	draw_title(1, 1);
	return 0;
}

int BC_Menu::draw_title(int flash, int flush)
{
	BC_Resources *resources = top_level->get_resources();
	int text_offset = 0;

	if(active && menu_popup)
	{
// Menu is pulled down and title is recessed.

		if(menu_bar->menu_title_bg[0])
		{

			menu_bar->draw_9segment(x, 0, w, menu_bar->get_h(), menu_bar->menu_title_bg[2]);
		}
		else
		{
			menu_bar->draw_3d_box(x, y, w, h, 
				resources->menu_shadow, 
				BLACK, 
				resources->menu_down,
				resources->menu_down,
				resources->menu_light);
		}
		text_offset = 1;
	}
	else
// Menu is not pulled down.
	{
		if(highlighted)
		{

			if(menu_bar->menu_title_bg[0])
			{

				menu_bar->draw_9segment(x, 0, w, menu_bar->get_h(), menu_bar->menu_title_bg[1]);
			}
			else
			{
				menu_bar->set_color(resources->menu_highlighted);
				menu_bar->draw_box(x, y, w, h);
			}
		}
		else
		{

			if(menu_bar->menu_title_bg[0])
			{

				menu_bar->draw_9segment(x, 0, w, menu_bar->get_h(), menu_bar->menu_title_bg[0]);
			}
			else
			{
				menu_bar->draw_background(x, y, w, h);
			}
		}
	}

	menu_bar->set_color(resources->menu_title_text);
	menu_bar->set_font(MEDIUMFONT);
	menu_bar->draw_text(x + 10 + text_offset, 
		h / 2 + menu_bar->get_text_ascent(MEDIUMFONT) / 2 + 1 + text_offset, 
		text);
	if(flash) menu_bar->flash(flush);

	return 0;
}

int BC_Menu::deactivate_menu()
{
	if(active)
	{
		menu_popup->deactivate_menu();
		active = 0;
		draw_title(1, 1);
	}
	return 0;
}

int BC_Menu::unhighlight()
{
	if(highlighted)
	{
		highlighted = 0;
		draw_title(1, 1);
	}
	return 0;
}
