#ifndef IMPORT_H
#define IMPORT_H


#include "guicast.h"
#include "import.inc"
#include "importgui.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "robotdb.inc"
#include "thread.h"


class Import : public Thread
{
public:
	Import(MWindow *mwindow);
	~Import();

	void start();
	void run();
	void import_cd(int number);
	void clear_cd(int number);

	MWindow *mwindow;
	ImportGUI *gui;
// Database of new pancake
	RobotDB *db[2];
	Mutex *startup_lock;
};


#endif
