
/*
 * CINELERRA
 * Copyright (C) 1997-2017 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcdelete.h"
#include "bcfilebox.h"
#include "bclistboxitem.h"
#include "bcnewfolder.h"
#include "bcpixmap.h"
#include "bcrename.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "clip.h"
#include "condition.h"
#include "filesystem.h"
#include "keys.h"
#include "language.h"
#include "mutex.h"
#include <string.h>
#include <sys/stat.h>







BC_FileBoxRecent::BC_FileBoxRecent(BC_FileBox *filebox, int x, int y)
 : BC_ListBox(x, 
	y, 
	DP(250), 
	filebox->get_text_height(MEDIUMFONT) * FILEBOX_HISTORY_SIZE + 
		BC_ScrollBar::get_span(SCROLL_HORIZ) +
		LISTBOX_MARGIN * 2,
	LISTBOX_TEXT, 
	&filebox->recent_dirs, 
	0, 
	0, 
	1, 
	0, 
	1)
{
	this->filebox = filebox;
	set_justify(LISTBOX_LEFT);
}

int BC_FileBoxRecent::handle_event()
{
	if(get_selection(0, 0) >= 0)
	{
		filebox->submit_dir(get_selection(0, 0)->get_text());
	}
	return 1;
}











BC_FileBoxListBox::BC_FileBoxListBox(int x, int y, BC_FileBox *filebox)
 : BC_ListBox(x, 
 	y, 
 	filebox->get_listbox_w(), 
	filebox->get_listbox_h(y), 
 	filebox->get_display_mode(), 
 	filebox->list_column, 
	filebox->column_titles,
	filebox->column_width,
	filebox->columns,
	filebox->reload_yposition(filebox->fs->get_current_dir()),
	0,
	filebox->select_multiple ? LISTBOX_MULTIPLE : LISTBOX_SINGLE,
	ICON_LEFT,
	0)
{ 
	this->filebox = filebox;
	set_sort_column(filebox->sort_column);
	set_sort_order(filebox->sort_order);
	set_allow_drag_column(1);
}

BC_FileBoxListBox::~BC_FileBoxListBox()
{
}

int BC_FileBoxListBox::handle_event()
{
	filebox->submit_file(filebox->textbox->get_text());
	return 1;
}

int BC_FileBoxListBox::selection_changed()
{
	BC_ListBoxItem *item = get_selection(
		filebox->column_of_type(FILEBOX_NAME), 0);

	if(item)
	{
		char path[BCTEXTLEN];
		strcpy(path, item->get_text());
		filebox->textbox->update(path);
		filebox->fs->extract_dir(filebox->directory, path);
		filebox->fs->extract_name(filebox->filename, path);
		filebox->fs->complete_path(path);
		strcpy(filebox->current_path, path);
		strcpy(filebox->submitted_path, path);
	}
	return 1;
}

int BC_FileBoxListBox::column_resize_event()
{
	for(int i = 0; i < filebox->columns; i++)
		BC_WindowBase::get_resources()->filebox_columnwidth[i] = 
			filebox->column_width[i] = 
			get_column_width(i);
	return 1;
}

int BC_FileBoxListBox::sort_order_event()
{
	get_resources()->filebox_sortcolumn = filebox->sort_column = get_sort_column();
	get_resources()->filebox_sortorder = filebox->sort_order = get_sort_order();
	filebox->refresh(0, 0);
	return 1;
}

int BC_FileBoxListBox::move_column_event()
{
	filebox->move_column(get_from_column(), get_to_column());
	return 1;
}

int BC_FileBoxListBox::evaluate_query(char *string)
{
// Search name column
	ArrayList<BC_ListBoxItem*> *column = 
		&filebox->list_column[filebox->column_of_type(FILEBOX_NAME)];
// Get current selection
	int current_selection = get_selection_number(0, 0);

// Get best score in remaining items
	int lowest_score = 0x7fffffff;
	int best_item = -1;
	if(current_selection < 0) current_selection = 0;
//	for(int i = current_selection + 1; i < column->size(); i++)
	for(int i = current_selection; i < column->size(); i++)
	{
		int len1 = strlen(string);
		int len2 = strlen(column->get(i)->get_text());
		int current_score = strncasecmp(string, 
			column->get(i)->get_text(),
			MIN(len1, len2));
//printf(" %d i=%d %d %s %s\n", __LINE__, i, current_score, string, column->get(i)->get_text());

		if(abs(current_score) < lowest_score)
		{
			lowest_score = abs(current_score);
			best_item = i;
		}
	}


	return best_item;
}




BC_FileBoxTextBox::BC_FileBoxTextBox(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, 
 	y, 
	filebox->get_w() - DP(20), 
	1, 
	filebox->want_directory ?
		filebox->directory :
 		filebox->filename)
{
	this->filebox = filebox; 
}

BC_FileBoxTextBox::~BC_FileBoxTextBox()
{
}




int BC_FileBoxTextBox::handle_event()
{
	int result = 0;
	if(get_keypress() != RETURN)
	{
		result = calculate_suggestions(&filebox->list_column[0]);
	}
	return result;
}







BC_FileBoxFilterText::BC_FileBoxFilterText(int x, int y, BC_FileBox *filebox)
 : BC_TextBox(x, 
 	y, 
	filebox->get_w() - DP(50), 
	1, 
	filebox->get_resources()->filebox_filter)
{
	this->filebox = filebox;
}

int BC_FileBoxFilterText::handle_event()
{
	filebox->update_filter(get_text());
	return 0;
}




BC_FileBoxFilterMenu::BC_FileBoxFilterMenu(int x, int y, BC_FileBox *filebox)
 : BC_ListBox(x, 
 	y, 
	filebox->get_w() - DP(30), 
	DP(120), 
	LISTBOX_TEXT, 
	&filebox->filter_list, 
	0, 
	0, 
	1, 
	0, 
	1)
{
	this->filebox = filebox;
	set_tooltip(_("Change the filter"));
}

int BC_FileBoxFilterMenu::handle_event()
{
	filebox->filter_text->update(
		get_selection(filebox->column_of_type(FILEBOX_NAME), 0)->get_text());
	filebox->update_filter(
		get_selection(filebox->column_of_type(FILEBOX_NAME), 0)->get_text());
	return 0;
}










BC_FileBoxCancel::BC_FileBoxCancel(BC_FileBox *filebox)
 : BC_CancelButton(filebox)
{
	this->filebox = filebox;
	set_tooltip(_("Cancel the operation"));
}

BC_FileBoxCancel::~BC_FileBoxCancel()
{
}

int BC_FileBoxCancel::handle_event()
{
//	filebox->submit_file(filebox->textbox->get_text());
	filebox->newfolder_thread->interrupt();
    filebox->store_yposition();
	filebox->set_done(1);
	return 1;
}







BC_FileBoxUseThis::BC_FileBoxUseThis(BC_FileBox *filebox)
 : BC_Button(filebox->get_w() / 2 - 
 		BC_WindowBase::get_resources()->usethis_button_images[0]->get_w() / 2, 
 	filebox->ok_button->get_y(), 
	BC_WindowBase::get_resources()->usethis_button_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Submit the directory"));
}

BC_FileBoxUseThis::~BC_FileBoxUseThis()
{
}

int BC_FileBoxUseThis::handle_event()
{
// printf("BC_FileBoxUseThis::handle_event %d '%s'\n", 
// __LINE__, 
// filebox->textbox->get_text());
	filebox->submit_file(filebox->textbox->get_text(), 1);
	return 1;
}





BC_FileBoxOK::BC_FileBoxOK(BC_FileBox *filebox)
 : BC_OKButton(filebox, 
 	!filebox->want_directory ? 
		BC_WindowBase::get_resources()->ok_images :
		BC_WindowBase::get_resources()->filebox_descend_images)
{
	this->filebox = filebox; 
	if(filebox->want_directory)
		set_tooltip(_("Descend directory"));
	else
		set_tooltip(_("Submit the file"));
}

BC_FileBoxOK::~BC_FileBoxOK()
{
}

int BC_FileBoxOK::handle_event()
{
//printf("BC_FileBoxOK::handle_event %d\n", __LINE__);
	filebox->submit_file(filebox->textbox->get_text());
	return 1;
}




// 
// 
// BC_FileBoxText::BC_FileBoxText(int x, int y, BC_FileBox *filebox)
//  : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_text_images)
// {
// 	this->filebox = filebox; 
// 	set_tooltip(_("Display text"));
// }
// int BC_FileBoxText::handle_event()
// {
// 	filebox->create_listbox(filebox->listbox->get_x(), filebox->listbox->get_y(), LISTBOX_TEXT);
// 	filebox->listbox->show_window(1);
// 	return 1;
// }
// 
// 
// BC_FileBoxIcons::BC_FileBoxIcons(int x, int y, BC_FileBox *filebox)
//  : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_icons_images)
// {
// 	this->filebox = filebox; 
// 	set_tooltip(_("Display icons"));
// }
// int BC_FileBoxIcons::handle_event()
// {
// 	filebox->create_listbox(filebox->listbox->get_x(), filebox->listbox->get_y(), LISTBOX_ICONS);
// 	filebox->listbox->show_window(1);
// 	return 1;
// }


BC_FileBoxNewfolder::BC_FileBoxNewfolder(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_newfolder_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Create new folder"));
}
int BC_FileBoxNewfolder::handle_event()
{
	filebox->newfolder_thread->start_new_folder();
	return 1;
}


BC_FileBoxRename::BC_FileBoxRename(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_rename_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Rename file"));
}
int BC_FileBoxRename::handle_event()
{
	filebox->rename_thread->start_rename();
	return 1;
}

BC_FileBoxUpdir::BC_FileBoxUpdir(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_updir_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Up a directory"));
}
int BC_FileBoxUpdir::handle_event()
{
// Need a temp so submit_file can expand it
	sprintf(string, _(".."));
	filebox->submit_file(string);
	return 1;
}

BC_FileBoxDelete::BC_FileBoxDelete(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_delete_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Delete files"));
}
int BC_FileBoxDelete::handle_event()
{
	filebox->unlock_window();
	filebox->delete_thread->start();
	filebox->lock_window("BC_FileBoxDelete::handle_event");
	return 1;
}

BC_FileBoxReload::BC_FileBoxReload(int x, int y, BC_FileBox *filebox)
 : BC_Button(x, y, BC_WindowBase::get_resources()->filebox_reload_images)
{
	this->filebox = filebox; 
	set_tooltip(_("Refresh"));
}
int BC_FileBoxReload::handle_event()
{
	filebox->refresh(0, 0);
	return 1;
}








Mutex *BC_FileBox::history_lock;
ArrayList<BC_DirectoryPosition*> BC_FileBox::directory_positions;


BC_FileBox::BC_FileBox(int x, 
		int y, 
		const char *init_path,
		const char *title,
		const char *caption,
		int show_all_files,
		int want_directory,
		int multiple_files,
		int h_padding)
 : BC_Window(title, 
 	x,
	y,
 	BC_WindowBase::get_resources()->filebox_w, 
	BC_WindowBase::get_resources()->filebox_h, 
	DP(10), 
	DP(10),
	1,
	0,
	1)
{
	fs = new FileSystem;
// 	if(want_directory)
// 	{
// 		fs->set_want_directory();
// 		columns = DIRBOX_COLUMNS;
// 		columns = FILEBOX_COLUMNS;
// 	}
// 	else
	{
		columns = FILEBOX_COLUMNS;
	}

	list_column = new ArrayList<BC_ListBoxItem*>[columns];
	column_type = new int[columns];
	column_width = new int[columns];

	filter_text = 0;
	filter_popup = 0;
	usethis_button = 0;

	strcpy(this->caption, caption);
	strcpy(this->current_path, init_path);
	strcpy(this->submitted_path, init_path);
	select_multiple = multiple_files;
	this->want_directory = want_directory;
	if(show_all_files) fs->set_show_all();
	fs->complete_path(this->current_path);
	fs->complete_path(this->submitted_path);
	fs->extract_dir(directory, this->current_path);
	fs->extract_name(filename, this->current_path);

// printf("BC_FileBox::BC_FileBox %d '%s' '%s' '%s'\n", 
// __LINE__, 
// this->submitted_path,
// directory,
// filename);

// 	if(want_directory)
// 	{
// 		for(int i = 0; i < columns; i++)
// 		{
// 			column_type[i] = get_resources()->dirbox_columntype[i];
// 			column_width[i] = get_resources()->dirbox_columnwidth[i];
// 			column_titles[i] = BC_FileBox::columntype_to_text(column_type[i]);
// 		}
// 		sort_column = get_resources()->dirbox_sortcolumn;
// 		sort_order = get_resources()->dirbox_sortorder;
// 	}
// 	else
	{
		for(int i = 0; i < columns; i++)
		{
			column_type[i] = get_resources()->filebox_columntype[i];
			column_width[i] = get_resources()->filebox_columnwidth[i];
			column_titles[i] = (char*)BC_FileBox::columntype_to_text(column_type[i]);
		}
		sort_column = get_resources()->filebox_sortcolumn;
		sort_order = get_resources()->filebox_sortorder;
	}



// Test if current directory exists
	if(!fs->is_dir(directory))
	{
		sprintf(this->current_path, "~");
		fs->complete_path(this->current_path);
		fs->set_current_dir(this->current_path);
//		fs->update(this->current_path);
		strcpy(directory, fs->get_current_dir());
		filename[0] = 0;
	}
	else
	{
		fs->change_dir(directory, 0);
	}


	if(h_padding == -1)
	{
		h_padding = BC_WindowBase::get_resources()->ok_images[0]->get_h() - 
			20;
	}
	this->h_padding = h_padding;
	delete_thread = new BC_DeleteThread(this);

    if(!history_lock)
    {
        history_lock = new Mutex("BC_FileBox::history_lock", 1);
    }
}

BC_FileBox::~BC_FileBox()
{
// this has to be destroyed before tables, because it can call for an update!
	delete newfolder_thread;
	delete fs;
	delete_tables();
	for(int i = 0; i < TOTAL_ICONS; i++)
		delete icons[i];
	filter_list.remove_all_objects();
	delete [] list_column;
	delete [] column_type;
	delete [] column_width;
	delete delete_thread;
    delete rename_thread;
	recent_dirs.remove_all_objects();
}

void BC_FileBox::create_objects()
{
    int margin = DP(10);
	int x = margin, y = margin;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int directory_title_margin = MAX(DP(20),
		resources->filebox_text_images[0]->get_h());
	

// Create recent dir list
	create_history();

// Directories aren't filtered in FileSystem so skip this
	if(!want_directory)
	{
		filter_list.append(new BC_ListBoxItem("*"));
		filter_list.append(new BC_ListBoxItem("[*.ifo][*.vob]"));
		filter_list.append(new BC_ListBoxItem("[*.mp2][*.mp3][*.wav]"));
		filter_list.append(new BC_ListBoxItem("[*.avi][*.mpg][*.m2v][*.m1v][*.mov]"));
		filter_list.append(new BC_ListBoxItem("heroine*"));
		filter_list.append(new BC_ListBoxItem("*.xml"));
		fs->set_filter(get_resources()->filebox_filter);
	}

//	fs->update(directory);
	create_icons();
	create_tables();

	add_subwindow(ok_button = new BC_FileBoxOK(this));
	if(want_directory)
		add_subwindow(usethis_button = new BC_FileBoxUseThis(this));
	add_subwindow(cancel_button = new BC_FileBoxCancel(this));

	add_subwindow(new BC_Title(x, y, caption));

	x = get_w() - resources->filebox_icons_images[0]->get_w() - DP(10);

//	add_subwindow(icon_button = new BC_FileBoxIcons(x, y, this));
//	x -= resources->filebox_text_images[0]->get_w() + DP(5);

//	add_subwindow(text_button = new BC_FileBoxText(x, y, this));
//	x -= resources->filebox_newfolder_images[0]->get_w() + DP(5);

	add_subwindow(folder_button = new BC_FileBoxNewfolder(x, y, this));
	x -= resources->filebox_delete_images[0]->get_w() + DP(5);

	add_subwindow(rename_button = new BC_FileBoxRename(x, y, this));
	x -= resources->filebox_delete_images[0]->get_w() + DP(5);

	add_subwindow(delete_button = new BC_FileBoxDelete(x, y, this));
	x -= resources->filebox_reload_images[0]->get_w() + DP(5);

	add_subwindow(reload_button = new BC_FileBoxReload(x, y, this));
	x -= resources->filebox_updir_images[0]->get_w() + DP(5);

	add_subwindow(updir_button = new BC_FileBoxUpdir(x, y, this));

	x = margin;
	y += directory_title_margin;

	add_subwindow(recent_popup = new BC_FileBoxRecent(this, 
		x, 
		y - get_resources()->listbox_button[0]->get_h() +
			BC_Title::calculate_h(this, "0")));
	x += recent_popup->get_w();

	add_subwindow(directory_title = 
		new BC_Title(x, 
            y, 
            fs->get_current_dir(), 
            MEDIUMFONT, 
            -1, 
            0, 
            get_w() - x - margin));

	x = margin;
	y += directory_title->get_h() + DP(5);
	listbox = 0;

	create_listbox(x, y, get_display_mode());
	y += listbox->get_h() + margin;
	add_subwindow(textbox = new BC_FileBoxTextBox(x, y, this));
	y += textbox->get_h() + margin;


	if(!want_directory)
	{
		add_subwindow(filter_text = new BC_FileBoxFilterText(x, y, this));
		add_subwindow(filter_popup = 
			new BC_FileBoxFilterMenu(x + filter_text->get_w(), y, this));;
	}

// listbox has to be active because refresh might be called from newfolder_thread
 	listbox->activate();
	newfolder_thread = new BC_NewFolderThread(this);
	
	rename_thread = new BC_RenameThread(this);


	show_window();
}

int BC_FileBox::get_listbox_w()
{
	return get_w() - DP(20);
}

int BC_FileBox::get_listbox_h(int y)
{
	int result = get_h() - 
		y - 
		h_padding;
	if(want_directory)
		result -= BC_WindowBase::get_resources()->dirbox_margin;
	else
		result -= BC_WindowBase::get_resources()->filebox_margin;

	return result;
}

int BC_FileBox::create_icons()
{
	for(int i = 0; i < TOTAL_ICONS; i++)
	{
		icons[i] = new BC_Pixmap(this, 
			BC_WindowBase::get_resources()->type_to_icon[i],
			PIXMAP_ALPHA);
	}
	return 0;
}

int BC_FileBox::resize_event(int w, int h)
{
    int margin = DP(10);
	draw_background(0, 0, w, h);
	flash(0);

// OK button handles resize event itself
// 	ok_button->reposition_window(ok_button->get_x(), 
// 		h - (get_h() - ok_button->get_y()));
// 	cancel_button->reposition_window(w - (get_w() - cancel_button->get_x()), 
// 		h - (get_h() - cancel_button->get_y()));
	if(usethis_button)
		usethis_button->reposition_window(w / 2 - DP(50), 
			h - (get_h() - usethis_button->get_y()));


	if(filter_popup) filter_popup->reposition_window(w - (get_w() - filter_popup->get_x()), 
		h - (get_h() - filter_popup->get_y()),
		w - DP(30),
		0);


	if(filter_text) filter_text->reposition_window(filter_text->get_x(), 
		h - (get_h() - filter_text->get_y()),
		w - (get_w() - filter_text->get_w()),
		1);

	textbox->reposition_window(textbox->get_x(), 
		h - (get_h() - textbox->get_y()),
		w - (get_w() - textbox->get_w()),
		1);
	listbox->reposition_window(listbox->get_x(),
		listbox->get_y(),
		w - (get_w() - listbox->get_w()),
		h - (get_h() - listbox->get_h()),
		0);
//	icon_button->reposition_window(w - (get_w() - icon_button->get_x()), 
//		icon_button->get_y());
//	text_button->reposition_window(w - (get_w() - text_button->get_x()), 
//		text_button->get_y());
	folder_button->reposition_window(w - (get_w() - folder_button->get_x()), 
		folder_button->get_y());
	rename_button->reposition_window(w - (get_w() - rename_button->get_x()), 
		rename_button->get_y());
	reload_button->reposition_window(w - (get_w() - reload_button->get_x()),
		reload_button->get_y());
	delete_button->reposition_window(w - (get_w() - delete_button->get_x()),
		delete_button->get_y());
	updir_button->reposition_window(w - (get_w() - updir_button->get_x()), 
		updir_button->get_y());
    directory_title->reposition(directory_title->get_x(),
        directory_title->get_y(),
        w - directory_title->get_x() - margin);
        
        
	set_w(w);
	set_h(h);
	get_resources()->filebox_w = get_w();
	get_resources()->filebox_h = get_h();
	flush();
	return 1;
}

int BC_FileBox::keypress_event()
{
	switch(get_keypress())
	{
		case 'w':
			if(ctrl_down())
            {
                store_yposition();
                set_done(1);
            }
			return 1;
			break;
	}
	return 0;
}

int BC_FileBox::close_event()
{
    store_yposition();
	set_done(1);
	return 1;
}

int BC_FileBox::handle_event()
{
	return 0;
}

int BC_FileBox::create_tables()
{
	delete_tables();
	char string[BCTEXTLEN];
	BC_ListBoxItem *new_item;

	fs->set_sort_order(sort_order);
	fs->set_sort_field(column_type[sort_column]);

// Directory is entered before this from a random source
	fs->update(0);
	for(int i = 0; i < fs->total_files(); i++)
	{
		FileItem *file_item = fs->get_entry(i);
		int is_dir = file_item->is_dir;
		BC_Pixmap* icon = get_icon(file_item->name, is_dir);

// Name entry
		new_item = new BC_ListBoxItem(file_item->name,
			icon, 
			is_dir ? get_resources()->directory_color : get_resources()->file_color);
		if(is_dir) new_item->set_searchable(0);
		list_column[column_of_type(FILEBOX_NAME)].append(new_item);
	
// Size entry
// 		if(!want_directory)
// 		{
			if(!is_dir)
			{
				sprintf(string, "%lld", (long long)file_item->size);
				new_item = new BC_ListBoxItem(string, get_resources()->file_color);
			}
			else
			{
				new_item = new BC_ListBoxItem("", get_resources()->directory_color);
			}

	 		list_column[column_of_type(FILEBOX_SIZE)].append(new_item);
//		}

// Date entry
		if(!is_dir || 1)
		{
			static const char *month_text[13] = 
			{
				"Null",
				"Jan",
				"Feb",
				"Mar",
				"Apr",
				"May",
				"Jun",
				"Jul",
				"Aug",
				"Sep",
				"Oct",
				"Nov",
				"Dec"
			};
			sprintf(string, 
				"%s %d, %d", 
				month_text[file_item->month],
				file_item->day,
				file_item->year);
			new_item = new BC_ListBoxItem(string, get_resources()->file_color);
		}
		else
		{
			new_item = new BC_ListBoxItem("", get_resources()->directory_color);
		}

		list_column[column_of_type(FILEBOX_DATE)].append(new_item);
	}
	
	return 0;
}

int BC_FileBox::delete_tables()
{
	for(int j = 0; j < columns; j++)
	{
		list_column[j].remove_all_objects();
	}
	return 0;
}

BC_Pixmap* BC_FileBox::get_icon(char *path, int is_dir)
{
	char *suffix = strrchr(path, '.');
	int icon_type = ICON_UNKNOWN;

	if(is_dir) return icons[ICON_FOLDER];

	if(suffix)
	{
		suffix++;
		if(*suffix != 0)
		{
			for(int i = 0; i < TOTAL_SUFFIXES; i++)
			{
				if(!strcasecmp(suffix, BC_WindowBase::get_resources()->suffix_to_type[i].suffix)) 
				{
					icon_type = BC_WindowBase::get_resources()->suffix_to_type[i].icon_type;
					break;
				}
			}
		}
	}

	return icons[icon_type];
}

const char* BC_FileBox::columntype_to_text(int type)
{
	switch(type)
	{
		case FILEBOX_NAME:
			return FILEBOX_NAME_TEXT;
			break;
		case FILEBOX_SIZE:
			return FILEBOX_SIZE_TEXT;
			break;
		case FILEBOX_DATE:
			return FILEBOX_DATE_TEXT;
			break;
	}
	return "";
}

int BC_FileBox::column_of_type(int type)
{
	for(int i = 0; i < columns; i++)
		if(column_type[i] == type) return i;
	return 0;
}



int BC_FileBox::refresh(int reload_y_position, int reset_y_position)
{
	create_tables();
	listbox->set_master_column(column_of_type(FILEBOX_NAME), 0);
    int yposition = listbox->get_yposition();
    if(reset_y_position)
    {
        yposition = 0;
    }
    
    if(reload_y_position)
    {
        yposition = this->reload_yposition(fs->get_current_dir());
    }
    
	listbox->update(list_column, 
		column_titles, 
		column_width,
		columns, 
		listbox->get_xposition(), 
		yposition,
		-1, 
		1);

	return 0;
}

int BC_FileBox::update_filter(const char *filter)
{
	fs->set_filter(filter);
//	fs->update(0);
	refresh(0, 0);
	strcpy(get_resources()->filebox_filter, filter);

	return 0;
}


void BC_FileBox::move_column(int src, int dst)
{
	if(src != dst)
	{

		ArrayList<BC_ListBoxItem*> *new_columns = 
			new ArrayList<BC_ListBoxItem*>[columns];
		int *new_types = new int[columns];
		int *new_widths = new int[columns];

	// Fill in remaining columns with consecutive data
		for(int out_column = 0, in_column = 0; 
			out_column < columns; 
			out_column++,
			in_column++)
		{
	// Copy destination column from src column
			if(out_column == dst)
			{
				for(int i = 0; i < list_column[src].total; i++)
				{
					new_columns[out_column].append(list_column[src].values[i]);
				}
				new_types[out_column] = column_type[src];
				new_widths[out_column] = column_width[src];
				in_column--;
			}
			else
			{
	// Skip source column
				if(in_column == src) in_column++;
				for(int i = 0; i < list_column[src].total; i++)
				{
					new_columns[out_column].append(list_column[in_column].values[i]);
				}
				new_types[out_column] = column_type[in_column];
				new_widths[out_column] = column_width[in_column];
			}
		}

	// Swap tables
		delete [] list_column;
		delete [] column_type;
		delete [] column_width;
		list_column = new_columns;
		column_type = new_types;
		column_width = new_widths;

		for(int i = 0; i < columns; i++)
		{
			get_resources()->filebox_columntype[i] = column_type[i];
			get_resources()->filebox_columnwidth[i] = column_width[i];
			column_titles[i] = (char*)BC_FileBox::columntype_to_text(column_type[i]);
		}
	}

	refresh(0, 0);
}


int BC_FileBox::submit_dir(char *dir)
{
    store_yposition();
	strcpy(directory, dir);
	fs->join_names(current_path, directory, filename);

// printf("BC_FileBox::submit_dir %d '%s' '%s' '%s'\n", 
// __LINE__, 
// current_path,
// directory,
// filename);
	strcpy(submitted_path, current_path);
	fs->change_dir(dir, 0);
	refresh(1, 1);
	directory_title->update(fs->get_current_dir());
	if(want_directory)
	{
    	textbox->update(fs->get_current_dir());
	}
    else
	{
    	textbox->update(filename);
	}
    listbox->reset_query();
	return 0;
}

int BC_FileBox::submit_file(const char *path, int use_this)
{
//printf("BC_FileBox::submit_file %d %s\n", __LINE__, fs->get_current_dir());
// Deactivate textbox to hide suggestions
	textbox->deactivate();

// store current y position
    store_yposition();

// If file wanted, take the current directory as the desired file.
// If directory wanted, ignore it.
	if(!path[0] && !want_directory)
	{
// save complete path
		strcpy(this->current_path, directory);
// save complete path
		strcpy(this->submitted_path, directory);
		update_history();
// Zero out filename
		filename[0] = 0;
		set_done(0);
		return 0;
	}

// is a directory, change directories
	if(fs->is_dir(path) && !use_this)
	{
		fs->change_dir(path, 0);
		refresh(1, 1);
		directory_title->update(fs->get_current_dir());
		strcpy(this->current_path, fs->get_current_dir());
		strcpy(this->submitted_path, fs->get_current_dir());
		strcpy(this->directory, fs->get_current_dir());
		filename[0] = 0;
		if(want_directory)
			textbox->update(fs->get_current_dir());
		else
			textbox->update("");
		listbox->reset_query();
		return 1;
	}
	else
// Is a file or desired directory.  Quit the operation.
	{
		char path2[BCTEXTLEN];
		strcpy(path2, path);

// save directory for defaults
		fs->extract_dir(directory, path2);     

// Just take the directory
		if(want_directory)
		{
			filename[0] = 0;
			strcpy(path2, directory);
		}
		else
// Take the complete path
		{
			fs->extract_name(filename, path2);     // save filename
		}

		fs->complete_path(path2);
		strcpy(this->current_path, path2);          // save complete path
		strcpy(this->submitted_path, path2);          // save complete path
		update_history();
		newfolder_thread->interrupt();
		set_done(0);
		return 0;
	}
	return 0;
}

void BC_FileBox::store_yposition()
{
    history_lock->lock("BC_FileBox::store_yposition");
   
   
// printf("BC_FileBox::store_yposition %d %s %d\n", 
// __LINE__, 
// fs->get_current_dir(), 
// listbox->get_yposition());
    int got_it = 0;
    for(int i = 0; i < directory_positions.size() && !got_it; i++)
    {
        BC_DirectoryPosition *value = directory_positions.get(i);
        if(!value->path.compare(fs->get_current_dir()))
        {
            // overwrite previous Y
            value->y_offset = listbox->get_yposition();
            got_it = 1;
        }
    }
    
    if(!got_it)
    {
        BC_DirectoryPosition *value = new BC_DirectoryPosition();
        directory_positions.append(value);
        value->path = fs->get_current_dir();
        value->y_offset = listbox->get_yposition();
    }
    
    history_lock->unlock();
}

int BC_FileBox::reload_yposition(char *directory)
{
    history_lock->lock("BC_FileBox::store_yposition");

    for(int i = 0; i < directory_positions.size(); i++)
    {
        BC_DirectoryPosition *value = directory_positions.get(i);
        if(!value->path.compare(directory))
        {
//printf("BC_FileBox::reload_yposition %d %s %d\n", __LINE__, directory, value->y_offset);
            int result = value->y_offset;
            history_lock->unlock();
            return result;
        }
    }

    
    history_lock->unlock();
    
    return 0;
}

void BC_FileBox::update_history()
{
// Look for path already in history
	BC_Resources *resources = get_resources();
	int new_slot = FILEBOX_HISTORY_SIZE - 1;

	for(int i = FILEBOX_HISTORY_SIZE - 1; i >= 0; i--)
	{
		if(resources->filebox_history[i].path[0] &&
			!strcmp(resources->filebox_history[i].path, directory))
		{
// Got matching path.
// Update ID.
			resources->filebox_history[i].id = resources->get_filebox_id();
			return;
		}
// // Shift down from this point.
// 			while(i > 0)
// 			{
// 				strcpy(resources->filebox_history[i], 
// 					resources->filebox_history[i - 1]);
// 				if(resources->filebox_history[i][0]) new_slot--;
// 				i--;
// 			}
// 			break;
// 		}
// 		else
// 			if(resources->filebox_history[i][0])
// 				new_slot--;
// 		else
// 			break;
	}

// Remove oldest entry if full
	if(resources->filebox_history[FILEBOX_HISTORY_SIZE - 1].path[0])
	{
		int oldest_id = 0x7fffffff;
		int oldest = 0;
		for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
		{
			if(resources->filebox_history[i].path[0] &&
				resources->filebox_history[i].id < oldest_id)
			{
				oldest_id = resources->filebox_history[i].id;
				oldest = i;
			}
		}

		for(int i = oldest; i < FILEBOX_HISTORY_SIZE - 1; i++)
		{
			strcpy(resources->filebox_history[i].path,
				resources->filebox_history[i + 1].path);
			resources->filebox_history[i].id = 
				resources->filebox_history[i + 1].id;
		}
	}

// Create new entry
	strcpy(resources->filebox_history[FILEBOX_HISTORY_SIZE - 1].path,
		directory);
	resources->filebox_history[FILEBOX_HISTORY_SIZE - 1].id = resources->get_filebox_id();

// Alphabetize
	int done = 0;
	while(!done)
	{
		done = 1;
		for(int i = 1; i < FILEBOX_HISTORY_SIZE; i++)
		{
			if((resources->filebox_history[i - 1].path[0] &&
				resources->filebox_history[i].path[0] &&
				strcasecmp(resources->filebox_history[i - 1].path,
					resources->filebox_history[i].path) > 0) ||
				resources->filebox_history[i - 1].path[0] == 0 &&
				resources->filebox_history[i].path[0])
			{
				done = 0;
				char temp[BCTEXTLEN];
				int id_temp;
				strcpy(temp, resources->filebox_history[i - 1].path);
				id_temp = resources->filebox_history[i - 1].id;
				strcpy(resources->filebox_history[i - 1].path,
					resources->filebox_history[i].path);
				resources->filebox_history[i - 1].id = 
					resources->filebox_history[i].id;
				strcpy(resources->filebox_history[i].path, temp);
				resources->filebox_history[i].id = id_temp;
			}
		}
	}

// 	if(new_slot < 0)
// 	{
// 		for(int i = FILEBOX_HISTORY_SIZE - 1; i > 0; i--)
// 		{
// 			strcpy(resources->filebox_history[i], 
// 					resources->filebox_history[i - 1]);
// 		}
// 		new_slot = 0;
// 	}
// 
// 	strcpy(resources->filebox_history[new_slot], directory);

	create_history();
	recent_popup->update(&recent_dirs,
		0,
		0,
		1);
}

void BC_FileBox::create_history()
{
	BC_Resources *resources = get_resources();
	recent_dirs.remove_all_objects();
	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
	{
		if(resources->filebox_history[i].path[0])
		{
			recent_dirs.append(new BC_ListBoxItem(resources->filebox_history[i].path));
		}
	}
}


int BC_FileBox::get_display_mode()
{
	return LISTBOX_TEXT;
//	return top_level->get_resources()->filebox_mode;
}

void BC_FileBox::create_listbox(int x, int y, int mode)
{
	if(listbox && listbox->get_display_mode() != mode)
	{
		delete listbox;
		listbox = 0;
		top_level->get_resources()->filebox_mode = mode;
	}

	if(!listbox)
    {
		add_subwindow(listbox = new BC_FileBoxListBox(x, y, this));
    }
}

char* BC_FileBox::get_path(int selection)
{
	if(selection == 0)
	{
		return get_submitted_path();
	}
	else
	{
		BC_ListBoxItem *item = listbox->get_selection(
			column_of_type(FILEBOX_NAME), selection - 1);
		if(item) 
		{
			fs->join_names(string, directory, item->get_text());
			return string;
		}
	}
	return 0;
}

char* BC_FileBox::get_submitted_path()
{
	return submitted_path;
}

char* BC_FileBox::get_current_path()
{
//printf("BC_FileBox::get_current_path 1 %s\n", current_path);
	return current_path;
}

char* BC_FileBox::get_newfolder_title()
{
	char *letter2 = strchr(title, ':');
	new_folder_title[0] = 0;
	if(letter2)
	{
		memcpy(new_folder_title, title, letter2 - title);
		new_folder_title[letter2 - title] = 0;
	}

	strcat(new_folder_title, _(": New folder"));

	return new_folder_title;
}

char* BC_FileBox::get_rename_title()
{
	char *letter2 = strchr(title, ':');
	new_folder_title[0] = 0;
	if(letter2)
	{
		memcpy(new_folder_title, title, letter2 - title);
		new_folder_title[letter2 - title] = 0;
	}

	strcat(new_folder_title, _(": Rename"));

	return new_folder_title;
}

char* BC_FileBox::get_delete_title()
{
	char *letter2 = strchr(title, ':');
	new_folder_title[0] = 0;
	if(letter2)
	{
		memcpy(new_folder_title, title, letter2 - title);
		new_folder_title[letter2 - title] = 0;
	}

	strcat(new_folder_title, _(": Delete"));

	return new_folder_title;
}

void BC_FileBox::delete_files()
{
// Starting at 1 causes it to ignore what's in the textbox.
	int i = 1;
	char *path;
	FileSystem fs;
	while((path = get_path(i)))
	{
// Not directory.  Remove it.
		if(!fs.is_dir(path))
		{
printf("BC_FileBox::delete_files: removing \"%s\"\n", path);
			remove(path);
		}
		i++;
	}
	refresh(0, 0);
}

BC_Button* BC_FileBox::get_ok_button()
{
	return ok_button;
}

BC_Button* BC_FileBox::get_cancel_button()
{
	return cancel_button;
}

