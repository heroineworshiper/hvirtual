#include "delete.h"
#include "deletegui.h"
#include "mwindow.h"
#include "robottheme.h"



Delete::Delete(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	gui = 0;
}

Delete::~Delete()
{
}

void Delete::start()
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

void Delete::run()
{
	mwindow->theme->get_delete_sizes();
	gui = new DeleteGUI(mwindow, this);
	gui->create_objects();
	int result = gui->run_window();
	delete gui;
	gui = 0;

	if(!result)
	{
		mwindow->delete_cd();
	}
}

