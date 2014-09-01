#include "robot.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>


void read_int16(int fd, int *value)
{
	unsigned char data[2];
	int size = 0;
	while(size < 2)
	{
		size += read(fd, &data[size], 1);
	}
	*value = (((int)data[0]) << 8) | data[1];
}

void write_completion(int fd)
{
	char data[1];
	data[0] = 0;
	write(fd, data, 1);
	fsync(fd);
//printf("write_completion 1\n");
	close(fd);
}


// Client thread joins client forks
void* client_thread(void *ptr)
{
	robot_server_t *server = (robot_server_t*)ptr;
	while(1)
	{
		pthread_mutex_lock(&server->next_command);


		int pid = fork();
		if(pid != 0)
		{
			server->current_pid = pid;
			int return_value;
			waitpid(server->current_pid, &return_value, 0);
			server->current_pid = -1;

// Send completion code
			write_completion(server->new_socket_fd);
			server->new_socket_fd = 0;
			pthread_mutex_unlock(&server->prev_command);
		}
		else
		{
// Run command
			run_command(server->command,
				server->src_row,
				server->src_column,
				server->dst_row,
				server->dst_column);
			return;
		}
	}
}

void read_arguments(robot_server_t *server)
{
// Read command arguments
	read_int16(server->new_socket_fd, &server->src_row);
	read_int16(server->new_socket_fd, &server->src_column);
	read_int16(server->new_socket_fd, &server->dst_row);
	read_int16(server->new_socket_fd, &server->dst_column);
}

void fork_client(robot_server_t *server, int new_socket_fd)
{
// Read command
	char new_command = -1;
	int result;
	while(!(result = read(new_socket_fd, &new_command, 1)))
	{
		;
	}

	switch(new_command)
	{
		case ABORT_OPERATION:
// Kill the previous operation first, then power down.
			if(server->current_pid >= 0)
			{
				printf("fork_client: killing %d\n", server->current_pid);
				kill(server->current_pid, SIGKILL);
			}
			else
			{
				printf("fork_client: not running.\n");
			}

// Wait previous command to finish
			pthread_mutex_lock(&server->prev_command);

// Power down
			server->command = POWER_OFF;
			server->new_socket_fd = new_socket_fd;
			read_arguments(server);

			pthread_mutex_unlock(&server->next_command);
			break;



		default:
// Wait for previous command to finish
			pthread_mutex_lock(&server->prev_command);
// Copy command information over
			server->command = new_command;
			server->new_socket_fd = new_socket_fd;
			read_arguments(server);

// Start next command
			pthread_mutex_unlock(&server->next_command);
			break;
	}
}


robot_server_t* new_server()
{
	robot_server_t *result = calloc(1, sizeof(robot_server_t));
	pthread_mutex_init(&result->next_command, 0);
	pthread_mutex_init(&result->prev_command, 0);
	pthread_mutex_lock(&result->next_command);
	result->current_pid = -1;
	return result;
}

void run_server(int port)
{
	printf("Running " TITLE " on port %d\n.", port);

	robot_server_t *server = new_server();

// Start client thread
	pthread_t thread_id;
	pthread_create(&thread_id, 0, client_thread, server);


// Start server loop
	struct sockaddr_in addr;
	int socket_fd;

	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short)port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("run_server: socket");
		return;
	}

	if(bind(socket_fd, 
		(struct sockaddr*)&addr, 
		sizeof(addr)) < 0)
	{
		fprintf(stderr, 
			"run_server: bind port %d: %s",
			port,
			strerror(errno));
		return;
	}


// Wait for connections
	while(1)
	{
		if(listen(socket_fd, 256) < 0)
    	{
    		perror("run_server: listen");
    		return;
    	}

		int new_socket_fd;
		struct sockaddr_in clientname;
		socklen_t size = sizeof(clientname);
    	if((new_socket_fd = accept(socket_fd,
                	(struct sockaddr*)&clientname, 
					&size)) < 0)
    	{
        	perror("run_server: accept");
        	return;
    	}
		else
		{
// Fork off new operation with new_socket_fd
			fork_client(server, new_socket_fd);
		}
	}
}

