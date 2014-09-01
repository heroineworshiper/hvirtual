#include "allocationtools.h"






AllocationTower::AllocationTower(MWindow *mwindow, int x, int y, int w)
 : BC_TumbleTextBox(tools->parent_window,
 	default_value, 
	min_value, 
 	max_value, 
	x, 
	y, 
	w)
{
	this->mwindow = mwindow;
}

int AllocationTower::handle_event()
{
	return 1;
}








AllocationRow::AllocationRow(MWindow *mwindow, 
	AllocationTools *tools, 
	int x, 
	int y, 
	int w,
	int default_value,
	int max_value,
	int min_value)
 : BC_TumbleTextBox(tools->parent_window,
 	default_value, 
	min_value, 
 	max_value, 
	x, 
	y, 
	w)
{
	this->mwindow = mwindow;
	this->tools = tools;
}

int AllocationRow::handle_event()
{
	return 1;
}







AllocationButton::AllocationButton(MWindow *mwindow, 
	int x, 
	int y)
 : BC_Button()
{
	this->mwindow = mwindow;
}

int AllocationButton::handle_event()
{
	return 1;
}





AllocationTools::AllocationTools(MWindow *mwindow, 
	BC_WindowBase *parent_window,,
	int x, 
	int y)
{
	this->mwindow = mwindow;
	this->parent_window = parent_window;
	thread = 0;
}

AllocationTools::~AllocationTools()
{
	if(thread) delete thread;
}

void AllocationTools::create_objects()
{
	thread = new AllocationPicker(mwindow, this);
}

void AllocationTools::reposition(int x, int y)
{
}
