#ifndef ROBOTTHEME_H
#define ROBOTTHEME_H

#include "bctheme.h"
#include "mwindow.inc"
#include "robotdb.inc"

class RobotTheme : public BC_Theme
{
public:
	RobotTheme(MWindow *mwindow);
	
	void initialize();

	void get_mwindow_sizes();
	void get_import_sizes();
	void get_options_sizes();
	void get_search_sizes();
	void get_checkin_sizes();
	void get_checkout_sizes();
	void get_move_sizes();
	void get_delete_sizes();
	void get_manual_sizes();


	void load_defaults();
	void save_defaults();

	MWindow *mwindow;
	int mwindow_x, mwindow_y, mwindow_w, mwindow_h;
	int in_files_x, in_files_y, in_files_w, in_files_h;
	int out_files_x, out_files_y, out_files_w, out_files_h;
	int info_x, info_y;
	int desc_x, desc_y, desc_w, desc_h;
	int buttons_x, buttons_y, buttons_w, button_w;
	int logo_x, logo_y;
	int message_x, message_y;
	int abort_x, abort_y;
	int savedesc_x, savedesc_y;

	int import_w, import_h;
	int import_list1_x, import_list1_y, import_list1_w, import_list1_h;
	int import_list2_x, import_list2_y, import_list2_w, import_list2_h;
	int import_browse1_x, import_browse1_y, import_browse1_w;
	int import_browse2_x, import_browse2_y, import_browse2_w;
	int import_dest_x, import_dest_y, import_dest_w, import_dest_h;
	int import_ok_x, import_ok_y;
	int import_checkin_x, import_checkin_y;
	int import_cancel_x, import_cancel_y;

	int manual_w, manual_h;

	int options_w, options_h;
	int options_path_x, options_path_y, options_path_w;
	int options_host_x, options_host_y, options_host_w;
	int options_port_x, options_port_y, options_port_w;

	int search_w, search_h;
	int search_string_x, search_string_y, search_string_w;
	int search_fields_x, search_fields_y;
	int search_case_x, search_case_y;
	int search_ok_x, search_ok_y;
	int search_again_x, search_again_y;
	int search_cancel_x, search_cancel_y;

	int checkin_w, checkin_h;
	int checkin_src_x, checkin_src_y, checkin_src_w, checkin_src_h;
	int checkin_dst_x, checkin_dst_y, checkin_dst_w, checkin_dst_h;

	int checkout_w, checkout_h;
	int checkout_src_x, checkout_src_y, checkout_src_w, checkout_src_h;
	int checkout_dst_x, checkout_dst_y, checkout_dst_w, checkout_dst_h;

	int move_w, move_h;
	int move_src_x, move_src_y, move_src_w, move_src_h;
	int move_dst_x, move_dst_y, move_dst_w, move_dst_h;

	int delete_w, delete_h;
	int delete_src_x, delete_src_y, delete_src_w, delete_src_h;
	int delete_checkout_x, delete_checkout_y;




	VFrame **checkout;
	VFrame **checkin;
	VFrame **browse;
	VFrame **abort;
	VFrame **home;
	VFrame *logo;
	VFrame *splash_logo;
};



#endif
