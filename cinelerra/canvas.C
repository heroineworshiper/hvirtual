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

#include "bcsignals.h"
#include "canvas.h"
#include "clip.h"
#include "cwindowgui.inc"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "preferences.h"
#include "recordconfig.h"
#include "recordmonitor.inc"
#include "theme.h"
#include "vframe.h"

#define FPS_X 0
#define FPS_Y 0

Canvas::Canvas(MWindow *mwindow,
	BC_WindowBase *subwindow, 
	int x, 
	int y, 
	int w, 
	int h,
	int output_w,
	int output_h,
	int use_scrollbars,
	int use_cwindow,
	int use_rwindow,
	int use_vwindow)
{
	reset();

	if(w < 10) w = 10;
	if(h < 10) h = 10;
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->output_w = output_w;
	this->output_h = output_h;
	this->use_scrollbars = use_scrollbars;
	this->use_cwindow = use_cwindow;
	this->use_rwindow = use_rwindow;
	this->use_vwindow = use_vwindow;
	this->root_w = subwindow->get_root_w(1, 0);
	this->root_h = subwindow->get_root_h(0);
	canvas_lock = new Mutex("Canvas::canvas_lock", 1);
}

Canvas::~Canvas()
{
	delete refresh_frame;
	delete canvas_menu;
 	delete yscroll;
 	delete xscroll;
    delete fps_subwindow;
    delete fps_fullscreen;
	delete canvas_subwindow;
	delete canvas_fullscreen;
	delete canvas_lock;
}

void Canvas::reset()
{
	use_scrollbars = 0;
	output_w = 0;
	output_h = 0;
    xscroll = 0;
    yscroll = 0;
	refresh_frame = 0;
    canvas_menu = 0;
	canvas_subwindow = 0;
	canvas_fullscreen = 0;
    fps_subwindow = 0;
    fps_fullscreen = 0;
	is_processing = 0;
	cursor_inside = 0;
}

void Canvas::lock_canvas(const char *location)
{
	canvas_lock->lock(location);
}

void Canvas::unlock_canvas()
{
	canvas_lock->unlock();
}

int Canvas::is_locked()
{
	return canvas_lock->is_locked();
}


BC_WindowBase* Canvas::get_canvas()
{
	if(get_fullscreen() && canvas_fullscreen) 
		return canvas_fullscreen;
	else
		return canvas_subwindow;
}

BC_WindowBase* Canvas::get_fps()
{
    if(get_fullscreen() && canvas_fullscreen)
        return fps_fullscreen;
    else
	    return fps_subwindow;
}


// Get dimensions given a zoom
void Canvas::calculate_sizes(float aspect_ratio, 
	int output_w, 
	int output_h, 
	float zoom, 
	int &w, 
	int &h)
{

// Horizontal stretch
	if((float)output_w / output_h <= aspect_ratio)
	{
		w = (int)((float)output_h * aspect_ratio * zoom);
		h = (int)((float)output_h * zoom);
	}
	else
// Vertical stretch
	{
		h = (int)((float)output_w / aspect_ratio * zoom);
		w = (int)((float)output_w * zoom);
	}
}

float Canvas::get_x_offset(EDL *edl, 
	int single_channel, 
	float zoom_x, 
	float conformed_w,
	float conformed_h)
{
	if(use_scrollbars)
	{
		if(xscroll) 
		{
// If the projection is smaller than the canvas, this forces it in the center.
//			if(conformed_w < w_visible)
//				return -(float)(w_visible - conformed_w) / 2;

			return (float)get_xscroll();
		}
		else
			return ((float)-get_canvas()->get_w() / zoom_x + 
				edl->session->output_w) / 2;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = get_canvas()->get_w();
		int canvas_h = get_canvas()->get_h();
		out_w = canvas_w;
		out_h = canvas_h;
		
		if((float)out_w / out_h > conformed_w / conformed_h)
		{
			out_w = (int)(out_h * conformed_w / conformed_h + 0.5);
		}
		
		if(out_w < canvas_w)
			return -(canvas_w - out_w) / 2 / zoom_x;
	}

	return 0;
}

float Canvas::get_y_offset(EDL *edl, 
	int single_channel, 
	float zoom_y, 
	float conformed_w,
	float conformed_h)
{
	if(use_scrollbars)
	{
		if(yscroll)
		{
// If the projection is smaller than the canvas, this forces it in the center.
//			if(conformed_h < h_visible)
//				return -(float)(h_visible - conformed_h) / 2;

			return (float)get_yscroll();
		}
		else
			return ((float)-get_canvas()->get_h() / zoom_y + 
				edl->session->output_h) / 2;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = get_canvas()->get_w();
		int canvas_h = get_canvas()->get_h();
		out_w = canvas_w;
		out_h = canvas_h;

		if((float)out_w / out_h <= conformed_w / conformed_h)
		{
			out_h = (int)((float)out_w / (conformed_w / conformed_h) + 0.5);
		}

//printf("Canvas::get_y_offset 1 %d %d %f\n", out_h, canvas_h, -((float)canvas_h - out_h) / 2);
		if(out_h < canvas_h)
			return -((float)canvas_h - out_h) / 2 / zoom_y;
	}

	return 0;
}

// This may not be used anymore
void Canvas::check_boundaries(EDL *edl, int &x, int &y, float &zoom)
{
	if(x + w_visible > w_needed) x = w_needed - w_visible;
	if(y + h_visible > h_needed) y = h_needed - h_visible;

	if(x < 0) x = 0;
	if(y < 0) y = 0;
}

void Canvas::update_scrollbars(int flush)
{
	if(use_scrollbars)
	{
		if(xscroll) xscroll->update_length(w_needed, get_xscroll(), w_visible, flush);
		if(yscroll) yscroll->update_length(h_needed, get_yscroll(), h_visible, flush);
	}
}

void Canvas::get_zooms(EDL *edl, 
	int single_channel, 
	float &zoom_x, 
	float &zoom_y,
	float &conformed_w,
	float &conformed_h)
{
	edl->calculate_conformed_dimensions(single_channel, 
		conformed_w, 
		conformed_h);

	if(use_scrollbars)
	{
		zoom_x = get_zoom() * 
			conformed_w / 
			edl->session->output_w;
		zoom_y = get_zoom() * 
			conformed_h / 
			edl->session->output_h;
	}
	else
	{
		int out_w, out_h;
		int canvas_w = get_canvas()->get_w();
		int canvas_h = get_canvas()->get_h();
	
		out_w = canvas_w;
		out_h = canvas_h;

		if((float)out_w / out_h > conformed_w / conformed_h)
		{
			out_w = (int)((float)out_h * conformed_w / conformed_h + 0.5);
		}
		else
		{
			out_h = (int)((float)out_w / (conformed_w / conformed_h) + 0.5);
		}

		zoom_x = (float)out_w / edl->session->output_w;
		zoom_y = (float)out_h / edl->session->output_h;
//printf("get zooms 2 %d %d %f %f\n", canvas_w, canvas_h, conformed_w, conformed_h);
	}
}

// Convert a coordinate on the canvas to a coordinate on the output
void Canvas::canvas_to_output(EDL *edl, int single_channel, float &x, float &y)
{
	float zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(edl, single_channel, zoom_x, zoom_y, conformed_w, conformed_h);

//printf("Canvas::canvas_to_output y=%f zoom_y=%f y_offset=%f\n", 
//	y, zoom_y, get_y_offset(edl, single_channel, zoom_y, conformed_w, conformed_h));

	x = (float)x / zoom_x + get_x_offset(edl, single_channel, zoom_x, conformed_w, conformed_h);
	y = (float)y / zoom_y + get_y_offset(edl, single_channel, zoom_y, conformed_w, conformed_h);
}

void Canvas::output_to_canvas(EDL *edl, int single_channel, float &x, float &y)
{
	float zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(edl, single_channel, zoom_x, zoom_y, conformed_w, conformed_h);

//printf("Canvas::output_to_canvas x=%f zoom_x=%f x_offset=%f\n", x, zoom_x, get_x_offset(edl, single_channel, zoom_x, conformed_w));

	x = (float)zoom_x * (x - get_x_offset(edl, single_channel, zoom_x, conformed_w, conformed_h));
	y = (float)zoom_y * (y - get_y_offset(edl, single_channel, zoom_y, conformed_w, conformed_h));
}



void Canvas::get_transfers(EDL *edl, 
	float &output_x1, 
	float &output_y1, 
	float &output_x2, 
	float &output_y2,
	float &canvas_x1, 
	float &canvas_y1, 
	float &canvas_x2, 
	float &canvas_y2,
	int canvas_w,
	int canvas_h)
{
// printf("Canvas::get_transfers %d canvas_w=%d canvas_h=%d\n", 
// __LINE__,  
// canvas_w, 
// canvas_h);

// automatic canvas size detection
	if(canvas_w < 0) canvas_w = get_canvas()->get_w();
	if(canvas_h < 0) canvas_h = get_canvas()->get_h();

// Canvas is zoomed to a portion of the output frame
// Not used in record monitor.
	if(use_scrollbars)
	{
		float in_x1, in_y1, in_x2, in_y2;
		float out_x1, out_y1, out_x2, out_y2;
		float zoom_x, zoom_y, conformed_w, conformed_h;

		get_zooms(edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
		out_x1 = 0;
		out_y1 = 0;
		out_x2 = canvas_w;
		out_y2 = canvas_h;
		in_x1 = 0;
		in_y1 = 0;
		in_x2 = canvas_w;
		in_y2 = canvas_h;

		canvas_to_output(edl, 0, in_x1, in_y1);
		canvas_to_output(edl, 0, in_x2, in_y2);

//printf("Canvas::get_transfers 1 %.0f %.0f %.0f %.0f -> %.0f %.0f %.0f %.0f\n",
//in_x1, in_y1, in_x2, in_y2, out_x1, out_y1, out_x2, out_y2);

		if(in_x1 < 0)
		{
			out_x1 += -in_x1 * zoom_x;
			in_x1 = 0;
		}

		if(in_y1 < 0)
		{
			out_y1 += -in_y1 * zoom_y;
			in_y1 = 0;
		}

		int output_w = get_output_w(edl);
		int output_h = get_output_h(edl);

		if(in_x2 > output_w)
		{
			out_x2 -= (in_x2 - output_w) * zoom_x;
			in_x2 = output_w;
		}

		if(in_y2 > output_h)
		{
			out_y2 -= (in_y2 - output_h) * zoom_y;
			in_y2 = output_h;
		}
// printf("Canvas::get_transfers 2 %.0f %.0f %.0f %.0f -> %.0f %.0f %.0f %.0f\n",
// 			in_x1, in_y1, in_x2, in_y2, out_x1, out_y1, out_x2, out_y2);

		output_x1 = in_x1;
		output_y1 = in_y1;
		output_x2 = in_x2;
		output_y2 = in_y2;
		canvas_x1 = out_x1;
		canvas_y1 = out_y1;
		canvas_x2 = out_x2;
		canvas_y2 = out_y2;

// Center on canvas
//		if(!scrollbars_exist())
//		{
//			out_x = canvas_w / 2 - out_w / 2;
//			out_y = canvas_h / 2 - out_h / 2;
//		}

	}
	else
// The output frame is normalized to the canvas
	{
// Default canvas coords fill the entire canvas
		canvas_x1 = 0;
		canvas_y1 = 0;
		canvas_x2 = canvas_w;
		canvas_y2 = canvas_h;

		if(edl)
		{
// Use EDL aspect ratio to shrink one of the canvas dimensions
			float out_w = canvas_x2 - canvas_x1;
			float out_h = canvas_y2 - canvas_y1;
            float aspect_ratio = edl->get_aspect_ratio();
//printf("Canvas::get_transfers %d aspect=%f\n", __LINE__, aspect_ratio);
//BC_Signals::dump_stack();

// always square pixels for recording.  See also RecordMonitorCanvas::zoom_resize_window
            if(use_rwindow && edl->session->auto_aspect)
            {
                aspect_ratio = (float)MWindow::preferences->vconfig_in->w / 
                    MWindow::preferences->vconfig_in->h;
            }

			if(out_w / out_h > aspect_ratio)
			{
				out_w = (int)(out_h * aspect_ratio + 0.5);
				canvas_x1 = canvas_w / 2 - out_w / 2;
			}
			else
			{
				out_h = (int)(out_w / aspect_ratio + 0.5);
				canvas_y1 = canvas_h / 2 - out_h / 2;
// printf("Canvas::get_transfers %d canvas_h=%d out_h=%f canvas_y1=%f\n",
// __LINE__,
// canvas_h,
// out_h,
// canvas_y1);
			}
			canvas_x2 = canvas_x1 + out_w;
			canvas_y2 = canvas_y1 + out_h;

// Get output frame coords from EDL
			output_x1 = 0;
			output_y1 = 0;
			output_x2 = get_output_w(edl);
			output_y2 = get_output_h(edl);
		}
		else
// No EDL to get aspect ratio or output frame coords from
		{
			output_x1 = 0;
			output_y1 = 0;
			output_x2 = this->output_w;
			output_y2 = this->output_h;
		}
	}

// Clamp to minimum value
	output_x1 = MAX(0, output_x1);
	output_y1 = MAX(0, output_y1);
	output_x2 = MAX(output_x1, output_x2);
	output_y2 = MAX(output_y1, output_y2);
	canvas_x1 = MAX(0, canvas_x1);
	canvas_y1 = MAX(0, canvas_y1);
	canvas_x2 = MAX(canvas_x1, canvas_x2);
	canvas_y2 = MAX(canvas_y1, canvas_y2);
// printf("Canvas::get_transfers %d %f,%f %f,%f -> %f,%f %f,%f\n",
// __LINE__,
// output_x1,
// output_y1,
// output_x2,
// output_y2,
// canvas_x1,
// canvas_y1,
// canvas_x2,
// canvas_y2);
}

void Canvas::clear()
{
	get_canvas()->clear_box(0, 
		0, 
		get_canvas()->get_w(), 
		get_canvas()->get_h());
    get_canvas()->flash(1);
}

void Canvas::draw_refresh(int flush)
{
    draw_refresh2(flush, 0);
}

void Canvas::draw_refresh2(int flush, EDL *edl)
{
// printf("Canvas::draw_refresh2 %d refresh_frame=%p edl=%p\n", 
// __LINE__, 
// refresh_frame,
// edl);
    float in_x1, in_y1, in_x2, in_y2;
    float out_x1, out_y1, out_x2, out_y2;

// default to the mane EDL
    if(!edl)
    {
        if(mwindow) 
            edl = mwindow->edl;
        else
        {
            printf("Canvas::draw_refresh %d no mwindow & no EDL\n", __LINE__);
            return;
        }
    }
    
    if(!refresh_frame /* || !mwindow */)
    {
        return;
    }
    
    
    get_transfers(
        //mwindow->edl, 
        edl,
	    in_x1, 
	    in_y1, 
	    in_x2, 
	    in_y2, 
	    out_x1, 
	    out_y1, 
	    out_x2, 
	    out_y2);


	if(!EQUIV(out_x1, 0) ||
		!EQUIV(out_y1, 0) ||
		!EQUIV(out_x2, get_canvas()->get_w()) ||
		!EQUIV(out_y2, get_canvas()->get_h()))
	{
		get_canvas()->clear_box(0, 
			0, 
			get_canvas()->get_w(), 
			get_canvas()->get_h());
	}
    
    
	if(out_x2 > out_x1 && 
		out_y2 > out_y1 && 
		in_x2 > in_x1 && 
		in_y2 > in_y1)
	{
// Can't use OpenGL here because it is called asynchronously of the
// playback operation.
		int dest_x = (int)out_x1; 
		int dest_y = (int)out_y1;
		int dest_w = (int)(out_x2 - out_x1);
		int dest_h = (int)(out_y2 - out_y1);
		int src_x = (int)in_x1;
		int src_y = (int)in_y1;
		int src_w = (int)(in_x2 - in_x1);
		int src_h = (int)(in_y2 - in_y1);
        if(cmodel_has_alpha(refresh_frame->get_color_model()))
        {
	        if(dest_w <= 0) dest_w = refresh_frame->get_w() - src_x;
	        if(dest_h <= 0) dest_h = refresh_frame->get_h() - src_y;
	        if(src_w <= 0) src_w = refresh_frame->get_w() - src_x;
	        if(src_h <= 0) src_h = refresh_frame->get_h() - src_y;
	        CLAMP(src_x, 0, refresh_frame->get_w() - 1);
	        CLAMP(src_y, 0, refresh_frame->get_h() - 1);
	        if(src_x + src_w > refresh_frame->get_w()) src_w = refresh_frame->get_w() - src_x;
	        if(src_y + src_h > refresh_frame->get_h()) src_h = refresh_frame->get_h() - src_y;

            BC_Bitmap *temp_bitmap = get_canvas()->get_temp_bitmap(
                dest_w,
                dest_h,
                get_canvas()->get_color_model());
            int checker_w = CHECKER_W;
            int checker_h = CHECKER_H;
// printf("Canvas::draw_refresh %d temp_bitmap=%d refresh_frame=%d rows=%p src=%d,%d %dx%d dst=%d,%d %dx%d\n", 
// __LINE__,
// temp_bitmap->get_color_model(),
// refresh_frame->get_color_model(),
// refresh_frame->get_rows()[0],
// src_x, 
// src_y, 
// src_w, 
// src_h,
// dest_x,
// dest_y,
// dest_w,
// dest_h);
// fflush(stdout);

            cmodel_transfer_alpha(temp_bitmap->get_row_pointers(), 
			    refresh_frame->get_rows(),
			    src_x, 
			    src_y, 
			    src_w, 
			    src_h,
			    0, 
			    0, 
			    dest_w, 
			    dest_h,
			    refresh_frame->get_color_model(), 
			    temp_bitmap->get_color_model(),
                refresh_frame->get_bytes_per_line(),
			    temp_bitmap->get_bytes_per_line(),
                checker_w,
                checker_h);

            get_canvas()->draw_bitmap(temp_bitmap, 
		        0, 
		        dest_x, 
		        dest_y,
		        dest_w,
		        dest_h,
		        0,
		        0,
		        -1,
		        -1,
		        0);

        }
        else
        {
//printf("Canvas::draw_refresh %d\n", __LINE__);
			get_canvas()->draw_vframe(refresh_frame,
                dest_x,
                dest_y,
                dest_w,
                dest_h,
                src_x,
                src_y,
                src_w,
                src_h,
				0);
        }
	}
}







int Canvas::scrollbars_exist()
{
	return(use_scrollbars && (xscroll || yscroll));
}

int Canvas::get_output_w(EDL *edl)
{
	if(use_scrollbars)
		return edl->session->output_w;
	else
		return edl->session->output_w;
}

int Canvas::get_output_h(EDL *edl)
{
	if(edl)
	{
		if(use_scrollbars)
			return edl->session->output_h;
		else
			return edl->session->output_h;
	}
    return 0;
}



void Canvas::get_scrollbars(EDL *edl, 
	int &canvas_x, 
	int &canvas_y, 
	int &canvas_w, 
	int &canvas_h)
{
	int need_xscroll = 0;
	int need_yscroll = 0;
//	int done = 0;
	float zoom_x, zoom_y, conformed_w, conformed_h;

	if(edl)
	{
		w_needed = edl->session->output_w;
		h_needed = edl->session->output_h;
		w_visible = w_needed;
		h_visible = h_needed;
	}
//printf("Canvas::get_scrollbars 1 %d %d\n", get_xscroll(), get_yscroll());

	if(use_scrollbars)
	{
		w_needed = edl->session->output_w;
		h_needed = edl->session->output_h;
		get_zooms(edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
//printf("Canvas::get_scrollbars 2 %d %d\n", get_xscroll(), get_yscroll());

//		while(!done)
//		{
			w_visible = (int)(canvas_w / zoom_x);
			h_visible = (int)(canvas_h / zoom_y);
//			done = 1;

//			if(w_needed > w_visible)
			if(1)
			{
				if(!need_xscroll)
				{
					need_xscroll = 1;
					canvas_h -= BC_ScrollBar::get_span(SCROLL_HORIZ);
//					done = 0;
				}
			}
			else
				need_xscroll = 0;

//			if(h_needed > h_visible)
			if(1)
			{
				if(!need_yscroll)
				{
					need_yscroll = 1;
					canvas_w -= BC_ScrollBar::get_span(SCROLL_VERT);
//					done = 0;
				}
			}
			else
				need_yscroll = 0;
//		}
//printf("Canvas::get_scrollbars %d %d %d %d %d %d\n", canvas_w, canvas_h, w_needed, h_needed, w_visible, h_visible);
//printf("Canvas::get_scrollbars 3 %d %d\n", get_xscroll(), get_yscroll());

		w_visible = (int)(canvas_w / zoom_x);
		h_visible = (int)(canvas_h / zoom_y);
	}

	if(need_xscroll)
	{
		if(!xscroll)
		{
			subwindow->add_subwindow(xscroll = new CanvasXScroll(edl, 
				this, 
                canvas_x,
                canvas_y + canvas_h,
				w_needed,
				get_xscroll(),
				w_visible,
				canvas_w));
			xscroll->show_window(0);
		}
		else
			xscroll->reposition_window(canvas_x, canvas_y + canvas_h, canvas_w);

		if(xscroll->get_length() != w_needed ||
			xscroll->get_handlelength() != w_visible)
			xscroll->update_length(w_needed, get_xscroll(), w_visible, 0);
	}
	else
	{
		if(xscroll) delete xscroll;
		xscroll = 0;
	}

	if(need_yscroll)
	{
		if(!yscroll)
		{
			subwindow->add_subwindow(yscroll = new CanvasYScroll(edl, 
				this,
                canvas_x + canvas_w,
                canvas_y,
				h_needed,
				get_yscroll(),
				h_visible,
				canvas_h));
			yscroll->show_window(0);
		}
		else
			yscroll->reposition_window(canvas_x + canvas_w, canvas_y, canvas_h);

		if(yscroll->get_length() != edl->session->output_h ||
			yscroll->get_handlelength() != h_visible)
			yscroll->update_length(h_needed, get_yscroll(), h_visible, 0);
	}
	else
	{
		if(yscroll) delete yscroll;
		yscroll = 0;
	}
//printf("Canvas::get_scrollbars 5 %d %d\n", get_xscroll(), get_yscroll());
}

void Canvas::reposition_window(EDL *edl, int x, int y, int w, int h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	view_x = x;
	view_y = y;
	view_w = w;
	view_h = h;
//printf("Canvas::reposition_window 1\n");
	get_scrollbars(edl, view_x, view_y, view_w, view_h);
//printf("Canvas::reposition_window %d %d %d %d\n", view_x, view_y, view_w, view_h);
	if(canvas_subwindow)
	{
		canvas_subwindow->reposition_window(view_x, view_y, view_w, view_h);

// Need to clear out the garbage in the back
		if(canvas_subwindow->get_video_on())
		{
			canvas_subwindow->set_color(BLACK);
			canvas_subwindow->draw_box(0, 
				0, 
				get_canvas()->get_w(), 
				get_canvas()->get_h());
			canvas_subwindow->flash(0);
		}
	}
	

	draw_refresh(0);
}

void Canvas::set_cursor(int cursor)
{
	get_canvas()->set_cursor(cursor, 0, 1);
}

int Canvas::get_cursor_x()
{
	return get_canvas()->get_cursor_x();
}

int Canvas::get_cursor_y()
{
	return get_canvas()->get_cursor_y();
}

int Canvas::get_buttonpress()
{
	return get_canvas()->get_buttonpress();
}


void Canvas::create_objects(EDL *edl)
{
	view_x = x;
	view_y = y;
	view_w = w;
	view_h = h;
	if(mwindow && edl) 
        get_scrollbars(edl, view_x, view_y, view_w, view_h);

	subwindow->unlock_window();
	create_canvas(0);
	subwindow->lock_window("Canvas::create_objects");

    if(mwindow)
    {
    	subwindow->add_subwindow(canvas_menu = new CanvasPopup(this));
	    canvas_menu->create_objects();

    	subwindow->add_subwindow(fullscreen_menu = new CanvasFullScreenPopup(this));
	    fullscreen_menu->create_objects();
    }
}

int Canvas::button_press_event()
{
	int result = 0;

	if(get_canvas()->get_buttonpress() == 3)
	{
//printf("Canvas::button_press_event %d %p %p\n", __LINE__, fullscreen_menu, canvas_menu);
		if(get_fullscreen())
        {
            if(fullscreen_menu)
            {
                if(fullscreen_menu->show_fps)
                    fullscreen_menu->show_fps->set_text(
                        CanvasFPS::calculate_text(MWindow::preferences->show_fps));
			    fullscreen_menu->activate_menu();
            }
		}
        else
		{
            if(canvas_menu)
            {
                if(canvas_menu->show_fps)
                    canvas_menu->show_fps->set_text(
                       CanvasFPS::calculate_text(MWindow::preferences->show_fps));
        	    canvas_menu->activate_menu();
            }
		}
        
        result = 1;
	}

	return result;
}

void Canvas::start_single()
{
	is_processing = 1;
	status_event();
}

void Canvas::stop_single()
{
	is_processing = 0;
	status_event();
}

void Canvas::start_video()
{
	is_processing = 1;
	if(get_canvas())
	{
		get_canvas()->start_video();
		status_event();
	}
}

void Canvas::stop_video()
{
	is_processing = 0;
	if(get_canvas())
	{
		get_canvas()->stop_video();
		status_event();
	}
}


void Canvas::start_fullscreen()
{
	set_fullscreen(1);
	create_canvas(1);
}

void Canvas::stop_fullscreen()
{
	set_fullscreen(0);
	create_canvas(1);
}

void Canvas::create_canvas(int flush)
{
	int video_on = 0;
	lock_canvas("Canvas::create_canvas");

    int margin = BC_Resources::theme->widget_border;

	if(!get_fullscreen())
// Enter windowed
	{
		if(canvas_fullscreen)
		{
			video_on = canvas_fullscreen->get_video_on();
			canvas_fullscreen->stop_video();
			canvas_fullscreen->lock_window("Canvas::create_canvas 2");
			canvas_fullscreen->hide_window();
			canvas_fullscreen->unlock_window();
		}


		if(!canvas_subwindow)
		{
			subwindow->add_subwindow(canvas_subwindow = new CanvasOutput(this, 
				view_x, 
				view_y, 
				view_w, 
				view_h));
	        if(use_cwindow)
            {
                canvas_subwindow->add_subwindow(fps_subwindow = new BC_SubWindow(
                    FPS_X,
                    FPS_Y,
                    canvas_subwindow->get_text_width(MEDIUMFONT, "000.0000") + margin * 2,
                    canvas_subwindow->get_text_ascent(MEDIUMFONT) + margin * 2));
            }
		}

	}
	else
// Enter fullscreen
	{

		if(canvas_subwindow)
		{
			video_on = canvas_subwindow->get_video_on();
			canvas_subwindow->stop_video();
		}


		if(!canvas_fullscreen)
		{
			canvas_fullscreen = new CanvasFullScreen(this,
        		root_w,
        		root_h);
	        if(use_cwindow)
            {
                canvas_fullscreen->add_subwindow(fps_fullscreen = new BC_SubWindow(
                    FPS_X,
                    FPS_Y,
                    canvas_fullscreen->get_text_width(MEDIUMFONT, "000.0000") + margin * 2,
                    canvas_fullscreen->get_text_ascent(MEDIUMFONT) + margin * 2));
                fps_fullscreen->show_window(1);
            } 
		}
		else
		{
			canvas_fullscreen->reposition_window(subwindow->get_root_x(0), 
				subwindow->get_root_y(0));
			canvas_fullscreen->show_window(0);
			canvas_fullscreen->raise_window(1);
		}

	}

	if(!video_on)
	{
		get_canvas()->lock_window("Canvas::create_canvas 1");
		draw_refresh(flush);
		get_canvas()->unlock_window();
	}

	if(video_on) get_canvas()->start_video();

    if(use_cwindow)
    {
        if(!MWindow::preferences->show_fps)
        {
            get_fps()->reposition_window(get_fps()->get_x(), -get_fps()->get_h());
        }
        else
        {
            get_fps()->reposition_window(get_fps()->get_x(), FPS_Y);
        }
    }

	unlock_canvas();
}

void Canvas::toggle_fps()
{
    if(use_cwindow)
    {
        MWindow::preferences->show_fps = !MWindow::preferences->show_fps;
        if(get_fps())
        {
            if(!MWindow::preferences->show_fps)
            {
                get_fps()->reposition_window(get_fps()->get_x(), -get_fps()->get_h());
            }
            else
            {
                get_fps()->reposition_window(get_fps()->get_x(), FPS_Y);
            }
        }
    }
}

int Canvas::cursor_leave_event_base(BC_WindowBase *caller)
{
	int result = 0;
	if(cursor_inside) result = cursor_leave_event();
	cursor_inside = 0;
	return result;
}

int Canvas::cursor_enter_event_base(BC_WindowBase *caller)
{
	int result = 0;
	if(caller->is_event_win() && caller->cursor_inside())
	{
		cursor_inside = 1;
		result = cursor_enter_event();
	}
	return result;
}

int Canvas::button_press_event_base(BC_WindowBase *caller)
{
	if(caller->is_event_win() && caller->cursor_inside())
	{
		return button_press_event();
	}
	return 0;
}

int Canvas::keypress_event(BC_WindowBase *caller)
{
	int caller_is_canvas = (caller == get_canvas());
	if(caller->get_keypress() == 'f')
	{
		caller->unlock_window();
		if(get_fullscreen())
			stop_fullscreen();
		else
			start_fullscreen();
		if(!caller_is_canvas) caller->lock_window("Canvas::keypress_event 1");
		return 1;
	}
	else
	if(caller->get_keypress() == ESC)
	{
		caller->unlock_window();
		if(get_fullscreen())
			stop_fullscreen();
		if(!caller_is_canvas) caller->lock_window("Canvas::keypress_event 2");
		return 1;
	}
	return 0;
}

















CanvasOutput::CanvasOutput(Canvas *canvas,
    int x,
    int y,
    int w,
    int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->canvas = canvas;
}

CanvasOutput::~CanvasOutput()
{
}

int CanvasOutput::cursor_leave_event()
{
	return canvas->cursor_leave_event_base(this);
}

int CanvasOutput::cursor_enter_event()
{
	return canvas->cursor_enter_event_base(this);
}

int CanvasOutput::button_press_event()
{
	return canvas->button_press_event_base(this);
}

int CanvasOutput::button_release_event()
{
	return canvas->button_release_event();
}

int CanvasOutput::cursor_motion_event()
{
	return canvas->cursor_motion_event();
}

int CanvasOutput::keypress_event()
{
	return canvas->keypress_event(this);
}













CanvasFullScreen::CanvasFullScreen(Canvas *canvas,
    int w,
    int h)
 : BC_FullScreen(canvas->subwindow, 
 	w, 
	h, 
	BLACK,
	0,
	0,
	0)
{
	this->canvas = canvas;
}

CanvasFullScreen::~CanvasFullScreen()
{
}













CanvasXScroll::CanvasXScroll(EDL *edl, 
	Canvas *canvas, 
    int x, 
    int y, 
	int length, 
	int position, 
	int handle_length,
    int pixels)
 : BC_ScrollBar(x, 
		y, 
		SCROLL_HORIZ, 
		pixels, 
		length, 
		position, 
		handle_length)
{
	this->canvas = canvas;
}

CanvasXScroll::~CanvasXScroll()
{
}

int CanvasXScroll::handle_event()
{
//printf("CanvasXScroll::handle_event %d %d %d\n", get_length(), get_value(), get_handlelength());
	canvas->update_zoom(get_value(), canvas->get_yscroll(), canvas->get_zoom());
	canvas->draw_refresh(1);
	return 1;
}






CanvasYScroll::CanvasYScroll(EDL *edl, 
	Canvas *canvas, 
    int x, 
    int y, 
	int length, 
	int position, 
	int handle_length,
    int pixels)
 : BC_ScrollBar(x, 
		y, 
		SCROLL_VERT, 
		pixels, 
		length, 
		position, 
		handle_length)
{
	this->canvas = canvas;
}

CanvasYScroll::~CanvasYScroll()
{
}

int CanvasYScroll::handle_event()
{
//printf("CanvasYScroll::handle_event %d %d\n", get_value(), get_length());
	canvas->update_zoom(canvas->get_xscroll(), get_value(), canvas->get_zoom());
	canvas->draw_refresh(1);
	return 1;
}






CanvasFullScreenPopup::CanvasFullScreenPopup(Canvas *canvas)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->canvas = canvas;
    show_fps = 0;
}


void CanvasFullScreenPopup::create_objects()
{
	if(canvas->use_cwindow)
    {
        add_item(new CanvasPopupAuto(canvas));
        add_item(show_fps = new CanvasFPS(canvas));
    }
	add_item(new CanvasSubWindowItem(canvas));
}


CanvasSubWindowItem::CanvasSubWindowItem(Canvas *canvas)
 : BC_MenuItem(_("Windowed"), "f", 'f')
{
	this->canvas = canvas;
}

int CanvasSubWindowItem::handle_event()
{
// It isn't a problem to delete the canvas from in here because the event
// dispatcher is the canvas subwindow.
	canvas->subwindow->unlock_window();
	canvas->stop_fullscreen();
	canvas->subwindow->lock_window("CanvasSubWindowItem::handle_event");
	return 1;
}








CanvasPopup::CanvasPopup(Canvas *canvas)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->canvas = canvas;
    toggle_controls = 0;
    show_fps = 0;
}

CanvasPopup::~CanvasPopup()
{
}

void CanvasPopup::create_objects()
{
	add_item(new CanvasPopupSize(canvas, _("Zoom 25%"), 0.25));
	add_item(new CanvasPopupSize(canvas, _("Zoom 33%"), 0.33));
	add_item(new CanvasPopupSize(canvas, _("Zoom 50%"), 0.5));
	add_item(new CanvasPopupSize(canvas, _("Zoom 75%"), 0.75));
	add_item(new CanvasPopupSize(canvas, _("Zoom 100%"), 1.0));
	add_item(new CanvasPopupSize(canvas, _("Zoom 150%"), 1.5));
	add_item(new CanvasPopupSize(canvas, _("Zoom 200%"), 2.0));
	add_item(new CanvasPopupSize(canvas, _("Zoom 300%"), 3.0));
	add_item(new CanvasPopupSize(canvas, _("Zoom 400%"), 4.0));
	if(canvas->use_cwindow)
	{
		add_item(new CanvasPopupAuto(canvas));
        add_item(new BC_MenuItem("-"));
		add_item(new CanvasPopupResetCamera(canvas));
		add_item(new CanvasPopupResetProjector(canvas));
		add_item(toggle_controls = new CanvasToggleControls(canvas));
        add_item(show_fps = new CanvasFPS(canvas));
	}
	if(canvas->use_rwindow)
	{
        add_item(new BC_MenuItem("-"));
//		add_item(new CanvasPopupResetTranslation(canvas));
		add_item(new CanvasPresetTranslation(canvas, TOP_LEFT, "Top left"));
		add_item(new CanvasPresetTranslation(canvas, TOP_RIGHT, "Top right"));
		add_item(new CanvasPresetTranslation(canvas, BOTTOM_LEFT, "Bottom left"));
		add_item(new CanvasPresetTranslation(canvas, BOTTOM_RIGHT, "Bottom right"));
	}
	if(canvas->use_vwindow)
	{
        add_item(new BC_MenuItem("-"));
		add_item(new CanvasPopupRemoveSource(canvas));
	}
	add_item(new CanvasFullScreenItem(canvas));
}



CanvasPopupAuto::CanvasPopupAuto(Canvas *canvas)
 : BC_MenuItem(_("Zoom Auto"))
{
	this->canvas = canvas;
}

int CanvasPopupAuto::handle_event()
{
	canvas->zoom_auto();
	return 1;
}


CanvasPopupSize::CanvasPopupSize(Canvas *canvas, char *text, float percentage)
 : BC_MenuItem(text)
{
	this->canvas = canvas;
	this->percentage = percentage;
}
CanvasPopupSize::~CanvasPopupSize()
{
}
int CanvasPopupSize::handle_event()
{
	canvas->zoom_resize_window(percentage);
	return 1;
}



CanvasPopupResetCamera::CanvasPopupResetCamera(Canvas *canvas)
 : BC_MenuItem(_("Reset camera"))
{
	this->canvas = canvas;
}
int CanvasPopupResetCamera::handle_event()
{
	canvas->reset_camera();
	return 1;
}



CanvasPopupResetProjector::CanvasPopupResetProjector(Canvas *canvas)
 : BC_MenuItem(_("Reset projector"))
{
	this->canvas = canvas;
}
int CanvasPopupResetProjector::handle_event()
{
	canvas->reset_projector();
	return 1;
}



CanvasPopupResetTranslation::CanvasPopupResetTranslation(Canvas *canvas)
 : BC_MenuItem(_("Reset translation"))
{
	this->canvas = canvas;
}
int CanvasPopupResetTranslation::handle_event()
{
	canvas->reset_translation();
	return 1;
}

CanvasPresetTranslation::CanvasPresetTranslation(Canvas *canvas, 
    int position, 
    const char *text)
 : BC_MenuItem(_(text))
{
	this->canvas = canvas;
    this->position = position;
}
int CanvasPresetTranslation::handle_event()
{
	canvas->preset_translation(position);
	return 1;
}



CanvasToggleControls::CanvasToggleControls(Canvas *canvas)
 : BC_MenuItem(calculate_text(canvas->get_cwindow_controls()))
{
	this->canvas = canvas;
}
int CanvasToggleControls::handle_event()
{
	canvas->toggle_controls();
	set_text(calculate_text(canvas->get_cwindow_controls()));
	return 1;
}

char* CanvasToggleControls::calculate_text(int cwindow_controls)
{
	if(!cwindow_controls) 
		return _("Show controls");
	else
		return _("Hide controls");
}







CanvasFullScreenItem::CanvasFullScreenItem(Canvas *canvas)
 : BC_MenuItem(_("Fullscreen"), "f", 'f')
{
	this->canvas = canvas;
}
int CanvasFullScreenItem::handle_event()
{
	canvas->subwindow->unlock_window();
	canvas->start_fullscreen();
	canvas->subwindow->lock_window("CanvasFullScreenItem::handle_event");
	return 1;
}


CanvasFPS::CanvasFPS(Canvas *canvas)
 : BC_MenuItem(calculate_text(MWindow::preferences->show_fps))
{
	this->canvas = canvas;
}
int CanvasFPS::handle_event()
{
	canvas->toggle_fps();
	set_text(calculate_text(MWindow::preferences->show_fps));
	return 1;
}
char* CanvasFPS::calculate_text(int value)
{
	if(!value) 
		return _("Show FPS");
	else
		return _("Hide FPS");
}









CanvasPopupRemoveSource::CanvasPopupRemoveSource(Canvas *canvas)
 : BC_MenuItem(_("Close source"))
{
	this->canvas = canvas;
}
int CanvasPopupRemoveSource::handle_event()
{
	canvas->close_source();
	return 1;
}


