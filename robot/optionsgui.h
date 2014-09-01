#ifndef OPTIONSGUI_H
#define OPTIONSGUI_H

#include "filegui.h"
#include "guicast.h"
#include "mwindow.inc"
#include "options.inc"
#include "optionsgui.inc"

class OptionsPath : public FileGUI
{
public:
	OptionsPath(MWindow *mwindow, Options *thread, int x, int y, int w);
	int handle_event();
	MWindow *mwindow;
	Options *thread;
};

class OptionsHost : public BC_TextBox
{
public:
	OptionsHost(MWindow *mwindow, Options *thread, int x, int y, int w);
	int handle_event();
	MWindow *mwindow;
	Options *thread;
};

class OptionsPort : public BC_TumbleTextBox
{
public:
	OptionsPort(MWindow *mwindow, Options *thread, int x, int y, int w);
	int handle_event();
	MWindow *mwindow;
	Options *thread;
};

class OptionsGUI : public BC_Window
{
public:
	OptionsGUI(MWindow *mwindow, Options *thread);
	~OptionsGUI();
	
	void create_objects();
	int keypress_event();
	int close_event();


	MWindow *mwindow;
	OptionsPath *path;
	Options *thread;
};



#endif
