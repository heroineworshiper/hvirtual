#include "mwindow.inc"
#include "splashgui.h"
#include "vframe.h"





SplashGUI::SplashGUI(MWindow *mwindow, VFrame *bg, int x, int y)
 : BC_Window(TITLE ": Loading DB",
		x,
		y,
		bg->get_w(),
		bg->get_h(),
		-1,
		-1,
		0,
		0,
		1)
{
	this->bg = bg;
	this->mwindow = mwindow;
	last_position = 0;
	total = 0;
}

SplashGUI::~SplashGUI()
{
}

void SplashGUI::create_objects()
{
	draw_vframe(bg, 0, 0);
	add_subwindow(progress = 
		new BC_ProgressBar(0, 
			get_h() - BC_WindowBase::get_resources()->progress_images[0]->get_h(),
			get_w(),
			1));
	flash();
	show_window();
}

void SplashGUI::update_progress_current(int current)
{
	if(last_position == 0 ||
		100 * (current - last_position) / total > 1)
	{
		progress->update(current);
		last_position = current;
	}
}

void SplashGUI::update_progress_length(int total)
{
	if(total != this->total)
	{
		progress->update_length(total);
		this->total = total;
	}
}
