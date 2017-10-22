
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "awindowgui.h"
#include "bcsignals.h"
#include "clip.h"
#include "colors.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "keyframegui.h"
#include "language.h"
#include "levelwindowgui.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "patchbay.h"
#include "playtransport.h"
#include "presetsgui.h"
#include "recordgui.h"
#include "recordmonitor.h"
#include "resourcepixmap.h"
#include "statusbar.h"
#include "theme.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "vwindowgui.h"
#include "zoombar.h"


#include <errno.h>
#include <string.h>

#define NEW_VFRAME \
new VFrame(0, -1, default_data.get_w(), default_data.get_h(), BC_RGBA8888, -1)

Theme::Theme()
 : BC_Theme()
{
	window_border = DP(10);
	widget_border = DP(5);
	this->mwindow = 0;
	theme_title = DEFAULT_THEME;
	data_buffer = 0;
	contents_buffer = 0;
	last_image = 0;
	mtransport_margin = DP(0);
	toggle_margin = DP(0);
	control_pixels = DP(50);
	timebar_cursor_color = RED;

	BC_WindowBase::get_resources()->bg_color = BLOND;
	BC_WindowBase::get_resources()->button_up = 0xffc000;
	BC_WindowBase::get_resources()->button_highlighted = 0xffe000;
	BC_WindowBase::get_resources()->recursive_resizing = 0;
	audio_color = BLACK;
	fade_h = DP(22);
	meter_h = DP(17);
	mode_h = DP(30);
	pan_h = DP(32);
	pan_x = DP(50);
	play_h = DP(22);
	title_h = DP(23);
	clock_bg_color = BLACK;
	clock_fg_color = GREEN;
	assetedit_color = YELLOW;

	preferences_category_overlap = 0;

	loadmode_w = DP(350);

#include "data/about_png.h"
	about_bg = new VFrame(about_png);

	pane_color = BLACK;
	drag_pane_color = WHITE;

	


}


// Need to delete everything here
Theme::~Theme()
{
	flush_images();	

	aspect_ratios.remove_all_objects();
	frame_rates.remove_all_objects();
	frame_sizes.remove_all_objects();
	sample_rates.remove_all_objects();
	zoom_values.remove_all_objects();
}

void Theme::flush_images()
{
	if(data_buffer) delete [] data_buffer;
	if(contents_buffer) delete [] contents_buffer;
	data_buffer = 0;
	contents_buffer = 0;
	contents.remove_all();
}

void Theme::initialize()
{
	message_normal = BLACK;
	message_error = RED;

// Force to use local data for images
	extern unsigned char _binary_theme_data_start[];
	set_data(_binary_theme_data_start);

// Set images which weren't set by subclass
	new_image("mode_add", "mode_add.png");
	new_image("mode_divide", "mode_divide.png");
	new_image("mode_multiply", "mode_multiply.png");
	new_image("mode_normal", "mode_normal.png");
	new_image("mode_replace", "mode_replace.png");
	new_image("mode_subtract", "mode_subtract.png");


// Images all themes have
	new_image("mwindow_icon", "heroine_icon.png");
	new_image("vwindow_icon", "heroine_icon.png");
	new_image("cwindow_icon", "heroine_icon.png");
	new_image("awindow_icon", "heroine_icon.png");
	new_image("record_icon", "heroine_icon.png");
	new_image("clip_icon", "clip_icon.png");


	new_image("aeffect_icon", "aeffect_icon.png");
	new_image("veffect_icon", "veffect_icon.png");
	new_image("atransition_icon", "atransition_icon.png");
	new_image("vtransition_icon", "atransition_icon.png");
}






void Theme::build_menus()
{


	aspect_ratios.append(new BC_ListBoxItem("3:2"));
	aspect_ratios.append(new BC_ListBoxItem("4:3"));
	aspect_ratios.append(new BC_ListBoxItem("16:9"));
	aspect_ratios.append(new BC_ListBoxItem("2.10:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.20:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.25:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.30:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.35:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.66:1"));
	frame_sizes.append(new BC_ListBoxItem("160x120"));
	frame_sizes.append(new BC_ListBoxItem("240x180"));
	frame_sizes.append(new BC_ListBoxItem("320x240"));
	frame_sizes.append(new BC_ListBoxItem("360x240"));
	frame_sizes.append(new BC_ListBoxItem("400x300"));
	frame_sizes.append(new BC_ListBoxItem("640x480"));
	frame_sizes.append(new BC_ListBoxItem("720x480"));
	frame_sizes.append(new BC_ListBoxItem("1280x720"));
	frame_sizes.append(new BC_ListBoxItem("1920x1080"));
	frame_sizes.append(new BC_ListBoxItem("3840x2160"));
	sample_rates.append(new BC_ListBoxItem("8000"));
	sample_rates.append(new BC_ListBoxItem("16000"));
	sample_rates.append(new BC_ListBoxItem("22050"));
	sample_rates.append(new BC_ListBoxItem("32000"));
	sample_rates.append(new BC_ListBoxItem("44100"));
	sample_rates.append(new BC_ListBoxItem("48000"));
	sample_rates.append(new BC_ListBoxItem("96000"));
	sample_rates.append(new BC_ListBoxItem("192000"));
	frame_rates.append(new BC_ListBoxItem("1"));
	frame_rates.append(new BC_ListBoxItem("5"));
	frame_rates.append(new BC_ListBoxItem("10"));
	frame_rates.append(new BC_ListBoxItem("12"));
	frame_rates.append(new BC_ListBoxItem("15"));
	frame_rates.append(new BC_ListBoxItem("23.97"));
	frame_rates.append(new BC_ListBoxItem("24"));
	frame_rates.append(new BC_ListBoxItem("25"));
	frame_rates.append(new BC_ListBoxItem("29.97"));
	frame_rates.append(new BC_ListBoxItem("30"));
	frame_rates.append(new BC_ListBoxItem("50"));
	frame_rates.append(new BC_ListBoxItem("59.94"));
	frame_rates.append(new BC_ListBoxItem("60"));
	char string[BCTEXTLEN];
	for(int i = 1; i < 17; i++)
	{
		sprintf(string, "%d", (int)pow(2, i));
		zoom_values.append(new BC_ListBoxItem(string));
	}
}


void Theme::overlay(VFrame *dst, VFrame *src, int in_x1, int in_x2)
{
	int w;
	int h;
	unsigned char **in_rows;
	unsigned char **out_rows;

	if(in_x1 < 0)
	{
		w = MIN(src->get_w(), dst->get_w());
		h = MIN(dst->get_h(), src->get_h());
		in_x1 = 0;
		in_x2 = w;
	}
	else
	{
		w = in_x2 - in_x1;
		h = MIN(dst->get_h(), src->get_h());
	}
	in_rows = src->get_rows();
	out_rows = dst->get_rows();

	switch(src->get_color_model())
	{
		case BC_RGBA8888:
			switch(dst->get_color_model())
			{
				case BC_RGBA8888:
					for(int i = 0; i < h; i++)
					{
						unsigned char *in_row = in_rows[i] + in_x1 * 4;
						unsigned char *out_row = out_rows[i];
						for(int j = 0; j < w; j++)
						{
							int opacity = in_row[3];
							int transparency = out_row[3] * (0xff - opacity) / 0xff;
							out_row[0] = (in_row[0] * opacity + out_row[0] * transparency) / 0xff;
							out_row[1] = (in_row[1] * opacity + out_row[1] * transparency) / 0xff;
							out_row[2] = (in_row[2] * opacity + out_row[2] * transparency) / 0xff;
							out_row[3] = MAX(in_row[3], out_row[3]);
							out_row += 4;
							in_row += 4;
						}
					}
					break;
				case BC_RGB888:
					for(int i = 0; i < h; i++)
					{
						unsigned char *in_row = in_rows[i] + in_x1 * 4;
						unsigned char *out_row = out_rows[i];
						for(int j = 0; j < w; j++)
						{
							int opacity = in_row[3];
							int transparency = 0xff - opacity;
							out_row[0] = (in_row[0] * opacity + out_row[0] * transparency) / 0xff;
							out_row[1] = (in_row[1] * opacity + out_row[1] * transparency) / 0xff;
							out_row[2] = (in_row[2] * opacity + out_row[2] * transparency) / 0xff;
							out_row += 3;
							in_row += 4;
						}
					}
					break;
			}
			break;
	}
}

void Theme::build_transport(char *title,
	unsigned char *png_overlay,
	VFrame **bg_data,
	int third)
{
	if(!png_overlay) return;
	VFrame default_data(png_overlay);
	VFrame *data[3];
	data[0] = NEW_VFRAME;
	data[1] = NEW_VFRAME;
	data[2] = NEW_VFRAME;
	data[0]->clear_frame();
	data[1]->clear_frame();
	data[2]->clear_frame();

	for(int i = 0; i < 3; i++)
	{
		int in_x1;
		int in_x2;
		if(!bg_data[i]) break;

		switch(third)
		{
			case 0:
				in_x1 = 0;
				in_x2 = default_data.get_w();
				break;

			case 1:
				in_x1 = (int)(bg_data[i]->get_w() * 0.33);
				in_x2 = in_x1 + default_data.get_w();
				break;

			case 2:
				in_x1 = bg_data[i]->get_w() - default_data.get_w();
				in_x2 = in_x1 + default_data.get_w();
				break;
		}

		overlay(data[i], 
			bg_data[i],
			in_x1,
			in_x2);
		overlay(data[i], 
			&default_data);
	}

	new_image_set_images(title, 3, data[0], data[1], data[2]);
}









void Theme::build_patches(VFrame** &data,
	unsigned char *png_overlay,
	VFrame **bg_data,
	int region)
{
	if(!png_overlay || !bg_data) return;
	VFrame default_data(png_overlay);
	data = new VFrame*[5];
	data[0] = NEW_VFRAME;
	data[1] = NEW_VFRAME;
	data[2] = NEW_VFRAME;
	data[3] = NEW_VFRAME;
	data[4] = NEW_VFRAME;

	for(int i = 0; i < 5; i++)
	{
		int in_x1;
		int in_x2;

		switch(region)
		{
			case 0:
				in_x1 = 0;
				in_x2 = default_data.get_w();
				break;

			case 1:
				in_x1 = (int)(bg_data[i]->get_w() * 0.33);
				in_x2 = in_x1 + default_data.get_w();
				break;

			case 2:
				in_x1 = bg_data[i]->get_w() - default_data.get_w();
				in_x2 = in_x1 + default_data.get_w();
				break;
		}

		overlay(data[i], 
			bg_data[i]);
		overlay(data[i], 
			&default_data);
	}
}








void Theme::build_button(VFrame** &data,
	unsigned char *png_overlay,
	VFrame *up_vframe,
	VFrame *hi_vframe,
	VFrame *dn_vframe)
{
	if(!png_overlay) return;
	VFrame default_data(png_overlay);

	if(!up_vframe || !hi_vframe || !dn_vframe) return;
	data = new VFrame*[3];
	data[0] = NEW_VFRAME;
	data[1] = NEW_VFRAME;
	data[2] = NEW_VFRAME;
	data[0]->copy_from(up_vframe);
	data[1]->copy_from(hi_vframe);
	data[2]->copy_from(dn_vframe);
	for(int i = 0; i < 3; i++)
		overlay(data[i], 
			&default_data);
}

void Theme::build_toggle(VFrame** &data,
	unsigned char *png_overlay,
	VFrame *up_vframe,
	VFrame *hi_vframe,
	VFrame *checked_vframe,
	VFrame *dn_vframe,
	VFrame *checkedhi_vframe)
{
	if(!png_overlay || 
		!up_vframe || 
		!hi_vframe || 
		!checked_vframe || 
		!dn_vframe || 
		!checkedhi_vframe) return;
	VFrame default_data(png_overlay);
	data = new VFrame*[5];
	data[0] = NEW_VFRAME;
	data[1] = NEW_VFRAME;
	data[2] = NEW_VFRAME;
	data[3] = NEW_VFRAME;
	data[4] = NEW_VFRAME;
	data[0]->copy_from(up_vframe);
	data[1]->copy_from(hi_vframe);
	data[2]->copy_from(checked_vframe);
	data[3]->copy_from(dn_vframe);
	data[4]->copy_from(checkedhi_vframe);
	for(int i = 0; i < 5; i++)
		overlay(data[i], 
			&default_data);
}


void Theme::get_mwindow_sizes(MWindowGUI *gui, int w, int h)
{
	mbuttons_x = DP(0);
	mbuttons_y = gui->mainmenu->get_h() + 1;
	mbuttons_w = w;
	mbuttons_h = get_image("mbutton_bg")->get_h();
	mclock_x = window_border - DP(5);
	mclock_y = mbuttons_y - 1 + mbuttons_h;
	mclock_w = get_image("clock_bg")->get_w() - DP(20);
	mclock_h = get_image("clock_bg")->get_h();
	mtimebar_x = get_image("patchbay_bg")->get_w();
	mtimebar_y = mbuttons_y - 1 + mbuttons_h;
	mtimebar_w = w - mtimebar_x;
	mtimebar_h = get_image("timebar_bg")->get_h();
	mzoom_h = DP(25);
	mzoom_x = DP(0);
	mzoom_y = h - get_image("statusbar")->get_h();
	mzoom_w = w;
	mstatus_x = DP(0);
	mstatus_y = mzoom_y + mzoom_h;
	mstatus_w = w;
	mstatus_h = h - mstatus_y;
	mstatus_message_x = DP(10);
	mstatus_message_y = DP(5);
	mstatus_progress_x = mstatus_w - statusbar_cancel_data[0]->get_w() - DP(240);
	mstatus_progress_y = mstatus_h - BC_WindowBase::get_resources()->progress_images[0]->get_h() - 3;
	mstatus_progress_w = DP(230);
	mstatus_cancel_x = mstatus_w - statusbar_cancel_data[0]->get_w();
	mstatus_cancel_y = mstatus_h - statusbar_cancel_data[0]->get_h();
	mcanvas_x = 0;
	mcanvas_y = mbuttons_y - 1 + mbuttons_h;
	mcanvas_w = w;
	mcanvas_h = mzoom_y - mtimebar_y;
	patchbay_x = 0;
	patchbay_y = mcanvas_y + mclock_h;
	patchbay_w = get_image("patchbay_bg")->get_w();
	patchbay_h = mcanvas_h - mclock_h;
	pane_w = get_image_set("xpane")[0]->get_w();
	pane_h = get_image_set("ypane")[0]->get_h();
	pane_x = mcanvas_x + mcanvas_w;
	pane_y = mcanvas_y + mcanvas_h;
	mhscroll_x = 0;
	mhscroll_y = mcanvas_y + mcanvas_h - BC_ScrollBar::get_span(SCROLL_HORIZ);
	mhscroll_w = w - BC_ScrollBar::get_span(SCROLL_VERT);
	mvscroll_x = mcanvas_x + mcanvas_w;
	mvscroll_y = mcanvas_y;
	mvscroll_h = mcanvas_h;
}

void Theme::get_pane_sizes(MWindowGUI *gui, 
	int *view_x,
	int *view_y,
	int *view_w, 
	int *view_h, 
	int number,
	int x,
	int y,
	int w, 
	int h)
{
	*view_x = x;
	*view_y = y;
	*view_w = w;
	*view_h = h;
	
// patchbay
	if(number == TOP_LEFT_PANE ||
		number == BOTTOM_LEFT_PANE)
	{
		*view_x += patchbay_w;
		*view_w -= patchbay_w;
	}

// timebar
	if(number == TOP_LEFT_PANE || 
		number == TOP_RIGHT_PANE)
	{
		*view_y += mtimebar_h;
		*view_h -= mtimebar_h;
	}

// samplescroll
	if(number == BOTTOM_LEFT_PANE ||
		number == BOTTOM_RIGHT_PANE ||
		gui->horizontal_panes() ||
		gui->total_panes() == 1)
	{
		*view_h -= BC_ScrollBar::get_span(SCROLL_HORIZ);
	}

// trackscroll
	if(number == TOP_RIGHT_PANE ||
		number == BOTTOM_RIGHT_PANE ||
		gui->vertical_panes() ||
		gui->total_panes() == 1)
	{
		*view_w -= BC_ScrollBar::get_span(SCROLL_VERT);
	}
}

void Theme::draw_mwindow_bg(MWindowGUI *gui)
{
}


void Theme::get_cwindow_sizes(CWindowGUI *gui, int cwindow_controls)
{
	czoom_w = DP(80);

	int edit_w = EditPanel::calculate_w(mwindow, 1, 14);
	int transport_w = PlayTransport::get_transport_width(mwindow) + toggle_margin;
	int zoom_w = ZoomPanel::calculate_w(czoom_w);
	int status_w = get_image("cwindow_active")->get_w();
// Space between buttons & status icon
	int division_w = DP(40);

	ctimebar_h = DP(16);

	if(cwindow_controls)
	{
SET_TRACE
		if(mwindow->edl->session->cwindow_meter)
		{
			cmeter_x = mwindow->session->cwindow_w - 
				MeterPanel::get_meters_width(this,
					mwindow->edl->session->audio_channels, 
					mwindow->edl->session->cwindow_meter);
		}
		else
		{
			cmeter_x = mwindow->session->cwindow_w + widget_border;
		}

		int buttons_h;
#ifdef USE_SLIDER
		cslider_x = DP(5);
		cslider_y = ccomposite_h + DP(20);
		cedit_y = cslider_y + BC_Slider::get_span(0);
#endif

		if(edit_w + 
			widget_border * 2 + 
			transport_w + widget_border + 
			zoom_w + widget_border + 
			division_w +
			status_w > cmeter_x)
		{
			buttons_h = get_image("cbuttons_left")->get_h();

			cedit_x = widget_border;
			cedit_y = mwindow->session->cwindow_h - 
				buttons_h + 
				ctimebar_h + 
				widget_border;

			ctransport_x = widget_border;
			ctransport_y = mwindow->session->cwindow_h - 
				get_image_set("autokeyframe")[0]->get_h() - 
				widget_border;

			czoom_x = ctransport_x + transport_w + widget_border;
			czoom_y = ctransport_y + widget_border;

			cstatus_x = DP(440);
			cstatus_y = mwindow->session->cwindow_h - 
				get_image("cwindow_active")->get_h() - DP(30);
		}
		else
		{
			buttons_h = ctimebar_h + 
				widget_border +
				EditPanel::calculate_h(mwindow) +
				widget_border;
			ctransport_x = widget_border;
			ctransport_y = mwindow->session->cwindow_h - 
				buttons_h + 
				ctimebar_h + 
				widget_border;
			
			cedit_x = ctransport_x + transport_w + widget_border;
			cedit_y = ctransport_y;

			czoom_x = cedit_x + edit_w + widget_border;
			czoom_y = cedit_y + widget_border;
//printf("Theme::get_cwindow_sizes %d %d %d\n", __LINE__, czoom_x, zoom_w);
			cstatus_x = czoom_x + zoom_w + division_w;
			cstatus_y = ctransport_y;
		}


		ccomposite_x = 0;
		ccomposite_y = DP(5);
		ccomposite_w = get_image("cpanel_bg")->get_w();
		ccomposite_h = mwindow->session->cwindow_h - buttons_h;







		ccanvas_x = ccomposite_x + ccomposite_w;
		ccanvas_y = 0;
		ccanvas_h = ccomposite_h;


		ccanvas_w = cmeter_x - ccanvas_x - widget_border;
SET_TRACE
	}
	else
// no controls
	{
SET_TRACE
		ccomposite_x = -get_image("cpanel_bg")->get_w();
		ccomposite_y = DP(0);
		ccomposite_w = get_image("cpanel_bg")->get_w();
		ccomposite_h = mwindow->session->cwindow_h - get_image("cbuttons_left")->get_h();

		cslider_x = DP(5);
		cslider_y = mwindow->session->cwindow_h;
		cedit_x = DP(10);
		cedit_y = cslider_y + DP(17);
		ctransport_x = DP(10);
		ctransport_y = cedit_y + DP(40);
		ccanvas_x = DP(0);
		ccanvas_y = DP(0);
		ccanvas_w = mwindow->session->cwindow_w;
		ccanvas_h = mwindow->session->cwindow_h;
		cmeter_x = mwindow->session->cwindow_w;
		cstatus_x = mwindow->session->cwindow_w;
		cstatus_y = mwindow->session->cwindow_h;
		czoom_x = mwindow->session->cwindow_w;
		czoom_y = mwindow->session->cwindow_h;
SET_TRACE
	}

SET_TRACE


	cmeter_y = widget_border;
	cmeter_h = mwindow->session->cwindow_h - cmeter_y - widget_border;

	cslider_w = ccanvas_x + ccanvas_w - cslider_x - widget_border;

//	ctimebar_x = ccanvas_x;
//	ctimebar_y = ccanvas_y + ccanvas_h;
//	ctimebar_w = ccanvas_w;
	ctimebar_x = DP(0);
	ctimebar_y = ccanvas_y + ccanvas_h;
	ctimebar_w = mwindow->session->cwindow_w;
	if(mwindow->edl->session->cwindow_meter)
	{
		ctimebar_w = cmeter_x - widget_border;
	}

SET_TRACE
}





void Theme::draw_awindow_bg(AWindowGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->awindow_w, mwindow->session->awindow_h);
	gui->flash();
}

void Theme::draw_vwindow_bg(VWindowGUI *gui)
{
// 	gui->clear_box(0, 
// 		0, 
// 		mwindow->session->vwindow_w, 
// 		mwindow->session->vwindow_h);
// // Timebar
// 	gui->draw_3segmenth(vtimebar_x, 
// 		vtimebar_y, 
// 		vtimebar_w, 
// 		vtimebar_bg_data,
// 		0);
// 	gui->flash();
}


void Theme::draw_cwindow_bg(CWindowGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->cwindow_w, mwindow->session->cwindow_h);
	gui->flash();
}

void Theme::draw_lwindow_bg(LevelWindowGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->lwindow_w, mwindow->session->lwindow_h);
	gui->flash();
}


void Theme::draw_rmonitor_bg(RecordMonitorGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->rmonitor_w, mwindow->session->rmonitor_h);
	gui->flash();
}


void Theme::draw_rwindow_bg(RecordGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->rwindow_w, mwindow->session->rwindow_h);
}


void Theme::draw_resource_bg(TrackCanvas *canvas,
	ResourcePixmap *pixmap, 
	int edit_x,
	int edit_w,
	int pixmap_x,
	int x1, 
	int y1, 
	int x2,
	int y2)
{
	VFrame *image;

	switch(mwindow->edl->local_session->zoom_track)
	{
		case 1024: image = get_image("resource1024");  break;
		case 512: image = get_image("resource512");  break;
		case 256: image = get_image("resource256");  break;
		case 128: image = get_image("resource128");  break;
		case 64:  image = get_image("resource64");   break;
		default:
		case 32:  image = get_image("resource32");   break;
	}

	canvas->draw_3segmenth(x1, 
		y1, 
		x2 - x1, 
		edit_x - pixmap_x,
		edit_w,
		image,
		pixmap);
}


void Theme::get_vwindow_sizes(VWindowGUI *gui)
{
	int edit_w = EditPanel::calculate_w(mwindow, 0, 10);
	int transport_w = PlayTransport::get_transport_width(mwindow) + toggle_margin;
// Space between buttons & time
	int division_w = DP(30);
	vtime_w = DP(150);
	vtimebar_h = DP(16);
	int vtime_border = DP(15);

	vmeter_y = widget_border;
	vmeter_h = mwindow->session->vwindow_h - cmeter_y - widget_border;

	int buttons_h;
	if(mwindow->edl->session->vwindow_meter)
	{
		vmeter_x = mwindow->session->vwindow_w - 
			MeterPanel::get_meters_width(this,
				mwindow->edl->session->audio_channels, 
				mwindow->edl->session->vwindow_meter);
	}
	else
	{
		vmeter_x = mwindow->session->vwindow_w + widget_border;
	}

	vcanvas_x = DP(0);
	vcanvas_y = DP(0);
	vcanvas_w = vmeter_x - vcanvas_x - widget_border;

#ifdef USE_SLIDER
	vslider_x = DP(10);
	vslider_y = vtimebar_y + DP(20);
	vslider_w = vtimebar_w - vslider_x;
	vedit_y = vslider_y + BC_Slider::get_span(0);
#endif

	if(edit_w +
		widget_border * 2 +
		transport_w + widget_border +
		vtime_w + division_w +
		vtime_border > vmeter_x)
	{
		buttons_h = get_image("vbuttons_left")->get_h();
		vedit_x = widget_border;
		vedit_y = mwindow->session->vwindow_h -
			buttons_h + 
			vtimebar_h + 
			widget_border;
		
		vtransport_x = widget_border;
		vtransport_y = mwindow->session->vwindow_h - 
			get_image_set("autokeyframe")[0]->get_h() - 
			widget_border;

		vdivision_x = DP(280);
		vtime_x = vdivision_x;
		vtime_y = vedit_y + DP(20);
	}
	else
	{
		buttons_h = vtimebar_h + 
			widget_border + 
			EditPanel::calculate_h(mwindow) +
			widget_border;
		vtransport_x = widget_border;
		vtransport_y = mwindow->session->vwindow_h - 
			buttons_h +
			vtimebar_h + 
			widget_border;
		
		vedit_x = vtransport_x + transport_w + widget_border;
		vedit_y = vtransport_y;
		
		vdivision_x = vedit_x + edit_w + division_w;
		vtime_x = vdivision_x + vtime_border;
		vtime_y = vedit_y + widget_border;
	}


//	vtimebar_x = vcanvas_x;
//	vtimebar_y = vcanvas_y + vcanvas_h;
//	vtimebar_w = vcanvas_w;

	vcanvas_h = mwindow->session->vwindow_h - buttons_h;
	vtimebar_x = DP(0);
	vtimebar_y = vcanvas_y + vcanvas_h;
	vtimebar_w = vmeter_x - widget_border;

}


void Theme::get_awindow_sizes(AWindowGUI *gui)
{
	abuttons_x = 0; 
	abuttons_y = 0;
	afolders_x = 0;
//	afolders_y = deletedisk_data[0]->get_h();
	afolders_y = 0;
	afolders_w = mwindow->session->afolders_w;
	afolders_h = mwindow->session->awindow_h - afolders_y;
	adivider_x = afolders_x + afolders_w;
	adivider_y = 0;
	adivider_w = DP(5);
	adivider_h = afolders_h;
	alist_x = afolders_x + afolders_w + DP(5);
	alist_y = afolders_y;
	alist_w = mwindow->session->awindow_w - alist_x;
	alist_h = afolders_h;
}

void Theme::get_rmonitor_sizes(int do_audio, 
	int do_video,
	int do_channel,
	int do_interlace,
	int do_avc,
	int audio_channels)
{
	int x = DP(10);
	int y = DP(3);


	if(do_avc)
	{
		rmonitor_canvas_y = DP(30);
		rmonitor_tx_x = DP(10);
		rmonitor_tx_y = DP(0);
	}
	else
	{
		rmonitor_canvas_y = 0;
		rmonitor_tx_x = 0;
		rmonitor_tx_y = 0;
	}


	if(do_channel)
	{
		y = DP(5);
		rmonitor_channel_x = x;
		rmonitor_channel_y = DP(5);
		x += DP(235);
		rmonitor_canvas_y = DP(35);
	}

	if(do_interlace)
	{
		y = DP(4);
		rmonitor_interlace_x = x;
		rmonitor_interlace_y = y;
	}


	if(do_audio)
	{
		if(do_video)
		{
			rmonitor_meter_x = mwindow->session->rmonitor_w - 
				MeterPanel::get_meters_width(this, audio_channels, 1);
			rmonitor_meter_w = mwindow->session->rmonitor_w;
		}
		else
		{
			rmonitor_meter_x = widget_border;
			rmonitor_meter_w = mwindow->session->rmonitor_w - widget_border * 2;
		}

		rmonitor_meter_y = DP(40);
		rmonitor_meter_h = mwindow->session->rmonitor_h - 
			DP(10) - 
			rmonitor_meter_y;
	}
	else
	{
		rmonitor_meter_x = mwindow->session->rmonitor_w;
	}

	rmonitor_canvas_x = 0;
	rmonitor_canvas_w = rmonitor_meter_x - rmonitor_canvas_x;
	if(do_audio) rmonitor_canvas_w -= DP(10);
	rmonitor_canvas_h = mwindow->session->rmonitor_h - rmonitor_canvas_y;

	if(!do_video && do_audio)
	{
		rmonitor_meter_y -= DP(30);
		rmonitor_meter_h += DP(30);
	}

}

void Theme::get_recordgui_sizes(RecordGUI *gui, int w, int h)
{
}

void Theme::get_batchrender_sizes(BatchRenderGUI *gui,
	int w, 
	int h)
{
	batchrender_x1 = DP(10);
	batchrender_x2 = DP(300);
	batchrender_x3 = DP(400);
}

void Theme::get_plugindialog_sizes()
{
	int x = DP(10), y = DP(30);
	plugindialog_new_x = x;
	plugindialog_new_y = y;
	plugindialog_shared_x = mwindow->session->plugindialog_w / 3;
	plugindialog_shared_y = y;
	plugindialog_module_x = mwindow->session->plugindialog_w * 2 / 3;
	plugindialog_module_y = y;

	plugindialog_new_w = plugindialog_shared_x - plugindialog_new_x - DP(10);
	plugindialog_new_h = mwindow->session->plugindialog_h - DP(100);
	plugindialog_shared_w = plugindialog_module_x - plugindialog_shared_x - DP(10);
	plugindialog_shared_h = mwindow->session->plugindialog_h - DP(100);
	plugindialog_module_w = mwindow->session->plugindialog_w - plugindialog_module_x - DP(10);
	plugindialog_module_h = mwindow->session->plugindialog_h - DP(100);

	plugindialog_newattach_x = plugindialog_new_x + DP(20);
	plugindialog_newattach_y = plugindialog_new_y + plugindialog_new_h + DP(10);
	plugindialog_sharedattach_x = plugindialog_shared_x + DP(20);
	plugindialog_sharedattach_y = plugindialog_shared_y + plugindialog_shared_h + DP(10);
	plugindialog_moduleattach_x = plugindialog_module_x + DP(20);
	plugindialog_moduleattach_y = plugindialog_module_y + plugindialog_module_h + DP(10);
}

// void Theme::get_presetdialog_sizes(PresetsWindow *gui)
// {
// 	int x = window_border;
// 	int y = window_border + BC_Title::calculate_h(gui, "P") + widget_border;
// 
// 	presets_list_x = x;
// 	presets_list_y = y;
// 	presets_list_w = mwindow->session->presetdialog_w / 2;
// 	presets_list_h = mwindow->session->presetdialog_h - 
// 		BC_OKButton::calculate_h() - 
// 		presets_list_y -
// 		widget_border -
// 		window_border;
// 	presets_text_x = presets_list_x + presets_list_w + widget_border;
// 	presets_text_y = y;
// 	presets_text_w = mwindow->session->presetdialog_w - presets_text_x - window_border;
// 	y += BC_TextBox::calculate_h(gui, 
// 		MEDIUMFONT, 
// 		1, 
// 		1) + widget_border;
// 
// 	presets_delete_x = presets_text_x;
// 	presets_delete_y = y;
// 	y += BC_GenericButton::calculate_h() + widget_border;
// 
// 	presets_save_x = presets_text_x;
// 	presets_save_y = y;
// 	y += BC_GenericButton::calculate_h() + widget_border;
// 
// 	presets_apply_x = presets_text_x;
// 	presets_apply_y = y;
// 	y += BC_GenericButton::calculate_h();
// }

void Theme::get_keyframedialog_sizes(KeyFrameWindow *gui)
{
	int x = window_border;
	int y = window_border + 
		BC_Title::calculate_h(gui, "P", LARGEFONT) + 
		widget_border;

	presets_list_x = x;
	presets_list_y = y;
#ifdef EDIT_KEYFRAME
	presets_list_w = mwindow->session->keyframedialog_w / 2 - 
		widget_border - 
		window_border;
#else
	presets_list_w = mwindow->session->keyframedialog_w - 
		presets_list_x -
		window_border;
#endif
	presets_list_h = mwindow->session->keyframedialog_h - 
		BC_OKButton::calculate_h() - 
		presets_list_y -
		widget_border -
		widget_border -
		BC_Title::calculate_h(gui, "P") - 
		widget_border -
		BC_TextBox::calculate_h(gui, 
			MEDIUMFONT, 
			1, 
			1) -
		widget_border -
		(BC_GenericButton::calculate_h() + widget_border) * 3 - 
		window_border;
	y += presets_list_h + widget_border + widget_border + BC_Title::calculate_h(gui, "P");
	presets_text_x = x;
	presets_text_y = y;
	presets_text_w = presets_list_w;
	y += BC_TextBox::calculate_h(gui, 
		MEDIUMFONT, 
		1, 
		1) + widget_border;

	presets_delete_x = presets_text_x;
	presets_delete_y = y;
	y += BC_GenericButton::calculate_h() + widget_border;

	presets_save_x = presets_text_x;
	presets_save_y = y;
	y += BC_GenericButton::calculate_h() + widget_border;

	presets_apply_x = presets_text_x;
	presets_apply_y = y;
	y += BC_GenericButton::calculate_h();

#ifdef EDIT_KEYFRAME
	x = mwindow->session->keyframedialog_w / 2 + widget_border;
	y = window_border + 
		BC_Title::calculate_h(gui, "P", LARGEFONT) + 
		widget_border;

	keyframe_list_x = x;
	keyframe_list_y = y;
	keyframe_list_w = mwindow->session->keyframedialog_w / 2 - 
		widget_border - 
		window_border;
	keyframe_list_h = mwindow->session->keyframedialog_h - 
		keyframe_list_y -
		widget_border -
		widget_border -
		BC_Title::calculate_h(gui, "P") -
		widget_border - 
		BC_TextBox::calculate_h(gui,
			MEDIUMFONT,
			1,
			1) -
		widget_border - 
		BC_Title::calculate_h(gui, "P") -
		widget_border -
		BC_OKButton::calculate_h() - 
		window_border;
// 	keyframe_text_x = keyframe_list_x + keyframe_list_w + widget_border;
// 	keyframe_text_y = y;
// 	keyframe_text_w = mwindow->session->keyframedialog_w - keyframe_text_x - window_border;
// 	y += BC_TextBox::calculate_h(gui, 
// 		MEDIUMFONT, 
// 		1, 
// 		1) + widget_border;
// 

 	y += keyframe_list_h + BC_Title::calculate_h(gui, "P") + widget_border + widget_border;
	keyframe_value_x = keyframe_list_x;
	keyframe_value_y = y;
	keyframe_value_w = keyframe_list_w;
	y += BC_TextBox::calculate_h(gui, 
		MEDIUMFONT, 
		1, 
		1) + widget_border;

	keyframe_all_x = keyframe_value_x;
	keyframe_all_y = y;

#endif

}


void Theme::get_menueffect_sizes(int use_list)
{
	if(use_list)
	{
		menueffect_list_x = DP(10);
		menueffect_list_y = DP(10);
		menueffect_list_w = mwindow->session->menueffect_w - DP(420);
		menueffect_list_h = mwindow->session->menueffect_h - 
			menueffect_list_y -
			BC_OKButton::calculate_h() - DP(10);
	}
	else
	{
		menueffect_list_x = DP(0);
		menueffect_list_y = DP(10);
		menueffect_list_w = DP(0);
		menueffect_list_h = DP(0);
	}

	menueffect_file_x = menueffect_list_x + menueffect_list_w + DP(10);
	menueffect_file_y = DP(10);

	menueffect_tools_x = menueffect_file_x;
	menueffect_tools_y = menueffect_file_y + DP(20);
}

void Theme::get_preferences_sizes()
{
}

void Theme::draw_preferences_bg(PreferencesWindow *gui)
{
}

void Theme::get_new_sizes(NewWindow *gui)
{
}

void Theme::draw_new_bg(NewWindow *gui)
{
}

void Theme::draw_setformat_bg(SetFormatWindow *window)
{
}







