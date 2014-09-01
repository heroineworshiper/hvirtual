#ifndef CHECKIN_H
#define CHECKIN_H


#include "checkingui.inc"
#include "mwindow.inc"
#include "thread.h"


class CheckIn : public Thread
{
public:
	CheckIn(MWindow *mwindow);
	~CheckIn();
	
	void start();
	void run();
	
	MWindow *mwindow;
	CheckInGUI *gui;
};


#endif
