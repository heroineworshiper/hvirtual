#ifndef TRANSPORTQUE_H
#define TRANSPORTQUE_H

#include "mutex.inc"

class TransportCommand
{
public:
	TransportCommand();
	~TransportCommand();

	void reset();
	TransportCommand& operator=(TransportCommand &command);
// Command is a single frame
	int single_frame();
// Command causes a change in position
	int change_position();
	
	
	int command;
	double start_position;
};

class TransportQue
{
public:
	TransportQue();
	~TransportQue();

	void send_command(int command, double start_position);
	TransportCommand command;
	Mutex *input_lock, *output_lock;
};

#endif
