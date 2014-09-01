#ifndef CHECKOUTGUI_H
#define CHECKOUTGUI_H

#include "allocationgui.h"
#include "checkoutgui.inc"
#include "guicast.h"
#include "mwindow.inc"


class CheckOutSource : public AllocationGUI
{
public:
	CheckOutSource(MWindow *mwindow, 
		CheckOutGUI *gui, 
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	MWindow *mwindow;
	CheckOutGUI *gui;
};

class CheckOutDest : public AllocationGUI
{
public:
	CheckOutDest(MWindow *mwindow, 
		CheckOutGUI *gui,
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	MWindow *mwindow;
	CheckOutGUI *gui;
};

class CheckOutGUI : public BC_Window
{
public:
	CheckOutGUI(MWindow *mwindow);
	~CheckOutGUI();

	void create_objects();
	int resize_event(int w, int h);
	int keypress_event();
	int close_event();


	MWindow *mwindow;
	BC_Title *source_title;
	CheckOutSource *source;
	BC_Title *dst_title;
	CheckOutDest *dest;
	BC_OKButton *ok;
	BC_CancelButton *cancel;
};


#endif
