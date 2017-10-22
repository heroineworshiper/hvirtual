
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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
#include "batchrender.h"
#include "bcsignals.h"
#include "confirmsave.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "filesystem.h"
#include "filexml.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "preferences.h"
#include "render.h"
#include "theme.h"
#include "transportque.h"
#include "vframe.h"





static const char *list_titles[] = 
{
	"Enabled", 
	"Output",
	"EDL",
	"Elapsed"
};

static int *list_widths = 0;

#define HALF_W DP(350)

BatchRenderMenuItem::BatchRenderMenuItem(MWindow *mwindow)
 : BC_MenuItem(_("Batch Render..."), "Shift+B", 'B')
{
	set_shift(1); 
	this->mwindow = mwindow;
}

int BatchRenderMenuItem::handle_event()
{
	mwindow->batch_render->start();
	return 1;
}








BatchRenderJob::BatchRenderJob(Preferences *preferences)
{
	this->preferences = preferences;
	asset = new Asset;
	edl_path[0] = 0;
	strategy = 0;
	enabled = 1;
	elapsed = 0;
}

BatchRenderJob::~BatchRenderJob()
{
	asset->Garbage::remove_user();
}

void BatchRenderJob::copy_from(BatchRenderJob *src)
{
	asset->copy_from(src->asset, 0);
	strcpy(edl_path, src->edl_path);
	strategy = src->strategy;
	enabled = src->enabled;
	elapsed = 0;
}

void BatchRenderJob::load(FileXML *file)
{
	int result = 0;

	edl_path[0] = 0;
	file->tag.get_property("EDL_PATH", edl_path);
	strategy = file->tag.get_property("STRATEGY", strategy);
	enabled = file->tag.get_property("ENABLED", enabled);
	elapsed = file->tag.get_property("ELAPSED", elapsed);
	fix_strategy();

	result = file->read_tag();
	if(!result)
	{
		if(file->tag.title_is("ASSET"))
		{
			file->tag.get_property("SRC", asset->path);
			asset->read(file, 0);
// The compression parameters are stored in the defaults to reduce
// coding maintenance.  The defaults must now be stuffed into the XML for
// unique storage.
			BC_Hash defaults;
			defaults.load_string(file->read_text());
			asset->load_defaults(&defaults,
				"",
				0,
				1,
				0,
				0,
				0);
		}
	}
}

void BatchRenderJob::save(FileXML *file)
{
	file->tag.set_property("EDL_PATH", edl_path);
	file->tag.set_property("STRATEGY", strategy);
	file->tag.set_property("ENABLED", enabled);
	file->tag.set_property("ELAPSED", elapsed);
	file->append_tag();
	file->append_newline();
	asset->write(file,
		0,
		"");

// The compression parameters are stored in the defaults to reduce
// coding maintenance.  The defaults must now be stuffed into the XML for
// unique storage.
	BC_Hash defaults;
	asset->save_defaults(&defaults, 
		"",
		0,
		1,
		0,
		0,
		0);
	char *string;
	defaults.save_string(string);
	file->append_text(string);
	delete [] string;
	file->tag.set_title("/JOB");
	file->append_tag();
	file->append_newline();
}

void BatchRenderJob::fix_strategy()
{
	strategy = Render::fix_strategy(strategy, preferences->use_renderfarm);
}










BatchRenderThread::BatchRenderThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	current_job = 0;
	rendering_job = -1;
	is_rendering = 0;
	default_job = 0;
	file_entries = 0;
}

BatchRenderThread::BatchRenderThread()
 : BC_DialogThread()
{
	mwindow = 0;
	current_job = 0;
	rendering_job = -1;
	is_rendering = 0;
	default_job = 0;
	file_entries = 0;
}

void BatchRenderThread::handle_close_event(int result)
{
// Save settings
	char path[BCTEXTLEN];
	path[0] = 0;
	save_jobs(path);
	save_defaults(mwindow->defaults);
	delete default_job;
	default_job = 0;
	jobs.remove_all_objects();
	if(file_entries)
	{
		file_entries->remove_all_objects();
		delete file_entries;
		file_entries = 0;
	}
}

BC_Window* BatchRenderThread::new_gui()
{
	current_start = 0.0;
	current_end = 0.0;
	default_job = new BatchRenderJob(mwindow->preferences);


	if(!list_widths)
	{
		list_widths = new int[4];
		list_widths[0] = 50;
		list_widths[1] = 100;
		list_widths[2] = 200;
		list_widths[3] = 100;
	}
	
	
	if(!file_entries)
	{
		file_entries = new ArrayList<BC_ListBoxItem*>;
		FileSystem fs;
		char string[BCTEXTLEN];
	// Load current directory
		fs.update(getcwd(string, BCTEXTLEN));
		for(int i = 0; i < fs.total_files(); i++)
		{
			file_entries->append(
				new BC_ListBoxItem(
					fs.get_entry(i)->get_name()));
		}
	}

	char path[BCTEXTLEN];
	path[0] = 0;
	load_jobs(path, mwindow->preferences);
	load_defaults(mwindow->defaults);
	this->gui = new BatchRenderGUI(mwindow, 
		this,
		mwindow->session->batchrender_x,
		mwindow->session->batchrender_y,
		mwindow->session->batchrender_w,
		mwindow->session->batchrender_h);
	this->gui->create_objects();
	return this->gui;
}


void BatchRenderThread::load_jobs(char *path, Preferences *preferences)
{
	FileXML file;
	int result = 0;

	jobs.remove_all_objects();
	if(path[0])
		file.read_from_file(path);
	else
		file.read_from_file(create_path(path));

	while(!result)
	{
		if(!(result = file.read_tag()))
		{
			if(file.tag.title_is("JOB"))
			{
				BatchRenderJob *job;
				jobs.append(job = new BatchRenderJob(preferences));
				job->load(&file);
			}
		}
	}
}

void BatchRenderThread::save_jobs(char *path)
{
	FileXML file;

	for(int i = 0; i < jobs.total; i++)
	{
		file.tag.set_title("JOB");
		jobs.values[i]->save(&file);
	}

	if(path[0])
		file.write_to_file(path);
	else
		file.write_to_file(create_path(path));
}

void BatchRenderThread::load_defaults(BC_Hash *defaults)
{
	if(default_job)
	{
		default_job->asset->load_defaults(defaults,
			"BATCHRENDER_",
			1,
			1,
			1,
			1,
			1);
		default_job->fix_strategy();
	}

	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		char string[BCTEXTLEN];
		sprintf(string, "BATCHRENDER_COLUMN%d", i);
		column_width[i] = defaults->get(string, list_widths[i]);
	}
}

void BatchRenderThread::save_defaults(BC_Hash *defaults)
{
	if(default_job)
	{
		default_job->asset->save_defaults(defaults,
			"BATCHRENDER_",
			1,
			1,
			1,
			1,
			1);
		defaults->update("BATCHRENDER_STRATEGY", default_job->strategy);
	}
	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		char string[BCTEXTLEN];
		sprintf(string, "BATCHRENDER_COLUMN%d", i);
		defaults->update(string, column_width[i]);
	}
//	defaults->update("BATCHRENDER_JOB", current_job);
	if(mwindow)
		mwindow->save_defaults();
	else
		defaults->save();
}

char* BatchRenderThread::create_path(char *string)
{
	FileSystem fs;
	sprintf(string, "%s", BCASTDIR);
	fs.complete_path(string);
	strcat(string, BATCH_PATH);
	return string;
}

void BatchRenderThread::new_job()
{
	BatchRenderJob *result = new BatchRenderJob(mwindow->preferences);
	result->copy_from(get_current_job());
	jobs.append(result);
	current_job = jobs.total - 1;
	gui->create_list(1);
	gui->change_job();
}

void BatchRenderThread::delete_job()
{
	if(current_job < jobs.total && current_job >= 0)
	{
		jobs.remove_object_number(current_job);
		if(current_job > 0) current_job--;
		gui->create_list(1);
		gui->change_job();
	}
}

void BatchRenderThread::use_current_edl()
{
// printf("BatchRenderThread::use_current_edl %d %p %s\n", 
// __LINE__, 
// mwindow->edl->path, 
// mwindow->edl->path);

	strcpy(get_current_edl(), mwindow->edl->path);
	gui->create_list(1);
	gui->edl_path_text->update(get_current_edl());
}

BatchRenderJob* BatchRenderThread::get_current_job()
{
	BatchRenderJob *result;
	if(current_job >= jobs.total || current_job < 0)
	{
		result = default_job;
	}
	else
	{
		result = jobs.values[current_job];
	}
	return result;
}


Asset* BatchRenderThread::get_current_asset()
{
	return get_current_job()->asset;
}

char* BatchRenderThread::get_current_edl()
{
	return get_current_job()->edl_path;
}


// Test EDL files for existence
int BatchRenderThread::test_edl_files()
{
	for(int i = 0; i < jobs.total; i++)
	{
		if(jobs.values[i]->enabled)
		{
			FILE *fd = fopen(jobs.values[i]->edl_path, "r");
			if(!fd)
			{
				char string[BCTEXTLEN];
				sprintf(string, _("EDL %s not found.\n"), jobs.values[i]->edl_path);
				if(mwindow)
				{
					ErrorBox error_box(PROGRAM_NAME ": Error",
						mwindow->gui->get_abs_cursor_x(1),
						mwindow->gui->get_abs_cursor_y(1));
					error_box.create_objects(string);
					error_box.run_window();
					gui->new_batch->enable();
					gui->delete_batch->enable();
				}
				else
				{
					fprintf(stderr, 
						"%s",
						string);
				}

				is_rendering = 0;
				return 1;
			}
			else
			{
				fclose(fd);
			}
		}
	}
	return 0;
}

void BatchRenderThread::calculate_dest_paths(ArrayList<char*> *paths,
	Preferences *preferences)
{
	for(int i = 0; i < jobs.total; i++)
	{
		BatchRenderJob *job = jobs.values[i];
		if(job->enabled)
		{
			PackageDispatcher *packages = new PackageDispatcher;

// Load EDL
			TransportCommand *command = new TransportCommand;
			FileXML *file = new FileXML;
			file->read_from_file(job->edl_path);

// Use command to calculate range.
			command->command = NORMAL_FWD;
			command->get_edl()->load_xml(file, 
				LOAD_ALL);
			command->change_type = CHANGE_ALL;
			command->set_playback_range();
			command->adjust_playback_range();

// Create test packages
			packages->create_packages(mwindow,
				command->get_edl(),
				preferences,
				job->strategy, 
				job->asset, 
				command->start_position, 
				command->end_position,
				0);

// Append output paths allocated to total
			for(int j = 0; j < packages->get_total_packages(); j++)
			{
				RenderPackage *package = packages->get_package(j);
				paths->append(strdup(package->path));
			}

// Delete package harness
			delete packages;
			delete command;
			delete file;
		}
	}
}


void BatchRenderThread::start_rendering(char *config_path,
	char *batch_path)
{
	BC_Hash *boot_defaults;
	Preferences *preferences;
	Render *render;
	BC_Signals *signals = new BC_Signals;

//PRINT_TRACE
// Initialize stuff which MWindow does.
	signals->initialize();
	MWindow::init_defaults(boot_defaults, config_path);
	load_defaults(boot_defaults);
	preferences = new Preferences;
	preferences->load_defaults(boot_defaults);
	MWindow::init_plugins(preferences, 0);
	BC_WindowBase::get_resources()->vframe_shm = 1;
	MWindow::init_fileserver(preferences);


//PRINT_TRACE
	load_jobs(batch_path, preferences);
	save_jobs(batch_path);
	save_defaults(boot_defaults);

//PRINT_TRACE
// Test EDL files for existence
	if(test_edl_files()) return;

//PRINT_TRACE

// Predict all destination paths
	ArrayList<char*> paths;
	calculate_dest_paths(&paths,
		preferences);

//PRINT_TRACE
	int result = ConfirmSave::test_files(0, &paths);
// Abort on any existing file because it's so hard to set this up.
	if(result) return;

//PRINT_TRACE
	render = new Render(0);
//PRINT_TRACE
	render->start_batches(&jobs, 
		boot_defaults,
		preferences);
//PRINT_TRACE
}

void BatchRenderThread::start_rendering()
{
	if(is_rendering) return;

	is_rendering = 1;
	char path[BCTEXTLEN];
	path[0] = 0;
	save_jobs(path);
	save_defaults(mwindow->defaults);
	gui->new_batch->disable();
	gui->delete_batch->disable();

// Test EDL files for existence
	if(test_edl_files()) return;

// Predict all destination paths
	ArrayList<char*> paths;
	calculate_dest_paths(&paths,
		mwindow->preferences);

// Test destination files for overwrite
	int result = ConfirmSave::test_files(mwindow, &paths);
	paths.remove_all_objects();

// User cancelled
	if(result)
	{
		is_rendering = 0;
		gui->new_batch->enable();
		gui->delete_batch->enable();
		return;
	}

	mwindow->render->start_batches(&jobs);
}

void BatchRenderThread::stop_rendering()
{
	if(!is_rendering) return;
	mwindow->render->stop_operation();
	is_rendering = 0;
}

void BatchRenderThread::update_active(int number)
{
	gui->lock_window("BatchRenderThread::update_active");
	if(number >= 0)
	{
		current_job = number;
		rendering_job = number;
	}
	else
	{
		rendering_job = -1;
		is_rendering = 0;
	}
	gui->create_list(1);
	gui->unlock_window();
}

void BatchRenderThread::update_done(int number, 
	int create_list, 
	double elapsed_time)
{
	gui->lock_window("BatchRenderThread::update_done");
	if(number < 0)
	{
		gui->new_batch->enable();
		gui->delete_batch->enable();
	}
	else
	{
		jobs.values[number]->enabled = 0;
		jobs.values[number]->elapsed = elapsed_time;
		if(create_list) gui->create_list(1);
	}
	gui->unlock_window();
}

void BatchRenderThread::move_batch(int src, int dst)
{
	BatchRenderJob *src_job = jobs.values[src];
	if(dst < 0) dst = jobs.total - 1;

	if(dst != src)
	{
		for(int i = src; i < jobs.total - 1; i++)
			jobs.values[i] = jobs.values[i + 1];
//		if(dst > src) dst--;
		for(int i = jobs.total - 1; i > dst; i--)
			jobs.values[i] = jobs.values[i - 1];
		jobs.values[dst] = src_job;
		gui->create_list(1);
	}
}









BatchRenderGUI::BatchRenderGUI(MWindow *mwindow, 
	BatchRenderThread *thread,
	int x,
	int y,
	int w,
	int h)
 : BC_Window(PROGRAM_NAME ": Batch Render", 
	x,
	y,
	w, 
	h, 
	DP(50), 
	DP(50), 
	1,
	0, 
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

BatchRenderGUI::~BatchRenderGUI()
{
	lock_window("BatchRenderGUI::~BatchRenderGUI");
	delete format_tools;
	unlock_window();
}


void BatchRenderGUI::create_objects()
{
	int margin = mwindow->theme->widget_border;

	lock_window("BatchRenderGUI::create_objects");
	mwindow->theme->get_batchrender_sizes(this, get_w(), get_h());
	create_list(0);

	int x = mwindow->theme->batchrender_x1;
	int y = margin;
	int x1 = mwindow->theme->batchrender_x1;
	int x2 = mwindow->theme->batchrender_x2;
	int x3 = mwindow->theme->batchrender_x3;
	int y1 = y;
	int y2;

// output file
	add_subwindow(output_path_title = new BC_Title(x1, y, _("Output path:")));
	y += DP(20);
	format_tools = new BatchFormat(mwindow,
					this, 
					thread->get_current_asset());
	format_tools->set_w(HALF_W - margin);
	format_tools->create_objects(x, 
						y, 
						1, 
						1, 
						1, 
						1, 
						0, 
						1, 
						0, 
						0, 
						&thread->get_current_job()->strategy, 
						0);

	x2 = x;
	y2 = y + DP(10);
	x += format_tools->get_w();
	y = y1;
	x1 = HALF_W;
	x3 = x + DP(80);

// input EDL
	x = HALF_W;
	add_subwindow(edl_path_title = new BC_Title(x, y, _("EDL Path:")));
	y += DP(20);
	add_subwindow(edl_path_text = new BatchRenderEDLPath(
		thread, 
		x, 
		y, 
		get_w() - x - DP(30), 
		thread->get_current_edl()));

	x += edl_path_text->get_w();
	add_subwindow(edl_path_browse = new BrowseButton(
		mwindow->theme,
		this,
		edl_path_text, 
		x, 
		y, 
		thread->get_current_edl(),
		_("Input EDL"),
		_("Select an EDL to load:"),
		0));

	x = x1;

	y += DP(30);
	add_subwindow(new_batch = new BatchRenderNew(thread, 
		x, 
		y));
	x += new_batch->get_w() + DP(10);

	add_subwindow(delete_batch = new BatchRenderDelete(thread, 
		x, 
		y));
	x = new_batch->get_x();
	y += new_batch->get_h() + mwindow->theme->widget_border;
	add_subwindow(use_current_edl = new BatchRenderCurrentEDL(thread,
		x,
		y));

	x = x2;
	y = y2;
	add_subwindow(list_title = new BC_Title(x, y, _("Batches to render:")));
	y += DP(20);
	add_subwindow(batch_list = new BatchRenderList(thread, 
		x, 
		y,
		get_w() - x - DP(10),
		get_h() - y - BC_GenericButton::calculate_h() - DP(15)));

	y += batch_list->get_h() + DP(10);
	add_subwindow(start_button = new BatchRenderStart(thread, 
	    x, 
	    y));
	x = get_w() / 2 -
		BC_GenericButton::calculate_w(this, _("Stop")) / 2;
	add_subwindow(stop_button = new BatchRenderStop(thread, 
		x, 
		y));
	x = get_w() - 
		BC_GenericButton::calculate_w(this, _("Close")) - 
		DP(10);
	add_subwindow(cancel_button = new BatchRenderCancel(thread, 
		x, 
		y));

	show_window(1);
	unlock_window();
}

int BatchRenderGUI::resize_event(int w, int h)
{
	mwindow->session->batchrender_w = w;
	mwindow->session->batchrender_h = h;
	mwindow->theme->get_batchrender_sizes(this, w, h);
	int margin = mwindow->theme->widget_border;

	int x = mwindow->theme->batchrender_x1;
	int y = margin;
	int x1 = mwindow->theme->batchrender_x1;
	int x2 = mwindow->theme->batchrender_x2;
	int x3 = mwindow->theme->batchrender_x3;
	int y1 = y;
	int y2;

	output_path_title->reposition_window(x1, y);
	y += DP(20);
	format_tools->reposition_window(x, y);
	x2 = x;
	y2 = y + DP(10);
	y = y1;
	x += format_tools->get_w();
	x1 = HALF_W;
	x3 = x + DP(80);

	x = HALF_W;
	edl_path_title->reposition_window(x, y);
	y += DP(20);
	edl_path_text->reposition_window(x, y, w - x - DP(30));
	x += edl_path_text->get_w();
	edl_path_browse->reposition_window(x, y);

 	x = x1;
// 	y += 30;
// 	status_title->reposition_window(x, y);
// 	x = x3;
// 	status_text->reposition_window(x, y);
// 	x = x1;
// 	y += 30;
// 	progress_bar->reposition_window(x, y, w - x - 10);

	y += DP(30);
	new_batch->reposition_window(x, y);
	x += new_batch->get_w() + DP(10);
	delete_batch->reposition_window(x, y);
	x = new_batch->get_x();
	y += new_batch->get_h() + mwindow->theme->widget_border;
	use_current_edl->reposition_window(x, y);

	x = x2;
	y = y2;
	int y_margin = get_h() - batch_list->get_h();
	list_title->reposition_window(x, y);
	y += DP(20);
	batch_list->reposition_window(x, y, w - x - 10, h - y_margin);

	y += batch_list->get_h() + DP(10);
	start_button->reposition_window(x, y);
	x = w / 2 - 
		stop_button->get_w() / 2;
	stop_button->reposition_window(x, y);
	x = w -
		cancel_button->get_w() - 
		DP(10);
	cancel_button->reposition_window(x, y);
	return 1;
}

int BatchRenderGUI::translation_event()
{
	mwindow->session->batchrender_x = get_x();
	mwindow->session->batchrender_y = get_y();
	return 1;
}

int BatchRenderGUI::close_event()
{
// Stop batch rendering
	unlock_window();
	thread->stop_rendering();
	lock_window("BatchRenderGUI::close_event");
	set_done(1);
	return 1;
}

void BatchRenderGUI::create_list(int update_widget)
{
	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		list_columns[i].remove_all_objects();
	}

	for(int i = 0; i < thread->jobs.total; i++)
	{
		BatchRenderJob *job = thread->jobs.values[i];
		char string[BCTEXTLEN];
		BC_ListBoxItem *enabled = new BC_ListBoxItem(job->enabled ? 
			(char*)"X" : 
			(char*)" ");
		BC_ListBoxItem *item1 = new BC_ListBoxItem(job->asset->path);
		BC_ListBoxItem *item2 = new BC_ListBoxItem(job->edl_path);
		BC_ListBoxItem *item3;
		if(job->elapsed)
			item3 = new BC_ListBoxItem(
				Units::totext(string,
					job->elapsed,
					TIME_HMS2));
		else
			item3 = new BC_ListBoxItem(_("Unknown"));
		list_columns[0].append(enabled);
		list_columns[1].append(item1);
		list_columns[2].append(item2);
		list_columns[3].append(item3);
		if(i == thread->current_job)
		{
			enabled->set_selected(1);
			item1->set_selected(1);
			item2->set_selected(1);
			item3->set_selected(1);
		}
		if(i == thread->rendering_job)
		{
			enabled->set_color(RED);
			item1->set_color(RED);
			item2->set_color(RED);
			item3->set_color(RED);
		}
	}

	if(update_widget)
	{
		batch_list->update(list_columns,
						list_titles,
						thread->column_width,
						BATCHRENDER_COLUMNS,
						batch_list->get_xposition(),
						batch_list->get_yposition(), 
						batch_list->get_highlighted_item(),  // Flat index of item cursor is over
						1,     // set all autoplace flags to 1
						1);
	}
}

void BatchRenderGUI::change_job()
{
	BatchRenderJob *job = thread->get_current_job();
	format_tools->update(job->asset, &job->strategy);
	edl_path_text->update(job->edl_path);
}








BatchFormat::BatchFormat(MWindow *mwindow,
			BatchRenderGUI *gui,
			Asset *asset)
 : FormatTools(mwindow, gui, asset)
{
	this->gui = gui;
	this->mwindow = mwindow;
}

BatchFormat::~BatchFormat()
{
}


int BatchFormat::handle_event()
{
	gui->create_list(1);
	return 1;
}











BatchRenderEDLPath::BatchRenderEDLPath(BatchRenderThread *thread, 
	int x, 
	int y, 
	int w, 
	char *text)
 : BC_TextBox(x, 
		y, 
		w, 
		1,
		text)
{
	this->thread = thread;
}


int BatchRenderEDLPath::handle_event()
{
// Suggestions
	calculate_suggestions(thread->file_entries);

	strcpy(thread->get_current_edl(), get_text());
	thread->gui->create_list(1);
	return 1;
}






BatchRenderNew::BatchRenderNew(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("New"))
{
	this->thread = thread;
}

int BatchRenderNew::handle_event()
{
	thread->new_job();
	return 1;
}

BatchRenderDelete::BatchRenderDelete(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->thread = thread;
}

int BatchRenderDelete::handle_event()
{
	thread->delete_job();
	return 1;
}






BatchRenderCurrentEDL::BatchRenderCurrentEDL(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Use Current EDL"))
{
	this->thread = thread;
}

int BatchRenderCurrentEDL::handle_event()
{
	thread->use_current_edl();
	return 1;
}



BatchRenderList::BatchRenderList(BatchRenderThread *thread, 
	int x, 
	int y,
	int w,
	int h)
 : BC_ListBox(x, 
 	y, 
	w, 
	h, 
	LISTBOX_TEXT,
	thread->gui->list_columns,
	list_titles,
	thread->column_width,
	BATCHRENDER_COLUMNS,
	0,
	0,
	LISTBOX_SINGLE,
	ICON_LEFT,
	1)
{
	this->thread = thread;
	dragging_item = 0;
	set_process_drag(0);
}

int BatchRenderList::handle_event()
{
	return 1;
}

int BatchRenderList::selection_changed()
{
	thread->current_job = get_selection_number(0, 0);
	thread->gui->change_job();
	if(get_cursor_x() < thread->column_width[0])
	{
		BatchRenderJob *job = thread->get_current_job();
		job->enabled = !job->enabled;
		thread->gui->create_list(1);
	}
	return 1;
}

int BatchRenderList::column_resize_event()
{
	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		thread->column_width[i] = get_column_width(i);
	}
	return 1;
}

int BatchRenderList::drag_start_event()
{
	if(BC_ListBox::drag_start_event())
	{
		dragging_item = 1;
		return 1;
	}

	return 0;
}

int BatchRenderList::drag_motion_event()
{
	if(BC_ListBox::drag_motion_event())
	{
		return 1;
	}
	return 0;
}

int BatchRenderList::drag_stop_event()
{
	if(dragging_item)
	{
		int src = get_selection_number(0, 0);
		int dst = get_highlighted_item();
		if(src != dst)
		{
			thread->move_batch(src, dst);
		}
		BC_ListBox::drag_stop_event();
	}
}













BatchRenderStart::BatchRenderStart(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, 
 	y, 
	_("Start"))
{
	this->thread = thread;
}

int BatchRenderStart::handle_event()
{
	thread->start_rendering();
	return 1;
}

BatchRenderStop::BatchRenderStop(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, 
 	y, 
	_("Stop"))
{
	this->thread = thread;
}

int BatchRenderStop::handle_event()
{
	unlock_window();
	thread->stop_rendering();
	lock_window("BatchRenderStop::handle_event");
	return 1;
}


BatchRenderCancel::BatchRenderCancel(BatchRenderThread *thread, 
	int x, 
	int y)
 : BC_GenericButton(x, 
 	y, 
	_("Close"))
{
	this->thread = thread;
}

int BatchRenderCancel::handle_event()
{
	unlock_window();
	thread->stop_rendering();
	lock_window("BatchRenderCancel::handle_event");
	thread->gui->set_done(1);
	return 1;
}

int BatchRenderCancel::keypress_event()
{
	if(get_keypress() == ESC) 
	{
		unlock_window();
		thread->stop_rendering();
		lock_window("BatchRenderCancel::keypress_event");
		thread->gui->set_done(1);
		return 1;
	}
	return 0;
}



