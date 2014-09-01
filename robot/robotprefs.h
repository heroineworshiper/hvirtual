#ifndef ROBOTPREFS_H
#define ROBOTPREFS_H

#include "bcwindowbase.inc"
#include "mwindow.inc"
#include "robotdb.inc"

class RobotPrefs
{
public:
	RobotPrefs(MWindow *mwindow);

	void load_defaults();
	void save_defaults();
	static char* number_to_sort(int number);
	static int sort_to_number(char *text);
	void copy_from(RobotPrefs *prefs);
	void dump();

	int sort_order;
	int in_column_widths[FIELDS];
	int out_column_widths[FIELDS];

// Check in
	int check_in_src_row;
	int check_in_dst_tower;
	int check_in_dst_row;
	int check_in_orig_tower;
	int check_in_orig_row;


// Check out
	int check_out_src_tower;
	int check_out_src_row;
	int check_out_dst_row;

// Move
	int move_src_tower;
	int move_src_row;
	int move_dst_tower;
	int move_dst_row;

// Delete
	int delete_tower;
	int delete_row;

// Import
	int import_column_widths[2][FIELDS];
	char import_path[2][BCTEXTLEN];
	int import_check_in;
	int import_tower;
	int import_row;

// Searches
	char search_string[BCTEXTLEN];
	int search_fields[FIELDS];
	int search_case;
	int search_backward;

// Database path
	char db_path[BCTEXTLEN];

// Robot host
	char hostname[BCTEXTLEN];
	int port;

	MWindow *mwindow;
	enum
	{
		SORT_PATH,
		SORT_SIZE,
		SORT_DATE,
		SORT_NUMBER,
		SORT_DESC
	};
};



#endif
