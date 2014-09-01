#ifndef ROBOT_H
#define ROBOT_H

#define TITLE "Heroine 1120"

#include <pthread.h>

/* Commands */
enum
{
	NO_OPERATION,
	SERVER,
	POWER_ON,
	POWER_OFF,
	GET_CD,
	PUT_CD,
	MOVE_CD,
	GET_CD_ROW,
	PUT_CD_ROW,
	MOVE_CD_ROW,
	GOTO_COLUMN,
	AUTO_LEVEL,
	GOTO_ROW,
	DO_TEST,
	SLIDE_FORWARD_STOP,
	SLIDE_BACKWARD_STOP,
	PULL_CD_OUT,
	PUSH_CD_IN,
	ABORT_OPERATION
};

typedef struct 
{
	pthread_mutex_t next_command;
	pthread_mutex_t prev_command;
	int current_pid;
	int new_socket_fd;
	char command;
	int src_row;
	int src_column;
	int dst_row;
	int dst_column;
} robot_server_t;

/* Format of server command packet */
/* 0 - command enumeration         */
/* 1,2 - Big endian src_row        */
/* 3,4 - Big endian src_column     */
/* 5,6 - Big endian dst_row        */
/* 7,8 - Big endian dst_column     */


void run_server(int port);
void run_command(int operation,
	int src_row,
	int src_column,
	int dst_row,
	int dst_column);

#endif
