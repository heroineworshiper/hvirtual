/*
 * CINELERRA
 * Copyright (C) 1997-2024 Adam Williams <broadcast at earthling dot net>
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
#include "assets.h"
#include "bcsignals.h"
#include "clip.h"
#include "clipedit.h"
#include "bchash.h"
#include "edl.h"
#include "edlfactory.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "indexable.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "tracks.h"
#include "transportque.h"
#include "vplayback.h"
#include "vtimebar.h"
#include "vwindow.h"
#include "vwindowgui.h"

// Use the asset format.
// If the framerates or samplerates don't match the project, 
// it won't paste synchronized media.
//#define STANDALONE_EDL

VWindow::VWindow(MWindow *mwindow) : BC_DialogThread()
{
	this->mwindow = mwindow;
// can't close the opengl objects
    set_keep_gui(1);
	indexable = 0;
	edl = 0;
    is_default = 0;
}


VWindow::~VWindow()
{
//printf("VWindow::~VWindow 1\n");
	delete playback_engine;
//printf("VWindow::~VWindow 1\n");
	delete_source(1, 0);
	delete clip_edit;
//printf("VWindow::~VWindow 2\n");
}

void VWindow::delete_source(int do_main_edl, int update_gui)
{
// printf("VWindow::delete_source %d %d %p %p\n", 
// __LINE__, 
// gui->get_window_lock(), 
// edl, 
// indexable);
//	if(do_main_edl) mwindow->edl->remove_vwindow_edl(get_edl());

	if(edl)
	{
		edl->Garbage::remove_user();
//printf("VWindow::delete_source %d\n", __LINE__);
		edl = 0;
	}

// 	if(mwindow->edl->vwindow_edl && !mwindow->edl->vwindow_edl_shared)
// 	{
// 		mwindow->edl->vwindow_edl->Garbage::remove_user();
// 		mwindow->edl->vwindow_edl = 0;
//		mwindow->edl->vwindow_edl_shared = 0;
// 	}

//printf("VWindow::delete_source %d\n", __LINE__);
	if(indexable) indexable->Garbage::remove_user();
	indexable = 0;

	if(update_gui) gui->change_source(0, _("Viewer"));
//printf("VWindow::delete_source %d\n", __LINE__);
}


void VWindow::load_defaults()
{
}

void VWindow::create_objects()
{
}


void VWindow::handle_done_event(int result)
{
//	delete playback_engine;
//	delete playback_cursor;
//	delete clip_edit;
//	playback_engine = 0;
//	playback_cursor = 0;
//	clip_edit = 0;


    playback_engine->interrupt_playback(1);

	int total = 0;
	for(int i = 0; i < mwindow->vwindows.size(); i++)
	{

//printf("VWindow::handle_done_event %d %d\n", __LINE__, mwindow->vwindows.get(i)->is_running());
		if(mwindow->vwindows.get(i)->get_edl()) total++;
	}
// subtract ourselves
	total--;

// Want the last vwindow open to be the one opened by window->viewer 
// & to show the source still open but want other vwindows to close their source
    if(total)
	{
        delete_source(1, 0);
printf("VWindow::handle_done_event %d\n", __LINE__);
        gui->canvas->clear();
printf("VWindow::handle_done_event %d\n", __LINE__);
    }

// Update the menu if no viewers visible
	if(!total)
	{
 		mwindow->gui->lock_window("VWindowGUI::close_event");
 		mwindow->gui->mainmenu->show_vwindow->set_checked(0);
 		mwindow->gui->unlock_window();

		mwindow->session->show_vwindow = 0;
 		mwindow->save_defaults();
	}
}



BC_Window* VWindow::new_gui()
{
	gui = new VWindowGUI(mwindow, this);
	gui->create_objects();




	playback_engine = new VPlayback(mwindow, this, gui->canvas);

// Start command loop
	playback_engine->create_objects();
	gui->transport->set_engine(playback_engine);

	clip_edit = new ClipEdit(mwindow, 0, this);
	return gui;
}


EDL* VWindow::get_edl()
{
//	return mwindow->edl->vwindow_edl;
	return edl;
}

Indexable* VWindow::get_source()
{
	return this->indexable;
}

void VWindow::change_source(int edl_number)
{
	if(!running()) return;

// 	if(mwindow->edl->get_vwindow_edl(edl_number))
// 	{
// 		this->edl = mwindow->edl->get_vwindow_edl(edl_number);
// 		this->edl->Garbage::add_user();
// 		gui->change_source(get_edl(), get_edl()->local_session->clip_title);
// 		update_position(CHANGE_ALL, 1, 1, 1);
// 	}
// 	else
	{
		delete_source(1, 1);
//		if(indexable) indexable->Garbage::remove_user();
//		indexable = 0;
//		mwindow->edl->vwindow_edl_shared = 0;
	}
}

void VWindow::change_source(Indexable *indexable)
{
//printf("VWindow::change_source %d running=%d\n", __LINE__, running());
	if(!running()) return;

//printf("VWindow::change_source %d\n", __LINE__);
// 	if(asset && this->asset &&
// 		asset->id == this->asset->id &&
// 		asset == this->asset) return;

//printf("VWindow::change_source %d\n", __LINE__);

	char title[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(title, indexable->path);
//printf("VWindow::change_source 1\n");

	delete_source(1, 0);
//printf("VWindow::change_source 1\n");

// Generate EDL off of main EDL for cutting
	Asset *asset = 0;
	EDL *nested_edl = 0;
	if(indexable->is_asset)
	{
		this->indexable = asset = new Asset;
		asset->copy_from((Asset*)indexable, 0);
	}
	else
	{
		this->indexable = nested_edl = new EDL;
		nested_edl->create_objects();
		nested_edl->copy_all((EDL*)indexable);
	}

// Create EDL inside the mane EDL
#ifndef STANDALONE_EDL
	this->edl = new EDL(mwindow->edl);
#else
// Create EDL as a standalone EDL
	this->edl = new EDL;
#endif
	this->edl->create_objects();
//	mwindow->edl->append_vwindow_edl(this->edl, 1);

//	mwindow->edl->vwindow_edl = new EDL(mwindow->edl);
//	mwindow->edl->vwindow_edl_shared = 0;
//	mwindow->edl->vwindow_edl->create_objects();

	if(asset)
    {
// All viewer EDLs currently share the mane EDL's session, so they can't
// conform to the asset.
		EDLFactory::asset_to_edl(this->edl, 
            asset, 
            0, 
#ifndef STANDALONE_EDL
            0); // conform
#else
            1,  // conform
            1); // auto aspect
#endif

printf("VWindow::change_source %d aspect=%f\n", __LINE__, edl->get_aspect_ratio());
	}
    else
	{
    	mwindow->edl_to_nested(this->edl, nested_edl);
    }

// Update GUI
	gui->change_source(this->edl, title);
	update_position(CHANGE_ALL, 1, 1, 1);



//printf("VWindow::change_source 2\n");
}

void VWindow::change_source(EDL *edl)
{
	if(!running()) return;
//printf("VWindow::change_source %d %p\n", __LINE__, edl);
// EDLs are identical
//	if(edl && mwindow->edl->vwindow_edl && 
//		edl->id == mwindow->edl->vwindow_edl->id) return;
	if(edl && get_edl() && edl->id == get_edl()->id) return;

	delete_source(1, 0);

	if(edl)
	{
//		mwindow->edl->append_vwindow_edl(edl, 1);
		this->edl = edl;
		this->edl->Garbage::add_user();

//		mwindow->edl->vwindow_edl = edl;
// in order not to later delete edl if it is shared
//		edl->Garbage::add_user();
//		mwindow->edl->vwindow_edl_shared = 1;

// Update GUI
		gui->change_source(edl, edl->local_session->clip_title);
		update_position(CHANGE_ALL, 1, 1, 1);
	}
	else
		gui->change_source(edl, _("Viewer"));
}

void VWindow::change_source(char *folder, int item)
{
//printf("VWindow::change_source %d\n", __LINE__);
	int result = 0;
	if(!running()) return;
// Search EDLs
	if(!strcasecmp(folder, CLIP_FOLDER))
	{
		if(item < mwindow->edl->clips.total)
		{
			change_source(mwindow->edl->clips.values[item]);
			result = 1;
		}
	}
	else
// Search media
	if(!strcasecmp(folder, MEDIA_FOLDER))
	{
		if(item < mwindow->edl->assets->total())
		{
			change_source(mwindow->edl->assets->get_item_number(item));
			result = 1;
		}
	}
	else
// Search extra clip folders
	{
	}
	
	if(!result)
	{
		delete_source(1, 1);
	}
}




void VWindow::goto_start()
{
	if(get_edl())
	{
		get_edl()->local_session->set_selectionstart(0);
		get_edl()->local_session->set_selectionend(0);
		update_position(CHANGE_NONE, 
			0, 
			1, 
			0);
	}
}

void VWindow::goto_end()
{
	if(get_edl())
	{
		double position = get_edl()->tracks->total_length();
		get_edl()->local_session->set_selectionstart(position);
		get_edl()->local_session->set_selectionend(position);
		update_position(CHANGE_NONE, 
			0, 
			1,
			0);
	}
}

void VWindow::update(int do_timebar)
{
	if(do_timebar)
		gui->timebar->update(1);
}

void VWindow::update_position(int change_type, 
	int use_slider, 
	int update_slider,
	int lock_window)
{
//printf("VWindow::update_position %d\n", __LINE__);
	EDL *edl = get_edl();
//printf("VWindow::update_position %d\n", __LINE__);
	if(edl)
	{
//printf("VWindow::update_position %d\n", __LINE__);

#ifdef USE_SLIDER

		if(use_slider) 
		{
			edl->local_session->set_selectionstart(gui->slider->get_value());
			edl->local_session->set_selectionend(gui->slider->get_value());
		}

		if(update_slider)
		{
			gui->slider->set_position();
		}

#endif


//printf("VWindow::update_position %d %p\n", __LINE__, playback_engine);
//edl->dump();
		playback_engine->que->send_command(CURRENT_FRAME, 
            1.0, // speed
			change_type,
			edl,
			1, // realtime
            0, // resume,
            0); // use_inout

//printf("VWindow::update_position %d\n", __LINE__);
		if(lock_window) gui->lock_window("VWindow::update_position");
		gui->clock->update(edl->local_session->get_selectionstart(1));
		if(lock_window) gui->unlock_window();
//printf("VWindow::update_position %d\n", __LINE__);
	}
}





int VWindow::update_position(double position)
{
	EDL *edl = get_edl();
	if(edl)
	{
		gui->unlock_window();

		playback_engine->interrupt_playback(1);

		position = mwindow->edl->align_to_frame(position, 0);
		position = MAX(0, position);

		edl->local_session->set_selectionstart(position);
		edl->local_session->set_selectionend(position);

		gui->lock_window("VWindow::update_position 1");
		update_position(CHANGE_NONE, 0, 1, 0);
	}

	return 1;
}


void VWindow::set_inpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->set_inpoint(edl->local_session->get_selectionstart(1));
		gui->timebar->update(1);
	}
}

void VWindow::set_outpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->set_outpoint(edl->local_session->get_selectionstart(1));
		gui->timebar->update(1);
	}
}

void VWindow::clear_inpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->local_session->unset_inpoint();
		gui->timebar->update(1);
	}
}

void VWindow::clear_outpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->local_session->unset_outpoint();
		gui->timebar->update(1);
	}
}

void VWindow::copy()
{
	EDL *edl = get_edl();
	if(edl)
	{
		double start = edl->local_session->get_selectionstart();
		double end = edl->local_session->get_selectionend();
		FileXML file;
		edl->copy(start,
			end,
			0,
			0,
			0,
			&file,
			"",
			1);
		mwindow->gui->lock_window();
		mwindow->gui->get_clipboard()->to_clipboard(file.string,
			strlen(file.string),
			ALL_SELECTIONS);
		mwindow->gui->unlock_window();
	}
}

void VWindow::splice_selection()
{
}

void VWindow::overwrite_selection()
{
}



