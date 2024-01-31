
/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef RENDERENGINE_H
#define RENDERENGINE_H


class RenderEngine;

#include "arender.inc"
#include "audiodevice.inc"
#include "cache.inc"
#include "canvas.inc"
#include "channel.inc"
#include "channeldb.inc"
#include "condition.inc"
#include "mutex.inc"
#include "playbackengine.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "thread.h"
#include "transportque.inc"
#include "videodevice.inc"
#include "vrender.inc"

class RenderEngine : public Thread
{
public:
	RenderEngine(PlaybackEngine *playback_engine, Preferences *preferences);
	~RenderEngine();

    void set_canvas(Canvas *output);
    void set_channeldb(ChannelDB *channeldb);
	void get_duty();
	void create_render_threads();
	void arm_render_threads();
	void start_render_threads();
	void wait_render_threads();
	void interrupt_playback();
	int get_output_w();
	int get_output_h();
	int brender_available(int position, int direction);
// Get current channel for the BUZ output
	Channel* get_current_channel();
	double get_tracking_position();
	CICache* get_acache();
	CICache* get_vcache();
// draw plugin GUI's
    void set_use_gui(int value);
    void set_nested(int value);
    void set_rendering(int value);
	void set_acache(CICache *cache);
	void set_vcache(CICache *cache);
    void set_vdevice(VideoDevice *vdevice);
// Get levels for tracking
	void get_output_levels(double *levels, int64_t position);
	void get_module_levels(ArrayList<double> *module_levels, int64_t position);
	EDL* get_edl();

	void run();
// Sends the command sequence, compensating for network latency
	int arm_command(TransportCommand *command);
// Start the command
	int start_command();

	int open_output();
	int close_output();
// return time in seconds to synchronize video against
	double sync_position();
// Called by VRender to reset the timers once the first frame is done.
	void reset_sync_position();
// return samples since start of playback
	int64_t session_position();

// Update preferences window
	void update_framerate(float framerate);

// Copy of command
	TransportCommand *command;
// Pointer to playback config for one head
	PlaybackConfig *config;
// Defined only for the master render engine
	PlaybackEngine *playback_engine;
// Copy of preferences
	Preferences *preferences;
// Canvas if being used for CWindow
	Canvas *output;

// Lock out new commands until completion
	Condition *input_lock;
// Lock out interrupts until started
	Condition *start_lock;
	Condition *output_lock;
// Lock out audio and synchronization timers until first frame is done
	Condition *first_frame_lock;
// Lock out interrupts before and after renderengine is active
	Mutex *interrupt_lock;



	int done;
	int is_nested;
    int is_rendering;
    int use_gui;
// If nested or rendering, the devices are owned by someone else
	AudioDevice *adevice;
	VideoDevice *vdevice;
	ARender *arender;
	VRender *vrender;
	int do_audio;
	int do_video;
// Timer for synchronization without audio
	Timer timer;
// If the termination came from interrupt or end of selection
	int interrupted;

// Channels for the BUZ output
	ChannelDB *channeldb;

// Samples in audio buffer to process
	int64_t fragment_len;
// Samples to send to audio device after speed adjustment
	int64_t adjusted_fragment_len;              
// CICaches for use if no playbackengine exists
	CICache *audio_cache, *video_cache;

	EDL *edl;
};








#endif
