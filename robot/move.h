#ifndef MOVE_H
#define MOVE_H


#include "movegui.inc"
#include "mwindow.inc"
#include "thread.h"


class Move : public Thread
{
public:
	Move(MWindow *mwindow);
	~Move();

	void start_move();
	void stop_move();
	void run();

	MWindow *mwindow;
	MoveGUI *gui;
};




#endif
