#ifndef OPTIONS_H
#define OPTIONS_H


#include "mwindow.inc"
#include "optionsgui.inc"
#include "robotprefs.inc"
#include "thread.h"


class Options : public Thread
{
public:
	Options(MWindow *mwindow);
	~Options();
	
	void start();
	void run();
	void apply_changes();

	MWindow *mwindow;
	OptionsGUI *gui;
	RobotPrefs *prefs;
};



#endif
