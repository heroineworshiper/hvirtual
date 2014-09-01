#ifndef ROBOTCLIENT_H
#define ROBOTCLIENT_H

#include "bcwindowbase.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "thread.h"

class RobotClient : public Thread
{
public:
	RobotClient(MWindow *mwindow, char *hostname, int port);
	~RobotClient();

	int open_socket();
	void create_objects();
// This one must be asynchronous to the command loop
	int send_abort();
	int send_command(char operation,
		int src_row,
		int src_column,
		int dst_row,
		int dst_column,
		char *command_text,
		char *completion_text);
	void write_int16(int socket_fd, int value);
	void run();

	Mutex *next_command;
	Mutex *prev_command;
	int command_running;
	MWindow *mwindow;
	char hostname[BCTEXTLEN];
	int port;
	int done;
	int socket_fd;
	char command_text[BCTEXTLEN];
	char completion_text[BCTEXTLEN];
};



#endif
