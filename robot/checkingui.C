#include "checkingui.h"
#include "mwindow.h"
#include "robotalloc.h"
#include "robotprefs.h"
#include "robottheme.h"




CheckInSource::CheckInSource(MWindow *mwindow, 
	CheckInGUI *gui,
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
	mwindow->allocation->get_loading_tower(),
	mwindow->prefs->check_in_src_row,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int CheckInSource::handle_event()
{
	mwindow->prefs->check_in_src_row = row;
	return 1;
}






CheckInDest::CheckInDest(MWindow *mwindow, 
	CheckInGUI *gui, 
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
	mwindow->prefs->check_in_dst_tower,
	mwindow->prefs->check_in_dst_row,
	1,
	1,
	0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CheckInDest::handle_event()
{
	mwindow->prefs->check_in_dst_tower = tower;
	mwindow->prefs->check_in_dst_row = row;
	return 1;
}








CheckInGUI::CheckInGUI(MWindow *mwindow)
 : BC_Window(TITLE ": Check in", 
	mwindow->theme->mwindow_x + 
		mwindow->theme->mwindow_w / 2 - 
		mwindow->theme->checkin_w / 2,
	mwindow->theme->mwindow_y +
		mwindow->theme->mwindow_h / 2 - 
		mwindow->theme->checkin_h / 2,
	mwindow->theme->checkin_w, 
	mwindow->theme->checkin_h, 
	10,
	10, 
	1,
	0, 
	1)
{
	this->mwindow = mwindow;
}

CheckInGUI::~CheckInGUI()
{
	delete source;
	delete dest;
}

void CheckInGUI::create_objects()
{
	add_subwindow(source_title = new BC_Title(mwindow->theme->checkin_src_x,
		mwindow->theme->checkin_src_y - 20,
		"Source:"));
	source = new CheckInSource(mwindow, 
		this, 
		mwindow->theme->checkin_src_x, 
		mwindow->theme->checkin_src_y, 
		mwindow->theme->checkin_src_w, 
		mwindow->theme->checkin_src_h);
	source->create_objects();

	add_subwindow(dst_title = new BC_Title(mwindow->theme->checkin_dst_x,
		mwindow->theme->checkin_dst_y - 20,
		"Destination:"));
	dest = new CheckInDest(mwindow, 
		this, 
		mwindow->theme->checkin_dst_x, 
		mwindow->theme->checkin_dst_y, 
		mwindow->theme->checkin_dst_w, 
		mwindow->theme->checkin_dst_h);
	dest->create_objects();

	add_subwindow(ok = new BC_OKButton(this));
	add_subwindow(cancel = new BC_CancelButton(this));

	show_window();
}

int CheckInGUI::keypress_event()
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

int CheckInGUI::close_event()
{
	set_done(1);
	return 1;
}

int CheckInGUI::resize_event(int w, int h)
{
	mwindow->theme->checkin_w = w;
	mwindow->theme->checkin_h = h;
	mwindow->theme->get_checkin_sizes();
	source_title->reposition_window(mwindow->theme->checkin_src_x,
		mwindow->theme->checkin_src_y - 20);
	source->reposition(mwindow->theme->checkin_src_x, 
		mwindow->theme->checkin_src_y, 
		mwindow->theme->checkin_src_w, 
		mwindow->theme->checkin_src_h);
	dst_title->reposition_window(mwindow->theme->checkin_dst_x,
		mwindow->theme->checkin_dst_y - 20);
	dest->reposition(mwindow->theme->checkin_dst_x, 
		mwindow->theme->checkin_dst_y, 
		mwindow->theme->checkin_dst_w, 
		mwindow->theme->checkin_dst_h);
	return 1;
}

