#ifndef ARENDER_H
#define ARENDER_H

#include "arender.inc"
#include "audiodevice.inc"
#include "file.inc"
#include "mwindow.inc"
#include "mutex.h"
#include "renderengine.inc"
#include "thread.h"

class ARender : public Thread
{
public:
	ARender(MWindow *mwindow, RenderEngine *render_engine);
	~ARender();

	void arm_playback();
	int start_playback();
	int interrupt_playback();
	int close_playback();
	void run();
	int wait_for_startup();
	int wait_for_completion();
	int reset_parameters();
	int renice();
	long sync_sample();
	void update_tracking();

	MWindow *mwindow;
	RenderEngine *render_engine;
	long buffer_length;
	char* buffer;
	long bytes;
	Mutex startup_lock;
	AudioDevice *audio;
	int buffer_size;
	int thread_done;
	int first_buffer;
};



#endif
