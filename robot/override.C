#include "mwindow.h"
#include "override.h"
#include "overridegui.h"
#include "robottheme.h"




Override::Override(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	gui = 0;
	this->mwindow = mwindow;
}

Override::~Override()
{
}


void Override::start(int delete_next)
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
		this->delete_next = delete_next;
		Thread::start();
	}
}

void Override::run()
{
	mwindow->theme->get_checkout_sizes();
	gui = new OverrideGUI(mwindow);
	gui->create_objects();
	int result = gui->run_window();
	delete gui;
	gui = 0;

	if(!result)
	{
		mwindow->check_out_cd(1, delete_next);
	}
}





