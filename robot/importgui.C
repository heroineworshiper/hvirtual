#include "import.h"
#include "importgui.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "robotdb.h"
#include "robotprefs.h"
#include "robottheme.h"


#include <string.h>






ImportPath::ImportPath(MWindow *mwindow,
	Import *import,
	int x,
	int y,
	int w,
	int number)
 : FileGUI(mwindow,
 	import->gui,
	x, 
	y, 
	w, 
	mwindow->prefs->import_path[number],
	1,
	"Import directory",
	"Select the mount point to import from.",
	0)
{
	this->mwindow = mwindow;
	this->number = number;
	this->import = import;
}

int ImportPath::handle_event()
{
	strcpy(mwindow->prefs->import_path[number], path);
	return 1;
}







ImportFileList::ImportFileList(MWindow *mwindow, 
	Import *import,
	int x, 
	int y, 
	int w, 
	int h, 
	int number)
 : BC_ListBox(x, 
		y, 
		w, 
		h,
		LISTBOX_TEXT,                   // Display text list or icons
		import->db[number]->data, // Each column has an ArrayList of BC_ListBoxItems.
		RobotDB::column_titles,             // Titles for columns.  Set to 0 for no titles
		mwindow->prefs->import_column_widths[number],                // width of each column
		FIELDS,                      // Total columns.  Only 1 in icon mode
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_SINGLE,  // Select one item or multiple items
		ICON_LEFT,        // Position of icon relative to text of each item
		1)
{
	this->mwindow = mwindow;
	this->number = number;
	this->import = import;
}

int ImportFileList::handle_event()
{
	return 1;
}

int ImportFileList::column_resize_event()
{
	for(int i = 0; i < FIELDS; i++)
		mwindow->prefs->import_column_widths[number][i] = get_column_width(i);
	mwindow->save_defaults();
	return 1;
}






ImportPathImport::ImportPathImport(MWindow *mwindow, 
	Import *import, 
	int x, 
	int y, 
	int number)
 : BC_GenericButton(x, y, "Import")
{
	this->mwindow = mwindow;
	this->import = import;
	this->number = number;
}

int ImportPathImport::handle_event()
{
	import->import_cd(number);
	return 1;
}








ImportPathClear::ImportPathClear(MWindow *mwindow, Import *import, int x, int y, int number)
 : BC_GenericButton(x, y, "Clear")
{
	this->mwindow = mwindow;
	this->import = import;
	this->number = number;
}


int ImportPathClear::handle_event()
{
	import->clear_cd(number);
	return 1;
}









ImportDest::ImportDest(MWindow *mwindow, 
	Import *import,
	int x, 
	int y, 
	int w, 
	int h)
 : AllocationGUI(mwindow, 
 	import->gui,
	x, 
	y, 
	w,
	h,
	mwindow->prefs->import_tower, 
	mwindow->prefs->import_row,
	1,
	1,
	0)
{
	this->mwindow = mwindow;
}

int ImportDest::handle_event()
{
	mwindow->prefs->import_tower = tower;
	mwindow->prefs->import_row = row;
	return 1;
}









ImportOK::ImportOK(MWindow *mwindow, int x, int y)
 : BC_OKButton(x, y)
{
	this->mwindow = mwindow;
}

int ImportOK::handle_event()
{
	set_done(0);
	return 1;
}







ImportCancel::ImportCancel(MWindow *mwindow, int x, int y)
 : BC_CancelButton(x, y)
{
	this->mwindow = mwindow;
}

int ImportCancel::handle_event()
{
	set_done(1);
	return 1;
}








ImportCheckIn::ImportCheckIn(MWindow *mwindow,
	int x, 
	int y)
 : BC_CheckBox(x, y, mwindow->prefs->import_check_in, "Check in next")
{
	this->mwindow = mwindow;
}
int ImportCheckIn::handle_event()
{
	mwindow->prefs->import_check_in = get_value();
	return 1;
}









ImportGUI::ImportGUI(MWindow *mwindow, Import *thread)
 : BC_Window(TITLE ": Import Row", 
	mwindow->theme->mwindow_x + 
		mwindow->theme->mwindow_w / 2 - 
		mwindow->theme->import_w / 2,
	mwindow->theme->mwindow_y +
		mwindow->theme->mwindow_h / 2 - 
		mwindow->theme->import_h / 2,
	mwindow->theme->import_w, 
	mwindow->theme->import_h, 
	10,
	10, 
	1,
	0, 
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

ImportGUI::~ImportGUI()
{
	delete path[0];
	delete path[1];
}

void ImportGUI::create_objects()
{
	int x = 10;
	int y = 10;


	add_subwindow(instructions = 
		new BC_Title(x, y, "Import 2 CD's for the new row."));




	x = mwindow->theme->import_browse1_x;
	y = mwindow->theme->import_browse1_y;
	add_subwindow(browse_text[0] = new BC_Title(x, y, "Disk 1 mount point:"));
	y += 20;
	path[0] = new ImportPath(mwindow, 
		thread,
		x, 
		y, 
		mwindow->theme->import_browse1_w, 
		0);
	path[0]->create_objects();
	add_subwindow(path_import[0] = new ImportPathImport(mwindow, 
		thread,
		x + mwindow->theme->import_browse1_w, 
		y, 
		0));
	add_subwindow(path_clear[0] = new ImportPathClear(mwindow, 
		thread, 
		x + mwindow->theme->import_browse1_w + path_import[0]->get_w() + 10, 
		y, 
		0));
	add_subwindow(list[0] = new ImportFileList(mwindow, 
		thread, 
		mwindow->theme->import_list1_x, 
		mwindow->theme->import_list1_y, 
		mwindow->theme->import_list1_w,
		mwindow->theme->import_list1_h,
		0));



	x = mwindow->theme->import_browse2_x;
	y = mwindow->theme->import_browse2_y;
	add_subwindow(browse_text[1] = new BC_Title(x, y, "Disk 2 mount point:"));
	y += 20;
	path[1] = new ImportPath(mwindow, 
		thread,
		x, 
		y, 
		mwindow->theme->import_browse2_w,
		1);
	path[1]->create_objects();
	add_subwindow(path_import[1] = new ImportPathImport(mwindow, 
		thread, 
		x + mwindow->theme->import_browse2_w, 
		y, 
		1));
	add_subwindow(path_clear[1] = new ImportPathClear(mwindow, 
		thread, 
		x + mwindow->theme->import_browse2_w + path_import[1]->get_w() + 10, 
		y, 
		1));
	add_subwindow(list[1] = new ImportFileList(mwindow, 
		thread, 
		mwindow->theme->import_list2_x, 
		mwindow->theme->import_list2_y, 
		mwindow->theme->import_list2_w,
		mwindow->theme->import_list2_h,
		1));


	add_subwindow(dest_title = new BC_Title(mwindow->theme->import_dest_x,
		mwindow->theme->import_dest_y - 20,
		"Destination:"));
	destination = new ImportDest(mwindow, 
		thread,
		mwindow->theme->import_dest_x, 
		mwindow->theme->import_dest_y, 
		mwindow->theme->import_dest_w, 
		mwindow->theme->import_dest_h);
	destination->create_objects();


	add_subwindow(ok = new ImportOK(mwindow, 
		mwindow->theme->import_ok_x, 
		mwindow->theme->import_ok_y));
	x = get_w() - 105;
	add_subwindow(cancel = new ImportCancel(mwindow, 
		mwindow->theme->import_cancel_x, 
		mwindow->theme->import_cancel_y));
	
	add_subwindow(check_in = new ImportCheckIn(mwindow,
		mwindow->theme->import_checkin_x, 
		mwindow->theme->import_checkin_y));

	show_window();
	flush();
}

int ImportGUI::resize_event(int w, int h)
{
	int x = 10;
	int y = 10;

	mwindow->theme->import_w = w;
	mwindow->theme->import_h = h;
	mwindow->theme->get_import_sizes();

	instructions->reposition_window(x, y);
	x = mwindow->theme->import_browse1_x;
	y = mwindow->theme->import_browse1_y;
	browse_text[0]->reposition_window(x, y);
	y += 20;
	path[0]->reposition(x, y, mwindow->theme->import_browse1_w);
	path_import[0]->reposition_window(x + mwindow->theme->import_browse1_w, y);
	path_clear[0]->reposition_window(x + mwindow->theme->import_browse1_w + path_import[0]->get_w() + 10, y);
	list[0]->reposition_window(mwindow->theme->import_list1_x, 
		mwindow->theme->import_list1_y, 
		mwindow->theme->import_list1_w,
		mwindow->theme->import_list1_h);



	x = mwindow->theme->import_browse2_x;
	y = mwindow->theme->import_browse2_y;
	browse_text[1]->reposition_window(x, y);
	y += 20;
	path[1]->reposition(x, y, mwindow->theme->import_browse2_w);
	path_import[1]->reposition_window(x + mwindow->theme->import_browse2_w, y);
	path_clear[1]->reposition_window(x + mwindow->theme->import_browse2_w + path_import[1]->get_w() + 10, y);
	list[1]->reposition_window(mwindow->theme->import_list2_x, 
		mwindow->theme->import_list2_y, 
		mwindow->theme->import_list2_w,
		mwindow->theme->import_list2_h);

	dest_title->reposition_window(mwindow->theme->import_dest_x,
		mwindow->theme->import_dest_y - 20);
	destination->reposition(mwindow->theme->import_dest_x, 
		mwindow->theme->import_dest_y, 
		mwindow->theme->import_dest_w, 
		mwindow->theme->import_dest_h);

	ok->reposition_window(mwindow->theme->import_ok_x, 
		mwindow->theme->import_ok_y);
	cancel->reposition_window(mwindow->theme->import_cancel_x, 
		mwindow->theme->import_cancel_y);
	check_in->reposition_window(mwindow->theme->import_checkin_x, 
		mwindow->theme->import_checkin_y);

	return 1;
}

int ImportGUI::keypress_event()
{
	switch(get_keypress())
	{
		case 'w':
			set_done(1);
			return 1;
			break;
	}
	return 0;
}

int ImportGUI::close_event()
{
	set_done(1);
	return 1;
}

void ImportGUI::update_list(int number)
{
	list[number]->update(thread->db[number]->data,
		RobotDB::column_titles,
		mwindow->prefs->import_column_widths[number],
		FIELDS,
		0,
		0,
		0,
		1);
}



