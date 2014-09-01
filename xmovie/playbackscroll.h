#ifndef PLAYBACKSCROLL_H
#define PLAYBACKSCROLL_H

#include "condition.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "thread.h"





class PlaybackScroll : public Thread
{
public:
	PlaybackScroll(MWindow *mwindow);
	~PlaybackScroll();
	
	void create_objects();
	int start_playback();
	int stop_playback();
	void run();

	Condition *startup_lock;
	Condition *pause_lock;
	Mutex *loop_lock;
	MWindow *mwindow;
	int done;
};




#endif
