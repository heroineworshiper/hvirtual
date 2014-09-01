#ifndef ALLOCATIONPICKER_H
#define ALLOCATIONPICKER_H



// Popup window that shows 2 tables, with towers in one table and rows in 
// another table.  The occupied ones, the available ones, and the currently 
// selected one are color coded.

#include "allocationgui.inc"
#include "mwindow.inc"
#include "thread.h"


class AllocationPicker : public Thread
{
public:
	AllocationPicker(MWindow *mwindow, AllocationTools *tools);
	~AllocationPicker();

	void start();
	void stop();
	void run();
	
	
	MWindow *mwindow;
	AllocationGUI *gui;
	AllocationTools *tools;
};



#endif
