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

#ifndef BCCAPTURE_H
#define BCCAPTURE_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

#include "bccapture.inc"
#include "bctimer.inc"
#include "bcwindowbase.inc"
#include "sizes.h"
//#include "thread.h"
#include "vframe.inc"

// global event reader
// class BC_CaptureThread : public Thread
// {
// public:
//     BC_CaptureThread(BC_Capture *capture);
//     ~BC_CaptureThread();
//     void run();
//     BC_Capture *capture;
// };


class KeypressState
{
public:
    KeypressState();
    void begin(int id, char *text, int is_button);
    void end(int id);
    void update(int elapsed);
    void reset();
// pressed during the last frame, regardless of the current up value
// immune to slow frame rates
    int pressed();

// -1 if not pressed. Original keysym if pressed
    int id;
// the text
    char text[BCTEXTLEN];
// times pressed since last up
    int times;
// ms remaneing to draw the button state
    int time;
// if the button is released
    int up;
};

class BC_Capture
{
public:
	BC_Capture(int w, int h, const char *display_path = "");
	virtual ~BC_Capture();

//    friend BC_CaptureThread;

	int init_window(const char *display_path);
// x1 and y1 are automatically adjusted if out of bounds
	int capture_frame(int &x1, 
		int &y1, 
		int cursor_size, // the scale of the cursor if nonzero
        int keypress_size);
	int get_w();
	int get_h();

	int w, h, default_depth;
	unsigned char **row_data;
	int bitmap_color_model;
    char key_text[BCTEXTLEN];
    int cursor_x;
    int cursor_y;
    int cursor_w;
    int cursor_h;

// keypress state
    KeypressState ctrl;
    KeypressState shift;
    KeypressState alt;
    KeypressState button;
    KeypressState key;

private:
	int allocate_data();
	int delete_data();
	int get_top_w();
	int get_top_h();
	
	inline void import_RGB565_to_RGB888(unsigned char* &output, unsigned char* &input)
	{
		*output++ = (*input & 0xf800) >> 8;
		*output++ = (*input & 0x7e0) >> 3;
		*output++ = (*input & 0x1f) << 3;

		input += 2;
		output += 3;
	};

	inline void import_BGR888_to_RGB888(unsigned char* &output, unsigned char* &input)
	{
		*output++ = input[2];
		*output++ = input[1];
		*output++ = input[0];

		input += 3;
		output += 3;
	};

	inline void import_BGR8888_to_RGB888(unsigned char* &output, unsigned char* &input)
	{
		*output++ = input[2];
		*output++ = input[1];
		*output++ = input[0];

		input += 4;
		output += 3;
	};

// measure the time since the last frame
    Timer *frame_time;

	int use_shm;
	unsigned char *data;
//    BC_CaptureThread *thread;
	XImage *ximage;
	XShmSegmentInfo shm_info;
	Display* display;
	Window rootwin;
	Visual *vis;
	int bits_per_pixel;
	int screen;
	long shm_event_type;
	int client_byte_order, server_byte_order;
};

#endif
