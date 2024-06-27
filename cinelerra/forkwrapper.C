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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/tcp.h>





ForkWrapper::ForkWrapper()
{
	done = 0;
	command_data = 0;
	command_allocated = 0;
    result_value = RESULT_UNDEFINED;
	result_data = 0;
	result_allocated = 0;
	parent_fd = -1;
	child_fd = -1;
	pid = 0;
	is_dummy = 0;
    lock = new Mutex("ForkWrapper::lock");
    title = "";
}

ForkWrapper::~ForkWrapper()
{
	int status;

//    PRINT_TRACE
	if(!is_dummy && pid) 
	{
		waitpid(pid, &status, 0);
	}
//    PRINT_TRACE

	delete [] command_data;
	if(parent_fd >= 0) close(parent_fd);
	if(child_fd >= 0) close(child_fd);
	delete lock;
}

void ForkWrapper::set_title(const char *title)
{
    this->title = title;
}

void ForkWrapper::start(int use_dummy)
{
// Create the process & child socket.
	child_fd = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(child_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    struct sockaddr_in parent_addr;
    socklen_t addrlen = sizeof(child_addr);
    bzero(&child_addr, addrlen);
// Bind to any available interface
    child_addr.sin_addr.s_addr = htonl(INADDR_ANY);
// Let the OS choose an ephemeral port
    child_addr.sin_port = 0;

    if (bind(child_fd, (struct sockaddr *)&child_addr, addrlen) < 0) {
        printf("ForkWrapper::start %d: bind failed %s\n", 
            __LINE__, 
            strerror(errno));
        close(child_fd);
        child_fd = -1;
        return;
    }

// Get the port number assigned by the OS
    getsockname(child_fd, (struct sockaddr *)&child_addr, &addrlen);

// create a temporary pipe to wait for the listen call before connecting
    int temp_pipe[2];
    int _ = pipe(temp_pipe);

// printf("ForkWrapper::start %d this=%p parent_fd=%d child_fd=%d\n", 
// __LINE__, 
// this,
// parent_fd,
// child_fd);

	pid = fork();

// Child process
	if(!pid)
	{
		BC_Signals::reset_locks();

// wait for the parent to connect
        listen(child_fd, 1);
// notify parent we're listening
        uint8_t temp[1];
        temp[0] = 0xff;
        int _ = write(temp_pipe[1], temp, 1);

        child_fd = accept(child_fd, 
            (struct sockaddr *) &parent_addr,
            &addrlen);
        close(temp_pipe[0]);
        close(temp_pipe[1]);
// handshake complete

// printf("ForkWrapper::start %d CHILD %s child_fd=%d pid=%d\n", 
// __LINE__, title, child_fd, getpid());
		init_child();
		child_loop();
		exit(0);
	}
    else
    {
// wait until the child starts
// printf("ForkWrapper::start %d PARENT %s parent_fd=%d pid=%d\n", 
// __LINE__, title, parent_fd, pid);
        uint8_t temp[1];
        int _ = read(temp_pipe[0], temp, 1);
        close(temp_pipe[0]);
        close(temp_pipe[1]);
// handshake complete
//printf("ForkWrapper::start %d child started\n", __LINE__);

// parent process
        if(!use_dummy)
        {
            connect_parent_fd();
        }
    }
}

void ForkWrapper::stop()
{
	send_command(EXIT_CODE, 
		0,
		0);
}

void ForkWrapper::connect_parent_fd()
{
    socklen_t addrlen = sizeof(child_addr);
    parent_fd = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(parent_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
// printf("ForkWrapper::connect_parent_fd %d: port=%d\n", 
// __LINE__, 
// child_addr.sin_port);
    if(connect(parent_fd, (struct sockaddr*)&child_addr, addrlen) < 0)
    {
        printf("ForkWrapper::connect_parent_fd() %d: connect failed %s\n", 
            __LINE__, 
            strerror(errno));
        close(parent_fd);
        parent_fd = -1;
        return;
    }
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
        if(result)
        {
            done = 1;
        }
        else
        {
// handle special commands
            switch(command_token)
            {
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

    int temp = write(parent_fd, header, sizeof(header));
	if(data && bytes) temp = write(parent_fd, data, bytes);
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

    if(pid == 0)
    {
        printf("ForkWrapper::child_running %d: pid=0\n", __LINE__);
        return 0;
    }

	sprintf(string, "/proc/%d/stat", pid);
	FILE *fd = fopen(string, "r");
	if(fd)
	{
		while(!feof(fd) && fgetc(fd) != ')')
			;

		fgetc(fd);
		int status = fgetc(fd);

////printf("ForkWrapper::child_running '%c'\n", status);
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

void ForkWrapper::setup_dummy(ForkWrapper *real_fork, 
    int pid, 
    struct sockaddr_in *child_addr)
{
    this->is_dummy = 1;
    this->child_addr = *child_addr;
	this->real_fork = real_fork;
    this->pid = pid;
    connect_parent_fd();
}



