#include "mwindow.h"
#include "mwindowgui.h"
#include "options.h"
#include "optionsgui.h"
#include "robotprefs.h"
#include "robottheme.h"


Options::Options(MWindow *mwindow)
 : Thread(0, 0, 0)
{
	this->mwindow = mwindow;
	prefs = new RobotPrefs(mwindow);
	gui = 0;
}

Options::~Options()
{
	delete prefs;
}

void Options::start()
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
		prefs->copy_from(mwindow->prefs);
		Thread::start();
	}
}

void Options::apply_changes()
{
	mwindow->gui->lock_window();
	mwindow->prefs->copy_from(prefs);
	mwindow->load_db();
	mwindow->restart_client();
	mwindow->gui->update_in_list();
	mwindow->gui->update_out_list();
	mwindow->gui->unlock_window();
}


void Options::run()
{
	mwindow->theme->get_options_sizes();
	prefs->copy_from(mwindow->prefs);
	gui = new OptionsGUI(mwindow, this);
	gui->create_objects();
	int result = gui->run_window();
	delete gui;
	gui = 0;
	if(!result)
	{
		mwindow->save_db(0, 1);
		apply_changes();
	}
}



	
	
