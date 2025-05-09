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

#include "assets.h"
#include "bccapture.h"
//#include "bccmodels.h"
#include "bcsignals.h"
#include "canvas.h"
#include "colormodels.h"
#include "cwindowgui.inc"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playback3d.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"
#include "strategies.inc"
#include "theme.h"
#include "vdevicex11.h"
#include "vframe.h"
#include "videodevice.h"
#include "videowindow.h"
#include "videowindowgui.h"

#include <string.h>
#include <unistd.h>

VDeviceX11::VDeviceX11(VideoDevice *device, Canvas *canvas)
 : VDeviceBase(device)
{
	reset_parameters();
	this->canvas = canvas;
}

VDeviceX11::~VDeviceX11()
{
	close_all();
}

int VDeviceX11::reset_parameters()
{
    is_open = 0;
    canvas = 0;
	output_frame = 0;
	window_id = 0;
	bitmap = 0;
	bitmap_type = 0;
	bitmap_w = 0;
	bitmap_h = 0;
	output_x1 = 0;
	output_y1 = 0;
	output_x2 = 0;
	output_y2 = 0;
	canvas_x1 = 0;
	canvas_y1 = 0;
	canvas_x2 = 0;
	canvas_y2 = 0;
	color_model_selected = 0;
	is_cleared = 0;
    is_rendering = 0;

	return 0;
}


int VDeviceX11::open_output()
{
//printf("VDeviceX11::open_output %d canvas=%p\n", __LINE__, canvas);
	if(canvas)
	{
        if(canvas->is_processing)
        {
            printf("VDeviceX11::open_output %d device in use\n", __LINE__);
            return 1;
        }


		canvas->lock_canvas("VDeviceX11::open_output");
		canvas->get_canvas()->lock_window("VDeviceX11::open_output");
		if(!device->single_frame)
		{
//printf("VDeviceX11::open_output %d canvas=%p\n", __LINE__, canvas);
        	canvas->start_video();
		}
        else
		{
        	canvas->start_single();
		}
        canvas->get_canvas()->unlock_window();

// Enable opengl in the first routine that needs it, to reduce the complexity.
		canvas->unlock_canvas();
        is_open = 1;
	}
	return 0;
}


int VDeviceX11::output_visible()
{
	if(!canvas) return 0;

	canvas->lock_canvas("VDeviceX11::output_visible");
	if(canvas->get_canvas()->get_hidden()) 
	{
		canvas->unlock_canvas();
		return 0; 
	}
	else 
	{
		canvas->unlock_canvas();
		return 1;
	}
}


int VDeviceX11::close_all()
{
	if(is_open && canvas)
	{
		canvas->lock_canvas("VDeviceX11::close_all 1");
		canvas->get_canvas()->lock_window("VDeviceX11::close_all 1");
	}

	if(is_open && canvas && output_frame)
	{
// Copy our output frame buffer to the canvas's permanent frame buffer.
// They must be different buffers because the output frame is being written
// while the user is redrawing the canvas frame buffer over and over.
//for(int i = 0; i < 1000000; i++) ((float*)output_frame->get_rows()[0])[i] = 1;

		int use_opengl = device->out_config->driver == PLAYBACK_X11_GL &&
			output_frame->get_opengl_state() == VFrame::SCREEN;
		int best_color_model = output_frame->get_color_model();

		if(use_opengl)
		{
            if(cmodel_components(output_frame->get_color_model()) == 3)
            {
                if(cmodel_is_yuv(output_frame->get_color_model()))
                	best_color_model = BC_YUV888;
                else
                	best_color_model = BC_RGB888;
            }
            else
            {
                if(cmodel_is_yuv(output_frame->get_color_model()))
                	best_color_model = BC_YUVA8888;
                else
                    best_color_model = BC_RGBA8888;
            }
        }

		if(canvas->refresh_frame &&
			(canvas->refresh_frame->get_w() != device->out_w ||
			canvas->refresh_frame->get_h() != device->out_h ||
			canvas->refresh_frame->get_color_model() != best_color_model))
		{
			delete canvas->refresh_frame;
			canvas->refresh_frame = 0;
//printf("VDeviceX11::close_all %d deleting refresh_frame\n", __LINE__);
		}

		if(!canvas->refresh_frame)
		{
// printf("VDeviceX11::close_all %d output_frame=%d creating refresh_frame cmodel=%d\n", 
// __LINE__, 
// output_frame->get_color_model(), 
// best_color_model);
			canvas->refresh_frame = new VFrame(0,
				-1,
				device->out_w,
				device->out_h,
				best_color_model,
				-1);
		}

//printf("VDeviceX11::close_all %d %d %d\n", __LINE__, use_opengl, best_color_model);

		if(use_opengl)
		{
			canvas->get_canvas()->unlock_window();
			canvas->unlock_canvas();

//printf("VDeviceX11::close_all %d state=%d\n", __LINE__, output_frame->get_opengl_state());
			MWindow::playback_3d->copy_from(canvas, 
				canvas->refresh_frame,
				output_frame,
				0);
			canvas->lock_canvas("VDeviceX11::close_all 2");
			canvas->get_canvas()->lock_window("VDeviceX11::close_all 2");
//            canvas->refresh_frame->flip_vert();
		}
		else
        if(output_frame->get_rows())
		{
// printf("VDeviceX11::close_all %d refresh_frame cmodel=%d refresh_frame=%dx%d output_frame cmodel=%d output_frame=%dx%d\n", 
// __LINE__,
// canvas->refresh_frame->get_color_model(),
// canvas->refresh_frame->get_w(),
// canvas->refresh_frame->get_h(),
// output_frame->get_color_model(),
// output_frame->get_w(),
// output_frame->get_h());
			if(canvas->refresh_frame->get_w() != output_frame->get_w() ||
				canvas->refresh_frame->get_h() != output_frame->get_h())
			{
// need to scale it
// printf("VDeviceX11::close_all %d output_frame=%p rows=%p\n", 
// __LINE__,
// output_frame,
// output_frame->get_rows());
				cmodel_transfer(canvas->refresh_frame->get_rows(), 
					output_frame->get_rows(),
					0,
					0,
					0,
                    0, // out_a_plane
					output_frame->get_y(),
					output_frame->get_u(),
					output_frame->get_v(),
                    output_frame->get_a(),
					0, 
					0, 
					output_frame->get_w(), 
					output_frame->get_h(),
					0, 
					0, 
					canvas->refresh_frame->get_w(), 
					canvas->refresh_frame->get_h(),
					output_frame->get_color_model(), 
					canvas->refresh_frame->get_color_model(),
					0,
					output_frame->get_bytes_per_line(),
					canvas->refresh_frame->get_bytes_per_line());
			}
			else
			{
				canvas->refresh_frame->copy_from(output_frame);
			}
		}





// // Update the status bug
// 		if(!device->single_frame)
// 		{
// 			canvas->stop_video();
// 		}
// 		else
// 		{
// 			canvas->stop_single();
// 		}

// Draw the first refresh with new frame.
// Doesn't work if openGL video because OpenGL doesn't have 
// the output buffer for video.
// Not necessary for any case if we mandate a frame advance after
// every stop.
		if(/* device->out_config->driver != PLAYBACK_X11_GL || 
			*/ device->single_frame)
		{
			canvas->draw_refresh(1);
		}
	}




	if(bitmap)
	{
//printf("VDeviceX11::close_all %d deleting bitmap\n", __LINE__);
		delete bitmap;
		bitmap = 0;
	}

	if(output_frame)
	{
//printf("VDeviceX11::close_all %d deleting output_frame\n", __LINE__);
		delete output_frame;
		output_frame = 0;
	}

	if(is_open && canvas)
	{

// Update the status bug
		if(!device->single_frame)
		{
			canvas->stop_video();
		}
		else
		{
			canvas->stop_single();
		}

		canvas->get_canvas()->unlock_window();
		canvas->unlock_canvas();
	}

	reset_parameters();

	return 0;
}

int VDeviceX11::get_display_colormodel(int file_colormodel)
{
	int result = -1;

	if(device->out_config->driver == PLAYBACK_X11_GL)
	{
		if(file_colormodel == BC_RGB888 ||
			file_colormodel == BC_RGBA8888 ||
			file_colormodel == BC_YUV888 ||
			file_colormodel == BC_YUVA8888 ||
			file_colormodel == BC_YUV_FLOAT ||
			file_colormodel == BC_RGB_FLOAT ||
			file_colormodel == BC_RGBA_FLOAT)
		{
			return file_colormodel;
		}
		
		return BC_RGB888;
	}

	if(!device->single_frame)
	{
		switch(file_colormodel)
		{
			case BC_YUV444P:
			case BC_YUV420P:
			case BC_YUV422P:
			case BC_YUV422:
				result = file_colormodel;
				break;
		}
	}

// 2 more colormodels are supported by OpenGL
	if(device->out_config->driver == PLAYBACK_X11_GL)
	{
		if(file_colormodel == BC_YUV_FLOAT ||
			file_colormodel == BC_RGB_FLOAT ||
			file_colormodel == BC_RGBA_FLOAT)
			result = file_colormodel;
	}

	if(result < 0)
	{
		switch(file_colormodel)
		{
			case BC_RGB888:
			case BC_RGBA8888:
			case BC_YUV888:
			case BC_YUVA8888:
			case BC_YUV_FLOAT:
			case BC_RGB_FLOAT:
			case BC_RGBA_FLOAT:
				result = file_colormodel;
				break;

			default:
                if(canvas)
                {
    				canvas->lock_canvas("VDeviceX11::get_display_colormodel");
	    			result = canvas->get_canvas()->get_color_model();
		    		canvas->unlock_canvas();
                }
                else
                {
                    result = file_colormodel;
                }
				break;
		}
	}

	return result;
}


void VDeviceX11::new_output_buffer(VFrame **result, 
	int file_colormodel, 
	EDL *edl)
{
// printf("VDeviceX11::new_output_buffer %d hardware_scaling=%d\n",
// __LINE__,
// bitmap ? bitmap->hardware_scaling() : 0);
	canvas->lock_canvas("VDeviceX11::new_output_buffer");
	canvas->get_canvas()->lock_window("VDeviceX11::new_output_buffer 1");

// Get the best colormodel the display can handle.
	int display_colormodel = get_display_colormodel(file_colormodel);

// printf("VDeviceX11::new_output_buffer %d driver=%d output_frame=%p file_colormodel=%d display_colormodel=%d\n", 
// __LINE__, 
// device->out_config->driver,
// output_frame, 
// file_colormodel, 
// display_colormodel);
// Only create OpenGL Pbuffer and texture.
	if(device->out_config->driver == PLAYBACK_X11_GL)
	{
// Create bitmap for initial load into texture.
        if(output_frame && 
            file_colormodel != output_frame->get_color_model())
        {
            delete output_frame;
            output_frame = 0;
        }

		if(!output_frame)
		{
// printf("VDeviceX11::new_output_buffer %d %d,%d\n",
// __LINE__,
// device->out_w, 
// device->out_h);
			output_frame = new VFrame(0, 
				-1,
				device->out_w, 
				device->out_h, 
				file_colormodel,
				-1);

//BUFFER2(output_frame->get_rows()[0], "VDeviceX11::new_output_buffer 1");
		}

		window_id = canvas->get_canvas()->get_id();
		output_frame->set_opengl_state(VFrame::RAM);
	}
	else
	{
		canvas->get_transfers(edl, 
			output_x1, 
			output_y1, 
			output_x2, 
			output_y2, 
			canvas_x1, 
			canvas_y1, 
			canvas_x2, 
			canvas_y2,
// Canvas may be a different size than the temporary bitmap for pure software
			-1,
			-1);
		canvas_w = canvas_x2 - canvas_x1;
		canvas_h = canvas_y2 - canvas_y1;


// can the direct frame be used?
		int direct_supported = (output_x1 == 0 && 
			output_y1 == 0 &&
			output_x2 == device->out_w &&
			output_y2 == device->out_h &&
			!canvas->xscroll &&
			!canvas->yscroll);
// printf("VDeviceX11::new_output_buffer %d canvas: %dx%d %d %d\n",
// __LINE__,
// (int)canvas_w,
// (int)canvas_h,
// (int)canvas_y1,
// (int)canvas_y2);

// printf("VDeviceX11::new_output_buffer %d x1=%d y1=%d x2=%d y2=%d out_w=%d out_h=%d display_colormodel=%d\n",
// __LINE__,
// (int)output_x1,
// (int)output_y1,
// (int)output_x2,
// (int)output_y2,
// (int)device->out_w,
// (int)device->out_h,
// display_colormodel);

// file wants direct frame but we need a temp
		if(file_colormodel == BC_BGR8888 && !direct_supported)
		{
			file_colormodel = BC_RGB888;
		}

// Conform existing bitmap to new colormodel and output size
		if(bitmap)
		{
// printf("VDeviceX11::new_output_buffer %d bitmap=%dx%d canvas=%dx%d canvas=%dx%d\n",
// __LINE__,
// bitmap->get_w(),
// bitmap->get_h(),
// canvas_w,
// canvas_h,);

			int size_change = (bitmap->get_w() != canvas_w ||
				bitmap->get_h() != canvas_h);
//			int size_change = (bitmap->get_w() != canvas->get_canvas()->get_w() ||
//				bitmap->get_h() != canvas->get_canvas()->get_h());

// Restart if output size changed or output colormodel changed.
// May have to recreate if transferring between windowed and fullscreen.
			if(!color_model_selected ||
				file_colormodel != output_frame->get_color_model() ||
				(!bitmap->hardware_scaling() && size_change))
			{
// printf("VDeviceX11::new_output_buffer %d file_colormodel=%d prev file_colormodel=%d bitmap=%p output_frame=%p\n",
// __LINE__,
// file_colormodel,
// output_frame->get_color_model(),
// bitmap,
// output_frame);

				delete bitmap;
				delete output_frame;
				bitmap = 0;
				output_frame = 0;

// Clear borders if size changed
				if(size_change)
				{
// printf("VDeviceX11::new_output_buffer %d w=%d h=%d canvas_x1=%d canvas_y1=%d canvas_x2=%d canvas_y2=%d\n", 
// __LINE__,
// (int)output->w,
// (int)output->h,
// (int)canvas_x1,
// (int)canvas_y1,
// (int)canvas_x2,
// (int)canvas_y2);
					canvas->get_canvas()->set_color(BLACK);

					if(canvas_y1 > 0)
					{
						canvas->get_canvas()->draw_box(0, 0, canvas->w, canvas_y1);
						canvas->get_canvas()->flash(0, 0, canvas->w, canvas_y1);
					}

					if(canvas_y2 < canvas->h)
					{
						canvas->get_canvas()->draw_box(0, canvas_y2, canvas->w, canvas->h - canvas_y2);
						canvas->get_canvas()->flash(0, canvas_y2, canvas->w, canvas->h - canvas_y2);
					}

					if(canvas_x1 > 0)
					{
						canvas->get_canvas()->draw_box(0, canvas_y1, canvas_x1, canvas_y2 - canvas_y1);
						canvas->get_canvas()->flash(0, canvas_y1, canvas_x1, canvas_y2 - canvas_y1);
					}

					if(canvas_x2 < canvas->w)
					{
						canvas->get_canvas()->draw_box(canvas_x2, canvas_y1, canvas->w - canvas_x2, canvas_y2 - canvas_y1);
						canvas->get_canvas()->flash(canvas_x2, canvas_y1, canvas->w - canvas_x2, canvas_y2 - canvas_y1);
					}
				}
			}
			else
// Update the ring buffer
			if(bitmap_type == BITMAP_PRIMARY)
			{
				output_frame->set_memory(0 /* (unsigned char*)bitmap->get_data() + bitmap->get_shm_offset() */,
					bitmap->get_shmid(),
					bitmap->get_y_plane(),
					bitmap->get_u_plane(),
					bitmap->get_v_plane(),
                    bitmap->get_bytes_per_line());
// printf("VDeviceX11::new_output_buffer %d rows=%p\n", 
// __LINE__, 
// output_frame->get_rows());
			}
		}

// Create new bitmap
		if(!bitmap)
		{

// printf("VDeviceX11::new_output_buffer %d file_colormodel=%d display_colormodel=%d\n", 
// __LINE__, 
// file_colormodel,
// display_colormodel);

// Try hardware accelerated
			switch(display_colormodel)
			{
// blit from the codec directly to the window, using the standard X11 color model.
// Must scale in the codec.  No cropping
				case BC_BGR8888:
				{
					if(direct_supported)
					{
// printf("VDeviceX11::new_output_buffer %d creating direct bitmap w=%d h=%d\n", 
// __LINE__, 
// canvas_w,
// canvas_h);

						bitmap = new BC_Bitmap(canvas->get_canvas(), 
								canvas_w,
								canvas_h,
								display_colormodel,
								1);
						output_frame = new VFrame(
							0 /* (unsigned char*)bitmap->get_data() + bitmap->get_shm_offset() */, 
							bitmap->get_shmid(),
							0,
							0,
							0,
							canvas_w,
							canvas_h,
							display_colormodel,
							-1);

// printf("VDeviceX11::new_output_buffer %d shmid=%d output_frame=%p rows=%p\n", 
// __LINE__, 
// bitmap->get_shmid(), 
// output_frame, 
// output_frame->get_rows());
						bitmap_type = BITMAP_PRIMARY;
					}
					break;
				}

				case BC_YUV420P:
					if(device->out_config->driver == PLAYBACK_X11_XV &&
						canvas->get_canvas()->accel_available(display_colormodel, 0) &&
						!canvas->use_scrollbars)
					{
//printf("VDeviceX11::new_output_buffer %d\n", 
//__LINE__);

						bitmap = new BC_Bitmap(canvas->get_canvas(), 
							device->out_w,
							device->out_h,
							display_colormodel,
							1);
						output_frame = new VFrame(
							0 /* (unsigned char*)bitmap->get_data() + bitmap->get_shm_offset() */, 
							bitmap->get_shmid(),
							bitmap->get_y_plane(),
							bitmap->get_u_plane(),
							bitmap->get_v_plane(),
							device->out_w,
							device->out_h,
							display_colormodel,
							-1);
						bitmap_type = BITMAP_PRIMARY;
					}
					break;

				case BC_YUV422P:
					if(device->out_config->driver == PLAYBACK_X11_XV &&
						canvas->get_canvas()->accel_available(display_colormodel, 0) &&
						!canvas->use_scrollbars)
					{
						bitmap = new BC_Bitmap(canvas->get_canvas(), 
							device->out_w,
							device->out_h,
							display_colormodel,
							1);
						output_frame = new VFrame(
							0 /* (unsigned char*)bitmap->get_data() + bitmap->get_shm_offset() */, 
							bitmap->get_shmid(),
							bitmap->get_y_plane(),
							bitmap->get_u_plane(),
							bitmap->get_v_plane(),
							device->out_w,
							device->out_h,
							display_colormodel,
							-1);
						bitmap_type = BITMAP_PRIMARY;
					}
					else
					if(device->out_config->driver == PLAYBACK_X11_XV &&
						canvas->get_canvas()->accel_available(BC_YUV422, 0))
					{
						bitmap = new BC_Bitmap(canvas->get_canvas(), 
							device->out_w,
							device->out_h,
							BC_YUV422,
							1);
						bitmap_type = BITMAP_TEMP;
					}
					break;

				case BC_YUV422:
					if(device->out_config->driver == PLAYBACK_X11_XV &&
						canvas->get_canvas()->accel_available(display_colormodel, 0) &&
						!canvas->use_scrollbars)
					{
						bitmap = new BC_Bitmap(canvas->get_canvas(), 
							device->out_w,
							device->out_h,
							display_colormodel,
							1);
						output_frame = new VFrame(
							0 /* (unsigned char*)bitmap->get_data() + bitmap->get_shm_offset() */, 
							bitmap->get_shmid(),
							bitmap->get_y_plane(),
							bitmap->get_u_plane(),
							bitmap->get_v_plane(),
							device->out_w,
							device->out_h,
							display_colormodel,
							-1);
						bitmap_type = BITMAP_PRIMARY;
					}
					else
					if(device->out_config->driver == PLAYBACK_X11_XV &&
						canvas->get_canvas()->accel_available(BC_YUV422P, 0))
					{
						bitmap = new BC_Bitmap(canvas->get_canvas(), 
							device->out_w,
							device->out_h,
							BC_YUV422P,
							1);
						bitmap_type = BITMAP_TEMP;
					}
					break;
			}

// Make an intermediate frame
			if(!bitmap)
			{
				display_colormodel = canvas->get_canvas()->get_color_model();
// printf("VDeviceX11::new_output_buffer %d creating temp display_colormodel=%d file_colormodel=%d %dx%d %dx%d %dx%d\n",
// __LINE__,
// display_colormodel,
// file_colormodel,
// device->out_w,
// device->out_h,
// canvas->get_canvas()->get_w(),
// canvas->get_canvas()->get_h(),
// canvas_w,
// canvas_h);
//printf("VDeviceX11::new_output_buffer %d creating bitmap\n", __LINE__);
				bitmap = new BC_Bitmap(canvas->get_canvas(), 
					canvas_w,
					canvas_h,
					display_colormodel,
					1);
				bitmap_type = BITMAP_TEMP;
			}

			if(bitmap_type == BITMAP_TEMP)
			{
// Intermediate frame
//printf("VDeviceX11::new_output_buffer %d creating output_frame\n", __LINE__);
				output_frame = new VFrame(0, 
					-1,
					device->out_w,
					device->out_h,
					file_colormodel,
					-1);
//BUFFER2(output_frame->get_rows()[0], "VDeviceX11::new_output_buffer 2");
			}
			color_model_selected = 1;
		}
	}

	*result = output_frame;
//printf("VDeviceX11::new_output_buffer 10 %d\n", canvas->get_canvas()->get_window_lock());

	canvas->get_canvas()->unlock_window();
	canvas->unlock_canvas();
}

void VDeviceX11::start_rendering()
{
    is_rendering = 1;
}

int VDeviceX11::start_playback()
{
// Record window is initialized when its monitor starts.
	if(!device->single_frame && canvas)
		canvas->start_video();
	return 0;
}

int VDeviceX11::stop_playback()
{
	if(!device->single_frame && canvas)
		canvas->stop_video();
// Record window goes back to monitoring
// get the last frame played and store it in the video_out
	return 0;
}

void VDeviceX11::output_to_bitmap(VFrame *output_frame)
{
	if(bitmap->hardware_scaling())
	{
//printf("VDeviceX11::output_to_bitmap %d\n", __LINE__);
		cmodel_transfer(bitmap->get_row_pointers(), 
			output_frame->get_rows(),
			0,
			0,
			0,
            0, // out_a_plane
			output_frame->get_y(),
			output_frame->get_u(),
			output_frame->get_v(),
			output_frame->get_a(),
			0, 
			0, 
			output_frame->get_w(), 
			output_frame->get_h(),
			0, 
			0, 
			bitmap->get_w(), 
			bitmap->get_h(),
			output_frame->get_color_model(), 
			bitmap->get_color_model(),
			0,
			output_frame->get_w(),
			bitmap->get_w());
	}
	else
	{
// multiply alpha with checkerboard in single frame mode only
        if(device->single_frame && cmodel_has_alpha(output_frame->get_color_model()))
        {
            int checker_w = CHECKER_W;
            int checker_h = CHECKER_H;
//printf("VDeviceX11::output_to_bitmap %d\n", __LINE__);
            cmodel_transfer_alpha(bitmap->get_row_pointers(), 
			    output_frame->get_rows(),
			    (int)output_x1, 
			    (int)output_y1, 
			    (int)(output_x2 - output_x1), 
			    (int)(output_y2 - output_y1),
			    0, 
			    0, 
			    (int)(canvas_x2 - canvas_x1), 
			    (int)(canvas_y2 - canvas_y1),
			    output_frame->get_color_model(), 
			    bitmap->get_color_model(),
                output_frame->get_bytes_per_line(),
			    bitmap->get_bytes_per_line(),
                checker_w,
                checker_h);
            
//printf("VDeviceX11::output_to_bitmap %d\n", __LINE__);
        }
        else
        {
// all other modes multiply alpha with black
// printf("VDeviceX11::output_to_bitmap %d bitmap: %dx%d output_frame: %dx%d\n",
// __LINE__,
// bitmap->get_w(),
// bitmap->get_h(),
// output_frame->get_w(),
// output_frame->get_h());
// printf("VDeviceX11::output_to_bitmap %d %d %d\n", 
// __LINE__, 
// output_frame->get_color_model(), 
// bitmap->get_color_model());
		    cmodel_transfer(bitmap->get_row_pointers(), 
			    output_frame->get_rows(),
			    0,
			    0,
			    0,
                0, // out_a_plane
			    output_frame->get_y(),
			    output_frame->get_u(),
			    output_frame->get_v(),
                output_frame->get_a(), // in_a_plane
			    (int)output_x1, 
			    (int)output_y1, 
			    (int)(output_x2 - output_x1), 
			    (int)(output_y2 - output_y1),
			    0, 
			    0, 
			    (int)(canvas_x2 - canvas_x1), 
			    (int)(canvas_y2 - canvas_y1),
			    output_frame->get_color_model(), 
			    bitmap->get_color_model(),
			    0,
			    output_frame->get_bytes_per_line(),
			    bitmap->get_bytes_per_line());
        }
	}
}

int VDeviceX11::write_buffer(VFrame *output_frame, EDL *edl)
{
	int i = 0;
	canvas->lock_canvas("VDeviceX11::write_buffer");
	canvas->get_canvas()->lock_window("VDeviceX11::write_buffer 1");



// printf("VDeviceX11::write_buffer %d bitmap_type=%d driver=%d\n", 
// __LINE__, 
// bitmap_type,
// device->out_config->driver);



// Convert colormodel
	if(bitmap_type == BITMAP_TEMP)
	{
//printf("VDeviceX11::write_buffer %d\n", __LINE__);


        output_to_bitmap(output_frame);

	}

//printf("VDeviceX11::write_buffer 4 %p\n", bitmap);
//for(i = 0; i < 1000; i += 4) bitmap->get_data()[i] = 128;
//printf("VDeviceX11::write_buffer 2 %d %d %d\n", bitmap_type, 
//	bitmap->get_color_model(), 
//	output->get_color_model());fflush(stdout);

// printf("VDeviceX11::write_buffer %d %dx%d %f %f %f %f -> %f %f %f %f\n",
// __LINE__,
// output->w,
// output->h,
// output_x1, 
// output_y1, 
// output_x2, 
// output_y2, 
// canvas_x1, 
// canvas_y1, 
// canvas_x2, 
// canvas_y2);



// Cause X server to display it
	if(device->out_config->driver == PLAYBACK_X11_GL)
	{
// Output is drawn in close_all if no video.
		if(canvas->get_canvas()->get_video_on())
		{
			int use_bitmap_extents = 0;
			canvas_w = -1;
			canvas_h = -1;
// Canvas may be a different size than the temporary bitmap for pure software
			if(bitmap_type == BITMAP_TEMP && 
				!bitmap->hardware_scaling())
			{
				canvas_w = bitmap->get_w();
				canvas_h = bitmap->get_h();
			}

			canvas->get_transfers(edl, 
				output_x1, 
				output_y1, 
				output_x2, 
				output_y2, 
				canvas_x1, 
				canvas_y1, 
				canvas_x2, 
				canvas_y2,
				canvas_w,
				canvas_h);


// Draw output frame directly.
			canvas->get_canvas()->unlock_window();
			canvas->unlock_canvas();
			MWindow::playback_3d->write_buffer(canvas, 
				output_frame,
				output_x1,
				output_y1,
				output_x2,
				output_y2,
				canvas_x1,
				canvas_y1,
				canvas_x2,
				canvas_y2,
				is_cleared);
			is_cleared = 0;
			canvas->lock_canvas("VDeviceX11::write_buffer 2");
			canvas->get_canvas()->lock_window("VDeviceX11::write_buffer 2");
		}
	}
	else
	if(bitmap->hardware_scaling())
	{
		canvas->get_canvas()->draw_bitmap(bitmap,
			!device->single_frame,
			(int)canvas_x1,
			(int)canvas_y1,
			(int)(canvas_x2 - canvas_x1),
			(int)(canvas_y2 - canvas_y1),
			(int)output_x1,
			(int)output_y1,
			(int)(output_x2 - output_x1),
			(int)(output_y2 - output_y1),
			0);
	}
	else
	{
// printf("VDeviceX11::write_buffer %d x=%d y=%d w=%d h=%d\n", 
// __LINE__, 
// (int)canvas_x1,
// (int)canvas_y1,
// canvas->get_canvas()->get_w(),
// canvas->get_canvas()->get_h());

		canvas->get_canvas()->draw_bitmap(bitmap,
			!device->single_frame,
			(int)canvas_x1,
			(int)canvas_y1,
			(int)(canvas_x2 - canvas_x1),
			(int)(canvas_y2 - canvas_y1),
			0, 
			0, 
			(int)(canvas_x2 - canvas_x1),
			(int)(canvas_y2 - canvas_y1),
			0);
//printf("VDeviceX11::write_buffer %d bitmap=%p\n", __LINE__, bitmap);
	}


// draw the FPS
//printf("VDeviceX11::write_buffer %d %p %d\n", __LINE__, device->mwindow, MWindow::preferences->show_fps);
    if(device->mwindow && canvas->get_fps() && MWindow::preferences->show_fps)
    {
        int margin = MWindow::theme->widget_border;
     	char string[BCTEXTLEN];
     	sprintf(string, "%.4f", device->mwindow->session->actual_frame_rate);

        canvas->get_fps()->clear_box(0, 0, canvas->get_fps()->get_w(), canvas->get_fps()->get_h());
        canvas->get_fps()->set_color(MWindow::theme->fps_color);
        canvas->get_fps()->set_font(MEDIUMFONT);
        canvas->get_fps()->draw_text(margin, canvas->get_fps()->get_h() - margin, string);
        canvas->get_fps()->flash();
    }

	canvas->get_canvas()->unlock_window();
	canvas->unlock_canvas();
	return 0;
}


void VDeviceX11::clear_output(VFrame *frame)
{
	is_cleared = 1;

//printf("VDeviceX11::clear_output %d %p frame=%p\n", __LINE__, output_frame, frame);
	MWindow::playback_3d->clear_output(canvas,
		 frame,
         !is_rendering && canvas->get_canvas()->get_video_on());

}


void VDeviceX11::clear_input(VFrame *frame)
{
	MWindow::playback_3d->clear_input(this->canvas, frame);
}

void VDeviceX11::convert_cmodel(VFrame *output, int dst_cmodel)
{
	MWindow::playback_3d->convert_cmodel(this->canvas,
		output, 
		dst_cmodel);
}

void VDeviceX11::convert_cmodel(VFrame *input, VFrame *output)
{
	MWindow::playback_3d->convert_cmodel(this->canvas,
        input,
		output);
}

void VDeviceX11::do_camera(VFrame *output,
	VFrame *input,
	float in_x1, 
	float in_y1, 
	float in_x2, 
	float in_y2, 
	float out_x1, 
	float out_y1, 
	float out_x2, 
	float out_y2)
{
	MWindow::playback_3d->do_camera(this->canvas, 
		output,
		input,
		in_x1, 
		in_y1, 
		in_x2, 
		in_y2, 
		out_x1, 
		out_y1, 
		out_x2, 
		out_y2);
}


void VDeviceX11::do_fade(VFrame *output_temp, float fade)
{
	MWindow::playback_3d->do_fade(this->canvas, output_temp, fade);
}

void VDeviceX11::do_mask(VFrame *output_temp, 
        VFrame *mask,
		int64_t start_position_project,
		MaskAutos *keyframe_set, 
		MaskAuto *keyframe,
		MaskAuto *default_auto)
{
	MWindow::playback_3d->do_mask(canvas,
		output_temp,
        mask,
		start_position_project,
		keyframe_set,
		keyframe,
		default_auto);
}

void VDeviceX11::overlay(VFrame *output_frame,
		VFrame *input, 
// This is the transfer from track to output frame
		float in_x1, 
		float in_y1, 
		float in_x2, 
		float in_y2, 
		float out_x1, 
		float out_y1, 
		float out_x2, 
		float out_y2, 
		float alpha,        // 0 - 1
		int mode,
		EDL *edl,
		int is_nested)
{
	int interpolation_type = edl->session->interpolation_type;

// printf("VDeviceX11::overlay 1:\n"
// "in_x1=%f in_y1=%f in_x2=%f in_y2=%f\n"
// "out_x1=%f out_y1=%f out_x2=%f out_y2=%f\n",
// in_x1, 
// in_y1, 
// in_x2, 
// in_y2, 
// out_x1,
// out_y1,
// out_x2,
// out_y2);
// Convert node coords to canvas coords in here

// If single frame playback or nested EDL, use full sized PBuffer as output.
//	if(device->single_frame || is_nested)
// always use pbuffer
	if(1)
	{
		MWindow::playback_3d->overlay(canvas, 
			input,
			in_x1, 
			in_y1, 
			in_x2, 
			in_y2, 
			out_x1,
			out_y1,
			out_x2,
			out_y2,
			alpha,  	  // 0 - 1
			mode,
			interpolation_type,
			output_frame,
			is_nested);
// printf("VDeviceX11::overlay 1 %p %d %d %d\n", 
// output_frame, 
// output_frame->get_w(),
// output_frame->get_h(),
// output_frame->get_opengl_state());
	}
	else
	{
		canvas->lock_canvas("VDeviceX11::overlay");
		canvas->get_canvas()->lock_window("VDeviceX11::overlay");

// This is the transfer from output frame to canvas
		canvas->get_transfers(edl, 
			output_x1, 
			output_y1, 
			output_x2, 
			output_y2, 
			canvas_x1, 
			canvas_y1, 
			canvas_x2, 
			canvas_y2,
			-1,
			-1);

		canvas->get_canvas()->unlock_window();
		canvas->unlock_canvas();


// Get transfer from track to canvas
		float track_xscale = (out_x2 - out_x1) / (in_x2 - in_x1);
		float track_yscale = (out_y2 - out_y1) / (in_y2 - in_y1);
		float canvas_xscale = (float)(canvas_x2 - canvas_x1) / (output_x2 - output_x1);
		float canvas_yscale = (float)(canvas_y2 - canvas_y1) / (output_y2 - output_y1);


// Get coordinates of canvas relative to track frame
		float track_x1 = (float)(output_x1 - out_x1) / track_xscale + in_x1;
		float track_y1 = (float)(output_y1 - out_y1) / track_yscale + in_y1;
		float track_x2 = (float)(output_x2 - out_x2) / track_xscale + in_x2;
		float track_y2 = (float)(output_y2 - out_y2) / track_yscale + in_y2;

// Clamp canvas coords to track boundary
		if(track_x1 < 0)
		{
			float difference = -track_x1;
			track_x1 += difference;
			canvas_x1 += difference * track_xscale * canvas_xscale;
		}
		if(track_y1 < 0)
		{
			float difference = -track_y1;
			track_y1 += difference;
			canvas_y1 += difference * track_yscale * canvas_yscale;
		}

		if(track_x2 > input->get_w())
		{
			float difference = track_x2 - input->get_w();
			track_x2 -= difference;
			canvas_x2 -= difference * track_xscale * canvas_xscale;
		}
		if(track_y2 > input->get_h())
		{
			float difference = track_y2 - input->get_h();
			track_y2 -= difference;
			canvas_y2 -= difference * track_yscale * canvas_yscale;
		}





// Overlay directly from track buffer to canvas, skipping output buffer
		if(track_x2 > track_x1 && 
			track_y2 > track_y1 &&
			canvas_x2 > canvas_x1 &&
			canvas_y2 > canvas_y1)
		{
			MWindow::playback_3d->overlay(canvas, 
				input,
				track_x1, 
				track_y1, 
				track_x2, 
				track_y2, 
				canvas_x1,
				canvas_y1,
				canvas_x2,
				canvas_y2,
				alpha,  	  // 0 - 1
				mode,
				interpolation_type,
				0);
		}
	}
}

void VDeviceX11::run_plugin(PluginClient *client)
{
	MWindow::playback_3d->run_plugin(canvas, client);
}

void VDeviceX11::copy_frame(VFrame *dst, VFrame *src, int want_texture)
{
	MWindow::playback_3d->copy_from(canvas, 
        dst, 
        src, 
        want_texture /* 1 */);
}

