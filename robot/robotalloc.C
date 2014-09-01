#include "textfile.h"
#include "robotalloc.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>



// Layout of CD towers.



static tower_layout_t default_layout[] = 
{
	{  4, 1},
	{94,  0},
	{94,  0},
	{94,  0}
};


RobotAlloc::RobotAlloc()
{
	allocation = 0;
}

RobotAlloc::~RobotAlloc()
{
	delete_allocation();
}


void RobotAlloc::create_objects()
{
// Set layout to default layout
	towers = sizeof(default_layout) / sizeof(tower_layout_t);
	layout = new tower_layout_t[towers];
	for(int i = 0; i < towers; i++)
		memcpy(&layout[i], &default_layout[i], sizeof(tower_layout_t));


	create_allocation();
}

void RobotAlloc::create_allocation()
{
	delete_allocation();
	allocation = new char*[towers];
	for(int i = 0; i < towers; i++)
	{
		allocation[i] = new char[get_rows(i)];
		bzero(allocation[i], get_rows(i));
	}
	
}

void RobotAlloc::delete_allocation()
{
	if(allocation)
	{
		for(int i = 0; i < towers; i++)
		{
			delete [] allocation[i];
		}
		delete [] allocation;
	}

	allocation = 0;
}

void RobotAlloc::dump()
{
	printf("RobotAlloc::dump 1 towers=%d\n", towers);
	int max_rows = 0;
	for(int i = 0; i < towers; i++)
	{
		printf("    tower=%d rows=%d loading=%d\n",
			i,
			layout[i].rows,
			layout[i].loading);
		if(layout[i].rows > max_rows)
			max_rows = layout[i].rows;
	}

	for(int i = 0; i < max_rows; i++)
	{
		printf("row=%03d: ", i);
		for(int j = 0; j < towers; j++)
		{
			if(i < layout[j].rows)
				printf("%d ", allocation[j][i]);
			else
				printf("  ");
		}
		printf("\n");
	}
}



int RobotAlloc::get_rows(int tower)
{
	return layout[tower].rows;
}

int RobotAlloc::get_loading(int tower)
{
	return layout[tower].loading;
}


char* RobotAlloc::get_allocation(int tower)
{
	return allocation[tower];
}

int RobotAlloc::get_allocation(int tower, int row)
{
	return allocation[tower][row];
}

void RobotAlloc::free_tower_row(int tower, int row)
{
	char *allocation = get_allocation(tower);
	if(!allocation[row])
		fprintf(stderr, "RobotAlloc::free_tower_row: tower %d row %d not allocated.\n", tower, row);
	allocation[row] = 0;
}

int RobotAlloc::get_tower_row(int *tower, int *row)
{
	*tower = -1;
	*row = -1;
	for(int i = 0; i < towers; i++)
	{
		int rows = get_rows(i);
		char *allocation = get_allocation(i);

		if(!get_loading(i))
		{
			for(int j = 0; j < rows; j++)
			{
				if(!allocation[j])
				{
					*tower = i;
					*row = j;
					return 0;
				}
			}
		}
	}
	return 1;
}

int RobotAlloc::new_tower_row(int tower, int row, int show_error)
{
	char *allocation = get_allocation(tower);
	if(allocation[row] && show_error)
	{
		fprintf(stderr, "RobotAlloc::new_tower_row tower=%d row=%d already allocated.\n",
			tower,
			row);
		return 1;
	}
	allocation[row] = 1;
	return 0;
}

int RobotAlloc::get_loading_tower()
{
	for(int i = 0; i < towers; i++)
		if(layout[i].loading) return i;
	return 0;
}

int RobotAlloc::write_layout(TextFile *file)
{
	char string[BCTEXTLEN];
	file->write_record(towers, "LAYT", 0, 0);
	for(int i = 0; i < towers; i++)
	{
		sprintf(string, "%d %d", 
			layout[i].rows, 
			layout[i].loading);
		file->write_record(0, "TOWR", strlen(string), string);
	}
}

int RobotAlloc::read_layout(TextFile *file)
{
// This is for future use.
return 0;
	char string[BCTEXTLEN];
	int result = 0;

	delete [] layout;
	towers = file->subrecords;
	layout = new tower_layout_t[towers];
	for(int i = 0; i < towers && !result; i++)
	{
		result = file->read_record();
		if(!result)
		{
			tower_layout_t *entry = &layout[i];
			memcpy(string, file->data, file->size);
			string[file->size] = 0;
			sscanf(string, "%d", &entry->rows);

			char *ptr = strchr(string, ' ');
			if(ptr)
			{
				ptr++;
				sscanf(ptr, "%d", &entry->loading);
			}
		}
	}

	create_allocation();
	return result;
}




