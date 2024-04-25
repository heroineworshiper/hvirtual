
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

#include "autoconf.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "keys.h"
#include "language.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "viewmenu.h"
#include "trackcanvas.h"





ShowAssets::ShowAssets(MWindow *mwindow, const char *hotkey)
 : BC_MenuItem(_("Show assets"), hotkey, hotkey[0])
{
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->show_assets); 
}

int ShowAssets::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->show_assets = get_checked();
	mwindow->gui->update(1,
		1,
		0,
		0,
		1, 
		0,
		0);
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("ShowAssets::handle_event");
	return 1;
}




ShowTitles::ShowTitles(MWindow *mwindow, const char *hotkey)
 : BC_MenuItem(_("Show titles"), hotkey, hotkey[0])
{
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->show_titles); 
}

int ShowTitles::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->show_titles = get_checked();
	mwindow->gui->update(1,
		1,
		0,
		0,
		1, 
		0,
		0);
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("ShowTitles::handle_event");
	return 1;
}



ShowTransitions::ShowTransitions(MWindow *mwindow, const char *hotkey)
 : BC_MenuItem(_("Show transitions"), hotkey, hotkey[0])
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->auto_conf->autos[TRANSITION_OVERLAYS]); 
}
int ShowTransitions::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->autos[TRANSITION_OVERLAYS] = get_checked();
	mwindow->gui->draw_overlays(1);
//	mwindow->gui->mainmenu->draw_items();
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("ShowTransitions::handle_event");
	return 1;
}





ShowAutomation::ShowAutomation(MWindow *mwindow, 
	const char *text,
	const char *hotkey,
	int subscript)
 : BC_MenuItem(text, hotkey, hotkey_to_code(hotkey))
{
	this->mwindow = mwindow;
	this->subscript = subscript;
	set_checked(mwindow->edl->session->auto_conf->autos[subscript]); 
}

int ShowAutomation::hotkey_to_code(const char *hotkey)
{
    if(!strcmp(hotkey, "F1")) return KEY_F1;
    else
    if(!strcmp(hotkey, "F2")) return KEY_F2;
    else
    if(!strcmp(hotkey, "F3")) return KEY_F3;
    else
    if(!strcmp(hotkey, "F4")) return KEY_F4;
    else
    if(!strcmp(hotkey, "F5")) return KEY_F5;
    else
    if(!strcmp(hotkey, "F6")) return KEY_F6;
    else
    if(!strcmp(hotkey, "F7")) return KEY_F7;
    else
    if(!strcmp(hotkey, "F8")) return KEY_F8;
    else
    if(!strcmp(hotkey, "F9")) return KEY_F9;
    else
    if(!strcmp(hotkey, "F10")) return KEY_F10;
    else
    if(!strcmp(hotkey, "F11")) return KEY_F11;
    else
    if(!strcmp(hotkey, "F12")) return KEY_F12;
    else
        return hotkey[0];
}

int ShowAutomation::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->auto_conf->autos[subscript] = get_checked();
// track height changes based on automation visibility
	mwindow->gui->update(1,
		1,
		0,
		0,
		1, 
		0,
		0);
	mwindow->gui->draw_overlays(1);
//	mwindow->gui->mainmenu->draw_items();
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("ShowAutomation::handle_event");
	return 1;
}

void ShowAutomation::update_toggle()
{
	set_checked(mwindow->edl->session->auto_conf->autos[subscript]);
}



PluginAutomation::PluginAutomation(MWindow *mwindow, const char *hotkey)
 : BC_MenuItem(_("Plugin keyframes"), hotkey, hotkey[0]) 
{ 
	this->mwindow = mwindow; 
}

int PluginAutomation::handle_event()
{
	set_checked(!get_checked());
	mwindow->edl->session->auto_conf->autos[PLUGIN_KEYFRAMES] = get_checked();
	mwindow->gui->draw_overlays(1);
	mwindow->gui->unlock_window();
	mwindow->gwindow->gui->update_toggles(1);
	mwindow->gui->lock_window("PluginAutomation::handle_event");
	return 1;
}


