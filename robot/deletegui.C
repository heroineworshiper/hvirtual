#include "deletegui.h"
#include "mwindow.h"
#include "robotprefs.h"
#include "robottheme.h"


DeleteLocation::DeleteLocation(MWindow *mwindow, 
	DeleteGUI *gui,
	int x, 
	int y, 
	int w, 
	int h)
 : AllocationGUI(mwindow,
 	gui,
	x,
	y,
	w,
	h,
	mwindow->prefs->delete_tower,
	mwindow->prefs->delete_row,
	0,
	1,
	0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int DeleteLocation::handle_event()
{
	mwindow->prefs->delete_tower = tower;
	mwindow->prefs->delete_row = row;
	return 1;
}















DeleteGUI::DeleteGUI(MWindow *mwindow, Delete *thread)
 : BC_Window(TITLE ": Delete pancake", 
	mwindow->theme->mwindow_x + 
		mwindow->theme->mwindow_w / 2 - 
		mwindow->theme->delete_w / 2,
	mwindow->theme->mwindow_y +
		mwindow->theme->mwindow_h / 2 - 
		mwindow->theme->delete_h / 2,
	mwindow->theme->delete_w, 
	mwindow->theme->delete_h, 
	10,
	10, 
	1,
	0, 
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

DeleteGUI::~DeleteGUI()
{
	delete location;
}

void DeleteGUI::create_objects()
{
	add_subwindow(location_title = new BC_Title(mwindow->theme->delete_src_x, 
		mwindow->theme->delete_src_y - 20,
		"Row to delete"));

	location = new DeleteLocation(mwindow, 
		this, 
		mwindow->theme->delete_src_x, 
		mwindow->theme->delete_src_y, 
		mwindow->theme->delete_src_w,
		mwindow->theme->delete_src_h);
	location->create_objects();

	add_subwindow(ok = new BC_OKButton(this));
	add_subwindow(cancel = new BC_CancelButton(this));

	show_window();
}

int DeleteGUI::resize_event(int w, int h)
{
	mwindow->theme->delete_w = w;
	mwindow->theme->delete_h = h;
	mwindow->theme->get_delete_sizes();
	location_title->reposition_window(mwindow->theme->delete_src_x, 
		mwindow->theme->delete_src_y - 20);
	location->reposition(mwindow->theme->delete_src_x, 
		mwindow->theme->delete_src_y, 
		mwindow->theme->delete_src_w,
		mwindow->theme->delete_src_h);
	return 1;
}

int DeleteGUI::keypress_event()
{
	switch(get_keypress())
	{
		case 'w':
			set_done(1);
			return 1;
			break;
	}
	return 0;
}

int DeleteGUI::close_event()
{
	set_done(1);
	return 1;
}
