#ifndef OVERRIDE_H
#define OVERRIDE_H

#include "mwindow.inc"
#include "thread.h"

class Override : public Thread
{
public:
	Override(MWindow *mwindow);
	~Override();

	void start();
	void run();

	OverrideGUI *gui;
	MWindow *mwindow;
};


#endif
