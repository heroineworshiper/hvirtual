
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

#ifndef COLORPICKER_H
#define COLORPICKER_H

#include "condition.inc"
#include "guicast.h"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"

class ColorWindow;
class PaletteWheel;
class PaletteWheelValue;
class PaletteOutput;
class PaletteHue;
class PaletteSaturation;
class PaletteValue;
class PaletteRed;
class PaletteGreen;
class PaletteBlue;
class PaletteAlpha;

class ColorThread : public Thread
{
public:
	ColorThread(int do_alpha = 0, char *title = 0);
	~ColorThread();


	void run();
	void start_window(int output, int alpha);
	virtual int handle_new_color(int output, int alpha);
	void update_gui(int output, int alpha);
	ColorWindow* get_gui();

private:
	friend class ColorObjectsListener;
	friend class ColorWindow;

	ColorWindow *window;
	Condition *completion;
// protects window, output, alpha
	Mutex *mutex;	
// Starting color
	int output;
	int alpha;
	int do_alpha;
	char *title;
};

// base class for adding color widgets to any window
class ColorObjects
{
public:
    ColorObjects(BC_Window *window, int do_alpha, int x, int y);
    virtual ~ColorObjects();
    
    static int calculate_w();
    static int calculate_h();
    
    void create_objects();
    virtual void handle_event();
// redraw with new values
    void update();
// convert HSV to new RGB & update
    void update_rgb();

    BC_Window *window;
	PaletteWheel *wheel;
	PaletteWheelValue *wheel_value;
	PaletteOutput *output;
	PaletteHue *hue;
	PaletteSaturation *saturation;
	PaletteValue *value;
	PaletteRed *red;
	PaletteGreen *green;
	PaletteBlue *blue;
	PaletteAlpha *alpha;
	VFrame *value_bitmap;

	float h, s, v, r, g, b, a;
    int do_alpha;
    int x, y;
};

// subclass for just the ColorWindow
class ColorObjectsListener : public ColorObjects
{
public:
    ColorObjectsListener(ColorWindow *window, 
        ColorThread *thread,
        int x,
        int y);
    void handle_event();
    
    ColorWindow *window;
    ColorThread *thread;
};


class ColorWindow : public BC_Window
{
public:
	ColorWindow(ColorThread *thread, int x, int y, char *title);

	void create_objects();
    static void create_color_objects(BC_Window *window);
	void update_values();
	int close_event();
	void update_display();
	void update_rgb();

	ColorThread *thread;
    ColorObjectsListener *color_objects;
};


class PaletteWheel : public BC_SubWindow
{
public:
	PaletteWheel(ColorObjects *objs, int x, int y);
	~PaletteWheel();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();

	void create_objects();
	int draw(float hue, float saturation);
	int get_angle(float x1, float y1, float x2, float y2);
	float torads(float angle);
	ColorObjects *objs;
	float oldhue;
	float oldsaturation;
	int button_down;
};

class PaletteWheelValue : public BC_SubWindow
{
public:
	PaletteWheelValue(ColorObjects *objs, int x, int y);
	~PaletteWheelValue();
	void create_objects();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	int draw(float hue, float saturation, float value);
	ColorObjects *objs;
	int button_down;
// Gradient
	VFrame *frame;
};

class PaletteOutput : public BC_SubWindow
{
public:
	PaletteOutput(ColorObjects *objs, int x, int y);
	~PaletteOutput();
	void create_objects();
	int handle_event();
	int draw();
	ColorObjects *objs;
};

class PaletteHue : public BC_ISlider
{
public:
	PaletteHue(ColorObjects *objs, int x, int y);
	~PaletteHue();
	int handle_event();
	ColorObjects *objs;
};

class PaletteSaturation : public BC_FSlider
{
public:
	PaletteSaturation(ColorObjects *objs, int x, int y);
	~PaletteSaturation();
	int handle_event();
	ColorObjects *objs;
};

class PaletteValue : public BC_FSlider
{
public:
	PaletteValue(ColorObjects *objs, int x, int y);
	~PaletteValue();
	int handle_event();
	ColorObjects *objs;
};

class PaletteRed : public BC_FSlider
{
public:
	PaletteRed(ColorObjects *objs, int x, int y);
	~PaletteRed();
	int handle_event();
	ColorObjects *objs;
};

class PaletteGreen : public BC_FSlider
{
public:
	PaletteGreen(ColorObjects *objs, int x, int y);
	~PaletteGreen();
	int handle_event();
	ColorObjects *objs;
};

class PaletteBlue : public BC_FSlider
{
public:
	PaletteBlue(ColorObjects *objs, int x, int y);
	~PaletteBlue();
	int handle_event();
	ColorObjects *objs;
};

class PaletteAlpha : public BC_FSlider
{
public:
	PaletteAlpha(ColorObjects *objs, int x, int y);
	~PaletteAlpha();
	int handle_event();
	ColorObjects *objs;
};

#endif
