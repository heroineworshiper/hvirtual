#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include "asset.inc"
#include "condition.inc"
#include "mutex.h"
#include "mwindow.inc"
#include "thread.h"
#include "bctimer.h"

#define TOTAL_BUFFERS 2

class AudioDevice : public Thread
{
public:
	AudioDevice(MWindow *mwindow, Asset *asset);
	~AudioDevice();

	int set_software_sync(int value);
// Returns number of samples in hardware buffer
	int start_playback();
	int interrupt_playback();     // Force break in playback
	int stop_playback();       // Flush the buffers
	int reset();
	long samples_rendered();
	int write_audio(char *buffer, long samples);
	int allocate_buffer(int buffer_num, long size);
	int next_buffer(int buffer_num);
	long samples_to_bytes(long samples);
	long get_fmt();
// Calculate channels based on mix strategy
	int get_channels();
	void run();

	Asset *asset;
	MWindow *mwindow;
	int dsp_out;
// synchronization
	int software_sync;
	long total_samples_written;
	long last_samples_written;
	long last_position;
	Timer timer;
	long actual_buffer;
	int playing_back;
	int current_inbuffer, current_outbuffer;
	char *output_buffers[TOTAL_BUFFERS];
	int done;
	long buffer_len[TOTAL_BUFFERS];
// Copy of mwindow parameter created at start
	int mix_strategy;

	Mutex *timer_lock;
	Condition *input_lock[TOTAL_BUFFERS];
	Condition *output_lock[TOTAL_BUFFERS];
};


#endif
