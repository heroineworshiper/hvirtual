#include "bcdisplayinfo.h"
#include "checkin.h"
#include "checkout.h"
#include "defaults.h"
#include "delete.h"
#include "filesystem.h"
#include "textfile.h"
#include "import.h"
#include "manual.h"
#include "move.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "options.h"
#include "robot.h"
#include "robotalloc.h"
#include "robotdb.h"
#include "robotclient.h"
#include "robotprefs.h"
#include "robottheme.h"
#include "search.h"
#include "splashgui.h"
#include "vframe.h"


#include <string.h>




int main(int argc, char *argv[])
{
	MWindow *mwindow = new MWindow;
	mwindow->create_objects();
	return 0;
}




MWindow::MWindow()
{
	reset();
}

MWindow::~MWindow()
{
}

void MWindow::reset()
{
	defaults = 0;
	in_db = 0;
	out_db = 0;
	splash_gui = 0;
}

int MWindow::test_lock()
{
	FILE *fd;
	if((fd = fopen(LOCK_PATH, "r")))
	{
		fclose(fd);
		printf(
"MWindow::test_lock: Lock file %s exists.  Another copy of %s is running.\n",
LOCK_PATH,
TITLE);
		return 1;
	}
	else
	{
		fd = fopen(LOCK_PATH, "w");
		if(!fd)
		{
			printf(
"MWindow::test_lock: error creating lock file %s.\n",
LOCK_PATH);
			return 1;
		}
		else
		{
			fputc(0, fd);
			fclose(fd);
			return 0;
		}
	}
}

void MWindow::erase_lock()
{
	remove(LOCK_PATH);
}

void MWindow::create_objects()
{
	if(test_lock()) return;

	load_defaults();

	prefs = new RobotPrefs(this);
	prefs->load_defaults();

	client = new RobotClient(this, prefs->hostname, prefs->port);
	client->create_objects();

	allocation = new RobotAlloc;
	allocation->create_objects();

	theme = new RobotTheme(this);
	theme->load_defaults();
	theme->initialize();

	BC_DisplayInfo display_info;
	splash_gui = new SplashGUI(this,
		theme->splash_logo, 
		display_info.get_root_w() / 2 - theme->splash_logo->get_w() / 2,
		display_info.get_root_h() / 2 - theme->splash_logo->get_h() / 2);
	splash_gui->create_objects();

	load_db();
//dump();

	check_out = new CheckOut(this);
	check_in = new CheckIn(this);
	import = new Import(this);
	delete_thread = new Delete(this);
	move_thread = new Move(this);
	search = new Search(this);
	options = new Options(this);
	manual = new ManualOverride(this);

	delete splash_gui;
	splash_gui = 0;

	gui = new MWindowGUI(this);
	gui->create_objects();
	gui->run_window();
//	save_db(0, 0);
	save_defaults();
	erase_lock();
}


void MWindow::load_defaults()
{
	if(!defaults)
	{
		char directory[BCTEXTLEN];
		FileSystem fs;
		sprintf(directory, "%s", ROBOTDIR);
		fs.complete_path(directory);
		if(fs.is_dir(directory))
		{
			fs.create_dir(directory);
		}

		strcat(directory, "/heroine1120.rc");
		defaults = new Defaults(directory);
		defaults->load();
	}
}


void MWindow::save_defaults()
{
	if(defaults)
	{
		prefs->save_defaults();
		theme->save_defaults();
		defaults->save();
	}
}

void MWindow::load_db()
{
	if(in_db) delete in_db;
	if(out_db) delete out_db;

	in_db = new RobotDB(this);
	out_db = new RobotDB(this);

// Load here
	FileSystem fs;
	fs.complete_path(prefs->db_path);
	TextFile *file = new TextFile(prefs->db_path, 1, 0);
	int result = file->read_db();
	int current_db = 0;

	while(!result)
	{
		result = file->read_record();
		if(!result)
		{
// Tower layout
			if(file->type_is("LAYT"))
			{
				allocation->read_layout(file);
			}
			else
// Database records
			if(file->type_is("TABL"))
			{
				if(current_db == 0)
					in_db->load(file);
				else
					out_db->load(file);
				current_db++;
			}
		}
	}

	delete file;

// Set allocation
	in_db->set_allocation();
}

void MWindow::save_db(int lock_window, int use_message)
{
	if(lock_window) gui->lock_window();
	FileSystem fs;
	fs.complete_path(prefs->db_path);

// Make backup
	char backup_path[BCTEXTLEN];
	sprintf(backup_path, "%s.bak", prefs->db_path);
	rename(prefs->db_path, backup_path);

// Write data
	TextFile *file = new TextFile(prefs->db_path, 0, 1);
	int result = 0;

// Layout
	allocation->write_layout(file);

// In records
	in_db->save(file);

// Out records
	out_db->save(file);

	result = file->write_db();


// Restore backup
	if(result)
	{
		char string[BCTEXTLEN];
		sprintf(string, "Error saving %s.\n", 
			prefs->db_path);
		rename(backup_path, prefs->db_path);
		if(use_message) display_message(string, RED);
	}
	else
	{
		char string[BCTEXTLEN];
		sprintf(string, "\"%s\" %dC written\n",
			prefs->db_path,
			file->get_size());
		if(use_message) display_message(string);
	}

	delete file;
	if(lock_window) gui->unlock_window();
}

void MWindow::start_import()
{
	import->start();
}

void MWindow::import_row(RobotDB *db[], int lock_window)
{
// Set tower and row in DB
	if(lock_window) gui->lock_window();


	db[0]->set_tower_row(prefs->import_tower, prefs->import_row);
	db[1]->set_tower_row(prefs->import_tower, prefs->import_row);

// Allocate position in tables
	allocation->new_tower_row(prefs->import_tower, prefs->import_row, 1);


// Add to DB
	in_db->append_db(db[0]);
	in_db->append_db(db[1]);
	out_db->deselect_all();
	out_db->append_db(db[0], -1, -1, 1);
	out_db->append_db(db[1], -1, -1);
	gui->update_in_list();
	gui->update_out_list();
	gui->update_description(0, 1);
	save_db(0);




// Call start_checkin if desired
	if(prefs->import_check_in)
	{
		start_checkin(1);
	}




	if(lock_window) gui->unlock_window();




}

void MWindow::start_checkin(int need_dst)
{
// Get dst row from src list
	if(need_dst)
	{
		BC_ListBoxItem *row_item = 
			gui->out_files->get_selection(RobotDB::ROW, 0);
		BC_ListBoxItem *tower_item = 
			gui->out_files->get_selection(RobotDB::TOWER, 0);

		if(row_item && tower_item)
		{
			prefs->check_in_dst_row = 
				prefs->check_in_orig_row = 
				atol(row_item->get_text());
			prefs->check_in_dst_tower = 
				prefs->check_in_orig_tower = 
				atol(tower_item->get_text());
		}
		else
		{
			display_message("Nothing to check in.", RED);
			return;
		}
	}

	check_in->start();
}

void MWindow::check_in_cd(int lock_window)
{
	if(lock_window) gui->lock_window();

// Command robot
	char string1[BCTEXTLEN];
	char string2[BCTEXTLEN];

	sprintf(string1, 
		"Checking pancake into tower %d row %d.\n",
		prefs->check_in_dst_tower,
		prefs->check_in_dst_row);
	sprintf(string2, 
		"Done checking into tower %d row %d.\n",
		prefs->check_in_dst_tower,
		prefs->check_in_dst_row);

	int result = send_command(MOVE_CD,
		prefs->check_in_src_row,
		allocation->get_loading_tower(),
		prefs->check_in_dst_row,
		prefs->check_in_dst_tower,
		string1,
		string2);

	if(result)
	{
		if(lock_window) gui->unlock_window();
		return;
	}

// Remove disk from checkout db.
	out_db->remove_cd(prefs->check_in_orig_row,
		prefs->check_in_orig_tower);

// Location of disk changed
	if(prefs->check_in_dst_row != prefs->check_in_orig_row ||
		prefs->check_in_dst_tower != prefs->check_in_orig_tower)
	{
		in_db->move_cd(prefs->check_in_orig_row,
			prefs->check_in_orig_tower,
			prefs->check_in_dst_row,
			prefs->check_in_dst_tower);
	}
	save_db(0, 0);

	gui->update_in_list();
	gui->update_out_list();
	if(lock_window) gui->unlock_window();

}




void MWindow::start_checkout(int need_src, int delete_next)
{
	if(need_src)
	{
		BC_ListBoxItem *row_item = gui->in_files->get_selection(RobotDB::ROW, 0);
		BC_ListBoxItem *tower_item = gui->in_files->get_selection(RobotDB::TOWER, 0);

		if(row_item && tower_item)
		{
			prefs->check_out_src_row = atol(row_item->get_text());
			prefs->check_out_src_tower = atol(tower_item->get_text());
		}
		else
		{
			display_message("Nothing to check out.", RED);
			return;
		}
	}

// Test if item is already checked out
	if(out_db->exists(prefs->check_out_src_row, prefs->check_out_src_tower))
	{
		display_message("Already checked out.", RED);
		return;
	}



	check_out->start(delete_next);
}

void MWindow::check_out_cd(int lock_window, int delete_next)
{
	if(lock_window) gui->lock_window();

// Test if item is already checked out because it may have changed 
	if(out_db->exists(prefs->check_out_src_row, prefs->check_out_src_tower))
	{
		display_message("Already checked out.", RED);
		return;
	}

// Command robot
	char string1[BCTEXTLEN];
	char string2[BCTEXTLEN];

	sprintf(string1, 
		"Checking pancake out of tower %d row %d.\n",
		prefs->check_out_src_tower,
		prefs->check_out_src_row);
	sprintf(string2, 
		"Done checking out of tower %d row %d.\n",
		prefs->check_out_src_tower,
		prefs->check_out_src_row);
	int result = send_command(MOVE_CD,
		prefs->check_out_src_row,
		prefs->check_out_src_tower,
		prefs->check_out_dst_row,
		allocation->get_loading_tower(),
		string1,
		string2);

	if(result)
	{
		if(lock_window) gui->unlock_window();
		return;
	}

// Copy CD to checkout db.
	if(!delete_next)
		out_db->append_db(in_db, 
			prefs->check_out_src_row, 
			prefs->check_out_src_tower);
	else
// Delete CD from checkin db.
		in_db->remove_cd(prefs->delete_row,
			prefs->delete_tower);
	gui->update_in_list();
	gui->update_out_list();


	save_db(0, 0);

	if(lock_window) gui->unlock_window();


}

void MWindow::start_delete()
{
	BC_ListBoxItem *row_item;
	BC_ListBoxItem *tower_item;

// Not in in_files
	if(gui->last_in)
	{
		row_item = gui->in_files->get_selection(RobotDB::ROW, 0);
		tower_item = gui->in_files->get_selection(RobotDB::TOWER, 0);
	}
	else
	{
		row_item = gui->out_files->get_selection(RobotDB::ROW, 0);
		tower_item = gui->out_files->get_selection(RobotDB::TOWER, 0);
	}


	if(row_item && tower_item)
	{
		prefs->check_out_src_row = 
			prefs->delete_row = 
			atol(row_item->get_text());
		prefs->check_out_src_tower = 
			prefs->delete_tower = 
			atol(tower_item->get_text());
	}
	else
	{
		display_message("Nothing to delete.", RED);
		return;
	}

	delete_thread->start();
}

void MWindow::delete_cd()
{
// If the item isn't checked out, check it out before deleting it
	if(!out_db->exists(prefs->delete_row,
			prefs->delete_tower))
	{
		start_checkout(0, 1);
	}
	else
// Delete it now
	{
		in_db->remove_cd(prefs->delete_row,
			prefs->delete_tower);
		out_db->remove_cd(prefs->delete_row,
			prefs->delete_tower);
		save_db(0, 0);
	}
	gui->update_in_list();
	gui->update_out_list();
}

void MWindow::abort_robot()
{
	int result = client->send_abort();
	if(result)
		display_message("Error contacting robot.", RED);
	else
		display_message("Operation aborted.");
}


void MWindow::go_home()
{
	int result = send_command(GOTO_COLUMN,
		0,
		0,
		0,
//		allocation->get_loading_tower(),
		1,
		"Returning home.",
		"Done returning home.");
	if(!result)
		display_message("Moving robot home.");
}

void MWindow::start_search()
{
	search->start();
}

void MWindow::do_search(int use_lock)
{
// Reset position
	if(use_lock) gui->lock_window();

	save_defaults();
	int attempt = 0;
	int got_it = 0;
	int search_position = gui->in_files->get_selection_number(0, 0);

// Nothing selected
	if(search_position < 0)
	{
		if(!prefs->search_backward)
			search_position = BC_ListBox::get_total_items(in_db->data, 0, 0);
		else
			search_position = -1;
	}

//prefs->dump();

// Start new search
	while(attempt < 2 && !got_it)
	{
// Search wrapped
		if(attempt == 1)
		{
			if(prefs->search_backward)
				search_position = BC_ListBox::get_total_items(in_db->data, 0, 0);
			else
				search_position = -1;
		}

// Get new item
		int old_item = search_position;
		search_position = in_db->do_search(search_position);

//printf("MWindow::do_search 20 %d %d\n", old_item, prefs->search_position);
// Select item in in_db and deselect everything else
		if(old_item != search_position)
		{
			in_db->deselect_all();
			BC_ListBox::collapse_recursive(in_db->data, 0);
			in_db->select_and_focus(search_position);
			gui->update_in_list(1);
			gui->update_description(1, 0);
			got_it = 1;
		}
		if(use_lock) gui->unlock_window();
		attempt++;
	}
}

void MWindow::manual_override()
{
	manual->start();
}

void MWindow::do_sort()
{

	in_db->sort();
	out_db->sort();
	gui->update_in_list();
	gui->update_out_list();
// Don't save DB since we didn't change any data.

}





void MWindow::display_message(char *text, int color)
{
	gui->message->set_color(color);
	gui->message->update(text);
}


void MWindow::start_move()
{
	BC_ListBoxItem *row_item = gui->in_files->get_selection(RobotDB::ROW, 0);
	BC_ListBoxItem *tower_item = gui->in_files->get_selection(RobotDB::TOWER, 0);

	if(row_item && tower_item)
	{
		prefs->move_dst_row = prefs->move_src_row = atol(row_item->get_text());
		prefs->move_dst_tower = prefs->move_src_tower = atol(tower_item->get_text());
	}
	else
	{
		display_message("Nothing to move.", RED);
		return;
	}
	move_thread->start();
}

void MWindow::move_cd()
{
	gui->lock_window();
	if(prefs->move_dst_row == prefs->move_src_row &&
		prefs->move_dst_tower == prefs->move_src_tower)
	{
		display_message("Destination is same as source.", RED);
	}
	else
	{
// Command robot
		int result = send_command(MOVE_CD,
			prefs->move_src_row,
			prefs->move_src_tower,
			prefs->move_dst_row,
			prefs->move_dst_tower,
			"Moving pancake.",
			"Done moving.");

// Update DB
		if(!result)
		{
			in_db->move_cd(prefs->move_src_row,
				prefs->move_src_tower,
				prefs->move_dst_row,
				prefs->move_dst_tower);
			out_db->move_cd(prefs->move_src_row,
				prefs->move_src_tower,
				prefs->move_dst_row,
				prefs->move_dst_tower);

// Fix allocation
			allocation->free_tower_row(prefs->move_src_tower,
				prefs->move_src_row);
			allocation->new_tower_row(prefs->move_dst_tower,
				prefs->move_dst_row, 0);

			gui->update_in_list();
			gui->update_out_list();
			save_db(0, 0);
		}
	}
	gui->unlock_window();
}

int MWindow::send_command(int command,
		int src_row,
		int src_column,
		int dst_row,
		int dst_column,
		char *command_text,
		char *completion_text)
{
	int result = 0;
	if(command == ABORT_OPERATION)
	{
		result = client->send_abort();
	}
	else
	if(client->command_running)
	{
		display_message("Robot is busy.", RED);
		result = 1;
	}
	else
	{
		result = client->send_command(command,
			src_row,
			src_column,
			dst_row,
			dst_column,
			command_text,
			completion_text);
	}
	return result;
}

void MWindow::restart_client()
{
	delete client;
	client = new RobotClient(this, prefs->hostname, prefs->port);
	client->create_objects();
}

void MWindow::start_options()
{
	options->start();
}

void MWindow::dump()
{
	allocation->dump();
}







