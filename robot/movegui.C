#include "movegui.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "robotalloc.h"
#include "robotprefs.h"
#include "robottheme.h"







MoveSource::MoveSource(MWindow *mwindow, 
	MoveGUI *gui,
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
	mwindow->prefs->move_src_tower,
	mwindow->prefs->move_src_row,
	0,
	1,
	0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int MoveSource::handle_event()
{
	mwindow->prefs->move_src_tower = tower;
	mwindow->prefs->move_src_row = row;
	return 1;
}






MoveDest::MoveDest(MWindow *mwindow, 
	MoveGUI *gui, 
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
	mwindow->prefs->move_dst_tower,
	mwindow->prefs->move_dst_row,
	1,
	1,
	0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int MoveDest::handle_event()
{
	mwindow->prefs->move_dst_tower = tower;
	mwindow->prefs->move_dst_row = row;
	return 1;
}











MoveGUI::MoveGUI(MWindow *mwindow)
 : BC_Window(TITLE ": Move Pancake", 
				mwindow->theme->mwindow_x + 
					mwindow->theme->mwindow_w / 2 - 
					mwindow->theme->move_w / 2,
				mwindow->theme->mwindow_y +
					mwindow->theme->mwindow_h / 2 - 
					mwindow->theme->move_h / 2,
				mwindow->theme->move_w, 
				mwindow->theme->move_h, 
				10, 
				10, 
				1,
				0, 
				1)
{
	this->mwindow = mwindow;
}

MoveGUI::~MoveGUI()
{
	delete src;
	delete dst;
}

void MoveGUI::create_objects()
{
	add_subwindow(src_title = new BC_Title(mwindow->theme->move_src_x, 
		mwindow->theme->move_src_y - 20,
		"Source:"));
	src = new MoveSource(mwindow, 
		this, 
		mwindow->theme->move_src_x, 
		mwindow->theme->move_src_y, 
		mwindow->theme->move_src_w, 
		mwindow->theme->move_src_h);
	src->create_objects();

	add_subwindow(dst_title = new BC_Title(mwindow->theme->move_dst_x, 
		mwindow->theme->move_dst_y - 20,
		"Destination:"));
	dst = new MoveDest(mwindow, 
		this, 
		mwindow->theme->move_dst_x, 
		mwindow->theme->move_dst_y, 
		mwindow->theme->move_dst_w, 
		mwindow->theme->move_dst_h);
	dst->create_objects();

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
}

int MoveGUI::resize_event(int w, int h)
{
	mwindow->theme->move_w = w;
	mwindow->theme->move_h = h;

	mwindow->theme->get_move_sizes();

	src_title->reposition_window(mwindow->theme->move_src_x, 
		mwindow->theme->move_src_y - 20);
	src->reposition(mwindow->theme->move_src_x, 
		mwindow->theme->move_src_y, 
		mwindow->theme->move_src_w, 
		mwindow->theme->move_src_h);
	dst_title->reposition_window(mwindow->theme->move_dst_x, 
		mwindow->theme->move_dst_y - 20);
	dst->reposition(mwindow->theme->move_dst_x, 
		mwindow->theme->move_dst_y, 
		mwindow->theme->move_dst_w, 
		mwindow->theme->move_dst_h);
}


int MoveGUI::keypress_event()
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

int MoveGUI::close_event()
{
	set_done(1);
	return 1;
}


