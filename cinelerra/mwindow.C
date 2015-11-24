/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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
#include "assets.h"
#include "audioalsa.h"
#include "awindowgui.h"
#include "awindow.h"
#include "batchrender.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "brender.h"
#include "cache.h"
#include "channel.h"
#include "channeldb.h"
#include "clip.h"
#include "colormodels.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "bchash.h"
#include "devicedvbinput.inc"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "fileformat.h"
#include "file.h"
#include "fileserver.h"
#include "filesystem.h"
#include "filexml.h"
#include "framecache.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "keyframegui.h"
#include "indexfile.h"
#include "language.h"
#include "levelwindowgui.h"
#include "levelwindow.h"
#include "loadfile.inc"
#include "localsession.h"
#include "maincursor.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainprogress.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "mutex.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "nestededls.h"
#include "new.h"
#include "patchbay.h"
#include "playback3d.h"
#include "playbackengine.h"
#include "plugin.h"
#include "pluginserver.h"
#include "pluginset.h"
#include "preferences.h"
#include "record.h"
#include "recordlabel.h"
#include "removethread.h"
#include "render.h"
#include "samplescroll.h"
#include "sighandler.h"
#include "splashgui.h"
#include "statusbar.h"
#include "theme.h"
#include "threadloader.h"
#include "timebar.h"
#include "tipwindow.h"
#include "trackcanvas.h"
#include "track.h"
#include "tracking.h"
#include "trackscroll.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
#include "vframe.h"
#include "videodevice.inc"
#include "videowindow.h"
#include "vplayback.h"
#include "vwindowgui.h"
#include "vwindow.h"
#include "wavecache.h"
#include "zoombar.h"

#include <string.h>



extern "C"
{




// Hack for libdv to remove glib dependancy

// void
// g_log (const char    *log_domain,
//        int  log_level,
//        const char    *format,
//        ...)
// {
// }
// 
// void
// g_logv (const char    *log_domain,
//        int  log_level,
//        const char    *format,
//        ...)
// {
// }
// 


// Hack for XFree86 4.1.0

int atexit(void (*function)(void))
{
	return 0;
}



}



ArrayList<PluginServer*>* MWindow::plugindb = 0;
FileServer* MWindow::file_server = 0;


MWindow::MWindow()
 : Thread(1, 0, 0)
{
	plugin_gui_lock = new Mutex("MWindow::plugin_gui_lock");
	dead_plugin_lock = new Mutex("MWindow::dead_plugin_lock");
	brender_lock = new Mutex("MWindow::brender_lock");
	keyframe_gui_lock = new Mutex("MWindow::keyframe_gui_lock");
	brender = 0;
	session = 0;
	channeldb_buz = new ChannelDB;
	channeldb_v4l2jpeg = new ChannelDB;
	dvb_input = 0;
	dvb_input_lock = new Mutex("MWindow::dvb_input_lock");
	splash_window = 0;
}


// Need to delete brender temporary here.
MWindow::~MWindow()
{
//printf("MWindow::~MWindow %d\n", __LINE__);

	brender_lock->lock("MWindow::quit");
	if(brender) delete brender;
	brender = 0;
	brender_lock->unlock();

	delete brender_lock;

	delete mainindexes;

// Save defaults for open plugins
	plugin_gui_lock->lock("MWindow::~MWindow");
	for(int i = 0; i < plugin_guis->size(); i++)
	{
		plugin_guis->get(i)->hide_gui();
	}
	plugin_gui_lock->unlock();

	clean_indexes();

	save_defaults();
// Give up and go to a movie
	exit(0);

// 	delete mainprogress;
// 	delete audio_cache;             // delete the cache after the assets
// 	delete video_cache;             // delete the cache after the assets
// 	delete frame_cache;
// 	if(gui) delete gui;
// 	delete undo;
// 	delete preferences;
// 	delete defaults;
// 	delete render;
// 	delete awindow;
// 	delete vwindow;
// 	delete cwindow;
// 	delete lwindow;
// 	plugin_guis->remove_all_objects();
// 	delete plugin_guis;
// 	delete plugin_gui_lock;
}


void MWindow::quit(int unlock)
{
	if(unlock) gui->unlock_window();




	brender_lock->lock("MWindow::quit");
	if(brender) delete brender;
	brender = 0;
	brender_lock->unlock();

	interrupt_indexes();
	clean_indexes();
	save_defaults();
// This is the last thread to exit
	playback_3d->quit();
	if(unlock) gui->lock_window("MWindow::quit");
}

void MWindow::init_error()
{
	new MainError(this);
}

void MWindow::create_defaults_path(char *string, const char *config_file)
{
// set the .bcast path
	FileSystem fs;

	sprintf(string, "%s", BCASTDIR);
	fs.complete_path(string);
	if(!fs.is_dir(string)) 
	{
		fs.create_dir(string); 
	}

// load the defaults
	strcat(string, config_file);
}

void MWindow::init_defaults(BC_Hash* &defaults, char *config_path)
{
	char path[BCTEXTLEN];
// Use user supplied path
	if(config_path[0])
	{
		strcpy(path, config_path);
	}
	else
	{
		create_defaults_path(path, CONFIG_FILE);
	}

	defaults = new BC_Hash(path);
	defaults->load();
}



// init plugins with splash screen
// void MWindow::init_plugin_path(Preferences *preferences, 
// 	FileSystem *fs,
// 	SplashGUI *splash_window,
// 	int *counter)
// {
// 	const int debug = 0;
// 	int result = 0;
// 	PluginServer *newplugin;
// 	Timer total_time;
// 
// 	if(debug) PRINT_TRACE
// 
// 	if(!result)
// 	{
// 		for(int i = 0; i < fs->dir_list.total; i++)
// 		{
// 			char path[BCTEXTLEN];
// 			strcpy(path, fs->dir_list.values[i]->path);
// 
// // File is a directory
// 			if(fs->is_dir(path))
// 			{
// 				continue;
// 			}
// 			else
// 			{
// // Try to query the plugin
// 				Timer timer;
// 				fs->complete_path(path);
// //				if(debug) printf("MWindow::init_plugin_path %d %s\n", __LINE__, path);
// 				PluginServer *new_plugin = new PluginServer(path);
// 				int result = new_plugin->open_plugin(1, preferences, 0, 0, -1);
// 
// 				if(!result)
// 				{
// 					plugindb->append(new_plugin);
// 					new_plugin->close_plugin();
// //					if(splash_window)
// //						splash_window->operation->update(_(new_plugin->title));
// 				}
// 				else
// 				if(result == PLUGINSERVER_IS_LAD)
// 				{
// 					delete new_plugin;
// // Open LAD subplugins
// 					int id = 0;
// 					do
// 					{
// 						new_plugin = new PluginServer(path);
// 						result = new_plugin->open_plugin(1,
// 							preferences,
// 							0,
// 							0,
// 							id);
// 						id++;
// 						if(!result)
// 						{
// 							plugindb->append(new_plugin);
// 							new_plugin->close_plugin();
// //							if(splash_window)
// //								splash_window->operation->update(_(new_plugin->title));
// //							else
// //							{
// //							}
// 						}
// 					}while(!result);
// 				}
// 				else
// 				{
// // Plugin failed to open
// 					delete new_plugin;
// 				}
// 				
// //				if(debug) printf("MWindow::init_plugin_path %d %d\n", __LINE__, (int)timer.get_difference());
// 			}
// 
// 			if(splash_window) splash_window->progress->update((*counter)++);
// 		}
// 	}
// 	
// 	if(debug) printf("MWindow::init_plugin_path %d total_time=%d\n", __LINE__, (int)total_time.get_difference());
// 
// }


// init plugins with index
void MWindow::init_plugin_path(Preferences *preferences, 
	char *path,
	int is_lad)
{
	const int debug = 0;
	int result = 0;
	PluginServer *newplugin;
	Timer total_time;
	FileSystem fs;
	char index_path[BCTEXTLEN];
	FILE *index_fd = 0;
	
	sprintf(index_path, "%s/%s", path, PLUGIN_FILE);
//printf("MWindow::init_plugin_path %d %s plugindb=%p\n", __LINE__, index_path, plugindb);


// Try loading index
	index_fd = fopen(index_path, "r");
	int index_version = -1;
	if(index_fd)
	{
// Get version
		int temp = fscanf(index_fd, "%d", &index_version);
	}

	if(index_fd && index_version == PLUGIN_FILE_VERSION)
	{
		while(!feof(index_fd))
		{
			char index_line[BCTEXTLEN];
			char *result2 = fgets(index_line, BCTEXTLEN, index_fd);

			if(result2)
			{
// Create plugin server from index entry
				PluginServer *new_plugin = new PluginServer(path);
				result = new_plugin->read_table(index_line);
//printf("%p new_plugin=%p %s", result2, new_plugin, index_line);
				if(!result)
				{
					plugindb->append(new_plugin);
				}
				else
				{
					delete new_plugin;
				}
			}
		}
	}
	else
	{
		index_fd = fopen(index_path, "w");

// Version of the index file
		fprintf(index_fd, "%d\n", PLUGIN_FILE_VERSION);


// Get directories
		if(is_lad)
			fs.set_filter("*.so");
		else
			fs.set_filter("[*.plugin][*.so]");

		result = fs.update(path);

		if(debug) PRINT_TRACE

		if(!result)
		{
			for(int i = 0; i < fs.dir_list.total; i++)
			{
				char path[BCTEXTLEN];
				strcpy(path, fs.dir_list.values[i]->path);

// File is a directory
				if(fs.is_dir(path))
				{
					continue;
				}
				else
				{
// Try to query the plugin
					Timer timer;
					fs.complete_path(path);
					PluginServer *new_plugin = new PluginServer(path);
					result = new_plugin->open_plugin(1, preferences, 0, 0, -1);

					if(!result)
					{
						new_plugin->write_table(index_fd);
						plugindb->append(new_plugin);
						new_plugin->close_plugin();
					}
					else
					if(result == PLUGINSERVER_IS_LAD)
					{
						delete new_plugin;
// Open LAD subplugins
						int id = 0;
						do
						{
							new_plugin = new PluginServer(path);
							result = new_plugin->open_plugin(1,
								preferences,
								0,
								0,
								id);
							id++;
							if(!result)
							{
								new_plugin->write_table(index_fd);
								plugindb->append(new_plugin);
								new_plugin->close_plugin();
							}
						}while(!result);
					}
					else
					{
// Plugin failed to open
						delete new_plugin;
					}
				}
			}
		}
	}
	
	fclose(index_fd);
	if(debug) printf("MWindow::init_plugin_path %d total_time=%d\n", __LINE__, (int)total_time.get_difference());

}

void MWindow::init_plugins(Preferences *preferences, 
	SplashGUI *splash_window)
{
	const int debug = 0;
	if(!plugindb) plugindb = new ArrayList<PluginServer*>;


	init_plugin_path(preferences, preferences->plugin_dir, 0);

// Parse LAD environment variable
	char *env = getenv("LADSPA_PATH");
	if(env)
	{
		char string[BCTEXTLEN];
		char *ptr1 = env;
		while(ptr1)
		{
			char *ptr = strchr(ptr1, ':');
			char *end;
			if(ptr)
			{
				end = ptr;
			}
			else
			{
				end = env + strlen(env);
			}

			if(end > ptr1)
			{
				int len = end - ptr1;
				memcpy(string, ptr1, len);
				string[len] = 0;

				init_plugin_path(preferences, string, 1);
			}

			if(ptr)
				ptr1 = ptr + 1;
			else
				ptr1 = ptr;
		};
	}


// the rest was for the splash dialog




// 	FileSystem cinelerra_fs;
// 	ArrayList<FileSystem*> lad_fs;
// 	int result = 0;
// 
// 	if(debug) PRINT_TRACE
// 
// // Get directories
// 	cinelerra_fs.set_filter("[*.plugin][*.so]");
// 	result = cinelerra_fs.update(preferences->plugin_dir);
// 
// 	if(debug) PRINT_TRACE
// 
// 	if(result)
// 	{
// 		fprintf(stderr, 
// 			_("MWindow::init_plugins: couldn't open %s directory\n"),
// 			preferences->plugin_dir);
// 	}
// 
// 	if(debug) PRINT_TRACE
// 	
// 	
// 	
// 	
// 	
// 	
// 	
// 	
// 	
// 	
// 
// // Parse LAD environment variable
// 	char *env = getenv("LADSPA_PATH");
// 	if(env)
// 	{
// 		char string[BCTEXTLEN];
// 		char *ptr1 = env;
// 		while(ptr1)
// 		{
// 			char *ptr = strchr(ptr1, ':');
// 			char *end;
// 			if(ptr)
// 			{
// 				end = ptr;
// 			}
// 			else
// 			{
// 				end = env + strlen(env);
// 			}
// 
// 			if(end > ptr1)
// 			{
// 				int len = end - ptr1;
// 				memcpy(string, ptr1, len);
// 				string[len] = 0;
// 
// 
// 				FileSystem *fs = new FileSystem;
// 				lad_fs.append(fs);
// 				fs->set_filter("*.so");
// 				result = fs->update(string);
// 
// 				if(result)
// 				{
// 					fprintf(stderr, 
// 						_("MWindow::init_plugins: couldn't open %s directory\n"),
// 						string);
// 				}
// 			}
// 
// 			if(ptr)
// 				ptr1 = ptr + 1;
// 			else
// 				ptr1 = ptr;
// 		};
// 	}
// 
// 	if(debug) PRINT_TRACE
// 
// 	int total = cinelerra_fs.total_files();
// 	int counter = 0;
// 	for(int i = 0; i < lad_fs.total; i++)
// 		total += lad_fs.values[i]->total_files();
// 	if(splash_window) splash_window->progress->update_length(total);
// 
// 	if(debug) PRINT_TRACE
// 
// 
// // Cinelerra
// 	init_plugin_path(preferences,
// 		&cinelerra_fs,
// 		splash_window,
// 		&counter);
// 
// // LAD
// 	for(int i = 0; i < lad_fs.total; i++)
// 		init_plugin_path(preferences,
// 			lad_fs.values[i],
// 			splash_window,
// 			&counter);
// 
// 	if(debug) PRINT_TRACE
// 
// 	lad_fs.remove_all_objects();
// 	if(debug) PRINT_TRACE



}

void MWindow::delete_plugins()
{
	for(int i = 0; i < plugindb->total; i++)
	{
		delete plugindb->values[i];
	}
	delete plugindb;
	plugindb = 0;
}

void MWindow::search_plugindb(int do_audio, 
		int do_video, 
		int is_realtime, 
		int is_transition,
		int is_theme,
		ArrayList<PluginServer*> &results)
{
// Get plugins
	for(int i = 0; i < MWindow::plugindb->total; i++)
	{
		PluginServer *current = MWindow::plugindb->values[i];

		if(current->audio == do_audio &&
			current->video == do_video &&
			(current->realtime == is_realtime || is_realtime < 0) &&
			current->transition == is_transition &&
			current->theme == is_theme)
			results.append(current);
	}

// Alphabetize list by title
	int done = 0;
	while(!done)
	{
		done = 1;
		
		for(int i = 0; i < results.total - 1; i++)
		{
			PluginServer *value1 = results.values[i];
			PluginServer *value2 = results.values[i + 1];
			if(strcmp(_(value1->title), _(value2->title)) > 0)
			{
				done = 0;
				results.values[i] = value2;
				results.values[i + 1] = value1;
			}
		}
	}
}

PluginServer* MWindow::scan_plugindb(char *title,
		int data_type)
{
// 	if(data_type < 0)
// 	{
// 		printf("MWindow::scan_plugindb data_type < 0\n");
// 		return 0;
// 	}

	for(int i = 0; i < plugindb->total; i++)
	{
		PluginServer *server = plugindb->values[i];
		if(server->title &&
			!strcasecmp(server->title, title) &&
			(data_type < 0 ||
				(data_type == TRACK_AUDIO && server->audio) ||
				(data_type == TRACK_VIDEO && server->video))) 
			return plugindb->values[i];
	}
	return 0;
}

void MWindow::init_preferences()
{
	preferences = new Preferences;
	preferences->load_defaults(defaults);
	session = new MainSession(this);
	session->load_defaults(defaults);
}

void MWindow::clean_indexes()
{
	FileSystem fs;
	int total_excess;
	long oldest;
	int oldest_item = -1;
	int result;
	char string[BCTEXTLEN];
	char string2[BCTEXTLEN];

// Delete extra indexes
	fs.set_filter("*.idx");
	fs.complete_path(preferences->index_directory);
	fs.update(preferences->index_directory);
//printf("MWindow::clean_indexes 1 %d\n", fs.dir_list.total);

// Eliminate directories
	result = 1;
	while(result)
	{
		result = 0;
		for(int i = 0; i < fs.dir_list.total && !result; i++)
		{
			fs.join_names(string, preferences->index_directory, fs.dir_list.values[i]->name);
			if(fs.is_dir(string))
			{
				delete fs.dir_list.values[i];
				fs.dir_list.remove_number(i);
				result = 1;
			}
		}
	}
	total_excess = fs.dir_list.total - preferences->index_count;

//printf("MWindow::clean_indexes 2 %d\n", fs.dir_list.total);
	while(total_excess > 0)
	{
// Get oldest
		for(int i = 0; i < fs.dir_list.total; i++)
		{
			fs.join_names(string, preferences->index_directory, fs.dir_list.values[i]->name);

			if(i == 0 || fs.get_date(string) <= oldest)
			{
				oldest = fs.get_date(string);
				oldest_item = i;
			}
		}

		if(oldest_item >= 0)
		{
// Remove index file
			fs.join_names(string, 
				preferences->index_directory, 
				fs.dir_list.values[oldest_item]->name);
//printf("MWindow::clean_indexes 1 %s\n", string);
			if(remove(string))
				perror("delete_indexes");
			delete fs.dir_list.values[oldest_item];
			fs.dir_list.remove_number(oldest_item);

// Remove table of contents if it exists
			strcpy(string2, string);
			char *ptr = strrchr(string2, '.');
			if(ptr)
			{
//printf("MWindow::clean_indexes 2 %s\n", string2);
				sprintf(ptr, ".toc");
				remove(string2);
			}
		}

		total_excess--;
	}
}

void MWindow::init_awindow()
{
	awindow = new AWindow(this);
	awindow->create_objects();
}

void MWindow::init_gwindow()
{
	gwindow = new GWindow(this);
	gwindow->create_objects();
}

void MWindow::init_tipwindow()
{
	twindow = new TipWindow(this);
	twindow->start();
}

void MWindow::init_theme()
{
	Timer timer;
	theme = 0;

// Replace blond theme with SUV since it doesn't work
	if(!strcasecmp(preferences->theme, "Blond"))
		strcpy(preferences->theme, DEFAULT_THEME);

	for(int i = 0; i < plugindb->total; i++)
	{
		if(plugindb->values[i]->theme &&
			!strcasecmp(preferences->theme, plugindb->values[i]->title))
		{
			PluginServer plugin = *plugindb->values[i];
			plugin.open_plugin(0, preferences, 0, 0, -1);
			theme = plugin.new_theme();
			theme->mwindow = this;
			strcpy(theme->path, plugin.path);
			plugin.close_plugin();
		}
	}

	if(!theme)
	{
		fprintf(stderr, _("MWindow::init_theme: theme %s not found.\n"), preferences->theme);
		exit(1);
	}

// Load default images & settings
	theme->Theme::initialize();
// Load user images & settings
	theme->initialize();
// Create menus with user colors
	theme->build_menus();
	init_menus();

	theme->check_used();

//printf("MWindow::init_theme %d total_time=%d\n", __LINE__, (int)timer.get_difference());
}

void MWindow::init_3d()
{
	playback_3d = new Playback3D(this);
	playback_3d->create_objects();
}

void MWindow::init_edl()
{
	edl = new EDL;
	edl->create_objects();
    edl->load_defaults(defaults);
	edl->create_default_tracks();
	edl->tracks->update_y_pixels(theme);
}

void MWindow::init_compositor()
{
	cwindow = new CWindow(this);
    cwindow->create_objects();
}

void MWindow::init_levelwindow()
{
	lwindow = new LevelWindow(this);
	lwindow->create_objects();
}

VWindow* MWindow::new_viewer(int start_it)
{
// Default vwindow
	VWindow *vwindow = new VWindow(this);
	vwindows.append(vwindow);
	vwindow->load_defaults();
	vwindow->create_objects();
	if(start_it) vwindow->start();
	return vwindow;
}

void MWindow::init_cache()
{
	audio_cache = new CICache(preferences);
	video_cache = new CICache(preferences);
	frame_cache = new FrameCache;
	wave_cache = new WaveCache;
}

void MWindow::init_channeldb()
{
	channeldb_buz->load("channeldb_buz");
	channeldb_v4l2jpeg->load("channeldb_v4l2jpeg");
}

void MWindow::init_menus()
{
	char string[BCTEXTLEN];
	cmodel_to_text(string, BC_RGB888);
	colormodels.append(new ColormodelItem(string, BC_RGB888));
	cmodel_to_text(string, BC_RGBA8888);
	colormodels.append(new ColormodelItem(string, BC_RGBA8888));
//	cmodel_to_text(string, BC_RGB161616);
//	colormodels.append(new ColormodelItem(string, BC_RGB161616));
//	cmodel_to_text(string, BC_RGBA16161616);
//	colormodels.append(new ColormodelItem(string, BC_RGBA16161616));
	cmodel_to_text(string, BC_RGB_FLOAT);
	colormodels.append(new ColormodelItem(string, BC_RGB_FLOAT));
	cmodel_to_text(string, BC_RGBA_FLOAT);
	colormodels.append(new ColormodelItem(string, BC_RGBA_FLOAT));
	cmodel_to_text(string, BC_YUV888);
	colormodels.append(new ColormodelItem(string, BC_YUV888));
	cmodel_to_text(string, BC_YUVA8888);
	colormodels.append(new ColormodelItem(string, BC_YUVA8888));
//	cmodel_to_text(string, BC_YUV161616);
//	colormodels.append(new ColormodelItem(string, BC_YUV161616));
//	cmodel_to_text(string, BC_YUVA16161616);
//	colormodels.append(new ColormodelItem(string, BC_YUVA16161616));
}

void MWindow::init_indexes()
{
	mainindexes = new MainIndexes(this);
	mainindexes->start_loop();
}

void MWindow::init_gui()
{
	gui = new MWindowGUI(this);
	gui->lock_window("MWindow::init_gui");
	gui->create_objects();
	gui->unlock_window();
	gui->load_defaults(defaults);
}

void MWindow::init_signals()
{
	sighandler = new SigHandler;
	sighandler->initialize();
ENABLE_BUFFER
}

void MWindow::init_render()
{
	render = new Render(this);
//	renderlist = new Render(this);
	batch_render = new BatchRenderThread(this);
}

void MWindow::init_brender()
{
	if(preferences->use_brender && !brender)
	{
		brender_lock->lock("MWindow::init_brender 1");
		brender = new BRender(this);
		brender->initialize();
		session->brender_end = 0;
		brender_lock->unlock();
	}
	else
	if(!preferences->use_brender && brender)
	{
		brender_lock->lock("MWindow::init_brender 2");
		delete brender;
		brender = 0;
		session->brender_end = 0;
		brender_lock->unlock();
	}
	if(brender) brender->restart(edl);
}

void MWindow::restart_brender()
{
//printf("MWindow::restart_brender 1\n");
	if(brender) brender->restart(edl);
}

void MWindow::stop_brender()
{
	if(brender) brender->stop();
}

int MWindow::brender_available(int position)
{
	int result = 0;
	brender_lock->lock("MWindow::brender_available 1");
	if(brender)
	{
		if(brender->map_valid)
		{
			brender->map_lock->lock("MWindow::brender_available 2");
			if(position < brender->map_size &&
				position >= 0)
			{
//printf("MWindow::brender_available 1 %d %d\n", position, brender->map[position]);
				if(brender->map[position] == BRender::RENDERED)
					result = 1;
			}
			brender->map_lock->unlock();
		}
	}
	brender_lock->unlock();
	return result;
}

void MWindow::set_brender_range()
{
	edl->session->brender_start = edl->local_session->get_selectionstart(1);
	edl->session->brender_end = edl->local_session->get_selectionend(1);

	if(EQUIV(edl->session->brender_end, edl->session->brender_start))
	{
		edl->session->brender_end = edl->tracks->total_video_length();
	}

	restart_brender();
	gui->draw_overlays(1);
}



int MWindow::load_filenames(ArrayList<char*> *filenames, 
	int load_mode,
	int update_filename)
{
	ArrayList<EDL*> new_edls;
	ArrayList<Asset*> new_assets;
	ArrayList<File*> new_files;
	const int debug = 0;
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

//	save_defaults();
	gui->start_hourglass();

// Need to stop playback since tracking depends on the EDL not getting
// deleted.
	cwindow->playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	cwindow->playback_engine->interrupt_playback(0);
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

	for(int i = 0; i < vwindows.size(); i++)
	{
		VWindow *vwindow = vwindows.get(i);
		if(vwindow->running())
		{
			vwindow->playback_engine->que->send_command(STOP,
				CHANGE_NONE, 
				0,
				0);
			vwindow->playback_engine->interrupt_playback(0);
		}
	}

if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	undo->update_undo_before();


if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Define new_edls and new_assets to load
	int result = 0;
	for(int i = 0; i < filenames->size(); i++)
	{
// Get type of file
		File *new_file = new File;
		Asset *new_asset = new Asset(filenames->get(i));
		EDL *new_edl = new EDL;
		char string[BCTEXTLEN];

		new_edl->create_objects();
		new_edl->copy_session(edl);

		sprintf(string, "Loading %s", new_asset->path);
		gui->show_message(string);
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

		result = new_file->open_file(preferences, new_asset, 1, 0);
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);


		switch(result)
		{
// Convert media file to EDL
			case FILE_OK:
// Warn about odd image dimensions
				if(new_asset->video_data &&
					((new_asset->width % 2) ||
					(new_asset->height % 2)))
				{
					char string[BCTEXTLEN];
					sprintf(string, "%s's resolution is %dx%d.\nImages with odd dimensions may not decode properly.",
						new_asset->path,
						new_asset->width,
						new_asset->height);
					MainError::show_error(string);
				}


				if(load_mode != LOADMODE_RESOURCESONLY)
				{
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
					asset_to_edl(new_edl, new_asset);
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
					new_edls.append(new_edl);
					new_asset->Garbage::remove_user();
					new_asset = 0;
				}
				else
				{
					new_assets.append(new_asset);
				}

// Set filename to nothing for assets since save EDL would overwrite them.
				if(load_mode == LOADMODE_REPLACE || 
					load_mode == LOADMODE_REPLACE_CONCATENATE)
				{
					set_filename("");
// Reset timeline position
					for(int i = 0; i < TOTAL_PANES; i++)
					{
						new_edl->local_session->view_start[i] = 0;
						new_edl->local_session->track_start[i] = 0;
					}
				}

				result = 0;
				break;

// File not found
			case FILE_NOT_FOUND:
				sprintf(string, _("Failed to open %s"), new_asset->path);
				gui->show_message(string, theme->message_error);
				result = 1;
				break;

// Unknown format
			case FILE_UNRECOGNIZED_CODEC:
			{
// Test index file
				IndexFile indexfile(this, new_asset);
				result = indexfile.open_index();
				if(!result)
				{
					indexfile.close_index();
				}

// Test existing EDLs
				if(result)
				{
					for(int j = 0; j < new_edls.total + 1; j++)
					{
						Asset *old_asset;
						if(j == new_edls.total)
						{
							if(old_asset = edl->assets->get_asset(new_asset->path))
							{
								*new_asset = *old_asset;
								result = 0;
							}
						}
						else
						{
							if(old_asset = new_edls.values[j]->assets->get_asset(new_asset->path))
							{
								*new_asset = *old_asset;
								result = 0;
							}
						}
					}
				}

// Prompt user
				if(result)
				{
					char string[BCTEXTLEN];
					FileSystem fs;
					fs.extract_name(string, new_asset->path);

					strcat(string, _("'s format couldn't be determined."));
					new_asset->audio_data = 1;
					new_asset->format = FILE_PCM;
					new_asset->channels = defaults->get("AUDIO_CHANNELS", 2);
					new_asset->sample_rate = defaults->get("SAMPLE_RATE", 44100);
					new_asset->bits = defaults->get("AUDIO_BITS", 16);
					new_asset->byte_order = defaults->get("BYTE_ORDER", 1);
					new_asset->signed_ = defaults->get("SIGNED_", 1);
					new_asset->header = defaults->get("HEADER", 0);

					FileFormat fwindow(this);
					fwindow.create_objects(new_asset, string);
					result = fwindow.run_window();


					defaults->update("AUDIO_CHANNELS", new_asset->channels);
					defaults->update("SAMPLE_RATE", new_asset->sample_rate);
					defaults->update("AUDIO_BITS", new_asset->bits);
					defaults->update("BYTE_ORDER", new_asset->byte_order);
					defaults->update("SIGNED_", new_asset->signed_);
					defaults->update("HEADER", new_asset->header);
					save_defaults();
				}

// Append to list
				if(!result)
				{
// Recalculate length
					delete new_file;
					new_file = new File;
					result = new_file->open_file(preferences, new_asset, 1, 0);

					if(load_mode != LOADMODE_RESOURCESONLY)
					{
						asset_to_edl(new_edl, new_asset);
						new_edls.append(new_edl);
						new_asset->Garbage::remove_user();
						new_asset = 0;
					}
					else
					{
						new_assets.append(new_asset);
					}
				}
				else
				{
					result = 1;
				}
				break;
			}

			case FILE_IS_XML:
			{
				FileXML xml_file;
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
				xml_file.read_from_file(filenames->values[i]);
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

				if(load_mode == LOADMODE_NESTED)
				{
// Load temporary EDL for nesting.
					EDL *nested_edl = new EDL;
					nested_edl->create_objects();
					nested_edl->set_path(filenames->values[i]);
					nested_edl->load_xml(&xml_file, LOAD_ALL);
//printf("MWindow::load_filenames %p %s\n", nested_edl, nested_edl->project_path);
					edl_to_nested(new_edl, nested_edl);
					nested_edl->Garbage::remove_user();
				}
				else
				{
// Load EDL for pasting
					new_edl->load_xml(&xml_file, LOAD_ALL);
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
					test_plugins(new_edl, filenames->values[i]);
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

					if(load_mode == LOADMODE_REPLACE || 
						load_mode == LOADMODE_REPLACE_CONCATENATE)
					{
						strcpy(session->filename, filenames->values[i]);
						strcpy(new_edl->local_session->clip_title, 
							filenames->values[i]);
						if(update_filename)
							set_filename(new_edl->local_session->clip_title);
					}
				}		

				new_edls.append(new_edl);
				result = 0;
				break;
			}
		}

		if(result)
		{
			new_edl->Garbage::remove_user();
			new_asset->Garbage::remove_user();
			new_edl = 0;
			new_asset = 0;
		}

// Store for testing index
		new_files.append(new_file);
	}

if(debug) printf("MWindow::load_filenames %d\n", __LINE__);


	if(!result) gui->statusbar->default_message();







if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Paste them.
// Don't back up here.
	if(new_edls.size())
	{
// For pasting, clear the active region
		if(load_mode == LOADMODE_PASTE ||
			load_mode == LOADMODE_NESTED)
		{
			double start = edl->local_session->get_selectionstart();
			double end = edl->local_session->get_selectionend();
			if(!EQUIV(start, end))
				edl->clear(start, 
					end,
					edl->session->labels_follow_edits,
					edl->session->plugins_follow_edits,
					edl->session->autos_follow_edits);
		}

		paste_edls(&new_edls, 
			load_mode,
			0,
			-1,
			edl->session->labels_follow_edits, 
			edl->session->plugins_follow_edits,
			edl->session->autos_follow_edits);
	}




if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Add new assets to EDL and schedule assets for index building.
	int got_indexes = 0;
	for(int i = 0; i < new_edls.size(); i++)
	{
		EDL *new_edl = new_edls.get(i);
		for(int j = 0; j < new_edl->nested_edls->size(); j++)
		{
			mainindexes->add_next_asset(0, 
				new_edl->nested_edls->get(j));
			got_indexes = 1;
			edl->nested_edls->update_index(new_edl->nested_edls->get(j));
		}

	}

if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	if(new_assets.size())
	{
		for(int i = 0; i < new_assets.size(); i++)
		{
			Asset *new_asset = new_assets.get(i);

			File *new_file = 0;
			File *index_file = 0;
			int got_it = 0;
			for(int j = 0; j < new_files.size(); j++)
			{
				new_file = new_files.get(j);
				if(!strcmp(new_file->asset->path,
					new_asset->path))
				{
					got_it = 1;
					break;
				}
			}

			mainindexes->add_next_asset(got_it ? new_file : 0, 
				new_asset);
			got_indexes = 1;
			edl->assets->update(new_asset);

		}


	}
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Start examining next batch of index files
	if(got_indexes) mainindexes->start_build();
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

// Open plugin GUIs
	Track *track = edl->tracks->first;
	while(track)
	{
		for(int j = 0; j < track->plugin_set.size(); j++)
		{
			PluginSet *plugins = track->plugin_set.get(j);
			Plugin *plugin = plugins->get_first_plugin();

			while(plugin)
			{
				if(load_mode == LOADMODE_REPLACE ||
					load_mode == LOADMODE_REPLACE_CONCATENATE)
				{
					if(plugin->plugin_type == PLUGIN_STANDALONE &&
						plugin->show)
					{
						show_plugin(plugin);
					}
				}
				else
				{
					plugin->show = 0;
				}

				plugin = (Plugin*)plugin->next;
			}
		}

		track = track->next;
	}

// need to update undo before project, since mwindow is unlocked & a new load
// can begin here.  Should really prevent loading until we're done.
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	undo->update_undo_after(_("load"), LOAD_ALL);

	for(int i = 0; i < new_edls.size(); i++)
	{
		new_edls.get(i)->remove_user();
	}
if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

	new_edls.remove_all();

	for(int i = 0; i < new_assets.size(); i++)
	{
		new_assets.get(i)->Garbage::remove_user();
	}

	new_assets.remove_all();
	new_files.remove_all_objects();


if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	if(load_mode == LOADMODE_REPLACE ||
		load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		session->changes_made = 0;
	}
	else
	{
		session->changes_made = 1;
	}

	gui->stop_hourglass();


if(debug) printf("MWindow::load_filenames %d\n", __LINE__);
	update_project(load_mode);

if(debug) printf("MWindow::load_filenames %d\n", __LINE__);

	return 0;
}




void MWindow::test_plugins(EDL *new_edl, char *path)
{
	char string[BCTEXTLEN];

// Do a check weather plugins exist
	for(Track *track = new_edl->tracks->first; track; track = track->next)
	{
		for(int k = 0; k < track->plugin_set.total; k++)
		{
			PluginSet *plugin_set = track->plugin_set.values[k];
			for(Plugin *plugin = (Plugin*)plugin_set->first; 
				plugin; 
				plugin = (Plugin*)plugin->next)
			{
				if(plugin->plugin_type == PLUGIN_STANDALONE)
				{
// ok we need to find it in plugindb
					int plugin_found = 0;

					for(int j = 0; j < plugindb->size(); j++)
					{
						PluginServer *server = plugindb->get(j);
						if(server->title &&
							!strcasecmp(server->title, plugin->title) &&
							((track->data_type == TRACK_AUDIO && server->audio) ||
							(track->data_type == TRACK_VIDEO && server->video)) &&
							(!server->transition))
							plugin_found = 1;
					}

					if (!plugin_found) 
					{
						sprintf(string, 
							"The effect '%s' in file '%s' is not part of your installation of Cinelerra.\n"
							"The project won't be rendered as it was meant and Cinelerra might crash.\n",
							plugin->title, 
							path); 
						MainError::show_error(string);
					}
				}
			}
		}

		for(Edit *edit = (Edit*)track->edits->first; 
			edit; 
			edit = (Edit*)edit->next)
		{
			if (edit->transition)
			{
// ok we need to find transition in plugindb

				int transition_found = 0;
				for(int j = 0; j < plugindb->size(); j++)
				{
					PluginServer *server = plugindb->get(j);
					if(server->title &&
						!strcasecmp(server->title, edit->transition->title) &&
						((track->data_type == TRACK_AUDIO && server->audio) ||
						(track->data_type == TRACK_VIDEO && server->video)) &&
						(server->transition))
						transition_found = 1;
				}

				if (!transition_found) 
				{
					sprintf(string, 
						"The transition '%s' in file '%s' is not part of your installation of Cinelerra.\n"
						"The project won't be rendered as it was meant and Cinelerra might crash.\n",
						edit->transition->title, 
						path); 
					MainError::show_error(string);
				}
			}
		}
	}
}


void MWindow::init_shm()
{
// Fix shared memory
	FILE *fd = fopen("/proc/sys/kernel/shmmax", "w");
	if(fd)
	{
		fprintf(fd, "0x7fffffff");
		fclose(fd);
	}
	fd = 0;

	fd = fopen("/proc/sys/kernel/shmmax", "r");
	if(!fd)
	{
		MainError::show_error("MWindow::init_shm: couldn't open /proc/sys/kernel/shmmax for reading.\n");
		return;
	}

	int64_t result = 0;
	long long temp;
	int temp2;
	temp2 = fscanf(fd, "%lld", &temp);
	result = temp;
	fclose(fd);
	fd = 0;
	if(result < 0x7fffffff)
	{
		char string[BCTEXTLEN];
		sprintf(string, "MWindow::init_shm: /proc/sys/kernel/shmmax is 0x%llx.\n"
			"It should be at least 0x7fffffff for Cinelerra.\n",
			(long long)result);
		MainError::show_error(string);
	}
}


void MWindow::init_fileserver(Preferences *preferences)
{
#ifdef USE_FILEFORK
	file_server = new FileServer(preferences);
	file_server->start();
#endif
}

void MWindow::create_objects(int want_gui, 
	int want_new,
	char *config_path)
{
	char string[BCTEXTLEN];
	FileSystem fs;
	const int debug = 0;
	edl = 0;


	if(debug) PRINT_TRACE

// For some reason, init_signals must come after show_splash or the signals won't
// get trapped.
	init_signals();

	if(debug) PRINT_TRACE


	init_3d();

	if(debug) PRINT_TRACE


// This is required in the top thread for Alsa to work

#ifdef HAVE_ALSA
	ArrayList<char*> *alsa_titles = new ArrayList<char*>;
	AudioALSA::list_devices(alsa_titles, 0, MODEPLAY);
	alsa_titles->remove_all_objects();
	delete alsa_titles;
#endif

	if(debug) PRINT_TRACE


	remove_thread = new RemoveThread;
	remove_thread->create_objects();
//	show_splash();



	init_error();
	if(debug) PRINT_TRACE

	init_defaults(defaults, config_path);
	init_preferences();
	if(debug) PRINT_TRACE

	init_plugins(preferences, splash_window);
	if(debug) PRINT_TRACE
	if(splash_window) splash_window->operation->update(_("Initializing GUI"));
	if(debug) PRINT_TRACE
	init_theme();
	if(debug) PRINT_TRACE


// Initialize before too much else is running
// Preferences & theme are required for building MPEG table of contents
	init_fileserver(preferences);

// Default project created here
	init_edl();
	if(debug) PRINT_TRACE

	Timer timer;

	init_compositor();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

//printf("MWindow::create_objects %d session->show_vwindow=%d\n", __LINE__, session->show_vwindow);
	if(session->show_vwindow) new_viewer(1);
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_gui();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_awindow();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_levelwindow();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_cache();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_indexes();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	init_channeldb();

	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	init_gwindow();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	init_render();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	init_brender();
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	mainprogress = new MainProgress(this, gui);
	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	undo = new MainUndo(this);

	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	plugin_guis = new ArrayList<PluginServer*>;
	dead_plugins = new ArrayList<PluginServer*>;
	keyframe_threads = new ArrayList<KeyFrameThread*>;

	if(debug) printf("MWindow::create_objects %d vwindows=%d show_vwindow=%d\n", 
		__LINE__, 
		vwindows.size(),
		session->show_vwindow);

// Show all vwindows
// 	if(session->show_vwindow)
// 	{
// 		for(int j = 0; j < vwindows.size(); j++)
// 		{
// 			VWindow *vwindow = vwindows.get(j);
// 			if(debug) printf("MWindow::create_objects %d vwindow=%p\n", 
// 				__LINE__, 
// 				vwindow);
// 			if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
// 			vwindow->gui->lock_window("MWindow::create_objects 1");
// 			if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
// 			vwindow->gui->show_window();
// 			if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
// 			vwindow->gui->unlock_window();
// 		}
// 	}


	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());

	if(session->show_cwindow) 
	{
		cwindow->gui->lock_window("MWindow::create_objects 1");
		cwindow->gui->show_window();
		cwindow->gui->unlock_window();
	}

	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	if(session->show_awindow)
	{
		awindow->gui->lock_window("MWindow::create_objects 1");
		awindow->gui->show_window();
		awindow->gui->unlock_window();
	}

	if(debug) printf("MWindow::create_objects %d total_time=%d\n", __LINE__, (int)timer.get_difference());
	if(session->show_lwindow)
	{
		lwindow->gui->lock_window("MWindow::create_objects 1");
		lwindow->gui->show_window();
		lwindow->gui->unlock_window();
	}

	if(debug) printf("MWindow::create_objects %d total_time=%d gwindow=%p\n", 
		__LINE__, 
		(int)timer.get_difference(),
		gwindow->gui);
	if(session->show_gwindow)
	{
		gwindow->gui->lock_window("MWindow::create_objects 1");
		gwindow->gui->show_window();
		gwindow->gui->unlock_window();
	}

	if(debug) PRINT_TRACE

	gui->lock_window("MWindow::create_objects 1");
	gui->mainmenu->load_defaults(defaults);
	gui->mainmenu->update_toggles(0);
	gui->update_patchbay();
	gui->draw_canvas(0, 0);
	gui->draw_cursor(1);
	gui->show_window();
	gui->raise_window();
	gui->unlock_window();

	if(debug) PRINT_TRACE



	if(preferences->use_tipwindow)
		init_tipwindow();
	if(debug) PRINT_TRACE


	hide_splash();
	init_shm();
	if(debug) PRINT_TRACE

	BC_WindowBase::get_resources()->vframe_shm = 1;

}


void MWindow::show_splash()
{
#include "data/heroine_logo11_png.h"
	VFrame *frame = new VFrame(heroine_logo11_png);
	BC_DisplayInfo display_info;
	splash_window = new SplashGUI(frame, 
		display_info.get_root_w() / 2 - frame->get_w() / 2,
		display_info.get_root_h() / 2 - frame->get_h() / 2);
	splash_window->create_objects();
}

void MWindow::hide_splash()
{
	if(splash_window)
		delete splash_window;
	splash_window = 0;
}


void MWindow::start()
{
ENABLE_BUFFER
//PRINT_TRACE
//	vwindows.get(DEFAULT_VWINDOW)->start();
	awindow->start();
//PRINT_TRACE
	cwindow->start();
//PRINT_TRACE
	lwindow->start();
//PRINT_TRACE
	gwindow->start();
//PRINT_TRACE
	Thread::start();
//PRINT_TRACE
	playback_3d->start();
//PRINT_TRACE
}

void MWindow::run()
{
	gui->run_window();
}

void MWindow::show_vwindow()
{
	int total_running = 0;
	session->show_vwindow = 1;

//printf("MWindow::show_vwindow %d %d\n", __LINE__, vwindows.size());

// Raise all windows which are visible
	for(int j = 0; j < vwindows.size(); j++)
	{
		VWindow *vwindow = vwindows.get(j);
		if(vwindow->is_running())
		{
			vwindow->gui->lock_window("MWindow::show_vwindow");
			vwindow->gui->show_window(0);
			vwindow->gui->raise_window();
			vwindow->gui->flush();
			vwindow->gui->unlock_window();
			total_running++;
		}
	}

// If no windows visible but thread exists, start it
	if(!total_running)
	{
		if(vwindows.size())
		{
			vwindows.get(DEFAULT_VWINDOW)->start();
		}
	}

// No visible windows & no existing thread
	if(!vwindows.size())
	{
		new_viewer(1);
	}

	gui->mainmenu->show_vwindow->set_checked(1);
}

void MWindow::show_awindow()
{
	session->show_awindow = 1;
	awindow->gui->lock_window("MWindow::show_awindow");
	awindow->gui->show_window();
	awindow->gui->raise_window();
	awindow->gui->flush();
	awindow->gui->unlock_window();
	gui->mainmenu->show_awindow->set_checked(1);
}

void MWindow::show_cwindow()
{
	session->show_cwindow = 1;
	cwindow->show_window();
	gui->mainmenu->show_cwindow->set_checked(1);
}

void MWindow::show_gwindow()
{
	session->show_gwindow = 1;

	gwindow->gui->lock_window("MWindow::show_gwindow");
	gwindow->gui->show_window();
	gwindow->gui->raise_window();
	gwindow->gui->flush();
	gwindow->gui->unlock_window();

	gui->mainmenu->show_gwindow->set_checked(1);
}

void MWindow::show_lwindow()
{
	session->show_lwindow = 1;
	lwindow->gui->lock_window("MWindow::show_lwindow");
	lwindow->gui->show_window();
	lwindow->gui->raise_window();
	lwindow->gui->flush();
	lwindow->gui->unlock_window();
	gui->mainmenu->show_lwindow->set_checked(1);
}

void MWindow::tile_windows()
{
	session->default_window_positions();
	gui->default_positions();
}

void MWindow::toggle_loop_playback()
{
	edl->local_session->loop_playback = !edl->local_session->loop_playback;
	set_loop_boundaries();
	save_backup();

	gui->draw_overlays(1);
	sync_parameters(CHANGE_PARAMS);
}

//void MWindow::set_titles(int value)
//{
//	edl->session->show_titles = value;
//	trackmovement(edl->local_session->track_start);
//}

void MWindow::set_auto_keyframes(int value, int lock_mwindow, int lock_cwindow)
{
	if(lock_mwindow) gui->lock_window("MWindow::set_auto_keyframes");
	edl->session->auto_keyframes = value;
	gui->mbuttons->edit_panel->keyframe->update(value);
	gui->flush();
	if(lock_mwindow) gui->unlock_window();

	if(lock_cwindow) cwindow->gui->lock_window("MWindow::set_auto_keyframes");
	cwindow->gui->edit_panel->keyframe->update(value);
	cwindow->gui->flush();
	if(lock_cwindow) cwindow->gui->unlock_window();
}

void MWindow::set_keyframe_type(int mode)
{
	gui->lock_window("MWindow::set_keyframe_type");
	edl->local_session->floatauto_type = mode;
	gui->mainmenu->update_toggles(0);
	gui->unlock_window();
}

int MWindow::set_editing_mode(int new_editing_mode, int lock_mwindow, int lock_cwindow)
{
	if(lock_mwindow) gui->lock_window("MWindow::set_editing_mode");
	edl->session->editing_mode = new_editing_mode;
	gui->mbuttons->edit_panel->editing_mode = edl->session->editing_mode;
	gui->mbuttons->edit_panel->update();
	gui->set_editing_mode(1);
	if(lock_mwindow) gui->unlock_window();


	if(lock_cwindow) cwindow->gui->lock_window("MWindow::set_editing_mode");
	cwindow->gui->edit_panel->update();
	cwindow->gui->edit_panel->editing_mode = edl->session->editing_mode;
	if(lock_cwindow) cwindow->gui->unlock_window();
	return 0;
}


void MWindow::sync_parameters(int change_type)
{

// Sync engines which are playing back
	if(cwindow->playback_engine->is_playing_back)
	{
		if(change_type == CHANGE_PARAMS)
		{
// TODO: block keyframes until synchronization is done
			cwindow->playback_engine->sync_parameters(edl);
		}
		else
// Stop and restart
		{
			int command = cwindow->playback_engine->command->command;
			cwindow->playback_engine->que->send_command(STOP,
				CHANGE_NONE, 
				0,
				0);
// Waiting for tracking to finish would make the restart position more
// accurate but it can't lock the window to stop tracking for some reason.
// Not waiting for tracking gives a faster response but restart position is
// only as accurate as the last tracking update.
			cwindow->playback_engine->interrupt_playback(0);
			cwindow->playback_engine->que->send_command(command,
					change_type, 
					edl,
					1,
					0);
		}
	}
	else
	{
		cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
							change_type,
							edl,
							1);
	}
}

void MWindow::age_caches()
{
	int64_t prev_memory_usage = 0;
	int64_t memory_usage = 0;
	int result = 0;

	do
	{
//printf("MWindow::age_caches %d %lld %lld\n", __LINE__, memory_usage, preferences->cache_size);
		memory_usage = audio_cache->get_memory_usage(1);
//printf("MWindow::age_caches %d %lld %lld\n", __LINE__, memory_usage, preferences->cache_size);
		memory_usage += video_cache->get_memory_usage(1);
//printf("MWindow::age_caches %d %lld %lld\n", __LINE__, memory_usage, preferences->cache_size);
		memory_usage += frame_cache->get_memory_usage();
		memory_usage += wave_cache->get_memory_usage();
// printf("MWindow::age_caches %d %lld %lld %lld %lld\n", 
// __LINE__, 
// preferences->cache_size,
// audio_cache->get_memory_usage(1),
// video_cache->get_memory_usage(1),
// frame_cache->get_memory_usage(), 
// wave_cache->get_memory_usage(),
// memory_usage);

		if(memory_usage > preferences->cache_size)
		{
			int target = 1;
			int oldest1 = audio_cache->get_oldest();
			int oldest2 = video_cache->get_oldest();
			if(oldest2 < oldest1) target = 2;
			int oldest3 = frame_cache->get_oldest();
			if(oldest3 < oldest1 && oldest3 < oldest2) target = 3;
			int oldest4 = wave_cache->get_oldest();
			if(oldest4 < oldest3 && oldest4 < oldest2 && oldest4 < oldest1) target = 4;

//printf("MWindow::age_caches %d %d\n", __LINE__, target);

			switch(target)
			{
				case 1: audio_cache->delete_oldest(); break;
				case 2: video_cache->delete_oldest(); break;
				case 3: frame_cache->delete_oldest(); break;
				case 4: wave_cache->delete_oldest(); break;
			}
		}
		prev_memory_usage = memory_usage;
		memory_usage = audio_cache->get_memory_usage(1) +
			video_cache->get_memory_usage(1) +
			frame_cache->get_memory_usage() +
			wave_cache->get_memory_usage();
	}while(!result && 
		prev_memory_usage != memory_usage && 
		memory_usage > preferences->cache_size);
}


void MWindow::show_keyframe_gui(Plugin *plugin)
{
	keyframe_gui_lock->lock("MWindow::show_keyframe_gui");
// Find existing thread
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		if(keyframe_threads->get(i)->plugin == plugin)
		{
			keyframe_threads->get(i)->start_window(plugin, 0);
			keyframe_gui_lock->unlock();
			return;
		}
	}

// Find unused thread
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		if(!keyframe_threads->get(i)->plugin)
		{
			keyframe_threads->get(i)->start_window(plugin, 0);
			keyframe_gui_lock->unlock();
			return;
		}
	}

// Create new thread
	KeyFrameThread *thread = new KeyFrameThread(this);
	keyframe_threads->append(thread);
	thread->start_window(plugin, 0);

	keyframe_gui_lock->unlock();
}





void MWindow::show_plugin(Plugin *plugin)
{
	int done = 0;

SET_TRACE
// Remove previously deleted plugin GUIs
	dead_plugin_lock->lock("MWindow::delete_plugin");
	for(int i = 0; i < dead_plugins->size(); i++)
	{
		delete dead_plugins->get(i);
	}
	dead_plugins->remove_all();
	dead_plugin_lock->unlock();

//printf("MWindow::show_plugin %d\n", __LINE__);
SET_TRACE


	plugin_gui_lock->lock("MWindow::show_plugin");
	for(int i = 0; i < plugin_guis->total; i++)
	{
// Pointer comparison
		if(plugin_guis->values[i]->plugin == plugin)
		{
			plugin_guis->values[i]->raise_window();
			done = 1;
			break;
		}
	}
SET_TRACE

//printf("MWindow::show_plugin 1\n");
	if(!done)
	{
		if(!plugin->track)
		{
			printf("MWindow::show_plugin track not defined.\n");
		}
		PluginServer *server = scan_plugindb(plugin->title,
			plugin->track->data_type);

//printf("MWindow::show_plugin %p %d\n", server, server->uses_gui);
		if(server && server->uses_gui)
		{
			PluginServer *gui = plugin_guis->append(new PluginServer(*server));
// Needs mwindow to do GUI
			gui->set_mwindow(this);
			gui->open_plugin(0, preferences, edl, plugin, -1);
			gui->show_gui();
			plugin->show = 1;
		}
	}
	plugin_gui_lock->unlock();
//printf("MWindow::show_plugin %d\n", __LINE__);
SET_TRACE
//sleep(1);
//printf("MWindow::show_plugin 2\n");
}

void MWindow::hide_plugin(Plugin *plugin, int lock)
{
	plugin->show = 0;
// Update the toggle
	gui->lock_window("MWindow::hide_plugin");
	gui->update(0, 1, 0, 0, 0, 0, 0);
	gui->unlock_window();

	if(lock) plugin_gui_lock->lock("MWindow::hide_plugin");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin == plugin)
		{
			PluginServer *ptr = plugin_guis->values[i];
			plugin_guis->remove(ptr);
			if(lock) plugin_gui_lock->unlock();
// Last command executed in client side close
// Schedule for deletion
			ptr->hide_gui();
			delete_plugin(ptr);
//sleep(1);
//			return;
		}
	}
	if(lock) plugin_gui_lock->unlock();
}

void MWindow::delete_plugin(PluginServer *plugin)
{
	dead_plugin_lock->lock("MWindow::delete_plugin");
	dead_plugins->append(plugin);
	dead_plugin_lock->unlock();
}

void MWindow::hide_plugins()
{
	plugin_gui_lock->lock("MWindow::hide_plugins 1");
	while(plugin_guis->size())
	{
		PluginServer *ptr = plugin_guis->get(0);
		plugin_guis->remove(ptr);
		plugin_gui_lock->unlock();
// Last command executed in client side close
// Schedule for deletion
		ptr->hide_gui();
		delete_plugin(ptr);
		plugin_gui_lock->lock("MWindow::hide_plugins 2");
	}
	plugin_gui_lock->unlock();

	hide_keyframe_guis();
}

void MWindow::hide_keyframe_guis()
{
	keyframe_gui_lock->lock("MWindow::hide_keyframe_guis");
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		keyframe_threads->get(i)->close_window();
	}
	keyframe_gui_lock->unlock();
}

void MWindow::hide_keyframe_gui(Plugin *plugin)
{
	keyframe_gui_lock->lock("MWindow::hide_keyframe_gui");
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		if(keyframe_threads->get(i)->plugin = plugin)
		{
			keyframe_threads->get(i)->close_window();
			break;
		}
	}
	keyframe_gui_lock->unlock();
}

void MWindow::update_keyframe_guis()
{
// Send new configuration to keyframe GUI's
	keyframe_gui_lock->lock("MWindow::update_keyframe_guis");
	for(int i = 0; i < keyframe_threads->size(); i++)
	{
		KeyFrameThread *ptr = keyframe_threads->get(i);
		if(edl->tracks->plugin_exists(ptr->plugin))
			ptr->update_gui(1);
		else
		{
			ptr->close_window();
		}
	}
	keyframe_gui_lock->unlock();
}

void MWindow::update_plugin_guis(int do_keyframe_guis)
{
// Send new configuration to plugin GUI's
	plugin_gui_lock->lock("MWindow::update_plugin_guis");

	for(int i = 0; i < plugin_guis->size(); i++)
	{
		PluginServer *ptr = plugin_guis->get(i);
		if(edl->tracks->plugin_exists(ptr->plugin))
			ptr->update_gui();
		else
		{
// Schedule for deletion if no plugin
			plugin_guis->remove_number(i);
			i--;
			
			ptr->hide_gui();
			delete_plugin(ptr);
		}
	}


// Change plugin variable if not visible
	Track *track = edl->tracks->first;
	while(track)
	{
		for(int i = 0; i < track->plugin_set.size(); i++)
		{
			Plugin *plugin = (Plugin*)track->plugin_set.get(i)->first;
			while(plugin)
			{
				int got_it = 0;
				for(int i = 0; i < plugin_guis->size(); i++)
				{
					PluginServer *server = plugin_guis->get(i);
					if(server->plugin == plugin) 
					{
						got_it = 1;
						break;
					}
				}
				
				if(!got_it) plugin->show = 0;

				plugin = (Plugin*)plugin->next;
			}
		}
		
		track = track->next;
	}

	plugin_gui_lock->unlock();


	if(do_keyframe_guis) update_keyframe_guis();
}

int MWindow::plugin_gui_open(Plugin *plugin)
{
	int result = 0;
	plugin_gui_lock->lock("MWindow::plugin_gui_open");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin->identical_location(plugin))
		{
			result = 1;
			break;
		}
	}
	plugin_gui_lock->unlock();
	return result;
}

void MWindow::render_plugin_gui(void *data, Plugin *plugin)
{
	plugin_gui_lock->lock("MWindow::render_plugin_gui");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin->identical_location(plugin))
		{
			plugin_guis->values[i]->render_gui(data);
			break;
		}
	}
	plugin_gui_lock->unlock();
}

void MWindow::render_plugin_gui(void *data, int size, Plugin *plugin)
{
	plugin_gui_lock->lock("MWindow::render_plugin_gui");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		if(plugin_guis->values[i]->plugin->identical_location(plugin))
		{
			plugin_guis->values[i]->render_gui(data, size);
			break;
		}
	}
	plugin_gui_lock->unlock();
}


void MWindow::update_plugin_states()
{
	plugin_gui_lock->lock("MWindow::update_plugin_states");
	for(int i = 0; i < plugin_guis->total; i++)
	{
		int result = 0;
// Get a plugin GUI
		Plugin *src_plugin = plugin_guis->values[i]->plugin;
		PluginServer *src_plugingui = plugin_guis->values[i];

// Search for plugin in EDL.  Only the master EDL shows plugin GUIs.
		for(Track *track = edl->tracks->first; 
			track && !result; 
			track = track->next)
		{
			for(int j = 0; 
				j < track->plugin_set.total && !result; 
				j++)
			{
				PluginSet *plugin_set = track->plugin_set.values[j];
				for(Plugin *plugin = (Plugin*)plugin_set->first; 
					plugin && !result; 
					plugin = (Plugin*)plugin->next)
				{
					if(plugin == src_plugin &&
						!strcmp(plugin->title, src_plugingui->title)) result = 1;
				}
			}
		}


// Doesn't exist anymore
		if(!result)
		{
			hide_plugin(src_plugin, 0);
			i--;
		}
	}
	plugin_gui_lock->unlock();
}


void MWindow::update_plugin_titles()
{
	for(int i = 0; i < plugin_guis->total; i++)
	{
		plugin_guis->values[i]->update_title();
	}
}

int MWindow::asset_to_edl(EDL *new_edl, 
	Asset *new_asset, 
	RecordLabels *labels)
{
const int debug = 0;
if(debug) printf("MWindow::asset_to_edl %d new_asset->layers=%d\n", 
__LINE__,
new_asset->layers);
// Keep frame rate, sample rate, and output size unchanged.
// These parameters would revert the project if VWindow displayed an asset
// of different size than the project.
	if(new_asset->video_data)
	{
		new_edl->session->video_tracks = new_asset->layers;
	}
	else
		new_edl->session->video_tracks = 0;

if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);





	if(new_asset->audio_data)
	{
		new_edl->session->audio_tracks = new_asset->channels;
	}
	else
		new_edl->session->audio_tracks = 0;
//printf("MWindow::asset_to_edl 2 %d %d\n", new_edl->session->video_tracks, new_edl->session->audio_tracks);

if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);
	new_edl->create_default_tracks();
//printf("MWindow::asset_to_edl 2 %d %d\n", new_edl->session->video_tracks, new_edl->session->audio_tracks);
if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);



//printf("MWindow::asset_to_edl 3\n");
	new_edl->insert_asset(new_asset,
		0,
		0, 
		0, 
		labels);
//printf("MWindow::asset_to_edl 3\n");
if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);





	char string[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(string, new_asset->path);
//printf("MWindow::asset_to_edl 3\n");

	strcpy(new_edl->local_session->clip_title, string);
//printf("MWindow::asset_to_edl 4 %s\n", string);
if(debug) printf("MWindow::asset_to_edl %d\n", __LINE__);

	return 0;
}

int MWindow::edl_to_nested(EDL *new_edl, 
	EDL *nested_edl)
{

// Keep frame rate, sample rate, and output size unchanged.
// These parameters would revert the project if VWindow displayed an asset
// of different size than the project.



// Nest all video & audio outputs
	new_edl->session->video_tracks = 1;
	new_edl->session->audio_tracks = nested_edl->session->audio_channels;
	new_edl->create_default_tracks();



	new_edl->insert_asset(0,
		nested_edl,
		0, 
		0, 
		0);

	char string[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(string, nested_edl->path);
//printf("MWindow::edl_to_nested %p %s\n", nested_edl, nested_edl->path);

	strcpy(new_edl->local_session->clip_title, string);

	return 0;
}

// Reset everything after a load.
void MWindow::update_project(int load_mode)
{
	const int debug = 0;
	
	if(debug) PRINT_TRACE
	restart_brender();
	edl->tracks->update_y_pixels(theme);

	if(debug) PRINT_TRACE

	if(load_mode == LOADMODE_REPLACE ||
		load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		gui->load_panes();
	}

	gui->update(1, 1, 1, 1, 1, 1, 1);
	if(debug) PRINT_TRACE
	gui->unlock_window();

	cwindow->gui->lock_window("MWindow::update_project 1");
	cwindow->update(0, 0, 1, 1, 1);
	cwindow->gui->unlock_window();

	if(debug) PRINT_TRACE

// Close all the vwindows
	if(load_mode == LOADMODE_REPLACE ||
		load_mode == LOADMODE_REPLACE_CONCATENATE)
	{
		if(debug) PRINT_TRACE
		int first_vwindow = 0;
		if(session->show_vwindow) first_vwindow = 1;
// Change visible windows to no source
		for(int i = 0; i < first_vwindow && i < vwindows.size(); i++)
		{
			vwindows.get(i)->change_source(0);
		}

// Close remaining windows
		for(int i = first_vwindow; i < vwindows.size(); i++)
		{
			vwindows.get(i)->close_window();
		}
		if(debug) PRINT_TRACE
	}
	else
	if(vwindows.size())
	{
		VWindow *vwindow = vwindows.get(DEFAULT_VWINDOW);
		if(vwindow->is_running())
		{
			vwindow->gui->lock_window("MWindow::update_project");
			vwindow->update(1);
			vwindow->gui->unlock_window();
		}
	}

	if(debug) PRINT_TRACE
	cwindow->gui->lock_window("MWindow::update_project 2");
#ifdef USE_SLIDER
	cwindow->gui->slider->set_position();
#endif
	cwindow->gui->timebar->update(0);
	cwindow->gui->unlock_window();

	if(debug) PRINT_TRACE
	cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
		CHANGE_ALL,
		edl,
		1);

	if(debug) PRINT_TRACE

	awindow->gui->lock_window("MWindow::update_project");
	awindow->gui->update_assets();
	awindow->gui->flush();
	awindow->gui->unlock_window();

	if(debug) PRINT_TRACE

	gui->lock_window("MWindow::update_project");
	gui->flush();
	if(debug) PRINT_TRACE
}


void MWindow::rebuild_indices()
{
	char source_filename[BCTEXTLEN], index_filename[BCTEXTLEN];
	for(int i = 0; i < session->drag_assets->total; i++)
	{
//printf("MWindow::rebuild_indices 1 %s\n", session->drag_assets->values[i]->path);
// Erase file
		IndexFile::get_index_filename(source_filename, 
			preferences->index_directory,
			index_filename, 
			session->drag_assets->values[i]->path);
		remove(index_filename);
// Schedule index build
		IndexState *index_state = session->drag_assets->values[i]->index_state;
		index_state->index_status = INDEX_NOTTESTED;
		mainindexes->add_next_asset(0, session->drag_assets->values[i]);
	}
	mainindexes->start_build();
}


void MWindow::save_backup()
{
	FileXML file;
	edl->set_path(session->filename);
	edl->save_xml(&file, 
		BACKUP_PATH,
		0,
		0);
	file.terminate_string();
	char path[BCTEXTLEN];
	FileSystem fs;
	strcpy(path, BACKUP_PATH);
	fs.complete_path(path);

	if(file.write_to_file(path))
	{
		char string2[256];
		sprintf(string2, _("Couldn't open %s for writing."), BACKUP_PATH);
		gui->show_message(string2);
	}
}


int MWindow::create_aspect_ratio(float &w, float &h, int width, int height)
{
	int denominator;
	if(!width || !height) return 1;
	float fraction = (float)width / height;

	for(denominator = 1; 
		denominator < 100 && 
			fabs(fraction * denominator - (int)(fraction * denominator)) > .001; 
		denominator++)
		;

	w = denominator * width / height;
	h = denominator;
	return 0;
}

void MWindow::reset_caches()
{
	frame_cache->remove_all();
	wave_cache->remove_all();
	audio_cache->remove_all();
	video_cache->remove_all();
	if(cwindow->playback_engine && cwindow->playback_engine->audio_cache)
		cwindow->playback_engine->audio_cache->remove_all();
	if(cwindow->playback_engine && cwindow->playback_engine->video_cache)
		cwindow->playback_engine->video_cache->remove_all();
	
	for(int i = 0; i < vwindows.size(); i++)
	{
		VWindow *vwindow = vwindows.get(i);
		if(vwindow->is_running())
		{
			if(vwindow->playback_engine && vwindow->playback_engine->audio_cache)
				vwindow->playback_engine->audio_cache->remove_all();
			if(vwindow->playback_engine && vwindow->playback_engine->video_cache)
				vwindow->playback_engine->video_cache->remove_all();
		}
	}
}

void MWindow::remove_asset_from_caches(Asset *asset)
{
	frame_cache->remove_asset(asset);
	wave_cache->remove_asset(asset);
	audio_cache->delete_entry(asset);
	video_cache->delete_entry(asset);
	if(cwindow->playback_engine && cwindow->playback_engine->audio_cache)
		cwindow->playback_engine->audio_cache->delete_entry(asset);
	if(cwindow->playback_engine && cwindow->playback_engine->video_cache)
		cwindow->playback_engine->video_cache->delete_entry(asset);
	for(int i = 0; i < vwindows.size(); i++)
	{
		VWindow *vwindow = vwindows.get(i);
		if(vwindow->is_running())
		{
			if(vwindow->playback_engine && vwindow->playback_engine->audio_cache)
				vwindow->playback_engine->audio_cache->delete_entry(asset);
			if(vwindow->playback_engine && vwindow->playback_engine->video_cache)
				vwindow->playback_engine->video_cache->delete_entry(asset);
		}
	}
}



void MWindow::remove_assets_from_project(int push_undo /* = 0 */, 
		int redraw /* 1 */,
		ArrayList<Indexable*> *drag_assets /* mwindow->session->drag_assets */,
		ArrayList<EDL*> *drag_clips /* mwindow->session->drag_clips */)
{
	if(drag_assets)
	{
		for(int i = 0; i < drag_assets->size(); i++)
		{
			Indexable *indexable = drag_assets->get(i);
			if(indexable->is_asset) remove_asset_from_caches((Asset*)indexable);
		}
	}

// Remove from VWindow.
	if(drag_clips)
	{
		for(int i = 0; i < drag_clips->size(); i++)
		{
			for(int j = 0; j < vwindows.size(); j++)
			{
				VWindow *vwindow = vwindows.get(j);
				if(vwindow->is_running())
				{
					if(drag_clips->get(i) == vwindow->get_edl())
					{
						vwindow->gui->lock_window("MWindow::remove_assets_from_project 1");
						vwindow->delete_source(1, 1);
						vwindow->gui->unlock_window();
					}
				}
			}
		}
	}
	
	if(drag_assets)
	{
		for(int i = 0; i < drag_assets->size(); i++)
		{
			for(int j = 0; j < vwindows.size(); j++)
			{
				VWindow *vwindow = vwindows.get(j);
				if(vwindow->is_running())
				{
					if(drag_assets->get(i) == vwindow->get_source())
					{
						vwindow->gui->lock_window("MWindow::remove_assets_from_project 2");
						vwindow->delete_source(1, 1);
						vwindow->gui->unlock_window();
					}
				}
			}
		}
	}
	
	if(push_undo) undo->update_undo_before();
	if(drag_assets) edl->remove_from_project(drag_assets);
	if(drag_clips) edl->remove_from_project(session->drag_clips);
	if(redraw) save_backup();
	if(push_undo) undo->update_undo_after(_("remove assets"), LOAD_ALL);
	if(redraw) 
	{
		restart_brender();

		gui->lock_window("MWindow::remove_assets_from_project 3");
		gui->update(1,
			1,
			1,
			1,
			0, 
			1,
			0);
		gui->unlock_window();

		awindow->gui->lock_window("MWindow::remove_assets_from_project 4");
		awindow->gui->update_assets();
		awindow->gui->flush();
		awindow->gui->unlock_window();

	// Removes from playback here
		sync_parameters(CHANGE_ALL);
	}
}

void MWindow::remove_assets_from_disk()
{
// Remove from disk
	for(int i = 0; i < session->drag_assets->total; i++)
	{
		remove(session->drag_assets->values[i]->path);
	}

	remove_assets_from_project(1, 
		1, 
		session->drag_assets,
		session->drag_clips);
}

void MWindow::dump_plugins()
{
	for(int i = 0; i < plugindb->total; i++)
	{
		printf("audio=%d video=%d rt=%d multi=%d synth=%d transition=%d theme=%d %s\n",
			plugindb->values[i]->audio,
			plugindb->values[i]->video,
			plugindb->values[i]->realtime,
			plugindb->values[i]->multichannel,
			plugindb->values[i]->get_synthesis(),
			plugindb->values[i]->transition,
			plugindb->values[i]->theme,
			plugindb->values[i]->title);
	}
}

























int MWindow::save_defaults()
{
	gui->save_defaults(defaults);
	edl->save_defaults(defaults);
	session->save_defaults(defaults);
	preferences->save_defaults(defaults);

	for(int i = 0; i < plugin_guis->total; i++)
	{
// Pointer comparison
		plugin_guis->values[i]->save_defaults();
	}

	defaults->save();
	return 0;
}

int MWindow::run_script(FileXML *script)
{
	int result = 0, result2 = 0;
	while(!result && !result2)
	{
		result = script->read_tag();
		if(!result)
		{
			if(script->tag.title_is("new_project"))
			{
// Run new in immediate mode.
//				gui->mainmenu->new_project->run_script(script);
			}
			else
			if(script->tag.title_is("record"))
			{
// Run record as a thread.  It is a terminal command.
				;
// Will read the complete scipt file without letting record read it if not
// terminated.
				result2 = 1;
			}
			else
			{
				printf("MWindow::run_script: Unrecognized command: %s\n",script->tag.get_title() );
			}
		}
	}
	return result2;
}

// ================================= synchronization


int MWindow::interrupt_indexes()
{
	mainindexes->interrupt_build();
	return 0; 
}



void MWindow::next_time_format()
{
	switch(edl->session->time_format)
	{
		case TIME_HMS: edl->session->time_format = TIME_HMSF; break;
		case TIME_HMSF: edl->session->time_format = TIME_SAMPLES; break;
		case TIME_SAMPLES: edl->session->time_format = TIME_SAMPLES_HEX; break;
		case TIME_SAMPLES_HEX: edl->session->time_format = TIME_FRAMES; break;
		case TIME_FRAMES: edl->session->time_format = TIME_FEET_FRAMES; break;
		case TIME_FEET_FRAMES: edl->session->time_format = TIME_SECONDS; break;
		case TIME_SECONDS: edl->session->time_format = TIME_HMS; break;
	}

	time_format_common();
}

void MWindow::prev_time_format()
{
	switch(edl->session->time_format)
	{
		case TIME_HMS: edl->session->time_format = TIME_SECONDS; break;
		case TIME_SECONDS: edl->session->time_format = TIME_FEET_FRAMES; break;
		case TIME_FEET_FRAMES: edl->session->time_format = TIME_FRAMES; break;
		case TIME_FRAMES: edl->session->time_format = TIME_SAMPLES_HEX; break;
		case TIME_SAMPLES_HEX: edl->session->time_format = TIME_SAMPLES; break;
		case TIME_SAMPLES: edl->session->time_format = TIME_HMSF; break;
		case TIME_HMSF: edl->session->time_format = TIME_HMS; break;
	}

	time_format_common();
}

void MWindow::time_format_common()
{
	gui->lock_window("MWindow::next_time_format");
	gui->redraw_time_dependancies();


	char string[BCTEXTLEN], string2[BCTEXTLEN];
	sprintf(string, _("Using %s"), Units::print_time_format(edl->session->time_format, string2));
	gui->show_message(string);
	gui->flush();
	gui->unlock_window();
}


int MWindow::set_filename(const char *filename)
{
	strcpy(session->filename, filename);
	strcpy(edl->path, filename);


	if(gui)
	{
		if(filename[0] == 0)
		{
			gui->set_title(PROGRAM_NAME);
		}
		else
		{
			FileSystem dir;
			char string[BCTEXTLEN], string2[BCTEXTLEN];
			dir.extract_name(string, filename);
			sprintf(string2, PROGRAM_NAME ": %s", string);
			gui->set_title(string2);
		}
	}
	return 0; 
}








int MWindow::set_loop_boundaries()
{
	double start = edl->local_session->get_selectionstart();
	double end = edl->local_session->get_selectionend();
	
	if(start != 
		end) 
	{
		;
	}
	else
	if(edl->tracks->total_length())
	{
		start = 0;
		end = edl->tracks->total_length();
	}
	else
	{
		start = end = 0;
	}

	if(edl->local_session->loop_playback && start != end)
	{
		edl->local_session->loop_start = start;
		edl->local_session->loop_end = end;
	}
	return 0; 
}







int MWindow::reset_meters()
{
	cwindow->gui->lock_window("MWindow::reset_meters 1");
	cwindow->gui->meters->reset_meters();
	cwindow->gui->unlock_window();

	for(int j = 0; j < vwindows.size(); j++)
	{
		VWindow *vwindow = vwindows.get(j);
		if(vwindow->is_running())
		{
			vwindow->gui->lock_window("MWindow::reset_meters 2");
			vwindow->gui->meters->reset_meters();
			vwindow->gui->unlock_window();
		}
	}

	lwindow->gui->lock_window("MWindow::reset_meters 3");
	lwindow->gui->panel->reset_meters();
	lwindow->gui->unlock_window();

	gui->lock_window("MWindow::reset_meters 4");
	gui->reset_meters();
	gui->unlock_window();
	return 0; 
}

