
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "bcbutton.h"
#include "bcmeter.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcwindow.h"
#include "colors.h"
#include "fonts.h"
#include "vframe.h"
#include <string.h>

// Which source image to replicate
#define METER_NORMAL 0
#define METER_GREEN 1
#define METER_RED 2
#define METER_YELLOW 3
#define METER_WHITE 4
#define METER_OVER 5

// Region of source image to replicate
#define METER_LEFT 0
#define METER_MID 1
#define METER_RIGHT 3


BC_Meter::BC_Meter(int x, 
	int y, 
	int orientation, 
	int pixels, 
	int min, 
	int max,
	int mode, 
	int use_titles,
	int span,
    int is_gain_change)
 : BC_SubWindow(x, y, -1, -1)
{
    this->is_gain_change = is_gain_change;
	this->over_delay = 150;
	this->peak_delay = 15;
	this->use_titles = use_titles;
	this->min = min;
	this->max = max;
	this->mode = mode;
	this->orientation = orientation;
	this->pixels = pixels;
	this->span = span;
//printf("BC_Meter::draw_face %d w=%d pixels=%d\n", __LINE__, w, pixels);
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) images[i] = 0;
	db_titles.set_array_delete();
}

BC_Meter::~BC_Meter()
{
	db_titles.remove_all_objects();
	title_pixels.remove_all();
	tick_w.remove_all();
	tick_pixels.remove_all();
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) delete images[i];
}

int BC_Meter::get_title_w()
{
	return get_resources()->meter_title_w;
}

int BC_Meter::get_meter_w()
{
	return get_resources()->ymeter_images[0]->get_w();
}


int BC_Meter::set_delays(int over_delay, int peak_delay)
{
	this->over_delay = over_delay;
	this->peak_delay = peak_delay;
	return 0;
}

int BC_Meter::initialize()
{
	peak_timer = 0;
	level_pixel = peak_pixel = 0;
	over_timer = 0;
	over_count = 0;
    
    if(is_gain_change)
    {
        peak = level = 0;
    }
    else
    {
    	peak = level = -100;
    }

	if(orientation == METER_VERT)
	{
		set_images(get_resources()->ymeter_images);
		h = pixels;
		if(span < 0) 
		{
			w = images[0]->get_w();
			if(use_titles) w += get_title_w();
		}
		else 
			w = span;
	}
	else
	{
		set_images(get_resources()->xmeter_images);
		h = images[0]->get_h();
		w = pixels;
		if(use_titles) h += get_title_w();
	}

// calibrate the db titles
	get_divisions();

	BC_SubWindow::initialize();
	draw_titles(0);
	draw_face(0);
	show_window(0);
	return 0;
}

void BC_Meter::set_images(VFrame **data)
{
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) delete images[i];
	for(int i = 0; i < TOTAL_METER_IMAGES; i++) 
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
}

int BC_Meter::reposition_window(int x, int y, int span, int pixels)
{
	if(pixels < 0) pixels = this->pixels;
	this->span = span;
	this->pixels = pixels;
	if(orientation == METER_VERT)
		BC_SubWindow::reposition_window(x, 
			y, 
			this->span < 0 ? w : span, 
			pixels);
	else
		BC_SubWindow::reposition_window(x, y, pixels, get_h());

//printf("BC_Meter::reposition_window 1 %d %d %d %d\n", x, y, w, h);
	get_divisions();

//set_color(WHITE);
//draw_box(0, 0, w, h);
//flash();	
//return 0;
	draw_titles(0);
	draw_face(0);
	return 0;
}

int BC_Meter::reset()
{
	level = min;
	peak = min;
	level_pixel = peak_pixel = 0;
	peak_timer = 0;
	over_timer = 0;
	over_count = 0;
	draw_face(1);
	return 0;
}

int BC_Meter::button_press_event()
{
	if(cursor_inside() && is_event_win())
	{
		reset_over();
		return 1;
	}
	return 0;
}


int BC_Meter::reset_over()
{
	over_timer = 0;
	return 0;
}

int BC_Meter::change_format(int mode, int min, int max)
{
	this->mode = mode;
	this->min = min;
	this->max = max;
	reposition_window(get_x(), get_y(), w, pixels);
	return 0;
}

int BC_Meter::level_to_pixel(float level)
{
	int result;
	if(mode == METER_DB)
	{
		result = (int)(pixels * 
			(level - min) / 
			(max - min));
		if(level <= min) result = 0;
	}
	else
	{
// Not implemented anymore
		result = 0;
	}
	
	return result;
}


void BC_Meter::get_divisions()
{
	int i;
	int j, j_step;
	int division, division_step;
	char string[BCTEXTLEN];
	char *new_string;


	db_titles.remove_all_objects();
	title_pixels.remove_all();
	tick_pixels.remove_all();
	tick_w.remove_all();

	low_division = 0;
	medium_division = 0;
	high_division = pixels;

	int current_pixel;
// Create tick marks and titles in one pass
	for(int current = min; current <= max; current++)
	{
		if(orientation == METER_VERT)
		{
// Create tick mark
			current_pixel = (pixels - METER_MARGIN * 2 - 2) * 
				(current - min) /
				(max - min) + 2;
			tick_pixels.append(current_pixel);

// Create titles in selected positions
			if(current == min || 
				current == max ||
				current == 0 ||
				(current - min > 4 && max - current > 4 && !(current % 5)))
			{
				int title_pixel = (pixels - METER_MARGIN * 2) * 
					(current - min) /
					(max - min);
				sprintf(string, "%d", (int)labs(current));
				new_string = new char[strlen(string) + 1];
				strcpy(new_string, string);
				db_titles.append(new_string);
				title_pixels.append(title_pixel);
				tick_w.append(TICK_W1);
			}
			else
			{
				tick_w.append(TICK_W2);
			}
		}
		else
		{
			current_pixel = (pixels - METER_MARGIN * 2) *
				(current - min) /
				(max - min);
			tick_pixels.append(current_pixel);
			tick_w.append(TICK_W1);
// Titles not supported for horizontal
		}

// Create color divisions
		if(current == -20)
		{
			low_division = current_pixel;
		}
		else
		if(current == -5)
		{
			medium_division = current_pixel;
		}
		else
		if(current == 0)
		{
			high_division = current_pixel;
		}
	}
// if(orientation == METER_VERT)
// printf("BC_Meter::get_divisions %d %d %d %d\n",
// low_division, medium_division, high_division, pixels);
}

void BC_Meter::draw_titles(int flush)
{
	if(!use_titles) return;
    int tick_fudge = DP(1);

	set_font(get_resources()->meter_font);

	if(orientation == METER_HORIZ)
	{
		draw_top_background(parent_window, 0, 0, get_w(), get_title_w());

		for(int i = 0; i < db_titles.total; i++)
		{
			draw_text(0, title_pixels.values[i], db_titles.values[i]);
		}

		flash(0, 0, get_w(), get_title_w(), flush);
	}
	else
	if(orientation == METER_VERT)
	{
		draw_top_background(parent_window, 0, 0, get_title_w(), get_h());

// Titles
		for(int i = 0; i < db_titles.total; i++)
		{
			int title_y = pixels - 
				title_pixels.values[i];
			if(i == 0) 
				title_y -= get_text_descent(get_resources()->meter_font);
			else
			if(i == db_titles.total - 1)
				title_y += get_text_ascent(get_resources()->meter_font);
			else
				title_y += get_text_ascent(get_resources()->meter_font) / 2;
            int title_x = get_title_w() - 
                TICK_W1 - 
                tick_fudge -
                get_text_width(get_resources()->meter_font, db_titles.values[i]);

			set_color(get_resources()->meter_font_color);
			draw_text(title_x, 
				title_y,
				db_titles.values[i]);
		}

		for(int i = 0; i < tick_pixels.total; i++)
		{
// Tick marks
			int tick_y = pixels - tick_pixels.values[i] - METER_MARGIN;
			set_color(get_resources()->meter_font_color);
			draw_line(get_title_w() - tick_w.get(i) - tick_fudge, 
                tick_y, 
                get_title_w() - tick_fudge, 
                tick_y);
			
			if(get_resources()->meter_3d)
			{
				set_color(BLACK);
				draw_line(get_title_w() - tick_w.get(i), 
                    tick_y + 1, 
                    get_title_w(), 
                    tick_y + 1);
			}
		}

		flash(0, 0, get_title_w(), get_h(), flush);
	}
}

int BC_Meter::region_pixel(int region)
{
	VFrame **reference_images = get_resources()->xmeter_images;
	int result = 0;

	if(region == METER_RIGHT) 
		result = region * reference_images[0]->get_w() / 4;
	else
		result = region * reference_images[0]->get_w() / 4;

	return result;
}

int BC_Meter::region_pixels(int region)
{
	int x1;
	int x2;
	int result;
	VFrame **reference_images = get_resources()->xmeter_images;
	
	x1 = region * reference_images[0]->get_w() / 4;
	x2 = (region + 1) * reference_images[0]->get_w() / 4;
	if(region == METER_MID) 
		result = (x2 - x1) * 2;
	else 
		result = x2 - x1;
	return result;
}

void BC_Meter::draw_face(int flush)
{
	VFrame **reference_images = get_resources()->xmeter_images;
	int level_pixel = level_to_pixel(level);
	int peak_pixel2 = level_to_pixel(peak);
	int peak_pixel1 = peak_pixel2 - 2;
	int left_pixel = region_pixel(METER_MID);
	int right_pixel = pixels - region_pixels(METER_RIGHT);
	int pixel = 0;
	int image_number = 0, region = 0;
	int in_span, in_start;
	int x = use_titles ? get_title_w() : 0;
	int w = use_titles ? (this->w - get_title_w()) : this->w;

	draw_top_background(parent_window, x, 0, w, h);

// printf("BC_Meter::draw_face %d span=%d this->w=%d get_title_w()=%d %d %d\n", 
// __LINE__, 
// span, 
// this->w,
// get_title_w(),
// w, 
// h);

    if(is_gain_change)
    {
        int in_h = images[0]->get_h();
        int in_third = in_h / 3;
        int in_third3 = in_h - in_third * 2;

// printf("BC_Meter::draw_face %d level=%f level_pixel=%d high_division=%d\n", 
// __LINE__, 
// level,
// level_pixel,
// high_division);


// fudge a line when no gain change
        if(level_pixel == high_division)
        {
            level_pixel += 1;
        }

        while(pixel < pixels)
        {
// Select image to draw & extents
            if(level_pixel < high_division)
            {
// always vertical
                if(pixel < level_pixel)
                {
                    image_number = METER_NORMAL;
                    in_span = level_pixel - pixel;
                }
                else
                if(pixel < high_division)
                {
                    image_number = METER_RED;
                    in_span = high_division - pixel;
                }
                else
                {
                    image_number = METER_NORMAL;
                    in_span = pixels - pixel;
                }
            }
            else
            {
// determine pixel range & image to draw
                if(pixel < high_division)
                {
                    image_number = METER_NORMAL;
                    in_span = high_division - pixel;
                }
                else
                if(pixel < level_pixel)
                {
                    image_number = METER_GREEN;
                    in_span = level_pixel - pixel;
                }
                else
                {
                    image_number = METER_NORMAL;
                    in_span = pixels - pixel;
                }
            }

// determine starting point in source to draw
// draw starting section
            if(pixel == 0)
            {
                in_start = 0;
            }
            else
// draw middle section
            if(pixels - pixel > in_third3)
            {
                in_start = in_third;
            }
            else
// draw last section
            {
                in_start = in_third * 2;
            }

// clamp the region to the source dimensions
            if(in_start < in_third * 2)
            {
                if(in_span > in_third)
                {
                    in_span = in_third;
                }
            }
            else
// last segment
            if(pixels - pixel < in_third3)
            {
                in_span = pixels - pixel;
                in_start = in_h - in_span;
            }

// printf("BC_Meter::draw_face %d dst_y=%d src_y=%d pixels=%d pixel=%d in_h=%d in_start=%d in_span=%d in_third=%d in_third3=%d\n", 
// __LINE__, 
// get_h() - pixel - in_span,
// in_h - in_start - in_span,
// pixels,
// pixel,
// in_h,
// in_start,
// in_span,
// in_third,
// in_third3);
            draw_pixmap(images[image_number],
				x,
				get_h() - pixel - in_span,
				get_w(),
				in_span + 1,
				0,
				in_h - in_start - in_span);
            pixel += in_span;
        }
    }
    else
    {
	    while(pixel < pixels)
	    {
// Select image to draw
		    if(pixel < level_pixel ||
			    (pixel >= peak_pixel1 && pixel < peak_pixel2))
		    {
			    if(pixel < low_division)
				    image_number = METER_GREEN;
			    else
			    if(pixel < medium_division)
				    image_number = METER_YELLOW;
			    else
			    if(pixel < high_division)
				    image_number = METER_RED;
			    else
				    image_number = METER_WHITE;
		    }
		    else
		    {
			    image_number = METER_NORMAL;
		    }

// Select region of image to duplicate
		    if(pixel < left_pixel)
		    {
			    region = METER_LEFT;
			    in_start = pixel + region_pixel(region);
			    in_span = region_pixels(region) - (in_start - region_pixel(region));
		    }
		    else
		    if(pixel < right_pixel)
		    {
			    region = METER_MID;
			    in_start = region_pixel(region);
			    in_span = region_pixels(region);
		    }
		    else
		    {
			    region = METER_RIGHT;
			    in_start = (pixel - right_pixel) + region_pixel(region);
			    in_span = region_pixels(region) - (in_start - region_pixel(region));;
		    }

    //printf("BC_Meter::draw_face region %d pixel %d pixels %d in_start %d in_span %d\n", region, pixel, pixels, in_start, in_span);
		    if(in_span > 0)
		    {
    // Clip length to peaks
			    if(pixel < level_pixel && pixel + in_span > level_pixel)
				    in_span = level_pixel - pixel;
			    else
			    if(pixel < peak_pixel1 && pixel + in_span > peak_pixel1)
				    in_span = peak_pixel1 - pixel;
			    else
			    if(pixel < peak_pixel2 && pixel + in_span > peak_pixel2) 
				    in_span = peak_pixel2 - pixel;

    // Clip length to color changes
			    if(image_number == METER_GREEN && pixel + in_span > low_division)
				    in_span = low_division - pixel;
			    else
			    if(image_number == METER_YELLOW && pixel + in_span > medium_division)
				    in_span = medium_division - pixel;
			    else
			    if(image_number == METER_RED && pixel + in_span > high_division)
				    in_span = high_division - pixel;

    // Clip length to regions
			    if(pixel < left_pixel && pixel + in_span > left_pixel)
				    in_span = left_pixel - pixel;
			    else
			    if(pixel < right_pixel && pixel + in_span > right_pixel)
				    in_span = right_pixel - pixel;

    //printf("BC_Meter::draw_face image_number %d pixel %d pixels %d in_start %d in_span %d\n", image_number, pixel, pixels, in_start, in_span);
    //printf("BC_Meter::draw_face %d %d %d %d\n", orientation, region, images[image_number]->get_h() - in_start - in_span);
			    if(orientation == METER_HORIZ)
			    {
				    draw_pixmap(images[image_number], 
					    pixel, 
					    x, 
					    in_span + 1, 
					    get_h(), 
					    in_start, 
					    0);
			    }
			    else
			    {
    //printf("BC_Meter::draw_face %d %d\n", __LINE__, span);
				    if(span < 0)
				    {
					    draw_pixmap(images[image_number],
						    x,
						    get_h() - pixel - in_span,
						    get_w(),
						    in_span + 1,
						    0,
						    images[image_number]->get_h() - in_start - in_span);
				    }
				    else
				    {
					    int total_w = get_w() - x;
					    int third = images[image_number]->get_w() / 3 + 1;


					    for(int x1 = 0; x1 < total_w; x1 += third)
					    {
						    int in_x = 0;
						    int in_w = third;
						    if(x1 >= third) in_x = third;
						    if(x1 >= total_w - third)
						    {
							    in_x = images[image_number]->get_w() - 
								    (total_w - x1);
							    in_w = total_w - x1;
						    }

						    int in_y = images[image_number]->get_h() - in_start - in_span;
    //printf("BC_Meter::draw_face %d %d %d\n", __LINE__, get_w(), x + x1 + in_w, in_x, in_y, in_w, span);


						    draw_pixmap(images[image_number],
							    x + x1,
							    get_h() - pixel - in_span,
							    in_w,
							    in_span + 1,
							    in_x,
							    in_y);
					    }
				    }
			    }

			    pixel += in_span;
		    }
		    else
		    {
    // Sanity check
			    break;
		    }
	    }
    }

	if(over_timer)
	{
		if(orientation == METER_HORIZ)
			draw_pixmap(images[METER_OVER], 
				DP(10), 
				DP(2));
		else
			draw_pixmap(images[METER_OVER],
				x + DP(2),
				get_h() - DP(100));

		over_timer--;
	}

   	if(orientation == METER_HORIZ)
		flash(0, 0, pixels, get_h(), flush);
	else
		flash(x, 0, w, pixels, flush);
}

int BC_Meter::update(float new_value, int over)
{
	peak_timer++;

	if(mode == METER_DB)
	{
		if(new_value == 0) 
			level = min;
		else
			level = DB::todb(new_value);        // db value
	}


	if(is_gain_change && fabs(level) > fabs(peak) ||
        !is_gain_change && level > peak || 
        peak_timer > peak_delay)
	{
		peak = level;
		peak_timer = 0;
	}

// if(orientation == METER_HORIZ)
// printf("BC_Meter::update %f\n", level);
	if(over) over_timer = over_delay;	
// only draw if window is visible

	draw_face(1);
	return 0;
}
