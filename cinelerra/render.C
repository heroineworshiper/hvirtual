/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "arender.h"
#include "asset.h"
#include "auto.h"
#include "awindow.h"
#include "awindowgui.h"
#include "batchrender.h"
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "compresspopup.h"
#include "condition.h"
#include "confirmsave.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "bchash.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "formatcheck.h"
#include "formatpopup.h"
#include "formattools.h"
#include "indexable.h"
#include "labels.h"
#include "language.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "module.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "patchbay.h"
#include "playabletracks.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "quicktime.h"
#include "renderfarm.h"
#include "render.h"
#include "statusbar.h"
#include "theme.h"
#include "timebar.h"
#include "tracks.h"
#include "transportque.h"
#include "vedit.h"
#include "vframe.h"
#include "videoconfig.h"
#include "vrender.h"

#include <ctype.h>
#include <string.h>



RenderItem::RenderItem(MWindow *mwindow)
 : BC_MenuItem(_("Render..."), "Shift+R", 'R')
{
	this->mwindow = mwindow;
	set_shift(1);
}

int RenderItem::handle_event() 
{
	mwindow->gui->unlock_window();
	mwindow->render->start_interactive();
	mwindow->gui->lock_window("RenderItem::handle_event");
	return 1;
}










RenderProgress::RenderProgress(MWindow *mwindow, Render *render)
 : Thread()
{
	this->mwindow = mwindow;
	this->render = render;
	last_value = 0;
	Thread::set_synchronous(1);
}

RenderProgress::~RenderProgress()
{
	Thread::cancel();
	Thread::join();
}


void RenderProgress::run()
{
	Thread::disable_cancel();
	while(1)
	{
		if(render->total_rendered != last_value)
		{
			render->progress->update(render->total_rendered);
			last_value = render->total_rendered;
			
			if(mwindow) mwindow->preferences_thread->update_rates();
		}

		Thread::enable_cancel();
		sleep(1);
		Thread::disable_cancel();
	}
}










MainPackageRenderer::MainPackageRenderer(Render *render)
 : PackageRenderer()
{
	this->render = render;
}



MainPackageRenderer::~MainPackageRenderer()
{
}


int MainPackageRenderer::get_master()
{
	return 1;
}

int MainPackageRenderer::get_result()
{
	return render->result;
}

void MainPackageRenderer::set_result(int value)
{
	if(value)
		render->result = value;
	
	
	
}

void MainPackageRenderer::set_progress(int64_t value)
{
	render->counter_lock->lock("MainPackageRenderer::set_progress");
// Increase total rendered for all nodes
	render->total_rendered += value;

// Update frames per second for master node
	render->preferences->set_rate(frames_per_second, -1);

//printf("MainPackageRenderer::set_progress %d %ld %f\n", __LINE__, (long)value, frames_per_second);

// If non interactive, print progress out
	if(!render->progress)
	{
		int64_t current_eta = render->progress_timer->get_scaled_difference(1000);
		if(current_eta - render->last_eta > 1000)
		{
			double eta = 0;


			if(render->total_rendered)
			{
				eta = current_eta /
					1000 *
					render->progress_max /
					render->total_rendered -
					current_eta /
					1000;
			}

			char string[BCTEXTLEN];
			Units::totext(string, 
				eta,
				TIME_HMS2);

			printf("\r%d%% ETA: %s      ", (int)(100 * 
				(float)render->total_rendered / 
					render->progress_max),
				string);
			fflush(stdout);
			render->last_eta = current_eta;
		}
	}

	render->counter_lock->unlock();

// This locks the preferences
	if(mwindow) mwindow->preferences->copy_rates_from(preferences);
}

int MainPackageRenderer::progress_cancelled()
{

// printf("MainPackageRenderer::progress_cancelled %d progress canceled=%d batch canceled=%d\n", 
// __LINE__, 
// render->progress ? render->progress->is_cancelled() : 0,
// render->batch_cancelled);

	return (render->progress && render->progress->is_cancelled()) || 
		render->batch_cancelled;
}












Render::Render(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	in_progress = 0;
	progress = 0;
	preferences = 0;
	elapsed_time = 0.0;
	package_lock = new Mutex("Render::package_lock");
	counter_lock = new Mutex("Render::counter_lock");
	completion = new Condition(0, "Render::completion");
	progress_timer = new Timer;
	thread = new RenderThread(mwindow, this);
	asset = 0;
	result = 0;
}

Render::~Render()
{
	delete package_lock;
	delete counter_lock;
	delete completion;
// May be owned by someone else.  This is owned by mwindow, so we don't care
// about deletion.
//	delete preferences;
	delete progress_timer;
	asset->Garbage::remove_user();
	delete thread;
}

void Render::start_interactive()
{
	if(!thread->running())
	{
		mode = Render::INTERACTIVE;
		BC_DialogThread::start();
	}
	else
	{
		ErrorBox error_box(PROGRAM_NAME ": Error",
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
		error_box.create_objects("Already rendering");
		error_box.raise_window();
		error_box.run_window();
	}
}

// when rendering from the GUI
void Render::start_batches(ArrayList<BatchRenderJob*> *jobs)
{
//printf("Render::start_batches %d running=%d\n", __LINE__, thread->running());
	if(!thread->running())
	{
		mode = Render::BATCH;
		batch_cancelled = 0;
		in_progress = 1;
		result = 0;
		this->jobs = jobs;
		completion->reset();
		start_render();
	}
}

// when rendering from the command line
void Render::start_batches(ArrayList<BatchRenderJob*> *jobs,
	BC_Hash *boot_defaults,
	Preferences *preferences)
{
	mode = Render::BATCH;
	batch_cancelled = 0;
	in_progress = 1;
	this->jobs = jobs;
	this->preferences = preferences;

	completion->reset();
PRINT_TRACE
	thread->run();
PRINT_TRACE
	this->preferences = 0;
}


BC_Window* Render::new_gui()
{
	this->jobs = 0;
	batch_cancelled = 0;
	in_progress = 0;
	format_error = 0;
	result = 0;
	completion->reset();
	RenderWindow *window = 0;
	
	if(mode == Render::INTERACTIVE)
	{
// Fix the asset for rendering
		if(!asset) asset = new Asset;
		load_defaults(asset);
		check_asset(mwindow->edl, *asset);

// Get format from user
		window = new RenderWindow(mwindow, 
			this, 
			asset,
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
		window->create_objects();
	}
	else
	{
		;
	}

	return window;
}


void Render::handle_close_event(int result)
{
	int format_error = 0;
	const int debug = 0;

	if(!result)
	{
		if(debug) printf("Render::handle_close_event %d\n", __LINE__);
// Check the asset format for errors.
		FormatCheck format_check(asset);
		if(debug) printf("Render::handle_close_event %d\n", __LINE__);
		format_error = format_check.check_format();
		if(debug) printf("Render::handle_close_event %d\n", __LINE__);
	}


	save_defaults(asset);
	mwindow->save_defaults();

	if(!format_error && !result)
	{
		if(debug) printf("Render::handle_close_event %d\n", __LINE__);

		if(!result) start_render();
		if(debug) printf("Render::handle_close_event %d\n", __LINE__);

	}
}



void Render::stop_operation()
{
	if(Thread::running() ||
		in_progress)
	{
printf("Render::stop_operation %d\n", __LINE__);
		batch_cancelled = 1;
// Wait for completion
		completion->lock("Render::stop_operation");
		completion->reset();
	}
}




int Render::check_asset(EDL *edl, Asset &asset)
{
	if(asset.video_data && 
		edl->tracks->playable_video_tracks() &&
		File::supports_video(asset.format))
	{
		asset.video_data = 1;
		asset.layers = 1;
		asset.width = edl->session->output_w;
		asset.height = edl->session->output_h;
	}
	else
	{
		asset.video_data = 0;
		asset.layers = 0;
	}

	if(asset.audio_data && 
		edl->tracks->playable_audio_tracks() &&
		File::supports_audio(asset.format))
	{
		asset.audio_data = 1;
		asset.channels = edl->session->audio_channels;
		if(asset.format == FILE_MOV) asset.byte_order = 0;
	}
	else
	{
		asset.audio_data = 0;
		asset.channels = 0;
	}

	if(!asset.audio_data &&
		!asset.video_data)
	{
		return 1;
	}
	return 0;
}

int Render::fix_strategy(int strategy, int use_renderfarm)
{
	if(use_renderfarm)
	{
		if(strategy == FILE_PER_LABEL)
			strategy = FILE_PER_LABEL_FARM;
		else
		if(strategy == SINGLE_PASS)
			strategy = SINGLE_PASS_FARM;
	}
	else
	{
		if(strategy == FILE_PER_LABEL_FARM)
			strategy = FILE_PER_LABEL;
		else
		if(strategy == SINGLE_PASS_FARM)
			strategy = SINGLE_PASS;
	}
	return strategy;
}

void Render::start_progress()
{
	char filename[BCTEXTLEN];
	char string[BCTEXTLEN];
	FileSystem fs;

	progress_max = Units::to_int64(default_asset->sample_rate * 
			(total_end - total_start)) +
		Units::to_int64(preferences->render_preroll * 
			packages->total_allocated * 
			default_asset->sample_rate);
	progress_timer->update();
	last_eta = 0;
	if(mwindow)
	{
// Generate the progress box
		fs.extract_name(filename, default_asset->path);
		sprintf(string, _("Rendering %s..."), filename);

// Don't bother with the filename since renderfarm defeats the meaning
		progress = mwindow->mainprogress->start_progress(_("Rendering..."), 
			progress_max);
		render_progress = new RenderProgress(mwindow, this);
		render_progress->start();
	}
}

void Render::stop_progress()
{
	if(progress)
	{
		char string[BCTEXTLEN], string2[BCTEXTLEN];
		delete render_progress;
		progress->get_time(string);
		elapsed_time = progress->get_time();
		progress->stop_progress();
		delete progress;

		sprintf(string2, _("Rendering took %s"), string);
		mwindow->gui->lock_window("");
		mwindow->gui->show_message(string2);
		mwindow->gui->stop_hourglass();
		mwindow->gui->unlock_window();
	}
	progress = 0;
}



void Render::start_render()
{
	thread->start();
}


void Render::create_filename(char *path, 
	char *default_path, 
	int current_number,
	int total_digits,
	int number_start)
{
	int i, j, k;
	int len = strlen(default_path);
	char printf_string[BCTEXTLEN];
	int found_number = 0;

	for(i = 0, j = 0; i < number_start; i++, j++)
	{
		printf_string[j] = default_path[i];
	}

// Found the number
	sprintf(&printf_string[j], "%%0%dd", total_digits);
	j = strlen(printf_string);
	i += total_digits;

// Copy remainder of string
	for( ; i < len; i++, j++)
	{
		printf_string[j] = default_path[i];
	}
	printf_string[j] = 0;
// Print the printf argument to the path
	sprintf(path, printf_string, current_number);
}

void Render::get_starting_number(char *path, 
	int &current_number,
	int &number_start, 
	int &total_digits,
	int min_digits)
{
	int i, j;
	int len = strlen(path);
	char number_text[BCTEXTLEN];
	char *ptr = 0;
	char *ptr2 = 0;

	total_digits = 0;
	number_start = 0;

// Search for last /
	ptr2 = strrchr(path, '/');

// Search for first 0 after last /.
	if(ptr2)
		ptr = strchr(ptr2, '0');

	if(ptr && isdigit(*ptr))
	{
		number_start = ptr - path;

// Store the first number
		char *ptr2 = number_text;
		while(isdigit(*ptr))
			*ptr2++ = *ptr++;
		*ptr2++ = 0;
		current_number = atol(number_text);
		total_digits = strlen(number_text);
	}


// No number found or number not long enough
	if(total_digits < min_digits)
	{
		current_number = 1;
		number_start = len;
		total_digits = min_digits;
	}
}







int Render::load_defaults(Asset *asset)
{
	strategy = mwindow->defaults->get("RENDER_STRATEGY", SINGLE_PASS);
	load_mode = mwindow->defaults->get("RENDER_LOADMODE", LOADMODE_NEW_TRACKS);

// some defaults which work
	asset->video_data = 1;
	asset->audio_data = 1;
	asset->format = FILE_MOV;
	strcpy(asset->acodec, QUICKTIME_MP4A);
	strcpy(asset->vcodec, QUICKTIME_H264);

	asset->load_defaults(mwindow->defaults, 
		"RENDER_", 
		1,
		1,
		1,
		1,
		1);


	return 0;
}

int Render::save_defaults(Asset *asset)
{
	mwindow->defaults->update("RENDER_STRATEGY", strategy);
	mwindow->defaults->update("RENDER_LOADMODE", load_mode);




	asset->save_defaults(mwindow->defaults, 
		"RENDER_",
		1,
		1,
		1,
		1,
		1);

	return 0;
}









RenderThread::RenderThread(MWindow *mwindow, Render *render)
 : Thread(0, 0, 0)
{
	this->mwindow = mwindow;
	this->render = render;
}

RenderThread::~RenderThread()
{
}


void RenderThread::render_single(int test_overwrite, 
	Asset *asset,
	EDL *edl,
	int strategy)
{
	char string[BCTEXTLEN];
// Total length in seconds
	double total_length;
	int last_audio_buffer;
	RenderFarmServer *farm_server = 0;
	FileSystem fs;
	int total_digits;      // Total number of digits including padding the user specified.
	int number_start;      // Character in the filename path at which the number begins
	int current_number;    // The number the being injected into the filename.
	int done = 0;
	const int debug = 0;

//printf("RenderThread::render_single %d\n", __LINE__);
	render->in_progress = 1;


	render->default_asset = asset;
	render->progress = 0;
	render->result = 0;

	if(mwindow)
	{
		if(!render->preferences)
			render->preferences = new Preferences;

		render->preferences->copy_from(mwindow->preferences);
	}


// Create rendering command
	TransportCommand *command = new TransportCommand;
	command->command = NORMAL_FWD;
	command->get_edl()->copy_all(edl);
	command->change_type = CHANGE_ALL;
// Get highlighted playback range
	command->set_playback_range();
// Adjust playback range with in/out points
	command->adjust_playback_range();
	render->packages = new PackageDispatcher;


// Configure preview monitor
	VideoOutConfig vconfig;
	PlaybackConfig *playback_config = new PlaybackConfig;

// Create caches
	CICache *audio_cache = new CICache(render->preferences);
	CICache *video_cache = new CICache(render->preferences);

	render->default_asset->frame_rate = command->get_edl()->session->frame_rate;
	render->default_asset->sample_rate = command->get_edl()->session->sample_rate;

// Conform asset to EDL.  Find out if any tracks are playable.
	render->result = render->check_asset(command->get_edl(), 
		*render->default_asset);

	if(!render->result)
	{
// Get total range to render
		render->total_start = command->start_position;
		render->total_end = command->end_position;
		total_length = render->total_end - render->total_start;

// Nothing to render
		if(EQUIV(total_length, 0))
		{
			render->result = 1;
		}
	}







// Generate packages
	if(!render->result)
	{
// Stop background rendering
		if(mwindow) mwindow->stop_brender();

		fs.complete_path(render->default_asset->path);
		strategy = Render::fix_strategy(strategy, render->preferences->use_renderfarm);

		render->result = render->packages->create_packages(mwindow,
			command->get_edl(),
			render->preferences,
			strategy, 
			render->default_asset, 
			render->total_start, 
			render->total_end,
			test_overwrite);
	}










	done = 0;
	render->total_rendered = 0;

	if(!render->result)
	{
// Start dispatching external jobs
		if(mwindow)
		{
			mwindow->gui->lock_window("Render::render 1");
			mwindow->gui->show_message(_("Starting render farm"));
			mwindow->gui->start_hourglass();
			mwindow->gui->unlock_window();
		}
		else
		{
			printf("Render::render: starting render farm\n");
		}

		if(strategy == SINGLE_PASS_FARM || strategy == FILE_PER_LABEL_FARM)
		{
			farm_server = new RenderFarmServer(mwindow,
				render->packages,
				render->preferences, 
				1,
				&render->result,
				&render->total_rendered,
				render->counter_lock,
				render->default_asset,
				command->get_edl(),
				0);
			render->result = farm_server->start_clients();

			if(render->result)
			{
				if(mwindow)
				{
					mwindow->gui->lock_window("Render::render 2");
					mwindow->gui->show_message(_("Failed to start render farm"),
						mwindow->theme->message_error);
					mwindow->gui->stop_hourglass();
					mwindow->gui->unlock_window();
				}
				else
				{
					printf("Render::render: Failed to start render farm\n");
				}
			}
		}
	}




// Perform local rendering


	if(!render->result)
	{
		render->start_progress();
	



		MainPackageRenderer package_renderer(render);
		render->result = package_renderer.initialize(mwindow,
				command->get_edl(),   // Copy of master EDL
				render->preferences, 
				render->default_asset);







		while(!render->result)
		{
// Get unfinished job
			RenderPackage *package;

			if(strategy == SINGLE_PASS_FARM)
			{
				package = render->packages->get_package(
					package_renderer.frames_per_second, 
					-1, 
					1);
			}
			else
			{
				package = render->packages->get_package(0, -1, 1);
			}

// Exit point
			if(!package) 
			{
				done = 1;
				break;
			}



			Timer timer;
			timer.update();

			if(package_renderer.render_package(package))
			{
				render->result = 1;
			}

		} // file_number



printf("Render::render_single: Session finished.\n");





		if(strategy == SINGLE_PASS_FARM || strategy == FILE_PER_LABEL_FARM)
		{
			farm_server->wait_clients();
		}

if(debug) printf("Render::render %d\n", __LINE__);

// Notify of error
		if(render->result && 
			(!render->progress || !render->progress->is_cancelled()) &&
			!render->batch_cancelled)
		{
if(debug) printf("Render::render %d\n", __LINE__);
			if(mwindow)
			{
if(debug) printf("Render::render %d\n", __LINE__);
				ErrorBox error_box(PROGRAM_NAME ": Error",
					mwindow->gui->get_abs_cursor_x(1),
					mwindow->gui->get_abs_cursor_y(1));
				error_box.create_objects(_("Error rendering data."));
				error_box.raise_window();
				error_box.run_window();
if(debug) printf("Render::render %d\n", __LINE__);
			}
			else
			{
				printf("Render::render: Error rendering data\n");
			}
		}
if(debug) printf("Render::render %d\n", __LINE__);

// Delete the progress box
		render->stop_progress();

if(debug) printf("Render::render %d\n", __LINE__);




	}


// Paste all packages into timeline if desired

	if(!render->result && 
		render->load_mode != LOADMODE_NOTHING && 
		mwindow &&
		render->mode != Render::BATCH)
	{
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->gui->lock_window("Render::render 3");
if(debug) printf("Render::render %d\n", __LINE__);

		mwindow->undo->update_undo_before();

if(debug) printf("Render::render %d\n", __LINE__);


		ArrayList<Indexable*> *assets = render->packages->get_asset_list();
if(debug) printf("Render::render %d\n", __LINE__);
		if(render->load_mode == LOADMODE_PASTE)
			mwindow->clear(0, 1);
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->load_assets(assets, 
			-1, 
			render->load_mode,
			0,
			0,
			mwindow->edl->session->labels_follow_edits,
			mwindow->edl->session->plugins_follow_edits,
			mwindow->edl->session->autos_follow_edits);
if(debug) printf("Render::render %d\n", __LINE__);
		for(int i = 0; i < assets->size(); i++)
			assets->get(i)->Garbage::remove_user();
		delete assets;
if(debug) printf("Render::render %d\n", __LINE__);


		mwindow->save_backup();
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->undo->update_undo_after(_("render"), LOAD_ALL);
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->update_plugin_guis();
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->gui->update(1, 
			2,
			1,
			1,
			1,
			1,
			0);
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->sync_parameters(CHANGE_ALL);
if(debug) printf("Render::render %d\n", __LINE__);
		mwindow->gui->unlock_window();
		
		
		mwindow->awindow->gui->lock_window("Render::render");
		mwindow->awindow->gui->update_assets();
		mwindow->awindow->gui->flush();
		mwindow->awindow->gui->unlock_window();
		
if(debug) printf("Render::render %d\n", __LINE__);
	}

if(debug) printf("Render::render %d\n", __LINE__);

// Disable hourglass
	if(mwindow)
	{
		mwindow->gui->lock_window("Render::render 3");
		mwindow->gui->stop_hourglass();
		mwindow->gui->unlock_window();
	}

//printf("Render::render 110\n");
// Need to restart because brender always stops before render.
	if(mwindow)
		mwindow->restart_brender();
	if(farm_server) delete farm_server;
	delete command;
	delete playback_config;
	delete audio_cache;
	delete video_cache;
// Must delete packages after server
	delete render->packages;

	render->packages = 0;
	render->in_progress = 0;
	render->completion->unlock();
if(debug) printf("Render::render %d\n", __LINE__);
}

void RenderThread::run()
{
	if(render->mode == Render::INTERACTIVE)
	{
		render_single(1, render->asset, mwindow->edl, render->strategy);
	}
	else
	if(render->mode == Render::BATCH)
	{
// PRINT_TRACE
// printf("RenderThread::run %d total jobs=%d result=%d\n", 
// __LINE__, 
// render->jobs->total, 
// render->result);
		for(int i = 0; i < render->jobs->total && !render->result; i++)
		{
//PRINT_TRACE
			BatchRenderJob *job = render->jobs->values[i];
//PRINT_TRACE
			if(job->enabled)
			{
				if(mwindow)
				{
					mwindow->batch_render->update_active(i);
				}
				else
				{
					printf("Render::run: %s\n", job->edl_path);
				}

//PRINT_TRACE

				FileXML *file = new FileXML;
				EDL *edl = new EDL;
				edl->create_objects();
				file->read_from_file(job->edl_path);
				edl->load_xml(file, LOAD_ALL);

//PRINT_TRACE
				render_single(0, job->asset, edl, job->strategy);

//PRINT_TRACE
				edl->Garbage::remove_user();
				delete file;
				if(!render->result)
				{
					if(mwindow)
					{
						mwindow->batch_render->update_done(i, 1, render->elapsed_time);
					}
					else
					{
						char string[BCTEXTLEN];
						render->elapsed_time = 
							(double)render->progress_timer->get_scaled_difference(1);
						Units::totext(string,
							render->elapsed_time,
							TIME_HMS2);
						printf("Render::run: done in %s\n", string);
					}
				}
				else
				{
					if(mwindow)
						mwindow->batch_render->update_active(-1);
					else
						printf("Render::run: failed\n");
				}
			}
//PRINT_TRACE
		}

		if(mwindow)
		{
			mwindow->batch_render->update_active(-1);
			mwindow->batch_render->update_done(-1, 0, 0);
		}
	}
}









#define WIDTH DP(410)
#define HEIGHT DP(360)


RenderWindow::RenderWindow(MWindow *mwindow, 
	Render *render, 
	Asset *asset,
	int x, 
	int y)
 : BC_Window(PROGRAM_NAME ": Render", 
 	x - WIDTH / 2,
	y - HEIGHT / 2,
 	WIDTH, 
	HEIGHT,
	(int)BC_INFINITY,
	(int)BC_INFINITY,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->render = render;
	this->asset = asset;
}

RenderWindow::~RenderWindow()
{
SET_TRACE
	lock_window("RenderWindow::~RenderWindow");
SET_TRACE
	delete format_tools;
SET_TRACE
	delete loadmode;
SET_TRACE
	unlock_window();
SET_TRACE
}



void RenderWindow::create_objects()
{
	int margin = mwindow->theme->widget_border;
	int x = DP(10), y = margin;
	lock_window("RenderWindow::create_objects");
	add_subwindow(new BC_Title(x, 
		y, 
		(char*)((render->strategy == FILE_PER_LABEL || 
				render->strategy == FILE_PER_LABEL_FARM) ? 
			_("Select the first file to render to:") : 
			_("Select a file to render to:"))));
	y += DP(25);

	format_tools = new FormatTools(mwindow,
					this, 
					asset);
	format_tools->create_objects(x, 
		y, 
		1, 
		1, 
		1, 
		1, 
		1,
		0,
		0,
		&render->strategy,
		0);

	loadmode = new LoadMode(mwindow, 
		this, 
		x, 
		y, 
		&render->load_mode, 
		1,
		0);
	loadmode->create_objects();

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}
