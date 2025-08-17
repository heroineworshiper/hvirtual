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
#include "bctimer.h"
#include "edl.h"
#include "filexml.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "nestededls.h"
#include <string.h>
#include "undostack.h"

MainUndo::MainUndo(MWindow *mwindow)
{ 
	this->mwindow = mwindow;
	undo_stack = new UndoStack;
	last_update = new Timer;
}

MainUndo::~MainUndo()
{
	delete undo_stack;
	delete last_update;
}


void MainUndo::update_undo_entry(const char *description, 
	uint32_t load_flags,
	void *creator, 
	int changes_made)
{
	FileXML file;

	mwindow->edl->save_xml(&file, 
		"",
		0,
		0);
	file.terminate_string();
	if(changes_made) mwindow->session->changes_made = 1;

// Remove all entries after current and create new one
	UndoStackItem *current = undo_stack->push();

    current->set_modified(mwindow->session->changes_made);
	current->set_flags(load_flags);
	current->set_data(file.string);
	current->set_description((char*)description);
	current->set_creator(creator);
	current->set_filename(mwindow->session->filename);
// printf("MainUndo::update_undo_entry %d current=%p\n%s\n", 
// __LINE__, 
// current,
// file.string);

// Can't undo only 1 record.
	if(undo_stack->total() > 1)
	{
//		mwindow->gui->lock_window("MainUndo::update_undo");
		mwindow->gui->mainmenu->undo->update_caption(description);
		mwindow->gui->mainmenu->redo->update_caption("");
//		mwindow->gui->unlock_window();
	}

// Update timer
	last_update->update();
}

void MainUndo::update_undo_before(const char *description, void *creator)
{
//printf("MainUndo::update_undo_before %d\n", __LINE__);
	if(undo_stack->current && !(undo_stack->number_of(undo_stack->current) % 2))
	{
		printf("MainUndo::update_undo_before %d \"%s\": must be on an after entry to do this. size=%d\n",
			__LINE__,
			description,
			undo_stack->total());

// dump stack
		undo_stack->dump();

// Move up an entry to get back in sync
//		return;
	}


// Discard if creator matches previous before entry and within a time limit
	if(creator)
	{
		UndoStackItem *current = undo_stack->current;
// Currently on an after entry
		if(current)
		{
			current = PREVIOUS;
		}

// Now on a before entry
		if(current)
		{
			if(current->get_creator() == creator &&
				!strcmp(current->get_description(), description) &&
				last_update->get_difference() < UNDO_SPAN)
			{
// Before entry has same creator within minimum time.  Reuse it.
// Stack must point to the before entry
				undo_stack->current = current;
				return;
			}
		}
	}

// Append new entry after current position
	update_undo_entry("", 0, creator, 0);
}

void MainUndo::update_undo_after(const char *description, 
	uint32_t load_flags,
	int changes_made)
{
//printf("MainUndo::update_undo_after %d\n", __LINE__);
	if(undo_stack->number_of(undo_stack->current) % 2)
	{
		printf("MainUndo::update_undo_after %d \"%s\": must be on a before entry to do this. size=%d\n",
			__LINE__,
			description,
			undo_stack->total());

// dump stack
		undo_stack->dump();
// Not getting any update_undo_before to get back in sync, so just append 1 here
//		return;
	}

	update_undo_entry(description, load_flags, 0, changes_made);

// Update the before entry flags
	UndoStackItem *current = undo_stack->last;
	if(current) current = PREVIOUS;
	if(current)
	{
		current->set_flags(load_flags);
		current->set_description((char*)description);
	}

// always has to be done after update_undo_after
    mwindow->gui->put_event([](void *ptr)
        {
            MWindow::instance->update_modified();
        },
        0);
}


int MainUndo::undo()
{
	UndoStackItem *current = undo_stack->current;
	char after_description[BCTEXTLEN];
	after_description[0] = 0;

//printf("MainUndo::undo 1\n");
//undo_stack->dump();

// Rewind to an after entry
	if(current && !(undo_stack->number_of(current) % 2))
	{
		current = PREVIOUS;
	}

// Rewind to a before entry
	if(current && (undo_stack->number_of(current) % 2))
	{
		strcpy(after_description, current->get_description());
		current = PREVIOUS;
	}

// Now have an even number
	if(current)
	{
// store our place in the stack
		undo_stack->current = current;
// Set the redo text to the current description
		if(mwindow->gui) 
			mwindow->gui->mainmenu->redo->update_caption(
				after_description);

		FileXML file;
		char *current_data = current->get_data();
		if(current_data)
		{
//printf("MainUndo::undo %d %s\n", __LINE__, current_data);
			file.read_from_string(current_data);
			load_from_undo(&file, current->get_flags());
//printf("MainUndo::undo %d\n", __LINE__);
			mwindow->set_filename(current->get_filename());
            mwindow->session->changes_made = current->get_modified();
			delete [] current_data;

// move current entry back one step
			undo_stack->pull();    


			if(mwindow->gui)
			{
// Now update the menu with the after entry
				current = PREVIOUS;
// Must be a previous entry to perform undo
				if(current)
					mwindow->gui->mainmenu->undo->update_caption(
						current->get_description());
				else
					mwindow->gui->mainmenu->undo->update_caption("");
//printf("MainUndo::undo %d %d\n", __LINE__, mwindow->session->changes_made);
                mwindow->update_modified();
			}
		}
	}


//undo_stack->dump();
	reset_creators();
	return 0;
}

int MainUndo::redo()
{
	UndoStackItem *current = undo_stack->current;
//printf("MainUndo::redo 1\n");
//undo_stack->dump();

// Get 1st entry
	if(!current) current = undo_stack->first;

// Advance to a before entry
	if(current && (undo_stack->number_of(current) % 2))
	{
		current = NEXT;
	}

// Advance to an after entry
	if(current && !(undo_stack->number_of(current) % 2))
	{
		current = NEXT;
	}

	if(current)
	{
		FileXML file;
		char *current_data = current->get_data();
// store our place in the stack
		undo_stack->current = current;

		if(current_data)
		{
			mwindow->set_filename(current->get_filename());
			file.read_from_string(current_data);
            mwindow->session->changes_made = current->get_modified();
			load_from_undo(&file, current->get_flags());
			delete [] current_data;

			if(mwindow->gui)
			{
// Update menu
				mwindow->gui->mainmenu->undo->update_caption(current->get_description());

// Get next after entry
				current = NEXT;			
				if(current)
					current = NEXT;

				if(current)
					mwindow->gui->mainmenu->redo->update_caption(current->get_description());
				else
					mwindow->gui->mainmenu->redo->update_caption("");
                mwindow->update_modified();
			}
		}
	}
	reset_creators();
//undo_stack->dump();
	return 0;
}


// Here the master EDL loads
int MainUndo::load_from_undo(FileXML *file, uint32_t load_flags)
{
//printf("MainUndo::load_from_undo %d flags=0x%x\n", __LINE__, load_flags);
	mwindow->edl->load_xml(file, load_flags);
	for(Asset *asset = mwindow->edl->assets->first;
		asset;
		asset = asset->next)
	{
		mwindow->mainindexes->add_next_asset(0, asset);
	}

	for(int i = 0; i < mwindow->edl->nested_edls->size(); i++)
	{
		EDL *nested_edl = mwindow->edl->nested_edls->get(i);
		mwindow->mainindexes->add_next_asset(0, nested_edl);
	}
	mwindow->mainindexes->start_build();
	return 0;
}


void MainUndo::reset_creators()
{
	for(UndoStackItem *current = undo_stack->first;
		current;
		current = NEXT)
	{
		current->set_creator(0);
	}
}

void MainUndo::reset_modified()
{
// printf("MainUndo::reset_modified %d current=%p\n", 
// __LINE__, undo_stack->current);

	for(UndoStackItem *current = undo_stack->first;
		current;
		current = NEXT)
	{
		current->set_modified(1);
    }

    if(undo_stack->current)
    {
        UndoStackItem *current = undo_stack->current;
        current->set_modified(0);
        if((undo_stack->number_of(current) % 2))
        {
// on an after entry.  Update the next before entry
            current = NEXT;
            if(current) current->set_modified(0);
        }
        else
        {
// on a before entry.  Update the previous after entry
            current = PREVIOUS;
            if(current) current->set_modified(0);
        }
	}
    mwindow->session->changes_made = 0;
}


