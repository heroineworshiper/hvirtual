#include "checkin.h"
#include "checkingui.h"
#include "mwindow.h"
#include "robottheme.h"



CheckIn::CheckIn(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	gui = 0;
	this->mwindow = mwindow;
}

CheckIn::~CheckIn()
{
}

void CheckIn::start()
{
	if(gui)
	{
		gui->lock_window();
		gui->raise_window();
		gui->flush();
		gui->unlock_window();
	}
	else
	{
		Thread::start();
	}
}

void CheckIn::run()
{
	mwindow->theme->get_checkin_sizes();
	gui = new CheckInGUI(mwindow);
	gui->create_objects();
	int result = gui->run_window();
	delete gui;
	gui = 0;

	if(!result)
	{
		mwindow->check_in_cd(1);
	}
}

