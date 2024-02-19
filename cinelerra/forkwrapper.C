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



#include "bcsignals.h"
#include "bcwindowbase.inc"
#include "forkwrapper.h"
#include "mutex.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>






ForkWrapper::ForkWrapper()
{
	done = 0;
	command_data = 0;
	command_allocated = 0;
    result_value = RESULT_UNDEFINED;
	result_data = 0;
	result_allocated = 0;
	parent_fd = 0;
	child_fd = 0;
	pid = 0;
	is_dummy = 0;
    lock = new Mutex("ForkWrapper::lock");
    title = "";
}

ForkWrapper::~ForkWrapper()
{
	int status;
	if(!is_dummy && pid) 
	{
		waitpid(pid, &status, 0);
	}

	delete [] command_data;
	if(parent_fd) close(parent_fd);
	if(child_fd) close(child_fd);
	delete lock;
}

void ForkWrapper::set_title(const char *title)
{
    this->title = title;
}

void ForkWrapper::start()
{
// Create the process & socket pair.
	int sockets[2];
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	parent_fd = sockets[0];
	child_fd = sockets[1];
// printf("ForkWrapper::start %d this=%p parent_fd=%d child_fd=%d\n", 
// __LINE__, 
// this,
// parent_fd,
// child_fd);

	pid = fork();

// Child process
	if(!pid)
	{
//printf("ForkWrapper::start %d CHILD %s child_fd=%d pid=%d\n", 
//__LINE__, title, child_fd, getpid());

		BC_Signals::reset_locks();
		init_child();
		child_loop();
		exit(0);
	}
    else
    {
//printf("ForkWrapper::start %d PARENT %s parent_fd=%d pid=%d\n", 
//__LINE__, title, parent_fd, pid);
    }
}

void ForkWrapper::stop()
{
	send_command(EXIT_CODE, 
		0,
		0);
}

void ForkWrapper::init_child()
{
	printf("ForkWrapper::init_child %d\n", __LINE__);
}

int ForkWrapper::handle_command()
{
	printf("ForkWrapper::handle_command %d\n", __LINE__);
	return 0;
}

void ForkWrapper::child_loop()
{
	int result = 0;
	const int debug = 0;

	while(!done)
	{
		if(debug) printf("ForkWrapper::run %d this=%p parent_fd=%d child_fd=%d\n", 
			__LINE__, 
			this,
			parent_fd,
			child_fd);

		result = read_command(0);



		if(debug) printf("ForkWrapper::run %d this=%p result=%d command_token=%d\n", 
			__LINE__, 
			this, 
			result, 
			command_token);

// server crashed
        if(result < 0)
        {
            done = 1;
        }
        else
        if(!result)
        {
// handle special commands
            switch(command_token)
            {
                case FORWARD_COMMAND:
                {
//printf("ForkWrapper::child_loop %d FORWARD_COMMAND this=%p\n", __LINE__, this);
// forward a command to a fork in the tunnel memory space
                    ForkWrapper *real_fork = *(ForkWrapper**)command_data;
                    uint8_t *wrapped_data = command_data + sizeof(ForkWrapper*);
                    int wrapped_bytes = command_bytes - sizeof(ForkWrapper*);

// Don't forward certain wrapped commands.  Just read a result code.
// The tunnel stays locked & the dummy send_command blocks until we get a response.
                    int wrapped_command = *(int*)wrapped_data;
                    if(wrapped_command != READ_RESULT)
                    {
                        int temp = write(real_fork->parent_fd, wrapped_data, wrapped_bytes);
                    }

// read result from the real fork
                    real_fork->read_result();
// send result to the tunnel
                    send_result(real_fork->result_value, 
                        real_fork->result_data, 
                        real_fork->result_bytes);
//printf("ForkWrapper::child_loop %d FORWARD_COMMAND this=%p result=%d\n", 
//__LINE__,
//this,
//(int)real_fork->result_value);
                    break;
                }

// read a result code outside a dummy ForkWrapper
                case READ_RESULT:
                    break;

		        case EXIT_CODE:
        	        done = 1;
                    break;

		        default:
    			    handle_command();
            }
        }
	}
}


int ForkWrapper::send_command(int token, 
		unsigned char *data,
		int bytes)
{
	uint8_t header[sizeof(int) * 2];
// printf("ForkWrapper::send_command %d this=%p parent_fd=%d token=%d data=%p bytes=%d\n", 
// __LINE__, 
// this,
// parent_fd, 
// token,
// data,
// bytes);
	*(int*)(header + 0) = token;
	*(int*)(header + sizeof(int)) = bytes;

    if(is_dummy)
    {
// wrap it in a command to the tunnel
        uint8_t wrapper_header[sizeof(int) * 2];
        int wrapped_bytes = sizeof(ForkWrapper*) + sizeof(header) + bytes;
        *(int*)(wrapper_header) = FORWARD_COMMAND;
        *(int*)(wrapper_header + sizeof(int)) = wrapped_bytes;

        uint8_t wrapper_buffer[sizeof(ForkWrapper*)];
        *(ForkWrapper**)wrapper_buffer = real_fork;
        

        tunnel->lock->lock("ForkWrapper::send_command");

        int temp = write(tunnel->parent_fd, wrapper_header, sizeof(wrapper_header));
        temp = write(tunnel->parent_fd, wrapper_buffer, sizeof(wrapper_buffer));
        temp = write(tunnel->parent_fd, header, sizeof(header));
        if(data && bytes) temp = write(tunnel->parent_fd, data, bytes);

// copy the response from the tunnel to this dummy
        tunnel->read_result();
        result_value = tunnel->result_value;
        result_bytes = tunnel->result_bytes;
        if(result_bytes > 0)
        {
            if(result_allocated < result_bytes)
            {
                delete [] result_data;
                result_data = new unsigned char[result_bytes];
                result_allocated = result_bytes;
            }
            memcpy(result_data, tunnel->result_data, result_bytes);
        }

        tunnel->lock->unlock();
    }
    else
    {
    	int temp = write(parent_fd, header, sizeof(header));
	    if(data && bytes) temp = write(parent_fd, data, bytes);
    }
	return 0;
}

int ForkWrapper::read_command(int use_timeout)
{
	unsigned char header[sizeof(int) * 2];
    int result = 0;
//printf("ForkWrapper::read_command %d child_fd=%d\n", __LINE__, child_fd);
    if(!use_timeout)
    {
        result = read(child_fd, header, sizeof(header));
    }
    else
    {
    	result = read_timeout(child_fd, header, sizeof(header));
    }

    if(result <= 0)
    {
        return 1;
    }

//printf("ForkWrapper::read_command %d child_fd=%d\n", __LINE__, child_fd);
	command_token = *(int*)(header + 0);
	command_bytes = *(int*)(header + sizeof(int));

//printf("ForkWrapper::read_command %d command_token=%d command_bytes=%d\n", 
//__LINE__, 
//command_token, 
//command_bytes);
	if(command_bytes && command_allocated < command_bytes)
	{
		delete [] command_data;
		command_data = new unsigned char[command_bytes];
		command_allocated = command_bytes;
	}
	if(command_bytes) 
    {
        result = read_timeout(child_fd, command_data, command_bytes);
    }

    if(result <= 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int ForkWrapper::send_result(int64_t value, unsigned char *data, int data_size)
{
	unsigned char buffer[sizeof(int64_t) + sizeof(int)];
	*(int64_t*)(buffer + 0) = value;
	*(int*)(buffer + sizeof(int64_t)) = data_size;

// printf("ForkWrapper::send_result %d: %s child_fd=%d value=%d data_size=%d\n", 
// __LINE__, 
// title,
// child_fd,
// (int)value, 
// data_size);

	int temp = write(child_fd, buffer, sizeof(buffer));
	if(data && data_size) temp = write(child_fd, data, data_size);
	return 0;
}

// Return -1 if the sender is dead
int ForkWrapper::read_timeout(int fd, unsigned char *data, int size)
{
	fd_set rfds;
	struct timeval timeout_struct;
	int bytes_read = 0;

// Poll child status while doing timed reads
	while(bytes_read < size)
	{
		timeout_struct.tv_sec = 1;
		timeout_struct.tv_usec = 0;
		FD_ZERO(&rfds);
//		FD_SET(parent_fd, &rfds);
		FD_SET(fd, &rfds);
//		int result = select(parent_fd + 1, &rfds, 0, 0, &timeout_struct);
		int result = select(fd + 1, &rfds, 0, 0, &timeout_struct);

		if(result <= 0 && !child_running()) return -1;


		if(result > 0)
		{
			result = read(fd, data + bytes_read, size - bytes_read);
			if(result <= 0) 
            {
// timeout or socket closed
                result = -1;
                break;
            }

            bytes_read += result;
		}
	}

	return bytes_read;
}

int64_t ForkWrapper::read_result()
{
    if(is_dummy)
    {
// return values populated by send_command
        return result_value;
    }

	unsigned char buffer[sizeof(int64_t) + sizeof(int)];
//printf("ForkWrapper::read_result %d parent_fd=%d\n", __LINE__, parent_fd);

	if(read_timeout(parent_fd, buffer, sizeof(buffer)) <= 0)
    {
        printf("ForkWrapper::read_result %d timed out\n", __LINE__);
        result_value = READ_RESULT_FAILED;
        return READ_RESULT_FAILED;
    }
//printf("ForkWrapper::read_result %d  parent_fd=%d\n", __LINE__, parent_fd);

	result_value = *(int64_t*)(buffer + 0);
	result_bytes = *(int*)(buffer + sizeof(int64_t));

// printf("ForkWrapper::read_result %d %s parent_fd=%d result_value=%d result_bytes=%d\n", 
// __LINE__, 
// title,
// parent_fd,
// (int)result_value,
// result_bytes);

	if(result_bytes && result_allocated < result_bytes)
	{
		delete [] result_data;
		result_data = new unsigned char[result_bytes];
		result_allocated = result_bytes;
	}

	if(result_bytes) 
	{
		if(read_timeout(parent_fd, result_data, result_bytes) <= 0) 
		{
            printf("ForkWrapper::read_result %d timed out\n", __LINE__);
			return READ_RESULT_FAILED;
		}
	}
//printf("ForkWrapper::read_result %d  parent_fd=%d\n", __LINE__, parent_fd);

	return result_value;
}

// return 1 if the child is running
int ForkWrapper::child_running()
{
	char string[BCTEXTLEN];
	sprintf(string, "/proc/%d/stat", pid);
	FILE *fd = fopen(string, "r");
	if(fd)
	{
		while(!feof(fd) && fgetc(fd) != ')')
			;

		fgetc(fd);
		int status = fgetc(fd);

//printf("ForkWrapper::child_running '%c'\n", status);
		fclose(fd);
		if(status == 'Z') 
		{
			printf("ForkWrapper::child_running %d: this=%p is_dummy=%d process %d dead\n", 
                __LINE__, 
                this,
                is_dummy,
                pid);
			return 0;
		}
		else
			return 1;
	}
	else
	{
		printf("ForkWrapper::child_running %d: this=%p is_dummy=%d process %d not found\n", 
            __LINE__, 
            this,
            is_dummy,
            pid);
		return 0;
	}
}

void ForkWrapper::setup_dummy(ForkWrapper *real_fork, ForkWrapper *tunnel)
{
    this->is_dummy = 1;
    this->tunnel = tunnel;
	this->real_fork = real_fork;
}



