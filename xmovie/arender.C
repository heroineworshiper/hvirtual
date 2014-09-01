#include "asset.h"
#include "arender.h"
#include "audiodevice.h"
#include "condition.h"
#include "file.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "renderengine.h"
#include "transportque.h"

#include <sys/resource.h>
#include <unistd.h>

ARender::ARender(MWindow *mwindow, RenderEngine *render_engine)
{
	reset_parameters();
	this->mwindow = mwindow;
	this->render_engine = render_engine;
	set_synchronous(1);
}

ARender::~ARender()
{
}

int ARender::reset_parameters()
{
	audio = 0;
	buffer = 0;
	buffer_size = 0;
	return 0;
}

void ARender::arm_playback()
{
	int result = 0;
	thread_done = 0;

	if(!render_engine->command->single_frame())
	{

		if(!render_engine->command->change_position())
		{
			render_engine->seek_lock->lock();
			mwindow->audio_file->lock_read();
			mwindow->audio_file->set_audio_stream(mwindow->audio_stream);
// Percentage seeking not available for pure audio but extra commands
// are needed to synchronize the video.
			if(render_engine->vrender)
			{
				mwindow->audio_file->synchronize_position(mwindow->video_file);
				mwindow->audio_file->set_position(render_engine->command->start_position);
			}
			else
				mwindow->audio_file->set_audio_position((long)(render_engine->command->start_position *
					mwindow->audio_file->get_audio_length()));
			mwindow->audio_file->unlock_read();
			render_engine->seek_lock->unlock();
		}

		audio = new AudioDevice(mwindow, mwindow->asset);
		audio->set_software_sync(mwindow->software_sync);
		buffer_size = audio->start_playback();
		bytes = mwindow->asset->bits / 
			8 * 
			mwindow->asset->channels * 
			buffer_size;
		buffer = new char[bytes];
//printf("ARender::arm_playback %d\n", buffer_size);
	}

	startup_lock.lock();
}

int ARender::start_playback()
{
	
	Thread::start();
	return 0;
}

int ARender::interrupt_playback()
{
	thread_done = 1;
	if(audio) audio->interrupt_playback();
	return 0;
}

int ARender::close_playback()
{
	if(audio)
	{
		audio->stop_playback();
		delete audio;
		delete buffer;
	}
	reset_parameters();
	return 0;
}

int ARender::wait_for_startup()
{
	startup_lock.lock();
	startup_lock.unlock();
	return 0;
}

int ARender::wait_for_completion()
{
	join();
	return 0;
}

long ARender::sync_sample()
{
	if(audio)
		return audio->samples_rendered();
	else
		return 0;
}

int ARender::renice()
{
	if(setpriority(PRIO_PROCESS, getpid(), mwindow->audio_priority) < 0) 
	{
		perror("ARender::renice");
		return 1;
	}
	return 0;
}

void ARender::update_tracking()
{
	mwindow->audio_file->lock_read();
	long total = mwindow->audio_file->get_audio_length();
	mwindow->audio_file->unlock_read();

	double percentage, seconds;
	percentage = (double)sync_sample() / total + render_engine->command->start_position;
	seconds = percentage * total / mwindow->asset->rate;
	render_engine->playback_engine->update_tracking(percentage, seconds);
}


void ARender::run()
{
	renice();
	startup_lock.unlock();

	first_buffer = 1;

	while(audio && !thread_done && buffer_size)
	{
		buffer_length = buffer_size;

// Calculate position from starting point and device
		if(!render_engine->vrender)
		{
			update_tracking();
		}

		mwindow->audio_file->lock_read();
		mwindow->audio_file->read_audio(buffer, buffer_length);
		mwindow->audio_file->unlock_read();

// Wait until video is ready
		if(first_buffer)
		{
			render_engine->first_frame_lock->lock("ARender::process_buffer");
			first_buffer = 0;
		}

		if(!thread_done) audio->write_audio(buffer, buffer_length);
		if(!thread_done) thread_done = mwindow->audio_file->end_of_audio();
	}

	if(!render_engine->vrender)
		update_tracking();
// Flush the audio device
	if(audio && audio->playing_back) audio->write_audio(0, 0);
}

