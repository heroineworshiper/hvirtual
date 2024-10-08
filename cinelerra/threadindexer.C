
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
#include "assets.h"
#include "bcprogressbox.h"
#include "condition.h"
#include "bchash.h"
#include "filesystem.h"
#include "guicast.h"
#include "indexfile.h"
#include "language.h"
#include "loadfile.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "threadindexer.h"

#include <string.h>

// This doesn't appear to be used anymore.




ThreadIndexer::ThreadIndexer(MWindow *mwindow, Assets *assets)
 : Thread()
{
	this->mwindow = mwindow;
	this->assets = assets;
	set_synchronous(0);
	indexfile = new IndexFile(mwindow);
	interrupt_lock = new Condition(0, "ThreadIndexer::ThreadIndexer");
}

ThreadIndexer::~ThreadIndexer()
{
	delete indexfile;
	delete interrupt_lock;
}

int ThreadIndexer::start_build()
{
	interrupt_flag = 0;
	start();
}

// build all here to allow editing during build
void ThreadIndexer::run()
{
// check locations of each asset
	FILE *test_file;
	Asset *current_asset;
	char new_filename[1024], old_filename[1024];
	int result = 0;
	BC_ProgressBox *progress = 0;

	for(current_asset = assets->first; current_asset; current_asset = current_asset->next)
	{
		if(!(test_file = fopen(current_asset->path, "rb")))
		{
// file doesn't exist
			strcpy(old_filename, current_asset->path);

			result = 1;
// get location from user
			char directory[1024];
			sprintf(directory, "~");
			mwindow->defaults->get("DIRECTORY", directory);

			char string[1024], name[1024];
			FileSystem dir;
			dir.extract_name(name, old_filename);
			sprintf(string, _("Where is %s?"), name);

			LocateFileWindow window(mwindow, directory, string);
			window.create_objects();
			int result2 = window.run_window();

			mwindow->defaults->update("DIRECTORY", 
				window.get_submitted_path());

			if(result2 == 1)
			{
				new_filename[0] = 0;
			}
			else
			{
				strcpy(new_filename, 
					window.get_submitted_path());
			}

// update assets containing old location
			assets->update_old_filename(old_filename, new_filename);
		}
		else
		{
			fclose(test_file);
		}
	}


// test index of each asset
	for(current_asset = assets->first; 
		current_asset && !interrupt_flag; 
		current_asset = current_asset->next)
	{
// test for an index file already built before creating progress bar
		if(current_asset->index_status == INDEX_NOTTESTED && 
			current_asset->audio_data)
		{
			if(indexfile->open_index(mwindow, current_asset))
			{
// doesn't exist
// try to create now
				if(!progress)
				{
					progress = new BC_ProgressBox(mwindow->gui->get_abs_cursor_x(1),
						mwindow->gui->get_abs_cursor_y(1),
						_("Building Indexes..."), 
						1);
					progress->start();
				}

				if(progress->is_cancelled()) interrupt_flag = 1;
			}
			else
			{
				if(current_asset->index_status == INDEX_NOTTESTED) 
					current_asset->index_status = INDEX_READY;   // index has been tested
				indexfile->close_index();
			}
		}
	}

// progress box is only created when an index is built
	if(progress)     
	{	
		progress->stop_progress();
		delete progress;
		progress = 0;
	}

	interrupt_lock->unlock();
}

int ThreadIndexer::interrupt_build()
{
	interrupt_flag = 1;
	indexfile->interrupt_index();
	interrupt_lock->lock(" ThreadIndexer::interrupt_build");
	interrupt_lock->unlock();
}

