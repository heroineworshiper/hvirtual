#include "manual.h"
#include "manualgui.h"
#include "mwindow.h"
#include "robottheme.h"



ManualOverride::ManualOverride(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	gui = 0;
	this->mwindow = mwindow;
}

ManualOverride::~ManualOverride()
{
}

void ManualOverride::start()
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

void ManualOverride::run()
{
	mwindow->theme->get_manual_sizes();
	gui = new ManualGUI(mwindow);
	gui->create_objects();
	int result = gui->run_window();
	delete gui;
	gui = 0;
}

