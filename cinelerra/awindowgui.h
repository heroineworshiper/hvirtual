/*
 * CINELERRA
 * Copyright (C) 2008-2026 Adam Williams <broadcast at earthling dot net>
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

#ifndef AWINDOWGUI_H
#define AWINDOWGUI_H

#include "arraylist.h"
#include "assetpopup.inc"
#include "asset.inc"
#include "assets.inc"
#include "awindow.inc"
#include "awindowmenu.inc"
#include "edl.inc"
#include "guicast.h"
#include "indexable.inc"
#include "mwindow.inc"
#include "newfolder.inc"
#include "pluginserver.inc"

//class AWindowAssets;
//class AWindowFolders;
//class AWindowNewFolder;
//class AWindowDeleteFolder;
//class AWindowRenameFolder;
class AWindowDeleteDisk;
class AWindowDeleteProject;
//class AWindowDivider;
class AWindowInfo;
class AWindowRedrawIndex;
class AWindowPaste;
//class AWindowAppend;
class AWindowView;

#define AWINDOW_COLUMNS 4

class AWindowGUI;

// Not really a picon anymore but a list item that references 
// an object in the EDL
class AssetPicon : public BC_ListBoxItem
{
public:
	AssetPicon(MWindow *mwindow, 
        AWindowGUI *gui, 
        Asset *asset,
	    EDL *nested_edl,
        EDL *clip,
        PluginServer *plugin);
	virtual ~AssetPicon();

	void create_objects(Asset *asset,
	    EDL *nested_edl,
        EDL *clip,
        PluginServer *plugin);
	void reset();

	MWindow *mwindow;
	AWindowGUI *gui;
	BC_Pixmap *icon;
	VFrame *icon_vframe;
// ID of thing pointed to
	int id;

// Check ID first.  Update these next before dereferencing
// TODO: probably need to store ID's instead of pointers
// Asset if asset or nested EDL
//	Indexable *indexable;
// EDL if clip
//	EDL *edl;
// Server if a plugin
	PluginServer *plugin;
// non persistent objects
    int is_nested;
    int is_asset;
    int is_clip;

	int in_use;
    int64_t size;
    int month;
    int day;
    int year;
    int64_t calendar_time;

	int persistent;
};

class AWindowFolders : public BC_ListBox
{
public:
	AWindowFolders(AWindowGUI *gui, int x, int y);
	int handle_event();
    AWindowGUI *gui;
};

class AWindowAssets : public BC_ListBox
{
public:
	AWindowAssets(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h);
	~AWindowAssets();
	
	int handle_event();
	int selection_changed();
//	void draw_background();
	int drag_start_event();
	int drag_motion_event();
	int drag_stop_event();
	int button_press_event();
	int column_resize_event();
    int sort_order_event();
    int move_column_event();
    int evaluate_query(char *string);

	MWindow *mwindow;
	AWindowGUI *gui;
};


class AWindowGUI : public BC_Window
{
public:
	AWindowGUI(MWindow *mwindow, AWindow *awindow);
	~AWindowGUI();

	void create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	int keypress_event();
// the mane updater
	void update_assets(int do_folder = 1);
    void clear_tables();
//	void sort_assets();
//	void reposition_objects();
	int current_folder_number();
// Call back for MWindow entry point
	int drag_motion();
	int drag_stop();
// Collect items into the drag vectors of MainSession
	void collect_assets();
	void create_persistent_folder(ArrayList<BC_ListBoxItem*> *output, 
		int do_audio, 
		int do_video, 
		int is_realtime, 
		int is_transition);
    const char* columntype_to_text(int type);
// Return the selected asset in asset_list
//	Indexable* selected_asset();
//	PluginServer* selected_plugin();
//	AssetPicon* selected_folder();

	MWindow *mwindow;
	AWindow *awindow;

    int folders_w;
    int folders_h;

	AWindowAssets *asset_list;
	AWindowFolders *folder_list;
    BC_Title *folder_title;
//	AWindowDivider *divider;

// Fixed tables for the listbox
// TODO: probably should be ditched if we're not making custom picons
	ArrayList<BC_ListBoxItem*> folders;
	ArrayList<BC_ListBoxItem*> assets;
	ArrayList<BC_ListBoxItem*> aeffects;
	ArrayList<BC_ListBoxItem*> veffects;
	ArrayList<BC_ListBoxItem*> atransitions;
	ArrayList<BC_ListBoxItem*> vtransitions;

// Currently displayed tables for the listbox
// Either BC_ListBoxItem or AssetPicon
	ArrayList<BC_ListBoxItem*> column_data[ASSET_COLUMNS];
	const char *column_titles[ASSET_COLUMNS];

// Fixed icons for dragging
	BC_Pixmap *folder_icon;
	BC_Pixmap *file_icon;
	BC_Pixmap *audio_icon;
	BC_Pixmap *video_icon;
	BC_Pixmap *clip_icon;
	BC_Pixmap *atransition_icon;
	BC_Pixmap *vtransition_icon;
	BC_Pixmap *aeffect_icon;
	BC_Pixmap *veffect_icon;
	NewFolderThread *newfolder_thread;

// Popup menus
	AssetPopup *asset_menu;
//	AssetListMenu *assetlist_menu;
//	FolderListMenu *folderlist_menu;
// Temporary for reading picons from files
//	VFrame *temp_picon;

private:
//	void update_folder_list();
	void update_asset_list();
	void filter_column_data();
	void copy_tables(ArrayList<BC_ListBoxItem*> *src, const char *folder);
	void sort_tables();
};


// class AWindowDivider : public BC_SubWindow
// {
// public:
// 	AWindowDivider(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h);
// 	~AWindowDivider();
// 
// 	int button_press_event();
// 	int cursor_motion_event();
// 	int button_release_event();
// 
// 	MWindow *mwindow;
// 	AWindowGUI *gui;
// };

// class AWindowFolders : public BC_ListBox
// {
// public:
// 	AWindowFolders(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h);
// 	~AWindowFolders();
// 	
// 	int selection_changed();
// 	int button_press_event();
// 
// 	MWindow *mwindow;
// 	AWindowGUI *gui;
// };
// 
// class AWindowNewFolder : public BC_Button
// {
// public:
// 	AWindowNewFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y);
// 	int handle_event();
// 	MWindow *mwindow;
// 	AWindowGUI *gui;
// 	int x, y;
// };
// 
// class AWindowDeleteFolder : public BC_Button
// {
// public:
// 	AWindowDeleteFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y);
// 	int handle_event();
// 	MWindow *mwindow;
// 	AWindowGUI *gui;
// 	int x, y;
// };
// 
// class AWindowRenameFolder : public BC_Button
// {
// public:
// 	AWindowRenameFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y);
// 	int handle_event();
// 	MWindow *mwindow;
// 	AWindowGUI *gui;
// 	int x, y;
// };

class AWindowDeleteDisk : public BC_Button
{
public:
	AWindowDeleteDisk(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowDeleteProject : public BC_Button
{
public:
	AWindowDeleteProject(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowInfo : public BC_Button
{
public:
	AWindowInfo(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowRedrawIndex : public BC_Button
{
public:
	AWindowRedrawIndex(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowPaste : public BC_Button
{
public:
	AWindowPaste(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

// class AWindowAppend : public BC_Button
// {
// public:
// 	AWindowAppend(MWindow *mwindow, AWindowGUI *gui, int x, int y);
// 	int handle_event();
// 	MWindow *mwindow;
// 	AWindowGUI *gui;
// 	int x, y;
// };

class AWindowView : public BC_Button
{
public:
	AWindowView(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

#endif
