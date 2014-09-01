#include "allocationgui.h"
#include "mwindow.h"
#include "robotalloc.h"




AllocationTower::AllocationTower(MWindow *mwindow, 
	AllocationGUI *gui,
	int x, 
	int y, 
	int w, 
	int h)
 : BC_ListBox(x, 
		y, 
		w, 
		h,
		LISTBOX_TEXT,                   // Display text list or icons
		&gui->tower_data,               // Each column has an ArrayList of BC_ListBoxItems.
		0,             // Titles for columns.  Set to 0 for no titles
		0,                // width of each column
		1,                      // Total columns.  Only 1 in icon mode
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_SINGLE,  // Select one item or multiple items
		ICON_LEFT,        // Position of icon relative to text of each item
		0)                  // Allow user to drag icons around
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int AllocationTower::handle_event()
{
	gui->parent_window->set_done(0);
	return 1;
}

int AllocationTower::selection_changed()
{
	BC_ListBoxItem *item = get_selection(0, 0);
	if(item)
	{
		gui->tower = atol(item->get_text());
		gui->update_row_table();
		gui->handle_event();
	}

	return 1;
}





AllocationRow::AllocationRow(MWindow *mwindow, 
	AllocationGUI *gui,
	int x, 
	int y, 
	int w, 
	int h)
 : BC_ListBox(x, 
		y, 
		w, 
		h,
		LISTBOX_TEXT,                   // Display text list or icons
		&gui->row_data,               // Each column has an ArrayList of BC_ListBoxItems.
		0,             // Titles for columns.  Set to 0 for no titles
		0,                // width of each column
		1,                      // Total columns.  Only 1 in icon mode
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_SINGLE,  // Select one item or multiple items
		ICON_LEFT,        // Position of icon relative to text of each item
		0)                  // Allow user to drag icons around
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int AllocationRow::handle_event()
{
	gui->parent_window->set_done(0);
	return 1;
}

int AllocationRow::selection_changed()
{
	BC_ListBoxItem *item = get_selection(0, 0);
	if(item)
	{
		gui->row = atol(item->get_text());
		gui->handle_event();
	}
	return 1;
}









AllocationGUI::AllocationGUI(MWindow *mwindow, 
	BC_WindowBase *parent_window,
	int x, 
	int y,
	int w,
	int h, 
	int tower, 
	int row,
	int destination,
	int use_tower,
	int use_loading)
{
	this->mwindow = mwindow;
	this->parent_window = parent_window;
	this->tower = default_tower = tower;
	this->row = default_row = row;
	this->destination = destination;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->use_tower = use_tower;
	this->use_loading = use_loading;
}

AllocationGUI::~AllocationGUI()
{
}

void AllocationGUI::create_objects()
{
// Generate list tables
	calculate_tower_table();
	calculate_row_table();

	int x1, y1, w1, h1;
	x1 = x;
	y1 = y + 20;
	h1 = h - 20;
	if(use_tower)
	{
		w1 = w / 2 - 5;
		parent_window->add_subwindow(tower_title = new BC_Title(x1, 
			y1 - 20,
			"Tower:"));
		parent_window->add_subwindow(tower_list = new AllocationTower(mwindow,
			this,
			x1,
			y1,
			w1,
			h1));
		x1 = x + w / 2 + 5;
	}
	else
	{
		w1 = w;
	}

	parent_window->add_subwindow(row_title = new BC_Title(x1, 
		y1 - 20,
		"Row:"));
	parent_window->add_subwindow(row_list = new AllocationRow(mwindow,
		this,
		x1,
		y1,
		w1,
		h1));
}

void AllocationGUI::reposition(int x, int y, int w, int h)
{
	int x1, y1, w1, h1;
	x1 = x;
	y1 = y + 20;
	h1 = h - 20;
	if(use_tower)
	{
		w1 = w / 2 - 5;
		tower_title->reposition_window(x1, y1 - 20);
		tower_list->reposition_window(x1, y1, w1, h1);
		x1 = x + w / 2 + 5;
	}
	else
	{
		w1 = w;
	}
	row_title->reposition_window(x1, y1 - 20);
	row_list->reposition_window(x1, y1, w1, h1);
}

void AllocationGUI::calculate_tower_table()
{
	for(int i = 0; i < mwindow->allocation->towers; i++)
	{
		if(!mwindow->allocation->get_loading(i) ||
			use_loading)
		{
			char string[BCTEXTLEN];
			BC_ListBoxItem *item;
			sprintf(string, "%d", i);
			tower_data.append(item = new BC_ListBoxItem(string));
			item->set_selected(i == tower);
		}
	}
}

void AllocationGUI::calculate_row_table()
{
	int rows = mwindow->allocation->get_rows(tower);
	row_data.remove_all_objects();

	for(int i = rows - 1; i >= 0; i--)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%d", i);
		int color;
		int allocation = mwindow->allocation->get_allocation(tower, i);

		color = (destination ? !allocation : allocation);

// printf("AllocationGUI::calculate_row_table 1 %d %d %d %d %d %d\n",
// color,
// use_loading,
// tower,
// default_tower,
// row,
// default_row);

		if(color ||
			use_loading ||
			(tower == default_tower &&
			i == default_row))
			color = BLACK;
		else
			color = MEGREY;




		BC_ListBoxItem *item;
		row_data.append(item = new BC_ListBoxItem(string, color));
		item->set_selectable(color == BLACK);
		item->set_selected(i == row);
	}
}

void AllocationGUI::update_row_table()
{
	calculate_row_table();
	row_list->update(&row_data,
		0,
		0,
		1,
		row_list->get_xposition(),
		row_list->get_yposition(),
		row_list->get_highlighted_item(),
		1);
}

int AllocationGUI::handle_event()
{
	return 1;
}
