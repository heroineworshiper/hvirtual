/*
 * CINELERRA
 * Copyright (C) 2009-2024 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "bcsignals.h"
#include "cache.h"
#include "condition.h"
#include "datatype.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "playabletracks.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "strategies.inc"
#include "tracks.h"
#include "transportque.h"
#include "units.h"
#include "vdevicex11.inc"
#include "vedit.h"
#include "vframe.h"
#include "videoconfig.h"
#include "videodevice.h"
#include "virtualconsole.h"
#include "virtualvconsole.h"
#include "vmodule.h"
#include "vrender.h"
#include "vtrack.h"





VRender::VRender(RenderEngine *renderengine)
 : CommonRender(renderengine)
{
	data_type = TRACK_VIDEO;
	transition_temp = 0;
	overlayer = new OverlayFrame(renderengine->preferences->processors);
	input_temp = 0;
	vmodule_render_fragment = 0;
	playback_buffer = 0;
	session_frame = 0;
	asynchronous = 0;     // render 1 frame at a time
	framerate_counter = 0;
	video_out = 0;
	render_strategy = -1;
    timer = new Timer(1);
}

VRender::~VRender()
{
	if(input_temp) delete input_temp;
	if(transition_temp) delete transition_temp;
	if(overlayer) delete overlayer;
}


VirtualConsole* VRender::new_vconsole_object() 
{
	return new VirtualVConsole(renderengine, this);
}

int VRender::get_total_tracks()
{
	return renderengine->get_edl()->tracks->total_video_tracks();
}

Module* VRender::new_module(Track *track)
{
	return new VModule(renderengine, this, 0, track);
}

int VRender::flash_output()
{
	if(video_out)
		return renderengine->vdevice->write_buffer(video_out, renderengine->get_edl());
	else
		return 0;
}

int VRender::process_buffer(VFrame *video_out, 
	int64_t input_position,
	int use_opengl)
{
// process buffer for non realtime
	int i, j;
	int64_t render_len = 1;
	int reconfigure = 0;
//printf("VRender::process_buffer %d use_opengl=%d\n", __LINE__, use_opengl);

	this->video_out = video_out;

	current_position = input_position;

	reconfigure = vconsole->test_reconfigure(input_position, 
		render_len);

	if(reconfigure) restart_playback();
	return process_buffer(input_position, use_opengl);
}


int VRender::process_buffer(int64_t input_position,
	int use_opengl)
{
	VEdit *playable_edit = 0;
	int colormodel;
	int use_vconsole = 1;
	int use_brender = 0;
	int result = 0;
	int use_cache = renderengine->command->single_frame();
	const int debug = 0;

// printf("VRender::process_buffer %d current_position=%d\n", 
// __LINE__,
// (int)current_position);

	if(MWindow::preferences->dump_playback) 
        MWindow::indent += 2;

// Determine the rendering strategy for this frame.
	use_vconsole = get_use_vconsole(&playable_edit, 
		input_position,
		use_brender);

// Negotiate color model
// If the virtual console is faster than direct, make sure the colorspace 
// of video_out is optimized for the codec.
	colormodel = get_colormodel(playable_edit, use_vconsole, use_brender);
	if(debug) printf("VRender::process_buffer %d\n", __LINE__);



//printf("VRender::process_buffer %d\n", __LINE__);
// Get output buffer from device
	if(renderengine->command->realtime &&
		!renderengine->is_nested)
	{

		renderengine->vdevice->new_output_buffer(&video_out, 
			colormodel, 
			renderengine->get_edl());
	}

// printf("VRender::process_buffer %d video_out=%p current_position=%d\n", 
// __LINE__, 
// video_out,
// (int)current_position);

// printf("VRender::process_buffer use_vconsole=%d colormodel=%d video_out=%p\n", 
// use_vconsole, 
// colormodel,
// video_out);
// Read directly from file to video_out
	if(!use_vconsole)
	{

		if(use_brender)
		{
			Asset *asset = renderengine->preferences->brender_asset;
			File *file = renderengine->get_vcache()->check_out(asset,
				renderengine->get_edl());

			if(file)
			{
				int64_t corrected_position = current_position;
				if(renderengine->command->get_direction() == PLAY_REVERSE)
					corrected_position--;

				file->stop_video_thread();
				if(use_cache) file->set_cache_frames(1);
				int64_t normalized_position = (int64_t)(corrected_position *
					asset->frame_rate /
					renderengine->get_edl()->session->frame_rate);

				file->set_video_position(normalized_position,
					0);
				file->read_frame(video_out, 0, 0, 0);

		        video_out->set_opengl_state(VFrame::RAM);

				if(use_cache) file->set_cache_frames(0);
				renderengine->get_vcache()->check_in(asset);
			}

		}
		else
		if(playable_edit)
		{
		    if(debug) printf("VRender::process_buffer %d color_model=%d w=%d h=%d\n", 
                __LINE__,
                video_out->get_color_model(),
                video_out->get_w(),
                video_out->get_h());
            VDeviceX11 *x11_device = 0;
            if(use_opengl && renderengine && renderengine->vdevice)
            {    x11_device = (VDeviceX11*)renderengine->vdevice->get_output_base();
    			if(!x11_device) use_opengl = 0;
            }
// printf("VRender::process_buffer %d current_position=%d\n", 
// __LINE__,
// (int)current_position);


			result = ((VEdit*)playable_edit)->read_frame(video_out, 
				current_position, 
				renderengine->command->get_direction(),
				renderengine->get_vcache(),
				1,
				use_cache,
				0,
                use_opengl,
                x11_device);
//             printf("VRender::process_buffer %d state=%d color_model=%d\n", 
//                 __LINE__, 
//                 video_out->get_opengl_state(),
//                 video_out->get_color_model());
		}


	}
	else
// Read into virtual console
	{
//printf("VRender::process_buffer %d\n", __LINE__);




// process this buffer now in the virtual console
		result = ((VirtualVConsole*)vconsole)->process_buffer(input_position,
			use_opengl);
//printf("VRender::process_buffer %d\n", __LINE__);
	}


	if(MWindow::preferences->dump_playback) 
    {
        char string[BCTEXTLEN];
        MWindow::indent -= 2;
        cmodel_to_text(string, colormodel);
        printf("%sVRender::process_buffer %d EDL='%s' position=%ld use_vconsole=%d colormodel='%s' use_gl=%d\n", 
            MWindow::print_indent(),
            __LINE__, 
            renderengine->get_edl()->path,
            (long)input_position,
            use_vconsole,
            string,
            use_opengl);
    }

	return result;
}

// Determine if virtual console is needed
int VRender::get_use_vconsole(VEdit* *playable_edit, 
	int64_t position,
	int &use_brender)
{
	Track *playable_track = 0;
	*playable_edit = 0;
//printf("VRender::get_use_vconsole %d %p\n", __LINE__, renderengine);


// Background rendering completed
	if((use_brender = renderengine->brender_available(position, 
		renderengine->command->get_direction())) != 0) 
		return 0;

// Descend into EDL nest
	return renderengine->get_edl()->get_use_vconsole(playable_edit,
		position, 
		renderengine->command->get_direction(),
		vconsole->playable_tracks);
}

int VRender::get_colormodel(VEdit *playable_edit, 
	int use_vconsole,
	int use_brender)
{
	int colormodel = renderengine->get_edl()->session->color_model;

	if(!use_vconsole && !renderengine->command->single_frame())
	{
// Get best colormodel supported by the file
		int driver = renderengine->config->vconfig->driver;
		File *file;
		Asset *asset;

		if(use_brender)
		{
			asset = renderengine->preferences->brender_asset;
		}
		else
		{
			int64_t source_position = 0;
			asset = playable_edit->get_nested_asset(&source_position,
				current_position,
				renderengine->command->get_direction());
		}

		if(asset)
		{
			file = renderengine->get_vcache()->check_out(asset,
				renderengine->get_edl());

			if(file)
			{
				colormodel = file->get_best_colormodel(0,
                    renderengine->config->vconfig);
//printf("VRender::get_colormodel %d driver=%d colormodel=%d\n", __LINE__, driver, colormodel);
				renderengine->get_vcache()->check_in(asset);
			}
		}
	}

	return colormodel;
}







void VRender::run()
{
	int reconfigure;
	const int debug = 0;

// Want to know how many seconds rendering each frame takes.
// Then use this number to predict the next frame that should be rendered.
// Be suspicious of frames that render late so have a countdown
// before we start dropping.
	double current_time, start_time, end_time; // Absolute times
	int64_t next_frame;  // Actual position.
	int64_t last_delay = 0;  // delay used before last frame
	int64_t skip_countdown = VRENDER_THRESHOLD;    // frames remaining until drop
	int64_t delay_countdown = VRENDER_THRESHOLD;  // Frames remaining until delay
// Number of frames before next reconfigure
	int64_t current_input_length;
// Number of frames to skip.
	int64_t frame_step = 1;
	int use_opengl = (renderengine->vdevice && 
		renderengine->vdevice->out_config->driver == PLAYBACK_X11_GL);

	first_frame = 1;

// Number of frames since start of rendering
	session_frame = 0;
	framerate_counter = 0;
	framerate_timer.update();
    timer->reset_delay();

	start_lock->unlock();
	if(debug) printf("VRender::run %d\n", __LINE__);


	while(!done && 
		!renderengine->vdevice->interrupt)
	{
// Perform the most time consuming part of frame decompression now.
// Want the condition before, since only 1 frame is rendered 
// and the number of frames skipped after this frame varies.
		current_input_length = 1;

		reconfigure = vconsole->test_reconfigure(current_position, 
			current_input_length);


		if(debug) printf("VRender::run %d\n", __LINE__);
		if(reconfigure) restart_playback();

		if(debug) printf("VRender::run %d\n", __LINE__);
		process_buffer(current_position, use_opengl);


		if(debug) printf("VRender::run %d\n", __LINE__);

		if(renderengine->command->single_frame())
		{
//			printf("VRender::run %d\n", __LINE__);
			flash_output();
			frame_step = 1;
			done = 1;
		}
		else
// Perform synchronization
		{
// Determine the delay until the frame needs to be shown.
			current_time = renderengine->sync_position() * 
				renderengine->command->get_speed();
// latest time at which the frame can be shown.
			end_time = (double)session_frame / renderengine->get_edl()->session->frame_rate;
// earliest time by which the frame needs to be shown.
			start_time = (double)(session_frame - 1) / renderengine->get_edl()->session->frame_rate;
//printf("VRender::run %d\n", __LINE__);

			if(first_frame || end_time < current_time)
			{
// Frame rendered late or this is the first frame.  Flash it now.
				flash_output();

				if(renderengine->get_edl()->session->video_every_frame)
				{
// User wants every frame.
					frame_step = 1;
				}
				else
				if(skip_countdown > 0)
				{
// Maybe just a freak.
					frame_step = 1;
					skip_countdown--;
				}
				else
				{
// Get the frames to skip.
					delay_countdown = VRENDER_THRESHOLD;
					frame_step = 1;
					frame_step += (int64_t)(current_time * renderengine->get_edl()->session->frame_rate);
					frame_step -= (int64_t)(end_time * renderengine->get_edl()->session->frame_rate);
				}
			}
			else
			{
// Frame rendered early or just in time.
				frame_step = 1;

				if(delay_countdown > 0)
				{
// Maybe just a freak
					delay_countdown--;
				}
				else
				{
					skip_countdown = VRENDER_THRESHOLD;
					if(start_time > current_time)
					{
						int64_t delay_time = (int64_t)((start_time - current_time) * 
							1000);
// TODO: need to interrupt here
						timer->delay(delay_time);
					}
					else
					{
// Came after the earliest sample so keep going
					}
				}

// Flash frame now.
//printf("VRender::run %d %lld\n", __LINE__, current_input_length);
				flash_output();
			}
		}
		if(debug) printf("VRender::run %d\n", __LINE__);

// Trigger audio to start
		if(first_frame)
		{
			renderengine->first_frame_lock->unlock();
			first_frame = 0;
			renderengine->reset_sync_position();
		}
		if(debug) printf("VRender::run %d frame_step=%d\n", __LINE__, (int)frame_step);

		session_frame += frame_step;

// advance position in project
		current_input_length = frame_step;


// Subtract frame_step in a loop to allow looped playback to drain
// printf("VRender::run %d done=%d frame_step=%d current_input_length=%d\n", 
// __LINE__,
// done,
// (int)frame_step, 
// (int)current_input_length);
		while(frame_step && current_input_length)
		{
// trim current_input_length to range
			get_boundaries(current_input_length);
// advance 1 frame
			advance_position(current_input_length);
			frame_step -= current_input_length;
			current_input_length = frame_step;
			if(done) break;
// printf("VRender::run %d %d %d %d\n", 
// __LINE__,
// done,
// (int)frame_step, 
// (int)current_input_length);
		}

		if(debug) printf("VRender::run %d current_position=%lld done=%d\n", 
			__LINE__, 
			(long long)current_position,
			done);

// Update tracking.
		if(renderengine->command->realtime &&
			renderengine->playback_engine &&
			renderengine->command->command != CURRENT_FRAME)
		{
			renderengine->playback_engine->update_tracking(fromunits(current_position));
		}
		if(debug) printf("VRender::run %d\n", __LINE__);

// Calculate the framerate counter
		framerate_counter++;
		if(framerate_counter >= renderengine->get_edl()->session->frame_rate && 
			renderengine->command->realtime)
		{
			renderengine->update_framerate((float)framerate_counter / 
				((float)framerate_timer.get_difference() / 1000));
			framerate_counter = 0;
			framerate_timer.update();
		}
		if(debug) printf("VRender::run %d done=%d\n", __LINE__, done);
	}


// In case we were interrupted before the first loop
	renderengine->first_frame_lock->unlock();
	stop_plugins();
	if(debug) printf("VRender::run %d done=%d\n", __LINE__, done);
}

// int VRender::start_playback()
// {
// // start reading input and sending to vrenderthread
// // use a thread only if there's a video device
// 	if(renderengine->command->realtime)
// 	{
// 		start();
// 	}
//     return 0;
// }

void VRender::interrupt_playback()
{
    timer->cancel_delay();
}

int64_t VRender::tounits(double position, int round)
{
	if(round)
		return Units::round(position * renderengine->get_edl()->session->frame_rate);
	else
		return Units::to_int64(position * renderengine->get_edl()->session->frame_rate);
}

double VRender::fromunits(int64_t position)
{
	return (double)position / renderengine->get_edl()->session->frame_rate;
}




















