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

#include "clip.h"
#include "cursors.h"
#include "curves.h"
#include "curveswindow.h"
#include "language.h"
#include "theme.h"


CurvesReset::CurvesReset(CurvesMain *plugin, 
	int x,
	int y)
 : BC_GenericButton(x, y, _("Reset channel"))
{
	this->plugin = plugin;
}

int CurvesReset::handle_event()
{
	plugin->config.reset(plugin->mode, 1);
	((CurvesWindow*)plugin->thread->window)->update(1, 1, 1);
	plugin->send_configure_change();
	return 1;
}


CurvesMode::CurvesMode(CurvesMain *plugin, 
	int x, 
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->mode == value, text)
{
	this->plugin = plugin;
	this->value = value;
}

int CurvesMode::handle_event()
{
	CurvesWindow *gui = (CurvesWindow*)plugin->thread->window;
    plugin->mode = value;
    gui->update(1, 1, 1);
    return 1;
}


CurvesToggle::CurvesToggle(CurvesMain *plugin, 
	int x, 
	int y,
    int *output,
    const char *text)
 : BC_CheckBox(x, y, plugin->config.plot, text)
{
    this->plugin = plugin;
    this->output = output;
}

int CurvesToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}



CurvesCanvas::CurvesCanvas(CurvesMain *plugin,
	CurvesWindow *gui,
	int x,
	int y,
	int w,
	int h)
 : BC_SubWindow(x,
 	y,
	w,
	h,
	plugin->get_theme()->graph_bg_color)
{
	this->plugin = plugin;
	this->gui = gui;
    is_drag = 0;
}

void CurvesCanvas::point_to_canvas(int &x, int &y, curve_point_t *point)
{
    x = get_w() * (point->x - CURVE_MIN) / (CURVE_MAX - CURVE_MIN);
    y = get_h() - get_h() * (point->y - CURVE_MIN) / (CURVE_MAX - CURVE_MIN);
}

void CurvesCanvas::canvas_to_point(curve_point_t &point, int x, int y)
{
    point.x = (float)x * (CURVE_MAX - CURVE_MIN) / get_w() + CURVE_MIN;
    point.y = (float)(get_h() - y) * (CURVE_MAX - CURVE_MIN) / get_h() + CURVE_MIN;
}

int CurvesCanvas::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
// Check existing points
        for(int j = 0; j < plugin->config.points[plugin->mode].size(); j++)
        {
            curve_point_t *point = plugin->config.points[plugin->mode].get_pointer(j);
            int x, y;
            point_to_canvas(x, y, point);
			if(get_cursor_x() <= x + POINT_W / 2 && 
                get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() <= y + POINT_W / 2 && 
                get_cursor_y() >= y - POINT_W / 2)
			{
				is_drag = 1;
				plugin->current_point = j;
                x_offset = get_cursor_x() - x;
                y_offset = get_cursor_y() - y;
                set_cursor(CROSS_CURSOR, 0, 1);
				return 1;
			}
        }

        if(get_cursor_x() >= 0 &&
            get_cursor_x() < get_w() &&
            get_cursor_y() >= 0 &&
            get_cursor_y() < get_h())
        {
// Create new point
            curve_point_t point;
            canvas_to_point(point, get_cursor_x(), get_cursor_y());
            is_drag = 1;
            plugin->current_point = plugin->config.set_point(plugin->mode, point);
            x_offset = 0;
            y_offset = 0;
            gui->update(1, 0, 1);
            plugin->send_configure_change();
            set_cursor(CROSS_CURSOR, 0, 1);
		    return 1;
        }
    }
    return 0;
}

int CurvesCanvas::cursor_motion_event()
{
    if(is_drag)
    {
// update a point
        curve_point_t *point = plugin->config.points[plugin->mode].get_pointer(
            plugin->current_point);
        canvas_to_point(*point, get_cursor_x() - x_offset, get_cursor_y() - y_offset);

// clamp 1st & last point
        if(plugin->current_point == 0)
        {
            point->x = MAX(CURVE_MIN, point->x);
        }
        else
        if(plugin->current_point == plugin->config.points[plugin->mode].size() - 1)
        {
            point->x = MIN(CURVE_MAX, point->x);
        }

// clamp Y of all points
        CLAMP(point->y, CURVE_MIN, CURVE_MAX);
        
        gui->update(1, 0, 1);
        plugin->send_configure_change();
		return 1;
    }
    else
// Change cursor over points
    {
	    if(is_event_win() && cursor_inside())
	    {
		    int new_cursor = CROSS_CURSOR;
            for(int j = 0; j < plugin->config.points[plugin->mode].size(); j++)
            {
                curve_point_t *point = plugin->config.points[plugin->mode].get_pointer(j);
                int x, y;
                point_to_canvas(x, y, point);
			    if(get_cursor_x() <= x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				    get_cursor_y() <= y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			    {
				    new_cursor = UPRIGHT_ARROW_CURSOR;
				    break;
			    }
            }

// out of active area
            if(get_cursor_x() < 0 ||
                get_cursor_x() >= get_w() ||
                get_cursor_y() < 0 ||
                get_cursor_y() >= get_h())
            {
                new_cursor = ARROW_CURSOR;
            }

		    if(new_cursor != get_cursor())
		    {
			    set_cursor(new_cursor, 0, 1);
		    }
        }
    }

    return 0;
}

int CurvesCanvas::button_release_event()
{
    if(is_drag)
    {
// test for a delete
        curve_point_t *point = plugin->config.points[plugin->mode].get_pointer(
            plugin->current_point);
        int delete_it = 0;
        if(plugin->current_point > 0)
        {
            if(point->x < plugin->config.points[plugin->mode].get(
                plugin->current_point - 1).x)
                delete_it = 1;
        }

        if(plugin->current_point < plugin->config.points[plugin->mode].size() - 1)
        {
            if(point->x >= plugin->config.points[plugin->mode].get(
                plugin->current_point + 1).x)
                delete_it = 1;
        }

        if(point->x > CURVE_MAX || point->x < CURVE_MIN)
            delete_it = 1;
        
        if(delete_it) plugin->config.points[plugin->mode].remove_number(
            plugin->current_point);
        is_drag = 0;
        gui->update(1, 0, 1);
        plugin->send_configure_change();
        return 1;
    }
    return 0;
}



CurvesText::CurvesText(CurvesMain *plugin,
	CurvesWindow *gui,
	int x,
	int y,
    int w,
    int min,
    int max,
    int is_x,
    int is_y,
    int is_number)
 : BC_TumbleTextBox(gui, 
		0,
		min,
		max,
		x, 
		y, 
		w)
{
    this->plugin = plugin;
    this->gui = gui;
    this->is_x = is_x;
    this->is_y = is_y;
    this->is_number = is_number;
}

CurvesText::CurvesText(CurvesMain *plugin,
	CurvesWindow *gui,
	int x,
	int y,
    int w,
    float min,
    float max,
    int is_x,
    int is_y,
    int is_number)
 : BC_TumbleTextBox(gui, 
		(float)0.0,
		(float)min,
		(float)max,
		x, 
		y, 
		w)
{
    this->plugin = plugin;
    this->gui = gui;
    this->is_x = is_x;
    this->is_y = is_y;
    this->is_number = is_number;
}


int CurvesText::handle_event()
{
    if(is_number)
    {
        plugin->current_point = atoi(get_text());
        CLAMP(plugin->current_point, 0, plugin->config.points[plugin->mode].size() - 1);
        plugin->current_point = MAX(plugin->current_point, 0);
        gui->update(1, 0, 1);
    }

    if(is_x || is_y)
    {
        if(plugin->current_point >= 0 &&
            plugin->current_point < plugin->config.points[plugin->mode].size())
        {
            curve_point_t *point = plugin->config.points[plugin->mode].get_pointer(
                plugin->current_point);
            if(is_x) point->x = atof(get_text());
            if(is_y) point->y = atof(get_text());
            plugin->config.boundaries();
            gui->update(1, 0, 0);
            plugin->send_configure_change();
        }
    }
    return 1;
}



CurvesWindow::CurvesWindow(CurvesMain *plugin)
 : PluginClientWindow(plugin, 
	plugin->w, 
	plugin->h, 
	WINDOW_W, 
	WINDOW_H, 
	1)
{
	this->plugin = plugin;
}

CurvesWindow::~CurvesWindow()
{
}


void CurvesWindow::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
	int x = margin, y = margin;

	add_subwindow(mode_v = new CurvesMode(plugin, 
		x, 
		y,
		HISTOGRAM_VALUE,
		_("Value")));
	x += mode_v->get_w() + margin;
	add_subwindow(mode_r = new CurvesMode(plugin, 
		x, 
		y,
		HISTOGRAM_RED,
		_("Red")));
	x += mode_r->get_w() + margin;
	add_subwindow(mode_g = new CurvesMode(plugin, 
		x, 
		y,
		HISTOGRAM_GREEN,
		_("Green")));
	x += mode_g->get_w() + margin;
	add_subwindow(mode_b = new CurvesMode(plugin, 
		x, 
		y,
		HISTOGRAM_BLUE,
		_("Blue")));

    y += mode_b->get_h() + margin;
    x = margin;
    canvas_x = x;
    canvas_y = y;
    canvas_w = get_w() - margin * 2;
    canvas_h = get_h() - 
        y - 
        margin * 5 - 
        BC_GenericButton::calculate_h() -
        BC_CheckBox::calculate_h(this) * 2 -
        BC_TumbleTextBox::calculate_h();

	add_subwindow(canvas = new CurvesCanvas(plugin,
		this,
		canvas_x, 
		canvas_y, 
		canvas_w, 
		canvas_h));

    y += canvas_h + margin;
    int x1 = x;
    add_subwindow(title1 = new BC_Title(x, y, _("Point:")));
    x += title1->get_w() + margin;
    number = new CurvesText(plugin,
	    this,
	    x,
	    y,
        DP(50),
        (int)0,
        (int)MAX_POINTS,
        0,
        0,
        1);
    number->create_objects();
    x += number->get_w() + margin;

    add_subwindow(title2 = new BC_Title(x, y, _("X:")));
    x += title2->get_w() + margin;
    this->x = new CurvesText(plugin,
	    this,
	    x,
	    y,
        DP(90),
        (float)CURVE_MIN,
        (float)CURVE_MAX,
        1,
        0,
        0);
    this->x->set_increment(0.001);
    this->x->set_precision(3);
    this->x->create_objects();
    x += this->x->get_w() + margin;
    
    
    add_subwindow(title3 = new BC_Title(x, y, _("Y:")));
    x += title3->get_w() + margin;
    this->y = new CurvesText(plugin,
	    this,
	    x,
	    y,
        DP(90),
        (float)CURVE_MIN,
        (float)CURVE_MAX,
        0,
        1,
        0);
    this->y->set_increment(0.001);
    this->y->set_precision(3);
    this->y->create_objects();
    x = x1;
    y += this->x->get_h() + margin;
    
    
    
    add_subwindow(reset = new CurvesReset(plugin, 
		x, 
		y));
    y += reset->get_h() + margin;
    add_subwindow(plot = new CurvesToggle(plugin, 
		x, 
		y,
        &plugin->config.plot,
        _("Plot histogram")));
    y += plot->get_h() + margin;
    add_subwindow(split = new CurvesToggle(plugin, 
		x, 
		y,
        &plugin->config.split,
        _("Split output")));
    update(1, 1, 1);

	flash(0);
	show_window(1);
}

int CurvesWindow::resize_event(int w, int h)
{
	int xdiff = w - get_w();
	int ydiff = h - get_h();

	canvas_w = canvas_w + xdiff;
	canvas_h = canvas_h + ydiff;
    canvas->reposition_window(canvas_x,
		canvas_y,
		canvas_w,
		canvas_h);
    title1->reposition_window(title1->get_x(), title1->get_y() + ydiff);
    title2->reposition_window(title2->get_x(), title2->get_y() + ydiff);
    title3->reposition_window(title3->get_x(), title3->get_y() + ydiff);
    number->reposition_window(number->get_x(), number->get_y() + ydiff);
    this->x->reposition_window(this->x->get_x(), this->x->get_y() + ydiff);
    this->y->reposition_window(this->y->get_x(), this->y->get_y() + ydiff);
    reset->reposition_window(reset->get_x(), reset->get_y() + ydiff);
    plot->reposition_window(plot->get_x(), plot->get_y() + ydiff);
    split->reposition_window(split->get_x(), split->get_y() + ydiff);
    update(1, 1, 1);

	plugin->w = w;
	plugin->h = h;
	flash();
}

void CurvesWindow::update(int do_canvas,
	int do_toggles,
    int do_text)
{
    if(do_toggles)
    {
		mode_v->update(plugin->mode == HISTOGRAM_VALUE ? 1 : 0);
		mode_r->update(plugin->mode == HISTOGRAM_RED ? 1 : 0);
		mode_g->update(plugin->mode == HISTOGRAM_GREEN ? 1 : 0);
		mode_b->update(plugin->mode == HISTOGRAM_BLUE ? 1 : 0);
        plot->update(plugin->config.plot);
        split->update(plugin->config.split);
    }
    
    if(do_text)
    {
        CLAMP(plugin->current_point, 0, plugin->config.points[plugin->mode].size() - 1);
        plugin->current_point = MAX(plugin->current_point, 0);
        
        number->update((int64_t)plugin->current_point);
        if(plugin->current_point >= 0 &&
            plugin->current_point < plugin->config.points[plugin->mode].size())
        {
            curve_point_t *point = plugin->config.points[plugin->mode].get_pointer(
                plugin->current_point);
            this->x->update(point->x);
            this->y->update(point->y);
        }
    }

    if(do_canvas)
    {
        canvas->set_color(plugin->get_theme()->graph_bg_color);
        canvas->draw_box(0, 0, canvas->get_w(), canvas->get_h());

// the histogram
        plugin->tools->draw(canvas, 
            0, 
            canvas_h, 
            plugin->mode,
            CURVE_MIN,
            CURVE_MAX);



// 1:1 line
//        canvas->set_line_dashes(1);
	    canvas->set_color(plugin->get_theme()->graph_active_color);
        canvas->draw_line(0, canvas_h, canvas_w, 0);
//        canvas->set_line_dashes(0);


        canvas->set_font(MEDIUMFONT);
	    canvas->set_color(plugin->get_theme()->graph_grid_color);
        canvas->draw_text(plugin->get_theme()->widget_border, 
	        canvas->get_h() / 2, 
	        _("Output"));
        canvas->draw_text(canvas->get_w() / 2 - get_text_width(MEDIUMFONT, _("Input")) / 2, 
	        canvas->get_h() - plugin->get_theme()->widget_border, 
	        _("Input"));

// the line
        int y1, y2;
        canvas->set_color(plugin->get_theme()->graph_active_color);
        canvas->set_line_width(2);
        for(int i = 0; i < canvas_w; i++)
        {
            float output = plugin->calculate_level(
                (float)i * (CURVE_MAX - CURVE_MIN) / canvas_w + CURVE_MIN,
                plugin->mode,
                0);
            y2 = canvas_h - (int)(output * canvas_h);
            if(i > 0)
                canvas->draw_line(i - 1, y1, i, y2);
            y1 = y2;
        }
        canvas->set_line_width(1);

// the points
// printf("CurvesWindow::update %d mode=%d size=%d\n", 
// __LINE__, 
// plugin->mode,
// plugin->config.points[plugin->mode].size());
        for(int i = 0; i < plugin->config.points[plugin->mode].size(); i++)
        {
            curve_point_t *point = plugin->config.points[plugin->mode].get_pointer(i);
            int x, y;
            canvas->point_to_canvas(x, y, point);
            if(plugin->current_point == i)
                canvas->draw_box(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
            else
                canvas->draw_rectangle(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
        }
        canvas->flash(1);
    }

}



