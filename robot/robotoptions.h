#ifndef ROBOTOPTIONS_H
#define ROBOTOPTIONS_H

#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"



class RobotOptions : public Thread;
{
public:
	RobotOptions(MWindow *mwindow);
	
	
	void start();
	void run();

	RobotOptionsGUI *gui;
	MWindow *mwindow;
};


#endif
