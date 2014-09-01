#include "bcsignals.h"
#include "condition.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playbackscroll.h"
#include "bctimer.h"

#include <unistd.h>

PlaybackScroll::PlaybackScroll(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	pause_lock = new Condition(1, "PlaybackScroll::pause_lock");
	loop_lock = new Mutex("PlaybackScroll::loop_lock");
	startup_lock = new Condition(0, "PlaybackScroll::startup_lock");
	done = 0;
}

PlaybackScroll::~PlaybackScroll()
{
	done = 1;
//	Thread::cancel();
	Thread::join();
	delete pause_lock;
	delete loop_lock;
	delete startup_lock;
}

void PlaybackScroll::create_objects()
{
}


int PlaybackScroll::start_playback()
{
	done = 0;
	Thread::start();
	startup_lock->lock("PlaybackScroll::start_playback");
	return 0;
}

int PlaybackScroll::stop_playback()
{
	done = 1;
// Doesn't work in NPTL for some reason.
//	Thread::cancel();
	Thread::join();
	return 0;
}

void PlaybackScroll::run()
{
	double percentage, seconds;
	Timer timer;

	startup_lock->unlock();
	while(!done)
	{
		if(done) break;
		loop_lock->lock("PlaybackScroll::run");
		if(mwindow->engine->tracking_active)   
		{
			if(mwindow->engine)
				mwindow->engine->current_position(&percentage, &seconds);
			mwindow->gui->lock_window();
			mwindow->gui->update_position(percentage, seconds, 1);
			mwindow->gui->flush();
			mwindow->gui->unlock_window();
		}
		loop_lock->unlock();

		if(!done)
		{
			Thread::enable_cancel();
			timer.delay(100);
			Thread::disable_cancel();
		}
	}
}


