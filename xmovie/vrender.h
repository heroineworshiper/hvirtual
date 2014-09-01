#ifndef VRENDER_H
#define VRENDER_H

#include "mwindow.inc"
#include "mutex.inc"
#include "renderengine.inc"
#include "thread.h"
#include "bctimer.inc"


// Want to count down a certain number of late frames before
// we give up and start dropping.

class VRender : public Thread
{
public:
	VRender(MWindow *mwindow, RenderEngine *render_engine);
	~VRender();

	void arm_playback();
	int start_playback();
	int close_playback();
	int interrupt_playback();
	void run();
	int wait_for_startup();
	int wait_for_completion();
	void write_output();
	void update_tracking();
	void update_framerate();

	MWindow *mwindow;
	RenderEngine *render_engine;
	Mutex *startup_lock;
	Timer *timer;      // Delay
	Timer *framerate_timer;   // Calculate framerate
	int done;
	int framerate_counter;
	int first_frame;
	long current_frame, last_frame;
	long current_sample, last_sample;
	long end_sample, start_sample;
	int skip_countdown, delay_countdown;
};




#endif
