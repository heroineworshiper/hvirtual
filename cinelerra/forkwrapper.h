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

#ifndef FORKWRAPPER_H
#define FORKWRAPPER_H

// Utility functions for all the forking classes


#include "mutex.inc"
#include <stdint.h>

// special command codes
#define EXIT_CODE 0x7ffffffe
#define FORWARD_COMMAND 0x7fffffff
// read a result code from a tunnel without running a command
#define READ_RESULT 0x7ffffffd

// special result codes
#define READ_RESULT_FAILED -0x8000000000000000
#define RESULT_UNDEFINED -0x7fffffffffffffff


class ForkWrapper;


class ForkWrapper
{
public:
	ForkWrapper();
	virtual ~ForkWrapper();

// title for debugging
    void set_title(const char *title);
// Can't start in the constructor because it'll erase the subclass constructor.
	void start();
// Called by subclass to send a command to exit the loop
// TODO: needs to support tunneling from a dummy
	void stop();
// run commands in the child
	void child_loop();
	virtual void init_child();
	virtual int handle_command();
// return 1 if the child is running
	int child_running();
// Return -1 if the fd is dead or 0 on success
	int read_timeout(int fd, unsigned char *data, int size);

// Called by parent to send commands
	int send_command(int token, 
		unsigned char *data,
		int bytes);

// Called by child to get commands
	int read_command(int use_timeout);
// Called by parent to read a result code
	int64_t read_result();

// read a result from a daisychained process
    int64_t read_result(ForkWrapper *src);

// Called by child to send result
	int send_result(int64_t value, unsigned char *data, int data_size);

// set this FileFork to forward to a real FileFork through a tunnel
    void setup_dummy(ForkWrapper *real_fork, ForkWrapper *tunnel, int pid);

	int done;
	int pid;

	int parent_fd;
	int child_fd;

// if this ForkWrapper is a dummy pointing to a real filefork in a tunnel process
// memory space
	int is_dummy;
// dummies forward all their commands through a tunnel ForkWrapper
// to their real child & the tunnel ForkWrapper copies the responses to the
// dummy's result variables
// The dummy's read_result always instantly returns the last result code received
// by the tunnel
    ForkWrapper *real_fork;
    ForkWrapper *tunnel;

// undefined on the parent
// updated by read_command on the child
	int command_token;
	unsigned char *command_data;
	int command_bytes;
	int command_allocated;


    int64_t result_value;
	unsigned char *result_data;
	int result_bytes;
	int result_allocated;
// Only allow 1 send_command + read_result at a time
	Mutex *lock;
    const char *title;
};



#endif



