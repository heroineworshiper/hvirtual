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

#include "bcdisplay.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "clip.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>

#define TEST_SIZE 128
#define TEST_SIZE2 164
#define TEST_SIZE3 196
int BC_DisplayInfo::top_border = -1;
int BC_DisplayInfo::left_border = -1;
int BC_DisplayInfo::bottom_border = -1;
int BC_DisplayInfo::right_border = -1;
int BC_DisplayInfo::auto_reposition_x = -1;
int BC_DisplayInfo::auto_reposition_y = -1;
int BC_DisplayInfo::dpi = 100;


BC_DisplayInfo::BC_DisplayInfo(const char *display_name, int show_error)
{
	init_window(display_name, show_error);
}

BC_DisplayInfo::~BC_DisplayInfo()
{
#ifndef SINGLE_THREAD
	XCloseDisplay(display);
#endif
}


void BC_DisplayInfo::parse_geometry(char *geom, int *x, int *y, int *width, int *height)
{
	XParseGeometry(geom, x, y, (unsigned int*)width, (unsigned int*)height);
}

void BC_DisplayInfo::test_window(int &x_out, 
	int &y_out, 
	int &x_out2, 
	int &y_out2, 
	int x_in, 
	int y_in)
{
#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::test_window");
#endif

	unsigned long mask = CWEventMask | CWWinGravity;
	XSetWindowAttributes attr;
	XSizeHints size_hints;

	x_out = 0;
	y_out = 0;
	x_out2 = 0;
	y_out2 = 0;
	attr.event_mask = StructureNotifyMask;
	attr.win_gravity = SouthEastGravity;
	Window win = XCreateWindow(display, 
			rootwin, 
			x_in, 
			y_in, 
			TEST_SIZE, 
			TEST_SIZE, 
			0, 
			default_depth, 
			InputOutput, 
			vis, 
			mask, 
			&attr);
	XGetNormalHints(display, win, &size_hints);
	size_hints.flags = PPosition | PSize;
	size_hints.x = x_in;
	size_hints.y = y_in;
	size_hints.width = TEST_SIZE;
	size_hints.height = TEST_SIZE;
	XSetStandardProperties(display, 
		win, 
		"x", 
		"x", 
		None, 
		0, 
		0, 
		&size_hints);

	XMapWindow(display, win); 
	XFlush(display);
	XSync(display, 0);

	XMoveResizeWindow(display, 
		win, 
		x_in, 
		y_in,
		TEST_SIZE2,
		TEST_SIZE2);
	XFlush(display);
	XSync(display, 0);

	XResizeWindow(display, 
		win, 
		TEST_SIZE3,
		TEST_SIZE3);
	XFlush(display);
	XSync(display, 0);

	XEvent event;
	int last_w = 0;
	int last_h = 0;
	int state = 0;


	do
	{
		XNextEvent(display, &event);
// printf("BC_DisplayInfo::test_window %d state=%d event=%d XPending=%d\n", 
// __LINE__, 
// state,
// event.type, 
// XPending(display));
		if(event.type == ConfigureNotify && event.xany.window == win)
		{
// printf("BC_DisplayInfo::test_window %d state=%d ConfigureNotify last_w=%d last_h=%d w=%d h=%d\n", 
// __LINE__,
// state,
// last_w,
// last_h,
// event.xconfigure.width,
// event.xconfigure.height);
// Get creation repositioning
// Should get an event for XCreateWindow, XMoveResizeWindow, XResizeWindow
// + a bunch of dupes.
			if(last_w != event.xconfigure.width || last_h != event.xconfigure.height)
			{
// didn't get the event for XCreateWindow
                if(state == 0 && event.xconfigure.width != TEST_SIZE)
                {
                    printf("BC_DisplayInfo::test_window %d dropped XCreateWindow\n", 
                        __LINE__);
                    state++;
                }

				state++;
				last_w = event.xconfigure.width;
				last_h = event.xconfigure.height;
			}

			if(state == 1)
			{
				x_out = MAX(event.xconfigure.x + event.xconfigure.border_width - x_in, x_out);
				y_out = MAX(event.xconfigure.y + event.xconfigure.border_width - y_in, y_out);
                if(x_out == 0 && y_out == 0)
                {
// assume no window manager & quit
printf("BC_DisplayInfo::test_window %d: No window manager\n", 
__LINE__);
                    x_out2 = 0;
                    y_out2 = 0;
                    state = 3;
                }
			}
			else
			if(state == 2)
// Get moveresize repositioning.  
// A window manager is required to get here.
// A bare X server will get stuck.
			{
				x_out2 = MAX(event.xconfigure.x + event.xconfigure.border_width - x_in, x_out2);
				y_out2 = MAX(event.xconfigure.y + event.xconfigure.border_width - y_in, y_out2);
			}
// printf("BC_DisplayInfo::test_window 2 state=%d x_out=%d y_out=%d x_in=%d y_in=%d w=%d h=%d\n",
// state,
// event.xconfigure.x + event.xconfigure.border_width, 
// event.xconfigure.y + event.xconfigure.border_width, 
// x_in, 
// y_in, 
// event.xconfigure.width, 
// event.xconfigure.height);
		}
 	}while(state != 3);


	XDestroyWindow(display, win);
	XFlush(display);
	XSync(display, 0);

	x_out = MAX(0, x_out);
	y_out = MAX(0, y_out);
	x_out = MIN(x_out, 30);
	y_out = MIN(y_out, 30);


#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
}

void BC_DisplayInfo::init_borders()
{
	if(top_border < 0)
	{

		test_window(left_border, 
			top_border, 
			auto_reposition_x, 
			auto_reposition_y, 
			0, 
			0);
		right_border = left_border;
		bottom_border = left_border;
// printf("BC_DisplayInfo::init_borders border=%d %d auto=%d %d\n", 
// left_border, 
// top_border, 
// auto_reposition_x, 
// auto_reposition_y);
	}
}


int BC_DisplayInfo::get_top_border()
{
	init_borders();
	return top_border;
}

int BC_DisplayInfo::get_left_border()
{
	init_borders();
	return left_border;
}

int BC_DisplayInfo::get_right_border()
{
	init_borders();
	return right_border;
}

int BC_DisplayInfo::get_bottom_border()
{
	init_borders();
	return bottom_border;
}

void BC_DisplayInfo::init_window(const char *display_name, int show_error)
{
	if(display_name && display_name[0] == 0) display_name = NULL;
//printf("BC_DisplayInfo::init_window %d %s\n", __LINE__, display_name);

#ifdef SINGLE_THREAD
	display = BC_Display::get_display(display_name);
#else
	
// This function must be the first Xlib
// function a multi-threaded program calls
	XInitThreads();

	if((display = XOpenDisplay(display_name)) == NULL)
	{
		if(show_error)
		{
  			printf("BC_DisplayInfo::init_window: cannot connect to X server.\n");
  			if(getenv("DISPLAY") == NULL)
    			printf("'DISPLAY' environment variable not set.\n");
			exit(1);
		}
		return;
 	}
#endif // SINGLE_THREAD

#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::init_window");
#endif

	screen = DefaultScreen(display);
	rootwin = RootWindow(display, screen);
	vis = DefaultVisual(display, screen);
	default_depth = DefaultDepth(display, screen);

	dpi = (int)(XDisplayWidth(display, screen) * 25.4 /
		XDisplayWidthMM(display, screen));


//	XDisplayHeight(display, screen);
//	XDisplayHeightMM(display, screen);
	
	
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif // SINGLE_THREAD
}


int BC_DisplayInfo::get_root_w()
{
#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::get_root_w");
#endif
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	int result = WidthOfScreen(screen_ptr);
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
	return result;
}

int BC_DisplayInfo::get_root_h()
{
#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::get_root_h");
#endif
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	int result = HeightOfScreen(screen_ptr);
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
	return result;
}

int BC_DisplayInfo::get_abs_cursor_x()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::get_abs_cursor_x");
#endif
	XQueryPointer(display, 
	   rootwin, 
	   &temp_win, 
	   &temp_win,
       &abs_x, 
	   &abs_y, 
	   &win_x, 
	   &win_y, 
	   &temp_mask);
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
	return abs_x;
}

int BC_DisplayInfo::get_abs_cursor_y()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_DisplayInfo::get_abs_cursor_y");
#endif
	XQueryPointer(display, 
	   rootwin, 
	   &temp_win, 
	   &temp_win,
       &abs_x, 
	   &abs_y, 
	   &win_x, 
	   &win_y, 
	   &temp_mask);
#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#endif
	return abs_y;
}




