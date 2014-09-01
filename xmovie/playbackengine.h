#ifndef PLAYBACKENGINE_H
#define PLAYBACKENGINE_H

#include "arender.inc"
#include "condition.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "mutex.h"
#include "renderengine.inc"
#include "thread.h"
#include "bctimer.h"
#include "transportque.inc"
#include "vrender.inc"


class PlaybackEngine : public Thread
{
public:
	PlaybackEngine(MWindow *mwindow);
	~PlaybackEngine();

	void run();

	int reset_parameters();
	int close_playback();  // normal ending
	int wait_for_startup();
	int current_position(double *percentage, double *seconds);
	void update_tracking(double tracking_position, double tracking_time);
	long PlaybackEngine::current_sample();
	void interrupt_playback(int wait_engine);

// Next command
	TransportQue *que;
// Currently running command
	TransportCommand *command;

	RenderEngine *render_engine;
	int done;
	int playing_back;
	PlaybackScroll *scroll;
	MWindow *mwindow;
	Condition *startup_lock;
	Condition *complete;
// Starting sample for audio-only positioning
	long starting_sample;  
// Time of last tracking_position update
	Timer *tracking_timer;
// Use the timer only when this is 1
	int tracking_active;
	Mutex *tracking_lock;
	Mutex *interrupt_lock;
// Current position in percentage and seconds
	double tracking_position, tracking_time;
};




#endif
