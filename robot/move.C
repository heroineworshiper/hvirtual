#include "move.h"
#include "movegui.h"
#include "mwindow.h"
#include "robottheme.h"



Move::Move(MWindow *mwindow)
{
	this->mwindow = mwindow;
	gui = 0;
}

Move::~Move()
{
}


void Move::start_move()
{
	if(Thread::running())
	{
		if(gui)
		{
			gui->lock_window();
			gui->raise_window();
			gui->flush();
			gui->unlock_window();
		}
		return;
	}
	Thread::start();
}

void Move::stop_move()
{
	if(gui)
	{
		gui->lock_window();
		gui->set_done(1);
		gui->unlock_window();
	}
}

void Move::run()
{
	mwindow->theme->get_move_sizes();
	gui = new MoveGUI(mwindow);
	gui->create_objects();
	int result = gui->run_window();
	delete gui;
	gui = 0;
	if(!result)
	{
		mwindow->move_cd();
	}
}


