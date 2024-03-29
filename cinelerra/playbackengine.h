/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef PLAYBACKENGINE_H
#define PLAYBACKENGINE_H

#include "arraylist.h"
#include "audiodevice.inc"
#include "cache.inc"
#include "canvas.inc"
#include "channeldb.inc"
#include "condition.inc"
#include "bchash.inc"
#include "edl.inc"
#include "mwindow.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "preferences.inc"
#include "previewer.inc"
#include "renderengine.inc"
#include "thread.h"
#include "bctimer.h"
#include "tracking.inc"
#include "transportque.inc"

class PlaybackEngine : public Thread
{
public:
	PlaybackEngine();
	virtual ~PlaybackEngine();

	void create_objects();
    void set_canvas(Canvas *output);
    void set_previewer(Previewer *previewer);
// draw plugin GUI's
    void set_use_gui(int value);
	virtual int create_render_engine();
	void delete_render_engine();
	int arm_render_engine();
	void start_render_engine();
	void wait_render_engine();
	void create_cache();
	void perform_change();
	void sync_parameters(EDL *edl);
// Set wait_tracking for events that change the cursor location but
// be sure to unlock the windows
	void interrupt_playback(int wait_tracking = 0);
// Get levels for tracking.  Return 0 if no audio.
	int get_output_levels(double *levels, long position);
	int get_module_levels(ArrayList<double> *module_levels, long position);
// The MWindow starts the playback cursor loop
// The other windows start a slider loop
// For pausing only the cursor is run
	virtual void init_cursor();
	virtual void stop_cursor();
	virtual int brender_available(long position);
// User overrides this to update the position displayed
	virtual void update_tracker(double position);
	void init_tracking();
	void stop_tracking();
// The playback cursor calls this to calculate the current tracking position
	virtual double get_tracking_position();
// Reset the transport after completion
	virtual void update_transport(int command, int paused);
// The render engines call this to update tracking variables in the playback engine.
	void update_tracking(double position);
// Get the output channel table for the current device or 0 if none exists.
	ChannelDB* get_channeldb();

	void run();

// enable hacks for Previewers
    Previewer *previewer;
// Maintain caches through console changes
	CICache *audio_cache, *video_cache;
    Tracking *tracking;
// Maintain playback cursor on GUI
	int tracking_active;
// Tracking variables updated by render engines
	double tracking_position;
// Not accurate until the first update_tracking, at which time
// tracking_active is incremented to 2.
	Timer tracking_timer;
// Lock access to tracking data
	Mutex *tracking_lock;
// Lock access to renderengine between interrupt and deletion
	Mutex *renderengine_lock;
// Block returns until tracking loop is finished
	Condition *tracking_done;
// Pause the main loop for the PAUSE command
	Condition *pause_lock;
// Wait until thread has started
	Condition *start_lock;

	Canvas *output;
// Copy of main preferences
	Preferences *preferences;
// Next command
	TransportQue *que;
// Currently executing command
	TransportCommand *command;
// Last command which affected transport
	int last_command;
	int done;
	int do_cwindow;
    int use_gui;
// Render engine
	RenderEngine *render_engine;

// Used by label commands to get current position
	int is_playing_back;

// General purpose debugging register
	int debug;
};





#endif
