
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

#include "mwindow.h"
#include "threadloader.h"

// loads command line filenames at startup

ThreadLoader::ThreadLoader(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
}

ThreadLoader::~ThreadLoader()
{
}

int ThreadLoader::set_paths(ArrayList<char *> *paths)
{
	this->paths = paths;
    return 0;
}

void ThreadLoader::run()
{
	int import_ = 0;
	for(int i = 0; i < paths->total; i++)
	{
//printf("%s\n", paths->values[i]);
//		mwindow->load(paths->values[i], import_);
		import_ = 1;
	}
}
