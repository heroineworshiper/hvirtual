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

#include "asset.h"
#include "audioconfig.h"
#include "audiodevice.inc"
#include "bcmeter.inc"
#include "bcsignals.h"
#include "cache.inc"
#include "clip.h"
#include "bchash.h"
#include "file.inc"
#include "filesystem.h"
#include "guicast.h"
#include "mutex.h"
#include "preferences.h"
#include "theme.h"
#include "videoconfig.h"
#include "videodevice.inc"
#include "playbackconfig.h"
#include "quicktime.h"
#include "recordconfig.h"

#include <string.h>
#include <unistd.h>


//#define CLAMP(x, y, z) (x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))






extern void get_exe_path(char *result);





Preferences::Preferences()
{
// Set defaults

	preferences_lock = new Mutex("Preferences::preferences_lock");



	get_exe_path(plugin_dir);

	index_directory.assign(BCASTDIR);
	if(index_directory.length() > 0)
	{
	    FileSystem fs;
    	fs.complete_path(&index_directory);
	}

    cache_size = 0xa00000;
	index_size = 0x300000;
	index_count = 100;
//	use_thumbnails = 1;
	theme[0] = 0;

    dump_playback = 0;
    use_gl_rendering = 0;
    use_hardware_decoding = 0;
    use_ffmpeg_mov = 0;
    show_fps = 0;

    align_deglitch = 1;
    align_synchronize = 0;
    align_extend = 0;

	use_renderfarm = 0;
	force_uniprocessor = 0;
	renderfarm_port = DEAMON_PORT;
	render_preroll = 0.5;
	brender_preroll = 0;
	renderfarm_mountpoint[0] = 0;
	renderfarm_vfs = 0;
	renderfarm_job_count = 20;
	processors = calculate_processors(0);
	real_processors = calculate_processors(1);

// Default brender asset
	brender_asset = new Asset;
	brender_asset->audio_data = 0;
	brender_asset->video_data = 1;
	sprintf(brender_asset->path, "/tmp/brender");
	brender_asset->format = FILE_JPEG_LIST;
	brender_asset->jpeg_quality = 80;

	use_brender = 0;
	brender_fragment = 1;
	local_rate = 0.0;

//	use_tipwindow = 0;
	override_dpi = 0;
	dpi = BASE_DPI;

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		for(int j = 0; j < i + 1; j++)
		{
			int position = 180 - (360 * j / (i + 1));
			while(position < 0) position += 360;
			channel_positions[i * MAXCHANNELS + j] = position;
		}
	}

	playback_config = new PlaybackConfig;
	aconfig_in = new AudioInConfig;
	vconfig_in = new VideoInConfig;
	recording_format = new Asset;
}

Preferences::~Preferences()
{
	brender_asset->Garbage::remove_user();
	delete preferences_lock;
	delete playback_config;
	recording_format->Garbage::remove_user();
	delete aconfig_in;
	delete vconfig_in;
}

void Preferences::dump()
{
    printf("Preferences::dump %d: %p\n", __LINE__, this);
    playback_config->dump();
}

char* Preferences::get_cwindow_display()
{
	if(playback_config->vconfig->x11_host[0])
		return playback_config->vconfig->x11_host;
	else
		return 0;
}

void Preferences::copy_rates_from(Preferences *preferences)
{
	preferences_lock->lock("Preferences::copy_rates_from");
// Need to match node titles in case the order changed and in case
// one of the nodes in the source is the master node.
	local_rate = preferences->local_rate;

	for(int j = 0; 
		j < preferences->renderfarm_nodes.total; 
		j++)
	{
		double new_rate = preferences->renderfarm_rate.values[j];
// Put in the master node
		if(preferences->renderfarm_nodes.values[j][0] == '/')
		{
			if(!EQUIV(new_rate, 0.0))
				local_rate = new_rate;
		}
		else
// Search for local node and copy it to that node
		if(!EQUIV(new_rate, 0.0))
		{
			for(int i = 0; i < renderfarm_nodes.total; i++)
			{
				if(!strcmp(preferences->renderfarm_nodes.values[j], renderfarm_nodes.values[i]) &&
					preferences->renderfarm_ports.values[j] == renderfarm_ports.values[i])
				{
					renderfarm_rate.values[i] = new_rate;
					break;
				}
			}
		}
	}

//printf("Preferences::copy_rates_from 1 %f %f\n", local_rate, preferences->local_rate);
	preferences_lock->unlock();
}

void Preferences::copy_from(Preferences *that)
{
// ================================= Performance ================================
	index_directory.assign(that->index_directory);
	index_size = that->index_size;
	index_count = that->index_count;
//	use_thumbnails = that->use_thumbnails;
	strcpy(theme, that->theme);

//	use_tipwindow = that->use_tipwindow;
	override_dpi = that->override_dpi;
	dpi = that->dpi;

	cache_size = that->cache_size;
	force_uniprocessor = that->force_uniprocessor;
	processors = that->processors;
	real_processors = that->real_processors;
	renderfarm_nodes.remove_all_objects();
	renderfarm_ports.remove_all();
	renderfarm_enabled.remove_all();
	renderfarm_rate.remove_all();
	local_rate = that->local_rate;
	for(int i = 0; i < that->renderfarm_nodes.size(); i++)
	{
		add_node(that->renderfarm_nodes.get(i), 
			that->renderfarm_ports.get(i),
			that->renderfarm_enabled.get(i),
			that->renderfarm_rate.get(i));
	}

	aconfig_in->copy_from(that->aconfig_in);
	vconfig_in->copy_from(that->vconfig_in);
	playback_config->copy_from(that->playback_config);
	playback_software_position = that->playback_software_position;
	view_follows_playback = that->view_follows_playback;
	real_time_playback = that->real_time_playback;
	scrub_chop = that->scrub_chop;
    memcpy(speed_table, that->speed_table, sizeof(speed_table));
	recording_format->copy_from(that->recording_format, 0);

    dump_playback = that->dump_playback;
    use_gl_rendering = that->use_gl_rendering;
    use_hardware_decoding = that->use_hardware_decoding;
    use_ffmpeg_mov = that->use_ffmpeg_mov;
    show_fps = that->show_fps;

    align_deglitch = that->align_deglitch;
    align_synchronize = that->align_synchronize;
    align_extend = that->align_extend;

	use_renderfarm = that->use_renderfarm;
	renderfarm_port = that->renderfarm_port;
	render_preroll = that->render_preroll;
	brender_preroll = that->brender_preroll;
	renderfarm_job_count = that->renderfarm_job_count;
	renderfarm_vfs = that->renderfarm_vfs;
	strcpy(renderfarm_mountpoint, that->renderfarm_mountpoint);
	renderfarm_consolidate = that->renderfarm_consolidate;
	use_brender = that->use_brender;
	brender_fragment = that->brender_fragment;
	brender_asset->copy_from(that->brender_asset, 0);

// Check boundaries

	FileSystem fs;
	if(index_directory.length() > 0)
	{
		fs.complete_path(&index_directory);
		fs.add_end_slash(&index_directory);
	}
	
// 	if(strlen(global_plugin_dir))
// 	{
// 		fs.complete_path(global_plugin_dir);
// 		fs.add_end_slash(global_plugin_dir);
// 	}
// 

// Redo with the proper value of force_uniprocessor
	if(force_uniprocessor && processors > 1)
	{
		processors = calculate_processors(0);
	}


	boundaries();
}

void Preferences::boundaries()
{
	renderfarm_job_count = MAX(renderfarm_job_count, 1);
	CLAMP(cache_size, MIN_CACHE_SIZE, MAX_CACHE_SIZE);
}

Preferences& Preferences::operator=(Preferences &that)
{
printf("Preferences::operator=\n");
	copy_from(&that);
	return *this;
}

void Preferences::print_channels(char *string, 
	int *channel_positions, 
	int channels)
{
	char string3[BCTEXTLEN];
	string[0] = 0;
	for(int j = 0; j < channels; j++)
	{
		sprintf(string3, "%d", channel_positions[j]);
		strcat(string, string3);
		if(j < channels - 1)
			strcat(string, ",");
	}
}

void Preferences::scan_channels(char *string, 
	int *channel_positions, 
	int channels)
{
	char string2[BCTEXTLEN];
	int len = strlen(string);
	int current_channel = 0;
	for(int i = 0; i < len; i++)
	{
		strcpy(string2, &string[i]);
		for(int j = 0; j < BCTEXTLEN; j++)
		{
			if(string2[j] == ',' || string2[j] == 0)
			{
				i += j;
				string2[j] = 0;
				break;
			}
		}
		channel_positions[current_channel++] = atoi(string2);
		if(current_channel >= channels) break;
	}
}

int Preferences::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	playback_config->load_defaults(defaults);
	view_follows_playback = defaults->get("VIEW_FOLLOWS_PLAYBACK", 1);
	real_time_playback = defaults->get("PLAYBACK_REALTIME", 0);
	scrub_chop = defaults->get("SCRUB_CHOP", 1);
    speed_table[SPEED_NUMPAD_1] = defaults->get("SPEED_NUMPAD_1", 0.0);
    speed_table[SPEED_NUMPAD_2] = defaults->get("SPEED_NUMPAD_2", 1.0);
    speed_table[SPEED_NUMPAD_3] = defaults->get("SPEED_NUMPAD_3", 2.0);
    speed_table[SPEED_NUMPAD_ENTER] = defaults->get("SPEED_NUMPAD_ENTER", 4.0);
    speed_table[SPEED_NUMPAD_DEL] = defaults->get("SPEED_NUMPAD_DEL", 0.5);
    speed_table[SPEED_BUTTON_1] = defaults->get("SPEED_BUTTON_1", 1.0);
    speed_table[SPEED_BUTTON_2] = defaults->get("SPEED_BUTTON_2", 2.0);
	playback_software_position = defaults->get("PLAYBACK_SOFTWARE_POSITION", 0);
	aconfig_in->load_defaults(defaults);
	vconfig_in->load_defaults(defaults);
// set some defaults that work
	recording_format->video_data = 1;
	recording_format->audio_data = 1;
	recording_format->format = FILE_MOV;
	strcpy(recording_format->acodec, QUICKTIME_TWOS);
	strcpy(recording_format->vcodec, QUICKTIME_JPEG);
	recording_format->channels = 2;
	recording_format->sample_rate = 48000;
	recording_format->bits = 16;
	recording_format->dither = 0;
	recording_format->load_defaults(defaults,
		"RECORD_", 
		1,
		1,
		1,
		1,
		1);

//	use_tipwindow = defaults->get("USE_TIPWINDOW", use_tipwindow);
	override_dpi = defaults->get("OVERRIDE_DPI", override_dpi);
	dpi = defaults->get("DPI", dpi);
//printf("Preferences::load_defaults %d dpi=%d\n", __LINE__, dpi);

	defaults->get("INDEX_DIRECTORY", &index_directory);
	index_size = defaults->get("INDEX_SIZE", index_size);
	index_count = defaults->get("INDEX_COUNT", index_count);
//	use_thumbnails = defaults->get("USE_THUMBNAILS", use_thumbnails);

//	sprintf(global_plugin_dir, PLUGIN_DIR);
//	defaults->get("GLOBAL_PLUGIN_DIR", global_plugin_dir);
//	if(getenv("GLOBAL_PLUGIN_DIR"))
//	{
//		strcpy(global_plugin_dir, getenv("GLOBAL_PLUGIN_DIR"));
//	}

	strcpy(theme, DEFAULT_THEME);
	defaults->get("THEME", theme);

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		char string2[BCTEXTLEN];
		sprintf(string, "CHANNEL_POSITIONS%d", i);
		print_channels(string2, 
			&channel_positions[i * MAXCHANNELS], 
			i + 1);

		defaults->get(string, string2);
		
		scan_channels(string2,
			&channel_positions[i * MAXCHANNELS], 
			i + 1);
	}

	brender_asset->load_defaults(defaults, 
		"BRENDER_", 
		1,
		1,
		1,
		0,
		0);



	force_uniprocessor = defaults->get("FORCE_UNIPROCESSOR", 0);
	use_brender = defaults->get("USE_BRENDER", use_brender);
	brender_fragment = defaults->get("BRENDER_FRAGMENT", brender_fragment);
	cache_size = defaults->get("CACHE_SIZE", cache_size);
	local_rate = defaults->get("LOCAL_RATE", local_rate);
    dump_playback = defaults->get("DUMP_PLAYBACK", dump_playback);
    use_gl_rendering = defaults->get("USE_GL_RENDERING", use_gl_rendering);
//    use_hardware_decoding = defaults->get("USE_HARDWARE_DECODING", use_hardware_decoding);
//    use_ffmpeg_mov = defaults->get("USE_FFMPEG_MOV", use_ffmpeg_mov);
// DEBUG
//use_ffmpeg_mov = 1;
    show_fps = defaults->get("SHOW_FPS", show_fps);

    align_deglitch = defaults->get("ALIGN_DEGLITCH", align_deglitch);
    align_synchronize = defaults->get("ALIGN_SYNCHRONIZE", align_synchronize);
    align_extend = defaults->get("ALIGN_EXTEND", align_extend);

	use_renderfarm = defaults->get("USE_RENDERFARM", use_renderfarm);
	renderfarm_port = defaults->get("RENDERFARM_PORT", renderfarm_port);
	render_preroll = defaults->get("RENDERFARM_PREROLL", render_preroll);
	brender_preroll = defaults->get("BRENDER_PREROLL", brender_preroll);
	renderfarm_job_count = defaults->get("RENDERFARM_JOBS_COUNT", renderfarm_job_count);
	renderfarm_consolidate = defaults->get("RENDERFARM_CONSOLIDATE", renderfarm_consolidate);
//	renderfarm_vfs = defaults->get("RENDERFARM_VFS", renderfarm_vfs);
	defaults->get("RENDERFARM_MOUNTPOINT", renderfarm_mountpoint);
	int renderfarm_total = defaults->get("RENDERFARM_TOTAL", 0);

	for(int i = 0; i < renderfarm_total; i++)
	{
		sprintf(string, "RENDERFARM_NODE%d", i);
		char result[BCTEXTLEN];
		int result_port = 0;
		int result_enabled = 0;
		float result_rate = 0.0;

		result[0] = 0;
		defaults->get(string, result);

		sprintf(string, "RENDERFARM_PORT%d", i);
		result_port = defaults->get(string, renderfarm_port);

		sprintf(string, "RENDERFARM_ENABLED%d", i);
		result_enabled = defaults->get(string, result_enabled);

		sprintf(string, "RENDERFARM_RATE%d", i);
		result_rate = defaults->get(string, result_rate);

		if(result[0] != 0)
		{
			add_node(result, result_port, result_enabled, result_rate);
		}
	}

// Redo with the proper value of force_uniprocessor
	processors = calculate_processors(0);

	boundaries();

	return 0;
}

int Preferences::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];


//	defaults->update("USE_TIPWINDOW", use_tipwindow);
	defaults->update("OVERRIDE_DPI", override_dpi);
	defaults->update("DPI", dpi);

	defaults->update("CACHE_SIZE", cache_size);
	defaults->update("INDEX_DIRECTORY", &index_directory);
	defaults->update("INDEX_SIZE", index_size);
	defaults->update("INDEX_COUNT", index_count);
//	defaults->update("USE_THUMBNAILS", use_thumbnails);
//	defaults->update("GLOBAL_PLUGIN_DIR", global_plugin_dir);
	defaults->update("THEME", theme);


	for(int i = 0; i < MAXCHANNELS; i++)
	{
		char string2[BCTEXTLEN];
		sprintf(string, "CHANNEL_POSITIONS%d", i);
		print_channels(string2, &channel_positions[i * MAXCHANNELS], i + 1);
		defaults->update(string, string2);
	}

	defaults->update("FORCE_UNIPROCESSOR", force_uniprocessor);
	brender_asset->save_defaults(defaults, 
		"BRENDER_",
		1,
		1,
		1,
		0,
		0);
	defaults->update("USE_BRENDER", use_brender);
	defaults->update("BRENDER_FRAGMENT", brender_fragment);
	defaults->update("DUMP_PLAYBACK", dump_playback);
	defaults->update("USE_GL_RENDERING", use_gl_rendering);
	defaults->update("USE_HARDWARE_DECODING", use_hardware_decoding);
	defaults->update("USE_FFMPEG_MOV", use_ffmpeg_mov);
	defaults->update("SHOW_FPS", show_fps);

    defaults->update("ALIGN_DEGLITCH", align_deglitch);
    defaults->update("ALIGN_SYNCHRONIZE", align_synchronize);
    defaults->update("ALIGN_EXTEND", align_extend);

	defaults->update("USE_RENDERFARM", use_renderfarm);
	defaults->update("LOCAL_RATE", local_rate);
	defaults->update("RENDERFARM_PORT", renderfarm_port);
	defaults->update("RENDERFARM_PREROLL", render_preroll);
	defaults->update("BRENDER_PREROLL", brender_preroll);
//	defaults->update("RENDERFARM_VFS", renderfarm_vfs);
	defaults->update("RENDERFARM_MOUNTPOINT", renderfarm_mountpoint);
	defaults->update("RENDERFARM_JOBS_COUNT", renderfarm_job_count);
	defaults->update("RENDERFARM_CONSOLIDATE", renderfarm_consolidate);
	defaults->update("RENDERFARM_TOTAL", (int64_t)renderfarm_nodes.total);
	for(int i = 0; i < renderfarm_nodes.total; i++)
	{
		sprintf(string, "RENDERFARM_NODE%d", i);
		defaults->update(string, renderfarm_nodes.values[i]);
		sprintf(string, "RENDERFARM_PORT%d", i);
		defaults->update(string, renderfarm_ports.values[i]);
		sprintf(string, "RENDERFARM_ENABLED%d", i);
		defaults->update(string, renderfarm_enabled.values[i]);
		sprintf(string, "RENDERFARM_RATE%d", i);
		defaults->update(string, renderfarm_rate.values[i]);
	}


	playback_config->save_defaults(defaults);
    defaults->update("PLAYBACK_REALTIME", real_time_playback);
    defaults->update("SCRUB_CHOP", scrub_chop);
    defaults->update("SPEED_NUMPAD_1", speed_table[SPEED_NUMPAD_1]);
    defaults->update("SPEED_NUMPAD_2", speed_table[SPEED_NUMPAD_2]);
    defaults->update("SPEED_NUMPAD_3", speed_table[SPEED_NUMPAD_3]);
    defaults->update("SPEED_NUMPAD_ENTER", speed_table[SPEED_NUMPAD_ENTER]);
    defaults->update("SPEED_NUMPAD_DEL", speed_table[SPEED_NUMPAD_DEL]);
    defaults->update("SPEED_BUTTON_1", speed_table[SPEED_BUTTON_1]);
    defaults->update("SPEED_BUTTON_2", speed_table[SPEED_BUTTON_2]);
    defaults->update("VIEW_FOLLOWS_PLAYBACK", view_follows_playback);
    defaults->update("PLAYBACK_SOFTWARE_POSITION", playback_software_position);
	recording_format->save_defaults(defaults,
		"RECORD_",
		1,
		1,
		1,
		1,
		1);
	aconfig_in->save_defaults(defaults);
    vconfig_in->save_defaults(defaults);
	return 0;
}


void Preferences::add_node(const char *text, int port, int enabled, float rate)
{
	if(text[0] == 0) return;

	preferences_lock->lock("Preferences::add_node");
	char *new_item = new char[strlen(text) + 1];
	strcpy(new_item, text);
	renderfarm_nodes.append(new_item);
	renderfarm_ports.append(port);
	renderfarm_enabled.append(enabled);
	renderfarm_rate.append(rate);
	preferences_lock->unlock();
}

void Preferences::delete_node(int number)
{
	preferences_lock->lock("Preferences::delete_node");
	if(number < renderfarm_nodes.total && number >= 0)
	{
		delete [] renderfarm_nodes.values[number];
		renderfarm_nodes.remove_number(number);
		renderfarm_ports.remove_number(number);
		renderfarm_enabled.remove_number(number);
		renderfarm_rate.remove_number(number);
	}
	preferences_lock->unlock();
}

void Preferences::delete_nodes()
{
	preferences_lock->lock("Preferences::delete_nodes");
	for(int i = 0; i < renderfarm_nodes.total; i++)
		delete [] renderfarm_nodes.values[i];
	renderfarm_nodes.remove_all();
	renderfarm_ports.remove_all();
	renderfarm_enabled.remove_all();
	renderfarm_rate.remove_all();
	preferences_lock->unlock();
}

void Preferences::reset_rates()
{
	for(int i = 0; i < renderfarm_nodes.total; i++)
	{
		renderfarm_rate.values[i] = 0.0;
	}
	local_rate = 0.0;
}

float Preferences::get_rate(int node)
{
	if(node < 0)
	{
		return local_rate;
	}
	else
	{
		int total = 0;
		for(int i = 0; i < renderfarm_nodes.size(); i++)
		{
			if(renderfarm_enabled.get(i)) total++;
			if(total == node + 1)
			{
				return renderfarm_rate.get(i);
			}
		}
	}
	
	return 0;
}

void Preferences::set_rate(float rate, int node)
{
//printf("Preferences::set_rate %f %d\n", rate, node);
	if(node < 0)
	{
		local_rate = rate;
	}
	else
	{
		int total = 0;
		for(int i = 0; i < renderfarm_nodes.size(); i++)
		{
			if(renderfarm_enabled.get(i)) total++;
			if(total == node + 1)
			{
				renderfarm_rate.set(i, rate);
				return;
			}
		}
	}
}

float Preferences::get_avg_rate(int use_master_node)
{
	preferences_lock->lock("Preferences::get_avg_rate");
	float total = 0.0;
	if(renderfarm_rate.total)
	{
		int enabled = 0;
		if(use_master_node)
		{
			if(EQUIV(local_rate, 0.0))
			{
				preferences_lock->unlock();
				return 0.0;
			}
			else
			{
				enabled++;
				total += local_rate;
			}
		}

		for(int i = 0; i < renderfarm_rate.total; i++)
		{
			if(renderfarm_enabled.values[i])
			{
				enabled++;
				total += renderfarm_rate.values[i];
				if(EQUIV(renderfarm_rate.values[i], 0.0)) 
				{
					preferences_lock->unlock();
					return 0.0;
				}
			}
		}

		if(enabled)
			total /= enabled;
		else
			total = 0.0;
	}
	preferences_lock->unlock();

	return total;
}

void Preferences::sort_nodes()
{
	int done = 0;

	while(!done)
	{
		done = 1;
		for(int i = 0; i < renderfarm_nodes.total - 1; i++)
		{
			if(strcmp(renderfarm_nodes.values[i], renderfarm_nodes.values[i + 1]) > 0)
			{
				char *temp = renderfarm_nodes.values[i];
				int temp_port = renderfarm_ports.values[i];

				renderfarm_nodes.values[i] = renderfarm_nodes.values[i + 1];
				renderfarm_nodes.values[i + 1] = temp;

				renderfarm_ports.values[i] = renderfarm_ports.values[i + 1];
				renderfarm_ports.values[i + 1] = temp_port;

				renderfarm_enabled.values[i] = renderfarm_enabled.values[i + 1];
				renderfarm_enabled.values[i + 1] = temp_port;

				renderfarm_rate.values[i] = renderfarm_rate.values[i + 1];
				renderfarm_rate.values[i + 1] = temp_port;
				done = 0;
			}
		}
	}
}

void Preferences::edit_node(int number, 
	const char *new_text, 
	int new_port, 
	int new_enabled)
{
	char *new_item = new char[strlen(new_text) + 1];
	strcpy(new_item, new_text);

	delete [] renderfarm_nodes.values[number];
	renderfarm_nodes.values[number] = new_item;
	renderfarm_ports.values[number] = new_port;
	renderfarm_enabled.values[number] = new_enabled;
}

int Preferences::get_enabled_nodes()
{
	int result = 0;
	for(int i = 0; i < renderfarm_enabled.total; i++)
		if(renderfarm_enabled.values[i]) result++;
	return result;
}

const char* Preferences::get_node_hostname(int number)
{
	int total = 0;
	for(int i = 0; i < renderfarm_nodes.total; i++)
	{
		if(renderfarm_enabled.values[i])
		{
			if(total == number)
				return renderfarm_nodes.values[i];
			else
				total++;
		}
	}
	return "";
}

int Preferences::get_node_port(int number)
{
	int total = 0;
	for(int i = 0; i < renderfarm_ports.total; i++)
	{
		if(renderfarm_enabled.values[i])
		{
			if(total == number)
				return renderfarm_ports.values[i];
			else
				total++;
		}
	}
	return -1;
}


int Preferences::calculate_processors(int interactive)
{
/* Get processor count */
	int result = 1;
	FILE *proc;



	if(force_uniprocessor && !interactive) return 1;

	if(proc = fopen("/proc/cpuinfo", "r"))
	{
		char string[BCTEXTLEN];
		while(!feof(proc))
		{
			char *temp = fgets(string, BCTEXTLEN, proc);
			if(!strncasecmp(string, "processor", 9))
			{
				char *ptr = strchr(string, ':');
				if(ptr)
				{
					ptr++;
					result = atol(ptr) + 1;
				}
			}
			else
			if(!strncasecmp(string, "cpus detected", 13))
			{
				char *ptr = strchr(string, ':');
				if(ptr)
				{
					ptr++;
					result = atol(ptr);
				}
			}
		}
		fclose(proc);
	}

	return result;
}


float Preferences::get_playback_value(int index)
{
    float value = speed_table[index];
// convert frame advance
    if(EQUIV(value, 0.0)) return 1.0;
// convert the rest
    return value;
}


int Preferences::get_playback_command(int index, int direction)
{
    float value = speed_table[index];
// printf("Preferences::get_playback_command %d index=%d value=%f\n", 
// __LINE__, index, value);
// convert frame advance
    if(EQUIV(value, 0.0))
    {
        if(direction == PLAY_FORWARD) 
            return SINGLE_FRAME_FWD;
        else
            return SINGLE_FRAME_REWIND;
    }

// convert the rest
    if(direction == PLAY_FORWARD)
        return PLAY_FWD;
    else
        return PLAY_REV;
}

static const float speeds[] = 
{ 
    0.0, 
    0.5, 
    1.0, 
    1.5, 
    2.0, 
    3.0, 
    4.0 
};
static const char* speed_texts[] = 
{
    "Frame",
    ".5x",
    "1x",
    "1.5x",
    "2x",
    "3x",
    "4x"
};

const char* Preferences::speed_to_text(float value)
{
    for(int i = 0; i < sizeof(speeds) / sizeof(float); i++)
        if(EQUIV(value, speeds[i])) return speed_texts[i];
    return speed_texts[2];
}

float Preferences::text_to_speed(const char *text)
{
    for(int i = 0; i < sizeof(speeds) / sizeof(float); i++)
        if(!strcmp(text, speed_texts[i])) return speeds[i];
    return speeds[2];
}

int Preferences::total_speeds()
{
    return sizeof(speeds) / sizeof(float);
}

float Preferences::speed(int index)
{
    return speeds[index];
}








