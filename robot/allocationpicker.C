#include "allocationpicker.h"



AllocationPicker::AllocationPicker(MWindow *mwindow, AllocationTools *tools)
 : Thread(0, 0, 0)
{
	this->mwindow = mwindow;
	this->tools = tools;
	gui = 0;
}

AllocationPicker::~AllocationPicker()
{
}

void AllocationPicker::start_import()
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

void AllocationPicker::stop_import()
{
	if(gui)
	{
		gui->lock_window();
		gui->set_done(1);
		gui->unlock_window();
	}
}

void AllocationPicker::run()
{
	mwindow->theme->get_allocationgui_sizes();
	gui = new AllocationGUI(mwindow,
		this,
		mwindow->get_abs_cursor_x(),
		mwindow->get_abs_cursor_y());
	gui->create_objects();
	int result = gui->run_window();
	delete gui;
	gui = 0;
}

