#include "filegui.h"
#include "filethread.h"
#include "mwindow.h"
#include "mwindowgui.h"


#include <string.h>


FileWindow::FileWindow(MWindow *mwindow,
	FileGUI *gui,
	int x,
	int y)
 : BC_FileBox(x - BC_WindowBase::get_resources()->filebox_w / 2, 
 	y - BC_WindowBase::get_resources()->filebox_h / 2,
	gui->path,
	gui->title,
	gui->caption,
// Set to 1 to get hidden files. 
	gui->show_hidden,
// Want only directories
	gui->want_directory,
	0,
	0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}






FileThread::FileThread(MWindow *mwindow,
	FileGUI *gui)
 : Thread(0, 0, 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	completion = new Mutex;
	window = 0;
}

FileThread::~FileThread()
{
	if(window)
	{
		window->lock_window();
		window->set_done(1);
		window->unlock_window();
		completion->lock();
		completion->unlock();
	}
	delete completion;
}

void FileThread::start_browse()
{
	if(Thread::running())
	{
		window->lock_window();
		window->raise_window();
		window->flush();
		window->unlock_window();
	}
	else
	{
		completion->lock();
		Thread::start();
	}
}

void FileThread::run()
{
	window = new FileWindow(mwindow,
		gui,
		mwindow->gui->get_abs_cursor_x(1),
		mwindow->gui->get_abs_cursor_y(1));
	window->create_objects();
	int result = window->run_window();
	strcpy(gui->path, window->get_path(0));
	delete window;
	window = 0;

	if(!result)
	{
		gui->update_textbox();
	}
	completion->unlock();
}




