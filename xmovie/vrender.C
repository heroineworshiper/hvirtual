#include "asset.h"
#include "condition.h"
#include "file.h"
#include "settings.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "renderengine.h"
#include "settings.h"
#include "bctimer.h"
#include "transportque.h"
#include "units.h"
#include "vframe.h"
#include "vrender.h"

#include <unistd.h>

VRender::VRender(MWindow *mwindow, RenderEngine *render_engine)
{
	this->mwindow = mwindow;
	this->render_engine = render_engine;
	startup_lock = new Mutex;
	timer = new Timer;
	framerate_timer = new Timer;
	set_synchronous(1);
}

VRender::~VRender()
{
	delete startup_lock;
	delete timer;
	delete framerate_timer;
}

void VRender::arm_playback()
{
	if(!render_engine->command->single_frame())
	{
		mwindow->gui->start_video();
	}

	done = 0;
	current_frame = 0;
	last_frame = 0;
	current_sample = 0;
	last_sample = 0;
	framerate_timer->update();
	framerate_counter = 0;
	skip_countdown = VRENDER_THRESHOLD;
	delay_countdown = VRENDER_THRESHOLD;


	if(!render_engine->command->change_position())
	{
		render_engine->seek_lock->lock();
		mwindow->video_file->lock_read();
		mwindow->video_file->set_video_stream(mwindow->video_stream);
		mwindow->video_file->set_position(render_engine->command->start_position);
		mwindow->video_file->unlock_read();
		render_engine->seek_lock->unlock();
	}
}

int VRender::start_playback()
{
	Thread::start();
	return 0;
}

int VRender::interrupt_playback()
{
	done = 1;
	return 0;
}

int VRender::close_playback()
{
	double percentage, time;
//printf("VRender::close_playback 1\n");
	if(!render_engine->command->single_frame())
	{
		mwindow->gui->lock_window("VRender::close_playback");
		mwindow->gui->stop_video();
		mwindow->gui->unlock_window();
	}
	mwindow->video_file->get_position(&percentage, &time);
//printf("VRender::close_playback 10\n");
	return 0;
}

int VRender::wait_for_startup()
{
	startup_lock->lock();
	startup_lock->unlock();
	return 0;
}

int VRender::wait_for_completion()
{
	join();
	return 0;
}

void VRender::write_output()
{
// Don't need to lock canvas if fullscreen since they share the same display
//printf("VRender::write_output 1\n");
	mwindow->gui->lock_window("VRender::write_output");
	mwindow->gui->write_output();
	mwindow->gui->unlock_window();
//printf("VRender::write_output 2\n");
}

void VRender::update_framerate()
{
	if(!framerate_counter)
	{
		mwindow->actual_framerate = (float)((int)mwindow->asset->frame_rate)  / 
			((float)framerate_timer->get_difference() / 1000);
		if(mwindow->settingsmenu->thread->thread_running)
		{
			mwindow->settingsmenu->thread->update_framerate();
		}
		framerate_counter = (int)mwindow->asset->frame_rate;
		framerate_timer->update();
	}
	framerate_counter--;
}

void VRender::update_tracking()
{
	double percentage = 0, seconds = 0;
	mwindow->video_file->lock_read();
	mwindow->video_file->get_position(&percentage, &seconds);
//printf("VRender::update_tracking 1 %f\n", percentage);
	mwindow->video_file->unlock_read();
	render_engine->playback_engine->update_tracking(percentage, seconds);
}

void VRender::run()
{
	startup_lock->unlock();
	first_frame = 1;

	while(!done)
	{
// Update the framerate counter
		update_framerate();


// Skip frames now
		if(render_engine->command->command == FRAME_REVERSE)
			mwindow->video_file->frame_back();
		else
		if(current_frame - last_frame > 1)
			mwindow->video_file->drop_frames(current_frame - last_frame - 1);
		last_frame = current_frame;

// Get output frame
		mwindow->gui->lock_window("VRender::run");
		VFrame *output_frame = mwindow->gui->get_output_frame();
		int in_y1 = 0, in_y2 = mwindow->asset->height;
		mwindow->get_cropping(in_y1, in_y2);
		mwindow->gui->unlock_window();



// Perform frame decompression
		mwindow->video_file->lock_read();
		mwindow->video_file->read_frame(output_frame, in_y1, in_y2);
		mwindow->video_file->unlock_read();


// Update tracking
		update_tracking();
		if(render_engine->command->single_frame())
		{
			write_output();
			done = 1;
		}
		else
		{
// Determine the delay until the frame needs to be shown.
			current_sample = render_engine->sync_sample();
// latest sample at which the frame can be shown.
			end_sample = Units::tosamples(current_frame, 
				mwindow->asset->rate, 
				mwindow->asset->frame_rate);
// earliest sample by which the frame needs to be shown.
			start_sample = Units::tosamples(current_frame - 1, 
				mwindow->asset->rate, 
				mwindow->asset->frame_rate);

			if(first_frame || end_sample < current_sample)
			{
// Frame rendered late or is first frame.  Flash it now.
				write_output();

				if(mwindow->every_frame)
				{
// User wants every frame.
					current_frame++;
				}
				else
				if(skip_countdown > 0)
				{
// Maybe just a freak.
					current_frame++;
					skip_countdown--;
				}
				else
				{
// Get the frames to skip.
					delay_countdown = VRENDER_THRESHOLD;
					int skip = 1 + 
						(long)Units::toframes(current_sample, 
							mwindow->asset->rate, 
							mwindow->asset->frame_rate) - 
						(long)Units::toframes(end_sample, 
							mwindow->asset->rate, 
							mwindow->asset->frame_rate);
					current_frame += skip;
				}
			}
			else
			{
// Frame rendered early or just in time.
				current_frame++;

				if(delay_countdown > 0)
				{
// Maybe just a freak
					delay_countdown--;
				}
				else
				{
					skip_countdown = VRENDER_THRESHOLD;
					if(start_sample > current_sample)
					{
// Came before the earliest sample so delay
						long delay = (long)((float)(start_sample - current_sample) / mwindow->asset->rate * 1000);
						timer->delay(delay);
					}
					else
					{
// Came after the earliest sample so keep going
					}
				}

// Flash frame now.
				write_output();
			}
		}

// Trigger audio to start
//printf("VRender::run 1\n");
		if(first_frame)
		{
			render_engine->first_frame_lock->unlock();
			first_frame = 0;
			render_engine->reset_sync_sample();
		}
//printf("VRender::run 2\n");


		if(mwindow->video_file->end_of_video()) done = 1;
	}

// In case we were interrupted before the first loop
	render_engine->first_frame_lock->unlock();
}
