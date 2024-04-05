/*
 * CINELERRA
 * Copyright (C) 2009-2022 Adam Williams <broadcast at earthling dot net>
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

#include "amodule.h"
#include "arender.h"
#include "asset.h"
#include "audiodevice.h"
#include "bcsignals.h"
#include "channeldb.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "mainsession.h"
#include "tracks.h"
#include "transportque.h"
#include "videodevice.h"
#include "vrender.h"
#include "workarounds.h"



RenderEngine::RenderEngine(PlaybackEngine *playback_engine,
 	Preferences *preferences)
 : Thread(1, 0, 0)
{
	this->playback_engine = playback_engine;
	channeldb = 0;
    output = 0;
	is_nested = 0;
    is_rendering = 0;
	adevice = 0;
	vdevice = 0;
	config = new PlaybackConfig;
	arender = 0;
	vrender = 0;
	do_audio = 0;
	do_video = 0;
	interrupted = 0;
 	this->preferences = new Preferences;
 	this->command = new TransportCommand;
 	this->preferences->copy_from(preferences);
	edl = 0;
    use_gui = 0;

	audio_cache = 0;
	video_cache = 0;


	input_lock = new Condition(1, "RenderEngine::input_lock");
	start_lock = new Condition(1, "RenderEngine::start_lock");
	output_lock = new Condition(1, "RenderEngine::output_lock");
	interrupt_lock = new Mutex("RenderEngine::interrupt_lock");
	first_frame_lock = new Condition(1, "RenderEngine::first_frame_lock");
}

RenderEngine::~RenderEngine()
{
	close_output();
	delete command;
	delete preferences;
	if(arender) delete arender;
	if(vrender) delete vrender;
	delete input_lock;
	delete start_lock;
	delete output_lock;
	delete interrupt_lock;
	delete first_frame_lock;
	delete config;
	edl->Garbage::remove_user();
}


void RenderEngine::set_canvas(Canvas *output)
{
    this->output = output;
}



void RenderEngine::set_channeldb(ChannelDB *channeldb)
{
	this->channeldb = channeldb;
}

void RenderEngine::set_use_gui(int value)
{
    this->use_gui = value;
}

EDL* RenderEngine::get_edl()
{
//	return command->get_edl();
	return edl;
}

int RenderEngine::arm_command(TransportCommand *command)
{
    int result = 0;
	const int debug = 0;
// Prevent this renderengine from accepting another command until finished.
// Since the renderengine is often deleted after the input_lock command it must
// be locked here as well as in the calling routine.
	if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);


	input_lock->lock("RenderEngine::arm_command");

// printf("RenderEngine::arm_command %d edl=%p channels=%d\n", 
// __LINE__, 
// edl, 
// command->get_edl()->session->audio_channels);
	if(!edl)
	{
		edl = new EDL;
		edl->create_objects();
		edl->copy_all(command->get_edl());
	}
	this->command->copy_from(command);

// printf("RenderEngine::arm_command %d channels=%d\n", 
// __LINE__, 
// get_edl()->session->audio_channels);
// Fix background rendering asset to use current dimensions and ignore
// headers.
	preferences->brender_asset->frame_rate = command->get_edl()->session->frame_rate;
	preferences->brender_asset->width = command->get_edl()->session->output_w;
	preferences->brender_asset->height = command->get_edl()->session->output_h;
	preferences->brender_asset->use_header = 0;
	preferences->brender_asset->layers = 1;
	preferences->brender_asset->video_data = 1;


	done = 0;
	interrupted = 0;

// Retool configuration for this node
	this->config->copy_from(preferences->playback_config);
	VideoOutConfig *vconfig = this->config->vconfig;
	AudioOutConfig *aconfig = this->config->aconfig;
	if(command->realtime)
	{
		if(command->single_frame() && 
            vconfig->driver != PLAYBACK_X11_GL &&
            vconfig->driver != PLAYBACK_PREVIEW)
		{
			vconfig->driver = PLAYBACK_X11;
		}
	}
	else
	{
		vconfig->driver = PLAYBACK_X11;
	}

	if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);


	get_duty();

	if(do_audio)
	{
		fragment_len = aconfig->fragment_size;
// Larger of audio_module_fragment and fragment length adjusted for speed
// Extra memory must be allocated for rendering slow motion.
		adjusted_fragment_len = (int64_t)((float)aconfig->fragment_size / 
			command->get_speed() + 0.5);
		if(adjusted_fragment_len < aconfig->fragment_size)
			adjusted_fragment_len = aconfig->fragment_size;
	}

// Set lock so audio doesn't start until video has started.
	if(do_video)
	{
		while(first_frame_lock->get_value() > 0) 
			first_frame_lock->lock("RenderEngine::arm_command");
	}
	else
// Set lock so audio doesn't wait for video which is never to come.
	{
		while(first_frame_lock->get_value() <= 0)
			first_frame_lock->unlock();
	}
	if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);

	result = open_output();
	if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);

    if(!result)
    {
	    if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);
	    create_render_threads();
	    if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);
	    arm_render_threads();
	    if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);
    }
    else
    {
    	input_lock->unlock();
    }
	if(debug) printf("RenderEngine::arm_command %d\n", __LINE__);

	return result;
}

void RenderEngine::get_duty()
{
	do_audio = 0;
	do_video = 0;

//printf("RenderEngine::get_duty %d\n", __LINE__);
	if(!command->single_frame() &&
		get_edl()->tracks->playable_audio_tracks() &&
		get_edl()->session->audio_channels)
	{
		do_audio = 1;
	}

//printf("RenderEngine::get_duty %d\n", __LINE__);
	if(get_edl()->tracks->playable_video_tracks())
	{
//printf("RenderEngine::get_duty %d\n", __LINE__);
		do_video = 1;
	}
}

void RenderEngine::create_render_threads()
{
	if(do_video && !vrender)
	{
		vrender = new VRender(this);
	}

	if(do_audio && !arender)
	{
		arender = new ARender(this);
	}
}


int RenderEngine::get_output_w()
{
	return get_edl()->session->output_w;
}

int RenderEngine::get_output_h()
{
	return get_edl()->session->output_h;
}

int RenderEngine::brender_available(int position, int direction)
{
	if(playback_engine)
	{
		int64_t corrected_position = position;
		if(direction == PLAY_REVERSE)
			corrected_position--;
		return playback_engine->brender_available(corrected_position);
	}
	else
		return 0;
}

Channel* RenderEngine::get_current_channel()
{
	if(channeldb)
	{
		switch(config->vconfig->driver)
		{
			case PLAYBACK_BUZ:
				if(config->vconfig->buz_out_channel >= 0 && 
					config->vconfig->buz_out_channel < channeldb->size())
				{
					return channeldb->get(config->vconfig->buz_out_channel);
				}
				break;
			case VIDEO4LINUX2JPEG:
				break;
		}
	}
	return 0;
}

CICache* RenderEngine::get_acache()
{
	if(playback_engine)
		return playback_engine->audio_cache;
	else
		return audio_cache;
}

CICache* RenderEngine::get_vcache()
{
	if(playback_engine)
		return playback_engine->video_cache;
	else
		return video_cache;
}

void RenderEngine::set_nested(int value)
{
    this->is_nested = value;
}

void RenderEngine::set_rendering(int value)
{
    this->is_rendering = value;
}

void RenderEngine::set_acache(CICache *cache)
{
	this->audio_cache = cache;
}

void RenderEngine::set_vcache(CICache *cache)
{
	this->video_cache = cache;
}

void RenderEngine::set_vdevice(VideoDevice *vdevice)
{
    this->vdevice = vdevice;
}


double RenderEngine::get_tracking_position()
{
	if(playback_engine) 
		return playback_engine->get_tracking_position();
	else
		return 0;
}

int RenderEngine::open_output()
{
    int result = 0;
	if(command->realtime && !is_nested && !is_rendering)
	{
// Allocate devices
		if(do_audio)
		{
			adevice = new AudioDevice;
		}

		if(do_video)
		{
			vdevice = new VideoDevice(MWindow::instance);
		}

// Initialize sharing


// Start playback
		if(do_audio && do_video)
		{
			vdevice->set_adevice(adevice);
			adevice->set_vdevice(vdevice);
		}



// Retool playback configuration
		if(do_audio)
		{

// printf("RenderEngine::open_output %d: sample_rate=%d channels=%d fragment=%d\n",
// __LINE__,
// (int)get_edl()->session->sample_rate, 
// (int)get_edl()->session->audio_channels,
// (int)adjusted_fragment_len);

			if(adevice->open_output(config->aconfig, 
				get_edl()->session->sample_rate, 
				adjusted_fragment_len,
				get_edl()->session->audio_channels,
				preferences->real_time_playback))
				do_audio = 0;
			else
			{
				adevice->set_software_positioning(
					preferences->playback_software_position);
				adevice->start_playback();
			}
		}

		if(do_video)
		{
			result = vdevice->open_output(config->vconfig, 
				get_edl()->session->frame_rate,
				get_output_w(),
				get_output_h(),
				output,
				command->single_frame());
            if(!result)
            {
			    Channel *channel = get_current_channel();
			    if(channel) vdevice->set_channel(channel);
			    vdevice->set_quality(80);
			    vdevice->set_cpus(preferences->processors);
                if(playback_engine) vdevice->set_previewer(playback_engine->previewer);
            }
            else
            {
                delete vdevice;
                vdevice = 0;
            }
		}
	}

	return result;
}

void RenderEngine::reset_sync_position()
{
	timer.update();
}

double RenderEngine::sync_position()
{
// Use audio device
// No danger of race conditions because the output devices are closed after all
// threads join.
	if(do_audio)
	{
		return (double)adevice->current_position() /
            get_edl()->session->sample_rate;
	}

// use a software timer
	if(do_video)
	{
		return (double)timer.get_difference() / 1000;
	}
    return 0;
}


int RenderEngine::start_command()
{
	if(command->realtime && !is_nested)
	{
		interrupt_lock->lock("RenderEngine::start_command");
		start_lock->lock("RenderEngine::start_command 1");
		Thread::start();
		start_lock->lock("RenderEngine::start_command 2");
		start_lock->unlock();
	}
	return 0;
}

void RenderEngine::arm_render_threads()
{
	if(do_audio)
	{
		arender->arm_command();
	}

	if(do_video)
	{
		vrender->arm_command();
	}
}


void RenderEngine::start_render_threads()
{
// Synchronization timer.  Gets reset once again after the first video frame.
	timer.update();

	if(do_audio)
	{
		arender->start_command();
	}

	if(do_video)
	{
		vrender->start_command();
	}
}

void RenderEngine::update_framerate(float framerate)
{
	MWindow::instance->session->actual_frame_rate = framerate;
}

void RenderEngine::wait_render_threads()
{
	if(do_audio)
	{
		arender->Thread::join();
	}

	if(do_video)
	{
		vrender->Thread::join();
	}
}

void RenderEngine::interrupt_playback()
{
	interrupt_lock->lock("RenderEngine::interrupt_playback");
	interrupted = 1;
	if(adevice)
	{
		adevice->interrupt_playback();
	}
	if(vdevice)
	{
		vdevice->interrupt_playback();
	}
	interrupt_lock->unlock();
}

int RenderEngine::close_output()
{
// Nested engines share devices
	if(!is_nested && !is_rendering)
	{
		if(adevice)
		{
			adevice->close_all();
			delete adevice;
			adevice = 0;
		}



		if(vdevice)
		{
			vdevice->close_all();
			delete vdevice;
			vdevice = 0;
		}
	}

	return 0;
}

void RenderEngine::get_output_levels(double *levels, int64_t position)
{
	if(do_audio)
	{
		int history_entry = arender->get_history_number(arender->level_samples, 
			position);
		for(int i = 0; i < MAXCHANNELS; i++)
		{
			if(arender->audio_out[i])
				levels[i] = arender->level_history[i][history_entry];
		}
	}
}

void RenderEngine::get_module_levels(ArrayList<double> *module_levels, int64_t position)
{
	if(do_audio)
	{
		for(int i = 0; i < arender->total_modules; i++)
		{
//printf("RenderEngine::get_module_levels %p %p\n", ((AModule*)arender->modules[i]), ((AModule*)arender->modules[i])->level_samples);
			int history_entry = arender->get_history_number(((AModule*)arender->modules[i])->level_samples, position);

			module_levels->append(((AModule*)arender->modules[i])->level_history[history_entry]);
		}
	}
}





void RenderEngine::run()
{
	start_render_threads();
	start_lock->unlock();
	interrupt_lock->unlock();

	wait_render_threads();

	interrupt_lock->lock("RenderEngine::run");


	if(interrupted && playback_engine)
	{
		playback_engine->tracking_position = playback_engine->get_tracking_position();
	}

	close_output();

// Fix the tracking position
	if(playback_engine)
	{
		if(command->command == CURRENT_FRAME)
		{
//printf("RenderEngine::run %d %d\n", __LINE__, (int)playback_engine->tracking_position);
			playback_engine->tracking_position = command->playbackstart;
		}
		else
		{
// Make sure transport doesn't issue a pause command next
//printf("RenderEngine::run 4.1 %d\n", playback_engine->tracking_position);
			if(!interrupted)
			{
				if(do_audio)
				{
                	playback_engine->tracking_position = 
						(double)arender->current_position / 
							command->get_edl()->session->sample_rate;
				}
                else
				if(do_video)
				{
					playback_engine->tracking_position = 
						(double)vrender->current_position / 
							command->get_edl()->session->frame_rate;
				}
			}

			if(!interrupted) playback_engine->command->command = STOP;

			playback_engine->stop_tracking();

		}
		playback_engine->is_playing_back = 0;
	}

	input_lock->unlock();
	interrupt_lock->unlock();
}



