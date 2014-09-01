#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "robot.h"
#include "robotclient.h"


#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


RobotClient::RobotClient(MWindow *mwindow, 
	char *hostname,
	int port)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->port = port;
	strcpy(this->hostname, hostname);
	done = 0;
	command_running = 0;
	next_command = new Mutex;
	prev_command = new Mutex;
	next_command->lock();
}

RobotClient::~RobotClient()
{
	done = 1;
	next_command->unlock();
	Thread::join();
	delete next_command;
	delete prev_command;
}

void RobotClient::create_objects()
{
	Thread::start();
}

void RobotClient::write_int16(int socket_fd, int value)
{
	unsigned char data[2];
	data[0] = ((unsigned char)value & 0xff00) >> 8;
	data[1] = ((unsigned char)value & 0xff);
	write(socket_fd, data, 2);
}

int RobotClient::open_socket()
{
	int result;
// Connect to server
	if((result = socket(PF_INET, SOCK_STREAM, 0)) <= 0)
	{
		perror("RobotClient::open_socket");
		prev_command->unlock();
		return -1;
	}

	struct sockaddr_in addr;
	struct hostent *hostinfo;
   	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	hostinfo = gethostbyname(hostname);
	if(hostinfo == NULL)
    {
    	fprintf(stderr, 
			"RobotClient::open_socket: unknown host %s.\n", 
			hostname);
    	prev_command->unlock();
		close(result);
		return -1;
    }

	addr.sin_addr = *(struct in_addr *) hostinfo->h_addr;

	if(connect(result, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		fprintf(stderr, "RobotClient::open_socket: %s: %s\n", 
			hostname, 
			strerror(errno));
		prev_command->unlock();
		close(result);
		return -1;
	}

	return result;
}

int RobotClient::send_abort()
{
	int socket_fd = open_socket();
	if(socket_fd < 0) return 1;

// Send abort packet
	char operation = ABORT_OPERATION;
	write(socket_fd, &operation, 1);
	write_int16(socket_fd, 0);
	write_int16(socket_fd, 0);
	write_int16(socket_fd, 0);
	write_int16(socket_fd, 0);
	char data[1];
	while(!read(socket_fd, data, 1))
	{
		;
	}
	close(socket_fd);

	return 0;
}

int RobotClient::send_command(char operation,
	int src_row,
	int src_column,
	int dst_row,
	int dst_column,
	char *command_text,
	char *completion_text)
{
	int result = 0;
// Wait for previous command to finish
	prev_command->lock();


	strcpy(this->command_text, command_text);
	strcpy(this->completion_text, completion_text);

	socket_fd = open_socket();

	if(socket_fd < 0)
	{
		mwindow->display_message("Error contacting robot.", RED);
		return 1;
	}
	else
		mwindow->display_message(command_text);

// Send command packet
	write(socket_fd, &operation, 1);
	write_int16(socket_fd, src_row);
	write_int16(socket_fd, src_column);
	write_int16(socket_fd, dst_row);
	write_int16(socket_fd, dst_column);

	command_running = 1;
	next_command->unlock();
	return result;
}

void RobotClient::run()
{
	while(!done)
	{
		next_command->lock();
		if(done) break;

// Wait for completion
		char data[1];
		while(!read(socket_fd, data, 1))
		{
			;
		}
		close(socket_fd);

		command_running = 0;
		mwindow->gui->lock_window();
		mwindow->display_message(completion_text);
		mwindow->gui->unlock_window();
		prev_command->unlock();
	}
}





