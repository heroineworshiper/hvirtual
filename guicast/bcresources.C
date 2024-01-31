/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "bcipc.h"
#include "bclistbox.inc"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bctheme.h"
#include "bcwindowbase.h"
#include "colors.h"
//#include "bccmodels.h"
#include "fonts.h"
#include "language.h"
#include "vframe.h"

#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <unistd.h>
 




int BC_Resources::error = 0;

VFrame* BC_Resources::bg_image = 0;
VFrame* BC_Resources::menu_bg = 0;
BC_Theme* BC_Resources::theme = 0;


int BC_Resources::override_dpi = 0;
int BC_Resources::dpi = BASE_DPI;


suffix_to_type_t BC_Resources::suffix_to_type[] = 
{
	{ "m2v", ICON_FILM },
	{ "mov", ICON_FILM },
	{ "mp2", ICON_SOUND },
	{ "mp3", ICON_SOUND },
	{ "mpg", ICON_FILM },
	{ "vob", ICON_FILM },
	{ "wav", ICON_SOUND }
};

BC_Signals* BC_Resources::signal_handler = 0;
int BC_Resources::initialized = 0;

const char* BC_Resources::small_font;
const char* BC_Resources::medium_font;
const char* BC_Resources::large_font;
const char* BC_Resources::clock_font;
int BC_Resources::large_fontsize;
int BC_Resources::medium_fontsize;
int BC_Resources::small_fontsize;
int BC_Resources::clock_fontsize;
Mutex* BC_Resources::xft_lock;

//const char* BC_Resources::small_fontset;
// const char* BC_Resources::medium_fonts;
// const char* BC_Resources::large_fontset;
// const char* BC_Resources::clock_fontset;


const char* BC_Resources::small_font_xft;
const char* BC_Resources::medium_font_xft;
const char* BC_Resources::large_font_xft;
const char* BC_Resources::clock_font_xft;
double BC_Resources::large_font_xftsize;
double BC_Resources::medium_font_xftsize;
double BC_Resources::small_font_xftsize;
double BC_Resources::clock_font_xftsize;



int BC_Resources::x_error_handler(Display *display, XErrorEvent *event)
{
// 	char string[1024];
// 	XGetErrorText(event->display, event->error_code, string, 1024);
// 	printf("BC_Resources::x_error_handler: error_code=%d opcode=%d,%d %s\n", 
// 		event->error_code, 
// 		event->request_code,
// 		event->minor_code,
// 		string);
//    printf("BC_Resources::x_error_handler %d display=%p\n", __LINE__, display);

	BC_Resources::error = 1;

// This bug only happens in 32 bit mode.
	if(sizeof(long) == 4)
		BC_WindowBase::get_resources()->use_xft = 0;
	return 0;
}



BC_Resources::BC_Resources()
{
    vframe_shm = 0;
}

BC_Resources::~BC_Resources()
{
}

void BC_Resources::init()
{
	if(!initialized)
	{
		initialized = 1;
		

		synchronous = 0;
		display_info = new BC_DisplayInfo((char*)"", 0);

// get DPI from BC_DisplayInfo
		if(!override_dpi)
		{
			dpi = display_info->dpi;
		}
		
		id_lock = new Mutex("BC_Resources::id_lock");
		create_window_lock = new Mutex("BC_Resources::create_window_lock", 1);
		xft_lock = new Mutex("BC_Resources::xft_lock", 1);
		id = 0;
		filebox_id = 0;

// create a default theme
        theme = new BC_Theme;

		for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
			filebox_history[i].path[0] = 0;

#ifdef HAVE_XFT
		XftInitFtLibrary();
#endif
		init_fonts();

		use_xvideo = 1;

	#include "images/file_film_png.h"
	#include "images/file_folder_png.h"
	#include "images/file_sound_png.h"
	#include "images/file_unknown_png.h"
	#include "images/file_column_png.h"
		static VFrame* default_type_to_icon[] = 
		{
			new VFrame(file_folder_png),
			new VFrame(file_unknown_png),
			new VFrame(file_film_png),
			new VFrame(file_sound_png),
			new VFrame(file_column_png)
		};
		type_to_icon = default_type_to_icon;


	#include "images/bar_png.h"
		static VFrame* default_bar = new VFrame(bar_png);
		bar_data = default_bar;


	#include "images/cancel_up_png.h"
	#include "images/cancel_hi_png.h"
	#include "images/cancel_dn_png.h"
		static VFrame* default_cancel_images[] = 
		{
			new VFrame(cancel_up_png),
			new VFrame(cancel_hi_png),
			new VFrame(cancel_dn_png)
		};

	#include "images/ok_up_png.h"
	#include "images/ok_hi_png.h"
	#include "images/ok_dn_png.h"
		static VFrame* default_ok_images[] = 
		{
			new VFrame(ok_up_png),
			new VFrame(ok_hi_png),
			new VFrame(ok_dn_png)
		};

	#include "images/usethis_up_png.h"
	#include "images/usethis_uphi_png.h"
	#include "images/usethis_dn_png.h"
		static VFrame* default_usethis_images[] = 
		{
			new VFrame(usethis_up_png),
			new VFrame(usethis_uphi_png),
			new VFrame(usethis_dn_png)
		};


	#include "images/checkbox_checked_png.h"
	#include "images/checkbox_dn_png.h"
	#include "images/checkbox_checkedhi_png.h"
	#include "images/checkbox_up_png.h"
	#include "images/checkbox_hi_png.h"
		static VFrame* default_checkbox_images[] =  
		{
			new VFrame(checkbox_up_png),
			new VFrame(checkbox_hi_png),
			new VFrame(checkbox_checked_png),
			new VFrame(checkbox_dn_png),
			new VFrame(checkbox_checkedhi_png)
		};

	#include "images/radial_checked_png.h"
	#include "images/radial_dn_png.h"
	#include "images/radial_checkedhi_png.h"
	#include "images/radial_up_png.h"
	#include "images/radial_hi_png.h"
		static VFrame* default_radial_images[] =  
		{
			new VFrame(radial_up_png),
			new VFrame(radial_hi_png),
			new VFrame(radial_checked_png),
			new VFrame(radial_dn_png),
			new VFrame(radial_checkedhi_png)
		};

		static VFrame* default_label_images[] =  
		{
			new VFrame(radial_up_png),
			new VFrame(radial_hi_png),
			new VFrame(radial_checked_png),
			new VFrame(radial_dn_png),
			new VFrame(radial_checkedhi_png)
		};

	#include "images/check_png.h"
		static VFrame* default_check_image = new VFrame(check_png);
		check = default_check_image;


	#include "images/file_rename_up_png.h"
	#include "images/file_rename_hi_png.h"
	#include "images/file_rename_dn_png.h"
	#include "images/file_updir_up_png.h"
	#include "images/file_updir_hi_png.h"
	#include "images/file_updir_dn_png.h"
	#include "images/file_delete_up_png.h"
	#include "images/file_delete_hi_png.h"
	#include "images/file_delete_dn_png.h"
// 	#include "images/file_text_up_png.h"
// 	#include "images/file_text_hi_png.h"
// 	#include "images/file_text_dn_png.h"
// 	#include "images/file_icons_up_png.h"
// 	#include "images/file_icons_hi_png.h"
// 	#include "images/file_icons_dn_png.h"
// 		static VFrame* default_filebox_text_images[] = 
// 		{
// 			new VFrame(file_text_up_png),
// 			new VFrame(file_text_hi_png),
// 			new VFrame(file_text_dn_png)
// 		};
// 
// 		static VFrame* default_filebox_icons_images[] = 
// 		{
// 			new VFrame(file_icons_up_png),
// 			new VFrame(file_icons_hi_png),
// 			new VFrame(file_icons_dn_png)
// 		};

		static VFrame* default_filebox_updir_images[] =  
		{
			new VFrame(file_updir_up_png),
			new VFrame(file_updir_hi_png),
			new VFrame(file_updir_dn_png)
		};
		filebox_updir_images = default_filebox_updir_images;

	#include "images/file_newfolder_up_png.h"
	#include "images/file_newfolder_hi_png.h"
	#include "images/file_newfolder_dn_png.h"
		static VFrame* default_filebox_newfolder_images[] = 
		{
			new VFrame(file_newfolder_up_png),
			new VFrame(file_newfolder_hi_png),
			new VFrame(file_newfolder_dn_png)
		};
		filebox_newfolder_images = default_filebox_newfolder_images;

	#include "images/file_preview_up_png.h"
	#include "images/file_preview_hi_png.h"
	#include "images/file_preview_dn_png.h"
	#include "images/file_preview_checked_png.h"
	#include "images/file_preview_checkedhi_png.h"
		static VFrame* default_filebox_preview_images[] = 
		{
			new VFrame(file_preview_up_png),
			new VFrame(file_preview_hi_png),
            new VFrame(file_preview_checked_png),
			new VFrame(file_preview_dn_png),
            new VFrame(file_preview_checkedhi_png)
		};
		filebox_preview_images = default_filebox_preview_images;


		static VFrame* default_filebox_rename_images[] = 
		{
			new VFrame(file_rename_up_png),
			new VFrame(file_rename_hi_png),
			new VFrame(file_rename_dn_png)
		};
		filebox_rename_images = default_filebox_rename_images;

		static VFrame* default_filebox_delete_images[] = 
		{
			new VFrame(file_delete_up_png),
			new VFrame(file_delete_hi_png),
			new VFrame(file_delete_dn_png)
		};
		filebox_delete_images = default_filebox_delete_images;

	#include "images/file_reload_up_png.h"
	#include "images/file_reload_hi_png.h"
	#include "images/file_reload_dn_png.h"
		static VFrame* default_filebox_reload_images[] =
		{
			new VFrame(file_reload_up_png),
			new VFrame(file_reload_hi_png),
			new VFrame(file_reload_dn_png)
		};
		filebox_reload_images = default_filebox_reload_images;

	#include "images/listbox_button_dn_png.h"
	#include "images/listbox_button_hi_png.h"
	#include "images/listbox_button_up_png.h"
		static VFrame* default_listbox_button[] = 
		{
			new VFrame(listbox_button_up_png),
			new VFrame(listbox_button_hi_png),
			new VFrame(listbox_button_dn_png)
		};
		listbox_button = default_listbox_button;

	#include "images/menu_popup_bg_png.h"
		static VFrame* default_listbox_bg = 0;
		listbox_bg = default_listbox_bg;

	#include "images/listbox_expandchecked_png.h"
	#include "images/listbox_expandcheckedhi_png.h"
	#include "images/listbox_expanddn_png.h"
	#include "images/listbox_expandup_png.h"
	#include "images/listbox_expanduphi_png.h"
		static VFrame* default_listbox_expand[] = 
		{
			new VFrame(listbox_expandup_png),
			new VFrame(listbox_expanduphi_png),
			new VFrame(listbox_expandchecked_png),
			new VFrame(listbox_expanddn_png),
			new VFrame(listbox_expandcheckedhi_png),
		};
		listbox_expand = default_listbox_expand;

	#include "images/listbox_columnup_png.h"
	#include "images/listbox_columnhi_png.h"
	#include "images/listbox_columndn_png.h"
		static VFrame* default_listbox_column[] = 
		{
			new VFrame(listbox_columnup_png),
			new VFrame(listbox_columnhi_png),
			new VFrame(listbox_columndn_png)
		};
		listbox_column = default_listbox_column;


	#include "images/listbox_up_png.h"
	#include "images/listbox_dn_png.h"
		listbox_up = new VFrame(listbox_up_png);
		listbox_dn = new VFrame(listbox_dn_png);
		listbox_title_overlap = DP(0);
		listbox_title_margin = DP(0);
		listbox_title_color = BLACK;
		listbox_title_hotspot = DP(5);

		listbox_border1 = DKGREY;
		listbox_border2_hi = RED;
		listbox_border2 = BLACK;
		listbox_border3_hi = RED;
		listbox_border3 = MEGREY;
		listbox_border4 = WHITE;
		listbox_selected = BLUE;
		listbox_highlighted = LTGREY;
		listbox_inactive = WHITE;
		listbox_text = BLACK;




	#include "images/horizontal_slider_bg_up_png.h"
	#include "images/horizontal_slider_bg_hi_png.h"
	#include "images/horizontal_slider_bg_dn_png.h"
	#include "images/horizontal_slider_fg_up_png.h"
	#include "images/horizontal_slider_fg_hi_png.h"
	#include "images/horizontal_slider_fg_dn_png.h"
		static VFrame *default_horizontal_slider_data[] = 
		{
			new VFrame(horizontal_slider_fg_up_png),
			new VFrame(horizontal_slider_fg_hi_png),
			new VFrame(horizontal_slider_fg_dn_png),
			new VFrame(horizontal_slider_bg_up_png),
			new VFrame(horizontal_slider_bg_hi_png),
			new VFrame(horizontal_slider_bg_dn_png),
		};

	#include "images/vertical_slider_bg_up_png.h"
	#include "images/vertical_slider_bg_hi_png.h"
	#include "images/vertical_slider_bg_dn_png.h"
	#include "images/vertical_slider_fg_up_png.h"
	#include "images/vertical_slider_fg_hi_png.h"
	#include "images/vertical_slider_fg_dn_png.h"
		static VFrame *default_vertical_slider_data[] = 
		{
			new VFrame(vertical_slider_fg_up_png),
			new VFrame(vertical_slider_fg_hi_png),
			new VFrame(vertical_slider_fg_dn_png),
			new VFrame(vertical_slider_bg_up_png),
			new VFrame(vertical_slider_bg_hi_png),
			new VFrame(vertical_slider_bg_dn_png),
		};
		horizontal_slider_data = default_horizontal_slider_data;
		vertical_slider_data = default_vertical_slider_data;

	#include "images/pot_hi_png.h"
	#include "images/pot_up_png.h"
	#include "images/pot_dn_png.h"
		static VFrame *default_pot_images[] = 
		{
			new VFrame(pot_up_png),
			new VFrame(pot_hi_png),
			new VFrame(pot_dn_png)
		};

	#include "images/progress_up_png.h"
	#include "images/progress_hi_png.h"
		static VFrame* default_progress_images[] = 
		{
			new VFrame(progress_up_png),
			new VFrame(progress_hi_png)
		};


	#include "images/pan_up_png.h"
	#include "images/pan_hi_png.h"
	#include "images/pan_popup_png.h"
	#include "images/pan_channel_png.h"
	#include "images/pan_stick_png.h"
	#include "images/pan_channel_small_png.h"
	#include "images/pan_stick_small_png.h"
		static VFrame* default_pan_data[] = 
		{
			new VFrame(pan_up_png),
			new VFrame(pan_hi_png),
			new VFrame(pan_popup_png),
			new VFrame(pan_channel_png),
			new VFrame(pan_stick_png),
			new VFrame(pan_channel_small_png),
			new VFrame(pan_stick_small_png)
		};
		pan_data = default_pan_data;
		pan_text_color = YELLOW;

// 	#include "images/7seg_small/0_png.h"
// 	#include "images/7seg_small/1_png.h"
// 	#include "images/7seg_small/2_png.h"
// 	#include "images/7seg_small/3_png.h"
// 	#include "images/7seg_small/4_png.h"
// 	#include "images/7seg_small/5_png.h"
// 	#include "images/7seg_small/6_png.h"
// 	#include "images/7seg_small/7_png.h"
// 	#include "images/7seg_small/8_png.h"
// 	#include "images/7seg_small/9_png.h"
// 	#include "images/7seg_small/colon_png.h"
// 	#include "images/7seg_small/period_png.h"
// 	#include "images/7seg_small/a_png.h"
// 	#include "images/7seg_small/b_png.h"
// 	#include "images/7seg_small/c_png.h"
// 	#include "images/7seg_small/d_png.h"
// 	#include "images/7seg_small/e_png.h"
// 	#include "images/7seg_small/f_png.h"
// 	#include "images/7seg_small/space_png.h"
// 	#include "images/7seg_small/dash_png.h"
// 		static VFrame* default_medium_7segment[] = 
// 		{
// 			new VFrame(_0_png),
// 			new VFrame(_1_png),
// 			new VFrame(_2_png),
// 			new VFrame(_3_png),
// 			new VFrame(_4_png),
// 			new VFrame(_5_png),
// 			new VFrame(_6_png),
// 			new VFrame(_7_png),
// 			new VFrame(_8_png),
// 			new VFrame(_9_png),
// 			new VFrame(colon_png),
// 			new VFrame(period_png),
// 			new VFrame(a_png),
// 			new VFrame(b_png),
// 			new VFrame(c_png),
// 			new VFrame(d_png),
// 			new VFrame(e_png),
// 			new VFrame(f_png),
// 			new VFrame(space_png),
// 			new VFrame(dash_png)
// 		};

	#include "images/tumble_bottomdn_png.h"
	#include "images/tumble_topdn_png.h"
	#include "images/tumble_hi_png.h"
	#include "images/tumble_up_png.h"
		static VFrame* default_tumbler_data[] = 
		{
			new VFrame(tumble_up_png),
			new VFrame(tumble_hi_png),
			new VFrame(tumble_bottomdn_png),
			new VFrame(tumble_topdn_png)
		};

	#include "images/xmeter_normal_png.h"
	#include "images/xmeter_green_png.h"
	#include "images/xmeter_red_png.h"
	#include "images/xmeter_yellow_png.h"
	#include "images/xmeter_white_png.h"
	#include "images/over_horiz_png.h"
	#include "images/ymeter_normal_png.h"
	#include "images/ymeter_green_png.h"
	#include "images/ymeter_red_png.h"
	#include "images/ymeter_yellow_png.h"
	#include "images/ymeter_white_png.h"
	#include "images/over_vertical_png.h"
		static VFrame* default_xmeter_data[] =
		{
			new VFrame(),
			new VFrame(),
			new VFrame(),
			new VFrame(),
			new VFrame(),
			new VFrame()
		};
		

		static VFrame* default_ymeter_data[] =
		{
			new VFrame(),
			new VFrame(),
			new VFrame(),
			new VFrame(),
			new VFrame(),
			new VFrame()
		};

	#include "images/generic_up_png.h"
	#include "images/generic_hi_png.h"
	#include "images/generic_dn_png.h"

		static VFrame* default_generic_button_data[] = 
		{
			new VFrame(generic_up_png),
			new VFrame(generic_hi_png),
			new VFrame(generic_dn_png)
		};

		generic_button_images = default_generic_button_data;
		generic_button_margin = DP(15);



	#include "images/hscroll_handle_up_png.h"
	#include "images/hscroll_handle_hi_png.h"
	#include "images/hscroll_handle_dn_png.h"
	#include "images/hscroll_handle_bg_png.h"
	#include "images/hscroll_left_up_png.h"
	#include "images/hscroll_left_hi_png.h"
	#include "images/hscroll_left_dn_png.h"
	#include "images/hscroll_right_up_png.h"
	#include "images/hscroll_right_hi_png.h"
	#include "images/hscroll_right_dn_png.h"
	#include "images/vscroll_handle_up_png.h"
	#include "images/vscroll_handle_hi_png.h"
	#include "images/vscroll_handle_dn_png.h"
	#include "images/vscroll_handle_bg_png.h"
	#include "images/vscroll_left_up_png.h"
	#include "images/vscroll_left_hi_png.h"
	#include "images/vscroll_left_dn_png.h"
	#include "images/vscroll_right_up_png.h"
	#include "images/vscroll_right_hi_png.h"
	#include "images/vscroll_right_dn_png.h"
		static VFrame *default_hscroll_data[] = 
		{
			new VFrame(hscroll_handle_up_png), 
			new VFrame(hscroll_handle_hi_png), 
			new VFrame(hscroll_handle_dn_png), 
			new VFrame(hscroll_handle_bg_png), 
			new VFrame(hscroll_left_up_png), 
			new VFrame(hscroll_left_hi_png), 
			new VFrame(hscroll_left_dn_png), 
			new VFrame(hscroll_right_up_png), 
			new VFrame(hscroll_right_hi_png), 
			new VFrame(hscroll_right_dn_png)
		};
		static VFrame *default_vscroll_data[] = 
		{
			new VFrame(vscroll_handle_up_png), 
			new VFrame(vscroll_handle_hi_png), 
			new VFrame(vscroll_handle_dn_png), 
			new VFrame(vscroll_handle_bg_png), 
			new VFrame(vscroll_left_up_png), 
			new VFrame(vscroll_left_hi_png), 
			new VFrame(vscroll_left_dn_png), 
			new VFrame(vscroll_right_up_png), 
			new VFrame(vscroll_right_hi_png), 
			new VFrame(vscroll_right_dn_png)
		};
		hscroll_data = default_hscroll_data;
		vscroll_data = default_vscroll_data;
		scroll_minhandle = DP(10);


		use_shm = -1;

// Initialize
		bg_color = WHITE;
		bg_shadow1 = DKGREY;
		bg_shadow2 = BLACK;
		bg_light1 = WHITE;
		bg_light2 = bg_color;


		border_light1 = bg_color;
		border_light2 = MEGREY;
		border_shadow1 = BLACK;
		border_shadow2 = bg_color;

		default_text_color = BLACK;
		disabled_text_color = MEGREY;

		button_light = MEGREY;           // bright corner
		button_highlighted = LTGREY;  // face when highlighted
		button_down = MDGREY;         // face when down
		button_up = MEGREY;           // face when up
		button_shadow = BLACK;       // dark corner

		tumble_data = default_tumbler_data;
		tumble_duration = 150;

		ok_images = default_ok_images;
		cancel_images = default_cancel_images;
		usethis_button_images = default_usethis_images;
		filebox_descend_images = default_ok_images;

		checkbox_images = default_checkbox_images;
		radial_images = default_radial_images;
		label_images = default_label_images;

		menu_light = LTCYAN;
		menu_highlighted = LTBLUE;
		menu_down = MDCYAN;
		menu_up = MECYAN;
		menu_shadow = DKCYAN;


	#include "images/menuitem_up_png.h"
	#include "images/menuitem_hi_png.h"
	#include "images/menuitem_dn_png.h"
	#include "images/menubar_up_png.h"
	#include "images/menubar_hi_png.h"
	#include "images/menubar_dn_png.h"
	#include "images/menubar_bg_png.h"

		static VFrame *default_menuitem_data[] = 
		{
			new VFrame(menuitem_up_png),
			new VFrame(menuitem_hi_png),
			new VFrame(menuitem_dn_png),
		};
		menu_item_bg = default_menuitem_data;


		static VFrame *default_menubar_data[] = 
		{
			new VFrame(menubar_up_png),
			new VFrame(menubar_hi_png),
			new VFrame(menubar_dn_png),
		};
		menu_title_bg = default_menubar_data;

		menu_popup_bg = new VFrame(menu_popup_bg_png);

		menu_bar_bg = new VFrame(menubar_bg_png);

		popupmenu_images = 0;


		popupmenu_margin = DP(10);
		popupmenu_triangle_margin = DP(10);

		min_menu_w = DP(0);
		menu_title_text = BLACK;
		popup_title_text = BLACK;
		menu_item_text = BLACK;
		progress_text = BLACK;



		text_default = BLACK;
		highlight_inverse = WHITE ^ BLUE;
		text_background = WHITE;
		text_background_hi = LTYELLOW;
		text_background_noborder_hi = LTGREY;
		text_background_noborder = -1;
		text_border1 = DKGREY;
		text_border2 = BLACK;
		text_border2_hi = RED;
		text_border3 = MEGREY;
		text_border3_hi = LTPINK;
		text_border4 = WHITE;
		text_highlight = LTGREY;
		text_inactive_highlight = MEGREY;

		toggle_highlight_bg = 0;
		toggle_text_margin = DP(0);

	// Delays must all be different for repeaters
		double_click = 300;
		blink_rate = 250;
		scroll_repeat = 150;
		tooltip_delay = 1000;
		tooltip_bg_color = YELLOW;
		tooltips_enabled = 1;

		filebox_margin = DP(110);
		dirbox_margin = DP(90);
		filebox_mode = LISTBOX_TEXT;
		sprintf(filebox_filter, "*");
		filebox_w = DP(640);
		filebox_h = DP(480);
        filebox_preview_w = DP(150);
        filebox_show_preview = 1;
		filebox_columntype[0] = FILEBOX_NAME;
		filebox_columntype[1] = FILEBOX_SIZE;
		filebox_columntype[2] = FILEBOX_DATE;
		filebox_columnwidth[0] = DP(200);
		filebox_columnwidth[1] = DP(100);
		filebox_columnwidth[2] = DP(100);
		dirbox_columntype[0] = FILEBOX_NAME;
		dirbox_columntype[1] = FILEBOX_DATE;
		dirbox_columnwidth[0] = DP(200);
		dirbox_columnwidth[1] = DP(100);

//		filebox_text_images = default_filebox_text_images;
//		filebox_icons_images = default_filebox_icons_images;
		directory_color = BLUE;
		file_color = BLACK;

		filebox_sortcolumn = 0;
		filebox_sortorder = BC_ListBox::SORT_ASCENDING;
		dirbox_sortcolumn = 0;
		dirbox_sortorder = BC_ListBox::SORT_ASCENDING;


		pot_images = default_pot_images;
		pot_offset = 2;
//		pot_x1 = pot_images[0]->get_w() / 2 - pot_offset;
//		pot_y1 = pot_images[0]->get_h() / 2 - pot_offset;
//		pot_r = pot_x1;
		pot_needle_color = BLACK;

		progress_images = default_progress_images;

		xmeter_images = default_xmeter_data;
		ymeter_images = default_ymeter_data;
		
		xmeter_images[0]->read_png(xmeter_normal_png, dpi);
		xmeter_images[1]->read_png(xmeter_green_png, dpi);
		xmeter_images[2]->read_png(xmeter_red_png, dpi);
		xmeter_images[3]->read_png(xmeter_yellow_png, dpi);
		xmeter_images[4]->read_png(xmeter_white_png, dpi);
		xmeter_images[5]->read_png(over_horiz_png, dpi);
		

		ymeter_images[0]->read_png(ymeter_normal_png, dpi);
		ymeter_images[1]->read_png(ymeter_green_png, dpi);
		ymeter_images[2]->read_png(ymeter_red_png, dpi);
		ymeter_images[3]->read_png(ymeter_yellow_png, dpi);
		ymeter_images[4]->read_png(ymeter_white_png, dpi);
		ymeter_images[5]->read_png(over_vertical_png, dpi);

		

		meter_font = SMALLFONT;
		meter_font_color = RED;
		meter_title_w = DP(20);
		meter_3d = 0;
//		medium_7segment = default_medium_7segment;


	//	use_fontset = 0;

	// Xft has priority over font set
	#ifdef HAVE_XFT
	// But Xft dies in 32 bit mode after some amount of drawing.
		use_xft = 1;
	#else
		use_xft = 0;
	#endif


		drag_radius = DP(10);
		recursive_resizing = 1;

	
		
	}
}


static int nearest_size(const int *supported, int size)
{
	int i = 1;
	while(1)
	{
		if(supported[i] == 0 || 
			supported[i] > size)
		{
			return supported[i - 1];
		}
		
		if(supported[i] == size)
		{
			return supported[i];
		}
		
		i++;
	}
	
	return supported[0];
}

void BC_Resources::init_fonts()
{
// supported helvetica sizes in points
	int supported[] = { 11, 14, 17, 20, 25, 34, 0 };
//printf("BC_Resources::BC_Resources %d dpi=%d\n", __LINE__, dpi);
	small_font = N_("-*-helvetica-medium-r-normal");
	medium_font = N_("-*-helvetica-bold-r-normal");
	large_font = N_("-*-helvetica-bold-r-normal");
	clock_font = N_("-*-helvetica-bold-r-normal");
// fixed fonts use point sizes, not pixel sizes
	large_fontsize = nearest_size(supported, DP(24));
	medium_fontsize = nearest_size(supported, DP(14));
	small_fontsize = nearest_size(supported, DP(10));
	clock_fontsize = nearest_size(supported, DP(24));


	small_font_xft = N_("Sans");
	medium_font_xft = N_("Sans");
	large_font_xft = N_("Sans");
	clock_font_xft = N_("Sans");

// XFT uses pixel sizes
	large_font_xftsize = DP(24);
	medium_font_xftsize = DP(16);
	small_font_xftsize = DP(12);
	clock_font_xftsize = DP(21);
}

int BC_Resources::initialize_display(BC_WindowBase *window)
{
// Set up IPC cleanup handlers
//	bc_init_ipc();

// Test for shm.  Must come before yuv test
	init_shm(window);
	return 0;
}


int BC_Resources::init_shm(BC_WindowBase *window)
{
	use_shm = 1;
	XSetErrorHandler(BC_Resources::x_error_handler);

	if(!XShmQueryExtension(window->display)) 
    {
        use_shm = 0;
    }
	else
	{
		XShmSegmentInfo test_shm;
		XImage *test_image;
		unsigned char *data;
		test_image = XShmCreateImage(window->display, window->vis, window->default_depth,
                ZPixmap, (char*)NULL, &test_shm, 5, 5);

		test_shm.shmid = shmget(IPC_PRIVATE, 5 * test_image->bytes_per_line, (IPC_CREAT | 0777 ));
		data = (unsigned char *)shmat(test_shm.shmid, NULL, 0);
    	shmctl(test_shm.shmid, IPC_RMID, 0);
		BC_Resources::error = 0;
 	   	XShmAttach(window->display, &test_shm);
    	XSync(window->display, False);
		if(BC_Resources::error) 
        {
            use_shm = 0;
        }
        
		XDestroyImage(test_image);
		shmdt(test_shm.shmaddr);
	}
//	XSetErrorHandler(0);
	return 0;
}




BC_Synchronous* BC_Resources::get_synchronous()
{
	return synchronous;
}

void BC_Resources::set_synchronous(BC_Synchronous *synchronous)
{
	this->synchronous = synchronous;
}







int BC_Resources::get_top_border()
{
	return display_info->get_top_border();
}

int BC_Resources::get_left_border()
{
	return display_info->get_left_border();
}

int BC_Resources::get_right_border()
{
	return display_info->get_right_border();
}

int BC_Resources::get_bottom_border()
{
	return display_info->get_bottom_border();
}


int BC_Resources::get_bg_color() { return bg_color; }

int BC_Resources::get_bg_shadow1() { return bg_shadow1; }

int BC_Resources::get_bg_shadow2() { return bg_shadow2; }

int BC_Resources::get_bg_light1() { return bg_light1; }

int BC_Resources::get_bg_light2() { return bg_light2; }


int BC_Resources::dp_to_px(int dp)
{
	if(dpi < MIN_DPI)
	{
		return dp;
	}
	
	return dp * dpi / BASE_DPI;
}

double BC_Resources::dp_to_px(double dp)
{
	if(dpi < MIN_DPI)
	{
		return dp;
	}
	
	return dp * dpi / BASE_DPI;
}

int BC_Resources::get_id()
{
	id_lock->lock("BC_Resources::get_id");
	int result = id++;
	id_lock->unlock();
	return result;
}

int BC_Resources::get_filebox_id()
{
	id_lock->lock("BC_Resources::get_filebox_id");
	int result = filebox_id++;
	id_lock->unlock();
	return result;
}


void BC_Resources::set_signals(BC_Signals *signal_handler)
{
	BC_Resources::signal_handler = signal_handler;
}

BC_Signals* BC_Resources::get_signals()
{
	return BC_Resources::signal_handler;
}
