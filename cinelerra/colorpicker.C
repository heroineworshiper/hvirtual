
/*
 * CINELERRA
 * Copyright (C) 1997-2019 Adam Williams <broadcast at earthling dot net>
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
#include "colorpicker.h"
#include "condition.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.inc"
#include "cicolors.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>


ColorThread::ColorThread(int do_alpha, char *title)
 : Thread()
{
	window = 0;
	this->title = title;
	this->do_alpha = do_alpha;
	set_synchronous(0);
	mutex = new Mutex("ColorThread::mutex");
	completion = new Condition(1, "ColorThread::completion");
}

ColorThread::~ColorThread()
{
	if(running())
	{
		window->set_done(0);
		completion->lock("ColorThread::~ColorThread");
		completion->unlock();
	}
	delete mutex;
	delete completion;
}

void ColorThread::start_window(int output, int alpha)
{
	mutex->lock("ColorThread::start_window 1");
	this->output = output;
	this->alpha = alpha;
	mutex->unlock();

	if(!running())
	{
		completion->lock("ColorThread::start_window");
		Thread::start();
	}
	else
	{
		window->raise_window();
		window->flush();
	}
}

void ColorThread::run()
{
	BC_DisplayInfo info;
//printf("ColorThread::run 1\n");
	char window_title[BCTEXTLEN];

	strcpy(window_title, PROGRAM_NAME ": ");
	if(title)
		strcat(window_title, title);
	else
		strcat(window_title, _("Color Picker"));


	mutex->lock("ColorThread::run 1");
	window = new ColorWindow(this, 
		info.get_abs_cursor_x() - 200, 
		info.get_abs_cursor_y() - 200,
		window_title);
	window->create_objects();
	mutex->unlock();
	window->run_window();
	mutex->lock("lorThread::run 2");
	delete window;
	window = 0;
	mutex->unlock();
	completion->unlock();
}

void ColorThread::update_gui(int output, int alpha)
{
	mutex->lock("ColorThread::update_gui");
	if (window)
	{
		this->output = output;
		this->alpha = alpha;
		window->update_values();
		window->lock_window();
		window->update_display();
		window->unlock_window();
	}
	mutex->unlock();
}

int ColorThread::handle_new_color(int output, int alpha)
{
	printf("ColorThread::handle_new_color undefined.\n");
	return 0;
}

ColorWindow* ColorThread::get_gui()
{
	return window;
}


ColorObjects::ColorObjects(BC_Window *window, int do_alpha, int x, int y)
{
    this->window = window;
    this->do_alpha = do_alpha;
    this->x = x;
    this->y = y;
}

ColorObjects::~ColorObjects()
{
}

void ColorObjects::create_objects()
{
    int x = this->x;
    int y = this->y;
    
	window->add_tool(wheel = new PaletteWheel(this, x, y));
	wheel->create_objects();

	x += DP(180);
	window->add_tool(wheel_value = new PaletteWheelValue(this, x, y));
	wheel_value->create_objects();


	y += DP(180);
	x = this->x; 
	window->add_tool(output = new PaletteOutput(this, x, y));
	output->create_objects();
	
	x += DP(230); 
	y = this->y;
	window->add_tool(new BC_Title(x, y, _("Hue"), SMALLFONT));
	y += DP(15);
	window->add_tool(hue = new PaletteHue(this, x, y));
	y += DP(30);
	window->add_tool(new BC_Title(x, y, _("Saturation"), SMALLFONT));
	y += DP(15);
	window->add_tool(saturation = new PaletteSaturation(this, x, y));
	y += DP(30);
	window->add_tool(new BC_Title(x, y, _("Value"), SMALLFONT));
	y += DP(15);
	window->add_tool(value = new PaletteValue(this, x, y));
	y += DP(30);
	window->add_tool(new BC_Title(x, y, _("Red"), SMALLFONT));
	y += DP(15);
	window->add_tool(red = new PaletteRed(this, x, y));
	y += DP(30);
	window->add_tool(new BC_Title(x, y, _("Green"), SMALLFONT));
	y += DP(15);
	window->add_tool(green = new PaletteGreen(this, x, y));
	y += DP(30);
	window->add_tool(new BC_Title(x, y, _("Blue"), SMALLFONT));
	y += DP(15);
	window->add_tool(blue = new PaletteBlue(this, x, y));

	if(do_alpha)
	{
		y += DP(30);
		window->add_tool(new BC_Title(x, y, _("Alpha"), SMALLFONT));
		y += DP(15);
		window->add_tool(alpha = new PaletteAlpha(this, x, y));
	}

}

void ColorObjects::handle_event()
{
}

void ColorObjects::update()
{
	float r, g, b;
	if(h < 0) h = 0;
	if(h > 360) h = 360;
	if(s < 0) s = 0;
	if(s > 1) s = 1;
	if(v < 0) v = 0;
	if(v > 1) v = 1;
	if(a < 0) a = 0;
	if(a > 1) a = 1;

	wheel->draw(wheel->oldhue, 
				wheel->oldsaturation);
	wheel->oldhue = h;
	wheel->oldsaturation = s;
	wheel->draw(h, s);
	wheel->flash();
	wheel_value->draw(h, s, v);
	wheel_value->flash();
	output->draw();
	output->flash();
	hue->update((int)h);
	saturation->update(s);
	value->update(v);

	HSV::hsv_to_rgb(r, g, b, h, s, v);
	red->update(r);
	green->update(g);
	blue->update(b);
	if(do_alpha)
	{
		alpha->update(a);
	}
}

void ColorObjects::update_rgb()
{
	HSV::rgb_to_hsv(red->get_value(), 
				green->get_value(), 
				blue->get_value(), 
				h, 
				s, 
				v);
	update();
}





ColorObjectsListener::ColorObjectsListener(ColorWindow *window, 
    ColorThread *thread,
    int x,
    int y)
 : ColorObjects(window, thread->do_alpha, x, y)
{
    this->window = window;
    this->thread = thread;
}

void ColorObjectsListener::handle_event()
{
	float r, g, b;
	HSV::hsv_to_rgb(r, g, b, h, s, v);
	int result = (((int)(r * 255)) << 16) | (((int)(g * 255)) << 8) | ((int)(b * 255));
	thread->handle_new_color(result, (int)(a * 255));
}






ColorWindow::ColorWindow(ColorThread *thread, int x, int y, char *title)
 : BC_Window(title, 
	x,
	y,
	DP(410), 
	DP(320), 
	DP(410), 
	DP(320), 
	0, 
	0,
	1)
{
	this->thread = thread;
}

void ColorWindow::create_objects()
{
	int x = DP(10), y = DP(10);
	
	lock_window("ColorWindow::create_objects");
    
    color_objects = new ColorObjectsListener(this, thread, x, y);
	update_values();
    color_objects->create_objects();

	update_display();
	show_window();
	unlock_window();
	return;
}


void ColorWindow::update_values()
 {
 	color_objects->r = (float)((thread->output & 0xff0000) >> 16) / 255;
 	color_objects->g = (float)((thread->output & 0xff00) >> 8) / 255;
 	color_objects->b = (float)((thread->output & 0xff)) / 255;
 	HSV::rgb_to_hsv(color_objects->r, 
        color_objects->g, 
        color_objects->b, 
        color_objects->h, 
        color_objects->s, 
        color_objects->v);
 	color_objects->a = (float)thread->alpha / 255;
}


int ColorWindow::close_event()
{
	set_done(0);
	return 1;
}


void ColorWindow::update_display()
{
	color_objects->update();
}






PaletteWheel::PaletteWheel(ColorObjects *objs, int x, int y)
 : BC_SubWindow(x, y, DP(170), DP(170))
{
	this->objs = objs;
	oldhue = 0;
	oldsaturation = 0;
	button_down = 0;
}
PaletteWheel::~PaletteWheel()
{
}

int PaletteWheel::button_press_event()
{
	if(get_cursor_x() >= 0 && get_cursor_x() < get_w() &&
		get_cursor_y() >= 0 && get_cursor_y() < get_h() && 
		is_event_win())
	{
		button_down = 1;
		cursor_motion_event();
		return 1;
	}
	return 0;
}

int PaletteWheel::cursor_motion_event()
{
	int x1, y1, distance;
	if(button_down && is_event_win())
	{
		objs->h = get_angle(get_w() / 2, 
			get_h() / 2, 
			get_cursor_x(), 
			get_cursor_y());
		x1 = get_w() / 2 - get_cursor_x();
		y1 = get_h() / 2 - get_cursor_y();
		distance = (int)sqrt(x1 * x1 + y1 * y1);
		if(distance > get_w() / 2) distance = get_w() / 2;
		objs->s = (float)distance / (get_w() / 2);
		objs->update();
		objs->handle_event();
		return 1;
	}
	return 0;
}

int PaletteWheel::button_release_event()
{
	if(button_down)
	{
		button_down = 0;
		return 1;
	}
	return 0;
}

void PaletteWheel::create_objects()
{
// Upper right
//printf("PaletteWheel::create_objects 1\n");
	float h;
	float s;
	float v = 1;
	float r, g, b;
	float x1, y1, x2, y2;
	float distance;
	int default_r, default_g, default_b;
	VFrame frame(0, -1, get_w(), get_h(), BC_RGBA8888, -1);
	x1 = get_w() / 2;
	y1 = get_h() / 2;
	default_r = (get_resources()->get_bg_color() & 0xff0000) >> 16;
	default_g = (get_resources()->get_bg_color() & 0xff00) >> 8;
	default_b = (get_resources()->get_bg_color() & 0xff);
//printf("PaletteWheel::create_objects 1\n");

	int highlight_r = (get_resources()->button_light & 0xff0000) >> 16;
	int highlight_g = (get_resources()->button_light & 0xff00) >> 8;
	int highlight_b = (get_resources()->button_light & 0xff);

	for(y2 = 0; y2 < get_h(); y2++)
	{
		unsigned char *row = (unsigned char*)frame.get_rows()[(int)y2];
		for(x2 = 0; x2 < get_w(); x2++)
		{
			distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
			if(distance > x1)
			{
				row[(int)x2 * 4] = default_r;
				row[(int)x2 * 4 + 1] = default_g;
				row[(int)x2 * 4 + 2] = default_b;
				row[(int)x2 * 4 + 3] = 0;
			}
			else
			if(distance > x1 - 1)
			{
				int r_i, g_i, b_i;
				if(get_h() - y2 < x2)
				{
					r_i = highlight_r;
					g_i = highlight_g;
					b_i = highlight_b;
				}
				else
				{
					r_i = 0;
					g_i = 0;
					b_i = 0;
				}

				row[(int)x2 * 4] = r_i;
				row[(int)x2 * 4 + 1] = g_i;
				row[(int)x2 * 4 + 2] = b_i;
				row[(int)x2 * 4 + 3] = 255;
			}
			else
			{
				h = get_angle(x1, y1, x2, y2);
				s = distance / x1;
				HSV::hsv_to_rgb(r, g, b, h, s, v);
				row[(int)x2 * 4] = (int)(r * 255);
				row[(int)x2 * 4 + 1] = (int)(g * 255);
				row[(int)x2 * 4 + 2] = (int)(b * 255);
				row[(int)x2 * 4 + 3] = 255;
			}
		}
	}
//printf("PaletteWheel::create_objects 1\n");

	draw_vframe(&frame, 
		0, 
		0, 
		get_w(), 
		get_h(), 
		0, 
		0, 
		get_w(), 
		get_h(), 
		0);
//printf("PaletteWheel::create_objects 1\n");

	oldhue = objs->h;
	oldsaturation = objs->s;
//printf("PaletteWheel::create_objects 1\n");
	draw(oldhue, oldsaturation);
//printf("PaletteWheel::create_objects 1\n");
	flash();
//printf("PaletteWheel::create_objects 2\n");
}

float PaletteWheel::torads(float angle)
{
	return (float)angle / 360 * 2 * M_PI;
}


int PaletteWheel::draw(float hue, float saturation)
{
	int x, y, w, h;
	w = get_w() / 2;
	h = get_h() / 2;

	if(hue > 0 && hue < 90)
	{
		x = (int)(w - w * cos(torads(90 - hue)) * saturation);
		y = (int)(h - h * sin(torads(90 - hue)) * saturation);
	}
	else
	if(hue > 90 && hue < 180)
	{
		x = (int)(w - w * cos(torads(hue - 90)) * saturation);
		y = (int)(h + h * sin(torads(hue - 90)) * saturation);
	}
	else
	if(hue > 180 && hue < 270)
	{
		x = (int)(w + w * cos(torads(270 - hue)) * saturation);
		y = (int)(h + h * sin(torads(270 - hue)) * saturation);
	}
	else
	if(hue > 270 && hue < 360)
	{
		x = (int)(w + w * cos(torads(hue - 270)) * saturation);
		y = (int)(h - w * sin(torads(hue - 270)) * saturation);
	}
	else
	if(hue == 0) 
	{
		x = w;
		y = (int)(h - h * saturation);
	}
	else
	if(hue == 90)
	{
		x = (int)(w - w * saturation);
		y = h;
	}
	else
	if(hue == 180)
	{
		x = w;
		y = (int)(h + h * saturation);
	}
	else
	if(hue == 270)
	{
		x = (int)(w + w * saturation);
		y = h;
	}

	set_inverse();
	set_color(WHITE);
	draw_circle(x - 5, y - 5, 10, 10);
	set_opaque();
	return 0;
}

int PaletteWheel::get_angle(float x1, float y1, float x2, float y2)
{
	float result = -atan2(x2 - x1, y1 - y2) * (360 / M_PI / 2);
	if (result < 0)
		result += 360;
	return (int)result;
}

PaletteWheelValue::PaletteWheelValue(ColorObjects *objs, int x, int y)
 : BC_SubWindow(x, y, DP(40), DP(170), BLACK)
{
	this->objs = objs;
	button_down = 0;
}
PaletteWheelValue::~PaletteWheelValue()
{
	delete frame;
}

void PaletteWheelValue::create_objects()
{
	frame = new VFrame(0, -1, get_w(), get_h(), BC_RGB888, -1);
	draw(objs->h, objs->s, objs->v);
	flash();
}

int PaletteWheelValue::button_press_event()
{
//printf("PaletteWheelValue::button_press 1 %d\n", is_event_win());
	if(get_cursor_x() >= 0 && get_cursor_x() < get_w() &&
		get_cursor_y() >= 0 && get_cursor_y() < get_h() && 
		is_event_win())
	{
//printf("PaletteWheelValue::button_press 2\n");
		button_down = 1;
		cursor_motion_event();
		return 1;
	}
	return 0;
}

int PaletteWheelValue::cursor_motion_event()
{
	int x1, y1, distance;
	if(button_down && is_event_win())
	{
//printf("PaletteWheelValue::cursor_motion 1\n");
		objs->v = 1.0 - (float)(get_cursor_y() - 2) / (get_h() - 4);
		objs->update();
		objs->handle_event();
		return 1;
	}
	return 0;
}

int PaletteWheelValue::button_release_event()
{
	if(button_down)
	{
//printf("PaletteWheelValue::button_release 1\n");
		button_down = 0;
		return 1;
	}
	return 0;
}

int PaletteWheelValue::draw(float hue, float saturation, float value)
{
	float r_f, g_f, b_f;
	int i, j, r, g, b;

	for(i = get_h() - 3; i >= 2; i--)
	{
		unsigned char *row = (unsigned char*)frame->get_rows()[i];
		HSV::hsv_to_rgb(r_f, 
			g_f, 
			b_f, 
			hue, 
			saturation, 
			1.0 - (float)(i - 2) / (get_h() - 4));
		r = (int)(r_f * 255);
		g = (int)(g_f * 255);
		b = (int)(b_f * 255);
		for(j = 0; j < get_w(); j++)
		{
 			row[j * 3] = r;
 			row[j * 3 + 1] = g;
 			row[j * 3 + 2] = b;
		}
	}

	draw_3d_border(0, 
		0, 
		get_w(), 
		get_h(), 
		1);
	draw_vframe(frame, 
		2, 
		2, 
		get_w() - 4, 
		get_h() - 4, 
		2, 
		2, 
		get_w() - 4, 
		get_h() - 4, 
		0);
	set_color(BLACK);
	draw_line(2, 
		get_h() - 3 - (int)(value * (get_h() - 5)), 
		get_w() - 3, 
		get_h() - 3 - (int)(value * (get_h() - 5)));
//printf("PaletteWheelValue::draw %d %f\n", __LINE__, value);

	return 0;
}

PaletteOutput::PaletteOutput(ColorObjects *objs, int x, int y)
 : BC_SubWindow(x, y, DP(180), DP(30), BLACK)
{
	this->objs = objs;
}
PaletteOutput::~PaletteOutput()
{
}


void PaletteOutput::create_objects()
{
	draw();
	flash();
}

int PaletteOutput::handle_event()
{
	return 1;
}

int PaletteOutput::draw()
{
	float r_f, g_f, b_f;
	
	HSV::hsv_to_rgb(r_f, g_f, b_f, objs->h, objs->s, objs->v);
	set_color(((int)(r_f * 255) << 16) | ((int)(g_f * 255) << 8) | ((int)(b_f * 255)));
	draw_box(2, 2, get_w() - 4, get_h() - 4);
	draw_3d_border(0, 
		0, 
		get_w(), 
		get_h(), 
		1);


	return 0;
}

#define SLIDER_W DP(160)

PaletteHue::PaletteHue(ColorObjects *objs, int x, int y)
 : BC_ISlider(x, y, 0, SLIDER_W, DP(200), 0, 359, (int)(objs->h), 0)
{
	this->objs = objs;
}
PaletteHue::~PaletteHue()
{
}

int PaletteHue::handle_event()
{
	objs->h = get_value();
	objs->update();
	objs->handle_event();
	return 1;
}

PaletteSaturation::PaletteSaturation(ColorObjects *objs, int x, int y)
 : BC_FSlider(x, y, 0, SLIDER_W, DP(200), 0, 1.0, objs->s, 0)
{
	this->objs = objs;
	set_precision(0.01);
}
PaletteSaturation::~PaletteSaturation()
{
}

int PaletteSaturation::handle_event()
{
//printf("PaletteSaturation::handle_event 1 %f\n", get_value());
	objs->s = get_value();
	objs->update();
//printf("PaletteSaturation::handle_event 2 %f\n", get_value());
	objs->handle_event();
	return 1;
}

PaletteValue::PaletteValue(ColorObjects *objs, int x, int y)
 : BC_FSlider(x, y, 0, SLIDER_W, DP(200), 0, 1.0, objs->v, 0)
{
	this->objs = objs;
	set_precision(0.01);
}
PaletteValue::~PaletteValue()
{
}

int PaletteValue::handle_event()
{
	objs->v = get_value();
	objs->update();
	objs->handle_event();
	return 1;
}


PaletteRed::PaletteRed(ColorObjects *objs, int x, int y)
 : BC_FSlider(x, y, 0, SLIDER_W, DP(200), 0, 1, objs->r, 0)
{
	this->objs = objs;
	set_precision(0.01);
}
PaletteRed::~PaletteRed()
{
}

int PaletteRed::handle_event()
{
	objs->update_rgb();
	objs->handle_event();
	return 1;
}

PaletteGreen::PaletteGreen(ColorObjects *objs, int x, int y)
 : BC_FSlider(x, y, 0, SLIDER_W, DP(200), 0, 1, objs->g, 0)
{
	this->objs = objs;
	set_precision(0.01);
}
PaletteGreen::~PaletteGreen()
{
}

int PaletteGreen::handle_event()
{
	objs->update_rgb();
	objs->handle_event();
	return 1;
}

PaletteBlue::PaletteBlue(ColorObjects *objs, int x, int y)
 : BC_FSlider(x, y, 0, SLIDER_W, DP(200), 0, 1, objs->b, 0)
{
	this->objs = objs;
	set_precision(0.01);
}
PaletteBlue::~PaletteBlue()
{
}

int PaletteBlue::handle_event()
{
	objs->update_rgb();
	objs->handle_event();
	return 1;
}

PaletteAlpha::PaletteAlpha(ColorObjects *objs, int x, int y)
 : BC_FSlider(x, y, 0, SLIDER_W, DP(200), 0, 1, objs->a, 0)
{
	this->objs = objs;
	set_precision(0.01);
}
PaletteAlpha::~PaletteAlpha()
{
}

int PaletteAlpha::handle_event()
{
	objs->a = get_value();
	objs->handle_event();
	return 1;
}


