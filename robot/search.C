#include "mwindow.h"
#include "robottheme.h"
#include "search.h"
#include "searchgui.h"




Search::Search(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	gui = 0;
	this->mwindow = mwindow;
}

Search::~Search()
{
}

void Search::start()
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
		Thread::start();
	}
}

void Search::run()
{
	mwindow->theme->get_search_sizes();
	gui = new SearchGUI(mwindow, this);
	gui->create_objects();
	int result = gui->run_window();
	delete gui;
	gui = 0;
}

