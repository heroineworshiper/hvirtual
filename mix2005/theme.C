#include "configure.h"
#include "guicast.h"
#include "mixer.h"
#include "mixergui.h"
#include "theme.h"






Theme::Theme(Mixer *mixer)
 : BC_Theme()
{
	this->mixer = mixer;
	is_bright = 0;
}

void Theme::initialize()
{

	is_bright = mixer->defaults->get("BRIGHT_THEME", 0);


	extern unsigned char _binary_theme_data_start[];
	set_data(_binary_theme_data_start);
	BC_Resources *resources = BC_WindowBase::get_resources();
	resources->scroll_minhandle = 20;

	if(is_bright)
	{
		resources->bg_color = 0xffffff;
		resources->text_default = 0x000000;
		resources->text_background = 0x808080;
		resources->tooltip_bg_color = 0xffffff;
		resources->generic_button_margin = 20;
		resources->popupmenu_margin = 10;
		resources->popupmenu_triangle_margin = 15;


		resources->menu_light = 0xffffff;
		resources->menu_highlighted = 0xe0e0e0;
		resources->menu_down = 0xc0c0c0;
		resources->menu_up = 0xffffff;
		resources->menu_shadow = 0x000000;
		resources->toggle_text_margin = 0;

		resources->pot_images = new_image_set(
			"pot_data", 
			4, 
			"bright_pot_up.png",
			"bright_pot_hi.png",
			"bright_pot_dn.png",
			"hscroll_handle_bg.png");

		resources->vscroll_data = new_image_set(
			"vscroll_data", 
			10, 
			"bright_vscroll_handle_up.png",
			"bright_vscroll_handle_hi.png",
			"bright_vscroll_handle_dn.png",
			"bright_vscroll_handle_bg.png",
			"bright_vscroll_left_up.png",
			"bright_vscroll_left_hi.png",
			"bright_vscroll_left_dn.png",
			"bright_vscroll_right_up.png",
			"bright_vscroll_right_hi.png",
			"bright_vscroll_right_dn.png");


		resources->hscroll_data = new_image_set(
			"hscroll_data", 
			10, 
			"bright_hscroll_handle_up.png",
			"bright_hscroll_handle_hi.png",
			"bright_hscroll_handle_dn.png",
			"bright_hscroll_handle_bg.png",
			"bright_hscroll_left_up.png",
			"bright_hscroll_left_hi.png",
			"bright_hscroll_left_dn.png",
			"bright_hscroll_right_up.png",
			"bright_hscroll_right_hi.png",
			"bright_hscroll_right_dn.png");


		resources->checkbox_images = new_image_set(5,
			"bright_checkbox_up.png",
			"bright_checkbox_hi.png",
			"bright_checkbox_checked.png",
			"bright_checkbox_dn.png",
			"bright_checkbox_checkedhi.png");
		resources->generic_button_images = new_image_set(
			"generic_data", 
			3, 
			"bright_generic_up.png",
			"bright_generic_hi.png",
			"bright_generic_dn.png");
	}
	else
	{
		resources->bg_color = 0x373737;
		resources->text_default = 0xbfbfbf;
		resources->text_background = 0x373737;
		resources->menu_popup_bg = new_image("menu_popup_bg", "menu_bg.png");
		main_bg = new_image("main_bg", "background.png");

		resources->menu_item_bg = new_image_set("menu_item_bg", 
			3, 
			"generic_up.png",
			"generic_hi.png",
			"generic_dn.png");
		resources->menu_light = 0x969696;
		resources->menu_highlighted = 0x646464;
		resources->menu_down = 0x373737;
		resources->menu_up = 0x373737;
		resources->menu_shadow = 0x000000;
		resources->toggle_highlight_bg = new_image("toggle_highlight_bg",
			"text_highlight.png");
		resources->toggle_text_margin = 10;


		resources->vscroll_data = new_image_set(
			"vscroll_data", 
			10, 
			"vscroll_handle_up.png",
			"vscroll_handle_hi.png",
			"vscroll_handle_dn.png",
			"vscroll_handle_bg.png",
			"vscroll_left_up.png",
			"vscroll_left_hi.png",
			"vscroll_left_dn.png",
			"vscroll_right_up.png",
			"vscroll_right_hi.png",
			"vscroll_right_dn.png");
		resources->hscroll_data = new_image_set(
			"hscroll_data", 
			10, 
			"hscroll_handle_up.png",
			"hscroll_handle_hi.png",
			"hscroll_handle_dn.png",
			"hscroll_handle_bg.png",
			"hscroll_left_up.png",
			"hscroll_left_hi.png",
			"hscroll_left_dn.png",
			"hscroll_right_up.png",
			"hscroll_right_hi.png",
			"hscroll_right_dn.png");
		resources->pot_images = new_image_set(
			"pot_data", 
			4, 
			"pot_up.png",
			"pot_hi.png",
			"pot_dn.png",
			"hscroll_handle_bg.png");
		resources->checkbox_images = new_image_set(5,
			"checkbox_up.png",
			"checkbox_hi.png",
			"checkbox_checked.png",
			"checkbox_dn.png",
			"checkbox_checkedhi.png");
		resources->generic_button_images = new_image_set(
			"generic_data", 
			3, 
			"generic_up.png",
			"generic_hi.png",
			"generic_dn.png");
	}

	resources->default_text_color = resources->text_default;
	resources->popup_title_text = resources->text_default;
	resources->menu_title_text = resources->text_default;
	resources->menu_item_text = resources->text_default;



	resources->pot_needle_color = resources->text_default;
	resources->pot_offset = 1;
//	resources->pot_x1 = resources->pot_images[0]->get_w() / 2 - 1;
//	resources->pot_y1 = resources->pot_images[0]->get_h() / 2 - 1;



}

void Theme::calculate_main_sizes()
{
	main_subwindow_w = mixer->w - BC_ScrollBar::get_span(SCROLL_VERT) - 2;
	main_subwindow_h = mixer->h - BC_ScrollBar::get_span(SCROLL_HORIZ) - 2;

	main_vscroll_x = main_subwindow_w + 1;
	main_vscroll_y = 1;
	main_vscroll_h = main_subwindow_h;
	main_hscroll_x = 1;
	main_hscroll_y = main_subwindow_h + 1;
	main_hscroll_w = main_subwindow_w;
}

void Theme::draw_main_bg()
{
	if(!is_bright)
	{
		mixer->gui->draw_9segment(0, 
			0, 
			mixer->w,
			mixer->h,
			main_bg);
	}
	mixer->gui->flash();
}

void Theme::calculate_config_sizes()
{
	config_subwindow_w = mixer->configure_thread->w - BC_ScrollBar::get_span(SCROLL_VERT) - 2;
	config_subwindow_h = mixer->configure_thread->h - BC_ScrollBar::get_span(SCROLL_HORIZ) - 2;

	config_vscroll_x = config_subwindow_w + 1;
	config_vscroll_y = 1;
	config_vscroll_h = config_subwindow_h;
	config_hscroll_x = 1;
	config_hscroll_y = config_subwindow_h + 1;
	config_hscroll_w = config_subwindow_w;
}

void Theme::draw_config_bg()
{
	if(!is_bright)
	{
		mixer->configure_thread->gui->draw_9segment(0, 
			0, 
			mixer->configure_thread->w,
			mixer->configure_thread->h,
			main_bg);
	}
	mixer->configure_thread->gui->flash();
}





