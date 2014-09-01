#ifndef DELETEGUI_H
#define DELETEGUI_H


#include "allocationgui.h"
#include "delete.inc"
#include "deletegui.inc"
#include "guicast.h"
#include "mwindow.inc"




class DeleteLocation : public AllocationGUI
{
public:
	DeleteLocation(MWindow *mwindow, 
		DeleteGUI *gui, 
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	MWindow *mwindow;
	DeleteGUI *gui;
};



class DeleteGUI : public BC_Window
{
public:
	DeleteGUI(MWindow *mwindow, Delete *thread);
	~DeleteGUI();

	void create_objects();
	int keypress_event();
	int close_event();
	int resize_event(int w, int h);

	MWindow *mwindow;
	Delete *thread;
	BC_Title *location_title;
	DeleteLocation *location;
	BC_OKButton *ok;
	BC_CancelButton *cancel;
};



#endif
