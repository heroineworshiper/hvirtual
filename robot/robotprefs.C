#include "defaults.h"
#include "guicast.h"
#include "mwindow.h"
#include "robotdb.h"
#include "robotprefs.h"

#include <string.h>

RobotPrefs::RobotPrefs(MWindow *mwindow)
{
	this->mwindow = mwindow;
}

static int default_widths[FIELDS] = 
{
	100,
	100,
	100,
	100,
	100,
	100
};

void RobotPrefs::load_defaults()
{
	char string[BCTEXTLEN];
	for(int i = 0; i < FIELDS; i++)
	{
		sprintf(string, "IN_WIDTH_%d", i);
		in_column_widths[i] = mwindow->defaults->get(string, default_widths[i]);

		sprintf(string, "OUT_WIDTH_%d", i);
		out_column_widths[i] = mwindow->defaults->get(string, default_widths[i]);

//printf("RobotPrefs::load_defaults 1 %d %d\n", in_column_widths[i], out_column_widths[i]);
		for(int j = 0; j < 2; j++)
		{
			sprintf(string, "IMPORT_WIDTH_%d_%d", j, i);
			import_column_widths[j][i] = 
				mwindow->defaults->get(string, default_widths[i]);

		}

		sprintf(string, "SEARCH_FIELD_%d", i);
		search_fields[i] = 0;
		search_fields[i] = mwindow->defaults->get(string, search_fields[i]);
	}

	strcpy(search_string, "heroine");
	mwindow->defaults->get("SEARCH_STRING", search_string);
	search_case = mwindow->defaults->get("SEARCH_CASE", 0);
	search_backward = mwindow->defaults->get("SEARCH_BACKWARD", 0);


	sort_order = mwindow->defaults->get("SORT_ORDER", SORT_NUMBER);

	import_path[0][0] = 0;
	import_path[1][0] = 0;
	mwindow->defaults->get("IMPORT_PATH0", import_path[0]);
	mwindow->defaults->get("IMPORT_PATH1", import_path[1]);
	mwindow->defaults->get("IMPORT_CHECK_IN", 0);
	import_check_in = mwindow->defaults->get("IMPORT_CHECK_IN", 1);

	check_in_src_row = mwindow->defaults->get("CHECK_IN_SRC_ROW", 0);
	check_in_dst_tower = mwindow->defaults->get("CHECK_IN_DST_TOWER", 0);
	check_in_dst_row = mwindow->defaults->get("CHECK_IN_DST_ROW", 0);
	
	check_out_src_tower = mwindow->defaults->get("CHECK_OUT_SRC_TOWER", 0);
	check_out_src_row = mwindow->defaults->get("CHECK_OUT_SRC_ROW", 0);
	check_out_dst_row = mwindow->defaults->get("CHECK_OUT_DST_ROW", 0);

	move_src_tower = mwindow->defaults->get("MOVE_SRC_TOWER", 0);
	move_src_row = mwindow->defaults->get("MOVE_SRC_ROW", 0);
	move_dst_tower = mwindow->defaults->get("MOVE_DST_TOWER", 0);
	move_dst_row = mwindow->defaults->get("MOVE_DST_ROW", 0);

	delete_tower = mwindow->defaults->get("DELETE_TOWER", 0);
	delete_row = mwindow->defaults->get("DELETE_ROW", 0);

	BC_WindowBase::get_resources()->filebox_w = mwindow->defaults->get("FILEBOX_W", 512);
	BC_WindowBase::get_resources()->filebox_h = mwindow->defaults->get("FILEBOX_H", 480);


	strcpy(db_path, ROBOTDIR "/heroine1120.db");
	mwindow->defaults->get("DB_PATH", db_path);
	strcpy(hostname, "artemis");
	mwindow->defaults->get("HOSTNAME", hostname);
	port = 500;
	port = mwindow->defaults->get("PORT", port);
}

void RobotPrefs::save_defaults()
{
	char string[BCTEXTLEN];
	for(int i = 0; i < FIELDS; i++)
	{
		sprintf(string, "IN_WIDTH_%d", i);
		mwindow->defaults->update(string, in_column_widths[i]);

		sprintf(string, "OUT_WIDTH_%d", i);
		mwindow->defaults->update(string, out_column_widths[i]);

//printf("RobotPrefs::save_defaults 1 %d %d\n", in_column_widths[i], out_column_widths[i]);
		for(int j = 0; j < 2; j++)
		{
			sprintf(string, "IMPORT_WIDTH_%d_%d", j, i);
			mwindow->defaults->update(string, import_column_widths[j][i]);
		}

		sprintf(string, "SEARCH_FIELD_%d", i);
		mwindow->defaults->update(string, search_fields[i]);
	}

	mwindow->defaults->update("SEARCH_STRING", search_string);
	mwindow->defaults->update("SEARCH_CASE", search_case);
	mwindow->defaults->update("SEARCH_BACKWARD", search_backward);

	mwindow->defaults->update("SORT_ORDER", sort_order);	

	mwindow->defaults->update("IMPORT_PATH0", import_path[0]);
	mwindow->defaults->update("IMPORT_PATH1", import_path[1]);
	mwindow->defaults->update("IMPORT_CHECK_IN", import_check_in);

	

	mwindow->defaults->update("CHECK_IN_SRC_ROW", check_in_src_row);
	mwindow->defaults->update("CHECK_IN_DST_TOWER", check_in_dst_tower);
	mwindow->defaults->update("CHECK_IN_DST_ROW", check_in_dst_row);
	
	mwindow->defaults->update("CHECK_OUT_SRC_TOWER", check_out_src_tower);
	mwindow->defaults->update("CHECK_OUT_SRC_ROW", check_out_src_row);
	mwindow->defaults->update("CHECK_OUT_DST_ROW", check_out_dst_row);



	mwindow->defaults->update("MOVE_SRC_TOWER", move_src_tower);
	mwindow->defaults->update("MOVE_SRC_ROW", move_src_row);
	mwindow->defaults->update("MOVE_DST_TOWER", move_dst_tower);
	mwindow->defaults->update("MOVE_DST_ROW", move_dst_row);

	mwindow->defaults->update("FILEBOX_W", BC_WindowBase::get_resources()->filebox_w);
	mwindow->defaults->update("FILEBOX_H", BC_WindowBase::get_resources()->filebox_h);

	mwindow->defaults->get("DELETE_TOWER", delete_tower);
	mwindow->defaults->get("DELETE_ROW", delete_row);

	mwindow->defaults->update("DB_PATH", db_path);
	mwindow->defaults->update("HOSTNAME", hostname);
	mwindow->defaults->update("PORT", port);
}

static char *sort_options[] =
{
	"Path",
	"Size",
	"Date",
	"Number",
	"Description"
};

char* RobotPrefs::number_to_sort(int number)
{
	return sort_options[number];
}

int RobotPrefs::sort_to_number(char *text)
{
	for(int i = 0; i < sizeof(sort_options) / sizeof(char*); i++)
	{
		if(!strcmp(sort_options[i], text)) return i;
	}
}

void RobotPrefs::copy_from(RobotPrefs *prefs)
{
	this->sort_order = prefs->sort_order;
	memcpy(this->in_column_widths, prefs->in_column_widths, sizeof(in_column_widths));
	memcpy(this->out_column_widths, prefs->out_column_widths, sizeof(out_column_widths));
	memcpy(this->import_column_widths, prefs->import_column_widths, sizeof(import_column_widths));
	memcpy(this->import_path, prefs->import_path, sizeof(import_path));

	memcpy(this->search_string, prefs->search_string, sizeof(search_string));
	memcpy(this->search_fields, prefs->search_fields, sizeof(search_fields));
	this->search_case = prefs->search_case;
	this->search_backward = prefs->search_backward;

	this->move_src_tower = prefs->move_src_tower;
	this->move_src_row = prefs->move_src_row;
	this->move_dst_tower = prefs->move_dst_tower;
	this->move_dst_row = prefs->move_dst_row;

	this->import_check_in = prefs->import_check_in;

	memcpy(this->db_path, prefs->db_path, sizeof(db_path));
	memcpy(this->hostname, prefs->hostname, sizeof(hostname));
	this->port = prefs->port;
}


void RobotPrefs::dump()
{
	printf("RobotPrefs::dump 1\n");
	printf("	search_string=%s\n", search_string);
	for(int i = 0; i < FIELDS; i++)
	{
		printf("	search_field %s = %d\n", 
			RobotDB::column_titles[i], 
			search_fields[i]);
	}
}






