#ifndef ALLOCATIONGUI_H
#define ALLOCATIONGUI_H


#include "allocationgui.inc"
#include "guicast.h"
#include "mwindow.inc"



class AllocationTower : public BC_ListBox
{
public:
	AllocationTower(MWindow *mwindow, 
		AllocationGUI *gui, 
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	int selection_changed();
	MWindow *mwindow;
	AllocationGUI *gui;
};

class AllocationRow : public BC_ListBox
{
public:
	AllocationRow(MWindow *mwindow, 
		AllocationGUI *gui, 
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	int selection_changed();
	MWindow *mwindow;
	AllocationGUI *gui;
};

class AllocationGUI
{
public:
// tower_index - the index number of the tower
// row_index - the index number of the row
// destination - If 1, the unallocated rows are selectable.  If 0 the
//     allocated rows are selectable.
	AllocationGUI(MWindow *mwindow, 
		BC_WindowBase *parent_window,
		int x,
		int y,
		int w,
		int h, 
		int tower, 
		int row,
		int destination,
		int use_tower,
		int use_loading);
	~AllocationGUI();

	void create_objects();
	void reposition(int x, int y, int w, int h);
	virtual int handle_event();
	void calculate_tower_table();
	void calculate_row_table();
	void update_row_table();

	MWindow *mwindow;
	BC_Title *tower_title;
	AllocationTower *tower_list;
	BC_Title *row_title;
	AllocationRow *row_list;
	BC_WindowBase *parent_window;
	int tower;
	int row;
	int default_tower;
	int default_row;
	int destination;
	int use_tower;
	int use_loading;
	int x, y, w, h;

	ArrayList<BC_ListBoxItem*> tower_data;
	ArrayList<BC_ListBoxItem*> row_data;
};



#endif
