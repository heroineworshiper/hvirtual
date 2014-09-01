
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

#ifndef FORKWRAPPER_H
#define FORKWRAPPER_H

// Utility functions for all the forking classes


#include <stdint.h>


class ForkWrapper
{
public:
	ForkWrapper();
	virtual ~ForkWrapper();

// Can't start in the constructor because it'll erase the subclass constructor.
	void start();
// Called by subclass to send a command to exit the loop
	void stop();
// Use the fd of another fork already in progress.
// Used to transfer a parent ForkWrapper from 1 process to another.
	void start_dummy(int parent_fd, int pid);
	void run();
	virtual void init_child();
	virtual int handle_command();
// return 1 if the child is running
	int child_running();
// Return 1 if the child is dead
	int read_timeout(unsigned char *data, int size);

	int done;

// Called by parent to send commands
	int send_command(int token, 
		unsigned char *data,
		int bytes);
// Called by child to get commands
	int read_command();
// Called by parent to read result
	int64_t read_result();
// Called by child to send result
	int send_result(int64_t value, unsigned char *data, int data_size);
// Called by child to send a file descriptor
	void send_fd(int fd);
// Called by parent to get a file descriptor
	int get_fd();

	int pid;
	int parent_fd;
	int child_fd;
	int is_dummy;

	int command_token;
	unsigned char *command_data;
	int command_bytes;
	int command_allocated;


	unsigned char *result_data;
	int result_bytes;
	int result_allocated;
};



#endif



