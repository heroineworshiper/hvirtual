/*
 * CINELERRA
 * Copyright (C) 1997-2024 Adam Williams <broadcast at earthling dot net>
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

#include "bcsignals.h"
#include "brighttheme.h"
#include "clip.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "patchbay.h"
#include "preferencesthread.h"
#include "recordgui.h"
#include "recordmonitor.h"
#include "setformat.h"
#include "statusbar.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "vwindowgui.h"




PluginClient* new_plugin(PluginServer *server)
{
	return new BrightThemeMain(server);
}







BrightThemeMain::BrightThemeMain(PluginServer *server)
 : PluginTClient(server)
{
}

BrightThemeMain::~BrightThemeMain()
{
}

const char* BrightThemeMain::plugin_title()
{
	return "Bright";
}

Theme* BrightThemeMain::new_theme()
{
	theme = new BrightTheme;
	return theme;
}








BrightTheme::BrightTheme()
 : Theme()
{
// append to base class data before initialize
	extern unsigned char _binary_brighttheme_data_start[];
	set_data(_binary_brighttheme_data_start);
}

BrightTheme::~BrightTheme()
{
}

void BrightTheme::initialize()
{
	BC_Resources *resources = BC_WindowBase::get_resources();

	resources->text_default = 0x000000;
	resources->text_background = 0xffffff;
	resources->text_background_hi = 0xffffff;
	resources->text_border1 = 0x000000;
	resources->text_border2 = 0xffffff;
	resources->text_border3 = 0xffffff;
	resources->text_border2_hi = 0x000000;
	resources->text_border3_hi = 0x000000;
	resources->text_border4 = 0x000000;
	resources->text_inactive_highlight = LTGREY;
	resources->text_highlight = LTGREY;

	resources->bg_color = 0xffffff;
	resources->default_text_color = 0x000000;
	resources->menu_title_text = 0x000000;
	resources->popup_title_text = 0x000000;
	resources->menu_item_text = 0x000000;
	resources->generic_button_margin = DP(5);
	resources->pot_needle_color = resources->text_default;
	resources->pot_offset = 1;
	resources->progress_text = resources->text_default;
	resources->meter_font_color = resources->default_text_color;
	resources->tooltip_bg_color = WHITE;
	clock_bg_color = WHITE;
	clock_fg_color = BLACK;

	resources->menu_light = 0x000000;
	resources->menu_highlighted = 0xe0e0e0;
	resources->menu_down = 0xc0c0c0;
	resources->menu_up = 0xffffff;
	resources->menu_shadow = 0x000000;
	resources->popupmenu_margin = DP(15);
	resources->popupmenu_triangle_margin = DP(5);

	resources->listbox_title_color = BLACK;

	resources->listbox_title_overlap = DP(20);
	resources->listbox_title_margin = DP(20);
	resources->listbox_title_hotspot = DP(20);
	resources->listbox_border1 = BLACK;
	resources->listbox_border2 = WHITE;
	resources->listbox_border3 = WHITE;
	resources->listbox_border2_hi = BLACK;
	resources->listbox_border3_hi = BLACK;
	resources->listbox_border4 = BLACK;
	resources->listbox_highlighted = 0xfefefe;
	resources->listbox_inactive = WHITE;
	resources->listbox_selected = 0xe0e0e0;
	resources->listbox_bg = 0;
	resources->listbox_text = BLACK;

	resources->filebox_margin = DP(130);
	resources->file_color = 0x000000;
	resources->directory_color = 0x0000ff;

	resources->scroll_minhandle = DP(24);

	new_toggle("loadmode_new.png", 
		"loadmode_up.png",
		"loadmode_hi.png",
		"loadmode_checked.png",
		"loadmode_dn.png",
		"loadmode_checkedhi.png",
		"loadmode_new");
	new_toggle("loadmode_none.png", 
		"loadmode_up.png",
		"loadmode_hi.png",
		"loadmode_checked.png",
		"loadmode_dn.png",
		"loadmode_checkedhi.png",
		"loadmode_none");
	new_toggle("loadmode_newcat.png", 
		"loadmode_up.png",
		"loadmode_hi.png",
		"loadmode_checked.png",
		"loadmode_dn.png",
		"loadmode_checkedhi.png",
		"loadmode_newcat");
	new_toggle("loadmode_cat.png", 
		"loadmode_up.png",
		"loadmode_hi.png",
		"loadmode_checked.png",
		"loadmode_dn.png",
		"loadmode_checkedhi.png",
		"loadmode_cat");
	new_toggle("loadmode_newtracks.png", 
		"loadmode_up.png",
		"loadmode_hi.png",
		"loadmode_checked.png",
		"loadmode_dn.png",
		"loadmode_checkedhi.png",
		"loadmode_newtracks");
	new_toggle("loadmode_paste.png", 
		"loadmode_up.png",
		"loadmode_hi.png",
		"loadmode_checked.png",
		"loadmode_dn.png",
		"loadmode_checkedhi.png",
		"loadmode_paste");
	new_toggle("loadmode_resource.png", 
		"loadmode_up.png",
		"loadmode_hi.png",
		"loadmode_checked.png",
		"loadmode_dn.png",
		"loadmode_checkedhi.png",
		"loadmode_resource");
	new_toggle("loadmode_nested.png", 
		"loadmode_up.png",
		"loadmode_hi.png",
		"loadmode_checked.png",
		"loadmode_dn.png",
		"loadmode_checkedhi.png",
		"loadmode_nested");



// 	resources->filebox_icons_images = new_button("icons.png",
// 		"fileboxbutton_up.png",
// 		"fileboxbutton_hi.png",
// 		"fileboxbutton_dn.png");
// 
// 	resources->filebox_text_images = new_button("text.png",
// 		"fileboxbutton_up.png",
// 		"fileboxbutton_hi.png",
// 		"fileboxbutton_dn.png");

	resources->filebox_newfolder_images = new_button("folder.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");

	resources->filebox_preview_images = new_toggle("preview.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_checked.png",
		"fileboxbutton_dn.png",
		"fileboxbutton_checkedhi.png");

	resources->filebox_rename_images = new_button("rename.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");

	resources->filebox_updir_images = new_button("updir.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");

	resources->filebox_delete_images = new_button("delete.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");

	resources->filebox_reload_images = new_button("reload.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");


	resources->filebox_descend_images = new_button("openfolder.png",
		"bigbutton_up.png", 
		"bigbutton_hi.png", 
		"bigbutton_dn.png");

	resources->usethis_button_images = 
		resources->ok_images = new_button("ok.png",
		"bigbutton_up.png", 
		"bigbutton_hi.png", 
		"bigbutton_dn.png");

	new_button("ok.png",
		"bigbutton_up.png", 
		"bigbutton_hi.png", 
		"bigbutton_dn.png",
		"new_ok_images");

	resources->cancel_images = new_button("cancel.png",
		"bigbutton_up.png", 
		"bigbutton_hi.png", 
		"bigbutton_dn.png");

	new_button("cancel.png",
		"bigbutton_up.png", 
		"bigbutton_hi.png", 
		"bigbutton_dn.png",
		"new_cancel_images");

	resources->bar_data = new_image("bar", "bar.png");


	resources->min_menu_w = DP(96);
	resources->menu_popup_bg = new_image("menu_popup_bg.png");
	resources->menu_item_bg = new_image_set(3,
		"menuitem_up.png",
		"menuitem_hi.png",
		"menuitem_dn.png");
	resources->menu_bar_bg = new_image("menubar_bg.png");
	resources->menu_title_bg = new_image_set(3, 
		"menubar_up.png",
		"menubar_hi.png",
		"menubar_dn.png");



	resources->toggle_text_margin = resources->generic_button_margin;
	resources->toggle_highlight_bg = new_image("toggle_highlight_bg",
		"generic_up.png");

	resources->generic_button_images = new_image_set(3, 
			"generic_up.png", 
			"generic_hi.png", 
			"generic_dn.png");
	resources->horizontal_slider_data = new_image_set(6,
			"hslider_fg_up.png",
			"hslider_fg_hi.png",
			"hslider_fg_dn.png",
			"hslider_bg_up.png",
			"hslider_bg_hi.png",
			"hslider_bg_dn.png");
	resources->progress_images = new_image_set(2,
			"progress_bg.png",
			"progress_hi.png");
	resources->tumble_data = new_image_set(4,
		"tumble_up.png",
		"tumble_hi.png",
		"tumble_bottom.png",
		"tumble_top.png");
	resources->listbox_button = new_button("listbox_button.png",
		"editpanel_up.png",
		"editpanel_hi.png",
		"editpanel_dn.png");
	resources->listbox_column = new_image_set(3,
		"column_up.png",
		"column_hi.png",
		"column_dn.png");
	resources->listbox_up = new_image("listbox_up.png");
	resources->listbox_dn = new_image("listbox_dn.png");
	resources->pan_data = new_image_set(7,
			"pan_up.png", 
			"pan_hi.png", 
			"pan_popup.png", 
			"pan_channel.png", 
			"pan_stick.png", 
			"pan_channel_small.png", 
			"pan_stick_small.png");
	resources->pan_text_color = BLACK;

	resources->pot_images = new_image_set(3,
		"bright_pot_up.png",
		"bright_pot_hi.png",
		"bright_pot_dn.png");

	resources->checkbox_images = new_image_set(5,
		"bright_checkbox_up.png",
		"bright_checkbox_hi.png",
		"bright_checkbox_checked.png",
		"bright_checkbox_dn.png",
		"bright_checkbox_checkedhi.png");

	resources->radial_images = new_image_set(5,
		"radial_up.png",
		"radial_hi.png",
		"radial_checked.png",
		"radial_dn.png",
		"radial_checkedhi.png");



	resources->ymeter_images = new_image_set(6, 
		"ymeter_normal.png",
		"ymeter_green.png",
		"ymeter_red.png",
		"ymeter_yellow.png",
		"ymeter_white.png",
		"ymeter_over.png");

	resources->xmeter_images = new_image_set(6, 
		"xmeter_normal.png",
		"xmeter_green.png",
		"xmeter_red.png",
		"xmeter_yellow.png",
		"xmeter_white.png",
		"xmeter_over.png");


	resources->hscroll_data = new_image_set(10,
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

	resources->vscroll_data = new_image_set(10,
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


	new_button("prevtip.png", "tipbutton_up.png", "tipbutton_hi.png", "tipbutton_dn.png", "prev_tip");
	new_button("nexttip.png", "tipbutton_up.png", "tipbutton_hi.png", "tipbutton_dn.png", "next_tip");
	new_button("closetip.png", "tipbutton_up.png", "tipbutton_hi.png", "tipbutton_dn.png", "close_tip");
	new_button("swap_extents.png",
		"editpanel_up.png",
		"editpanel_hi.png",
		"editpanel_dn.png",
		"swap_extents");




	preferences_category_overlap = DP(0);
	preferencescategory_x = DP(0);
	preferencescategory_y = DP(5);
	preferencestitle_x = DP(5);
	preferencestitle_y = DP(10);
	preferencesoptions_x = DP(5);
	preferencesoptions_y = DP(5);

// MWindow
	message_normal = resources->text_default;
    fps_color = WHITE;
	audio_color = 0x000000;
    zero_crossing_color = BLACK;
    graph_active_color = BLACK;
    graph_inactive_color = MEGREY;
    graph_grid_color = BLACK;
    graph_bg_color = WHITE;
    graph_border1_color = BLACK;
    graph_border2_color = BLACK;
    
	assetedit_color = BLACK;
	mtransport_margin = DP(20);
	toggle_margin = DP(20);
	timebar_cursor_color = BLACK;

    new_image("speaker", "speaker.png");
	new_image("mbutton_bg", "mbutton_bg.png");
	new_image("timebar_bg", "timebar_bg_flat.png");
	new_image("timebar_brender", "timebar_brender.png");
	new_image("clock_bg", "mclock_flat.png");
	new_image("patchbay_bg", "patchbay_bg.png");
	new_image("statusbar", "statusbar.png");
//	new_image("mscroll_filler", "mscroll_filler.png");
	pane_color = BLACK;
	drag_pane_color = BLACK;

	new_button("pane.png", "pane_up.png", "pane_hi.png", "pane_dn.png", "pane");
	new_image_set("xpane", 3, "xpane_up.png", "xpane_hi.png", "xpane_dn.png");
	new_image_set("ypane", 3, "ypane_up.png", "ypane_hi.png", "ypane_dn.png");

	new_image_set("zoombar_menu", 3, "zoompopup_up.png", "zoompopup_hi.png", "zoompopup_dn.png");
	new_image_set("zoombar_tumbler", 4, "tumble_up.png", "tumble_hi.png", "tumble_bottom.png", "tumble_top.png");

//	new_image_set("mode_popup", 3, "mode_up.png", "mode_hi.png", "mode_dn.png");
	new_image_set("mode_popup", 3, "zoompopup_up.png", "zoompopup_hi.png", "zoompopup_dn.png");
	new_image("mode_add", "mode_add.png");
	new_image("mode_divide", "mode_divide.png");
	new_image("mode_multiply", "mode_multiply.png");
	new_image("mode_normal", "mode_normal.png");
	new_image("mode_replace", "mode_replace.png");
	new_image("mode_subtract", "mode_subtract.png");
	new_image("mode_max", "mode_max.png");
	new_image("mode_min", "mode_min.png");

	new_image_set("plugin_on", 5, "plugin_on.png", "plugin_onhi.png", "plugin_onselect.png", "plugin_ondn.png", "plugin_onselecthi.png");
	new_image_set("plugin_show", 5, "plugin_show.png", "plugin_showhi.png", "plugin_showselect.png", "plugin_showdn.png", "plugin_showselecthi.png");

// CWindow
	new_image("cpanel_bg", "cpanel_bg.png");
//	new_image("cbuttons_left", "cbuttons_left.png");
//	new_image("cbuttons_right", "cbuttons_right.png");
	new_image("cbuttons", "cbuttons.png");
	new_image("cmeter_bg", "cmeter_bg.png");

// VWindow
	new_image("vbuttons_left", "vbuttons_left.png");
	new_image("vclock", "vclock.png");

	new_image("preferences_bg", "preferences_bg.png");


	new_image("new_bg", "new_bg.png");
	new_image("setformat_bg", "setformat_bg.png");


//	timebar_view_data = new_image("timebar_view.png");

	setformat_w = get_image("setformat_bg")->get_w();
	setformat_h = get_image("setformat_bg")->get_h();
	setformat_x1 = DP(15);
	setformat_x2 = DP(120);

	setformat_x3 = DP(315);
	setformat_x4 = DP(430);
	setformat_y1 = DP(20);
	setformat_y2 = DP(85);
	setformat_y3 = DP(125);
	setformat_margin = DP(30);
	setformat_channels_x = DP(25);
	setformat_channels_y = DP(242);
	setformat_channels_w = DP(250);
	setformat_channels_h = DP(250);

	loadfile_pad = get_image_set("loadmode_new")[0]->get_h() + 10;
	browse_pad = DP(20);


	new_toggle("playpatch.png", 
		"patch_up.png",
		"patch_hi.png",
		"patch_checked.png",
		"patch_dn.png",
		"patch_checkedhi.png",
		"playpatch_data");

	new_toggle("recordpatch.png", 
		"patch_up.png",
		"patch_hi.png",
		"patch_checked.png",
		"patch_dn.png",
		"patch_checkedhi.png",
		"recordpatch_data");

	new_toggle("gangpatch.png", 
		"patch_up.png",
		"patch_hi.png",
		"patch_checked.png",
		"patch_dn.png",
		"patch_checkedhi.png",
		"gangpatch_data");

	new_toggle("drawpatch.png", 
		"patch_up.png",
		"patch_hi.png",
		"patch_checked.png",
		"patch_dn.png",
		"patch_checkedhi.png",
		"drawpatch_data");


	new_image_set("mutepatch_data", 
		5,
		"mutepatch_up.png",
		"mutepatch_hi.png",
		"mutepatch_checked.png",
		"mutepatch_dn.png",
		"mutepatch_checkedhi.png");

	new_image_set("expandpatch_data", 
		5,
		"expandpatch_up.png",
		"expandpatch_hi.png",
		"expandpatch_checked.png",
		"expandpatch_dn.png",
		"expandpatch_checkedhi.png");

	build_bg_data();
	build_overlays();




	out_point = new_image_set(5,
		"out_up.png", 
		"out_hi.png", 
		"out_checked.png", 
		"out_dn.png", 
		"out_checkedhi.png");
	in_point = new_image_set(5,
		"in_up.png", 
		"in_hi.png", 
		"in_checked.png", 
		"in_dn.png", 
		"in_checkedhi.png");

	label_toggle = new_image_set(5,
		"labeltoggle_up.png", 
		"labeltoggle_uphi.png", 
		"label_checked.png", 
		"labeltoggle_dn.png", 
		"label_checkedhi.png");

	new_image_set("histogram_carrot",
		5,
		"histogram_carrot_up.png", 
		"histogram_carrot_hi.png", 
		"histogram_carrot_checked.png", 
		"histogram_carrot_dn.png", 
		"histogram_carrot_checkedhi.png");


	statusbar_cancel_data = new_image_set(3,
		"statusbar_cancel_up.png",
		"statusbar_cancel_hi.png",
		"statusbar_cancel_dn.png");


	VFrame *editpanel_up = new_image("editpanel_up.png");
	VFrame *editpanel_hi = new_image("editpanel_hi.png");
	VFrame *editpanel_dn = new_image("editpanel_dn.png");
	VFrame *editpanel_checked = new_image("editpanel_checked.png");
	VFrame *editpanel_checkedhi = new_image("editpanel_checkedhi.png");

	new_image("panel_divider", "panel_divider.png");
	new_button("bottom_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "bottom_justify");
	new_button("center_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "center_justify");
	new_button("channel.png", editpanel_up, editpanel_hi, editpanel_dn, "channel");

	new_toggle("histogram.png", 
		editpanel_up, 
		editpanel_hi, 
		editpanel_checked, 
		editpanel_dn, 
		editpanel_checkedhi, 
		"histogram_toggle");
	new_toggle("histogram_rgb.png", 
		editpanel_up, 
		editpanel_hi, 
		editpanel_checked, 
		editpanel_dn, 
		editpanel_checkedhi, 
		"histogram_rgb_toggle");
	new_toggle("waveform.png", 
		editpanel_up, 
		editpanel_hi, 
		editpanel_checked, 
		editpanel_dn, 
		editpanel_checkedhi, 
		"waveform_toggle");
	new_toggle("waveform_rgb.png", 
		editpanel_up, 
		editpanel_hi, 
		editpanel_checked, 
		editpanel_dn, 
		editpanel_checkedhi, 
		"waveform_rgb_toggle");
	new_toggle("scope.png", 
		editpanel_up, 
		editpanel_hi, 
		editpanel_checked, 
		editpanel_dn, 
		editpanel_checkedhi, 
		"scope_toggle");

	new_button("picture.png", editpanel_up, editpanel_hi, editpanel_dn, "picture");
	new_button("histogram.png", editpanel_up, editpanel_hi, editpanel_dn, "histogram");

	new_button("razor.png", editpanel_up, editpanel_hi, editpanel_dn, "razor");
	new_button("copy.png", editpanel_up, editpanel_hi, editpanel_dn, "copy");
	new_button("cut.png", editpanel_up, editpanel_hi, editpanel_dn, "cut");
	new_button("fit.png", editpanel_up, editpanel_hi, editpanel_dn, "fit");
	new_button("fitautos.png", editpanel_up, editpanel_hi, editpanel_dn, "fitautos");
	new_button("inpoint.png", editpanel_up, editpanel_hi, editpanel_dn, "inbutton");
	new_button("label.png", editpanel_up, editpanel_hi, editpanel_dn, "labelbutton");
	new_button("left_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "left_justify");
	new_button("magnify.png", editpanel_up, editpanel_hi, editpanel_dn, "magnify_button");
	new_button("middle_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "middle_justify");
	new_button("nextlabel.png", editpanel_up, editpanel_hi, editpanel_dn, "nextlabel");
	new_button("prevlabel.png", editpanel_up, editpanel_hi, editpanel_dn, "prevlabel");
	new_button("nextedit.png", editpanel_up, editpanel_hi, editpanel_dn, "nextedit");
	new_button("prevedit.png", editpanel_up, editpanel_hi, editpanel_dn, "prevedit");
	new_button("outpoint.png", editpanel_up, editpanel_hi, editpanel_dn, "outbutton");
	over_button = new_button("over.png", editpanel_up, editpanel_hi, editpanel_dn);
	overwrite_data = new_button("overwrite.png", editpanel_up, editpanel_hi, editpanel_dn);
	new_button("paste.png", editpanel_up, editpanel_hi, editpanel_dn, "paste");
	new_button("redo.png", editpanel_up, editpanel_hi, editpanel_dn, "redo");
	new_button("right_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "right_justify");
	splice_data = new_button("splice.png", editpanel_up, editpanel_hi, editpanel_dn);
	new_button("toclip.png", editpanel_up, editpanel_hi, editpanel_dn, "toclip");
	new_button("top_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "top_justify");
	new_button("undo.png", editpanel_up, editpanel_hi, editpanel_dn, "undo");
	new_button("wrench.png", editpanel_up, editpanel_hi, editpanel_dn, "wrench");


	VFrame *transport_up = new_image("transportup.png");
	VFrame *transport_hi = new_image("transporthi.png");
	VFrame *transport_dn = new_image("transportdn.png");

	new_button("end.png", transport_up, transport_hi, transport_dn, "end");
	new_button("fastfwd.png", transport_up, transport_hi, transport_dn, "fastfwd");
	new_button("fastrev.png", transport_up, transport_hi, transport_dn, "fastrev");
	new_button("play.png", transport_up, transport_hi, transport_dn, "play");
	new_button("framefwd.png", transport_up, transport_hi, transport_dn, "framefwd");
	new_button("framerev.png", transport_up, transport_hi, transport_dn, "framerev");
	new_button("pause.png", transport_up, transport_hi, transport_dn, "pause");
	new_button("record.png", transport_up, transport_hi, transport_dn, "record");
	new_button("singleframe.png", transport_up, transport_hi, transport_dn, "recframe");
	new_button("reverse.png", transport_up, transport_hi, transport_dn, "reverse");
	new_button("rewind.png", transport_up, transport_hi, transport_dn, "rewind");
	new_button("stop.png", transport_up, transport_hi, transport_dn, "stop");
	new_button("stop.png", transport_up, transport_hi, transport_dn, "stoprec");



// CWindow icons
	new_image("cwindow_inactive", "cwindow_inactive.png");
	new_image("cwindow_active", "cwindow_active.png");



	new_image_set("category_button",
		3,
		"preferencesbutton_dn.png",
		"preferencesbutton_dnhi.png",
		"preferencesbutton_dnlo.png");

	new_image_set("category_button_checked",
		3,
		"preferencesbutton_up.png",
		"preferencesbutton_uphi.png",
		"preferencesbutton_dnlo.png");



	new_image_set("color3way_point", 
		3,
		"color3way_up.png", 
		"color3way_hi.png", 
		"color3way_dn.png");




	new_toggle("arrow.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "arrow");
	new_toggle("autokeyframe.png", transport_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "autokeyframe");
	new_toggle("ibeam.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "ibeam");
	new_toggle("show_meters.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "meters");

	VFrame *cpanel_up = new_image("cpanel_up.png");
	VFrame *cpanel_hi = new_image("cpanel_hi.png");
	VFrame *cpanel_dn = new_image("cpanel_dn.png");
	VFrame *cpanel_checked = new_image("cpanel_checked.png");
	VFrame *cpanel_checkedhi = new_image("cpanel_checkedhi.png");


	new_toggle("camera.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "camera");
	new_toggle("crop.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "crop");
	new_toggle("eyedrop.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "eyedrop");
	new_toggle("magnify.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "magnify");
	new_toggle("mask.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "mask");
	new_toggle("ruler.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "ruler");
	new_toggle("projector.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "projector");
	new_toggle("protect.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "protect");
	new_toggle("titlesafe.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "titlesafe");
	new_toggle("toolwindow.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "tool");


// speed preferences
    new_image("speed_4", "speed_4.png");
    new_image("speed_5", "speed_5.png");
    new_image("speed_6", "speed_6.png");
    new_image("speed_plus", "speed_plus.png");
    new_image("speed_9", "speed_9.png");
    new_image("speed_1", "speed_1.png");
    new_image("speed_2", "speed_2.png");
    new_image("speed_3", "speed_3.png");
    new_image("speed_enter", "speed_enter.png");
    new_image("speed_period", "speed_period.png");
    new_image("speed_rev", "speed_rev.png");
    new_image("speed_fwd", "speed_fwd.png");


	flush_images();

	title_font = MEDIUMFONT;
	title_color = 0x000000;
	recordgui_fixed_color = BLACK;
	recordgui_variable_color = BLACK;

	channel_position_color = BLACK;
	resources->meter_title_w = DP(30);
}



void BrightTheme::build_bg_data()
{
// Audio settings
	channel_position_data = new VFrame();
	channel_position_data->read_png(get_image_data("channel_position.png"), BC_Resources::dpi);

// Track bitmaps
	new_image("resource1024", "resource1024.png");
	new_image("resource512", "resource512.png");
	new_image("resource256", "resource256.png");
	new_image("resource128", "resource128.png");
	new_image("resource64", "resource64.png");
	new_image("resource32", "resource32.png");
	new_image("plugin_bg_data", "plugin_bg.png");
	new_image("title_bg_data", "title_bg.png");
	new_image("vtimebar_bg_data", "vwindow_timebar.png");
}



void BrightTheme::build_overlays()
{
	keyframe_data = new VFrame(get_image_data("keyframe3.png"));
	camerakeyframe_data = new VFrame(get_image_data("camerakeyframe.png"));
	maskkeyframe_data = new VFrame(get_image_data("maskkeyframe.png"));
	modekeyframe_data = new VFrame(get_image_data("modekeyframe.png"));
	pankeyframe_data = new VFrame(get_image_data("pankeyframe.png"));
	projectorkeyframe_data = new VFrame(get_image_data("projectorkeyframe.png"));
}





void BrightTheme::get_mwindow_sizes(MWindowGUI *gui, int w, int h)
{
	Theme::get_mwindow_sizes(gui, w, h);
	mclock_y += 2;
	
	
}

void BrightTheme::get_vwindow_sizes(VWindowGUI *gui)
{
	Theme::get_vwindow_sizes(gui);
}




void BrightTheme::draw_rwindow_bg(RecordGUI *gui)
{
}

void BrightTheme::draw_rmonitor_bg(RecordMonitorGUI *gui)
{
}






void BrightTheme::draw_mwindow_bg(MWindowGUI *gui)
{
// Button bar
	gui->draw_3segmenth(mbuttons_x, 
		mbuttons_y - 1, 
		mwindow->session->mwindow_w, 
		get_image("mbutton_bg"));

	gui->draw_vframe(get_image("panel_divider"),
		mbuttons_x + DP(228),
		mbuttons_y - 1);

	gui->draw_vframe(get_image("panel_divider"),
		mbuttons_x + DP(320),
		mbuttons_y - 1);

// Clock
	gui->draw_3segmenth(0, 
		mbuttons_y - 1 + get_image("mbutton_bg")->get_h(),
		get_image("patchbay_bg")->get_w(), 
		get_image("clock_bg"));

// Patchbay
	gui->draw_3segmentv(patchbay_x, 
		patchbay_y, 
		patchbay_h, 
		get_image("patchbay_bg"));

// Track canvas
	int patchbay_w = get_image("patchbay_bg")->get_w();
	gui->clear_box(mcanvas_x + patchbay_w, 
		mcanvas_y + mtimebar_h, 
		mcanvas_w - BC_ScrollBar::get_span(SCROLL_VERT) - patchbay_w, 
		patchbay_h - BC_ScrollBar::get_span(SCROLL_HORIZ) - mtimebar_h);

// Timebar
	gui->draw_3segmenth(mtimebar_x, 
		mtimebar_y, 
		mtimebar_w, 
		get_image("timebar_bg"));

// Zoombar
// 	gui->set_color(0x373737);
// 	gui->draw_box(mzoom_x, 
// 		mzoom_y,
// 		mwindow->session->mwindow_w,
// 		25);

// Scrollbar filler
//	gui->draw_vframe(get_image("mscroll_filler"), 
//		mhscroll_x + mhscroll_w,
//		mvscroll_y + mvscroll_h);

// Status
	gui->draw_3segmenth(mzoom_x,
		mzoom_y,
		mzoom_w,
		get_image("statusbar"));


}

void BrightTheme::draw_cwindow_bg(CWindowGUI *gui)
{
	BC_Resources *resources = BC_WindowBase::get_resources();
	
	gui->set_color(WHITE);
	gui->draw_box(ccanvas_x + ccanvas_w - resources->vscroll_data[0]->get_w(),
		ccanvas_y + ccanvas_h - resources->hscroll_data[0]->get_h(),
		resources->vscroll_data[0]->get_w(),
		resources->hscroll_data[0]->get_h());
	gui->draw_3segmentv(0, 0, ccomposite_h, get_image("cpanel_bg"));
	gui->draw_3segmenth(0, 
        ccomposite_h, 
        mwindow->session->cwindow_w, 
        get_image("cbuttons"));
#ifdef USE_METERS
	if(mwindow->edl->session->cwindow_meter)
	{
		gui->draw_3segmenth(cstatus_x, 
			ccomposite_h, 
			cmeter_x - widget_border - cstatus_x, 
			get_image("cbuttons_right"));
		gui->draw_9segment(cmeter_x - widget_border, 
			0, 
			mwindow->session->cwindow_w - cmeter_x + widget_border, 
			mwindow->session->cwindow_h, 
			get_image("cmeter_bg"));
	}
	else
#endif
	{
// 		gui->draw_3segmenth(cstatus_x, 
// 			ccomposite_h, 
// 			cmeter_x - widget_border - cstatus_x + DP(100), 
// 			get_image("cbuttons_right"));
	}
}

void BrightTheme::draw_vwindow_bg(VWindowGUI *gui)
{
	gui->draw_3segmenth(0, 
		vcanvas_h, 
		mwindow->session->vwindow_w, 
		get_image("cbuttons"));


#ifdef USE_METERS
	if(mwindow->edl->session->vwindow_meter)
	{
		gui->draw_3segmenth(vdivision_x, 
			vcanvas_h, 
			vmeter_x - widget_border - vdivision_x, 
			get_image("cbuttons_right"));
		gui->draw_9segment(vmeter_x - widget_border,
			0,
			mwindow->session->vwindow_w - vmeter_x + widget_border, 
			mwindow->session->vwindow_h, 
			get_image("cmeter_bg"));
	}
	else
#endif
	{
// 		gui->draw_3segmenth(vdivision_x, 
// 			vcanvas_h, 
// 			vmeter_x - widget_border - vdivision_x + DP(100), 
// 			get_image("cbuttons_right"));
	}

// Clock border
	gui->draw_3segmenth(vtime_x - DP(20), 
		vtime_y + 1, 
		vtime_w + DP(40),
		get_image("vclock"));
}



void BrightTheme::draw_preferences_bg(PreferencesWindow *gui)
{
	gui->draw_vframe(get_image("preferences_bg"), 0, 0);
}


void BrightTheme::draw_new_bg(NewWindow *gui)
{
	gui->draw_vframe(get_image("new_bg"), 0, 0);
}

void BrightTheme::draw_setformat_bg(SetFormatWindow *gui)
{
	gui->draw_vframe(get_image("setformat_bg"), 0, 0);
}



