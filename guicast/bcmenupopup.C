
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

#include "bcmenubar.h"
#include "bcmenuitem.h"
#include "bcmenupopup.h"
#include "bcpixmap.h"
#include "bcpopup.h"
#include "bcresources.h"
#include "bcwindowbase.h"
#include "clip.h"

#include <string.h>



// ==================================== Menu Popup =============================

// Types of menu popups
#define MENUPOPUP_MENUBAR 0
#define MENUPOPUP_SUBMENU 1
#define MENUPOPUP_POPUP   2

BC_MenuPopup::BC_MenuPopup()
{
	window_bg = 0;
	item_bg[0] = 0;
	item_bg[1] = 0;
	item_bg[2] = 0;
	check = 0;
}

BC_MenuPopup::~BC_MenuPopup()
{
	while(menu_items.size())
	{
// Each menuitem recursively removes itself from the arraylist
		delete menu_items.get(0);
	}

	delete window_bg;
	delete item_bg[0];
	delete item_bg[1];
	delete item_bg[2];
	delete check;
}

int BC_MenuPopup::initialize(BC_WindowBase *top_level, 
		BC_MenuBar *menu_bar, 
		BC_Menu *menu, 
		BC_MenuItem *menu_item, 
		BC_PopupMenu *popup_menu)
{
	popup = 0;
	active = 0;
	this->menu = menu;
	this->menu_bar = menu_bar;
	this->menu_item = menu_item;
	this->popup_menu = popup_menu;
	this->top_level = top_level;

	if(menu_item) this->type = MENUPOPUP_SUBMENU;
	else
	if(menu) this->type = MENUPOPUP_MENUBAR;
	else
	if(popup_menu) this->type = MENUPOPUP_POPUP;

	BC_Resources *resources = top_level->get_resources();
	if(resources->menu_popup_bg)
	{
		window_bg = new BC_Pixmap(top_level, resources->menu_popup_bg);
	}
	
	if(resources->menu_item_bg)
	{
		item_bg[0] = new BC_Pixmap(top_level, resources->menu_item_bg[0], PIXMAP_ALPHA);
		item_bg[1] = new BC_Pixmap(top_level, resources->menu_item_bg[1], PIXMAP_ALPHA);
		item_bg[2] = new BC_Pixmap(top_level, resources->menu_item_bg[2], PIXMAP_ALPHA);
	}

	if(resources->check)
	{
		check = new BC_Pixmap(top_level, resources->check, PIXMAP_ALPHA);
	}

	return 0;
}

int BC_MenuPopup::add_item(BC_MenuItem *item)
{
	menu_items.append(item);
	item->initialize(top_level, menu_bar, this);
	return 0;
}

int BC_MenuPopup::remove_item(BC_MenuItem *item, int recursive)
{
	if(!item && menu_items.size() > 0)
	{
		item = menu_items.get(menu_items.size() - 1);
	}

	if(item)
	{
		menu_items.remove(item);
		item->menu_popup = 0;
		if(!recursive) delete item;
	}
	return 0;
}

int BC_MenuPopup::total_menuitems()
{
	return menu_items.total;
}

int BC_MenuPopup::dispatch_button_press()
{
	int result = 0;
	if(popup)
	{
		for(int i = 0; i < menu_items.total && !result; i++)
		{
			result = menu_items.values[i]->dispatch_button_press();
		}
		if(result) draw_items();
	}
	return 0;
}

int BC_MenuPopup::dispatch_button_release()
{
	int result = 0, redraw = 0;
	if(popup)
	{
		for(int i = 0; i < menu_items.total && !result; i++)
		{
			result = menu_items.values[i]->dispatch_button_release(redraw);
		}
		if(redraw) draw_items();
	}
	return result;
}

int BC_MenuPopup::dispatch_key_press()
{
	int result = 0;
	for(int i = 0; i < menu_items.total && !result; i++)
	{
		result = menu_items.values[i]->dispatch_key_press();
	}
	return result;
}

int BC_MenuPopup::dispatch_motion_event()
{
	int i, result = 0, redraw = 0;
	Window tempwin;

	if(popup)
	{
// Try submenus and items
		for(i = 0; i < menu_items.total; i++)
		{
			result |= menu_items.values[i]->dispatch_motion_event(redraw);
		}

		if(redraw) draw_items();
	}

	return result;
}

int BC_MenuPopup::dispatch_translation_event()
{
	if(popup)
	{
		int new_x = x + 
			(top_level->last_translate_x - 
			top_level->prev_x - 
			top_level->get_resources()->get_left_border());
		int new_y = y + 
			(top_level->last_translate_y - 
			top_level->prev_y -
			top_level->get_resources()->get_top_border());

// printf("BC_MenuPopup::dispatch_translation_event %d %d %d %d\n", 
// top_level->prev_x, 
// top_level->last_translate_x, 
// top_level->prev_y, 
// top_level->last_translate_y);
		popup->reposition_window(new_x, new_y, popup->get_w(), popup->get_h());
		top_level->flush();
		this->x = new_x;
		this->y = new_y;

		for(int i = 0; i < menu_items.total; i++)
		{
			menu_items.values[i]->dispatch_translation_event();
		}
	}
	return 0;
}


int BC_MenuPopup::dispatch_cursor_leave()
{
	int result = 0;
	
	if(popup)
	{
		for(int i = 0; i < menu_items.total; i++)
		{
			result |= menu_items.values[i]->dispatch_cursor_leave();
		}
		if(result) draw_items();
	}
	return 0;
}

int BC_MenuPopup::activate_menu(int x, 
	int y, 
	int w, 
	int h, 
	int top_window_coords, 
	int vertical_justify)
{
	Window tempwin;
	int new_x, new_y, top_w, top_h;
	top_w = top_level->get_root_w(1, 0);
	top_h = top_level->get_root_h(0);

	get_dimensions();

// Coords are relative to the main window
	if(top_window_coords)
		XTranslateCoordinates(top_level->display, 
			top_level->win, 
			top_level->rootwin, 
			x, 
			y, 
			&new_x, 
			&new_y, 
			&tempwin);
	else
// Coords are absolute
	{
		new_x = x; 
		new_y = y; 
	}

// All coords are now relative to root window.
	if(vertical_justify)
	{
		this->x = new_x;
		this->y = new_y + h;
		if(this->x + this->w > top_w) this->x -= this->x + this->w - top_w; // Right justify
		if(this->y + this->h > top_h) this->y -= this->h + h; // Bottom justify
	}
	else
	{
		this->x = new_x + w;
		this->y = new_y;
		if(this->x + this->w > top_w) this->x = new_x - this->w;
		if(this->y + this->h > top_h) this->y = new_y + h - this->h;
	}

	active = 1;
	if(menu_bar)
	{
		popup = new BC_Popup(menu_bar, 
					this->x, 
					this->y, 
					this->w, 
					this->h, 
					top_level->get_resources()->menu_up,
					1,
					menu_bar->bg_pixmap);
	}
	else
	{
		popup = new BC_Popup(top_level, 
					this->x, 
					this->y, 
					this->w, 
					this->h, 
					top_level->get_resources()->menu_up,
					1,
					0);
//		popup->set_background(top_level->get_resources()->menu_bg);
	}

	draw_items();
	popup->show_window();
	return 0;
}

int BC_MenuPopup::deactivate_submenus(BC_MenuPopup *exclude)
{
	for(int i = 0; i < menu_items.total; i++)
	{
		menu_items.values[i]->deactivate_submenus(exclude);
	}
	return 0;
}

int BC_MenuPopup::deactivate_menu()
{
	deactivate_submenus(0);

	if(popup) delete popup;
	popup = 0;
	active = 0;

	return 0;
}

int BC_MenuPopup::draw_items()
{
    if(!popup) return 0;
	if(menu_bar)
		popup->draw_top_tiles(menu_bar, 0, 0, w, h);
	else
		popup->draw_top_tiles(popup, 0, 0, w, h);

	if(window_bg)
	{
		popup->draw_9segment(0,
			0,
			w,
			h,
			window_bg);
	}
	else
	{
		popup->draw_3d_border(0, 0, w, h, 
			top_level->get_resources()->menu_light,
			top_level->get_resources()->menu_up,
			top_level->get_resources()->menu_shadow,
			BLACK);
	}

	for(int i = 0; i < menu_items.total; i++)
	{
		menu_items.values[i]->draw();
	}
	popup->flash();


	return 0;
}

int BC_MenuPopup::get_dimensions()
{
	int widest_text = 10, widest_key = 10;
	int text_w, key_w;
	int i = 0;

// pad for border
	h = 2;
// Set up parameters in each item and get total h. 
	for(i = 0; i < menu_items.total; i++)
	{
		text_w = 10 + top_level->get_text_width(MEDIUMFONT, menu_items.values[i]->text);
		if(menu_items.values[i]->checked) text_w += check->get_w() + 1;

		key_w = 10 + top_level->get_text_width(MEDIUMFONT, menu_items.values[i]->hotkey_text);
		if(text_w > widest_text) widest_text = text_w;
		if(key_w > widest_key) widest_key = key_w;

		if(!strcmp(menu_items.values[i]->text, "-")) 
			menu_items.values[i]->h = 5;
		else
		{
//			menu_items.values[i]->h = top_level->get_text_height(MEDIUMFONT) + 4;
			menu_items.values[i]->h = item_bg[0]->get_h();
		}

		menu_items.values[i]->y = h;
		menu_items.values[i]->highlighted = 0;
		menu_items.values[i]->down = 0;
		h += menu_items.values[i]->h;
	}
	w = widest_text + widest_key + 10;

	w = MAX(w, top_level->get_resources()->min_menu_w);
// pad for division
	key_x = widest_text + 5;
// pad for border
	h += 2;
	return 0;
}

int BC_MenuPopup::get_key_x()
{
	return key_x;
}

BC_Popup* BC_MenuPopup::get_popup()
{
	return popup;
}

int BC_MenuPopup::get_w()
{
	return w;
}





// ================================= Sub Menu ==================================

BC_SubMenu::BC_SubMenu() : BC_MenuPopup()
{
}

BC_SubMenu::~BC_SubMenu()
{
}

int BC_SubMenu::add_submenuitem(BC_MenuItem *item)
{
	add_item(item);
	return 0;
}

