#ifndef MANUALOVERRIDE_H
#define MANUALOVERRIDE_H


#include "manualgui.inc"
#include "mwindow.inc"
#include "thread.h"


class ManualOverride : public Thread
{
public:
	ManualOverride(MWindow *mwindow);
	~ManualOverride();
	
	void start();
	void run();
	
	MWindow *mwindow;
	ManualGUI *gui;
};


#endif
