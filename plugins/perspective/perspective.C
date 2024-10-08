/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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

#include "affine.h"
#include "cursors.h"
#include "language.h"
#include "perspective.h"
#include "theme.h"






REGISTER_PLUGIN(PerspectiveMain)



PerspectiveConfig::PerspectiveConfig()
{
	x1 = 0;
	y1 = 0;
	x2 = 100;
	y2 = 0;
	x3 = 100;
	y3 = 100;
	x4 = 0;
	y4 = 100;
	mode = AffineEngine::PERSPECTIVE;
	window_w = DP(400);
	window_h = DP(450);
	current_point = 0;
	forward = 1;
}

int PerspectiveConfig::equivalent(PerspectiveConfig &that)
{
	return 
		EQUIV(x1, that.x1) &&
		EQUIV(y1, that.y1) &&
		EQUIV(x2, that.x2) &&
		EQUIV(y2, that.y2) &&
		EQUIV(x3, that.x3) &&
		EQUIV(y3, that.y3) &&
		EQUIV(x4, that.x4) &&
		EQUIV(y4, that.y4) &&
		mode == that.mode &&
		forward == that.forward;
}

void PerspectiveConfig::copy_from(PerspectiveConfig &that)
{
	x1 = that.x1;
	y1 = that.y1;
	x2 = that.x2;
	y2 = that.y2;
	x3 = that.x3;
	y3 = that.y3;
	x4 = that.x4;
	y4 = that.y4;
	mode = that.mode;
	window_w = that.window_w;
	window_h = that.window_h;
	current_point = that.current_point;
	forward = that.forward;
}

void PerspectiveConfig::interpolate(PerspectiveConfig &prev, 
	PerspectiveConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->x1 = prev.x1 * prev_scale + next.x1 * next_scale;
	this->y1 = prev.y1 * prev_scale + next.y1 * next_scale;
	this->x2 = prev.x2 * prev_scale + next.x2 * next_scale;
	this->y2 = prev.y2 * prev_scale + next.y2 * next_scale;
	this->x3 = prev.x3 * prev_scale + next.x3 * next_scale;
	this->y3 = prev.y3 * prev_scale + next.y3 * next_scale;
	this->x4 = prev.x4 * prev_scale + next.x4 * next_scale;
	this->y4 = prev.y4 * prev_scale + next.y4 * next_scale;
	mode = prev.mode;
	forward = prev.forward;
}













PerspectiveWindow::PerspectiveWindow(PerspectiveMain *plugin)
 : PluginClientWindow(plugin,
	DP(400), 
	DP(450), 
	DP(400), 
	DP(450), 
	0)
{
//printf("PerspectiveWindow::PerspectiveWindow 1 %d %d\n", plugin->config.window_w, plugin->config.window_h);
	this->plugin = plugin; 
}

PerspectiveWindow::~PerspectiveWindow()
{
}

void PerspectiveWindow::create_objects()
{
    PerspectiveReset *reset;
    BC_Title *title;
    int margin = client->get_theme()->widget_border;
	int x = margin, y = margin;

	add_subwindow(canvas = new PerspectiveCanvas(plugin, 
		x, 
		y, 
		get_w() - DP(20), 
		get_h() - DP(140)));
	canvas->set_cursor(CROSS_CURSOR, 0, 0);
	y += canvas->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Point:")));
	x += title->get_w() + margin;

    add_subwindow(point1 = new PerspectivePoint(plugin, 
		x, 
		y, 
		0,
		_("1")));
    x += point1->get_w() + margin;
    add_subwindow(point2 = new PerspectivePoint(plugin, 
		x, 
		y, 
		1,
		_("2")));
    x += point2->get_w() + margin;
    add_subwindow(point3 = new PerspectivePoint(plugin, 
		x, 
		y, 
		2,
		_("3")));
    x += point3->get_w() + margin;
    add_subwindow(point4 = new PerspectivePoint(plugin, 
		x, 
		y, 
		3,
		_("4")));
    y += point4->get_h() + margin;
    x = margin;
    
    
	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w() + margin;
	this->x = new PerspectiveCoord(this, 
		plugin, 
		x, 
		y, 
		plugin->get_current_x(),
		1);
	this->x->create_objects();
	x += this->x->get_w() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
    x += title->get_w() + margin;
	this->y = new PerspectiveCoord(this, 
		plugin, 
		x, 
		y, 
		plugin->get_current_y(),
		0);
	this->y->create_objects();
	y += this->y->get_h() + margin;
	x = margin;
	add_subwindow(reset = new PerspectiveReset(plugin, x, y));
//	x += reset->get_w() + margin;
    y += reset->get_h() + margin;
    add_subwindow(title = new BC_Title(x, y, _("Mode:")));
    x += title->get_w() + margin;
	add_subwindow(mode = new PerspectiveMode(plugin, 
        this,
		x, 
		y, 
        get_w() - x - margin - BC_PopupMenu::calculate_w(0)));
    mode->create_objects();
	update_canvas();
	y += mode->get_h() + margin;
	x = margin;
	add_subwindow(title = new BC_Title(x, y, _("Perspective direction:")));
	x += title->get_w() + margin;
	add_subwindow(forward = new PerspectiveDirection(plugin, 
		x, 
		y, 
		1,
		_("Forward")));
	x += forward->get_w() + margin;
	add_subwindow(reverse = new PerspectiveDirection(plugin, 
		x, 
		y, 
		0,
		_("Reverse")));

	show_window();
}



int PerspectiveWindow::resize_event(int w, int h)
{
	return 1;
}

void PerspectiveWindow::update_canvas()
{
	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	int x1, y1, x2, y2, x3, y3, x4, y4;
	calculate_canvas_coords(x1, y1, x2, y2, x3, y3, x4, y4);

// printf("PerspectiveWindow::update_canvas %d,%d %d,%d %d,%d %d,%d\n",
// x1,
// y1,
// x2,
// y2,
// x3,
// y3,
// x4,
// y4);
// Draw divisions
	canvas->set_color(WHITE);

#define DIVISIONS 10
	for(int i = 0; i <= DIVISIONS; i++)
	{
// latitude
		canvas->draw_line(
			x1 + (x4 - x1) * i / DIVISIONS,
			y1 + (y4 - y1) * i / DIVISIONS,
			x2 + (x3 - x2) * i / DIVISIONS,
			y2 + (y3 - y2) * i / DIVISIONS);

// longitude
		canvas->draw_line(
			x1 + (x2 - x1) * i / DIVISIONS,
			y1 + (y2 - y1) * i / DIVISIONS,
			x4 + (x3 - x4) * i / DIVISIONS,
			y4 + (y3 - y4) * i / DIVISIONS);
	}

// Corners
#define RADIUS DP(5)
	if(plugin->config.current_point == 0)
	{
        for(int i = 0; i <= DIVISIONS / 2; i++)
        {
// latitude
            canvas->draw_half_line(
                x1 + (x4 - x1) * i / DIVISIONS,
			    y1 + (y4 - y1) * i / DIVISIONS,
			    x2 + (x3 - x2) * i / DIVISIONS,
			    y2 + (y3 - y2) * i / DIVISIONS);
// longitude
		    canvas->draw_half_line(
			    x1 + (x2 - x1) * i / DIVISIONS,
			    y1 + (y2 - y1) * i / DIVISIONS,
			    x4 + (x3 - x4) * i / DIVISIONS,
			    y4 + (y3 - y4) * i / DIVISIONS);
        }

    	canvas->draw_disc(x1 - RADIUS, y1 - RADIUS, RADIUS * 2, RADIUS * 2);
	}
    else
	{
    	canvas->draw_circle(x1 - RADIUS, y1 - RADIUS, RADIUS * 2, RADIUS * 2);
    }

	if(plugin->config.current_point == 1)
	{
        for(int i = 0; i <= DIVISIONS / 2; i++)
        {
// latitude
            canvas->draw_half_line(
			    x2 + (x3 - x2) * i / DIVISIONS,
			    y2 + (y3 - y2) * i / DIVISIONS,
                x1 + (x4 - x1) * i / DIVISIONS,
			    y1 + (y4 - y1) * i / DIVISIONS);
// longitude
		    canvas->draw_half_line(
			    x1 + (x2 - x1) * (i + DIVISIONS / 2) / DIVISIONS,
			    y1 + (y2 - y1) * (i + DIVISIONS / 2) / DIVISIONS,
			    x4 + (x3 - x4) * (i + DIVISIONS / 2) / DIVISIONS,
			    y4 + (y3 - y4) * (i + DIVISIONS / 2) / DIVISIONS);
        }

    	canvas->draw_disc(x2 - RADIUS, y2 - RADIUS, RADIUS * 2, RADIUS * 2);
	}
    else
	{
    	canvas->draw_circle(x2 - RADIUS, y2 - RADIUS, RADIUS * 2, RADIUS * 2);
    }

	if(plugin->config.current_point == 2)
	{
        for(int i = DIVISIONS / 2; i <= DIVISIONS; i++)
        {
// latitude
            canvas->draw_half_line(
			    x2 + (x3 - x2) * i / DIVISIONS,
			    y2 + (y3 - y2) * i / DIVISIONS,
                x1 + (x4 - x1) * i / DIVISIONS,
			    y1 + (y4 - y1) * i / DIVISIONS);
// longitude
		    canvas->draw_half_line(
			    x4 + (x3 - x4) * i / DIVISIONS,
			    y4 + (y3 - y4) * i / DIVISIONS,
                x1 + (x2 - x1) * i / DIVISIONS,
			    y1 + (y2 - y1) * i / DIVISIONS);
        }
    	canvas->draw_disc(x3 - RADIUS, y3 - RADIUS, RADIUS * 2, RADIUS * 2);
	}
    else
	{
    	canvas->draw_circle(x3 - RADIUS, y3 - RADIUS, RADIUS * 2, RADIUS * 2);
    }

	if(plugin->config.current_point == 3)
	{
        for(int i = DIVISIONS / 2; i <= DIVISIONS; i++)
        {
// latitude
            canvas->draw_half_line(
                x1 + (x4 - x1) * i / DIVISIONS,
			    y1 + (y4 - y1) * i / DIVISIONS,
			    x2 + (x3 - x2) * i / DIVISIONS,
			    y2 + (y3 - y2) * i / DIVISIONS);
// longitude
		    canvas->draw_half_line(
			    x4 + (x3 - x4) * (i - DIVISIONS / 2) / DIVISIONS,
			    y4 + (y3 - y4) * (i - DIVISIONS / 2) / DIVISIONS,
                x1 + (x2 - x1) * (i - DIVISIONS / 2) / DIVISIONS,
			    y1 + (y2 - y1) * (i - DIVISIONS / 2) / DIVISIONS);
        }
    	canvas->draw_disc(x4 - RADIUS, y4 - RADIUS, RADIUS * 2, RADIUS * 2);
	}
    else
	{
    	canvas->draw_circle(x4 - RADIUS, y4 - RADIUS, RADIUS * 2, RADIUS * 2);
    }

	canvas->flash();
}

void PerspectiveWindow::update_mode()
{
	mode->set_text(PerspectiveMode::value_to_text(plugin->config.mode));
	forward->update(plugin->config.forward);
	reverse->update(!plugin->config.forward);
}


void PerspectiveWindow::update_point()
{
	point1->update(plugin->config.current_point == 0);
	point2->update(plugin->config.current_point == 1);
	point3->update(plugin->config.current_point == 2);
	point4->update(plugin->config.current_point == 3);
}


void PerspectiveWindow::update_coord()
{
    
	x->update(plugin->get_current_x());
	y->update(plugin->get_current_y());
}

void PerspectiveWindow::calculate_canvas_coords(int &x1, 
	int &y1, 
	int &x2, 
	int &y2, 
	int &x3, 
	int &y3, 
	int &x4, 
	int &y4)
{
	int w = canvas->get_w() - 1;
	int h = canvas->get_h() - 1;
	if(plugin->config.mode == AffineEngine::PERSPECTIVE ||
		plugin->config.mode == AffineEngine::STRETCH)
	{
		x1 = (int)(plugin->config.x1 * w / 100);
		y1 = (int)(plugin->config.y1 * h / 100);
		x2 = (int)(plugin->config.x2 * w / 100);
		y2 = (int)(plugin->config.y2 * h / 100);
		x3 = (int)(plugin->config.x3 * w / 100);
		y3 = (int)(plugin->config.y3 * h / 100);
		x4 = (int)(plugin->config.x4 * w / 100);
		y4 = (int)(plugin->config.y4 * h / 100);
	}
	else
	{
		x1 = (int)(plugin->config.x1 * w) / 100;
		y1 = 0;
		x2 = x1 + w;
		y2 = 0;
		x4 = (int)(plugin->config.x4 * w) / 100;
		y4 = h;
		x3 = x4 + w;
		y3 = h;
	}
}




PerspectiveCanvas::PerspectiveCanvas(PerspectiveMain *plugin, 
	int x, 
	int y, 
	int w,
	int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
	state = PerspectiveCanvas::NONE;
}


void PerspectiveCanvas::draw_half_line(int x1, int y1, int x2, int y2)
{
    int half_x = (x1 + x2) / 2;
    int half_y = (y1 + y2) / 2;
// Always double line width
    set_line_width(2);
    draw_line(
        x1,
		y1,
		half_x,
		half_y);
    set_line_width(1);
}

int PerspectiveCanvas::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
// Set current point
		int x1, y1, x2, y2, x3, y3, x4, y4;
		int cursor_x = get_cursor_x();
		int cursor_y = get_cursor_y();
		((PerspectiveWindow*)plugin->thread->window)->calculate_canvas_coords(x1, y1, x2, y2, x3, y3, x4, y4);

		float distance1 = DISTANCE(cursor_x, cursor_y, x1, y1);
		float distance2 = DISTANCE(cursor_x, cursor_y, x2, y2);
		float distance3 = DISTANCE(cursor_x, cursor_y, x3, y3);
		float distance4 = DISTANCE(cursor_x, cursor_y, x4, y4);
// printf("PerspectiveCanvas::button_press_event %f %d %d %d %d\n", 
// distance3,
// cursor_x,
// cursor_y,
// x3,
// y3);
		float min = distance1;
		plugin->config.current_point = 0;
		if(distance2 < min)
		{
			min = distance2;
			plugin->config.current_point = 1;
		}
		if(distance3 < min)
		{
			min = distance3;
			plugin->config.current_point = 2;
		}
		if(distance4 < min)
		{
			min = distance4;
			plugin->config.current_point = 3;
		}

		if(plugin->config.mode == AffineEngine::SHEER)
		{
			if(plugin->config.current_point == 1)
				plugin->config.current_point = 0;
			else
			if(plugin->config.current_point == 2)
				plugin->config.current_point = 3;
		}
		start_cursor_x = cursor_x;
		start_cursor_y = cursor_y;

		if(alt_down() || shift_down())
		{
			if(alt_down())
				state = PerspectiveCanvas::DRAG_FULL;
			else
				state = PerspectiveCanvas::ZOOM;

// Get starting positions
			start_x1 = plugin->config.x1;
			start_y1 = plugin->config.y1;
			start_x2 = plugin->config.x2;
			start_y2 = plugin->config.y2;
			start_x3 = plugin->config.x3;
			start_y3 = plugin->config.y3;
			start_x4 = plugin->config.x4;
			start_y4 = plugin->config.y4;
		}
		else
		{
			state = PerspectiveCanvas::DRAG;

// Get starting positions
			start_x1 = plugin->get_current_x();
			start_y1 = plugin->get_current_y();
		}
        ((PerspectiveWindow*)plugin->thread->window)->update_point();
		((PerspectiveWindow*)plugin->thread->window)->update_coord();
		((PerspectiveWindow*)plugin->thread->window)->update_canvas();
		return 1;
	}

	return 0;
}

int PerspectiveCanvas::button_release_event()
{
	if(state != PerspectiveCanvas::NONE)
	{
		state = PerspectiveCanvas::NONE;
		return 1;
	}
	return 0;
}

int PerspectiveCanvas::cursor_motion_event()
{
	if(state != PerspectiveCanvas::NONE)
	{
		int w = get_w() - 1;
		int h = get_h() - 1;
		if(state == PerspectiveCanvas::DRAG)
		{
			plugin->set_current_x((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x1);
			plugin->set_current_y((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y1);
		}
		else
		if(state == PerspectiveCanvas::DRAG_FULL)
		{
			plugin->config.x1 = ((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x1);
			plugin->config.y1 = ((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y1);
			plugin->config.x2 = ((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x2);
			plugin->config.y2 = ((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y2);
			plugin->config.x3 = ((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x3);
			plugin->config.y3 = ((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y3);
			plugin->config.x4 = ((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x4);
			plugin->config.y4 = ((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y4);
		}
		else
		if(state == PerspectiveCanvas::ZOOM)
		{
			float center_x = (start_x1 +
				start_x2 +
				start_x3 +
				start_x4) / 4;
			float center_y = (start_y1 +
				start_y2 +
				start_y3 +
				start_y4) / 4;
			float zoom = (float)(get_cursor_y() - start_cursor_y + 640) / 640;
			plugin->config.x1 = center_x + (start_x1 - center_x) * zoom;
			plugin->config.y1 = center_y + (start_y1 - center_y) * zoom;
			plugin->config.x2 = center_x + (start_x2 - center_x) * zoom;
			plugin->config.y2 = center_y + (start_y2 - center_y) * zoom;
			plugin->config.x3 = center_x + (start_x3 - center_x) * zoom;
			plugin->config.y3 = center_y + (start_y3 - center_y) * zoom;
			plugin->config.x4 = center_x + (start_x4 - center_x) * zoom;
			plugin->config.y4 = center_y + (start_y4 - center_y) * zoom;
		}
		((PerspectiveWindow*)plugin->thread->window)->update_canvas();
		((PerspectiveWindow*)plugin->thread->window)->update_coord();
		plugin->send_configure_change();
		return 1;
	}

	return 0;
}






PerspectiveCoord::PerspectiveCoord(PerspectiveWindow *gui,
	PerspectiveMain *plugin, 
	int x, 
	int y,
	float value,
	int is_x)
 : BC_TumbleTextBox(gui, value, (float)-200, (float)200, x, y, DP(100))
{
	this->plugin = plugin;
	this->is_x = is_x;
}

int PerspectiveCoord::handle_event()
{
	if(is_x)
		plugin->set_current_x(atof(get_text()));
	else
		plugin->set_current_y(atof(get_text()));
	((PerspectiveWindow*)plugin->thread->window)->update_canvas();
	plugin->send_configure_change();
	return 1;
}








PerspectiveReset::PerspectiveReset(PerspectiveMain *plugin, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
}
int PerspectiveReset::handle_event()
{
	plugin->config.x1 = 0;
	plugin->config.y1 = 0;
	plugin->config.x2 = 100;
	plugin->config.y2 = 0;
	plugin->config.x3 = 100;
	plugin->config.y3 = 100;
	plugin->config.x4 = 0;
	plugin->config.y4 = 100;
	((PerspectiveWindow*)plugin->thread->window)->update_canvas();
	((PerspectiveWindow*)plugin->thread->window)->update_coord();
	plugin->send_configure_change();
	return 1;
}










#define MENU_OPTIONS 3
PerspectiveMode::PerspectiveMode(PerspectiveMain *plugin, 
    PerspectiveWindow *gui,
	int x, 
	int y,
    int w)
 : BC_PopupMenu(x,
    y, 
    w, 
    PerspectiveMode::value_to_text(plugin->config.mode), 
    1)
{
//printf("PerspectiveMode::PerspectiveMode %d w=%d\n", __LINE__, w);
	this->plugin = plugin;
	this->gui = gui;
}


int PerspectiveMode::handle_event()
{
	plugin->config.mode = text_to_value(get_text());
	gui->update_canvas();
	plugin->send_configure_change();
	return 1;
}

void PerspectiveMode::create_objects()
{
	for(int i = 0; i < MENU_OPTIONS; i++)
	{
		add_item(new BC_MenuItem(value_to_text(i)));
	}
}


const char* PerspectiveMode::value_to_text(int value)
{
	switch(value)
	{
		case AffineEngine::PERSPECTIVE: return _("Perspective");
		case AffineEngine::SHEER: return _("Sheer");
		case AffineEngine::STRETCH: return _("Stretch");
	}

	return _("Perspective");
}

int PerspectiveMode::text_to_value(char *text)
{
	for(int i = 0; i < MENU_OPTIONS; i++)
	{
		if(!strcmp(value_to_text(i), text)) return i;
	}

	return AffineEngine::PERSPECTIVE;
}



PerspectiveDirection::PerspectiveDirection(PerspectiveMain *plugin, 
	int x, 
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->config.forward == value, text)
{
	this->plugin = plugin;
	this->value = value;
}
int PerspectiveDirection::handle_event()
{
	plugin->config.forward = value;
	((PerspectiveWindow*)plugin->thread->window)->update_mode();
	plugin->send_configure_change();
	return 1;
}




PerspectivePoint::PerspectivePoint(PerspectiveMain *plugin, 
	int x, 
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->config.current_point == value, text)
{
	this->plugin = plugin;
	this->value = value;
}
int PerspectivePoint::handle_event()
{
	plugin->config.current_point = value;
	((PerspectiveWindow*)plugin->thread->window)->update_point();
	((PerspectiveWindow*)plugin->thread->window)->update_coord();
    ((PerspectiveWindow*)plugin->thread->window)->update_canvas();
	plugin->send_configure_change();
	return 1;
}










PerspectiveMain::PerspectiveMain(PluginServer *server)
 : PluginVClient(server)
{
	
	engine = 0;
	temp = 0;
}

PerspectiveMain::~PerspectiveMain()
{
	
	if(engine) delete engine;
	if(temp) delete temp;
}

const char* PerspectiveMain::plugin_title() { return N_("Perspective"); }
int PerspectiveMain::is_realtime() { return 1; }


NEW_PICON_MACRO(PerspectiveMain)

NEW_WINDOW_MACRO(PerspectiveMain, PerspectiveWindow)

LOAD_CONFIGURATION_MACRO(PerspectiveMain, PerspectiveConfig)



void PerspectiveMain::update_gui()
{
	if(thread)
	{
//printf("PerspectiveMain::update_gui 1\n");
		thread->window->lock_window();
//printf("PerspectiveMain::update_gui 2\n");
		load_configuration();
        ((PerspectiveWindow*)thread->window)->update_point();
		((PerspectiveWindow*)thread->window)->update_coord();
		((PerspectiveWindow*)thread->window)->update_mode();
		((PerspectiveWindow*)thread->window)->update_canvas();
		thread->window->unlock_window();
//printf("PerspectiveMain::update_gui 3\n");
	}
}





void PerspectiveMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("PERSPECTIVE");

	output.tag.set_property("X1", config.x1);
	output.tag.set_property("X2", config.x2);
	output.tag.set_property("X3", config.x3);
	output.tag.set_property("X4", config.x4);
	output.tag.set_property("Y1", config.y1);
	output.tag.set_property("Y2", config.y2);
	output.tag.set_property("Y3", config.y3);
	output.tag.set_property("Y4", config.y4);

	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("FORWARD", config.forward);
	output.tag.set_property("WINDOW_W", config.window_w);
	output.tag.set_property("WINDOW_H", config.window_h);
	output.append_tag();
	output.terminate_string();
}

void PerspectiveMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("PERSPECTIVE"))
			{
				config.x1 = input.tag.get_property("X1", config.x1);
				config.x2 = input.tag.get_property("X2", config.x2);
				config.x3 = input.tag.get_property("X3", config.x3);
				config.x4 = input.tag.get_property("X4", config.x4);
				config.y1 = input.tag.get_property("Y1", config.y1);
				config.y2 = input.tag.get_property("Y2", config.y2);
				config.y3 = input.tag.get_property("Y3", config.y3);
				config.y4 = input.tag.get_property("Y4", config.y4);

				config.mode = input.tag.get_property("MODE", config.mode);
				config.forward = input.tag.get_property("FORWARD", config.forward);
				config.window_w = input.tag.get_property("WINDOW_W", config.window_w);
				config.window_h = input.tag.get_property("WINDOW_H", config.window_h);
			}
		}
	}
}

float PerspectiveMain::get_current_x()
{
	switch(config.current_point)
	{
		case 0:
			return config.x1;
			break;
		case 1:
			return config.x2;
			break;
		case 2:
			return config.x3;
			break;
		case 3:
			return config.x4;
			break;
	}
    return 0;
}

float PerspectiveMain::get_current_y()
{
	switch(config.current_point)
	{
		case 0:
			return config.y1;
			break;
		case 1:
			return config.y2;
			break;
		case 2:
			return config.y3;
			break;
		case 3:
			return config.y4;
			break;
	}
    return 0;
}

void PerspectiveMain::set_current_x(float value)
{
	switch(config.current_point)
	{
		case 0:
			config.x1 = value;
			break;
		case 1:
			config.x2 = value;
			break;
		case 2:
			config.x3 = value;
			break;
		case 3:
			config.x4 = value;
			break;
	}
}

void PerspectiveMain::set_current_y(float value)
{
	switch(config.current_point)
	{
		case 0:
			config.y1 = value;
			break;
		case 1:
			config.y2 = value;
			break;
		case 2:
			config.y3 = value;
			break;
		case 3:
			config.y4 = value;
			break;
	}
}



int PerspectiveMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	int need_reconfigure = load_configuration();


// Do nothing
	if( EQUIV(config.x1, 0)   && EQUIV(config.y1, 0) &&
		EQUIV(config.x2, 100) && EQUIV(config.y2, 0) &&
		EQUIV(config.x3, 100) && EQUIV(config.y3, 100) &&
		EQUIV(config.x4, 0)   && EQUIV(config.y4, 100))
	{
		read_frame(frame, 
			0, 
			start_position, 
			frame_rate,
			get_use_opengl());
		return 1;
	}

// Opengl does some funny business with stretching.
	int use_opengl = get_use_opengl() &&
		(config.mode == AffineEngine::PERSPECTIVE || 
		config.mode == AffineEngine::SHEER);
	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		use_opengl);

	if(!engine) engine = new AffineEngine(get_project_smp() + 1,
		get_project_smp() + 1);

	if(use_opengl)
		return run_opengl();



	this->input = frame;
	this->output = frame;

	int w = frame->get_w();
	int h = frame->get_h();
	int color_model = frame->get_color_model();

	if(temp && 
		config.mode == AffineEngine::STRETCH &&
		(temp->get_w() != w * AFFINE_OVERSAMPLE ||
			temp->get_h() != h * AFFINE_OVERSAMPLE))
	{
		delete temp;
		temp = 0;
	}
	else
	if(temp &&
		(config.mode == AffineEngine::PERSPECTIVE ||
		config.mode == AffineEngine::SHEER) &&
		(temp->get_w() != w ||
			temp->get_h() != h))
	{
		delete temp;
		temp = 0;
	}

	if(config.mode == AffineEngine::STRETCH)
	{
		if(!temp)
		{
			temp = new VFrame(0,
					-1,
					w * AFFINE_OVERSAMPLE,
					h * AFFINE_OVERSAMPLE,
					color_model,
					-1);
		}
		temp->clear_frame();
	}

	if(config.mode == AffineEngine::PERSPECTIVE ||
		config.mode == AffineEngine::SHEER)
	{
		if(frame->get_rows()[0] == frame->get_rows()[0])
		{
			if(!temp) 
			{
				temp = new VFrame(0,
					-1,
					w,
					h,
					color_model,
					-1);
			}
			temp->copy_from(input);
			input = temp;
		}
		output->clear_frame();
	}


	engine->process(output,
		input,
		temp, 
		config.mode,
		config.x1,
		config.y1,
		config.x2,
		config.y2,
		config.x3,
		config.y3,
		config.x4,
		config.y4,
		config.forward);




// Resample

	if(config.mode == AffineEngine::STRETCH)
	{
#define RESAMPLE(type, components, chroma_offset) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *out_row = (type*)output->get_rows()[i]; \
		type *in_row1 = (type*)temp->get_rows()[i * AFFINE_OVERSAMPLE]; \
		type *in_row2 = (type*)temp->get_rows()[i * AFFINE_OVERSAMPLE + 1]; \
		for(int j = 0; j < w; j++) \
		{ \
			out_row[0] = (in_row1[0] +  \
					in_row1[components] +  \
					in_row2[0] +  \
					in_row2[components]) /  \
				AFFINE_OVERSAMPLE /  \
				AFFINE_OVERSAMPLE; \
			out_row[1] = ((in_row1[1] +  \
						in_row1[components + 1] +  \
						in_row2[1] +  \
						in_row2[components + 1]) -  \
					chroma_offset *  \
					AFFINE_OVERSAMPLE *  \
					AFFINE_OVERSAMPLE) /  \
				AFFINE_OVERSAMPLE /  \
				AFFINE_OVERSAMPLE + \
				chroma_offset; \
			out_row[2] = ((in_row1[2] +  \
						in_row1[components + 2] +  \
						in_row2[2] +  \
						in_row2[components + 2]) -  \
					chroma_offset *  \
					AFFINE_OVERSAMPLE *  \
					AFFINE_OVERSAMPLE) /  \
				AFFINE_OVERSAMPLE /  \
				AFFINE_OVERSAMPLE + \
				chroma_offset; \
			if(components == 4) \
			{ \
				out_row[3] = (in_row1[3] +  \
						in_row1[components + 3] +  \
						in_row2[3] +  \
						in_row2[components + 3]) /  \
					AFFINE_OVERSAMPLE /  \
					AFFINE_OVERSAMPLE; \
			} \
			out_row += components; \
			in_row1 += components * AFFINE_OVERSAMPLE; \
			in_row2 += components * AFFINE_OVERSAMPLE; \
		} \
	} \
}

		switch(frame->get_color_model())
		{
			case BC_RGB_FLOAT:
				RESAMPLE(float, 3, 0)
				break;
			case BC_RGB888:
				RESAMPLE(unsigned char, 3, 0)
				break;
			case BC_RGBA_FLOAT:
				RESAMPLE(float, 4, 0)
				break;
			case BC_RGBA8888:
				RESAMPLE(unsigned char, 4, 0)
				break;
			case BC_YUV888:
				RESAMPLE(unsigned char, 3, 0x80)
				break;
			case BC_YUVA8888:
				RESAMPLE(unsigned char, 4, 0x80)
				break;
			case BC_RGB161616:
				RESAMPLE(uint16_t, 3, 0)
				break;
			case BC_RGBA16161616:
				RESAMPLE(uint16_t, 4, 0)
				break;
			case BC_YUV161616:
				RESAMPLE(uint16_t, 3, 0x8000)
				break;
			case BC_YUVA16161616:
				RESAMPLE(uint16_t, 4, 0x8000)
				break;
		}
	}

	return 1;
}


int PerspectiveMain::handle_opengl()
{
#ifdef HAVE_GL
	engine->set_opengl(1);
	engine->process(get_output(),
		get_output(),
		get_output(), 
		config.mode,
		config.x1,
		config.y1,
		config.x2,
		config.y2,
		config.x3,
		config.y3,
		config.x4,
		config.y4,
		config.forward);
	engine->set_opengl(0);
	return 0;
#endif
    return 0;
}






