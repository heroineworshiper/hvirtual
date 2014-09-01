#ifndef FILETHREAD_H
#define FILETHREAD_H

#include "filegui.inc"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "thread.h"

class FileWindow : public BC_FileBox
{
public:
	FileWindow(MWindow *mwindow,
		FileGUI *gui,
		int x,
		int y);
	MWindow *mwindow;
	FileGUI *gui;
};

class FileThread : public Thread
{
public:
	FileThread(MWindow *mwindow,
		FileGUI *gui);
	~FileThread();

	void start_browse();
	void run();
	
	MWindow *mwindow;
	FileGUI *gui;
	FileWindow *window;
	Mutex *completion;
};


#endif
