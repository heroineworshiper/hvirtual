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

// global options changed in the preferences window

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "asset.inc"
#include "audioconfig.inc"
#include "bchash.inc"
#include "guicast.h"
#include "maxchannels.h"
#include "mutex.inc"
#include "preferences.inc"
#include "transportque.inc"
#include "videoconfig.inc"

#include <string>
using std::string;


class Preferences
{
public:
	Preferences();
	~Preferences();

	Preferences& operator=(Preferences &that);
	void copy_from(Preferences *that);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
    void dump();
	void boundaries();
// Used by CWindowGUI during initialization.
	char* get_cwindow_display();

	static void print_channels(char *string, 
		int *channel_positions, 
		int channels);
	static void scan_channels(char *string, 
		int *channel_positions, 
		int channels);

	void add_node(const char *text, int port, int enabled, float rate);
	void delete_node(int number);
	void delete_nodes();
	void reset_rates();
// Get average frame rate or 1.0
	float get_avg_rate(int use_master_node);
	void sort_nodes();
	void edit_node(int number, const char *new_text, int port, int enabled);
	int get_enabled_nodes();
	const char* get_node_hostname(int number);
	int get_node_port(int number);
// Copy frame rates.  Always used where the argument is the renderfarm and this is
// the master preferences.  This way, the value for master node is properly 
// translated from a unix socket to the local_rate.
	void copy_rates_from(Preferences *preferences);
// Set frame rate for a node.  Node -1 is the master node.
// The node number is relative to the enabled nodes.
	void set_rate(float rate, int node);
	float get_rate(int node);
// Calculate the number of cpus to use.  
// Determined by /proc/cpuinfo and force_uniprocessor.
// interactive forces it to ignore force_uniprocessor
	int calculate_processors(int interactive = 0);

// translate speed table to a command & value
    float get_playback_value(int index);
    int get_playback_command(int index, int direction);
// access to all possible speeds
    static const char* speed_to_text(float speed);
    static float text_to_speed(const char *text);
    static int total_speeds();
    static float speed(int index);

// ================================= Performance ================================
// directory to look in for indexes
	string index_directory;   
// size of index file in bytes
	int64_t index_size;                  
	int index_count;
// Use thumbnails in AWindow assets.
	int use_thumbnails;
// Title of theme
	char theme[BCTEXTLEN];
	double render_preroll;
	int brender_preroll;
	int force_uniprocessor;
// The number of cpus to use when rendering.
// Determined by /proc/cpuinfo and force_uniprocessor
	int processors;
// Number of processors for interactive operations.
	int real_processors;

// Default positions for channels
	int channel_positions[MAXCHANNELS * MAXCHANNELS];

	Asset *brender_asset;
	int use_brender;
// Number of frames in a brender job.
	int brender_fragment;
// Size of cache in bytes.
// Several caches of cache_size exist so multiply by 4.
// rendering, playback, timeline, preview
	int64_t cache_size;

// dump playback steps to the console
    int dump_playback;

    int use_gl_rendering;
// sometimes it's faster.  Sometimes it's slower depending on the hardware.
    int use_hardware_decoding;
// use ffmpeg to read quicktime/mp4
    int use_ffmpeg_mov;
// Show FPS in compositor
    int show_fps;

// parameters for align edits
    int align_deglitch;
    int align_synchronize;
    int align_extend;

	int use_renderfarm;
	int renderfarm_port;
// If the node starts with a / it's on the localhost using a path as the socket.
	ArrayList<char*> renderfarm_nodes;
	ArrayList<int>   renderfarm_ports;
	ArrayList<int>   renderfarm_enabled;
	ArrayList<float> renderfarm_rate;
// Rate of master node
	float local_rate;
	char renderfarm_mountpoint[BCTEXTLEN];
// Use virtual filesystem
	int renderfarm_vfs;
// Jobs per node
	int renderfarm_job_count;
// Consolidate output files
	int renderfarm_consolidate;

	int override_dpi;
	int dpi;

	PlaybackConfig *playback_config;
	VideoInConfig *vconfig_in;
	AudioInConfig *aconfig_in;
	Asset *recording_format;
	int view_follows_playback;
	int playback_software_position;
// Play audio in realtime priority
	int real_time_playback;
// scrubbing style
    int scrub_chop;
// scrubbing speeds.  0.0 is frame advance
    float speed_table[TOTAL_SPEEDS];

// ====================================== Plugin Set ==============================
	char plugin_dir[BCTEXTLEN];

// Required when updating renderfarm rates
	Mutex *preferences_lock;
};

#endif
