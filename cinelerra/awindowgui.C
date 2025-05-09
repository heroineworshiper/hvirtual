/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "assetedit.h"
#include "assetpopup.h"
#include "assets.h"
#include "awindowgui.h"
#include "awindowgui.inc"
#include "awindow.h"
#include "awindowmenu.h"
#include "bcsignals.h"
#include "cache.h"
#include "colormodels.h"
#include "cursors.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "indexable.h"
#include "language.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "nestededls.h"
#include "newfolder.h"
#include "preferences.h"
#include "theme.h"
#include "vframe.h"
#include "vwindowgui.h"
#include "vwindow.h"




AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	Indexable *indexable)
 : BC_ListBoxItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->indexable = indexable;
	this->id = indexable->id;
}

AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	EDL *edl)
 : BC_ListBoxItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->edl = edl;
	this->id = edl->id;
}

AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	const char *folder)
 : BC_ListBoxItem(folder, gui->folder_icon)
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
}

AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	PluginServer *plugin)
 : BC_ListBoxItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->plugin = plugin;
}

AssetPicon::~AssetPicon()
{
	if(icon)
	{
		if(icon != gui->file_icon &&
			icon != gui->audio_icon &&
			icon != gui->folder_icon &&
			icon != gui->clip_icon &&
			icon != gui->video_icon &&
			icon != gui->veffect_icon &&
			icon != gui->vtransition_icon &&
			icon != gui->aeffect_icon &&
			icon != gui->atransition_icon) 
		{
			delete icon;
			delete icon_vframe;
		}
	}
}

void AssetPicon::reset()
{
	plugin = 0;
	indexable = 0;
	edl = 0;
	icon = 0;
	icon_vframe = 0;
	in_use = 1;
	id = 0;
	persistent = 0;
}

void AssetPicon::create_objects()
{
	FileSystem fs;
	char name[BCTEXTLEN];
	int pixmap_w, pixmap_h;
	const int debug = 0;

	pixmap_h = 50;

	if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
	if(indexable)
	{
		fs.extract_name(name, indexable->path);
		set_text(name);
	}
	
	if(indexable && indexable->is_asset)
	{
		if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
		Asset *asset = (Asset*)indexable;
		if(asset->video_data)
		{
			if(mwindow->preferences->use_thumbnails)
// 			{
// 				gui->unlock_window();
// 				if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
// 				File *file = mwindow->video_cache->check_out(asset, 
// 					mwindow->edl,
// 					1);
// 				if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
// 
// 				if(file)
// 				{
// 					pixmap_w = pixmap_h * asset->width / asset->height;
// 
// 					file->set_layer(0);
// 					file->set_video_position(0, 0);
// 
// 					if(gui->temp_picon && 
// 						(gui->temp_picon->get_w() != asset->width ||
// 						gui->temp_picon->get_h() != asset->height))
// 					{
// 						delete gui->temp_picon;
// 						gui->temp_picon = 0;
// 					}
// 
// 					if(!gui->temp_picon)
// 					{
// 						gui->temp_picon = new VFrame(0, 
// 							-1,
// 							asset->width, 
// 							asset->height, 
// 							BC_RGB888,
// 							-1);
// 					}
// 
// 					file->read_frame(gui->temp_picon);
// 					if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
// 					mwindow->video_cache->check_in(asset);
// 					if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
// 					gui->lock_window("AssetPicon::create_objects 1");
// 					if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
// 
// 					if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
// 					icon = new BC_Pixmap(gui, pixmap_w, pixmap_h);
// 					if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
// 					icon->draw_vframe(gui->temp_picon,
// 						0, 
// 						0, 
// 						pixmap_w, 
// 						pixmap_h,
// 						0,
// 						0);
// //printf("%d %d\n", gui->temp_picon->get_w(), gui->temp_picon->get_h());
// 					icon_vframe = new VFrame(0, 
// 						-1,
// 						pixmap_w, 
// 						pixmap_h, 
// 						BC_RGB888,
// 						-1);
// 					cmodel_transfer(icon_vframe->get_rows(), /* Leave NULL if non existent */
// 						gui->temp_picon->get_rows(),
// 						0, /* Leave NULL if non existent */
// 						0,
// 						0,
// 						0, /* Leave NULL if non existent */
// 						0,
// 						0,
// 						0,        /* Dimensions to capture from input frame */
// 						0, 
// 						gui->temp_picon->get_w(), 
// 						gui->temp_picon->get_h(),
// 						0,       /* Dimensions to project on output frame */
// 						0, 
// 						pixmap_w, 
// 						pixmap_h,
// 						BC_RGB888, 
// 						BC_RGB888,
// 						0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
// 						0,       /* For planar use the luma rowspan */
// 						0);     /* For planar use the luma rowspan */
// 
// 					if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
// 
// 				}
// 				else
// 				{
// 					gui->lock_window("AssetPicon::create_objects 2");
// 					icon = gui->video_icon;
// 					icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_FILM];
// 				}
// 			}
//			else
			{
				icon = gui->video_icon;
				icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_FILM];			
			}
		}
		else
		if(asset->audio_data)
		{
			icon = gui->audio_icon;
			icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_SOUND];
		}
//printf("AssetPicon::create_objects 2\n");

		set_icon(icon);
		set_icon_vframe(icon_vframe);
		if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
	}
	else
	if(indexable && !indexable->is_asset)
	{
		set_icon(gui->video_icon);
		set_icon_vframe(BC_WindowBase::get_resources()->type_to_icon[ICON_FILM]);
		
	}
	else
	if(edl)
	{
//printf("AssetPicon::create_objects 4 %s\n", edl->local_session->clip_title);
		strcpy(name, edl->local_session->clip_title);
		set_text(name);
		set_icon(gui->clip_icon);
		set_icon_vframe(mwindow->theme->get_image("clip_icon"));
	}
	else
	if(plugin)
	{
		strcpy(name, _(plugin->title));
		set_text(name);
		if(plugin->picon)
		{
			if(plugin->picon->get_color_model() == BC_RGB888)
			{
				icon = new BC_Pixmap(gui, 
					plugin->picon->get_w(), 
					plugin->picon->get_h());
				icon->draw_vframe(plugin->picon,
					0,
					0,
					plugin->picon->get_w(), 
					plugin->picon->get_h(),
					0,
					0);
			}
			else
			{
				icon = new BC_Pixmap(gui, 
					plugin->picon, 
					PIXMAP_ALPHA);
			}
			icon_vframe = new VFrame(*plugin->picon);
		}
		else
		if(plugin->audio)
		{
			if(plugin->transition)
			{
				icon = gui->atransition_icon;
				icon_vframe = mwindow->theme->get_image("atransition_icon");
			}
			else
			{
				icon = gui->aeffect_icon;
				icon_vframe = mwindow->theme->get_image("aeffect_icon");
			}
		}
		else
		if(plugin->video)
		{
			if(plugin->transition)
			{
				icon = gui->vtransition_icon;
				icon_vframe = mwindow->theme->get_image("vtransition_icon");
			}
			else
			{
				icon = gui->veffect_icon;
				icon_vframe = mwindow->theme->get_image("veffect_icon");
			}
		}


		if(!icon)
		{		
			icon = gui->file_icon;
			icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_UNKNOWN];
		}

		set_icon(icon);
		set_icon_vframe(icon_vframe);
	}

	if(debug) printf("AssetPicon::create_objects %d\n", __LINE__);
}






AWindowGUI::AWindowGUI(MWindow *mwindow, AWindow *awindow)
 : BC_Window(PROGRAM_NAME ": Resources",
 	mwindow->session->awindow_x, 
    mwindow->session->awindow_y, 
    mwindow->session->awindow_w, 
    mwindow->session->awindow_h,
    100,
    100,
    1,
    1,
    1)
{
// printf("AWindowGUI::AWindowGUI %d %d %d %d\n",
// mwindow->session->awindow_x, 
// mwindow->session->awindow_y, 
// mwindow->session->awindow_w, 
// mwindow->session->awindow_h);
	this->mwindow = mwindow;
	this->awindow = awindow;
	temp_picon = 0;
}

AWindowGUI::~AWindowGUI()
{
	assets.remove_all_objects();
	folders.remove_all_objects();
	aeffects.remove_all_objects();
	veffects.remove_all_objects();
	atransitions.remove_all_objects();
	vtransitions.remove_all_objects();
	displayed_assets[1].remove_all_objects();
	delete file_icon;
	delete audio_icon;
	delete folder_icon;
	delete clip_icon;
	delete newfolder_thread;
	delete asset_menu;
	delete assetlist_menu;
//	delete folderlist_menu;
	if(temp_picon) delete temp_picon;
}

void AWindowGUI::create_objects()
{
	int x, y;
	AssetPicon *picon;

	lock_window("AWindowGUI::create_objects");
SET_TRACE
//printf("AWindowGUI::create_objects 1\n");
	asset_titles[0] = _("Title");
	asset_titles[1] = _("Comments");

SET_TRACE

	set_icon(mwindow->theme->get_image("awindow_icon"));
	file_icon = new BC_Pixmap(this, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_UNKNOWN],
		PIXMAP_ALPHA);

	folder_icon = new BC_Pixmap(this, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_FOLDER],
		PIXMAP_ALPHA);

	audio_icon = new BC_Pixmap(this, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_SOUND],
		PIXMAP_ALPHA);

	video_icon = new BC_Pixmap(this, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_FILM],
		PIXMAP_ALPHA);

SET_TRACE

	clip_icon = new BC_Pixmap(this, 
		mwindow->theme->get_image("clip_icon"),
		PIXMAP_ALPHA);
	atransition_icon = new BC_Pixmap(this, 
		mwindow->theme->get_image("atransition_icon"),
		PIXMAP_ALPHA);
	vtransition_icon = new BC_Pixmap(this, 
		mwindow->theme->get_image("vtransition_icon"),
		PIXMAP_ALPHA);
	aeffect_icon = new BC_Pixmap(this, 
		mwindow->theme->get_image("aeffect_icon"),
		PIXMAP_ALPHA);
	veffect_icon = new BC_Pixmap(this, 
		mwindow->theme->get_image("veffect_icon"),
		PIXMAP_ALPHA);

SET_TRACE

// Mandatory folders
	folders.append(picon = new AssetPicon(mwindow,
		this,
		AEFFECT_FOLDER));
	picon->persistent = 1;
	folders.append(picon = new AssetPicon(mwindow,
		this,
		VEFFECT_FOLDER));
	picon->persistent = 1;
	folders.append(picon = new AssetPicon(mwindow,
		this,
		ATRANSITION_FOLDER));
	picon->persistent = 1;
	folders.append(picon = new AssetPicon(mwindow,
		this,
		VTRANSITION_FOLDER));
	picon->persistent = 1;

SET_TRACE

	create_persistent_folder(&aeffects, 1, 0, 1, 0);
	create_persistent_folder(&veffects, 0, 1, 1, 0);
	create_persistent_folder(&atransitions, 1, 0, 0, 1);
	create_persistent_folder(&vtransitions, 0, 1, 0, 1);

SET_TRACE

	mwindow->theme->get_awindow_sizes(this);

SET_TRACE
	add_subwindow(asset_list = new AWindowAssets(mwindow,
		this,
 		mwindow->theme->alist_x, 
    	mwindow->theme->alist_y, 
    	mwindow->theme->alist_w, 
    	mwindow->theme->alist_h));

SET_TRACE
	add_subwindow(divider = new AWindowDivider(mwindow,
		this,
		mwindow->theme->adivider_x,
		mwindow->theme->adivider_y,
		mwindow->theme->adivider_w,
		mwindow->theme->adivider_h));

SET_TRACE
	divider->set_cursor(HSEPARATE_CURSOR, 0, 0);

SET_TRACE
	add_subwindow(folder_list = new AWindowFolders(mwindow,
		this,
 		mwindow->theme->afolders_x, 
    	mwindow->theme->afolders_y, 
    	mwindow->theme->afolders_w, 
    	mwindow->theme->afolders_h));
	
SET_TRACE

	x = mwindow->theme->abuttons_x;
	y = mwindow->theme->abuttons_y;

SET_TRACE

	newfolder_thread = new NewFolderThread(mwindow, this);

	add_subwindow(asset_menu = new AssetPopup(mwindow, this));
	asset_menu->create_objects();

SET_TRACE

	add_subwindow(assetlist_menu = new AssetListMenu(mwindow, this));

SET_TRACE
	assetlist_menu->create_objects();

SET_TRACE

//	add_subwindow(folderlist_menu = new FolderListMenu(mwindow, this));
//	folderlist_menu->create_objects();
//printf("AWindowGUI::create_objects 2\n");

SET_TRACE
	unlock_window();
}

int AWindowGUI::resize_event(int w, int h)
{
	mwindow->session->awindow_x = get_x();
	mwindow->session->awindow_y = get_y();
	mwindow->session->awindow_w = w;
	mwindow->session->awindow_h = h;

	mwindow->theme->get_awindow_sizes(this);
	mwindow->theme->draw_awindow_bg(this);

	asset_list->reposition_window(mwindow->theme->alist_x, 
    	mwindow->theme->alist_y, 
    	mwindow->theme->alist_w, 
    	mwindow->theme->alist_h);
	divider->reposition_window(mwindow->theme->adivider_x,
		mwindow->theme->adivider_y,
		mwindow->theme->adivider_w,
		mwindow->theme->adivider_h);
	folder_list->reposition_window(mwindow->theme->afolders_x, 
    	mwindow->theme->afolders_y, 
    	mwindow->theme->afolders_w, 
    	mwindow->theme->afolders_h);
	
	int x = mwindow->theme->abuttons_x;
	int y = mwindow->theme->abuttons_y;

// 	new_bin->reposition_window(x, y);
// 	x += new_bin->get_w();
// 	delete_bin->reposition_window(x, y);
// 	x += delete_bin->get_w();
// 	rename_bin->reposition_window(x, y);
// 	x += rename_bin->get_w();
// 	delete_disk->reposition_window(x, y);
// 	x += delete_disk->get_w();
// 	delete_project->reposition_window(x, y);
// 	x += delete_project->get_w();
// 	info->reposition_window(x, y);
// 	x += info->get_w();
// 	redraw_index->reposition_window(x, y);
// 	x += redraw_index->get_w();
// 	paste->reposition_window(x, y);
// 	x += paste->get_w();
// 	append->reposition_window(x, y);
// 	x += append->get_w();
// 	view->reposition_window(x, y);

	BC_WindowBase::resize_event(w, h);
	return 1;
}

int AWindowGUI::translation_event()
{
	mwindow->session->awindow_x = get_x();
	mwindow->session->awindow_y = get_y();
	return 0;
}

void AWindowGUI::reposition_objects()
{
	mwindow->theme->get_awindow_sizes(this);
	asset_list->reposition_window(mwindow->theme->alist_x, 
    	mwindow->theme->alist_y, 
    	mwindow->theme->alist_w, 
    	mwindow->theme->alist_h);
	divider->reposition_window(mwindow->theme->adivider_x,
		mwindow->theme->adivider_y,
		mwindow->theme->adivider_w,
		mwindow->theme->adivider_h);
	folder_list->reposition_window(mwindow->theme->afolders_x, 
    	mwindow->theme->afolders_y, 
    	mwindow->theme->afolders_w, 
    	mwindow->theme->afolders_h);
	flush();
}

int AWindowGUI::close_event()
{
	hide_window();
	mwindow->session->show_awindow = 0;
	unlock_window();

	mwindow->gui->lock_window("AWindowGUI::close_event");
	mwindow->gui->mainmenu->show_awindow->set_checked(0);
	mwindow->gui->unlock_window();

	lock_window("AWindowGUI::close_event");
	mwindow->save_defaults();
	return 1;
}


int AWindowGUI::keypress_event()
{
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

void AWindowGUI::update_folder_list()
{
//printf("AWindowGUI::update_folder_list 1\n");
	for(int i = 0; i < folders.total; i++)
	{
		AssetPicon *picon = (AssetPicon*)folders.values[i];
		picon->in_use--;
	}
//printf("AWindowGUI::update_folder_list 1\n");

// Search assets for folders
	for(int i = 0; i < mwindow->edl->folders.total; i++)
	{
		char *folder = mwindow->edl->folders.values[i];
		int exists = 0;
//printf("AWindowGUI::update_folder_list 1.1\n");

		for(int j = 0; j < folders.total; j++)
		{
			AssetPicon *picon = (AssetPicon*)folders.values[j];
			if(!strcasecmp(picon->get_text(), folder))
			{
				exists = 1;
				picon->in_use = 1;
				break;
			}
		}

		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, this, folder);
			picon->create_objects();
			folders.append(picon);
		}
//printf("AWindowGUI::update_folder_list 1.3\n");
	}
//printf("AWindowGUI::update_folder_list 1\n");
//for(int i = 0; i < folders.total; i++)
//	printf("AWindowGUI::update_folder_list %s\n", folders.values[i]->get_text());

// Delete excess
	for(int i = folders.total - 1; i >= 0; i--)
	{
		AssetPicon *picon = (AssetPicon*)folders.values[i];
		if(!picon->in_use && !picon->persistent)
		{
			delete picon;
			folders.remove_number(i);
		}
	}
//for(int i = 0; i < folders.total; i++)
//	printf("AWindowGUI::update_folder_list %s\n", folders.values[i]->get_text());
//printf("AWindowGUI::update_folder_list 2\n");
}

void AWindowGUI::create_persistent_folder(ArrayList<BC_ListBoxItem*> *output, 
	int do_audio, 
	int do_video, 
	int is_realtime, 
	int is_transition)
{
	ArrayList<PluginServer*> plugin_list;

// Get pointers to plugindb entries
	mwindow->search_plugindb(do_audio, 
			do_video, 
			is_realtime, 
			is_transition,
			0,
			plugin_list);

	for(int i = 0; i < plugin_list.total; i++)
	{
		PluginServer *server = plugin_list.values[i];
		int exists = 0;

// Create new listitem
		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, this, server);
			picon->create_objects();
			output->append(picon);
		}
	}
}

void AWindowGUI::update_asset_list()
{
    char name[BCTEXTLEN];
    FileSystem fs;

//printf("AWindowGUI::update_asset_list 1\n");
	for(int i = 0; i < assets.total; i++)
	{
		AssetPicon *picon = (AssetPicon*)assets.values[i];
		picon->in_use--;
	}





//printf("AWindowGUI::update_asset_list 2\n");


// Synchronize EDL clips
	for(int i = 0; i < mwindow->edl->clips.total; i++)
	{
		int exists = 0;
		
// Look for clip in existing listitems
		for(int j = 0; j < assets.total && !exists; j++)
		{
			AssetPicon *picon = (AssetPicon*)assets.values[j];
			
			if(picon->id == mwindow->edl->clips.values[i]->id)
			{
				picon->edl = mwindow->edl->clips.values[i];
				picon->set_text(mwindow->edl->clips.values[i]->local_session->clip_title);
				exists = 1;
				picon->in_use = 1;
			}
		}

// Create new listitem
		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, 
				this, 
				mwindow->edl->clips.values[i]);
			picon->create_objects();
			assets.append(picon);
		}
	}





//printf("AWindowGUI::update_asset_list %d\n", __LINE__);


// Synchronize EDL assets
	for(Asset *current = mwindow->edl->assets->first; 
		current; 
		current = NEXT)
	{
		int exists = 0;

// Look for asset in existing listitems
		for(int j = 0; j < assets.total && !exists; j++)
		{
			AssetPicon *picon = (AssetPicon*)assets.values[j];

			if(picon->id == current->id)
			{
// update the path
                fs.extract_name(name, current->path);
                picon->set_text(name);
				picon->indexable = current;
				exists = 1;
				picon->in_use = 1;
				break;
			}
		}

// Create new listitem
		if(!exists)
		{
//printf("AWindowGUI::update_asset_list %d\n", __LINE__);
			AssetPicon *picon = new AssetPicon(mwindow, this, current);
//printf("AWindowGUI::update_asset_list %d\n", __LINE__);
			picon->create_objects();
//printf("AWindowGUI::update_asset_list %d\n", __LINE__);
			assets.append(picon);
		}
	}



//printf("AWindowGUI::update_asset_list %d nested edls=%d\n", __LINE__, mwindow->edl->nested_edls->size());


// Synchronize nested EDLs
	for(int i = 0; i < mwindow->edl->nested_edls->size(); i++)
	{
		int exists = 0;
		Indexable *indexable = mwindow->edl->nested_edls->get(i);

// Look for asset in existing listitems
		for(int j = 0; j < assets.total && !exists; j++)
		{
			AssetPicon *picon = (AssetPicon*)assets.values[j];

			if(picon->id == indexable->id)
			{
// update the path
                fs.extract_name(name, indexable->path);
                picon->set_text(name);
				picon->indexable = indexable;
				exists = 1;
				picon->in_use = 1;
				break;
			}
		}

// Create new listitem
		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, 
				this, 
				indexable);
			picon->create_objects();
			assets.append(picon);
		}
	}









//printf("AWindowGUI::update_asset_list %d\n", __LINE__);
	for(int i = assets.size() - 1; i >= 0; i--)
	{
		AssetPicon *picon = (AssetPicon*)assets.get(i);
//printf("AWindowGUI::update_asset_list %s %d\n", picon->asset->path, picon->in_use);
		if(!picon->in_use)
		{
			delete picon;
			assets.remove_number(i);
		}
	}
//printf("AWindowGUI::update_asset_list 7 %d\n", assets.total);
}





void AWindowGUI::sort_assets()
{
//printf("AWindowGUI::sort_assets 1 %s\n", mwindow->edl->session->current_folder);
	if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER))
		sort_picons(&aeffects, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER))
		sort_picons(&veffects, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER))
		sort_picons(&atransitions, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		sort_picons(&vtransitions, 
			0);
	else
		sort_picons(&assets, 
			mwindow->edl->session->current_folder);

	update_assets();
}











void AWindowGUI::collect_assets()
{
	int i = 0;
	mwindow->session->drag_assets->remove_all();
	mwindow->session->drag_clips->remove_all();
	while(1)
	{
		AssetPicon *result = (AssetPicon*)asset_list->get_selection(0, i++);
		if(!result) break;

		if(result->indexable) mwindow->session->drag_assets->append(result->indexable);
		if(result->edl) mwindow->session->drag_clips->append(result->edl);
	}
}

void AWindowGUI::copy_picons(ArrayList<BC_ListBoxItem*> *dst, 
	ArrayList<BC_ListBoxItem*> *src, 
	char *folder)
{
// Remove current pointers
	dst[0].remove_all();
	dst[1].remove_all_objects();

// Create new pointers
//if(folder) printf("AWindowGUI::copy_picons 1 %s\n", folder);
	for(int i = 0; i < src->total; i++)
	{
		AssetPicon *picon = (AssetPicon*)src->values[i];
//printf("AWindowGUI::copy_picons 2 %s\n", picon->asset->folder);
		if(!folder ||
			(folder && picon->indexable && !strcasecmp(picon->indexable->folder, folder)) ||
			(folder && picon->edl && !strcasecmp(picon->edl->local_session->folder, folder)))
		{
			BC_ListBoxItem *item2, *item1;
			dst[0].append(item1 = picon);
			if(picon->edl)
				dst[1].append(item2 = new BC_ListBoxItem(picon->edl->local_session->clip_notes));
			else
				dst[1].append(item2 = new BC_ListBoxItem(""));
			item1->set_autoplace_text(1);
			item2->set_autoplace_text(1);
//printf("AWindowGUI::copy_picons 3 %s\n", picon->get_text());
		}
	}
}

void AWindowGUI::sort_picons(ArrayList<BC_ListBoxItem*> *src, 
		char *folder)
{
//printf("AWindowGUI::sort_picons 1\n")
	int done = 0;
	while(!done)
	{
		done = 1;
		for(int i = 0; i < src->size() - 1; i++)
		{
			BC_ListBoxItem *item1 = src->get(i);
			BC_ListBoxItem *item2 = src->get(i + 1);
			item1->set_autoplace_icon(1);
			item2->set_autoplace_icon(1);
			item1->set_autoplace_text(1);
			item2->set_autoplace_text(1);
			if(strcmp(item1->get_text(), item2->get_text()) > 0)
			{
				src->set(i + 1, item1);
				src->set(i, item2);
				done = 0;
			}
		}
	}
}


void AWindowGUI::filter_displayed_assets()
{
	if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER))
		copy_picons(displayed_assets, 
			&aeffects, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER))
		copy_picons(displayed_assets, 
			&veffects, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER))
		copy_picons(displayed_assets, 
			&atransitions, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		copy_picons(displayed_assets, 
			&vtransitions, 
			0);
	else
		copy_picons(displayed_assets, 
			&assets, 
			mwindow->edl->session->current_folder);
}


void AWindowGUI::update_assets()
{
//PRINT_TRACE
	update_folder_list();
//PRINT_TRACE
	update_asset_list();
//PRINT_TRACE
	filter_displayed_assets();
//PRINT_TRACE

//for(int i = 0; i < folders.total; i++)
//printf("AWindowGUI::update_assets 4\n");
//	printf("AWindowGUI::update_assets %s\n", folders.values[i]->get_text());
	if(mwindow->edl->session->folderlist_format != folder_list->get_format())
		folder_list->update_format(mwindow->edl->session->folderlist_format, 0);
//PRINT_TRACE
	folder_list->update(&folders,
		0,
		0,
		1,
		folder_list->get_xposition(),
		folder_list->get_yposition(),
		-1);
//PRINT_TRACE

	if(mwindow->edl->session->assetlist_format != asset_list->get_format())
		asset_list->update_format(mwindow->edl->session->assetlist_format, 0);
//PRINT_TRACE


	asset_list->update(displayed_assets,
		asset_titles,
		mwindow->session->asset_columns,
		ASSET_COLUMNS, 
		asset_list->get_xposition(),
		asset_list->get_yposition(),
		-1,
		0);
//PRINT_TRACE

	flush();
	return;
}

int AWindowGUI::current_folder_number()
{
	int result = -1;
	for(int i = 0; i < folders.total; i++)
	{
		if(!strcasecmp(folders.values[i]->get_text(), mwindow->edl->session->current_folder))
		{
			result = i;
			break;
		}
	}
	return result;
}

int AWindowGUI::drag_motion()
{
	if(get_hidden()) return 0;

	int result = 0;
	return result;
}

int AWindowGUI::drag_stop()
{
	if(get_hidden()) return 0;

	return 0;
}

Indexable* AWindowGUI::selected_asset()
{
	AssetPicon *picon = (AssetPicon*)asset_list->get_selection(0, 0);
	if(picon) return picon->indexable;
    return 0;
}

PluginServer* AWindowGUI::selected_plugin()
{
	AssetPicon *picon = (AssetPicon*)asset_list->get_selection(0, 0);
	if(picon) return picon->plugin;
    return 0;
}

AssetPicon* AWindowGUI::selected_folder()
{
	AssetPicon *picon = (AssetPicon*)folder_list->get_selection(0, 0);
    return picon;
}










AWindowDivider::AWindowDivider(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
AWindowDivider::~AWindowDivider()
{
}

int AWindowDivider::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		mwindow->session->current_operation = DRAG_PARTITION;
		return 1;
	}
	return 0;
}

int AWindowDivider::cursor_motion_event()
{
	if(mwindow->session->current_operation == DRAG_PARTITION)
	{
		mwindow->session->afolders_w = gui->get_relative_cursor_x();
		gui->reposition_objects();
	}
	return 0;
}

int AWindowDivider::button_release_event()
{
	if(mwindow->session->current_operation == DRAG_PARTITION)
	{
		mwindow->session->current_operation = NO_OPERATION;
		return 1;
	}
	return 0;
}






AWindowFolders::AWindowFolders(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_ListBox(x, 
 		y, 
		w, 
		h,
		mwindow->edl->session->folderlist_format == FOLDERS_ICONS ? 
			LISTBOX_ICONS : LISTBOX_TEXT, 
		&gui->folders, // Each column has an ArrayList of BC_ListBoxItems.
		0,             // Titles for columns.  Set to 0 for no titles
		0,                // width of each column
		1,                      // Total columns.
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_SINGLE,  // Select one item or multiple items
		ICON_TOP,        // Position of icon relative to text of each item
		1)               // Allow drags
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_drag_scroll(0);
}

AWindowFolders::~AWindowFolders()
{
}
	
int AWindowFolders::selection_changed()
{
	AssetPicon *picon = (AssetPicon*)get_selection(0, 0);
	if(picon)
	{
//		if(get_button_down() && get_buttonpress() == 3)
//		{
//			gui->folderlist_menu->update_titles();
//			gui->folderlist_menu->activate_menu();
//		}

		strcpy(mwindow->edl->session->current_folder, picon->get_text());
//printf("AWindowFolders::selection_changed 1\n");
		gui->asset_list->draw_background();
		gui->update_assets();
	}
	return 1;
}

int AWindowFolders::button_press_event()
{
	int result = 0;

	result = BC_ListBox::button_press_event();

	if(!result)
	{
//		if(get_buttonpress() == 3 && is_event_win() && cursor_inside())
//		{
//			gui->folderlist_menu->update_titles();
//			gui->folderlist_menu->activate_menu();
//			result = 1;
//		}
	}


	return result;
}







AWindowAssets::AWindowAssets(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_ListBox(x, 
 		y, 
		w, 
		h,
		mwindow->edl->session->assetlist_format == ASSETS_ICONS ? 
			LISTBOX_ICONS : LISTBOX_TEXT,
		&gui->assets,  	  // Each column has an ArrayList of BC_ListBoxItems.
		gui->asset_titles,             // Titles for columns.  Set to 0 for no titles
		mwindow->session->asset_columns,                // width of each column
		1,                      // Total columns.
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_MULTIPLE,  // Select one item or multiple items
		ICON_TOP,        // Position of icon relative to text of each item
		1)               // Allow drag
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_drag_scroll(0);
}

AWindowAssets::~AWindowAssets()
{
}

int AWindowAssets::button_press_event()
{
	int result = 0;

	result = BC_ListBox::button_press_event();

	if(!result && get_buttonpress() == 3 && is_event_win() && cursor_inside())
	{
		BC_ListBox::deactivate_selection();
		gui->assetlist_menu->update_titles();
		gui->assetlist_menu->activate_menu();
		result = 1;
	}


	return result;
}


int AWindowAssets::handle_event()
{
//printf("AWindowAssets::handle_event 1 %d %d\n", get_buttonpress(), get_selection(0, 0));
	if(get_selection(0, 0))
	{
		if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER))
		{
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER))
		{
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER))
		{
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		{
		}
		else
		if(mwindow->vwindows.size())
		{
//printf("AWindowAssets::handle_event 2 %d %d\n", get_buttonpress(), get_selection(0, 0));
			mwindow->vwindows.get(DEFAULT_VWINDOW)->gui->lock_window("AWindowAssets::handle_event");
			
			if(((AssetPicon*)get_selection(0, 0))->indexable)
				mwindow->vwindows.get(DEFAULT_VWINDOW)->change_source(((AssetPicon*)get_selection(0, 0))->indexable);
			else
			if(((AssetPicon*)get_selection(0, 0))->edl)
				mwindow->vwindows.get(DEFAULT_VWINDOW)->change_source(((AssetPicon*)get_selection(0, 0))->edl);

			mwindow->vwindows.get(DEFAULT_VWINDOW)->gui->unlock_window();
		}
		return 1;
	}

	return 0;
}

int AWindowAssets::selection_changed()
{
// Show popup window
	if(get_button_down() && get_buttonpress() == 3 && get_selection(0, 0))
	{
		if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER) || 
			!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER) ||
			!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER) ||
			!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		{
			gui->assetlist_menu->update_titles();
			gui->assetlist_menu->activate_menu();
		}
		else
		{
			if(((AssetPicon*)get_selection(0, 0))->indexable)
				gui->asset_menu->update();
			else
			if(((AssetPicon*)get_selection(0, 0))->edl)
				gui->asset_menu->update();



			gui->asset_menu->activate_menu();
		}

		BC_ListBox::deactivate_selection();
		return 1;
	}
	return 0;
}

void AWindowAssets::draw_background()
{
	BC_ListBox::draw_background();
	set_color(RED);
	set_font(LARGEFONT);
	draw_text(get_w() - 
			get_text_width(LARGEFONT, mwindow->edl->session->current_folder) - 4, 
		30, 
		mwindow->edl->session->current_folder, 
		-1, 
		get_bg_surface());
}

int AWindowAssets::drag_start_event()
{
	int collect_pluginservers = 0;
	int collect_assets = 0;

	if(BC_ListBox::drag_start_event())
	{
		if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER))
		{
			mwindow->session->current_operation = DRAG_AEFFECT;
			collect_pluginservers = 1;
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER))
		{
			mwindow->session->current_operation = DRAG_VEFFECT;
			collect_pluginservers = 1;
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER))
		{
			mwindow->session->current_operation = DRAG_ATRANSITION;
			collect_pluginservers = 1;
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		{
			mwindow->session->current_operation = DRAG_VTRANSITION;
			collect_pluginservers = 1;
		}
		else
		{
			mwindow->session->current_operation = DRAG_ASSET;
			collect_assets = 1;
		}

        if(mwindow->session->current_operation == DRAG_ASSET ||
            mwindow->session->current_operation == DRAG_AEFFECT ||
            mwindow->session->current_operation == DRAG_VEFFECT)
        {
            mwindow->session->free_drag = ctrl_down();
            mwindow->session->drag_diff_x = 0;
        }

		if(collect_pluginservers)
		{
			int i = 0;
			mwindow->session->drag_pluginservers->remove_all();
			while(1)
			{
				AssetPicon *result = (AssetPicon*)get_selection(0, i++);
				if(!result) break;
				
				mwindow->session->drag_pluginservers->append(result->plugin);
			}
		}
		
		if(collect_assets)
		{
			gui->collect_assets();
		}

		return 1;
	}
	return 0;
}

int AWindowAssets::drag_motion_event()
{
	BC_ListBox::drag_motion_event();
	unlock_window();

	mwindow->gui->lock_window("AWindowAssets::drag_motion_event");
	mwindow->gui->drag_motion(ctrl_down());
	mwindow->gui->unlock_window();

	for(int i = 0; i < mwindow->vwindows.size(); i++)
	{
		VWindow *vwindow = mwindow->vwindows.get(i);
		vwindow->gui->lock_window("AWindowAssets::drag_motion_event");
		vwindow->gui->drag_motion();
		vwindow->gui->unlock_window();
	}

	mwindow->cwindow->gui->lock_window("AWindowAssets::drag_motion_event");
	mwindow->cwindow->gui->drag_motion();
	mwindow->cwindow->gui->unlock_window();

	lock_window("AWindowAssets::drag_motion_event");
	return 0;
}

int AWindowAssets::drag_stop_event()
{
	int result = 0;

	result = gui->drag_stop();

	unlock_window();

	if(!result)
	{
		mwindow->gui->lock_window("AWindowAssets::drag_stop_event");
		result = mwindow->gui->drag_stop();
		mwindow->gui->unlock_window();
	}

	if(!result) 
	{
		for(int i = 0; i < mwindow->vwindows.size(); i++)
		{
			VWindow *vwindow = mwindow->vwindows.get(i);
			vwindow->gui->lock_window("AWindowAssets::drag_stop_event");
			result = vwindow->gui->drag_stop();
			vwindow->gui->unlock_window();
		}
	}

	if(!result) 
	{
		mwindow->cwindow->gui->lock_window("AWindowAssets::drag_stop_event");
		result = mwindow->cwindow->gui->drag_stop();
		mwindow->cwindow->gui->unlock_window();
	}

	lock_window("AWindowAssets::drag_stop_event");

	if(result) get_drag_popup()->set_animation(0);

	BC_ListBox::drag_stop_event();
	mwindow->session->current_operation = ::NO_OPERATION; // since NO_OPERATION is also defined in listbox, we have to reach for global scope...
	return 0;
}

int AWindowAssets::column_resize_event()
{
	mwindow->session->asset_columns[0] = get_column_width(0);
	mwindow->session->asset_columns[1] = get_column_width(1);
	return 1;
}












AWindowNewFolder::AWindowNewFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->newbin_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("New bin"));
}

int AWindowNewFolder::handle_event()
{
	gui->newfolder_thread->start_new_folder();
	return 1;
}

AWindowDeleteFolder::AWindowDeleteFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->deletebin_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete bin"));
}

int AWindowDeleteFolder::handle_event()
{
	if(gui->folder_list->get_selection(0, 0))
	{
		BC_ListBoxItem *folder = gui->folder_list->get_selection(0, 0);
		mwindow->delete_folder(folder->get_text());
	}
	return 1;
}

AWindowRenameFolder::AWindowRenameFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->renamebin_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Rename bin"));
}

int AWindowRenameFolder::handle_event()
{
	return 1;
}

AWindowDeleteDisk::AWindowDeleteDisk(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->deletedisk_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete asset from disk"));
}

int AWindowDeleteDisk::handle_event()
{
	return 1;
}

AWindowDeleteProject::AWindowDeleteProject(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->deleteproject_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete asset from project"));
}

int AWindowDeleteProject::handle_event()
{
	return 1;
}

// AWindowInfo::AWindowInfo(MWindow *mwindow, AWindowGUI *gui, int x, int y)
//  : BC_Button(x, y, mwindow->theme->infoasset_data)
// {
// 	this->mwindow = mwindow;
// 	this->gui = gui;
// 	set_tooltip(_("Edit information on asset"));
// }
// 
// int AWindowInfo::handle_event()
// {
// 	gui->awindow->asset_edit->edit_asset(gui->selected_asset());
// 	return 1;
// }

AWindowRedrawIndex::AWindowRedrawIndex(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->redrawindex_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Redraw index"));
}

int AWindowRedrawIndex::handle_event()
{
	return 1;
}

AWindowPaste::AWindowPaste(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->pasteasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Paste asset on recordable tracks"));
}

int AWindowPaste::handle_event()
{
	return 1;
}

AWindowAppend::AWindowAppend(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->appendasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Append asset in new tracks"));
}

int AWindowAppend::handle_event()
{
	return 1;
}

AWindowView::AWindowView(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->viewasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("View asset"));
}

int AWindowView::handle_event()
{
	return 1;
}
