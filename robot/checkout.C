#include "checkout.h"
#include "checkoutgui.h"
#include "mwindow.h"
#include "robottheme.h"




CheckOut::CheckOut(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	gui = 0;
	this->mwindow = mwindow;
}

CheckOut::~CheckOut()
{
}


void CheckOut::start(int delete_next)
{
	if(gui)
	{
		gui->lock_window();
		gui->raise_window();
		gui->flush();
		gui->unlock_window();
	}
	else
	{
		this->delete_next = delete_next;
		Thread::start();
	}
}

void CheckOut::run()
{
	mwindow->theme->get_checkout_sizes();
	gui = new CheckOutGUI(mwindow);
	gui->create_objects();
	int result = gui->run_window();
	delete gui;
	gui = 0;

	if(!result)
	{
		mwindow->check_out_cd(1, delete_next);
	}
}





