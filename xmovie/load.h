#ifndef LOAD_H
#define LOAD_H

#include "mwindow.inc"
#include "thread.h"

class LoadThread : public Thread
{
public:
	LoadThread(MWindow *mwindow);
	~LoadThread();
	void run();
	MWindow *mwindow;
};

#endif
