#ifndef MWINDOW_H
#define MWINDOW_H

#include "checkin.inc"
#include "checkout.inc"
#include "defaults.inc"
#include "delete.inc"
#include "guicast.h"
#include "import.inc"
#include "manual.inc"
#include "move.inc"
#include "mwindowgui.inc"
#include "options.inc"
#include "robotalloc.inc"
#include "robotclient.inc"
#include "robotdb.inc"
#include "robotprefs.inc"
#include "robottheme.inc"
#include "search.inc"
#include "splashgui.inc"

class MWindow
{
public:
	MWindow();
	~MWindow();

	void create_objects();
	void reset();
	int test_lock();
	void erase_lock();
	void load_defaults();
	void save_defaults();

	void start_import();
	void import_row(RobotDB *db[], int lock_window);

	void start_checkin(int need_dst);
	void check_in_cd(int lock_window);
	
	void start_checkout(int need_src, int delete_next = 0);
	void check_out_cd(int lock_window, int delete_next);

	void start_delete();
	void delete_cd();

	void start_move();
	void move_cd();
	void manual_override();
	void do_sort();

	void start_search();
	void do_search(int use_lock);

	void abort_robot();
	void go_home();
	void start_options();

	void display_message(char *text, int color = BLACK);

	void load_db();
	void save_db(int lock_window, int use_message = 1);
	int send_command(int command,
		int src_row,
		int src_column,
		int dst_row,
		int dst_column,
		char *command_text,
		char *completion_text);
	void restart_client();

	void dump();


	MWindowGUI *gui;
	SplashGUI *splash_gui;
	Defaults *defaults;
	RobotTheme *theme;
	RobotDB *in_db;
	RobotDB *out_db;
	RobotPrefs *prefs;
	RobotAlloc *allocation;
	RobotClient *client;

	ManualOverride *manual;
	CheckOut *check_out;
	CheckIn *check_in;
	Import *import;
	Delete *delete_thread;
	Move *move_thread;
	Search *search;
	Options *options;
};


#endif
