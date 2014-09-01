#ifndef MOVEGUI_H
#define MOVEGUI_H





#include "allocationgui.h"
#include "guicast.h"
#include "movegui.inc"
#include "mwindow.inc"







class MoveSource : public AllocationGUI
{
public:
	MoveSource(MWindow *mwindow, 
		MoveGUI *gui, 
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	MWindow *mwindow;
	MoveGUI *gui;
};

class MoveDest : public AllocationGUI
{
public:
	MoveDest(MWindow *mwindow, 
		MoveGUI *gui,
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	MWindow *mwindow;
	MoveGUI *gui;
};





class MoveGUI : public BC_Window
{
public:
	MoveGUI(MWindow *mwindow);
	~MoveGUI();
	void create_objects();
	int keypress_event();
	int close_event();
	int resize_event(int w, int h);

	BC_Title *src_title;
	MoveSource *src;
	BC_Title *dst_title;
	MoveDest *dst;

	MWindow *mwindow;
};




#endif
