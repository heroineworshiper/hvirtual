#include "checkin.h"
#include "checkout.h"
#include "delete.h"
#include "fonts.h"
#include "import.h"
#include "move.h"
#include "mwindowgui.h"
#include "options.h"
#include "robotdb.h"
#include "robotprefs.h"
#include "robottheme.h"
#include "search.h"







MWindowGUI::MWindowGUI(MWindow *mwindow)
 : BC_Window(TITLE ": Command Center", 
	mwindow->theme->mwindow_x,
	mwindow->theme->mwindow_y,
	mwindow->theme->mwindow_w, 
	mwindow->theme->mwindow_h, 
	514, 
	615, 
	1,
	0, 
	1)
{
	this->mwindow = mwindow;
	last_in = last_out = 0;
}

MWindowGUI::~MWindowGUI()
{
}

void MWindowGUI::create_objects()
{
	mwindow->theme->get_mwindow_sizes();

	draw_background();
	add_subwindow(in_title = new BC_Title(mwindow->theme->in_files_x, 
		mwindow->theme->in_files_y - 20,
		"Checked in files:"));
	add_subwindow(in_files = new InFileList(mwindow, 
		mwindow->theme->in_files_x, 
		mwindow->theme->in_files_y, 
		mwindow->theme->in_files_w, 
		mwindow->theme->in_files_h));

	add_subwindow(out_title = new BC_Title(mwindow->theme->out_files_x, 
		mwindow->theme->out_files_y - 20,
		"Checked out files:"));
	add_subwindow(out_files = new OutFileList(mwindow, 
		mwindow->theme->out_files_x, 
		mwindow->theme->out_files_y, 
		mwindow->theme->out_files_w, 
		mwindow->theme->out_files_h));

	add_subwindow(desc_title = new BC_Title(mwindow->theme->desc_x, 
		mwindow->theme->desc_y - 20,
		"Edit Description:"));
	add_subwindow(cd_desc = new Description(mwindow,
		mwindow->theme->desc_x,
		mwindow->theme->desc_y,
		mwindow->theme->desc_w,
		mwindow->theme->desc_h));
	add_subwindow(message = new BC_Title(mwindow->theme->message_x, 
		mwindow->theme->message_y,
		"Welcome to " TITLE));

	int x = mwindow->theme->buttons_x;
	int y = mwindow->theme->buttons_y;
	add_subwindow(check_out = new CheckOutButton(mwindow, x, y));
	y += 30;
	add_subwindow(check_in = new CheckInButton(mwindow, x, y));
	y += 30;
	add_subwindow(import_cd = new ImportCD(mwindow, x, y));
	y += 30;
	add_subwindow(home_button = new HomeButton(mwindow, x, y));
	y += 30;
	add_subwindow(delete_cd = new DeleteButton(mwindow, x, y));
	y += 30;
//	add_subwindow(move_cd = new MoveButton(mwindow, x, y));
//	y += 30;
	add_subwindow(search_cds = new SearchButton(mwindow, x, y));
	y += 30;
	add_subwindow(sort_title = new BC_Title(x, y, "Sort order:"));
	y += 20;
	add_subwindow(save_db = new SaveDB(mwindow, x, y));
	y += 30;
	add_subwindow(sort_cds = new Sort(mwindow, x, y));
	sort_cds->add_item(new BC_MenuItem(RobotPrefs::number_to_sort(RobotPrefs::SORT_PATH)));
	sort_cds->add_item(new BC_MenuItem(RobotPrefs::number_to_sort(RobotPrefs::SORT_DESC)));
	sort_cds->add_item(new BC_MenuItem(RobotPrefs::number_to_sort(RobotPrefs::SORT_SIZE)));
	sort_cds->add_item(new BC_MenuItem(RobotPrefs::number_to_sort(RobotPrefs::SORT_DATE)));
	sort_cds->add_item(new BC_MenuItem(RobotPrefs::number_to_sort(RobotPrefs::SORT_NUMBER)));
	y += 30;
	add_subwindow(options = new OptionsButton(mwindow, x, y));
	y += 30;
	add_subwindow(manual_override = new ManualButton(mwindow, x, y));
	add_subwindow(abort = new AbortButton(mwindow, 
		mwindow->theme->abort_x, 
		mwindow->theme->abort_y));
	add_subwindow(save_desc = new SaveDesc(mwindow, 
		mwindow->theme->savedesc_x,
		mwindow->theme->savedesc_y));

	x = mwindow->theme->info_x;
	y = mwindow->theme->info_y;

	int text_margin = 50;
	int text_offset = 150;
	int text_color = BLUE;

	add_subwindow(path_title = new BC_Title(x, y, "Path:"));
	add_subwindow(path_text = new BC_Title(x + text_margin, y, "-", MEDIUMFONT, text_color));

	y += 20;
	add_subwindow(size_title = new BC_Title(x, y, "Size:"));
	add_subwindow(size_text = new BC_Title(x + text_margin, y, "-", MEDIUMFONT, text_color));

	x += text_offset;
	add_subwindow(date_title = new BC_Title(x, y, "Date:"));
	add_subwindow(date_text = new BC_Title(x + text_margin, y, "-", MEDIUMFONT, text_color));

	x += text_offset;
	add_subwindow(tower_title = new BC_Title(x, y, "Tower:"));
	add_subwindow(tower_text = new BC_Title(x + text_margin, y, "-", MEDIUMFONT, text_color));

	x += text_offset;
	add_subwindow(row_title = new BC_Title(x, y, "Row:"));
	add_subwindow(row_text = new BC_Title(x + text_margin, y, "-", MEDIUMFONT, text_color));


	show_window();
}

int MWindowGUI::close_event()
{
	set_done(0);
	return 1;
}

int MWindowGUI::keypress_event()
{
	switch(get_keypress())
	{
		case 'g':
			if(ctrl_down())
			{
				mwindow->do_search(0);
				return 1;
			}
			break;

		case 'q':
			set_done(0);
			return 1;
			break;
	}
	return 0;
}

int MWindowGUI::resize_event(int w, int h)
{
	mwindow->theme->mwindow_w = w;
	mwindow->theme->mwindow_h = h;
	mwindow->theme->get_mwindow_sizes();
	draw_background();
	in_title->reposition_window(mwindow->theme->in_files_x, mwindow->theme->in_files_y - 20);
	in_files->reposition_window(mwindow->theme->in_files_x, 
		mwindow->theme->in_files_y, 
		mwindow->theme->in_files_w, 
		mwindow->theme->in_files_h);
	out_title->reposition_window(mwindow->theme->out_files_x, mwindow->theme->out_files_y - 20);
	out_files->reposition_window(mwindow->theme->out_files_x, 
		mwindow->theme->out_files_y, 
		mwindow->theme->out_files_w, 
		mwindow->theme->out_files_h);
	desc_title->reposition_window(mwindow->theme->desc_x, mwindow->theme->desc_y - 20);
	cd_desc->reposition_window(mwindow->theme->desc_x,
		mwindow->theme->desc_y,
		mwindow->theme->desc_w,
		Description::pixels_to_rows(mwindow->gui, MEDIUMFONT, mwindow->theme->desc_h));
	message->reposition_window(mwindow->theme->message_x, mwindow->theme->message_y);
	check_out->reposition_window(mwindow->theme->buttons_x, check_out->get_y());
	check_in->reposition_window(mwindow->theme->buttons_x, check_in->get_y());

	sort_title->reposition_window(mwindow->theme->buttons_x, sort_title->get_y());
	sort_cds->reposition_window(mwindow->theme->buttons_x, sort_cds->get_y());
	import_cd->reposition_window(mwindow->theme->buttons_x, import_cd->get_y());
	home_button->reposition_window(mwindow->theme->buttons_x, home_button->get_y());
	delete_cd->reposition_window(mwindow->theme->buttons_x, delete_cd->get_y());
//	move_cd->reposition_window(mwindow->theme->buttons_x, move_cd->get_y());
	search_cds->reposition_window(mwindow->theme->buttons_x, search_cds->get_y());
	save_db->reposition_window(mwindow->theme->buttons_x, save_db->get_y());
	options->reposition_window(mwindow->theme->buttons_x, options->get_y());
	manual_override->reposition_window(mwindow->theme->buttons_x, 
		manual_override->get_y());
	abort->reposition_window(mwindow->theme->abort_x, 
		mwindow->theme->abort_y);
	save_desc->reposition_window(mwindow->theme->savedesc_x,
		mwindow->theme->savedesc_y);

	int x = mwindow->theme->info_x;
	int y = mwindow->theme->info_y;
	int text_margin = 50;
	int text_offset = 150;

	path_title->reposition_window(x, y);
	path_text->reposition_window(x + text_margin, y);
	
	y += 20;
	size_title->reposition_window(x, y);
	size_text->reposition_window(x + text_margin, y);
	
	x += text_offset;
	date_title->reposition_window(x, y);
	date_text->reposition_window(x + text_margin, y);
	
	x += text_offset;
	tower_title->reposition_window(x, y);
	tower_text->reposition_window(x + text_margin, y);

	x += text_offset;
	row_title->reposition_window(x, y);
	row_text->reposition_window(x + text_margin, y);

	mwindow->save_defaults();
	return 1;
}

int MWindowGUI::translation_event()
{
	mwindow->theme->mwindow_x = get_x();
	mwindow->theme->mwindow_y = get_y();
	return 1;
}

void MWindowGUI::draw_background()
{
	clear_box(0, 0, mwindow->theme->mwindow_w, mwindow->theme->mwindow_h);
	draw_vframe(mwindow->theme->logo, 0, 0);
	flash();
}

void MWindowGUI::update_description(int in_list, int out_list)
{
	BC_ListBoxItem *item = 0;
	BC_ListBox *listbox;
	if(in_list)
	{
		listbox = in_files;
	}
	else
	if(out_list)
	{
		listbox = out_files;
	}


	item = listbox->get_selection(RobotDB::DESC, 0);
	if(item) 
		cd_desc->update(item->get_text());
	else
		cd_desc->update("");
	
	item = listbox->get_selection(RobotDB::PATH, 0);
	if(item)
		path_text->update(item->get_text());
	else
		path_text->update("-");

	item = listbox->get_selection(RobotDB::SIZE, 0);
	if(item)
		size_text->update(item->get_text());
	else
		size_text->update("-");

	item = listbox->get_selection(RobotDB::DATE, 0);
	if(item)
		date_text->update(item->get_text());
	else
		date_text->update("-");

	item = listbox->get_selection(RobotDB::TOWER, 0);
	if(item)
		tower_text->update(item->get_text());
	else
		tower_text->update("-");

	item = listbox->get_selection(RobotDB::ROW, 0);
	if(item)
		row_text->update(item->get_text());
	else
		row_text->update("-");

	last_in = in_list;
	last_out = out_list;
}

void MWindowGUI::description_to_item()
{
	BC_ListBoxItem *item = 0;
	BC_ListBoxItem *row_item = 0;
	BC_ListBoxItem *tower_item = 0;

//printf("MWindowGUI::description_to_item 1 %d %d\n", last_in, last_out);
	if(last_in)
	{
// Set value in in_db
		item = in_files->get_selection(RobotDB::DESC, 0);
		row_item = in_files->get_selection(RobotDB::ROW, 0);
		tower_item = in_files->get_selection(RobotDB::TOWER, 0);

		if(row_item && tower_item && item)
		{
//printf("MWindowGUI::description_to_item 2 %p %p %p\n", row_item, tower_item, item);
			item->set_text(cd_desc->get_text());

// Copy value to same item in out_db
			if(mwindow->out_db->copy_field(mwindow->in_db->data,
				item, 
				RobotDB::DESC))
			{
				update_out_list();
			}
			update_in_list();
		}
	}
	else
	if(last_out)
	{
		item = out_files->get_selection(RobotDB::DESC, 0);
		row_item = out_files->get_selection(RobotDB::ROW, 0);
		tower_item = out_files->get_selection(RobotDB::TOWER, 0);
//printf("MWindowGUI::description_to_item 3 %p %p %p\n", row_item, tower_item, item);

		if(row_item && tower_item && item)
		{
			item->set_text(cd_desc->get_text());

// Update in_db, too
			if(mwindow->in_db->copy_field(mwindow->out_db->data, 
				item, 
				RobotDB::DESC))
			{
				update_in_list();
			}
			update_out_list();
		}
	}
	mwindow->save_db(0);
}

void MWindowGUI::update_out_list()
{
	out_files->update(mwindow->out_db->data,
		RobotDB::column_titles,
		mwindow->prefs->out_column_widths,
		FIELDS,
		out_files->get_xposition(),
		out_files->get_yposition(),
		out_files->get_highlighted_item(),
		1,
		1);
}

void MWindowGUI::update_in_list(int center_selection)
{
	in_files->update(mwindow->in_db->data,
		RobotDB::column_titles,
		mwindow->prefs->in_column_widths,
		FIELDS,
		in_files->get_xposition(),
		in_files->get_yposition(),
		in_files->get_highlighted_item(),
		1,
		center_selection ? 0 : 1);
	if(center_selection)
		in_files->center_selection();
}













InFileList::InFileList(MWindow *mwindow, int x, int y, int w, int h)
 : BC_ListBox(x, 
		y, 
		w, 
		h,
		LISTBOX_TEXT,                   // Display text list or icons
		mwindow->in_db->data, // Each column has an ArrayList of BC_ListBoxItems.
		RobotDB::column_titles,             // Titles for columns.  Set to 0 for no titles
		mwindow->prefs->in_column_widths,                // width of each column
		FIELDS,                      // Total columns.  Only 1 in icon mode
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_SINGLE,  // Select one item or multiple items
		ICON_LEFT,        // Position of icon relative to text of each item
		0)
{
	this->mwindow = mwindow;
}

int InFileList::column_resize_event()
{
	for(int i = 0; i < FIELDS; i++)
	{
//printf("InFileList::column_resize_event 1 %d %d\n", i, get_column_width(i));
		mwindow->prefs->in_column_widths[i] = get_column_width(i);
	}
	mwindow->save_defaults();
	return 1;
}

int InFileList::handle_event()
{
	mwindow->start_checkout(1);
	return 1;
}

int InFileList::selection_changed()
{
	mwindow->gui->update_description(1, 0);
	return 1;
}


OutFileList::OutFileList(MWindow *mwindow, int x, int y, int w, int h)
 : BC_ListBox(x, 
		y, 
		w, 
		h,
		LISTBOX_TEXT,                   // Display text list or icons
		mwindow->out_db->data, // Each column has an ArrayList of BC_ListBoxItems.
		RobotDB::column_titles,             // Titles for columns.  Set to 0 for no titles
		mwindow->prefs->out_column_widths,                // width of each column
		FIELDS,                      // Total columns.  Only 1 in icon mode
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_SINGLE,  // Select one item or multiple items
		ICON_LEFT,        // Position of icon relative to text of each item
		0)
{
	this->mwindow = mwindow;
}

int OutFileList::column_resize_event()
{
	for(int i = 0; i < FIELDS; i++)
		mwindow->prefs->out_column_widths[i] = get_column_width(i);
	mwindow->save_defaults();
	return 1;
}

int OutFileList::handle_event()
{
	mwindow->start_checkin(1);
	return 1;
}

int OutFileList::selection_changed()
{
	mwindow->gui->update_description(0, 1);
	return 1;
}





Description::Description(MWindow *mwindow, int x, int y, int w, int h)
 : BC_TextBox(x, 
 	y, 
	w, 
	pixels_to_rows(mwindow->gui, MEDIUMFONT, h), 
	"")
{
	this->mwindow = mwindow;
}

int Description::handle_event()
{
	return 1;
}










CheckOutButton::CheckOutButton(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, 
 	y, 
	mwindow->theme->button_w, 
	"Check out...", 
	mwindow->theme->checkout)
{
	this->mwindow = mwindow;
}

int CheckOutButton::handle_event()
{
	mwindow->start_checkout(1);
	return 1;
}


CheckInButton::CheckInButton(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, 
 	y, 
	mwindow->theme->buttons_w,
	"Check in...", 
	mwindow->theme->checkin)
{
	this->mwindow = mwindow;
}

int CheckInButton::handle_event()
{
	mwindow->start_checkin(1);
	return 1;
}

Sort::Sort(MWindow *mwindow, int x, int y)
 : BC_PopupMenu(x,
 	y,
	mwindow->theme->buttons_w,
	RobotPrefs::number_to_sort(mwindow->prefs->sort_order))
{
	this->mwindow = mwindow;
}

int Sort::handle_event()
{
	mwindow->prefs->sort_order = RobotPrefs::sort_to_number(get_text());
	mwindow->do_sort();
	return 1;
}


ImportCD::ImportCD(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, 
 	y, 
	mwindow->theme->buttons_w,
	"Import...", 
	mwindow->theme->checkout)
{
	this->mwindow = mwindow;
}

int ImportCD::handle_event()
{
	mwindow->start_import();
	return 1;
}

HomeButton::HomeButton(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x,
 	y,
	mwindow->theme->buttons_w,
	"Go home",
	mwindow->theme->home)
{
	this->mwindow = mwindow;
}
int HomeButton::handle_event()
{
	mwindow->go_home();
	return 1;
}



DeleteButton::DeleteButton(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, 
 	y, 
	mwindow->theme->buttons_w,
	"Delete...")
{
	this->mwindow = mwindow;
}

int DeleteButton::handle_event()
{
	mwindow->start_delete();
	return 1;
}

// MoveButton::MoveButton(MWindow *mwindow, int x, int y)
//  : BC_GenericButton(x, 
//  	y, 
// 	mwindow->theme->buttons_w,
// 	"Move...")
// {
// 	this->mwindow = mwindow;
// }
// 
// int MoveButton::handle_event()
// {
// 	mwindow->start_move();
// 	return 1;
// }

SearchButton::SearchButton(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, 
 	y, 
	mwindow->theme->buttons_w,
	"Search...")
{
	this->mwindow = mwindow;
}

int SearchButton::handle_event()
{
	mwindow->start_search();
	return 1;
}

ManualButton::ManualButton(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, 
 	y, 
	mwindow->theme->buttons_w,
	"Override...")
{
	this->mwindow = mwindow;
}

int ManualButton::handle_event()
{
	mwindow->manual_override();
	return 1;
}

SaveDB::SaveDB(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, 
 	y, 
	mwindow->theme->buttons_w,
	"Save DB")
{
	this->mwindow = mwindow;
}

int SaveDB::handle_event()
{
	mwindow->save_db(0);
	return 0;
}

SaveDesc::SaveDesc(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, 
 	y, 
	mwindow->theme->buttons_w,
	"Save Desc")
{
	this->mwindow = mwindow;
}

int SaveDesc::handle_event()
{
	mwindow->gui->description_to_item();
	return 0;
}


OptionsButton::OptionsButton(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, 
 	y, 
	mwindow->theme->buttons_w,
	"Options...")
{
	this->mwindow = mwindow;
}

int OptionsButton::handle_event()
{
	mwindow->start_options();
	return 1;
}






AbortButton::AbortButton(MWindow *mwindow, int x, int y)
 : BC_Button(x,
 	y,
	mwindow->theme->abort)
{
	this->mwindow = mwindow;
}

int AbortButton::handle_event()
{
	mwindow->abort_robot();
	return 1;
}


