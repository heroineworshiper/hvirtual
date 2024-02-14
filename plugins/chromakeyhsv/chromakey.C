/*
 * CINELERRA
 * Copyright (C) 2012-2024 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
 * 
 */




// original chromakey HSV author:
// https://www.mail-archive.com/cinelerra@skolelinux.no/msg00379.html
// http://jcornet.free.fr/linux/chromakey.html

// He was trying to replicate the Avid SpectraMatte:
// https://resources.avid.com/SupportFiles/attach/Symphony_Effects_and_CC_Guide_v5.5.pdf
// page 691
// but the problem seems to be harder than he thought

// A rewrite got a slightly different spill light processing 
// but still not very useful:
// https://github.com/vanakala/cinelerra-cve/blob/master/plugins/chromakeyhsv/chromakey.C

// The only way to test it is to use the color swatch plugin.  
// Fix brightness to test the saturation wedge. 
// Fix saturation to test the brightness wedge.

// There are boundary artifacts caused by the color swatch showing a slice
// of a cube.
// If out slope > 0, constant 0% saturation or 0% brightness is all wedge.
// If out slope == 0, constant 0% saturation or constant 0% brightness has no wedge
// The general idea is if it's acting weird, switch between constant brightness 
// & constant saturation.

// TODO: integrated color swatch & alpha blur, but it takes a lot of space.
// spill threshold creates artifacts in YUV


#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "chromakey.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "playback3d.h"
#include "cicolors.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define WINDOW_W DP(400)
#define WINDOW_H DP(450)
#define SLIDER_W WINDOW_W - x - plugin->get_theme()->widget_border
#define MAX_SLOPE 100.0
#define MAX_VALUE 100.0

ChromaKeyConfig::ChromaKeyConfig ()
{
	red = 0.0;
	green = 1.0;
	blue = 0.0;

	min_brightness = 50.0;
	max_brightness = 100.0;
	tolerance = 15.0;
	saturation_start = 0.0;
	saturation_line = 50.0;

	in_slope = 2;
	out_slope = 2;
	alpha_offset = 0;

	spill_threshold = 0.0;
	spill_amount = 0.0;

	show_mask = 0;

}


void
ChromaKeyConfig::copy_from (ChromaKeyConfig & src)
{
	red = src.red;
	green = src.green;
	blue = src.blue;
	spill_threshold = src.spill_threshold;
	spill_amount = src.spill_amount;
	min_brightness = src.min_brightness;
	max_brightness = src.max_brightness;
	saturation_start = src.saturation_start;
	saturation_line = src.saturation_line;
	tolerance = src.tolerance;
	in_slope = src.in_slope;
	out_slope = src.out_slope;
	alpha_offset = src.alpha_offset;
	show_mask = src.show_mask;
}

int
ChromaKeyConfig::equivalent (ChromaKeyConfig & src)
{
	return (EQUIV (red, src.red) &&
		EQUIV (green, src.green) &&
		EQUIV (blue, src.blue) &&
		EQUIV (spill_threshold, src.spill_threshold) &&
		EQUIV (spill_amount, src.spill_amount) &&
		EQUIV (min_brightness, src.min_brightness) &&
		EQUIV (max_brightness, src.max_brightness) &&
		EQUIV (saturation_start, src.saturation_start) &&
		EQUIV (saturation_line, src.saturation_line) &&
		EQUIV (tolerance, src.tolerance) &&
		EQUIV (in_slope, src.in_slope) &&
		EQUIV (out_slope, src.out_slope) &&
		EQUIV (show_mask, src.show_mask) &&
		EQUIV (alpha_offset, src.alpha_offset));
}

void
ChromaKeyConfig::interpolate (ChromaKeyConfig & prev,
						ChromaKeyConfig & next,
						int64_t prev_frame,
						int64_t next_frame, int64_t current_frame)
{
	double next_scale =
		(double) (current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale =
		(double) (next_frame - current_frame) / (next_frame - prev_frame);

	this->red = prev.red * prev_scale + next.red * next_scale;
	this->green = prev.green * prev_scale + next.green * next_scale;
	this->blue = prev.blue * prev_scale + next.blue * next_scale;
	this->spill_threshold =
		prev.spill_threshold * prev_scale + next.spill_threshold * next_scale;
	this->spill_amount =
		prev.spill_amount * prev_scale + next.tolerance * next_scale;
	this->min_brightness =
		prev.min_brightness * prev_scale + next.min_brightness * next_scale;
	this->max_brightness =
		prev.max_brightness * prev_scale + next.max_brightness * next_scale;
	this->saturation_start =
		prev.saturation_start * prev_scale + next.saturation_start * next_scale;
	this->saturation_line =
		prev.saturation_line * prev_scale + next.saturation_line * next_scale;
	this->tolerance = prev.tolerance * prev_scale + next.tolerance * next_scale;
	this->in_slope = prev.in_slope * prev_scale + next.in_slope * next_scale;
	this->out_slope = prev.out_slope * prev_scale + next.out_slope * next_scale;
	this->alpha_offset =
		prev.alpha_offset * prev_scale + next.alpha_offset * next_scale;
	this->show_mask = next.show_mask;

}

int
ChromaKeyConfig::get_color ()
{
	int red = (int) (CLIP (this->red, 0, 1) * 0xff);
	int green = (int) (CLIP (this->green, 0, 1) * 0xff);
	int blue = (int) (CLIP (this->blue, 0, 1) * 0xff);
	return (red << 16) | (green << 8) | blue;
}



ChromaKeyWindow::ChromaKeyWindow (ChromaKeyHSV * plugin)
 : PluginClientWindow(plugin, 
		 WINDOW_W, 
		 WINDOW_H, 
		 WINDOW_W, 
		 WINDOW_H, 
		 0)
{
	this->plugin = plugin;
	color_thread = 0;
}

ChromaKeyWindow::~ChromaKeyWindow ()
{
	delete color_thread;
}

void
ChromaKeyWindow::create_objects ()
{
	int margin = client->get_theme()->widget_border;
	int y = margin, y1, x1 = 0, x2 = margin;
	int x = margin * 3;

	BC_Title *title;
	BC_Bar *bar;
	int ymargin = get_text_height(MEDIUMFONT) + margin;
	int ymargin2 = get_text_height(MEDIUMFONT) + margin * 2;

	add_subwindow (title = new BC_Title (x2, y, _("Color:")));

	add_subwindow (color = new ChromaKeyColor (plugin, this, x, y + DP(25)));

	add_subwindow (sample =
		 new BC_SubWindow (x + color->get_w () + margin, y, DP(100), DP(50)));
	y += sample->get_h () + margin;

	add_subwindow (use_colorpicker =
		 new ChromaKeyUseColorPicker (plugin, this, x, y));
	y += use_colorpicker->get_h() + margin;
	add_subwindow (show_mask = new ChromaKeyShowMask (plugin, x2, y));
	y += show_mask->get_h() + margin;

	add_subwindow(bar = new BC_Bar(x2, y, get_w() - x2 * 2));
	y += bar->get_h() + margin;
	y1 = y;
	add_subwindow (new BC_Title (x2, y, _("Key parameters:")));
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("Hue Tolerance:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("Min. Brightness:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("Max. Brightness:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("Saturation Start:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("Saturation Line:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin2;

	add_subwindow(bar = new BC_Bar(x2, y, get_w() - x2 * 2));
	y += bar->get_h() + margin;
	add_subwindow (title = new BC_Title (x2, y, _("Mask tweaking:")));
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("In Slope:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("Out Slope:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("Alpha Offset:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin2;



	add_subwindow(bar = new BC_Bar(x2, y, get_w() - x2 * 2));
	y += bar->get_h() + margin;
	add_subwindow (title = new BC_Title (x2, y, _("Spill light control:")));
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("Spill Threshold:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow (title = new BC_Title (x, y, _("Spill Compensation:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;


// create the sliders
	y = y1;
	y += ymargin;
	x1 += x;


	add_subwindow (tolerance = new ChromaKeySlider (plugin, x1, y, 0, MAX_VALUE, &plugin->config.tolerance));
	y += ymargin;
	add_subwindow (min_brightness = new ChromaKeySlider(plugin, x1, y, 0, MAX_VALUE, &plugin->config.min_brightness));
	y += ymargin;
	add_subwindow (max_brightness = new ChromaKeySlider (plugin, x1, y, 0, MAX_VALUE, &plugin->config.max_brightness));
	y += ymargin;
	add_subwindow (saturation_start = new ChromaKeySlider (plugin, x1, y, 0, MAX_VALUE, &plugin->config.saturation_start));
	y += ymargin;
	add_subwindow (saturation_line = new ChromaKeySlider (plugin, x1, y, 0, MAX_VALUE, &plugin->config.saturation_line));
	y += ymargin;

	y += bar->get_h() + margin;
	y += ymargin2;
	add_subwindow (in_slope = new ChromaKeySlider (plugin, x1, y, 0, MAX_SLOPE, &plugin->config.in_slope));
	y += ymargin;
	add_subwindow (out_slope = new ChromaKeySlider (plugin, x1, y, 0, MAX_SLOPE, &plugin->config.out_slope));
	y += ymargin;
	add_subwindow (alpha_offset = new ChromaKeySlider (plugin, x1, y, -MAX_VALUE, MAX_VALUE, &plugin->config.alpha_offset));
	 y += ymargin;

	y += bar->get_h() + margin;
	y += ymargin2;
	add_subwindow (spill_threshold = new ChromaKeySlider (plugin, x1, y, 0, MAX_VALUE, &plugin->config.spill_threshold));
	y += ymargin;
	add_subwindow (spill_amount = new ChromaKeySlider (plugin, x1, y, -MAX_VALUE, MAX_VALUE, &plugin->config.spill_amount));

	color_thread = new ChromaKeyColorThread (plugin, this);

	update_sample ();
	show_window ();
}

void
ChromaKeyWindow::update_sample ()
{
	sample->set_color (plugin->config.get_color ());
	sample->draw_box (0, 0, sample->get_w (), sample->get_h ());
	sample->set_color (BLACK);
	sample->draw_rectangle (0, 0, sample->get_w (), sample->get_h ());
	sample->flash ();
}



ChromaKeyColor::ChromaKeyColor (ChromaKeyHSV * plugin,
					ChromaKeyWindow * gui, int x, int y):
BC_GenericButton (x, y, _("Color..."))
{
	this->plugin = plugin;
	this->gui = gui;
}

int
ChromaKeyColor::handle_event ()
{
	gui->color_thread->start_window (plugin->config.get_color (), 0xff);
	return 1;
}



ChromaKeySlider::ChromaKeySlider(ChromaKeyHSV *plugin, 
    int x, 
    int y, 
    float min,
    float max,
    float *output)
 : BC_FSlider(x,
	y,
	0,
	SLIDER_W, 
	SLIDER_W, 
	min, 
    max, 
    *output)
{
    this->plugin = plugin;
    this->output = output;
    set_precision(0.01);
}

int ChromaKeySlider::handle_event()
{
	*output = get_value ();
	plugin->send_configure_change ();
	return 1;
}





ChromaKeyShowMask::ChromaKeyShowMask (ChromaKeyHSV * plugin, int x, int y):BC_CheckBox (x, y, plugin->config.show_mask,
			 _
			 ("Show Mask"))
{
	this->plugin = plugin;

}

int
ChromaKeyShowMask::handle_event ()
{
	plugin->config.show_mask = get_value ();
	plugin->send_configure_change ();
	return 1;
}

ChromaKeyUseColorPicker::ChromaKeyUseColorPicker (ChromaKeyHSV * plugin, ChromaKeyWindow * gui, int x, int y)
 : BC_GenericButton (x, y,
			_
			("Use color picker"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int
ChromaKeyUseColorPicker::handle_event ()
{
	plugin->config.red = plugin->get_red ();
	plugin->config.green = plugin->get_green ();
	plugin->config.blue = plugin->get_blue ();

	gui->update_sample ();


	plugin->send_configure_change ();
	return 1;
}





ChromaKeyColorThread::ChromaKeyColorThread (ChromaKeyHSV * plugin, ChromaKeyWindow * gui)
 : ColorThread (1, _("Inner color"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int
ChromaKeyColorThread::handle_new_color (int output, int alpha)
{
	plugin->config.red = (float) (output & 0xff0000) / 0xff0000;
	plugin->config.green = (float) (output & 0xff00) / 0xff00;
	plugin->config.blue = (float) (output & 0xff) / 0xff;

	get_gui()->unlock_window();
	gui->lock_window("ChromaKeyColorThread::handle_new_color");
	gui->update_sample ();
	gui->unlock_window();
	get_gui()->lock_window("ChromaKeyColorThread::handle_new_color");


	plugin->send_configure_change ();
	return 1;
}








ChromaKeyServer::ChromaKeyServer (ChromaKeyHSV * plugin)
// DEBUG
// : LoadServer (plugin->PluginClient::smp + 1, plugin->PluginClient::smp + 1)
 : LoadServer (1, 1)
{
	this->plugin = plugin;
}

void
ChromaKeyServer::init_packages ()
{
	for (int i = 0; i < get_total_packages (); i++)
		{
			ChromaKeyPackage *pkg = (ChromaKeyPackage *) get_package (i);
			pkg->y1 = plugin->input->get_h () * i / get_total_packages ();
			pkg->y2 = plugin->input->get_h () * (i + 1) / get_total_packages ();
		}

}
LoadClient *
ChromaKeyServer::new_client ()
{
	return new ChromaKeyUnit (plugin, this);
}

LoadPackage *
ChromaKeyServer::new_package ()
{
	return new ChromaKeyPackage;
}



ChromaKeyPackage::ChromaKeyPackage ():LoadPackage ()
{
}

ChromaKeyUnit::ChromaKeyUnit (ChromaKeyHSV * plugin, ChromaKeyServer * server):LoadClient
	(server)
{
	this->plugin = plugin;
}




#define ABS(a) ((a) < 0 ? -(a) : (a))

// Compute the same values in the opengl version
#define COMMON_VARIABLES \
	float red = plugin->config.red; \
	float green = plugin->config.green; \
	float blue = plugin->config.blue; \
 \
	float in_slope = plugin->config.in_slope / MAX_SLOPE; \
	float out_slope = plugin->config.out_slope / MAX_SLOPE; \
 \
/* Convert RGB key to HSV key */ \
	float hue_key, saturation_key, value_key, hue_offset = 0; \
	HSV::rgb_to_hsv(red,	\
		green, \
		blue, \
		hue_key, \
		saturation_key, \
		value_key); \
 \
/* hue range */ \
	float tolerance = (plugin->config.tolerance / MAX_VALUE) * 180; \
	float tolerance_in = tolerance - in_slope * 180; \
    tolerance_in = MAX(tolerance_in, 0); \
	float tolerance_out = tolerance + out_slope * 180; \
    tolerance_out = MIN(tolerance_out, 180); \
 \
/* distance of wedge point from center */ \
	float sat_distance = plugin->config.saturation_start / MAX_VALUE; \
/* XY shift of input color to get wedge point */ \
    float sat_x = -cos(TO_RAD(hue_key)) * sat_distance; \
    float sat_y = -sin(TO_RAD(hue_key)) * sat_distance; \
/* minimum saturation after wedge point */ \
	float min_s = plugin->config.saturation_line / MAX_VALUE; \
	float min_s_in = min_s + in_slope; \
	float min_s_out = min_s - out_slope; \
 \
	float min_v = plugin->config.min_brightness / MAX_VALUE; \
	float min_v_in = min_v + in_slope; \
	float min_v_out = min_v - out_slope; \
/* ignore min_brightness 0 */ \
    if(plugin->config.min_brightness == 0) \
        min_v_in = 0; \
 \
	float max_v = plugin->config.max_brightness / MAX_VALUE; \
	float max_v_in = max_v - in_slope; \
	float max_v_out = max_v + out_slope; \
/* handle wedge boundaries crossing over */ \
    if(max_v_in < min_v_in) max_v_in = min_v_in = (max_v_in + min_v_in) / 2; \
/* ignore max_brightness 100% */ \
    if(plugin->config.max_brightness == MAX_VALUE) \
        max_v_in = 1.0; \
 \
/* maximum h_diff of spill gradient */ \
	float spill_threshold = plugin->config.spill_threshold * 180 / MAX_VALUE; \
/* minimum h_diff of spill gradient */ \
	float spill_amount = 0; \
/* Divide S if spill_amount < 0 && multiply S if spill_amount > 0 */ \
    int scale_spill = 0; \
    if(plugin->config.spill_amount >= 0) \
        spill_amount = spill_threshold * plugin->config.spill_amount / MAX_VALUE; \
    else \
    { \
        scale_spill = 1; \
        spill_amount = (plugin->config.spill_amount - (-MAX_VALUE)) / MAX_VALUE; \
    } \
 \
	float alpha_offset = plugin->config.alpha_offset / MAX_VALUE;

template <typename component_type> 
void ChromaKeyUnit::process_chromakey(int components, 
	component_type max, 
	bool use_yuv, 
	ChromaKeyPackage *pkg) 
{ 	
	COMMON_VARIABLES

	int w = plugin->input->get_w();

// printf("ChromaKeyUnit::process_chromakey %d hue_key=%f sat_x=%f sat_y=%f\n", 
// __LINE__, 
// hue_key,
// sat_x,
// sat_y);
// printf("ChromaKeyUnit::process_chromakey %d tolerance_in=%f tolerance=%f tolerance_out=%f\n", 
// __LINE__, 
// tolerance_in,
// tolerance,
// tolerance_out);
// printf("ChromaKeyUnit::process_chromakey %d min_s_in=%f min_s=%f min_s_out=%f\n", 
// __LINE__, 
// min_s_in,
// min_s,
// min_s_out);
// printf("ChromaKeyUnit::process_chromakey %d min_v_in=%f min_v=%f min_v_out=%f\n", 
// __LINE__, 
// min_v_in,
// min_v,
// min_v_out);
// printf("ChromaKeyUnit::process_chromakey %d max_v_in=%f max_v=%f max_v_out=%f\n", 
// __LINE__, 
// max_v_in,
// max_v,
// max_v_out);

	for (int i = pkg->y1; i < pkg->y2; i++)
	{
		component_type *row = (component_type *) plugin->input->get_rows ()[i];

		for (int j = 0; j < w; j++)
		{
			float a = 1;

			float r = (float) row[0] / max;
			float g = (float) row[1] / max;
			float b = (float) row[2] / max;

// the input color
			float h, s, v;

// alpha contribution of each component of the HSV space
			float ah = 1, as = 1, av = 1;

			if (use_yuv)
			{
/* Convert pixel to RGB float */
				float y = r;
				float u = g;
				float v = b;
				YUV::yuv_to_rgb_f (r, g, b, y, u - 0.5, v - 0.5);
			}

			HSV::rgb_to_hsv (r, g, b, h, s, v);

// shift the color in XY to shift the wedge point
            float h_shifted, s_shifted;
            if(!EQUIV(plugin->config.saturation_start, 0))
            {
                float h_rad = TO_RAD(h);
                float x = cos(h_rad) * s;
                float y = sin(h_rad) * s;
                x += sat_x;
                y += sat_y;
                h_shifted = TO_DEG(atan2(y, x));
                s_shifted = hypot(x, y);
            }
            else
            {
                h_shifted = h;
                s_shifted = s;
            }



/* Get the difference between the current hue & the hue key */
			float h_diff = h_shifted - hue_key;
            if(h_diff < -180) h_diff += 360;
            else
            if(h_diff > 180) h_diff -= 360;
            h_diff = ABS(h_diff);

// alpha contribution from hue difference
// outside wedge < tolerance_out < tolerance_in < inside wedge < tolerance_in < tolerance_out < outside wedge
			if (tolerance_out > 0)
            {
// completely inside the wedge
			    if (h_diff < tolerance_in)
				    ah = 0;
                else
// between the outer & inner slope
 			    if(h_diff < tolerance_out)
				    ah = (h_diff - tolerance_in) / (tolerance_out - tolerance_in);
                if(ah > 1) ah = 1;
            }

// alpha contribution from saturation
// outside wedge < min_s_out < min_s_in < inside wedge
            if(s_shifted > min_s_out)
            {
// saturation with offset applied
// completely inside the wedge
                if(s_shifted > min_s_in)
                    as = 0;
// inside the gradient
                if(s_shifted >= min_s_out)
				    as = (min_s_in - s_shifted) / (min_s_in - min_s_out);
            }


// alpha contribution from brightness range
// brightness range is defined by 4 in/out variables
// outside wedge < min_v_out < min_v_in < inside wedge < max_v_in < max_v_out < outside wedge
            if(v > min_v_out)
            {
                if(v < min_v_in)
                    av = (min_v_in - v) / (min_v_in - min_v_out);
                else
                if(v <= max_v_in)
                    av = 0;
                else
                if(v <= max_v_out)
                    av = (v - max_v_in) / (max_v_out - max_v_in);
            }

// combine the alpha contribution of every component into a single alpha
            a = MAX(as, ah);
            a = MAX(a, av);

// Spill light processing
// desaturate a wedge around the hue key
// hue_key/s=0 < spill_amount < spill_threshold < no spill/s=1

// It's possible that the spill light control needs to factor in
// saturation & value instead of just hue.  As written, it just created a
// wedge of desaturation based on hue.

		    if (h_diff < spill_threshold)
		    {
                float s_scale = 0;
                if(h_diff > spill_threshold)
                    s_scale = 1;
                else
                if(!scale_spill && h_diff > spill_amount)
                    s_scale = (h_diff - spill_amount) / (spill_threshold - spill_amount);
                else
                if(scale_spill)
                    s_scale = h_diff / spill_threshold;

                if(scale_spill)
                {
                    float s2 = s * s_scale;
                    s = s * (1.0 - spill_amount) + s2 * spill_amount;
                }
                else
                    s *= s_scale;

			    HSV::hsv_to_rgb (r, g, b, h, s, v);

// store new color components
			    if (use_yuv)
		        {
			        float y;
			        float u;
			        float v;
			        YUV::rgb_to_yuv_f (r, g, b, y, u, v);
                    u += 0.5;
                    v += 0.5;
				    CLAMP (y, 0, 1.0);
				    CLAMP (u, 0, 1.0);
				    CLAMP (v, 0, 1.0);
			        row[0] = (component_type) ((float) y * max);
			        row[1] = (component_type) ((float) u * max);
			        row[2] = (component_type) ((float) v * max);
		        }
			    else
		        {
				    CLAMP (r, 0, 1.0);
				    CLAMP (g, 0, 1.0);
				    CLAMP (b, 0, 1.0);
			        row[0] = (component_type) ((float) r * max);
			        row[1] = (component_type) ((float) g * max);
			        row[2] = (component_type) ((float) b * max);
		        }
		    }

		    a += alpha_offset;
		    CLAMP (a, 0.0, 1.0);

		    if (plugin->config.show_mask)
		    {

			    if (use_yuv)
		        {
			        row[0] = (component_type) ((float) a * max);
			        row[1] = (component_type) ((float) max / 2);
			        row[2] = (component_type) ((float) max / 2);
		        }
			    else
		        {
		            row[0] = (component_type) ((float) a * max);
		            row[1] = (component_type) ((float) a * max);
		            row[2] = (component_type) ((float) a * max);
		        }
		    }

		    /* Multiply alpha and put back in frame */
		    if (components == 4)
		    {
			    row[3] = MIN ((component_type) (a * max), row[3]);
		    }
		    else if (use_yuv)
		    {
			    row[0] = (component_type) ((float) a * row[0]);
			    row[1] = (component_type) ((float) a * (row[1] - (max / 2 + 1)) +
					    max / 2 + 1);
			    row[2] = (component_type) ((float) a * (row[2] - (max / 2 + 1)) +
					    max / 2 + 1);
		    }
		    else
		    {
			    row[0] = (component_type) ((float) a * row[0]);
			    row[1] = (component_type) ((float) a * row[1]);
			    row[2] = (component_type) ((float) a * row[2]);
		    }

		    row += components;
	    }
	}
}




void ChromaKeyUnit::process_package(LoadPackage *package)
{
	ChromaKeyPackage *pkg = (ChromaKeyPackage*)package;


	switch(plugin->input->get_color_model())
	{
		case BC_RGB_FLOAT:
			process_chromakey<float> (3, 1.0, 0, pkg);
			break;
		case BC_RGBA_FLOAT:
			process_chromakey<float> ( 4, 1.0, 0, pkg);
			break;
		case BC_RGB888:
			process_chromakey<unsigned char> ( 3, 0xff, 0, pkg);
			break;
		case BC_RGBA8888:
			process_chromakey<unsigned char> ( 4, 0xff, 0, pkg);
			break;
		case BC_YUV888:
			process_chromakey<unsigned char> ( 3, 0xff, 1, pkg);
			break;
		case BC_YUVA8888:
			process_chromakey<unsigned char> ( 4, 0xff, 1, pkg);
			break;
	}

}





REGISTER_PLUGIN(ChromaKeyHSV)



ChromaKeyHSV::ChromaKeyHSV(PluginServer *server)
 : PluginVClient(server)
{
	
	engine = 0;
}

ChromaKeyHSV::~ChromaKeyHSV()
{
	
	if(engine) delete engine;
}


int ChromaKeyHSV::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	load_configuration();
	this->input = frame;
	this->output = frame;


	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		get_use_opengl());
	if(get_use_opengl()) return run_opengl();


	if(!engine) engine = new ChromaKeyServer(this);
	engine->process_packages();

	return 0;
}

const char* ChromaKeyHSV::plugin_title() { return N_("Chroma key (HSV)"); }
int ChromaKeyHSV::is_realtime() { return 1; }

NEW_PICON_MACRO(ChromaKeyHSV)

LOAD_CONFIGURATION_MACRO(ChromaKeyHSV, ChromaKeyConfig)


void
ChromaKeyHSV::save_data (KeyFrame * keyframe)
{
	FileXML output;
	output.set_shared_string (keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title ("CHROMAKEY_HSV");
	output.tag.set_property ("RED", config.red);
	output.tag.set_property ("GREEN", config.green);
	output.tag.set_property ("BLUE", config.blue);
	output.tag.set_property ("MIN_BRIGHTNESS", config.min_brightness);
	output.tag.set_property ("MAX_BRIGHTNESS", config.max_brightness);
	output.tag.set_property ("SATURATION_START", config.saturation_start);
	output.tag.set_property ("SATURATION_LINE", config.saturation_line);
	output.tag.set_property ("TOLERANCE", config.tolerance);
	output.tag.set_property ("IN_SLOPE", config.in_slope);
	output.tag.set_property ("OUT_SLOPE", config.out_slope);
	output.tag.set_property ("ALPHA_OFFSET", config.alpha_offset);
	output.tag.set_property ("SPILL_THRESHOLD", config.spill_threshold);
	output.tag.set_property ("SPILL_AMOUNT", config.spill_amount);
	output.tag.set_property ("SHOW_MASK", config.show_mask);
	output.append_tag ();
	output.terminate_string ();
}

void
ChromaKeyHSV::read_data (KeyFrame * keyframe)
{
	FileXML input;

	input.set_shared_string (keyframe->get_data(), strlen (keyframe->get_data()));

	while (!input.read_tag ())
		{
			if (input.tag.title_is ("CHROMAKEY_HSV"))
	{
		config.red = input.tag.get_property ("RED", config.red);
		config.green = input.tag.get_property ("GREEN", config.green);
		config.blue = input.tag.get_property ("BLUE", config.blue);
		config.min_brightness =
			input.tag.get_property ("MIN_BRIGHTNESS", config.min_brightness);
		config.max_brightness =
			input.tag.get_property ("MAX_BRIGHTNESS", config.max_brightness);
		config.saturation_start =
			input.tag.get_property ("SATURATION_START", config.saturation_start);
		config.saturation_line =
			input.tag.get_property ("SATURATION_LINE", config.saturation_line);
		config.tolerance =
			input.tag.get_property ("TOLERANCE", config.tolerance);
		config.in_slope =
			input.tag.get_property ("IN_SLOPE", config.in_slope);
		config.out_slope =
			input.tag.get_property ("OUT_SLOPE", config.out_slope);
		config.alpha_offset =
			input.tag.get_property ("ALPHA_OFFSET", config.alpha_offset);
		config.spill_threshold =
			input.tag.get_property ("SPILL_THRESHOLD",
						config.spill_threshold);
		config.spill_amount =
			input.tag.get_property ("SPILL_AMOUNT", config.spill_amount);
		config.show_mask =
			input.tag.get_property ("SHOW_MASK", config.show_mask);
	}
		}
}


NEW_WINDOW_MACRO(ChromaKeyHSV, ChromaKeyWindow)

void ChromaKeyHSV::update_gui ()
{
	if (thread)
		{
			load_configuration ();
			thread->window->lock_window ();
			((ChromaKeyWindow*)thread->window)->min_brightness->update (config.min_brightness);
			((ChromaKeyWindow*)thread->window)->max_brightness->update (config.max_brightness);
			((ChromaKeyWindow*)thread->window)->saturation_start->update (config.saturation_start);
			((ChromaKeyWindow*)thread->window)->saturation_line->update (config.saturation_line);
			((ChromaKeyWindow*)thread->window)->tolerance->update (config.tolerance);
			((ChromaKeyWindow*)thread->window)->in_slope->update (config.in_slope);
			((ChromaKeyWindow*)thread->window)->out_slope->update (config.out_slope);
			((ChromaKeyWindow*)thread->window)->alpha_offset->update (config.alpha_offset);
			((ChromaKeyWindow*)thread->window)->spill_threshold->update (config.spill_threshold);
			((ChromaKeyWindow*)thread->window)->spill_amount->update (config.spill_amount);
			((ChromaKeyWindow*)thread->window)->show_mask->update (config.show_mask);
			((ChromaKeyWindow*)thread->window)->update_sample ();
			thread->window->unlock_window ();
		}
}




int ChromaKeyHSV::handle_opengl()
{
#ifdef HAVE_GL
// For macro
	ChromaKeyHSV *plugin = this;
	COMMON_VARIABLES

	const char *yuv_shader = 
		"const vec3 black = vec3(0.0, 0.5, 0.5);\n"
		"\n"
		"vec4 yuv_to_rgb(vec4 color)\n"
		"{\n"
			YUV_TO_RGB_FRAG("color")
		"	return color;\n"
		"}\n"
		"\n"
		"vec4 rgb_to_yuv(vec4 color)\n"
		"{\n"
			RGB_TO_YUV_FRAG("color")
		"	return color;\n"
		"}\n";

	const char *rgb_shader = 
		"const vec3 black = vec3(0.0, 0.0, 0.0);\n"
		"\n"
		"vec4 yuv_to_rgb(vec4 color)\n"
		"{\n"
		"	return color;\n"
		"}\n"
		"vec4 rgb_to_yuv(vec4 color)\n"
		"{\n"
		"	return color;\n"
		"}\n";

	const char *hsv_shader = 
		"vec4 rgb_to_hsv(vec4 color)\n"
		"{\n"
			RGB_TO_HSV_FRAG("color")
		"	return color;\n"
		"}\n"
		"\n"
		"vec4 hsv_to_rgb(vec4 color)\n"
		"{\n"
			HSV_TO_RGB_FRAG("color")
		"	return color;\n"
		"}\n"
		"\n";

	const char *show_rgbmask_shader = 
		"vec4 show_mask(vec4 color, vec4 color2)\n"
		"{\n"
		"	return vec4(1.0, 1.0, 1.0, min(color.a, color2.a));"
		"}\n";

	const char *show_yuvmask_shader = 
		"vec4 show_mask(vec4 color, vec4 color2)\n"
		"{\n"
		"	return vec4(1.0, 0.5, 0.5, min(color.a, color2.a));"
		"}\n";

	const char *nomask_shader = 
		"vec4 show_mask(vec4 color, vec4 color2)\n"
		"{\n"
		"	return vec4(color.rgb, min(color.a, color2.a));"
		"}\n";

	extern unsigned char _binary_chromakey_sl_start[];
	static char *shader = (char*)_binary_chromakey_sl_start;

	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();

	const char* shader_stack[] = { 0, 0, 0, 0, 0 };

	switch(get_output()->get_color_model())
	{
		case BC_YUV888:
		case BC_YUVA8888:
			shader_stack[0] = yuv_shader;
			shader_stack[1] = hsv_shader;
			if(config.show_mask) 
				shader_stack[2] = show_yuvmask_shader;
			else
				shader_stack[2] = nomask_shader;
			shader_stack[3] = shader;
			break;

		default:
			shader_stack[0] = rgb_shader;
			shader_stack[1] = hsv_shader;
			if(config.show_mask) 
				shader_stack[2] = show_rgbmask_shader;
			else
				shader_stack[2] = nomask_shader;
			shader_stack[3] = shader;
			break;
	}


	unsigned int frag = VFrame::make_shader(0, 
		shader_stack[0], 
		shader_stack[1], 
		shader_stack[2], 
		shader_stack[3], 
		0);

	if(frag)
	{
		glUseProgram(frag);
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);
		glUniform1f(glGetUniformLocation(frag, "red"), red);
		glUniform1f(glGetUniformLocation(frag, "green"), green);
		glUniform1f(glGetUniformLocation(frag, "blue"), blue);
		glUniform1f(glGetUniformLocation(frag, "in_slope"), in_slope);
		glUniform1f(glGetUniformLocation(frag, "out_slope"), out_slope);
		glUniform1f(glGetUniformLocation(frag, "tolerance"), tolerance);
		glUniform1f(glGetUniformLocation(frag, "tolerance_in"), tolerance_in);
		glUniform1f(glGetUniformLocation(frag, "tolerance_out"), tolerance_out);
		glUniform1f(glGetUniformLocation(frag, "sat_x"), sat_x);
		glUniform1f(glGetUniformLocation(frag, "sat_y"), sat_y);
		glUniform1f(glGetUniformLocation(frag, "min_s"), min_s);
		glUniform1f(glGetUniformLocation(frag, "min_s_in"), min_s_in);
		glUniform1f(glGetUniformLocation(frag, "min_s_out"), min_s_out);
		glUniform1f(glGetUniformLocation(frag, "min_v"), min_v);
		glUniform1f(glGetUniformLocation(frag, "min_v_in"), min_v_in);
		glUniform1f(glGetUniformLocation(frag, "min_v_out"), min_v_out);
		glUniform1f(glGetUniformLocation(frag, "max_v"), max_v);
		glUniform1f(glGetUniformLocation(frag, "max_v_in"), max_v_in);
		glUniform1f(glGetUniformLocation(frag, "max_v_out"), max_v_out);
		glUniform1f(glGetUniformLocation(frag, "spill_threshold"), spill_threshold);
		glUniform1f(glGetUniformLocation(frag, "spill_amount"), spill_amount);
		glUniform1i(glGetUniformLocation(frag, "scale_spill"), scale_spill);
		glUniform1f(glGetUniformLocation(frag, "alpha_offset"), alpha_offset);
		glUniform1f(glGetUniformLocation(frag, "hue_key"), hue_key);
		glUniform1f(glGetUniformLocation(frag, "saturation_key"), saturation_key);
		glUniform1f(glGetUniformLocation(frag, "value_key"), value_key);
	}


	get_output()->bind_texture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if(cmodel_components(get_output()->get_color_model()) == 3)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		get_output()->clear_pbuffer();
	}
	get_output()->draw_texture();

	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDisable(GL_BLEND);


#endif
		return 0;
}

