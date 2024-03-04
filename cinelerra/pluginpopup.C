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
#include "keyframes.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "plugindialog.h"
#include "pluginpopup.h"
#include "presets.h"
#include "presetsgui.h"
#include "track.h"



PluginPopup::PluginPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	show = 0;
	presets = 0;
    copy_default = 0;
    paste_default = 0;
    paste = 0;
}

PluginPopup::~PluginPopup()
{
}

void PluginPopup::create_objects()
{
	add_item(change = new PluginPopupChange(mwindow, this));
	add_item(detach = new PluginPopupDetach(mwindow, this));
	add_item(new PluginPopupUp(mwindow, this));
	add_item(new PluginPopupDown(mwindow, this));
	add_item(on = new PluginPopupOn(mwindow, this));
}

int PluginPopup::update(double position,
    Plugin *plugin)
{
    this->position = position;
	this->plugin = plugin;

	if(show) remove_item(show);
	if(presets) remove_item(presets);
    if(copy_default) remove_item(copy_default);
    if(paste_default) remove_item(paste_default);
    if(paste) remove_item(paste);
	show = 0;
	presets = 0;
    copy_default = 0;
    paste_default = 0;
    paste = 0;

	if(plugin->plugin_type == PLUGIN_STANDALONE)
	{
		add_item(show = new PluginPopupShow(mwindow, this));
		add_item(presets = new PluginPresets(mwindow, this));
		show->set_checked(plugin->show);
        add_item(paste = new PluginPopupPaste(mwindow, this));
        add_item(copy_default = new PluginPopupCopyDefault(mwindow, this));
        add_item(paste_default = new PluginPopupPasteDefault(mwindow, this));
	}

	on->set_checked(plugin->on);
	return 0;
}









PluginPopupChange::PluginPopupChange(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Change..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new PluginDialogThread(mwindow);
}

PluginPopupChange::~PluginPopupChange()
{
	delete dialog_thread;
}

int PluginPopupChange::handle_event()
{
	dialog_thread->start_window(popup->plugin->track,
		popup->plugin,
		PROGRAM_NAME ": Change Effect",
		0,
		popup->plugin->track->data_type);
    return 0;
}








PluginPopupDetach::PluginPopupDetach(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Detach"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

PluginPopupDetach::~PluginPopupDetach()
{
}

int PluginPopupDetach::handle_event()
{
	mwindow->undo->update_undo_before();
	mwindow->hide_plugin(popup->plugin, 1);
	mwindow->hide_keyframe_gui(popup->plugin);
	popup->plugin->track->detach_effect(popup->plugin);
	mwindow->save_backup();
	mwindow->undo->update_undo_after(_("detach effect"), LOAD_ALL);


	mwindow->gui->lock_window("PluginPopupDetach::handle_event");
	mwindow->gui->update(0,
		1,
		0,
		0,
		0, 
		0,
		0);
	mwindow->gui->unlock_window();
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}







PluginPopupCopyDefault::PluginPopupCopyDefault(MWindow *mwindow, 
    PluginPopup *popup)
 : BC_MenuItem(_("Copy default keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
int PluginPopupCopyDefault::handle_event()
{
    mwindow->copy_keyframe(popup->plugin->keyframes->default_auto);
	return 1;
}





PluginPopupPasteDefault::PluginPopupPasteDefault(MWindow *mwindow, 
    PluginPopup *popup)
 : BC_MenuItem(_("Paste default keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
int PluginPopupPasteDefault::handle_event()
{
    mwindow->paste_automation(0,
        popup->plugin->keyframes, 
        popup->plugin->keyframes->default_auto);
	return 1;
}



PluginPopupPaste::PluginPopupPaste(MWindow *mwindow, 
    PluginPopup *popup)
 : BC_MenuItem(_("Paste keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
int PluginPopupPaste::handle_event()
{
    mwindow->paste_automation(popup->position,
        popup->plugin->keyframes, 
        0);
	return 1;
}



PluginPopupShow::PluginPopupShow(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Show"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

PluginPopupShow::~PluginPopupShow()
{
}

int PluginPopupShow::handle_event()
{
	mwindow->show_plugin(popup->plugin);
	mwindow->gui->update(0, 1, 0, 0, 0, 0, 0);
	return 1;
}




PluginPopupOn::PluginPopupOn(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("On"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

PluginPopupOn::~PluginPopupOn()
{
}

int PluginPopupOn::handle_event()
{
	popup->plugin->on = !get_checked();
	mwindow->gui->update(0, 1, 0, 0, 0, 0, 0);
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}


PluginPopupUp::PluginPopupUp(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Move up"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int PluginPopupUp::handle_event()
{
	mwindow->move_plugins_up(popup->plugin->plugin_set);
	return 1;
}



PluginPopupDown::PluginPopupDown(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Move down"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int PluginPopupDown::handle_event()
{
	mwindow->move_plugins_down(popup->plugin->plugin_set);
	return 1;
}



PluginPresets::PluginPresets(MWindow *mwindow, PluginPopup *popup)
 : BC_MenuItem(_("Presets..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int PluginPresets::handle_event()
{
	mwindow->show_keyframe_gui(popup->plugin, 0);
	return 1;
}

