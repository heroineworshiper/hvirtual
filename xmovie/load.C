#include "guicast.h"
#include "load.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include <string.h>

LoadThread::LoadThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
}
LoadThread::~LoadThread()
{
}
void LoadThread::run()
{
	int result = 0;
	{
		BC_FileBox window(mwindow->gui->get_abs_cursor_x(1), 
			mwindow->gui->get_abs_cursor_y(1), 
			mwindow->default_path, 
			"XMovie: Load", 
			"Select the file to load:");
		window.create_objects();
		result = window.run_window();
		strcpy(mwindow->default_path, window.get_path(0));
	}

	if(!result) 
	{
		mwindow->gui->lock_window();
		result = mwindow->load_file(mwindow->default_path, 1);
		mwindow->gui->unlock_window();
	}
}
