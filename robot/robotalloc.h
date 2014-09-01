#ifndef ROBOTALLOC_H
#define ROBOTALLOC_H


#include "textfile.inc"

// Stores layout.
typedef struct tower_layout_t
{
	int rows;
	int loading;
};


// Stores 1 or 0 for each slot.

class RobotAlloc
{
public:
	RobotAlloc();
	~RobotAlloc();

	void create_objects();
	void create_allocation();
	void delete_allocation();

	void dump();

// Assign tower and row to new permanent CD.  Return 1 if full.
	int new_tower_row(int tower, int row, int show_error);

// Return a free tower and row.  Return 1 if full.
	int get_tower_row(int *tower, int *row);
	

// Deassign a tower number and row from an old permanent CD.
	void free_tower_row(int tower, int row);

// Get rows in tower
	int get_rows(int tower);

// Get whether a tower is loading or not
	int get_loading(int tower);

// Get the allocation vector for the tower
	char* get_allocation(int tower);

// Determine if the tower and row is allocated
	int get_allocation(int tower, int row);

// Get number of loading tower
	int get_loading_tower();

	int write_layout(TextFile *file);
	int read_layout(TextFile *file);

	char **allocation;
	int towers;
	tower_layout_t *layout;
};




#endif
