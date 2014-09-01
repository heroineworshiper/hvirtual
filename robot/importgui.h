#ifndef IMPORTGUI_H
#define IMPORTGUI_H

#include "allocationgui.h"
#include "filegui.h"
#include "guicast.h"
#include "importgui.inc"

class ImportPath : public FileGUI
{
public:
	ImportPath(MWindow *mwindow,
		Import *import,
		int x,
		int y,
		int w,
		int number);
	int handle_event();
	int number;
	Import *import;
};

class ImportFileList : public BC_ListBox
{
public:
	ImportFileList(MWindow *mwindow, 
		Import *import, 
		int x, 
		int y, 
		int w, 
		int h, 
		int number);
	int handle_event();
	int column_resize_event();
	MWindow *mwindow;
	Import *import;
	int number;
};

class ImportPathImport : public BC_GenericButton
{
public:
	ImportPathImport(MWindow *mwindow, Import *import, int x, int y, int number);
	int handle_event();
	MWindow *mwindow;
	Import *import;
	int number;
};

class ImportPathClear : public BC_GenericButton
{
public:
	ImportPathClear(MWindow *mwindow, Import *import, int x, int y, int number);
	int handle_event();
	MWindow *mwindow;
	Import *import;
	int number;
};

class ImportDest : public AllocationGUI
{
public:
	ImportDest(MWindow *mwindow, 
		Import *import,
		int x, 
		int y,
		int w,
		int h);
	int handle_event();
	MWindow *mwindow;
	Import *import;
};

class ImportCheckIn : public BC_CheckBox
{
public:
	ImportCheckIn(MWindow *mwindow,
		int x, 
		int y);
	int handle_event();
	MWindow *mwindow;
};

class ImportOK : public BC_OKButton
{
public:
	ImportOK(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class ImportCancel : public BC_CancelButton
{
public:
	ImportCancel(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class ImportGUI : public BC_Window
{
public:
	ImportGUI(MWindow *mwindow, Import *thread);
	~ImportGUI();

	void create_objects();
	int keypress_event();
	int close_event();
	void update_list(int number);
	int resize_event(int w, int h);

	MWindow *mwindow;
	Import *thread;

	ImportPath *path[2];
	BC_Title *browse_text[2];
	ImportPathImport *path_import[2];
	ImportPathClear *path_clear[2];
	ImportFileList *list[2];
	BC_Title *instructions;
	BC_Title *list_text[2];
	BC_Title *dest_title;
	ImportDest *destination;
	ImportOK *ok;
	ImportCancel *cancel;
	ImportCheckIn *check_in;
};


#endif
