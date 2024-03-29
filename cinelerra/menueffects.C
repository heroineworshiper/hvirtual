/*
 * CINELERRA
 * Copyright (C) 2010-2017 Adam Williams <broadcast at earthling dot net>
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
#include "awindow.h"
#include "awindowgui.h"
#include "clip.h"
#include "confirmsave.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "formatcheck.h"
#include "indexfile.h"
#include "keyframe.h"
#include "keys.h"
#include "labels.h"
#include "language.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "menueffects.h"
#include "playbackengine.h"
#include "pluginarray.h"
#include "pluginserver.h"
#include "preferences.h"
#include "render.h"
#include "sighandler.h"
#include "theme.h"
#include "tracks.h"



MenuEffects::MenuEffects(MWindow *mwindow)
 : BC_MenuItem(_("Render effect..."))
{
	this->mwindow = mwindow;
}

MenuEffects::~MenuEffects()
{
}


int MenuEffects::handle_event()
{
	thread->set_title("");
	thread->start();
    return 0;
}





MenuEffectPacket::MenuEffectPacket(char *path, int64_t start, int64_t end)
{
	this->start = start;
	this->end = end;
	strcpy(this->path, path);
}

MenuEffectPacket::~MenuEffectPacket()
{
}






MenuEffectThread::MenuEffectThread(MWindow *mwindow, MenuEffects *menu_item)
{
	this->mwindow = mwindow;
	this->menu_item = menu_item;
	dead_plugins = new ArrayList<PluginServer*>;
	title[0] = 0;
}

MenuEffectThread::~MenuEffectThread()
{
	delete dead_plugins;
}





int MenuEffectThread::set_title(const char *title)
{
	strcpy(this->title, title);
    return 0;
}

// for recent effect menu items and running new effects
// prompts for an effect if title is blank
void MenuEffectThread::run()
{
	for(int i = 0; i < dead_plugins->size(); i++)
	{
		delete dead_plugins->get(i);
	}
	dead_plugins->remove_all();




// get stuff from main window
	ArrayList<PluginServer*> *plugindb = mwindow->plugindb;
	BC_Hash *defaults = mwindow->defaults;
	ArrayList<BC_ListBoxItem*> plugin_list;
	ArrayList<PluginServer*> local_plugindb;
	char string[1024];
	int i;
	int result = 0;
// Default configuration
	Asset *default_asset = new Asset;
// Output assets
	ArrayList<Indexable*> assets;


// check for recordable tracks
	if(!get_recordable_tracks(default_asset))
	{
		sprintf(string, _("No recordable tracks specified."));
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(string);
		error.run_window();
		default_asset->Garbage::remove_user();
		return;
	}

// check for plugins
	if(!plugindb->total)
	{
		sprintf(string, _("No plugins available."));
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(string);
		error.run_window();
		default_asset->Garbage::remove_user();
		return;
	}


// get default attributes for output file
// used after completion
	get_derived_attributes(default_asset, defaults);
//	to_tracks = defaults->get("RENDER_EFFECT_TO_TRACKS", 1);
	load_mode = defaults->get("RENDER_EFFECT_LOADMODE", LOADMODE_PASTE);
	strategy = defaults->get("RENDER_EFFECT_STRATEGY", SINGLE_PASS);

// get plugin information
	int need_plugin;
	if(!strlen(title)) 
		need_plugin = 1; 
	else 
		need_plugin = 0;

// generate a list of plugins for the window
	if(need_plugin)
	{
		mwindow->search_plugindb(default_asset->audio_data, 
			default_asset->video_data, 
			-1, 
			0,
			0,
			local_plugindb);

		for(int i = 0; i < local_plugindb.total; i++)
		{
			plugin_list.append(new BC_ListBoxItem(_(local_plugindb.values[i]->title)));
		}
	}

// find out which effect to run and get output file
	int plugin_number, format_error = 0;

	do
	{
		{
			MenuEffectWindow window(mwindow, 
				this, 
				need_plugin ? &plugin_list : 0, 
				default_asset);
			window.create_objects();
			result = window.run_window();
			plugin_number = window.result;
		}

		if(!result)
		{
			FormatCheck format_check(default_asset);
			format_error = format_check.check_format();
		}
	}while(format_error && !result);

// save defaults
	save_derived_attributes(default_asset, defaults);
	defaults->update("RENDER_EFFECT_LOADMODE", load_mode);
	defaults->update("RENDER_EFFECT_STRATEGY", strategy);
	mwindow->save_defaults();

// get plugin server to use and delete the plugin list
	PluginServer *plugin_server = 0;
	PluginServer *plugin = 0;
	if(need_plugin)
	{
		plugin_list.remove_all_objects();
		if(plugin_number > -1)
		{
			plugin_server = local_plugindb.values[plugin_number];
			strcpy(title, plugin_server->title);
		}
	}
	else
	{
		for(int i = 0; i < plugindb->total && !plugin_server; i++)
		{
			if(!strcmp(plugindb->values[i]->title, title))
			{
				plugin_server = plugindb->values[i];
				plugin_number = i;
			}
		}
	}

// Update the  most recently used effects and copy the plugin server.
	if(plugin_server)
	{
		plugin = new PluginServer(*plugin_server);
		fix_menu(title);
	}

	if(!result && !strlen(default_asset->path))
	{
		result = 1;        // no output path given
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(_("No output file specified."));
		error.run_window();
	}

	if(!result && plugin_number < 0)
	{
		result = 1;        // no output path given
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(_("No effect selected."));
		error.run_window();
	}

// Configuration for realtime plugins.
	KeyFrame plugin_data;        

// get selection to render
// Range
	double total_start, total_end;

	total_start = mwindow->edl->local_session->get_selectionstart();


	if(mwindow->edl->local_session->get_selectionend() == 
		mwindow->edl->local_session->get_selectionstart())
		total_end = mwindow->edl->tracks->total_playable_length();
	else
		total_end = mwindow->edl->local_session->get_selectionend();



// get native units for range
	total_start = to_units(total_start, 0);
	total_end = to_units(total_end, 1);



// Trick boundaries in case of a non-realtime synthesis plugin
	if(plugin && 
		!plugin->realtime && 
		total_end == total_start) total_end = total_start + 1;

// Units are now in the track's units.
	int64_t total_length = (int64_t)total_end - (int64_t)total_start;
// length of output file
	int64_t output_start, output_end;        

	if(!result && total_length <= 0)
	{
		result = 1;        // no output path given
		ErrorBox error(PROGRAM_NAME ": Error");
		error.create_objects(_("No selected range to process."));
		error.run_window();
	}

// ========================= get keyframe from user
	if(!result)
	{
// ========================= realtime plugin 
// no get_parameters
		if(plugin->realtime)
		{
// Open a prompt GUI
			MenuEffectPrompt prompt(mwindow, plugin);
			prompt.create_objects();
			char title[BCTEXTLEN];
			sprintf(title, PROGRAM_NAME ": %s", plugin->title);

// Open the plugin GUI
			plugin->set_mwindow(mwindow);
			plugin->set_keyframe(&plugin_data);
			plugin->set_prompt(&prompt);
			plugin->open_plugin(0, mwindow->preferences, mwindow->edl, 0, -1);
// Must set parameters since there is no plugin object to draw from.
			plugin->get_parameters((int64_t)total_start,
				(int64_t)total_end,
				1);
			plugin->show_gui();

// wait for user input
			result = prompt.run_window();

// Close plugin.
			plugin->save_data(&plugin_data);
			plugin->hide_gui();
			mwindow->hide_keyframe_gui(plugin);

// Can't delete here.
			dead_plugins->append(plugin);
			default_asset->sample_rate = mwindow->edl->session->sample_rate;
			default_asset->frame_rate = mwindow->edl->session->frame_rate;
			realtime = 1;
		}
		else
// ============================non realtime plugin 
		{
			plugin->set_mwindow(mwindow);
			plugin->open_plugin(0, mwindow->preferences, mwindow->edl, 0, -1);
			result = plugin->get_parameters((int64_t)total_start, 
				(int64_t)total_end, 
				get_recordable_tracks(default_asset));
// some plugins can change the sample rate and the frame rate


			if(!result)
			{
				default_asset->sample_rate = plugin->get_samplerate();
				default_asset->frame_rate = plugin->get_framerate();
			}
			delete plugin;
			realtime = 0;
		}

// Should take from first recordable track
		default_asset->width = mwindow->edl->session->output_w;
		default_asset->height = mwindow->edl->session->output_h;
	}

// Process the total length in fragments
	ArrayList<MenuEffectPacket*> packets;
	if(!result)
	{
		Label *current_label = mwindow->edl->labels->first;
		mwindow->stop_brender();

		int current_number;
		int number_start;
		int total_digits;
		Render::get_starting_number(default_asset->path, 
			current_number,
			number_start, 
			total_digits);



// Construct all packets for single overwrite confirmation
		for(int64_t fragment_start = (int64_t)total_start, fragment_end;
			fragment_start < (int64_t)total_end;
			fragment_start = fragment_end)
		{
// Get fragment end
			if(strategy == FILE_PER_LABEL || strategy == FILE_PER_LABEL_FARM)
			{
				while(current_label  &&
					to_units(current_label->position, 0) <= fragment_start)
					current_label = current_label->next;
				if(!current_label)
					fragment_end = (int64_t)total_end;
				else
					fragment_end = to_units(current_label->position, 0);
			}
			else
			{
				fragment_end = (int64_t)total_end;
			}

// Get path
			char path[BCTEXTLEN];
			if(strategy == FILE_PER_LABEL || strategy == FILE_PER_LABEL_FARM) 
			{
            	Render::create_filename(path, 
					default_asset->path, 
					current_number,
					total_digits,
					number_start);
			}
            else
			{
            	strcpy(path, default_asset->path);
			}
            current_number++;

			MenuEffectPacket *packet = new MenuEffectPacket(path, 
				fragment_start,
				fragment_end);
			packets.append(packet);
		}


// Test existence of files
		ArrayList<char*> paths;
		for(int i = 0; i < packets.total; i++)
		{
			paths.append(packets.values[i]->path);
		}
		result = ConfirmSave::test_files(mwindow, &paths);
		paths.remove_all();
	}



	for(int current_packet = 0; 
		current_packet < packets.total && !result; 
		current_packet++)
	{
		Asset *asset = new Asset(*default_asset);
		MenuEffectPacket *packet = packets.values[current_packet];
		int64_t fragment_start = packet->start;
		int64_t fragment_end = packet->end;
		strcpy(asset->path, packet->path);

		assets.append(asset);
		File *file = new File;

// Open the output file after getting the information because the sample rate
// is needed here.
		if(!result)
		{
// open output file in write mode
			file->set_processors(mwindow->preferences->processors);
			file->set_cache(mwindow->preferences->cache_size);
			if(file->open_file(mwindow->preferences, 
				asset, 
				0, 
				1))
			{
// open failed
				sprintf(string, _("Couldn't open %s"), asset->path);
				ErrorBox error(PROGRAM_NAME ": Error");
				error.create_objects(string);
				error.run_window();
				result = 1;
			}
			else
			{
				mwindow->sighandler->push_file(file);
				IndexFile::delete_index(mwindow->preferences, 
					asset);
			}
		}

// run plugins
		if(!result)
		{
// position file
			output_start = 0;

			PluginArray *plugin_array;
			plugin_array = create_plugin_array();

			plugin_array->start_plugins(mwindow, 
				mwindow->edl, 
				plugin_server, 
				&plugin_data,
				fragment_start,
				fragment_end,
				file);
			plugin_array->run_plugins();

			plugin_array->stop_plugins();
			mwindow->sighandler->pull_file(file);
			file->close_file();
			asset->audio_length = file->asset->audio_length;
			asset->video_length = file->asset->video_length;
			delete plugin_array;
		}

		delete file;
	}

	packets.remove_all_objects();

// paste output to tracks
	if(!result && load_mode != LOADMODE_NOTHING)
	{
		mwindow->gui->lock_window("MenuEffectThread::run");

		mwindow->undo->update_undo_before("", 0);
		if(load_mode == LOADMODE_PASTE)
			mwindow->clear(0, 1);

// force the format to be probed in case the encoder was different than the decoder
        for(int i = 0; i < assets.size(); i++)
        {
            Asset *asset = (Asset*)assets.get(i);
            asset->format = FILE_UNKNOWN;
        }

		mwindow->load_assets(&assets,
			-1,
			load_mode,
			0,
			0,
			mwindow->edl->session->labels_follow_edits, 
			mwindow->edl->session->plugins_follow_edits,
			mwindow->edl->session->autos_follow_edits);


		mwindow->save_backup();
		mwindow->undo->update_undo_after(title, LOAD_ALL);



		mwindow->restart_brender();
		mwindow->update_plugin_guis();
		mwindow->gui->update(1, 
			2,
			1,
			1,
			1,
			1,
			0);
		mwindow->sync_parameters(CHANGE_ALL);
		mwindow->gui->unlock_window();


		mwindow->awindow->gui->lock_window("MenuEffectThread::run");
		mwindow->awindow->gui->update_assets();
		mwindow->awindow->gui->flush();
		mwindow->awindow->gui->unlock_window();


	}

	for(int i = 0; i < assets.total; i++)
		assets.values[i]->Garbage::remove_user();
	assets.remove_all();

	default_asset->Garbage::remove_user();
}







MenuEffectItem::MenuEffectItem(MenuEffects *menueffect, char *string)
 : BC_MenuItem(string)
{
	this->menueffect = menueffect; 
}
int MenuEffectItem::handle_event()
{
	menueffect->thread->set_title(get_text());
	menueffect->thread->start();
    return 0;
}












MenuEffectWindow::MenuEffectWindow(MWindow *mwindow, 
	MenuEffectThread *menueffects, 
	ArrayList<BC_ListBoxItem*> *plugin_list, 
	Asset *asset)
 : BC_Window(PROGRAM_NAME ": Render effect", 
 		mwindow->gui->get_abs_cursor_x(1),
		mwindow->gui->get_abs_cursor_y(1) - mwindow->session->menueffect_h / 2,
		mwindow->session->menueffect_w, 
		mwindow->session->menueffect_h, 
		DP(580),
		DP(350),
		1,
		0,
		1)
{ 
	this->menueffects = menueffects; 
	this->plugin_list = plugin_list; 
	this->asset = asset;
	this->mwindow = mwindow;
	file_title = 0;
	format_tools = 0;
	loadmode = 0;
}

MenuEffectWindow::~MenuEffectWindow()
{
	lock_window("MenuEffectWindow::~MenuEffectWindow");
	delete format_tools;
	unlock_window();
}



void MenuEffectWindow::create_objects()
{
	int x, y;
	int margin = mwindow->theme->widget_border;
	result = -1;
	mwindow->session->menueffect_w = MAX(get_w(), DP(580));
	mwindow->session->menueffect_h = MAX(get_h(), DP(350));
	mwindow->theme->get_menueffect_sizes(plugin_list ? 1 : 0);

	lock_window("MenuEffectWindow::create_objects");
// only add the list if needed
	if(plugin_list)
	{
		add_subwindow(list_title = new BC_Title(mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y, 
			_("Select an effect")));
		add_subwindow(list = new MenuEffectWindowList(this, 
			mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y + list_title->get_h() + margin, 
			mwindow->theme->menueffect_list_w,
			mwindow->theme->menueffect_list_h - list_title->get_h() - margin,
			plugin_list));
	}

	add_subwindow(file_title = new BC_Title(
		mwindow->theme->menueffect_file_x, 
		mwindow->theme->menueffect_file_y, 
		(char*)((menueffects->strategy == FILE_PER_LABEL  || menueffects->strategy == FILE_PER_LABEL_FARM) ? 
			_("Select the first file to render to:") : 
			_("Select a file to render to:"))));

	x = mwindow->theme->menueffect_tools_x;
	y = mwindow->theme->menueffect_tools_y;
	format_tools = new FormatTools(mwindow,
					this, 
					asset);
	format_tools->set_w(get_w() - x);
	format_tools->create_objects(x, 
					y, 
					asset->audio_data, // do_audio
					asset->video_data, // do_video
					0, // prompt_audio
					0, // prompt_video
					1, // prompt_video_compression
                    1, // prompt_wrapper
					0, // locked_compressor
					0, // recording
					&menueffects->strategy,
					0);

	loadmode = new LoadMode(mwindow, 
		this, 
		x, 
		y, 
		&menueffects->load_mode, 
		1,
		0);
	loadmode->create_objects();

	add_subwindow(new MenuEffectWindowOK(this));
	add_subwindow(new MenuEffectWindowCancel(this));
	show_window();
	unlock_window();
}

int MenuEffectWindow::resize_event(int w, int h)
{
	int margin = mwindow->theme->widget_border;
	mwindow->session->menueffect_w = w;
	mwindow->session->menueffect_h = h;
	mwindow->theme->get_menueffect_sizes(plugin_list ? 1 : 0);

	if(plugin_list)
	{
		list_title->reposition_window(mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y);
		list->reposition_window(mwindow->theme->menueffect_list_x, 
			mwindow->theme->menueffect_list_y + list_title->get_h() + margin, 
			mwindow->theme->menueffect_list_w,
			mwindow->theme->menueffect_list_h - list_title->get_h() - margin);
	}

	if(file_title) file_title->reposition_window(mwindow->theme->menueffect_file_x, 
		mwindow->theme->menueffect_file_y);
	int x = mwindow->theme->menueffect_tools_x;
	int y = mwindow->theme->menueffect_tools_y;
	if(format_tools) 
	{
		format_tools->set_w(w - x);
		format_tools->reposition_window(x, y);
	}
	if(loadmode) loadmode->reposition_window(x, y);
    return 0;
}



MenuEffectWindowOK::MenuEffectWindowOK(MenuEffectWindow *window)
 : BC_OKButton(window)
{ 
	this->window = window; 
}

int MenuEffectWindowOK::handle_event() 
{ 
	if(window->plugin_list) 
		window->result = window->list->get_selection_number(0, 0); 
	
	window->set_done(0); 
    return 0;
}

int MenuEffectWindowOK::keypress_event() 
{ 
	if(get_keypress() == RETURN) 
	{ 
		handle_event(); 
		return 1; 
	}
	return 0;
}

MenuEffectWindowCancel::MenuEffectWindowCancel(MenuEffectWindow *window)
 : BC_CancelButton(window)
{ 
	this->window = window; 
}

int MenuEffectWindowCancel::handle_event() 
{ 
	window->set_done(1); 
    return 0;
}

int MenuEffectWindowCancel::keypress_event() 
{ 
	if(get_keypress() == ESC) 
	{ 
		handle_event(); 
		return 1; 
	}
	return 0;
}

MenuEffectWindowList::MenuEffectWindowList(MenuEffectWindow *window, 
	int x, 
	int y, 
	int w, 
	int h, 
	ArrayList<BC_ListBoxItem*> *plugin_list)
 : BC_ListBox(x, 
 		y, 
		w, 
		h, 
		LISTBOX_TEXT, 
		plugin_list)
{ 
	this->window = window; 
}

int MenuEffectWindowList::handle_event() 
{
	window->result = get_selection_number(0, 0);
	window->set_done(0); 
    return 0;
}

#define PROMPT_TEXT _("Set up effect panel and hit \"OK\"")

MenuEffectPrompt::MenuEffectPrompt(MWindow *mwindow, PluginServer *plugin_server)
 : BC_Window(PROGRAM_NAME ": Effect Prompt", 
 		mwindow->gui->get_abs_cursor_x(1) - DP(260) / 2,
		mwindow->gui->get_abs_cursor_y(1) - DP(300),
 		MenuEffectPrompt::calculate_w(mwindow->gui), 
		MenuEffectPrompt::calculate_h(mwindow), 
		MenuEffectPrompt::calculate_w(mwindow->gui),
		MenuEffectPrompt::calculate_h(mwindow),
		0,
		0,
		1)
{
	this->mwindow = mwindow;
	this->plugin_server = plugin_server;
}

int MenuEffectPrompt::calculate_w(BC_WindowBase *gui)
{
	int w = BC_Title::calculate_w(gui, PROMPT_TEXT) + DP(10);
	w = MAX(w, BC_OKButton::calculate_w() + BC_CancelButton::calculate_w() + DP(30));
	return w;
}

int MenuEffectPrompt::calculate_h(MWindow *mwindow)
{
	int margin = mwindow->theme->widget_border;
	int h = BC_Title::calculate_h(mwindow->gui, PROMPT_TEXT) + margin;
	h += BC_OKButton::calculate_h() + margin;
	h += BC_GenericButton::calculate_h() + margin;
	return h;
}


void MenuEffectPrompt::create_objects()
{
	int x = mwindow->theme->widget_border;
	int y = mwindow->theme->widget_border;
	int margin = mwindow->theme->widget_border;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x, y, PROMPT_TEXT));
	y += title->get_h() + margin;
	
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	MenuEffectPresets *button;
	add_subwindow(button = new MenuEffectPresets(mwindow, this, x, y));
	y += button->get_h() + margin;
	
	
	show_window();
	raise_window();
}







MenuEffectPresets::MenuEffectPresets(MWindow *mwindow, MenuEffectPrompt *gui, int x, int y)
 : BC_GenericButton(x, 
 	y,
	_("Presets..."))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int MenuEffectPresets::handle_event()
{
	mwindow->show_keyframe_gui(0, gui->plugin_server);
	return 1;
}





