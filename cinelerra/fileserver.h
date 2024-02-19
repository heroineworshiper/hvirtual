/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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


// This creates all the FileFork objects in a fork of its own so we don't
// duplicate open file descriptors & keep memory from getting freed.



#include "file.inc"
#include "filefork.inc"
#include "forkwrapper.h"
#include "mutex.inc"
#include "preferences.inc"




class FileServer : public ForkWrapper
{
public:
	FileServer();
	virtual ~FileServer();

// Creates a dummy filefork pointer & commands the server to start a real filefork.
// The real filefork's socket is copied & transferred to the dummy.
	FileFork* new_filefork();
	
// Called by the dummy filefork's destructor.
// Commands the server to delete the real filefork
	void delete_filefork(ForkWrapper *real_fork);

	void init_child();

	int handle_command();

// commands
	enum
	{
		NEW_FILEFORK,
		DELETE_FILEFORK,
	};
};










