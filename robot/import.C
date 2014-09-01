#include "import.h"
#include "importgui.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "robotalloc.h"
#include "robotdb.h"
#include "robotprefs.h"
#include "robottheme.h"

#include <string.h>
























Import::Import(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	gui = 0;
	startup_lock = new Mutex;
}

Import::~Import()
{
	delete startup_lock;
}

void Import::start()
{
	if(Thread::running())
	{
		if(gui)
		{
			gui->lock_window();
			gui->raise_window();
			gui->flush();
			gui->unlock_window();
		}
		return;
	}

// Unlock window here to allow thread to draw error message
	mwindow->gui->unlock_window();
	startup_lock->lock();
	Thread::start();
	startup_lock->lock();
	startup_lock->unlock();
	mwindow->gui->lock_window();
}

void Import::run()
{

// Get a default tower and row
	int result = mwindow->allocation->get_tower_row(&mwindow->prefs->import_tower, 
		&mwindow->prefs->import_row);

	if(!result)
	{
// Create temporary DB's for the pancake
		db[0] = new RobotDB(mwindow);
		db[1] = new RobotDB(mwindow);

// Read backup copies of DB's
		db[0]->load(ROBOTDIR "/import0.db");
		db[1]->load(ROBOTDIR "/import1.db");



		mwindow->theme->get_import_sizes();
		gui = new ImportGUI(mwindow, this);
		gui->create_objects();
		startup_lock->unlock();
		result = gui->run_window();

		startup_lock->lock();
		delete gui;
		gui = 0;
		startup_lock->unlock();

		if(!result)
		{
			mwindow->import_row(db, 1);

// Clear import db's
			db[0]->clear();
			db[1]->clear();
		}

// Save backup copies of DB's
		db[0]->save(ROBOTDIR "/import0.db");
		db[1]->save(ROBOTDIR "/import1.db");

		delete db[0];
		delete db[1];
	}
	else
	{
		mwindow->gui->lock_window();
		mwindow->display_message("Robot is full.", RED);
		mwindow->gui->unlock_window();
		startup_lock->unlock();
	}
}

void Import::import_cd(int number)
{
	db[number]->clear();
	db[number]->import_cd(mwindow->prefs->import_path[number]);
	gui->update_list(number);
}

void Import::clear_cd(int number)
{
	db[number]->clear();
	gui->update_list(number);
}






