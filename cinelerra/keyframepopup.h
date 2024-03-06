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

#ifndef KEYFRAMEPOPUP_H
#define KEYFRAMEPOPUP_H

#include "autoconf.inc"
#include "guicast.h"
#include "keyframegui.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "plugindialog.inc"
#include "keyframe.inc"
#include "automation.inc" 


class KeyframePopup;


class KeyframePopupEdit : public BC_MenuItem
{
public:
	KeyframePopupEdit(MWindow *mwindow, KeyframePopup *popup);
	int handle_event();
	MWindow *mwindow;
	KeyframePopup *popup;
};

// class KeyframePopupCopyDefault : public BC_MenuItem
// {
// public:
// 	KeyframePopupCopyDefault(MWindow *mwindow, KeyframePopup *popup);
// 	int handle_event();
// 	MWindow *mwindow;
// 	KeyframePopup *popup;
// };

class KeyframePopupPasteDefault : public BC_MenuItem
{
public:
	KeyframePopupPasteDefault(MWindow *mwindow, KeyframePopup *popup);
	int handle_event();
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupLinear : public BC_MenuItem
{
public:
	KeyframePopupLinear(MWindow *mwindow, KeyframePopup *popup);
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupBezier : public BC_MenuItem
{
public:
	KeyframePopupBezier(MWindow *mwindow, KeyframePopup *popup);
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupBezier2 : public BC_MenuItem
{
public:
	KeyframePopupBezier2(MWindow *mwindow, KeyframePopup *popup);
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupDelete : public BC_MenuItem
{
public:
	KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup);
	~KeyframePopupDelete();
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupHide : public BC_MenuItem
{
public:
	KeyframePopupHide(MWindow *mwindow, KeyframePopup *popup);
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupCopy : public BC_MenuItem
{
public:
	KeyframePopupCopy(MWindow *mwindow, KeyframePopup *popup, const char *title);
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupPaste : public BC_MenuItem
{
public:
	KeyframePopupPaste(MWindow *mwindow, KeyframePopup *popup, const char *text);
	int handle_event();
	MWindow *mwindow;
	KeyframePopup *popup;
};


class KeyframePopupPreset : public BC_MenuItem
{
public:
	KeyframePopupPreset(MWindow *mwindow, KeyframePopup *popup);
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};


class KeyframePopup : public BC_PopupMenu
{
public:
	KeyframePopup(MWindow *mwindow, MWindowGUI *gui);
	~KeyframePopup();

	void create_objects();
	int update(double position,
        Plugin *plugin, // enables preset operations
        Autos *autos, // enables default keyframe operations
        Auto *auto_); // enables single keyframe operations

	MWindow *mwindow;
	MWindowGUI *gui;

// pointers into the EDL
    double position;
	Plugin *plugin;
	Autos *autos;
	Auto *auto_;


    BC_MenuItem *bar;
	KeyframePopupEdit *edit;
	KeyframePopupLinear *key_linear;
	KeyframePopupBezier *key_bezier;
	KeyframePopupBezier2 *key_bezier2;
	KeyframePopupDelete *key_delete;
	KeyframePopupHide *key_hide;
	KeyframePopupCopy *key_copy;
    KeyframePopupPaste *paste;
	KeyframePopupPreset *preset;
    KeyframePopupPasteDefault *paste_default;
};

 #endif
