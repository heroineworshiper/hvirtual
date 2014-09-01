#include "arender.h"
#include "asset.h"
#include "bcsignals.h"
#include "condition.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "renderengine.h"
#include "playbackengine.h"
#include "playbackscroll.h"
#include "bctimer.h"
#include "transportque.h"
#include "vrender.h"


#include <unistd.h>


RenderEngine::RenderEngine(MWindow *mwindow, PlaybackEngine *playback_engine)
{
	reset();
	this->mwindow = mwindow;
	this->playback_engine = playback_engine;
	input_lock = new Mutex;
	command = new TransportCommand;
	*command = *playback_engine->command;
	set_synchronous(1);
	sync_timer = new Timer;
	interrupt_lock = new Mutex("RenderEngine::interrupt_lock");
	completion_lock = new Condition(1, "RenderEngine::completion_lock");
	seek_lock = new Mutex;
	first_frame_lock = new Condition(1, "RenderEngine::first_frame_lock");
}


RenderEngine::~RenderEngine()
{
	if(arender) delete arender;
	if(vrender) delete vrender;
	delete command;
	delete input_lock;
	delete sync_timer;
	delete interrupt_lock;
	delete completion_lock;
	delete seek_lock;
	delete first_frame_lock;
}

void RenderEngine::reset()
{
	arender = 0;
	vrender = 0;
}

void RenderEngine::run_command()
{
	sync_timer->update();
	completion_lock->lock("RenderEngine::run_command");
	Thread::start();
}

void RenderEngine::reset_sync_sample()
{
	sync_timer->update();
}

long RenderEngine::sync_sample()
{
	if(mwindow->software_sync || !arender)
	{
		return sync_timer->get_scaled_difference(mwindow->asset->rate);
	}
	else
		return arender->sync_sample();
}

void RenderEngine::interrupt_playback(int wait_engine)
{
	interrupt_lock->lock("RenderEngine::interrupt_playback");
	if(arender)
	{
		arender->interrupt_playback();
	}
	if(vrender)
	{
		vrender->interrupt_playback();
	}
	interrupt_lock->unlock();
//TRACE("RenderEngine::interrupt_playback 3");

	if(wait_engine)
	{
		completion_lock->lock("RenderEngine::interrupt_playback");
		completion_lock->unlock();
	}
//TRACE("RenderEngine::interrupt_playback 10");
}

void RenderEngine::run()
{
	interrupt_lock->lock("RenderEngine::run 1");
	if(mwindow->asset->video_data)
	{
		vrender = new VRender(mwindow, this);
		vrender->arm_playback();
		while(first_frame_lock->get_value() > 0) 
			first_frame_lock->lock("RenderEngine::run 1");
	}

// This one happens second so libmpeg3 can synchronize to the video
	if(mwindow->asset->audio_data)
	{
		arender = new ARender(mwindow, this);
		arender->arm_playback();
		if(!mwindow->asset->video_data)
			while(first_frame_lock->get_value() <= 0)
				first_frame_lock->unlock();
	}

	playback_engine->tracking_active = 1;
	if(!command->single_frame())
		mwindow->playback_scroll->start_playback();




	if(arender) arender->start_playback();
	if(vrender) vrender->start_playback();

	interrupt_lock->unlock();
	if(arender) arender->join();
	if(vrender) vrender->join();

	interrupt_lock->lock("RenderEngine::run 2");
	if(arender) arender->close_playback();
	if(vrender) vrender->close_playback();

	if(arender) delete arender;
	if(vrender) delete vrender;
	reset();
	interrupt_lock->unlock();

	playback_engine->tracking_active = 0;

// Overridden by a STOP_PLAYBACK command.
	double percentage, seconds;
	playback_engine->current_position(&percentage, &seconds);

// Only store resulting position if the command caused a change
	if(command->change_position())
	{
		mwindow->current_position = percentage;
	}

	if(!command->single_frame())
	{
		mwindow->playback_scroll->stop_playback();
	}
	else
	{
		mwindow->gui->lock_window();
		mwindow->gui->update_position(percentage, 
			seconds, 
			command->change_position());
		mwindow->gui->unlock_window();
	}

	mwindow->gui->lock_window();
	mwindow->gui->playbutton->set_mode(0);
	mwindow->gui->unlock_window();
	completion_lock->unlock();
}
