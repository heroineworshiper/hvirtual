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

#include "autoconf.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindowgui.h"
#include "keys.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "trackcanvas.h"





GWindowGUI::GWindowGUI(MWindow *mwindow,
	int w,
	int h)
 : BC_Window(PROGRAM_NAME ": Overlays",
 	mwindow->session->gwindow_x, 
    mwindow->session->gwindow_y, 
    w, 
    h,
    w,
    h,
    0,
    0,
    1)
{
	this->mwindow = mwindow;
	drag_operation = 0;
	new_status = 0;
}

static const char *other_text[] =
{
	"Assets",
	"Titles",
	"Transitions",
	"Plugin Autos"
};

static const char* other_keys[] = 
{
    "0", "1", "2", "7"
};

static const char *auto_text[] = 
{
	"Mute",          // 0
	"Camera X",     // 1
	"Camera Y",     // 2
	"Camera Z",     // 3
	"Projector X",  // 4
	"Projector Y",  // 5
	"Projector Z",  // 6
	"Fade",          // 7
	"Pan",           // 8
	"Mode",          // 9
	"Mask",          // 10
	"Speed"          // 11
};

static const int auto_keys[] = 
{
    '4', KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, '3', '6', '5', '8', '9'
};

static const char* auto_keys2[] = 
{
    "4", "F1", "F2", "F3", "F4", "F5", "F6", "3", "6", "5", "8", "9"
};

void GWindowGUI::calculate_extents(BC_WindowBase *gui, int *w, int *h, int *x2)
{
    int margin = BC_Resources::theme->widget_border;
	int temp1, temp2, temp3, temp4, temp5, temp6, temp7;
	int current_w, current_h;
	*w = 0;
	*h = margin;
    int key_w = 0;
    int max_key_w = 0;

// width of toggles
	for(int i = 0; i < OTHER_TOGGLES; i++)
	{
		BC_Toggle::calculate_extents(gui, 
			BC_WindowBase::get_resources()->checkbox_images,
			0,
			&temp1,
			&current_w,
			&current_h,
			&temp2,
			&temp3,
			&temp4,
			&temp5, 
			&temp6,
			&temp7, 
			other_text[i]);

		*w = MAX(current_w, *w);
		*h += current_h;

        key_w = BC_Title::calculate_w(gui, 
            other_keys[i]);
        if(key_w > max_key_w) max_key_w = key_w;
	}

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		BC_Toggle::calculate_extents(gui, 
			BC_WindowBase::get_resources()->checkbox_images,
			0,
			&temp1,
			&current_w,
			&current_h,
			&temp2,
			&temp3,
			&temp4,
			&temp5, 
			&temp6,
			&temp7, 
			auto_text[i]);
		*w = MAX(current_w, *w);
		*h += current_h;

        key_w = BC_Title::calculate_w(gui, 
            auto_keys2[i]);
        if(key_w > max_key_w) max_key_w = key_w;
	}

    *x2 = *w + margin * 2;
	*h += margin;
	*w += margin * 3 + max_key_w;
}



void GWindowGUI::create_objects()
{
    int margin = BC_Resources::theme->widget_border;
	int x = margin, y = margin;
    int x2 = 0;
    int w1, h1;
	lock_window("GWindowGUI::create_objects 1");
    calculate_extents(this, &w1, &h1, &x2);
    int title_h = BC_Title::calculate_h(this, "F1");


#define DO_OTHER(i) \
	add_tool(other[i] = new GWindowToggle(mwindow, \
		this, \
		x, \
		y, \
		-1, \
		i, \
		other_text[i])); \
    add_tool(new BC_Title(x2, y + other[i]->get_h() - title_h, other_keys[i])); \
	y += other[i]->get_h();

#define DO_AUTO(i) \
	add_tool(auto_toggle[i] = new GWindowToggle(mwindow, \
		this, \
		x, \
		y, \
		i, \
		-1, \
		auto_text[i])); \
    add_tool(new BC_Title(x2, y + auto_toggle[i]->get_h() - title_h, auto_keys2[i])); \
	y += auto_toggle[i]->get_h();
    
// put them in the same order as the menu
    DO_OTHER(0);
    DO_OTHER(1);
    DO_OTHER(2);
    DO_AUTO(7);
    DO_AUTO(0);
    DO_AUTO(9);
    DO_AUTO(8);
    DO_OTHER(3);
    DO_AUTO(10);
    DO_AUTO(11);
    DO_AUTO(1);
    DO_AUTO(2);
    DO_AUTO(3);
    DO_AUTO(4);
    DO_AUTO(5);
    DO_AUTO(6);

	unlock_window();
}

void GWindowGUI::update_mwindow()
{
	unlock_window();
	mwindow->gui->mainmenu->update_toggles(1);
	lock_window("GWindowGUI::update_mwindow");
}

void GWindowGUI::update_toggles(int use_lock)
{
	if(use_lock) lock_window("GWindowGUI::update_toggles");

	for(int i = 0; i < OTHER_TOGGLES; i++)
	{
		other[i]->update();
	}

	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		auto_toggle[i]->update();
	}

	if(use_lock) unlock_window();
}

int GWindowGUI::translation_event()
{
	mwindow->session->gwindow_x = get_x();
	mwindow->session->gwindow_y = get_y();
	return 0;
}

int GWindowGUI::close_event()
{
	hide_window();
	mwindow->session->show_gwindow = 0;
	unlock_window();

	mwindow->gui->lock_window("GWindowGUI::close_event");
	mwindow->gui->mainmenu->show_gwindow->set_checked(0);
	mwindow->gui->unlock_window();

	lock_window("GWindowGUI::close_event");
	mwindow->save_defaults();
	return 1;
}

int GWindowGUI::keypress_event()
{
    GWindowToggle *toggle = 0;
    for(int i = 0; i < OTHER_TOGGLES && !toggle; i++)
    {
        if(get_keypress() == other_keys[i][0]) toggle = other[i];
    }
    
    for(int i = 0; i < AUTOMATION_TOTAL && !toggle; i++)
    {
        if(get_keypress() == auto_keys[i]) toggle = auto_toggle[i];
    }
    
    if(toggle)
    {
        toggle->set_value(!toggle->get_value());
        toggle->handle_event();
        return 1;
    }

	switch(get_keypress())
	{
		case 'w':
		case 'W':
			if(ctrl_down())
			{
				close_event();
				return 1;
			}
			break;
	}
	return 0;
}

int GWindowGUI::cursor_motion_event()
{
	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();
//	int update_gui = 0;

	if(drag_operation)
	{
		if(cursor_y >= 0 &&
			cursor_y < get_h())
		{
			for(int i = 0; i < MAX(OTHER_TOGGLES, AUTOMATION_TOTAL); i++)
			{
				if(i < OTHER_TOGGLES)
				{
					if(cursor_y >= other[i]->get_y() &&
						cursor_y < other[i]->get_y() + other[i]->get_h())
					{
                        int prev_status = other[i]->BC_Toggle::get_value();
						other[i]->BC_Toggle::update(new_status);
                        if(prev_status != new_status) other[i]->handle_event();
//						update_gui = 1;
					}
				}
				
				if(i < AUTOMATION_TOTAL)
				{
					if(cursor_y >= auto_toggle[i]->get_y() &&
						cursor_y < auto_toggle[i]->get_y() + auto_toggle[i]->get_h())
					{
                        int prev_status = auto_toggle[i]->BC_Toggle::get_value();
						auto_toggle[i]->BC_Toggle::update(new_status);
                        if(prev_status != new_status) auto_toggle[i]->handle_event();
//						update_gui = 1;
					}
				}
			}
		}
	}

	return 0;
}









GWindowToggle::GWindowToggle(MWindow *mwindow, 
	GWindowGUI *gui, 
	int x, 
	int y, 
	int subscript, 
	int other,
	const char *text)
 : BC_CheckBox(x, 
 	y, 
	*get_main_value(mwindow, subscript, other), 
	text)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->subscript = subscript;
	this->other = other;
	set_select_drag(1);
}


int GWindowToggle::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
//printf("GWindowToggle::button_press_event %d %d\n", __LINE__, get_value());
		set_status(BC_Toggle::TOGGLE_DOWN);

		BC_Toggle::update(!get_value());
		gui->drag_operation = 1;
		gui->new_status = get_value();
//printf("GWindowToggle::button_press_event %d %d\n", __LINE__, get_value());

		return 1;
	}
	return 0;
}

int GWindowToggle::button_release_event()
{
    if(is_event_win())
    {
	    int result = BC_Toggle::button_release_event();
	    gui->drag_operation = 0;
	    handle_event();
	    return result;
    }
    else
        return 0;
}



int GWindowToggle::handle_event()
{
	*get_main_value(mwindow, subscript, other) = get_value();
	gui->update_mwindow();


// Update stuff in MWindow
	unlock_window();
	mwindow->gui->lock_window("GWindowToggle::handle_event");
	if(subscript >= 0)
	{
// track height changes based on automation visibility
		mwindow->gui->update(1,
			1,
			0,
			0,
			1, 
			0,
			0);
		mwindow->gui->draw_overlays(1, 1);
	}
	else
	{
		switch(other)
		{
			case ASSETS:
			case TITLES:
				mwindow->gui->update(1,
					1,
					0,
					0,
					1, 
					0,
					0);
				break;

			case TRANSITIONS:
			case PLUGIN_AUTOS:
				mwindow->gui->draw_overlays(1, 1);
				break;
		}
	}

	mwindow->gui->unlock_window();
	lock_window("GWindowToggle::handle_event");

	return 1;
}

int* GWindowToggle::get_main_value(MWindow *mwindow, int subscript, int other)
{
	if(subscript >= 0)
	{
		return &mwindow->edl->session->auto_conf->autos[subscript];
	}
	else
	{
		switch(other)
		{
			case ASSETS:
				return &mwindow->edl->session->show_assets;
				break;
			case TITLES:
				return &mwindow->edl->session->show_titles;
				break;
			case TRANSITIONS:
				return &mwindow->edl->session->auto_conf->autos[TRANSITION_OVERLAYS];
				break;
			case PLUGIN_AUTOS:
				return &mwindow->edl->session->auto_conf->autos[PLUGIN_KEYFRAMES];
				break;
		}
	}
    return 0;
}

void GWindowToggle::update()
{
	set_value(*get_main_value(mwindow, subscript, other));
}



