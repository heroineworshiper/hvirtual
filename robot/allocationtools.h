#ifndef ALLOCATIONGUI_H
#define ALLOCATIONGUI_H


// A widget set that appears in many windows.  Allows the user to 
// specify a tower and row.



#include "guicast.h"
#include "mwindow.inc"

class AllocationTower : public BC_TumbleTextBox
{
public:
	AllocationTower(MWindow *mwindow, int x, int y, int w);
	int handle_event();
	MWindow *mwindow;
};

class AllocationRow : public BC_TumbleTextBox
{
public:
	AllocationRow(MWindow *mwindow, int x, int y, int w);
	int handle_event();
	MWindow *mwindow;
};

class AllocationButton : public BC_Button
{
public:
	AllocationButton(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};


class AllocationTools
{
public:
// tower_return - the index number of the tower
// row_return - the index number of the row
// destination - If 1, the unallocated rows are selectable.  If 0 the
//     allocated rows are selectable.
	AllocationTools(MWindow *mwindow, 
		BC_WindowBase *parent_window,
		int x, 
		int y, 
		int *tower_return, 
		int *row_return,
		int destination);
	void create_objects();
	void reposition(int x, int y);
	virtual int handle_event();

	MWindow *mwindow;
	BC_WindowBase *parent_window;
	AllocationPicker *thread;
	int x, y;
};




#endif
