/*
 * CINELERRA
 * Copyright (C) 2009-2024 Adam Williams <broadcast at earthling dot net>
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

#include "file.inc"

#ifdef USE_FILEFORK

#include "bcresources.h"
#include "bcsignals.h"
#include "filefork.h"
#include "fileserver.h"
#include "mutex.h"


#include <unistd.h>


FileServer::FileServer() : ForkWrapper()
{
    set_title("FileServer");
}

FileServer::~FileServer()
{
	stop();
}

void FileServer::init_child()
{
	BC_WindowBase::get_resources()->vframe_shm = 1;
//printf("FileServer::init_child %d %d\n", __LINE__, getpid());
}

int FileServer::handle_command()
{
	const int debug = 0;
	switch(command_token)
	{
		case NEW_FILEFORK:
		{
			FileFork *file_fork = new FileFork;
			file_fork->start(1);
            
            int size = sizeof(FileFork*) + sizeof(int) + sizeof(struct sockaddr_in);
			unsigned char buffer[size];
            int offset = 0;

// store the pointer in this memory space
			*(ForkWrapper**)buffer = file_fork;
            offset += sizeof(ForkWrapper*);
            *(int*)(buffer + offset) = file_fork->pid;
            offset += sizeof(int);
			*(struct sockaddr_in*)(buffer + offset) = file_fork->child_addr;

			if(debug) printf("FileServer::handle_command NEW_FILEFORK %d parent_fd=%d file_fork=%p\n",
				__LINE__,
				file_fork->parent_fd,
				file_fork);
			send_result(0, buffer, size);
			break;
		}

		case DELETE_FILEFORK:
		{
// get the pointer in this memory space
			FileFork *file_fork = *(FileFork**)command_data;
			if(debug) printf("FileServer::handle_command DELETE_FILEFORK %d file_fork=%p\n",
				__LINE__,
				file_fork);
			delete file_fork;
            send_result(0, 0, 0);
			break;
		}
	}

	return 0;
}

FileFork* FileServer::new_filefork()
{
	ForkWrapper::lock->lock("FileServer::new_filefork");
	FileFork *dummy_fork = new FileFork;
    dummy_fork->set_title("Dummy FileFork");
// Create real file fork on the server
	send_command(FileServer::NEW_FILEFORK, 0, 0);
	read_result();

    int offset = 0;
    ForkWrapper *real_fork = *(ForkWrapper**)result_data;
    offset += sizeof(ForkWrapper*);
    int pid = *(int*)(result_data + offset);
    offset += sizeof(int);
    struct sockaddr_in *child_addr = (struct sockaddr_in*)(result_data + offset);

    dummy_fork->setup_dummy(real_fork, pid, child_addr);
// printf("FileServer::new_filefork %d this=%p parent_fd=%d dummy_fork=%p real_fork=%p\n",
// __LINE__,
// this,
// parent_fd,
// dummy_fork,
// dummy_fork->real_fork);
	ForkWrapper::lock->unlock();
	return dummy_fork;
}

void FileServer::delete_filefork(ForkWrapper *real_fork)
{
	ForkWrapper::lock->lock("FileServer::delete_filefork");
// Delete filefork on server
	unsigned char buffer[sizeof(ForkWrapper*)];
	*(ForkWrapper**)buffer = real_fork;
	send_command(FileServer::DELETE_FILEFORK, buffer, sizeof(ForkWrapper*));
	read_result();
    ForkWrapper::lock->unlock();
}



#endif // USE_FILEFORK





