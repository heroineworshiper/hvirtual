
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

#include "asset.h"
#include "bcsignals.h"
#include "bchash.h"
#include "edl.h"
#include "file.h"
#include "filesystem.h"
#include "indexfile.h"
#include "indexstate.h"
#include "condition.h"
#include "language.h"
#include "loadfile.h"
#include "guicast.h"
#include "mainindexes.h"
#include "mainprogress.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"

#include <string.h>


MainIndexes::MainIndexes(MWindow *mwindow)
 : Thread()
{
	set_synchronous(1);
	this->mwindow = mwindow;
	input_lock = new Condition(0, "MainIndexes::input_lock");
	next_lock = new Mutex("MainIndexes::next_lock");
	index_lock = new Mutex("MainIndexes::index_lock");
	interrupt_lock = new Condition(1, "MainIndexes::interrupt_lock");
	interrupt_flag = 0;
	indexfile = 0;
	done = 0;
}

MainIndexes::~MainIndexes()
{
	mwindow->mainprogress->cancelled = 1;
	stop_loop();
	delete next_lock;
	delete input_lock;
	delete interrupt_lock;
	delete indexfile;
	delete index_lock;
}

int MainIndexes::add_next_asset(File *file, Indexable *indexable)
{
	next_lock->lock("MainIndexes::add_next_asset");

// Test current asset
//printf("MainIndexes::add_next_asset %d\n", __LINE__);
	IndexFile indexfile(mwindow, indexable);
	IndexState *index_state = 0;
	index_state = indexable->index_state;

	int got_it = 0;
    int result = indexfile.open_index();

//printf("MainIndexes::add_next_asset %d result=%d\n", __LINE__, result);
	if(result == FILE_OK)
	{
		index_state->index_status = INDEX_READY;
		indexfile.close_index();
		got_it = 1;
	}

//printf("MainIndexes::add_next_asset %d frame_rate=%f\n", __LINE__, indexable->get_frame_rate());

// No index
	if(!got_it)
	{
		File *this_file = file;

        result = FILE_OK;
		if(!file && indexable->is_asset)
		{
			this_file = new File;
			result = this_file->open_file(mwindow->preferences,
				(Asset*)indexable,
				1,
				0);
		}
//printf("MainIndexes::add_next_asset %d result=%d\n", __LINE__, result);

        if(result == FILE_OK)
        {
		    string index_filename;
		    string source_filename;
            string path(indexable->path);
		    IndexFile::get_index_filename(&source_filename, 
			    &mwindow->preferences->index_directory, 
			    &index_filename, 
			    &path);

		    if(this_file /* && !this_file->get_index((char*)index_filename.c_str()) */)
		    {
			    if(!indexfile.open_index())
			    {
				    indexfile.close_index();
				    index_state->index_status = INDEX_READY;
				    got_it = 1;
			    }
		    }
        }

		if(this_file && !file) delete this_file;
	}

//printf("MainIndexes::add_next_asset %d %f\n", __LINE__, indexable->get_frame_rate());

// Put source in stack
	if(!got_it)
	{
		index_state->index_status = INDEX_NOTTESTED;
		next_indexables.append(indexable);
		indexable->add_user();
	}
	next_lock->unlock();
    
//printf("MainIndexes::add_next_asset %d result=%d\n", __LINE__, result);
    return result;
}

void MainIndexes::delete_current_sources()
{
	for(int i = 0; i < current_indexables.size(); i++)
		current_indexables.get(i)->Garbage::remove_user();
	current_indexables.remove_all();
}

void MainIndexes::start_loop()
{
	interrupt_flag = 0;
	Thread::start();
}

void MainIndexes::stop_loop()
{
	interrupt_flag = 1;
	done = 1;
	input_lock->unlock();
	interrupt_lock->unlock();
	Thread::join();
}


void MainIndexes::start_build()
{
//printf("MainIndexes::start_build 1\n");
	interrupt_flag = 0;
// Locked up when indexes were already being built and an indexable was 
// pasted.
//	interrupt_lock.lock();
	input_lock->unlock();
}

void MainIndexes::interrupt_build()
{
//printf("MainIndexes::interrupt_build 1\n");
	interrupt_flag = 1;
	index_lock->lock("MainIndexes::interrupt_build");
	if(indexfile) indexfile->interrupt_index();
	index_lock->unlock();
//printf("MainIndexes::interrupt_build 2\n");
	interrupt_lock->lock("MainIndexes::interrupt_build");
//printf("MainIndexes::interrupt_build 3\n");
	interrupt_lock->unlock();
//printf("MainIndexes::interrupt_build 4\n");
}

void MainIndexes::load_next_sources()
{
	delete_current_sources();

// Transfer from new list
	next_lock->lock("MainIndexes::load_next_sources");
	for(int i = 0; i < next_indexables.size(); i++)
		current_indexables.append(next_indexables.get(i));

// Clear pointers from new list only
	next_indexables.remove_all();
	next_lock->unlock();
}


void MainIndexes::run()
{
	while(!done)
	{
// Wait for new indexables to be released
		input_lock->lock("MainIndexes::run 1");
		if(done) return;


		interrupt_lock->lock("MainIndexes::run 2");
		load_next_sources();
		interrupt_flag = 0;






// test index of each indexable
		MainProgressBar *progress = 0;
		int total_sources = current_indexables.size();
		for(int i = 0; 
			i < total_sources && !interrupt_flag; 
			i++)
		{
			Indexable *indexable = 0;
// Take an indexable
			indexable = current_indexables.get(i);

			IndexState *index_state = 0;
			index_state = indexable->index_state;

//printf("MainIndexes::run 3 %s %d %d\n", indexable->path, indexable->index_status, indexable->audio_data);

			if(index_state->index_status == INDEX_NOTTESTED && 
				indexable->have_audio())
			{


				index_lock->lock("MainIndexes::run 1");
				indexfile = new IndexFile(mwindow, indexable);
				index_lock->unlock();


// Doesn't exist if this returns 1.
				if(indexfile->open_index())
				{
// Try to create index now.
					if(!progress)
					{
						if(mwindow->gui) mwindow->gui->lock_window("MainIndexes::run 1");
						progress = mwindow->mainprogress->start_progress(_("Building Indexes..."), 1);
						if(mwindow->gui) mwindow->gui->unlock_window();
					}


					indexfile->create_index(progress);
					if(progress->is_cancelled()) interrupt_flag = 1;
				}
				else
// Exists.  Update real thing.
				{
//printf("MainIndexes::run 8\n");
					if(index_state->index_status == INDEX_NOTTESTED)
					{
						index_state->index_status = INDEX_READY;
						if(mwindow->gui) mwindow->gui->lock_window("MainIndexes::run 2");
						mwindow->edl->set_index_file(indexable);
						if(mwindow->gui) mwindow->gui->unlock_window();
					}
					indexfile->close_index();
				}

				index_lock->lock("MainIndexes::run 2");
				delete indexfile;
				indexfile = 0;
				index_lock->unlock();
//printf("MainIndexes::run 8\n");
			}
//printf("MainIndexes::run 9\n");
		}

		if(progress)     // progress box is only created when an index is built
		{
			if(mwindow->gui) mwindow->gui->lock_window("MainIndexes::run 3");
			progress->stop_progress();
			delete progress;
			if(mwindow->gui) mwindow->gui->unlock_window();
			progress = 0;
		}






		interrupt_lock->unlock();
	}
}

