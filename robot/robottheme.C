#include "bcresources.h"
#include "colors.h"
#include "defaults.h"
#include "mwindow.h"
#include "robottheme.h"
#include "vframe.h"




RobotTheme::RobotTheme(MWindow *mwindow)
 : BC_Theme()
{
	this->mwindow = mwindow;
}

void RobotTheme::load_defaults()
{
	mwindow_x = mwindow->defaults->get("MWINDOW_X", -1);
	mwindow_y = mwindow->defaults->get("MWINDOW_Y", -1);
	mwindow_w = mwindow->defaults->get("MWINDOW_W", 720);
	mwindow_h = mwindow->defaults->get("MWINDOW_H", 540);
	import_w = mwindow->defaults->get("IMPORT_W", 720);
	import_h = mwindow->defaults->get("IMPORT_H", 540);
	checkin_w = mwindow->defaults->get("CHECKIN_W", 640);
	checkin_h = mwindow->defaults->get("CHECKIN_H", 480);
	checkout_w = mwindow->defaults->get("CHECKOUT_W", 640);
	checkout_h = mwindow->defaults->get("CHECKOUT_H", 480);
	move_w = mwindow->defaults->get("MOVE_W", 640);
	move_h = mwindow->defaults->get("MOVE_H", 480);
	delete_w = mwindow->defaults->get("DELETE_W", 640);
	delete_h = mwindow->defaults->get("DELETE_H", 480);
}

void RobotTheme::save_defaults()
{
	mwindow->defaults->update("MWINDOW_X", mwindow_x);
	mwindow->defaults->update("MWINDOW_Y", mwindow_y);
	mwindow->defaults->update("MWINDOW_W", mwindow_w);
	mwindow->defaults->update("MWINDOW_H", mwindow_h);
	mwindow->defaults->update("IMPORT_W", import_w);
	mwindow->defaults->update("IMPORT_H", import_h);
	mwindow->defaults->update("CHECKIN_W", checkin_w);
	mwindow->defaults->update("CHECKIN_H", checkin_h);
	mwindow->defaults->update("CHECKOUT_W", checkout_w);
	mwindow->defaults->update("CHECKOUT_H", checkout_h);
	mwindow->defaults->update("MOVE_W", move_w);
	mwindow->defaults->update("MOVE_H", move_h);
	mwindow->defaults->update("DELETE_W", delete_w);
	mwindow->defaults->update("DELETE_H", delete_h);
}

void RobotTheme::initialize()
{
	extern unsigned char _binary_data_start[];
	set_data(_binary_data_start);

	get_resources()->bg_color = BLOND;
	get_resources()->button_up = 0xffc000;
	get_resources()->button_highlighted = 0xffe000;
	get_resources()->recursive_resizing = 0;
	get_resources()->generic_button_images = 
		new_image_set(3, "generic_up.png", "generic_hi.png", "generic_dn.png");
	get_resources()->hscroll_data = new_image_set(10, 
		"hscroll_center_up.png",
		"hscroll_center_hi.png",
		"hscroll_center_dn.png",
		"hscroll_bg.png",
		"hscroll_back_up.png",
		"hscroll_back_hi.png",
		"hscroll_back_dn.png",
		"hscroll_fwd_up.png",
		"hscroll_fwd_hi.png",
		"hscroll_fwd_dn.png");
	get_resources()->vscroll_data = new_image_set(10, 
		"vscroll_center_up.png",
		"vscroll_center_hi.png",
		"vscroll_center_dn.png",
		"vscroll_bg.png",
		"vscroll_back_up.png",
		"vscroll_back_hi.png",
		"vscroll_back_dn.png",
		"vscroll_fwd_up.png",
		"vscroll_fwd_hi.png",
		"vscroll_fwd_dn.png");
	get_resources()->ok_images = new_button("ok.png",
		"generic_up.png",
		"generic_hi.png",
		"generic_dn.png");
	get_resources()->cancel_images = new_button("cancel.png",
		"generic_up.png",
		"generic_hi.png",
		"generic_dn.png");
	get_resources()->listbox_button = new_button("down.png",
		"buttonbar_up.png",
		"buttonbar_hi.png",
		"buttonbar_dn.png");
	get_resources()->tumble_data = new_image_set(4,
		"tumble_up.png",
		"tumble_hi.png",
		"tumble_botdn.png",
		"tumble_topdn.png");
	get_resources()->progress_images = new_image_set(2,
		"progress_bg.png",
		"progress_hi.png");

	checkout = new_image_set(3,
		"checkout_up.png",
		"checkout_hi.png",
		"checkout_dn.png");
	checkin = new_image_set(3,
		"checkin_up.png",
		"checkin_hi.png",
		"checkin_dn.png");
	browse = new_button("browse.png",
		"buttonbar_up.png",
		"buttonbar_hi.png",
		"buttonbar_dn.png");
	abort = new_image_set(3,
		"abort_up.png",
		"abort_hi.png",
		"abort_dn.png");
	home = new_image_set(3,
		"home_up.png",
		"home_hi.png",
		"home_dn.png");
	logo = new_image("logo.png");
	splash_logo = new_image("splash_logo.png");
}

void RobotTheme::get_mwindow_sizes()
{
	int y = 5;
	int text_margin = 10;
	int scroll_margin = 20;

	logo_x = y;
	logo_y = y;
	y += logo->get_h() + 5;

	buttons_x = mwindow_w / 2 - 70;
	buttons_y = y + text_margin;
	buttons_w = 130;
	button_w = buttons_w;

	desc_x = 10;
	desc_y = mwindow_h - 100;
	desc_w = mwindow_w - 10 - desc_x;
	desc_h = mwindow_h - 30 - desc_y;
	savedesc_x = desc_x + 130;
	savedesc_y = desc_y - 25;

	message_x = 10;
	message_y = mwindow_h - 30;

	in_files_x = 10;
	in_files_y = y + text_margin;
	in_files_w = buttons_x - 20;
	in_files_h = desc_y - 50 - in_files_y - text_margin - scroll_margin;

	out_files_x = buttons_x + buttons_w + 10;
	out_files_y = y + text_margin;
	out_files_w = mwindow_w - 10 - out_files_x;
	out_files_h = in_files_h;

	info_x = desc_x;
	info_y = in_files_y + in_files_h + 10;

	abort_x = mwindow_w - abort[0]->get_w() - 5;
	abort_y = mwindow_h - abort[0]->get_h() - 5;
}


void RobotTheme::get_import_sizes()
{
	import_browse1_x = 10;
	import_browse1_y = 30;
	import_browse1_w = import_w - 300;

	import_list1_x = 10;
	import_list1_y = import_browse1_y + 50;
	import_list1_w = import_w - 20;
	import_list1_h = import_h / 3 - import_list1_y - 10;

	import_browse2_x = 10;
	import_browse2_y = import_h / 3;
	import_browse2_w = import_w - 300;

	import_list2_x = 10;
	import_list2_y = import_browse2_y + 60;
	import_list2_w = import_w - 20;
	import_list2_h = import_h * 2 / 3 - import_list2_y - 20;

	import_dest_x = 10;
	import_dest_y = import_list2_y + import_list2_h + 30;
	import_dest_w = import_w - 20;
	import_dest_h = import_h - import_dest_y - 50;

	import_ok_x = 10;
	import_ok_y = import_h - 40;

	import_checkin_x = import_w / 2 - 50;
	import_checkin_y = import_h - 40;
	import_cancel_x = import_w - 110;
	import_cancel_y = import_h - 40;
}

void RobotTheme::get_options_sizes()
{
	options_w = 480;
	options_h = 150;
	options_path_x = 10;
	options_path_y = 30;
	options_path_w = options_w;
	options_host_x = 10;
	options_host_y = options_path_y + 50;
	options_host_w = options_w - 200;
	options_port_x = options_host_x + options_host_w + 10;
	options_port_y = options_host_y;
	options_port_w = 150;
}

void RobotTheme::get_search_sizes()
{
	search_w = 480;
	search_h = 330;
	search_string_x = 10;
	search_string_y = 30;
	search_string_w = 460;
	search_case_x = 10;
	search_case_y = search_string_y + 30;
	search_fields_x = 10;
	search_fields_y = search_case_y + 80;
	search_ok_x = 10;
	search_ok_y = search_h - 30;
	search_again_x = search_w / 2 - 50;
	search_again_y = search_ok_y;
	search_cancel_x = search_w - 90;
	search_cancel_y = search_ok_y;
}

void RobotTheme::get_checkin_sizes()
{
	checkin_src_x = checkin_w / 2 + 5;
	checkin_src_y = 30;
	checkin_src_w = checkin_w / 2 - 15;
	checkin_src_h = checkin_h / 2 - 60;
	checkin_dst_x = 10;
	checkin_dst_y = checkin_src_y + checkin_src_h + 30;
	checkin_dst_w = checkin_w - 20;
	checkin_dst_h = checkin_h - checkin_dst_y - 40;
}

void RobotTheme::get_checkout_sizes()
{
	checkout_src_x = 10;
	checkout_src_y = 30;
	checkout_src_w = checkout_w - 20;
	checkout_src_h = checkout_h / 2 - 50;
	checkout_dst_x = checkout_w / 2 + 5;
	checkout_dst_y = checkout_src_y + checkout_src_h + 30;
	checkout_dst_w = checkout_w / 2 - 15;
	checkout_dst_h = checkout_h - checkout_dst_y - 40;
}

void RobotTheme::get_move_sizes()
{
	move_src_x = 10;
	move_src_y = 30;
	move_src_w = move_w - 20;
	move_src_h = move_h / 2 - move_src_y - 30;
	move_dst_x = 10;
	move_dst_y = move_h / 2;
	move_dst_w = move_w - 20;
	move_dst_h = move_h - move_dst_y - 50;
}
void RobotTheme::get_manual_sizes()
{
	manual_w = 640;
	manual_h = 480;
}

void RobotTheme::get_delete_sizes()
{
	delete_src_x = 10;
	delete_src_y = 30;
	delete_src_w = delete_w - 20;
	delete_src_h = delete_h - 70;
	delete_checkout_x = delete_w / 2 - 50;
	delete_checkout_y = delete_h - 30;
}






