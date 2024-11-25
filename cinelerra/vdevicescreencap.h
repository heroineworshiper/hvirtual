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

#ifndef VDEVICESCREENCAP_H
#define VDEVICESCREENCAP_H


#include "guicast.h"
#include "textengine.inc"
#include "vdevicebase.h"

class VDeviceScreencap : public VDeviceBase
{
public:
	VDeviceScreencap(VideoDevice *device);
	~VDeviceScreencap();

    int reset_parameters();
	int open_input();
	int close_all();
	int read_buffer(VFrame *frame);
// Get best colormodel for recording
	int get_best_colormodel(Asset *asset);
    void draw_pixel(uint8_t *dst, int color_model, int alpha, int r, int g, int b);
    void fill_rect(VFrame *mask, int x, int y, int w, int h);
    void fill_tri(VFrame *mask, int x, int y, int w, int h, int up);
    void draw_mouse(VFrame *mask, 
        int line_thickness,
        int outline_size);
    void draw_mask(VFrame *text,
        VFrame *outline,
        int x, 
        int y);

// windows which overlay the screencap area
#define SCREENCAP_BORDERS 4
#define SCREENCAP_PIXELS 5
#define SCREENCAP_COLOR BLACK
	BC_Popup *screencap_border[SCREENCAP_BORDERS];

// Screen capture
	BC_Capture *capture_bitmap;
    TextEngine *text_engine;
    TextEngine *mouse_text_engine;
    VFrame *mouse_mask;
    VFrame *mouse_outline;
};




#endif
