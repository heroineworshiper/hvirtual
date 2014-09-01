#include "arender.h"
#include "asset.h"
#include "audiodevice.h"
#include "condition.h"
#include "file.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "renderengine.h"
#include "transportque.h"
#include "vrender.h"

PlaybackEngine::PlaybackEngine(MWindow *mwindow)
 : Thread()
{
	reset_parameters();
	this->mwindow = mwindow;
	que = new TransportQue;
	command = new TransportCommand;
	tracking_lock = new Mutex("PlaybackEngine::tracking_lock");
	tracking_timer = new Timer;
	interrupt_lock = new Mutex("PlaybackEngine::interrupt_lock");
	startup_lock = new Condition(0, "PlaybackEngine::startup_lock");
	complete = new Condition(0, "PlaybackEngine::complete");
}

PlaybackEngine::~PlaybackEngine()
{
	done = 1;
	que->output_lock->unlock();
	complete->lock("PlaybackEngine::~PlaybackEngine");

	if(render_engine) delete render_engine;
	delete que;
	delete command;
	delete tracking_lock;
	delete tracking_timer;
	delete interrupt_lock;
	delete startup_lock;
	delete complete;
}

int PlaybackEngine::reset_parameters()
{
	tracking_position = 0;
	tracking_active = 0;
	done = 0;
	playing_back = 0;
	scroll = 0;
	render_engine = 0;
	return 0;
}

// Get position for scrollbar and time box
int PlaybackEngine::current_position(double *percentage, double *seconds)
{
	tracking_lock->lock("PlaybackEngine::current_position");
	*percentage = tracking_position;
	*seconds = tracking_time;

	if(tracking_active)
	{
		*seconds += (double)tracking_timer->get_difference() / 1000.0;
	}
	tracking_lock->unlock();

	return 0;
}

void PlaybackEngine::update_tracking(double tracking_position, double tracking_time)
{
	tracking_lock->lock("PlaybackEngine::update_tracking");
	this->tracking_position = tracking_position;
	this->tracking_time = tracking_time;
	tracking_timer->update();
	tracking_lock->unlock();
}

void PlaybackEngine::interrupt_playback(int wait_engine)
{
	interrupt_lock->lock("PlaybackEngine::interrupt_playback");
	if(render_engine) render_engine->interrupt_playback(wait_engine);
	interrupt_lock->unlock();
}

void PlaybackEngine::run()
{
	startup_lock->unlock();
	
	do
	{
		que->output_lock->lock("PlaybackEngine::run 1");

		if(done) continue;
		if(!mwindow->asset) continue;


// Wait for last command to finish
		if(render_engine)
		{
			render_engine->join();
			interrupt_lock->lock("PlaybackEngine::run 2");
			delete render_engine;
			render_engine = 0;
			interrupt_lock->unlock();
		}

// Load new command
		que->input_lock->lock("PlaybackEngine::run 3");
		*command = que->command;
		que->command.reset();
		que->input_lock->unlock();

// Execute command
		switch(command->command)
		{
			case STOP_PLAYBACK:
				tracking_position = command->start_position;
				tracking_timer->update();
// Remember end position
				mwindow->current_position = tracking_position;
				break;

			case CURRENT_FRAME:
			case PLAY_FORWARD:
			case FRAME_FORWARD:
			case FRAME_REVERSE:
// Override queued position
				if(command->command == FRAME_FORWARD ||
					command->command == FRAME_REVERSE)
				{
					command->start_position = mwindow->current_position;
				}

//printf("PlaybackEngine::run 4 %f %f\n", tracking_position, command->start_position);
				tracking_position = command->start_position;
				tracking_timer->update();
				interrupt_lock->lock("PlaybackEngine::run 4");
				render_engine = new RenderEngine(mwindow, this);
				render_engine->run_command();
				interrupt_lock->unlock();
				break;
		}
//printf("PlaybackEngine::run 5\n");
	}while(!done);

	complete->unlock();
}
