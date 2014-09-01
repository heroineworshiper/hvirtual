#include "checkoutgui.h"
#include "mwindow.h"
#include "robotalloc.h"
#include "robotprefs.h"
#include "robottheme.h"






CheckOutSource::CheckOutSource(MWindow *mwindow, 
	CheckOutGUI *gui,
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
	mwindow->prefs->check_out_src_tower,
	mwindow->prefs->check_out_src_row,
	0,
	1,
	0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}


int CheckOutSource::handle_event()
{
	mwindow->prefs->check_out_src_tower = tower;
	mwindow->prefs->check_out_src_row = row;
	return 1;
}






CheckOutDest::CheckOutDest(MWindow *mwindow, 
	CheckOutGUI *gui, 
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
	mwindow->prefs->check_out_dst_row,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CheckOutDest::handle_event()
{
	mwindow->prefs->check_out_dst_row = row;
	return 1;
}








CheckOutGUI::CheckOutGUI(MWindow *mwindow)
 : BC_Window(TITLE ": Check out", 
	mwindow->theme->mwindow_x + 
		mwindow->theme->mwindow_w / 2 - 
		mwindow->theme->checkout_w / 2,
	mwindow->theme->mwindow_y +
		mwindow->theme->mwindow_h / 2 - 
		mwindow->theme->checkout_h / 2,
	mwindow->theme->checkout_w, 
	mwindow->theme->checkout_h, 
	10,
	10, 
	1,
	0, 
	1)
{
	this->mwindow = mwindow;
}

CheckOutGUI::~CheckOutGUI()
{
	delete source;
	delete dest;
}


void CheckOutGUI::create_objects()
{
	add_subwindow(source_title = new BC_Title(mwindow->theme->checkout_src_x,
		mwindow->theme->checkout_src_y - 20,
		"Source:"));
	source = new CheckOutSource(mwindow, 
		this, 
		mwindow->theme->checkout_src_x, 
		mwindow->theme->checkout_src_y, 
		mwindow->theme->checkout_src_w, 
		mwindow->theme->checkout_src_h);
	source->create_objects();

	add_subwindow(dst_title = new BC_Title(mwindow->theme->checkout_dst_x,
		mwindow->theme->checkout_dst_y - 20,
		"Destination:"));
	dest = new CheckOutDest(mwindow, 
		this, 
		mwindow->theme->checkout_dst_x, 
		mwindow->theme->checkout_dst_y, 
		mwindow->theme->checkout_dst_w, 
		mwindow->theme->checkout_dst_h);
	dest->create_objects();

	add_subwindow(ok = new BC_OKButton(this));
	add_subwindow(cancel = new BC_CancelButton(this));

	show_window();
}

int CheckOutGUI::resize_event(int w, int h)
{
	mwindow->theme->checkout_w = w;
	mwindow->theme->checkout_h = h;
	mwindow->theme->get_checkout_sizes();
	source_title->reposition_window(mwindow->theme->checkout_src_x,
		mwindow->theme->checkout_src_y - 20);
	source->reposition(mwindow->theme->checkout_src_x, 
		mwindow->theme->checkout_src_y, 
		mwindow->theme->checkout_src_w, 
		mwindow->theme->checkout_src_h);
	dst_title->reposition_window(mwindow->theme->checkout_dst_x,
		mwindow->theme->checkout_dst_y - 20);
	dest->reposition(mwindow->theme->checkout_dst_x, 
		mwindow->theme->checkout_dst_y, 
		mwindow->theme->checkout_dst_w, 
		mwindow->theme->checkout_dst_h);
	return 1;
}

int CheckOutGUI::keypress_event()
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

int CheckOutGUI::close_event()
{
	set_done(1);
	return 1;
}







