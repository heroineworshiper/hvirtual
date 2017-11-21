
/*
 * CINELERRA
 * Copyright (C) 2017 Adam Williams <broadcast at earthling dot net>
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

#include "bchash.h"
#include "bcsignals.h"
#include "edl.h"
#include "filesystem.h"
#include "keyframe.h"
#include "keyframes.h"
#include "keyframegui.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "preferences.h"
#include "presets.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"










KeyFrameThread::KeyFrameThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	plugin = 0;
	keyframe = 0;
	keyframe_data = new ArrayList<BC_ListBoxItem*>[KEYFRAME_COLUMNS];
	plugin_title[0] = 0;
	is_factory = 0;
	preset_text[0] = 0;
	window_title[0] = 0;
	column_titles[0] = (char*)"Parameter";
	column_titles[1] = (char*)"Value";
	column_width[0] = 0;
	column_width[1] = 0;
	presets_data = new ArrayList<BC_ListBoxItem*>;
	presets_db = new PresetsDB;
}

KeyFrameThread::~KeyFrameThread()
{
	for(int i = 0; i < KEYFRAME_COLUMNS; i++)
		keyframe_data[i].remove_all_objects();
	delete [] keyframe_data;
	presets_data->remove_all_objects();
	delete presets_data;
	is_factories.remove_all();
	preset_titles.remove_all_objects();
}




void KeyFrameThread::start_window(Plugin *plugin, 
	PluginServer *plugin_server, 
	KeyFrame *keyframe)
{

	if(!BC_DialogThread::is_running())
	{
		if(plugin && !mwindow->edl->tracks->plugin_exists(plugin)) return;

		this->keyframe = keyframe;
		this->plugin = plugin;
		this->plugin_server = plugin_server;
		if(plugin)
		{
			plugin->calculate_title(plugin_title, 0);
		}
		else
		{
			strcpy(plugin_title, plugin_server->title);
		}
		sprintf(window_title, PROGRAM_NAME ": %s Keyframe", plugin_title);



// Load all the presets from disk
		char path[BCTEXTLEN];


// system wide presets
		sprintf(path, "%s/%s", mwindow->preferences->plugin_dir, FACTORY_FILE);

		FileSystem fs;
		fs.complete_path(path);
		presets_db->load_from_file(path, 1, 1);



// user presets
		sprintf(path, "%s%s", BCASTDIR, PRESETS_FILE);
		fs.complete_path(path);



		presets_db->load_from_file(path, 0, 0);
		calculate_preset_list();

		if(plugin)
		{
			mwindow->gui->unlock_window();
		}
		
		BC_DialogThread::start();
		
		if(plugin)
		{
			mwindow->gui->lock_window("KeyFrameThread::start_window");
		}
	}
	else
	{
		BC_DialogThread::start();
	}
}

BC_Window* KeyFrameThread::new_gui()
{
	mwindow->gui->lock_window("KeyFrameThread::new_gui");
	
	int x = mwindow->gui->get_abs_cursor_x(0) - 
		mwindow->session->plugindialog_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) - 
		mwindow->session->plugindialog_h / 2;

	KeyFrameWindow *window = new KeyFrameWindow(mwindow, 
		this, 
		x, 
		y,
		window_title);

	window->create_objects();
	
	
	mwindow->gui->unlock_window();

	return window;
}

void KeyFrameThread::handle_done_event(int result)
{
// Apply the preset
	if(!result)
	{
		apply_preset(preset_text, is_factory);
	}
}

void KeyFrameThread::handle_close_event(int result)
{
	plugin = 0;
	plugin_server = 0;
	keyframe = 0;
}

void KeyFrameThread::close_window()
{
	lock_window("KeyFrameThread::close_window");
	if(get_gui())
	{
		get_gui()->lock_window("KeyFrameThread::close_window");
		get_gui()->set_done(1);
		get_gui()->unlock_window();
	}
	unlock_window();
}



void KeyFrameThread::calculate_preset_list()
{
	presets_data->remove_all_objects();
	is_factories.remove_all();
	preset_titles.remove_all_objects();
	int total_presets = presets_db->get_total_presets(plugin_title, 0);

// sort the list
	presets_db->sort(plugin_title);
	
	for(int i = 0; i < total_presets; i++)
	{
		char text[BCTEXTLEN];
		char *orig_title = presets_db->get_preset_title(
			plugin_title,
			i);
		int is_factory = presets_db->get_is_factory(plugin_title,
			i);
		sprintf(text, "%s%s", is_factory ? "*" : "", orig_title);
		presets_data->append(new BC_ListBoxItem(text));
		
		
		preset_titles.append(strdup(orig_title));
		is_factories.append(is_factory);
	}
}


void KeyFrameThread::save_preset(const char *title, int is_factory)
{
	get_gui()->unlock_window();
	
	
	if(plugin)
	{
		mwindow->gui->lock_window("KeyFrameThread::save_preset");
	
// Test EDL for plugin existence
		if(!mwindow->edl->tracks->plugin_exists(plugin))
		{
			mwindow->gui->unlock_window();
			get_gui()->lock_window("KeyFrameThread::save_preset 2");
			return;
		}


// Get current plugin keyframe
		EDL *edl = mwindow->edl;
		Track *track = plugin->track;
		keyframe = plugin->get_prev_keyframe(
			track->to_units(edl->local_session->get_selectionstart(1), 0), 
			PLAY_FORWARD);
//printf("KeyFrameThread::save_preset %d %p %p\n", __LINE__, keyframe, keyframe->get_data());
	}
	else
	{
		keyframe = plugin_server->keyframe;
//printf("KeyFrameThread::save_preset %d\n", __LINE__);
	}

//printf("KeyFrameThread::save_preset %d %p %p\n", __LINE__, keyframe, keyframe->get_data());
//plugin->dump();


// Send to database
	presets_db->save_preset(plugin_title, 
		title, 
		keyframe->get_data());

	if(plugin)
	{
		mwindow->gui->unlock_window();
	}
	
	get_gui()->lock_window("KeyFrameThread::save_preset 2");


// Update list
	calculate_preset_list();
	((KeyFrameWindow*)get_gui())->preset_list->update(presets_data,
		0,
		0,
		1);
}

void KeyFrameThread::delete_preset(const char *title, int is_factory)
{
	get_gui()->unlock_window();
	
	if(plugin)
	{
		mwindow->gui->lock_window("KeyFrameThread::save_preset");
	
// Test EDL for plugin existence
		if(!mwindow->edl->tracks->plugin_exists(plugin))
		{
			mwindow->gui->unlock_window();
			get_gui()->lock_window("KeyFrameThread::delete_preset 1");
			return;
		}
	}

	presets_db->delete_preset(plugin_title, title, is_factory);
	
	if(plugin)
	{
		mwindow->gui->unlock_window();
	}
	
	get_gui()->lock_window("KeyFrameThread::delete_preset 2");


// Update list
	calculate_preset_list();
	((KeyFrameWindow*)get_gui())->preset_list->update(presets_data,
		0,
		0,
		1);
}


void KeyFrameThread::apply_preset(const char *title, int is_factory)
{
	if(presets_db->preset_exists(plugin_title, title, is_factory))
	{
		get_gui()->unlock_window();


		if(plugin)
		{

			mwindow->gui->lock_window("KeyFrameThread::apply_preset");

	// Test EDL for plugin existence
			if(!mwindow->edl->tracks->plugin_exists(plugin))
			{
				mwindow->gui->unlock_window();
				get_gui()->lock_window("KeyFrameThread::apply_preset 1");
				return;
			}

			mwindow->undo->update_undo_before();



#ifdef USE_KEYFRAME_SPANNING
			KeyFrame keyframe;
			presets_db->load_preset(plugin_title, title, &keyframe, is_factory);
			plugin->keyframes->update_parameter(&keyframe);
#else
			KeyFrame *keyframe = plugin->get_keyframe();
			presets_db->load_preset(plugin_title, title, keyframe, is_factory);
#endif

			mwindow->save_backup();
			mwindow->undo->update_undo_after(_("apply preset"), LOAD_AUTOMATION); 

			mwindow->update_plugin_guis(0);
			mwindow->gui->draw_overlays(1);
			mwindow->sync_parameters(CHANGE_PARAMS);
			mwindow->gui->unlock_window();
		}
		else
		{
			presets_db->load_preset(plugin_title, title, plugin_server->keyframe, is_factory);
			plugin_server->update_gui();
		}

		get_gui()->lock_window("KeyFrameThread::apply_preset");
	}
}




KeyFrameWindow::KeyFrameWindow(MWindow *mwindow,
	KeyFrameThread *thread,
	int x,
	int y,
	char *title_string)
 : BC_Window(title_string, 
 	x,
	y,
	mwindow->session->keyframedialog_w, 
	mwindow->session->keyframedialog_h, 
	DP(320), 
	DP(240),
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void KeyFrameWindow::create_objects()
{
	Theme *theme = mwindow->theme;

	theme->get_keyframedialog_sizes(this);
	thread->column_width[0] = mwindow->session->keyframedialog_column1;
	thread->column_width[1] = mwindow->session->keyframedialog_column2;
	lock_window("KeyFrameWindow::create_objects");




	add_subwindow(title4 = new BC_Title(theme->presets_list_x,
		theme->presets_list_y - 
			BC_Title::calculate_h(this, (char*)"Py", LARGEFONT) - 
			theme->widget_border,
		_("Presets:"),
		LARGEFONT));
	add_subwindow(preset_list = new KeyFramePresetsList(thread,
		this,
		theme->presets_list_x,
		theme->presets_list_y,
		theme->presets_list_w, 
		theme->presets_list_h));
	add_subwindow(title5 = new BC_Title(theme->presets_text_x,
		theme->presets_text_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border,
		_("Preset title:")));
	add_subwindow(preset_text = new KeyFramePresetsText(thread,
		this,
		theme->presets_text_x,
		theme->presets_text_y,
		theme->presets_text_w));
	add_subwindow(delete_preset = new KeyFramePresetsDelete(thread,
		this,
		theme->presets_delete_x,
		theme->presets_delete_y));
	add_subwindow(save_preset = new KeyFramePresetsSave(thread,
		this,
		theme->presets_save_x,
		theme->presets_save_y));
	add_subwindow(apply_preset = new KeyFramePresetsApply(thread,
		this,
		theme->presets_apply_x,
		theme->presets_apply_y));




	add_subwindow(new KeyFramePresetsOK(thread, this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	unlock_window();
}


// called when going in & out of a factory preset
void KeyFrameWindow::update_editing()
{
	if(thread->is_factory)
	{
		delete_preset->disable();
		save_preset->disable();
	}
	else
	{
		delete_preset->enable();
		save_preset->enable();
	}
}



int KeyFrameWindow::resize_event(int w, int h)
{
	Theme *theme = mwindow->theme;
	mwindow->session->keyframedialog_w = w;
	mwindow->session->keyframedialog_h = h;
	theme->get_keyframedialog_sizes(this);





	title4->reposition_window(theme->presets_list_x,
		theme->presets_list_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border);
	title5->reposition_window(theme->presets_text_x,
		theme->presets_text_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border);
	preset_list->reposition_window(theme->presets_list_x,
		theme->presets_list_y,
		theme->presets_list_w, 
		theme->presets_list_h);
	preset_text->reposition_window(theme->presets_text_x,
		theme->presets_text_y,
		theme->presets_text_w);
	delete_preset->reposition_window(theme->presets_delete_x,
		theme->presets_delete_y);
	save_preset->reposition_window(theme->presets_save_x,
		theme->presets_save_y);
	apply_preset->reposition_window(theme->presets_apply_x,
		theme->presets_apply_y);

	return 0;
}













KeyFramePresetsList::KeyFramePresetsList(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y,
	int w, 
	int h)
 : BC_ListBox(x, 
		y, 
		w, 
		h,
		LISTBOX_TEXT,
		thread->presets_data)
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsList::selection_changed()
{
	int number = get_selection_number(0, 0);
	if(number >= 0)
	{
		strcpy(thread->preset_text, thread->preset_titles.get(number));
		thread->is_factory = thread->is_factories.get(number);
// show title without factory symbol in the textbox
		window->preset_text->update(
			thread->presets_data->get(number)->get_text());
		window->update_editing();
	}
	
	return 0;
}

int KeyFramePresetsList::handle_event()
{
	thread->apply_preset(thread->preset_text, thread->is_factory);
	window->set_done(0);
	return 0;
}










KeyFramePresetsText::KeyFramePresetsText(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y,
	int w)
 : BC_TextBox(x, 
	y, 
	w, 
	1, 
	thread->preset_text)
{
	this->thread = thread;
	this->window = window;
}

// user entered a title
int KeyFramePresetsText::handle_event()
{
	strcpy(thread->preset_text, get_text());
// once changed, it's now not a factory preset
	thread->is_factory = 0;
	window->update_editing();
	return 0;
}

















KeyFramePresetsDelete::KeyFramePresetsDelete(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsDelete::handle_event()
{
	if(!thread->is_factory)
	{
		thread->delete_preset(thread->preset_text, thread->is_factory);
	}
	return 1;
}







KeyFramePresetsSave::KeyFramePresetsSave(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y)
: BC_GenericButton(x, y, _("Save"))
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsSave::handle_event()
{
	if(!thread->is_factory)
	{
		thread->save_preset(thread->preset_text, thread->is_factory);
	}
	return 1;
}








KeyFramePresetsApply::KeyFramePresetsApply(KeyFrameThread *thread,
	KeyFrameWindow *window,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsApply::handle_event()
{
	thread->apply_preset(thread->preset_text, thread->is_factory);
	return 1;
}


KeyFramePresetsOK::KeyFramePresetsOK(KeyFrameThread *thread,
	KeyFrameWindow *window)
 : BC_OKButton(window)
{
	this->thread = thread;
	this->window = window;
}

int KeyFramePresetsOK::keypress_event()
{
	if(get_keypress() == RETURN)
	{
// Apply the preset
		if(thread->presets_db->preset_exists(thread->plugin_title, 
			thread->preset_text,
			thread->is_factory))
		{
			window->set_done(0);
			return 1;
		}
		else
// Save the preset
		{
			if(!thread->is_factory)
			{
				thread->save_preset(thread->preset_text, thread->is_factory);
				return 1;
			}
		}
	}
	return 0;
}







