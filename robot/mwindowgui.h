#ifndef MWINDOWGUI_H
#define MWINDOWGUI_H


#include "guicast.h"
#include "mwindow.h"


class MWindowGUI;



class InFileList : public BC_ListBox
{
public:
	InFileList(MWindow *mwindow, int x, int y, int w, int h);
	int column_resize_event();
	int handle_event();
	int selection_changed();
	MWindow *mwindow;
};

class OutFileList : public BC_ListBox
{
public:
	OutFileList(MWindow *mwindow, int x, int y, int w, int h);
	int column_resize_event();
	int handle_event();
	int selection_changed();
	MWindow *mwindow;
};

class Description : public BC_TextBox
{
public:
	Description(MWindow *mwindow, int x, int y, int w, int h);
	int handle_event();
	MWindow *mwindow;
};

class CheckOutButton : public BC_GenericButton
{
public:
	CheckOutButton(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class CheckInButton : public BC_GenericButton
{
public:
	CheckInButton(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class Sort : public BC_PopupMenu
{
public:
	Sort(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class ImportCD : public BC_GenericButton
{
public:
	ImportCD(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class DeleteButton : public BC_GenericButton
{
public:
	DeleteButton(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

/*
 * class MoveButton : public BC_GenericButton
 * {
 * public:
 * 	MoveButton(MWindow *mwindow, int x, int y);
 * 	int handle_event();
 * 	MWindow *mwindow;
 * };
 * 
 */
class SearchButton : public BC_GenericButton
{
public:
	SearchButton(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class ManualButton : public BC_GenericButton
{
public:
	ManualButton(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class SaveDB : public BC_GenericButton
{
public:
	SaveDB(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class SaveDesc : public BC_GenericButton
{
public:
	SaveDesc(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};


class OptionsButton : public BC_GenericButton
{
public:
	OptionsButton(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};

class AbortButton : public BC_Button
{
public:
	AbortButton(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};


class HomeButton : public BC_GenericButton
{
public:
	HomeButton(MWindow *mwindow, int x, int y);
	int handle_event();
	MWindow *mwindow;
};



class MWindowGUI : public BC_Window
{
public:
	MWindowGUI(MWindow *mwindow);
	~MWindowGUI();
	
	
	void create_objects();
	int close_event();
	int keypress_event();
	int resize_event(int w, int h);
	int translation_event();
	void draw_background();
	void update_description(int in_list, int out_list);
	void description_to_item();
	void update_in_list(int center_selection = 0);
	void update_out_list();

	InFileList *in_files;
	OutFileList *out_files;
	Description *cd_desc;
	CheckOutButton *check_out;
	CheckInButton *check_in;
	Sort *sort_cds;
	ImportCD *import_cd;
	DeleteButton *delete_cd;
//	MoveButton *move_cd;
	ManualButton *manual_override;
	SearchButton *search_cds;
	HomeButton *home_button;
	SaveDB *save_db;
	OptionsButton *options;
	AbortButton *abort;
	SaveDesc *save_desc;

	BC_Title *sort_title;
	BC_Title *in_title;
	BC_Title *out_title;
	BC_Title *desc_title;
	BC_Title *message;
	
	BC_Title *path_title;
	BC_Title *path_text;
	BC_Title *size_title;
	BC_Title *size_text;
	BC_Title *date_title;
	BC_Title *date_text;
	BC_Title *tower_title;
	BC_Title *tower_text;
	BC_Title *row_title;
	BC_Title *row_text;
	


// Last selection appearing in the description box
	int last_in, last_out;

	MWindow *mwindow;
};




#endif
