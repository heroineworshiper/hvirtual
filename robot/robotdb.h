#ifndef ROBOTDB_H
#define ROBOTDB_H

#include "guicast.h"
#include "mwindow.inc"
#include "robotdb.inc"
#include "textfile.inc"

class RobotDB
{
public:
	RobotDB(MWindow *mwindow);
	~RobotDB();

// For use from MWindow
	int load(TextFile *file);
	int save(TextFile *file);

// For use from Import
	int load(char *path);
	int save(char *path);

	void clear();

	int save_row(TextFile *file, 
		ArrayList<BC_ListBoxItem*> *data, 
		int number);
	int save_entry(TextFile *file, 
		ArrayList<BC_ListBoxItem*> *data, 
		int number);
	int load_row(TextFile *file, 
		ArrayList<BC_ListBoxItem*> *data, 
		int is_dir);
	int load_directory(TextFile *file, 
		ArrayList<BC_ListBoxItem*> *data,
		int total);
	int load_entry(TextFile *file, 
		ArrayList<BC_ListBoxItem*> *data, 
		int fields,
		int is_dir);

// Return the sublist which contains last_item in its path column.
	ArrayList<BC_ListBoxItem*>* get_sublist(ArrayList<BC_ListBoxItem*> *parent_sublist, 
		BC_ListBoxItem *last_item);
// Create new entry.  Return the first BC_ListBoxItem in the row.
	BC_ListBoxItem* new_entry(char *path, 
		int64_t size, 
		int64_t date,
		int tower,
		int row,
		char *desc,
		int is_directory,
		ArrayList<BC_ListBoxItem*> *sublist);  

	void import_dir(char *path, 
		BC_ListBoxItem *item, 
		int tower, 
		int row);
	void import_cd(char *path);

	void remove_cd(int row, int tower, ArrayList<BC_ListBoxItem*> *data = 0);
	void move_cd(int row1, 
		int tower1, 
		int row2, 
		int tower2, 
		ArrayList<BC_ListBoxItem*> *data = 0);
	int exists(int row, int tower);

// Return the absolute position of the next search match.
	int do_search(int search_position, int *counter = 0, 
		ArrayList<BC_ListBoxItem*> *data = 0);

// Called by do_search to perform comparison
	int search_matches(ArrayList<BC_ListBoxItem*> *data,
		int number);

// Called by MWindow::do_search to expand the item found
	int RobotDB::select_and_focus(int number,
		int *counter = 0, 
		ArrayList<BC_ListBoxItem*> *data = 0);

// Copy a field from one item in one DB to the same item in the this
// DB.
// return - 1 if destination item exists
//          0 if destination doesn't exist
	int copy_field(ArrayList<BC_ListBoxItem*> *src_data,
		BC_ListBoxItem *src_item,
		int field);

// Get the tower and row of the item.
// Returns 1 if it got it.
	int get_location(BC_ListBoxItem *item, 
		int field,
		int *tower,
		int *row,
		ArrayList<BC_ListBoxItem*> *data);
// Get the position in the pancake
	int get_pancake_location(BC_ListBoxItem *item,
		int field,
		int tower,
		int row,
		int *pancake_number,
		ArrayList<BC_ListBoxItem*> *data,
		int *counter = 0);
// Get the item
	BC_ListBoxItem* get_item(int field,
				int tower,
				int row,
				int number,
				ArrayList<BC_ListBoxItem*> *data = 0,
				int *counter = 0);

// Set allocation table usage by current DB.
	void set_allocation();

	void set_tower_row(int tower, 
		int row, 
		ArrayList<BC_ListBoxItem*> *data = 0);

// Set selection status of first item
	void set_selected(int value);

// Deselect all items
	void deselect_all(ArrayList<BC_ListBoxItem*> *data = 0);

// Sort DB
	void sort(ArrayList<BC_ListBoxItem*> *data = 0);
// Used by sort
	int compare_items(ArrayList<BC_ListBoxItem*> *data, int number);

	void append_db(RobotDB *db, 
		int row = -1, 
		int tower = -1, 
		int select_first = 0);
	static char* field_to_fourcc(int field);
	static int fourcc_to_field(char *fourcc);

	ArrayList<BC_ListBoxItem*> data[FIELDS];
	static char *column_titles[FIELDS];
	MWindow *mwindow;

	enum
	{
		PATH,
		DESC,
		SIZE,
		DATE,
		TOWER,
		ROW,
	};
};



#endif

