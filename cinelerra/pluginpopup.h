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

#ifndef PLUGINPOPUP_H
#define PLUGINPOPUP_H

class PluginPopupChange;
class PluginPopupDetach;
class PluginPopupCopyDefault;
class PluginPopupPasteDefault;
class PluginPopupOn;
class PluginPopupShow;
class PluginPresets;
class PluginPopupPaste;

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "plugindialog.inc"
#include "presets.inc"
#include "presetsgui.inc"



class PluginPopup : public BC_PopupMenu
{
public:
	PluginPopup(MWindow *mwindow, MWindowGUI *gui);
	~PluginPopup();

	void create_objects();
	int update(double position, Plugin *plugin);

	MWindow *mwindow;
	MWindowGUI *gui;
// Acquired through the update command as the plugin currently being operated on
	Plugin *plugin;
    double position;


	PluginPopupChange *change;
	PluginPopupDetach *detach;
	PluginPopupShow *show;
	PluginPopupOn *on;
	PluginPresets *presets;
    PluginPopupCopyDefault *copy_default;
    PluginPopupPasteDefault *paste_default;
    PluginPopupPaste *paste;
};

class PluginPopupAttach : public BC_MenuItem
{
public:
	PluginPopupAttach(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupAttach();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
	PluginDialogThread *dialog_thread;
};

class PluginPopupChange : public BC_MenuItem
{
public:
   PluginPopupChange(MWindow *mwindow, PluginPopup *popup);
   ~PluginPopupChange();

   int handle_event();

   MWindow *mwindow;
   PluginPopup *popup;
   PluginDialogThread *dialog_thread;
};


class PluginPopupDetach : public BC_MenuItem
{
public:
	PluginPopupDetach(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupDetach();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
};


class PluginPopupPaste : public BC_MenuItem
{
public:
	PluginPopupPaste(MWindow *mwindow, PluginPopup *popup);
	int handle_event();
	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupCopyDefault : public BC_MenuItem
{
public:
	PluginPopupCopyDefault(MWindow *mwindow, PluginPopup *popup);
	int handle_event();
	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupPasteDefault : public BC_MenuItem
{
public:
	PluginPopupPasteDefault(MWindow *mwindow, PluginPopup *popup);
	int handle_event();
	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupShow : public BC_MenuItem
{
public:
	PluginPopupShow(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupShow();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupOn : public BC_MenuItem
{
public:
	PluginPopupOn(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupOn();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupUp : public BC_MenuItem
{
public:
	PluginPopupUp(MWindow *mwindow, PluginPopup *popup);
	int handle_event();
	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupDown : public BC_MenuItem
{
public:
	PluginPopupDown(MWindow *mwindow, PluginPopup *popup);
	int handle_event();
	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPresets : public BC_MenuItem
{
public:
	PluginPresets(MWindow *mwindow, PluginPopup *popup);
	int handle_event();
	MWindow *mwindow;
	PluginPopup *popup;
};


#endif
