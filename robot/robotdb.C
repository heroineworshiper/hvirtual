#include "filesystem.h"
#include "mwindow.h"
#include "robotalloc.h"
#include "robotdb.h"
#include "robotprefs.h"
#include "splashgui.h"
#include "textfile.h"
#include "vframe.h"

#include <string.h>


char* RobotDB::column_titles[FIELDS] = 
{
	"Path",
	"Desc",
	"Size",
	"Date",
	"Tower",
	"Row"
};



RobotDB::RobotDB(MWindow *mwindow)
{
	this->mwindow = mwindow;
}

RobotDB::~RobotDB()
{
	clear();
}

void RobotDB::clear()
{
	for(int i = 0; i < FIELDS; i++)
		data[i].remove_all_objects();
}

int RobotDB::load(TextFile *file)
{
	int result = 0;
	clear();

	if(!file->type_is("TABL"))
		result = file->read_record();

	int total = file->subrecords;

	for(int i = 0; i < total && !result; i++)
	{
		result = load_row(file, data, 0);
	}
	return result;
}

int RobotDB::save(TextFile *file)
{
	file->write_record(data[0].total, "TABL", 0, "");
	for(int i = 0; i < data[0].total; i++)
	{
		save_row(file, data, i);
	}
	return 0;
}

int RobotDB::load(char *path)
{
	char fullpath[BCTEXTLEN];
	FileSystem fs;
	strcpy(fullpath, path);
	fs.complete_path(fullpath);

	TextFile *file = new TextFile(fullpath, 1, 0);
	int result = file->read_db();
	if(!result)
	{
		load(file);
	}
	delete file;
	return result;
}

int RobotDB::save(char *path)
{
	char fullpath[BCTEXTLEN];
	FileSystem fs;
	strcpy(fullpath, path);
	fs.complete_path(fullpath);
	TextFile *file = new TextFile(fullpath, 0, 1);
	save(file);
	int result = file->write_db();
	delete file;
	return result;
}

int RobotDB::save_row(TextFile *file, ArrayList<BC_ListBoxItem*> *data, int number)
{
	BC_ListBoxItem *item = data[0].values[number];
	int result = 0;
	if(item->get_sublist())
	{
		ArrayList<BC_ListBoxItem*> *table = item->get_sublist();
		int total = table[0].total;
		file->write_record(total + 1, "DIR ", 0, "");
		save_entry(file, data, number);
		for(int i = 0; i < total; i++)
		{
			result = save_row(file, table, i);
		}
	}
	else
	{
		save_entry(file, data, number);
	}
	return result;
}

int RobotDB::save_entry(TextFile *file, ArrayList<BC_ListBoxItem*> *data, int number)
{
	file->write_record(FIELDS, "ITEM", 0, "");
	for(int i = 0; i < FIELDS; i++)
	{
		BC_ListBoxItem *item = data[i].values[number];
		file->write_record(0, field_to_fourcc(i), strlen(item->get_text()), item->get_text());
	}
	return 0;
}

int RobotDB::load_row(TextFile *file, 
	ArrayList<BC_ListBoxItem*> *data, 
	int is_dir)
{
	int result = 0;

	if(mwindow->splash_gui)
	{
		mwindow->splash_gui->update_progress_length(file->get_size());
		mwindow->splash_gui->update_progress_current(file->get_position());
//printf("RobotDB::load_row 1 %d %d\n", file->get_position(), file->get_size());
	}


	result = file->read_record();
	if(!result)
	{
		if(!file->type)
		{
			result = 1;
		}
		else
		if(!memcmp(file->type, "DIR ", 4))
		{
			result = load_directory(file, data, file->subrecords);
		}
		else
		if(!memcmp(file->type, "ITEM", 4))
		{
			result = load_entry(file, data, file->subrecords, is_dir);
		}
	}
	return result;
}

int RobotDB::load_directory(TextFile *file, 
	ArrayList<BC_ListBoxItem*> *data,
	int total)
{
	int result = 0;

// Load the directory entry itself
	result = load_row(file, data, 1);

// Load the sublist
	if(!result)
	{
		total--;
		int data_size = data[0].total;
		BC_ListBoxItem *last_item = data[0].values[data_size - 1];
		data = last_item->new_sublist(FIELDS);

		for(int i = 0; i < total && !result; i++)
		{
			result = load_row(file, data, 0);
		}
	}

	return result;
}


int RobotDB::load_entry(TextFile *file, 
	ArrayList<BC_ListBoxItem*> *data, 
	int fields,
	int is_dir)
{
	int result = 0;

	while(!result && fields)
	{
		result = file->read_record();

		if(!file->type)
		{
			result = 1;
		}
		else
		{
			int field = fourcc_to_field(file->type);
			if(field >= 0)
			{
				char *string = new char[file->size + 1];
				memcpy(string, file->data, file->size);
				string[file->size] = 0;

				int color = (is_dir ? BLUE : BLACK);
				BC_ListBoxItem *item = new BC_ListBoxItem(string, color);

				data[field].append(item);
				fields--;
			}
			else
				result = 1;
		}
	}
	return result;
}


ArrayList<BC_ListBoxItem*>* RobotDB::get_sublist(ArrayList<BC_ListBoxItem*> *parent_sublist, 
	BC_ListBoxItem *last_item)
{
	ArrayList<BC_ListBoxItem*> *result =  0;
	for(int i = 0; i < parent_sublist->total; i++)
	{
		if(parent_sublist->values[PATH] == last_item)
		{
			return parent_sublist;
		}
		else
		if(parent_sublist->values[PATH]->get_sublist())
		{
			return get_sublist(parent_sublist->values[PATH]->get_sublist(),
				last_item);
		}
	}
}

char* RobotDB::field_to_fourcc(int field)
{
	switch(field)
	{
		case PATH:  return "PATH"; break;
		case SIZE:  return "SIZE"; break;
		case DATE:  return "DATE"; break;
		case DESC:  return "DESC"; break;
		case TOWER: return "TOWR"; break;
		case ROW:   return "ROW "; break;
	}
}

int RobotDB::fourcc_to_field(char *fourcc)
{
	if(!fourcc) return -1;

	if(!memcmp(fourcc, "PATH", 4)) 
		return PATH;
	else
	if(!memcmp(fourcc, "SIZE", 4))
		return SIZE;
	else
	if(!memcmp(fourcc, "DATE", 4))
		return DATE;
	else
	if(!memcmp(fourcc, "DESC", 4))
		return DESC;
	else
	if(!memcmp(fourcc, "TOWR", 4))
		return TOWER;
	else
	if(!memcmp(fourcc, "ROW ", 4))
		return ROW;

	return -1;
}

BC_ListBoxItem* RobotDB::new_entry(char *path, 
		int64_t size, 
		int64_t date,
		int tower,
		int row,
		char *desc,
		int is_directory,
		ArrayList<BC_ListBoxItem*> *sublist)
{
	char string[BCTEXTLEN];
	struct tm *broken_time;
	time_t calendar_time = date;
	BC_ListBoxItem *result;
	int color = (is_directory ? BLUE : BLACK);

	sublist[PATH].append(result = new BC_ListBoxItem(path, color));

	if(size >= 0)
		sprintf(string, "%lld", size);
	else
		string[0] = 0;
	sublist[SIZE].append(new BC_ListBoxItem(string, color));
	
	if(calendar_time <= 0)
	{
		string[0] = 0;
	}
	else
	{
		broken_time = localtime(&calendar_time);
		sprintf(string, "%d/%d/%d", 
			broken_time->tm_mon + 1, 
			broken_time->tm_mday, 
			broken_time->tm_year + 1900);
	}
	sublist[DATE].append(new BC_ListBoxItem(string, color));

	if(tower >= 0)
		sprintf(string, "%d", tower);
	else
		sprintf(string, "TBD");
	sublist[TOWER].append(new BC_ListBoxItem(string, color));

	if(row >= 0)
		sprintf(string, "%d", row);
	else
		sprintf(string, "TBD");
	sublist[ROW].append(new BC_ListBoxItem(string, color));

	sublist[DESC].append(new BC_ListBoxItem(desc, color));

	return result;
}


void RobotDB::import_dir(char *path, 
	BC_ListBoxItem *item, 
	int tower, 
	int row)
{
	FileSystem fs;
	fs.update(path);
	ArrayList<BC_ListBoxItem*> *sublist = item->new_sublist(FIELDS);

	for(int i = 0; i < fs.total_files(); i++)
	{
		FileItem *file_item = fs.get_entry(i);
		char *new_path = file_item->path;
		char *relative_path = file_item->name;
		if(relative_path[0] == '/') relative_path++;

		if(file_item->is_dir)
		{
			BC_ListBoxItem *item = new_entry(new_path,
				-1,
				-1,
				tower,
				row,
				"",
				file_item->is_dir,
				sublist);
			import_dir(new_path, item, tower, row);
		}
		else
		{
// Get the part after the import path
			BC_ListBoxItem *item = new_entry(relative_path,
				file_item->size,
				file_item->calendar_time,
				tower,
				row,
				"",
				file_item->is_dir,
				sublist);
		}
	}
}

void RobotDB::import_cd(char *path)
{

	BC_ListBoxItem *item = new_entry("CD",
		-1,
		0,
		-1,
		-1,
		"",
		1,
		data);


	FileSystem fs;
	fs.complete_path(path);
	import_dir(path, item, -1, -1);

}

void RobotDB::append_db(RobotDB *db, int row, int tower, int select_first)
{
	ArrayList<BC_ListBoxItem*> *src_data = db->data;
	for(int i = 0; i < src_data[0].total; i++)
	{
		int got_it = 0;


		if(row < 0 && tower < 0)
			got_it = 1;
		else
		{
			BC_ListBoxItem *row_item = src_data[ROW].values[i];
			if(atol(row_item->get_text()) == row)
			{
				BC_ListBoxItem *tower_item = src_data[TOWER].values[i];
				if(atol(tower_item->get_text()) == tower)
				{
					got_it = 1;
				}
			}
		}

		if(got_it)
		{
			for(int j = 0; j < FIELDS; j++)
			{
				ArrayList<BC_ListBoxItem*> *src_column = &src_data[j];
				ArrayList<BC_ListBoxItem*> *dst_column = &data[j];

				BC_ListBoxItem *new_item = new BC_ListBoxItem;
				BC_ListBoxItem *old_item = src_column->values[i];

				new_item->copy_from(old_item);
				dst_column->append(new_item);
				if(select_first && i == 0)
					new_item->set_selected(1);
			}
		}
	}
}

void RobotDB::set_tower_row(int tower, 
	int row, 
	ArrayList<BC_ListBoxItem*> *data)
{
	if(!data) data = this->data;
	ArrayList<BC_ListBoxItem*> *master_list = &data[0];
	ArrayList<BC_ListBoxItem*> *tower_list = &data[TOWER];
	ArrayList<BC_ListBoxItem*> *row_list = &data[ROW];
	char string[BCTEXTLEN];

	for(int i = 0; i < master_list->total; i++)
	{
		sprintf(string, "%d", tower);
		tower_list->values[i]->set_text(string);

		sprintf(string, "%d", row);
		row_list->values[i]->set_text(string);

		ArrayList<BC_ListBoxItem*> *sublist = master_list->values[i]->get_sublist();
		if(sublist)
		{
			set_tower_row(tower,
				row,
				sublist);
		}
	}
}

void RobotDB::set_allocation()
{
	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *row_item = data[ROW].values[i];
		BC_ListBoxItem *tower_item = data[TOWER].values[i];
		int row = atol(row_item->get_text());
		int tower = atol(tower_item->get_text());
		mwindow->allocation->new_tower_row(tower, row, 0);
	}
}

void RobotDB::remove_cd(int row, int tower, ArrayList<BC_ListBoxItem*> *data)
{
	if(!data) data = this->data;
	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *row_item = data[ROW].values[i];
		if(atol(row_item->get_text()) == row)
		{
			BC_ListBoxItem *tower_item = data[TOWER].values[i];
			if(atol(tower_item->get_text()) == tower)
			{
// Got it.  Descend into sublist
				BC_ListBoxItem *item = data[0].values[i];
				if(item->get_sublist())
				{
					remove_cd(row, tower, item->get_sublist());
				}

// Remove from current table
				for(int j = 0; j < FIELDS; j++)
				{
					data[j].remove_object_number(i);
				}
				i--;
			}
		}
	}
}

void RobotDB::move_cd(int row1, 
	int tower1, 
	int row2, 
	int tower2, 
	ArrayList<BC_ListBoxItem*> *data)
{
	if(!data) data = this->data;
	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *row_item = data[ROW].values[i];
		if(atol(row_item->get_text()) == row1)
		{
			BC_ListBoxItem *tower_item = data[TOWER].values[i];
			if(atol(tower_item->get_text()) == tower1)
			{
// Got it.  Descend into sublist
				BC_ListBoxItem *item = data[0].values[i];
				if(item->get_sublist())
				{
					move_cd(row1, 
						tower1, 
						row2, 
						tower2, 
						item->get_sublist());
				}

				char string[BCTEXTLEN];
				sprintf(string, "%d", tower2);
				data[TOWER].values[i]->set_text(string);
				sprintf(string, "%d", row2);
				data[ROW].values[i]->set_text(string);
			}
		}
	}
}


int RobotDB::exists(int row, int tower)
{
	for(int i = 0; i <data[0].total; i++)
	{
		BC_ListBoxItem *row_item = data[ROW].values[i];
		if(atol(row_item->get_text()) == row)
		{
			BC_ListBoxItem *tower_item = data[TOWER].values[i];
			if(atol(tower_item->get_text()) == tower)
			{
				return 1;
			}
		}
	}
	return 0;
}


int RobotDB::copy_field(ArrayList<BC_ListBoxItem*> *src_data,
	BC_ListBoxItem *src_item,
	int field)
{
// Get location of src_item
	int src_row = -1;
	int src_tower = -1;
	int src_number = -1;

	int location_result = get_location(src_item, 
		field, 
		&src_tower, 
		&src_row, 
		src_data);

// Get the item's position in the pancake.
	if(location_result)
	{
		int pancake_result = get_pancake_location(src_item,
			field,
			src_tower,
			src_row,
			&src_number,
			src_data);

// Get destination item
		if(pancake_result)
		{
			BC_ListBoxItem *dst_item = get_item(field,
				src_tower,
				src_row,
				src_number);

// Copy text
			if(dst_item)
			{
				dst_item->set_text(src_item->get_text());
				return 1;
			}
		}
	}

// Dst item was not found
	return 0;
}

int RobotDB::get_location(BC_ListBoxItem *item, 
		int field,
		int *tower,
		int *row,
		ArrayList<BC_ListBoxItem*> *data)
{
	for(int i = 0; i < data[0].total; i++)
	{
// Got it
		if(data[field].values[i] == item)
		{
			BC_ListBoxItem *tower_item = data[TOWER].values[i];
			BC_ListBoxItem *row_item = data[ROW].values[i];
			*tower = atol(tower_item->get_text());
			*row = atol(row_item->get_text());
			return 1;
		}

// Descend
		BC_ListBoxItem *first_item = data[0].values[i];
		if(first_item->get_sublist())
		{
			int result = get_location(item,
				field,
				tower,
				row,
				first_item->get_sublist());
			if(result) return result;
		}
	}
	return 0;
}


int RobotDB::get_pancake_location(BC_ListBoxItem *item,
		int field,
		int tower,
		int row,
		int *pancake_number,
		ArrayList<BC_ListBoxItem*> *data,
		int *counter)
{
	int temp = -1;
	if(!counter)
		counter = &temp;

	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *row_item = data[ROW].values[i];

		if(atol(row_item->get_text()) == row)
		{
			BC_ListBoxItem *tower_item = data[TOWER].values[i];
			if(atol(tower_item->get_text()) == tower)
			{
				(*counter)++;

// Got it
				if(data[field].values[i] == item)
				{
					*pancake_number = *counter;
					return 1;
				}
			}
		}

// Descend
		BC_ListBoxItem *first_item = data[0].values[i];
		if(first_item->get_sublist())
		{
			int result = get_pancake_location(item,
				field,
				tower,
				row,
				pancake_number,
				first_item->get_sublist(),
				counter);
			if(result) return result;
		}
	}

	return 0;
}


BC_ListBoxItem* RobotDB::get_item(int field,
				int tower,
				int row,
				int number,
				ArrayList<BC_ListBoxItem*> *data,
				int *counter)
{
	if(!data) data = this->data;
	int temp = -1;
	if(!counter)
		counter = &temp;


	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *row_item = data[ROW].values[i];
		if(atol(row_item->get_text()) == row)
		{
			BC_ListBoxItem *tower_item = data[TOWER].values[i];
			if(atol(tower_item->get_text()) == tower)
			{
				(*counter)++;

// Got it
				if((*counter) == number)
				{
					return data[field].values[i];
				}
			}
		}

// Descend
		BC_ListBoxItem *first_item = data[0].values[i];
		if(first_item->get_sublist())
		{
			BC_ListBoxItem *result = get_item(field,
				tower,
				row,
				number,
				first_item->get_sublist(),
				counter);
			if(result) return result;
		}
	}

	return 0;
}

void RobotDB::set_selected(int value)
{
	if(data[0].total)
	{
		for(int i = 0; i < FIELDS; i++)
			data[i].values[0]->set_selected(value);
	}
}

void RobotDB::deselect_all(ArrayList<BC_ListBoxItem*> *data)
{
	if(!data) data = this->data;
	for(int i = 0; i < data[0].total; i++)
	{
		for(int j = 0; j < FIELDS; j++)
			data[j].values[i]->set_selected(0);
		BC_ListBoxItem *item = data[0].values[i];
		if(item->get_sublist())
			deselect_all(item->get_sublist());
	}
}





int RobotDB::do_search(int search_position,
	int *counter, 
	ArrayList<BC_ListBoxItem*> *data)
{
	int temp;
	if(!data) data = this->data;
	if(!counter)
	{
		counter = &temp;
		if(mwindow->prefs->search_backward)
			(*counter) = BC_ListBox::get_total_items(data, 0, 0);
		else
			(*counter) = -1;
	}

	if(mwindow->prefs->search_backward)
	{
		for(int i = data[0].total - 1; i >= 0; i--)
		{
			BC_ListBoxItem *item = data[0].values[i];
			int result;
// Get sublist
			if(item->get_sublist())
			{
				result = do_search(search_position, counter, item->get_sublist());

// Got it in sublist
				if(result != search_position)
					return result;
			}
			(*counter)--;

// Inside valid search range
			if((*counter) < search_position)
			{
// Got it.
				if(search_matches(data, i)) return (*counter);
			}
		}
	}
	else
	{
		for(int i = 0; i < data[0].total; i++)
		{
			(*counter)++;
// Inside valid search range
			if((*counter) > search_position)
			{
// Got it.
				if(search_matches(data, i)) return (*counter);
			}

			BC_ListBoxItem *item = data[0].values[i];
			int result;

// Get sublist
			if(item->get_sublist())
			{
				result = do_search(search_position, counter, item->get_sublist());

// Got it in sublist
				if(result != search_position)
					return result;
			}
		}
	}

	return search_position;
}

int RobotDB::search_matches(ArrayList<BC_ListBoxItem*> *data,
	int number)
{
	int got_it = 0;
	for(int i = 0; i < FIELDS && !got_it; i++)
	{
		if(mwindow->prefs->search_fields[i])
		{
			BC_ListBoxItem *item = data[i].values[number];

			if(mwindow->prefs->search_case)
				got_it = !!strstr(item->get_text(), mwindow->prefs->search_string);
			else
				got_it = !!strcasestr(item->get_text(), mwindow->prefs->search_string);
		}
	}
	return got_it;
}

int RobotDB::select_and_focus(int number,
	int *counter, 
	ArrayList<BC_ListBoxItem*> *data)
{
	int temp;
	if(!data) data = this->data;
	if(!counter)
	{
		counter = &temp;
		(*counter) = BC_ListBox::get_total_items(data, 0, 0);
	}

	for(int i = data[0].total - 1; i >= 0; i--)
	{
// Descend into sublist
		BC_ListBoxItem *item = data[0].values[i];
		if(item->get_sublist())
		{
			int result = select_and_focus(number,
				counter,
				item->get_sublist());

// Expand if it got it
			if(result)
			{
				item->set_expand(1);
				return 1;
			}
		}


		(*counter)--;

// Got it
		if((*counter) == number)
		{
			for(int j = 0; j < FIELDS; j++)
				data[j].values[i]->set_selected(1);
			return 1;
		}
	}

	return 0;
}



void RobotDB::sort(ArrayList<BC_ListBoxItem*> *data)
{
	int done = 0;
	int iteration = 0;
	if(!data) data = this->data;


	while(!done)
	{
		done = 1;

		for(int i = 0; i < data[0].total - 1; i++)
		{
// Swap items
			if(compare_items(data, i))
			{
				for(int j = 0; j < FIELDS; j++)
				{
					BC_ListBoxItem *item1 = data[j].values[i];
					BC_ListBoxItem *item2 = data[j].values[i + 1];
					data[j].values[i] = item2;
					data[j].values[i + 1] = item1;
				}
				done = 0;
			}

// Sort sublist in the first iteration only
			if(!iteration)
			{
				BC_ListBoxItem *item = data[0].values[i];
				if(item->get_sublist())
				{
					sort(item->get_sublist());
				}
				if(i >= data[0].total - 2)
				{
					BC_ListBoxItem *item = data[0].values[i + 1];
					if(item->get_sublist())
					{
						sort(item->get_sublist());
					}
				}
			}

		}
		iteration++;
	}
}




int RobotDB::compare_items(ArrayList<BC_ListBoxItem*> *data, int number)
{
	BC_ListBoxItem *item1;
	BC_ListBoxItem *item2;
	switch(mwindow->prefs->sort_order)
	{
		case RobotPrefs::SORT_PATH:
			item1 = data[PATH].values[number];
			item2 = data[PATH].values[number + 1];
			return (strcmp(item1->get_text(), item2->get_text()) > 0);
			break;
		case RobotPrefs::SORT_DESC:
			item1 = data[DESC].values[number];
			item2 = data[DESC].values[number + 1];
			return (strcmp(item1->get_text(), item2->get_text()) > 0);
			break;
		case RobotPrefs::SORT_SIZE:
			item1 = data[SIZE].values[number];
			item2 = data[SIZE].values[number + 1];
			return (atol(item1->get_text()) > atol(item2->get_text()));
			break;
		case RobotPrefs::SORT_DATE:
		{
			item1 = data[DATE].values[number];
			item2 = data[DATE].values[number + 1];
			char *item1_text = item1->get_text();
			char *item2_text = item2->get_text();
			char *ptr = item1_text;

			int item1_month = atol(ptr);
			while(*ptr)
			{
				if(*ptr == '/')
					break;
				else
					ptr++;
			}
			int item1_day = atol(ptr);
			while(*ptr)
			{
				if(*ptr == '/')
					break;
				else
					ptr++;
			}
			int item1_year = atol(ptr);

			ptr = item2_text;
			int item2_month = atol(ptr);
			while(*ptr)
			{
				if(*ptr == '/')
					break;
				else
					ptr++;
			}
			int item2_day = atol(ptr);
			while(*ptr)
			{
				if(*ptr == '/')
					break;
				else
					ptr++;
			}
			int item2_year = atol(ptr);

			return (item1_year * 1000 + item1_month * 100 + item1_day >
				item2_year * 1000 + item2_month * 100 + item2_day);
			break;
		}
		case RobotPrefs::SORT_NUMBER:
		{
			item1 = data[TOWER].values[number];
			item2 = data[TOWER].values[number + 1];
			int tower1 = atol(item1->get_text());
			int tower2 = atol(item2->get_text());


			if(tower1 > tower2)
				return 1;
			else
			if(tower1 == tower2)
			{
				item1 = data[ROW].values[number];
				item2 = data[ROW].values[number + 1];
				if(atol(item1->get_text()) > atol(item2->get_text()))
					return 1;
			}

			break;
		}
	}


	return 0;
}

