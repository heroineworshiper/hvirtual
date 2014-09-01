#ifndef FILEGUI_H
#define FILEGUI_H


#include "filegui.inc"
#include "filethread.inc"
#include "guicast.h"
#include "mwindow.inc"


class FileText : public BC_TextBox
{
public:
	FileText(MWindow *mwindow,
		FileGUI *gui,
		int x,
		int y,
		int w,
		char *text);
	~FileText();
	int handle_event();
	FileGUI *gui;
	MWindow *mwindow;
};

class FileBrowse : public BC_Button
{
public:
	FileBrowse(MWindow *mwindow,
		FileGUI *gui,
		int x,
		int y);
	~FileBrowse();
	int handle_event();
	FileGUI *gui;
	MWindow *mwindow;
};

class FileGUI
{
public:
	FileGUI(MWindow *mwindow,
		BC_WindowBase *parent_window,
		int x, 
		int y,
		int w,
		char *path,
		int want_directory,
		char *title,
		char *caption,
		int show_hidden);
	~FileGUI();


	void create_objects();
	void reposition(int x, int y, int w);
	void update_textbox();
	void update(char *path);
	virtual int handle_event();

	MWindow *mwindow;
	BC_WindowBase *parent_window;
	FileThread *thread;
	char path[BCTEXTLEN];
	char title[BCTEXTLEN];
	char caption[BCTEXTLEN];
	FileText *text;
	FileBrowse *browse;
	int x;
	int y;
	int w;
	int want_directory;
	int show_hidden;
};

#endif
