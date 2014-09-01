#include "mutex.h"
#include "transportque.h"
#include "transportque.inc"


TransportCommand::TransportCommand()
{
	start_position = 0;
	reset();
}

TransportCommand::~TransportCommand()
{
}

void TransportCommand::reset()
{
	command = COMMAND_NONE;
//	start_position = 0;
}

TransportCommand& TransportCommand::operator=(TransportCommand &command)
{
	this->command = command.command;
	this->start_position = command.start_position;
	return *this;
}

int TransportCommand::single_frame()
{
	return(command == FRAME_FORWARD ||
		command == FRAME_REVERSE ||
		command == CURRENT_FRAME);
}

int TransportCommand::change_position()
{
	return(command == FRAME_FORWARD ||
		command == FRAME_REVERSE);
}












TransportQue::TransportQue()
{
	input_lock = new Mutex;
	output_lock = new Mutex;
	output_lock->lock();
	command.reset();
}

TransportQue::~TransportQue()
{
	delete input_lock;
	delete output_lock;
}

void TransportQue::send_command(int command, double start_position)
{
	input_lock->lock();
	this->command.command = command;
	this->command.start_position = start_position;
	input_lock->unlock();
	output_lock->unlock();
}


