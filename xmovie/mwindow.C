#include "asset.h"
#include "bcsignals.h"
#include "clip.h"
#include "colormodels.h"
#include "condition.h"
#include "defaults.h"
#include "file.h"
#include "filesystem.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playbackscroll.h"
#include "playlist.h"
#include "settings.h"
#include "theme.h"
#include "transportque.h"
#include "vframe.h"



#include <string.h>
#include <unistd.h>

MWindow::MWindow(ArrayList<char*> *init_playlist)
{
	this->init_playlist = init_playlist;
	engine = new PlaybackEngine(this);
	defaults = new Defaults("~/.xmovierc");
	defaults->load();
	reset_parameters();
}

MWindow::~MWindow()
{
	delete error_thread;
	delete defaults;
	delete gui;
	if(asset) delete asset;
	if(audio_file) delete audio_file;
	if(video_file) delete video_file;
	if(frame) delete frame;
	if(playlist) delete playlist;
	if(engine)
	{
		engine->join();
		delete engine;
	}
	if(playback_scroll)
		delete playback_scroll;
	reset_parameters();
}

int MWindow::reset_parameters()
{
	audio_file = 0;
	video_file = 0;
	asset = 0;
	frame = 0;
	playlist = 0;
	current_position = 0;
	return 0;
}

int MWindow::create_objects()
{
	load_defaults();
// Create theme
	theme = new GoldTheme;

//printf("MWindow::create_objects %d %d\n", mwindow_x, mwindow_y);
	gui = new MWindowGUI(this, 
					mwindow_x,
					mwindow_y,
					mwindow_w, 
					mwindow_h);
	gui->create_objects();
	gui->load_defaults(defaults);

//gui->get_resources()->use_xvideo = 0;
	error_thread = new ErrorThread(gui);
	engine = new PlaybackEngine(this);
	playback_scroll = new PlaybackScroll(this);
	playback_scroll->create_objects();
	engine->start();
	engine->startup_lock->lock("MWindow::create_objects");
	gui->show_window();
	return 0;
}

int MWindow::calculate_smp()
{
/* Get processor count */
	int result = 1;
	FILE *proc;
	if(proc = fopen("/proc/cpuinfo", "r"))
	{
		char string[1024];
		while(!feof(proc))
		{
			fgets(string, 1024, proc);
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
	return result - 1;
}

void MWindow::load_defaults()
{
	sprintf(default_path, "");
	defaults->get("PATH", default_path);
	every_frame = defaults->get("EVERYFRAME", 0);
	mwindow_x = defaults->get("MWINDOW_X", BC_INFINITY);
	mwindow_y = defaults->get("MWINDOW_Y", BC_INFINITY);
	mwindow_w = defaults->get("MWINDOW_W", 640);
	mwindow_h = defaults->get("MWINDOW_H", 480);
	fullscreen = defaults->get("FULLSCREEN", 0);
	aspect_w = defaults->get("ASPECTW", (float)4);
	aspect_h = defaults->get("ASPECTH", (float)3);
	letter_w = defaults->get("LETTERBOXW", (float)2.35);
	letter_h = defaults->get("LETTERBOXH", (float)1);
	smp = calculate_smp();
	use_mmx = defaults->get("USE_MMX", 1);
	mix_strategy = defaults->get("MIX_STRATEGY", DOLBY51_TO_STEREO);
	square_pixels = defaults->get("ASPECTSQUARE", 0);
	crop_letterbox = defaults->get("CROPLETTERBOX", 0);
//	convert_601 = defaults->get("601TORGB", 1);
	convert_601 = 0;
	use_deblocking = defaults->get("USEDEBLOCKING", 1);
	audio_priority = defaults->get("AUDIOPRIORITY", 0);
	software_sync = defaults->get("SOFTWARESYNC", 0);
	prebuffer_size = defaults->get("PREBUFFER_SIZE", 0);
	video_stream = defaults->get("VIDEOSTREAM", 0);
	audio_stream = defaults->get("AUDIOSTREAM", 0);
	video_device = defaults->get("VIDEO_DEVICE", VDEVICE_X11);
	actual_framerate = defaults->get("ACTUAL_FRAMERATE", 0.0);
}

void MWindow::save_defaults()
{
	gui->menu->save_loads(defaults);
	gui->save_defaults(defaults);
	defaults->update("MWINDOW_X", mwindow_x);
	defaults->update("MWINDOW_Y", mwindow_y);
	defaults->update("MWINDOW_W", mwindow_w);
	defaults->update("MWINDOW_H", mwindow_h);
	defaults->update("FULLSCREEN", fullscreen);
	defaults->update("EVERYFRAME", every_frame);
	defaults->update("ASPECTW", aspect_w);
	defaults->update("ASPECTH", aspect_h);
	defaults->update("LETTERBOXW", letter_w);
	defaults->update("LETTERBOXH", letter_h);
	defaults->update("USE_MMX", use_mmx);
	defaults->update("MIX_STRATEGY", mix_strategy);
	defaults->update("ASPECTSQUARE", square_pixels);
	defaults->update("CROPLETTERBOX", crop_letterbox);
//	defaults->update("601TORGB", convert_601);
	defaults->update("USEDEBLOCKING", use_deblocking);
	defaults->update("AUDIOPRIORITY", audio_priority);
	defaults->update("PATH", default_path);
	defaults->update("SOFTWARESYNC", software_sync);
	defaults->update("PREBUFFER_SIZE", prebuffer_size);
	defaults->update("VIDEOSTREAM", video_stream);
	defaults->update("AUDIOSTREAM", audio_stream);
	defaults->update("VIDEO_DEVICE", video_device);
	defaults->update("ACTUAL_FRAMERATE", actual_framerate);
	defaults->save();
}

int MWindow::run_program()
{
	gui->run_window();
	return 0;
}

int MWindow::load_file(char *path, int use_locking)
{
	if(use_locking) gui->unlock_window();
	engine->que->send_command(STOP_PLAYBACK, 0);
	engine->interrupt_playback(1);
	if(use_locking) gui->lock_window();
	close_file();

// Check for playlist
	playlist = new Playlist;
	if(!playlist->load(path))
	{
		delete playlist;
		playlist = 0;
	}

	asset = new Asset(path);
	File *test_file = new File(this);

// Load asset with information
	test_file->set_prebuffer(prebuffer_size);
	test_file->set_processors(smp ? 2 : 1);
	test_file->set_mmx(use_mmx);
	int result = test_file->open_file(asset);

// Open real files
	if(!result)
	{
// success
		if(asset->audio_data)
		{
			audio_file = test_file;
			test_file = 0;
			if(audio_stream >= asset->audio_streams) audio_stream = 0;
			audio_file->set_audio_stream(audio_stream);
			gui->menu->update_audio_streams(asset->audio_streams);
		}

// Must open audio before this so FileMPEG can copy the tables
		if(asset->video_data)
		{
			if(test_file)
				video_file = test_file;
			else
			{
				video_file = new File(this);
				video_file->set_prebuffer(prebuffer_size);
				video_file->set_processors(smp ? 2 : 1);
				video_file->set_mmx(use_mmx);
				video_file->open_file(asset);
			}

			if(video_stream >= asset->video_streams) video_stream = 0;
			video_file->set_video_stream(video_stream);
			gui->menu->update_video_streams(asset->video_streams);
		}

		FileSystem fs;
		char string1[1024], string2[1024];
		fs.extract_name(string1, asset->path, 0);
		sprintf(string2, "XMovie: %s", string1);
		gui->set_title(string2);
		gui->resize_scrollbar();
		current_position = 0;
		gui->update_position(0, 0, 1);
		engine->que->send_command(CURRENT_FRAME, current_position);

// Put something on the canvas
		gui->lock_window();
		if(asset->video_data)
		{
			gui->canvas->flash();
		}
		else
		{
			theme->draw_canvas_bg(gui->canvas);
		}

		gui->menu->add_load(path);
		gui->unlock_window();
		save_defaults();
		return 0;
	}
	else
	{
// failure
		switch(result)
		{
			case 1:
				error_thread->show_error("No such file or directory.");
				break;
			case 2:
				error_thread->show_error("Unsupported file format.");
				break;
			case 3:
				error_thread->show_error("The file contains no supported codecs.");
				break;
		}
		delete asset;
		asset = 0;
		save_defaults();
		return 1;
	}
	return 0;
}

int MWindow::set_audio_stream(int stream_number)
{
	this->audio_stream = stream_number;
	save_defaults();
	audio_file->set_audio_stream(stream_number);
	gui->menu->update_audio_streams(asset->audio_streams);
	return 0;
}

int MWindow::set_video_stream(int stream_number)
{
	this->video_stream = stream_number;
	save_defaults();
	video_file->set_video_stream(stream_number);
	gui->menu->update_video_streams(asset->video_streams);
	return 0;
}

float MWindow::get_aspect_ratio()
{
	if(square_pixels && asset && asset->video_data)
		return (float)asset->width / asset->height;
	else
		return aspect_w / aspect_h;
	return 0;
}

int MWindow::original_size(float percent)
{
	if(asset && asset->video_data)
	{
		get_full_size(theme->canvas_w, theme->canvas_h);
		theme->canvas_w = (int)(theme->canvas_w * percent);
		theme->canvas_h = (int)(theme->canvas_h * percent);
		theme->update_positions_from_canvas(this, gui);
		gui->resize_window(mwindow_w, mwindow_h);
		gui->resize_widgets();
		gui->window_resized = 1;

		if(!gui->canvas->video_is_on())
		{
			double percentage, seconds;
			engine->current_position(&percentage, &seconds);
			engine->que->send_command(CURRENT_FRAME, current_position);
		}
	}
	return 0;
}

int MWindow::get_full_size(int &full_w, int &full_h)
{
	float aspect_ratio = get_aspect_ratio();
	float letterbox_ratio = letter_w / letter_h;

	if(!asset) return 1;

	if(aspect_ratio > (float)asset->width / asset->height)
	{
		full_w = (int)((float)asset->height * aspect_ratio + 0.5);
		full_h = asset->height;
	}
	else
	{
		full_w = asset->width;
		full_h = (int)((float)asset->width / aspect_ratio + 0.5);
	}

	if(crop_letterbox)
	{
		full_h = (int)(full_w / letterbox_ratio);
	}
	return 0;
}

int MWindow::get_cropping(int &y1, int &y2)
{
	int h, y;
	if(crop_letterbox && letter_w > letter_h)
	{
		float aspect_ratio = get_aspect_ratio();
		float letterbox_ratio = letter_w / letter_h;

		h = (int)(((float)asset->width / letterbox_ratio) / ((float)asset->width / aspect_ratio) * asset->height + 0.5);
		y = (int)((float)(asset->height - h) / 2 + 0.5);
	}
	else
	{
		h = asset->height;
		y = 0;
	}
	y1 = y;
	y2 = y + h;
	return 0;
}

int MWindow::close_file()
{
	if(audio_file)
	{
		audio_file->close_file();
		delete audio_file;
	}
	if(video_file)
	{
		video_file->close_file();
		delete video_file;
	}
	if(playlist)
	{
		delete playlist;
	}

	if(asset) delete asset;
	if(frame) delete frame;
	reset_parameters();
	return 0;
}

int MWindow::quit()
{
	save_defaults();
	exit(0);



	if(engine)
	{
		engine->interrupt_playback(1);
		engine->join();
		delete engine;
		engine = 0;
	}

	if(playback_scroll)
	{
		delete playback_scroll;
		playback_scroll = 0;
	}

	close_file();
	save_defaults();
	return 0;
}
