#ifndef CHECKINGUI_H
#define CHECKINGUI_H

#include "allocationgui.h"
#include "checkingui.inc"
#include "guicast.h"
#include "mwindow.inc"

class CheckInSource : public AllocationGUI
{
public:
	CheckInSource(MWindow *mwindow, 
		CheckInGUI *gui, 
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	MWindow *mwindow;
	CheckInGUI *gui;
};

class CheckInDest : public AllocationGUI
{
public:
	CheckInDest(MWindow *mwindow, 
		CheckInGUI *gui,
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	MWindow *mwindow;
	CheckInGUI *gui;
};

class CheckInGUI : public BC_Window
{
public:
	CheckInGUI(MWindow *mwindow);
	~CheckInGUI();
	
	void create_objects();
	int keypress_event();
	int close_event();
	int resize_event(int w, int h);

	MWindow *mwindow;
	BC_Title *source_title;
	CheckInSource *source;
	BC_Title *dst_title;
	CheckInDest *dest;
	BC_OKButton *ok;
	BC_CancelButton *cancel;
};


#endif
