#ifndef SEARCHGUI_H
#define SEARCHGUI_H

#include "guicast.h"
#include "mwindow.inc"
#include "search.inc"
#include "searchgui.inc"


class SearchText : public BC_TextBox
{
public:
	SearchText(MWindow *mwindow, int x, int y, int w);
	int handle_event();
	MWindow *mwindow;
};


class SearchField : public BC_CheckBox
{
public:
	SearchField(MWindow *mwindow, 
		int x, 
		int y, 
		int *output,
		char *text);
	int handle_event();
	MWindow *mwindow;
	int value;
	int *output;
};

class SearchCase : public BC_CheckBox
{
public:
	SearchCase(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class SearchBackward : public BC_CheckBox
{
public:
	SearchBackward(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class SearchOK : public BC_GenericButton
{
public:
	SearchOK(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

/*
 * class SearchAgain : public BC_GenericButton
 * {
 * public:
 * 	SearchAgain(MWindow *mwindow, int x, int y);
 * 	int handle_event();
 * 	MWindow *mwindow;
 * };
 * 
 */
class SearchCancel : public BC_GenericButton
{
public:
	SearchCancel(MWindow *mwindow, SearchGUI *gui, int x, int y);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	SearchGUI *gui;
};

class SearchGUI : public BC_Window
{
public:
	SearchGUI(MWindow *mwindow, Search *thread);
	void create_objects();
	int keypress_event();
	int close_event();

	MWindow *mwindow;
	Search *thread;
};


#endif
