
/*
 * CINELERRA
 * Copyright (C) 2010-2013 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "clip.h"
#include "guicast.h"
#include "file.h"
#include "filesystem.h"
#include "formattools.h"
#include "language.h"
#include "maxchannels.h"
#include "mwindow.h"
#include "preferences.h"
#include "quicktime.h"
#include "theme.h"
#include "videodevice.inc"
#include <string.h>
#include <unistd.h>


FormatTools::FormatTools(MWindow *mwindow,
				BC_WindowBase *window, 
				Asset *asset)
{
	this->mwindow = mwindow;
	this->window = window;
	this->asset = asset;
	this->plugindb = mwindow->plugindb;

	aparams_button = 0;
	vparams_button = 0;
	aparams_thread = 0;
	vparams_thread = 0;
	channels_tumbler = 0;
	path_textbox = 0;
	path_button = 0;
	w = window->get_w();
	file_entries = 0;
}

FormatTools::~FormatTools()
{
SET_TRACE
	delete path_button;
SET_TRACE
	delete path_textbox;
SET_TRACE
	delete format_button;
SET_TRACE

	if(aparams_button) delete aparams_button;
SET_TRACE
	if(vparams_button) delete vparams_button;
SET_TRACE
	if(aparams_thread) delete aparams_thread;
SET_TRACE
	if(vparams_thread) delete vparams_thread;
SET_TRACE
	if(channels_tumbler) delete channels_tumbler;
SET_TRACE
	if(file_entries)
	{
		file_entries->remove_all_objects();
		delete file_entries;
	}
}

void FormatTools::create_objects(int &init_x, 
						int &init_y, 
						int do_audio,    // Include support for audio
						int do_video,   // Include support for video
						int prompt_audio,  // Include checkbox for audio
						int prompt_video,
						int prompt_audio_channels,
						int prompt_video_compression,
						char *locked_compressor,
						int recording,
						int *strategy,
						int brender)
{
	int x = init_x;
	int y = init_y;
	int margin = mwindow->theme->widget_border;

	this->locked_compressor = locked_compressor;
	this->recording = recording;
	this->use_brender = brender;
	this->do_audio = do_audio;
	this->do_video = do_video;
	this->prompt_audio = prompt_audio;
	this->prompt_audio_channels = prompt_audio_channels;
	this->prompt_video = prompt_video;
	this->prompt_video_compression = prompt_video_compression;
	this->strategy = strategy;


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

//printf("FormatTools::create_objects 1\n");

// Modify strategy depending on render farm
	if(strategy)
	{
		if(mwindow->preferences->use_renderfarm)
		{
			if(*strategy == FILE_PER_LABEL)
				*strategy = FILE_PER_LABEL_FARM;
			else
			if(*strategy == SINGLE_PASS)
				*strategy = SINGLE_PASS_FARM;
		}
		else
		{
			if(*strategy == FILE_PER_LABEL_FARM)
				*strategy = FILE_PER_LABEL;
			else
			if(*strategy == SINGLE_PASS_FARM)
				*strategy = SINGLE_PASS;
		}
	}

	if(!recording)
	{
		window->add_subwindow(path_textbox = new FormatPathText(x, y, this));
		x += path_textbox->get_w() + 5;
		window->add_subwindow(path_button = new BrowseButton(
			mwindow->theme,
			window,
			path_textbox, 
			x, 
			y, 
			asset->path,
			_("Output to file"),
			_("Select a file to write to:"),
			0));

// Set w for user.
		w = MAX(w, 305);
//		w = x + path_button->get_w() + margin;
		x -= path_textbox->get_w() + margin;
		y += 35;
	}
	else
	{
//		w = x + 305;
		w = DP(305);
	}

	window->add_subwindow(format_title = new BC_Title(x, y, _("File Format:")));
	x += format_title->get_w() + margin;
	window->add_subwindow(format_text = new BC_TextBox(x, 
		y, 
		DP(200), 
		1, 
		File::formattostr(asset->format)));
	x += format_text->get_w() + margin;

//printf("FormatTools::create_objects %d %p\n", __LINE__, window);
	window->add_subwindow(format_button = new FormatFormat(x, 
		y, 
		this));
	format_button->create_objects();

	x = init_x;
	y += format_button->get_h() + DP(10);
	if(do_audio)
	{
		window->add_subwindow(audio_title = new BC_Title(x, y, _("Audio:"), LARGEFONT, RED));
		x += audio_title->get_w() + margin;
		window->add_subwindow(aparams_button = new FormatAParams(mwindow, this, x, y));
		x += aparams_button->get_w() + margin;
		if(prompt_audio) 
		{
			window->add_subwindow(audio_switch = new FormatAudio(x, y, this, asset->audio_data));
		}
		x = init_x;
		y += aparams_button->get_h() + DP(20);

// Audio channels only used for recording.
// 		if(prompt_audio_channels)
// 		{
// 			window->add_subwindow(channels_title = new BC_Title(x, y, _("Number of audio channels to record:")));
// 			x += 260;
// 			window->add_subwindow(channels_button = new FormatChannels(x, y, this));
// 			x += channels_button->get_w() + 5;
// 			window->add_subwindow(channels_tumbler = new BC_ITumbler(channels_button, 1, MAXCHANNELS, x, y));
// 			y += channels_button->get_h() + 20;
// 			x = init_x;
// 		}

//printf("FormatTools::create_objects 6\n");
		aparams_thread = new FormatAThread(this);
	}

//printf("FormatTools::create_objects 7\n");
	if(do_video)
	{

//printf("FormatTools::create_objects 8\n");
		window->add_subwindow(video_title = new BC_Title(x, y, _("Video:"), LARGEFONT, RED));
		x += video_title->get_w() + margin;
		if(prompt_video_compression)
		{
			window->add_subwindow(vparams_button = new FormatVParams(mwindow, this, x, y));
			x += vparams_button->get_w() + margin;
		}

//printf("FormatTools::create_objects 9\n");
		if(prompt_video)
		{
			window->add_subwindow(video_switch = new FormatVideo(x, y, this, asset->video_data));
			y += video_switch->get_h();
		}
		else
		{
			y += vparams_button->get_h();
		}

//printf("FormatTools::create_objects 10\n");
		y += DP(10);
		vparams_thread = new FormatVThread(this);
	}

//printf("FormatTools::create_objects 11\n");

	x = init_x;
	if(strategy)
	{
		window->add_subwindow(multiple_files = new FormatMultiple(mwindow, x, y, strategy));
		y += multiple_files->get_h() + DP(10);
	}

//printf("FormatTools::create_objects 12\n");

	init_y = y;
}

void FormatTools::update_driver(int driver)
{
	this->video_driver = driver;

	switch(driver)
	{
		case CAPTURE_DVB:
		case CAPTURE_MPEG:
// Just give the user information about how the stream is going to be
// stored but don't change the asset.
// Want to be able to revert to user settings.
			if(asset->format != FILE_MPEG)
			{
				format_text->update(_("MPEG transport stream"));
				asset->format = FILE_MPEG;
			}
			locked_compressor = 0;
			audio_switch->update(1);
			video_switch->update(1);
			break;

		case CAPTURE_IEC61883:
		case CAPTURE_FIREWIRE:
		case CAPTURE_BUZ:
		case VIDEO4LINUX2JPEG:
		case CAPTURE_JPEG_WEBCAM:
			if(asset->format != FILE_AVI &&
				asset->format != FILE_MOV)
			{
				format_text->update(MOV_NAME);
				asset->format = FILE_MOV;
			}
			else
				format_text->update(File::formattostr(asset->format));

			switch(driver)
			{
				case CAPTURE_IEC61883:
				case CAPTURE_FIREWIRE:
					locked_compressor = (char*)QUICKTIME_DVSD;
					strcpy(asset->vcodec, QUICKTIME_DVSD);
					break;

				case CAPTURE_BUZ:
				case VIDEO4LINUX2JPEG:
					locked_compressor = (char*)QUICKTIME_MJPA;
					strcpy(asset->vcodec, QUICKTIME_MJPA);
					break;

				case CAPTURE_JPEG_WEBCAM:
					locked_compressor = (char*)QUICKTIME_JPEG;
					strcpy(asset->vcodec, QUICKTIME_JPEG);
					break;
			}

			audio_switch->update(asset->audio_data);
			video_switch->update(asset->video_data);
			break;





		default:
			format_text->update(File::formattostr(asset->format));
			locked_compressor = 0;
			audio_switch->update(asset->audio_data);
			video_switch->update(asset->video_data);
			break;
	}
	close_format_windows();
}



int FormatTools::handle_event()
{
	return 0;
}

Asset* FormatTools::get_asset()
{
	return asset;
}

void FormatTools::update_extension()
{
	const char *extension = File::get_tag(asset->format);
// split multiple extensions
	ArrayList<const char*> extensions;
	int len = strlen(extension);
	const char *extension_ptr = extension;
	for(int i = 0; i <= len; i++)
	{
		if(extension[i] == '/' || extension[i] == 0)
		{
			extensions.append(extension_ptr);
			extension_ptr = extension + i + 1;
		}
	}
	
	if(extensions.size())
	{
		char *ptr = strrchr(asset->path, '.');
		if(!ptr)
		{
			ptr = asset->path + strlen(asset->path);
			*ptr = '.';
		}
		ptr++;
		
		
// test for equivalent extension
		int need_extension = 1;
		int extension_len = 0;
		for(int i = 0; i < extensions.size() && need_extension; i++)
		{
			char *ptr1 = ptr;
			extension_ptr = extensions.get(i);
// test an extension
			need_extension = 0;
			while(*ptr1 != 0 && *extension_ptr != 0 && *extension_ptr != '/')
			{
				if(tolower(*ptr1) != tolower(*extension_ptr))
				{
					need_extension = 1;
					break;
				}
				ptr1++;
				extension_ptr++;
			}

			if(*ptr1 == 0 && 
				*extension_ptr != 0 &&
				*extension_ptr != '/')
				need_extension = 1;
		}

//printf("FormatTools::update_extension %d %d\n", __LINE__, need_extension);
// copy extension
		if(need_extension) 
		{
			char *ptr1 = ptr;
			extension_ptr = extensions.get(0);
			while(*extension_ptr != 0 && *extension_ptr != '/')
				*ptr1++ = *extension_ptr++;
			*ptr1 = 0;
		}

		int character1 = ptr - asset->path;
		int character2 = strlen(asset->path);
//		*(asset->path + character2) = 0;
		if(path_textbox) 
		{
			path_textbox->update(asset->path);
			path_textbox->set_selection(character1, character2, character2);
		}
	}
}

void FormatTools::update(Asset *asset, int *strategy)
{
	this->asset = asset;
	this->strategy = strategy;

	if(path_textbox) 
		path_textbox->update(asset->path);
	format_text->update(File::formattostr(plugindb, asset->format));
	if(do_audio && audio_switch) audio_switch->update(asset->audio_data);
	if(do_video && video_switch) video_switch->update(asset->video_data);
	if(strategy)
	{
		multiple_files->update(strategy);
	}
	close_format_windows();
}

void FormatTools::close_format_windows()
{
// This is done in ~file
	if(aparams_thread && aparams_thread->running())
	{
		aparams_thread->file->close_window();
		aparams_thread->join();
	}
	if(vparams_thread && vparams_thread->running())
	{
		vparams_thread->file->close_window();
		vparams_thread->join();
	}
}

int FormatTools::get_w()
{
	return w;
}

void FormatTools::set_w(int w)
{
	this->w = w;
}

void FormatTools::reposition_window(int &init_x, int &init_y)
{
	int x = init_x;
	int y = init_y;
	int margin = mwindow->theme->widget_border;

	if(path_textbox) 
	{
		path_textbox->reposition_window(x, y);
		x += path_textbox->get_w() + margin;
		path_button->reposition_window(x, y);
		x -= path_textbox->get_w() + margin;
		y += DP(35);
	}

	format_title->reposition_window(x, y);
	x += DP(90);
	format_text->reposition_window(x, y);
	x += format_text->get_w();
	format_button->reposition_window(x, y);

	x = init_x;
	y += format_button->get_h() + DP(10);

	if(do_audio)
	{
		audio_title->reposition_window(x, y);
		x += DP(80);
		aparams_button->reposition_window(x, y);
		x += aparams_button->get_w() + DP(10);
		if(prompt_audio) audio_switch->reposition_window(x, y);

		x = init_x;
		y += aparams_button->get_h() + DP(20);
		if(prompt_audio_channels)
		{
			channels_title->reposition_window(x, y);
			x += DP(260);
			channels_button->reposition_window(x, y);
			x += channels_button->get_w() + margin;
			channels_tumbler->reposition_window(x, y);
			y += channels_button->get_h() + DP(20);
			x = init_x;
		}
	}


	if(do_video)
	{
		video_title->reposition_window(x, y);
		x += DP(80);
		if(prompt_video_compression)
		{
			vparams_button->reposition_window(x, y);
			x += vparams_button->get_w() + DP(10);
		}

		if(prompt_video)
		{
			video_switch->reposition_window(x, y);
			y += video_switch->get_h();
		}
		else
		{
			y += vparams_button->get_h();
		}

		y += DP(10);
		x = init_x;
	}

	if(strategy)
	{
		multiple_files->reposition_window(x, y);
		y += multiple_files->get_h() + DP(10);
	}

	init_y = y;
}


int FormatTools::set_audio_options()
{
//	if(video_driver == CAPTURE_DVB)
//	{
//		return 0;
//	}

	if(!aparams_thread->running())
	{
		aparams_thread->start();
	}
	else
	{
		aparams_thread->file->raise_window();
	}
	return 0;
}

int FormatTools::set_video_options()
{
//	if(video_driver == CAPTURE_DVB)
//	{
//		return 0;
//	}

	if(!vparams_thread->running())
	{
		vparams_thread->start();
	}
	else
	{
		vparams_thread->file->raise_window();
	}

	return 0;
}





FormatAParams::FormatAParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{
	this->format = format;
	set_tooltip(_("Configure audio compression"));
}
FormatAParams::~FormatAParams() 
{
}
int FormatAParams::handle_event() 
{
	format->set_audio_options(); 
}





FormatVParams::FormatVParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{ 
	this->format = format; 
	set_tooltip(_("Configure video compression"));
}
FormatVParams::~FormatVParams() 
{
}
int FormatVParams::handle_event() 
{ 
	format->set_video_options(); 
}





FormatAThread::FormatAThread(FormatTools *format)
 : Thread(1, 0, 0)
{ 
	this->format = format; 
	file = new File;
	joined = 1;
}

FormatAThread::~FormatAThread() 
{
	if(!joined)
	{
		file->close_window();
		join();
		delete file;
	}
	else
	{
		delete file;
	}
}

void FormatAThread::start()
{
	if(!joined)
	{
		join();
	}

	joined = 0;
	Thread::start();
}


void FormatAThread::run()
{
	file->get_options(format->window, 
		format->plugindb, 
		format->asset, 
		1, 
		0,
		0);
}




FormatVThread::FormatVThread(FormatTools *format)
 : Thread(1, 0, 0)
{
	this->format = format;
	file = new File;
	joined = 1;
}

FormatVThread::~FormatVThread() 
{
	if(!joined)
	{
		file->close_window();
SET_TRACE
		join();
SET_TRACE
		delete file;
SET_TRACE
	}
	else
	{
SET_TRACE
		delete file;
SET_TRACE
	}
}

void FormatVThread::start()
{
	if(!joined)
	{
		join();
	}

	joined = 0;
	Thread::start();
}

void FormatVThread::run()
{
	file->get_options(format->window, 
		format->plugindb, 
		format->asset, 
		0, 
		1, 
		format->locked_compressor);
}





FormatPathText::FormatPathText(int x, int y, FormatTools *format)
 : BC_TextBox(x, 
 	y, 
	format->w - 
		format->mwindow->theme->get_image_set("wrench")[0]->get_w() - 
		x - DP(10), 
	1, 
	format->asset->path) 
{
	this->format = format; 
}

FormatPathText::~FormatPathText() 
{
}
int FormatPathText::handle_event() 
{
// Suggestions
	calculate_suggestions(format->file_entries);



	strcpy(format->asset->path, get_text());
	format->handle_event();

	return 1;
}




FormatAudio::FormatAudio(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x, 
 	y, 
	default_, 
	(char*)(format->recording ? _("Record audio tracks") : _("Render audio tracks")))
{ 
	this->format = format; 
}
FormatAudio::~FormatAudio() {}
int FormatAudio::handle_event()
{
	format->asset->audio_data = get_value();
}


FormatVideo::FormatVideo(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x, 
 	y, 
	default_, 
	(char*)(format->recording ? _("Record video tracks") : _("Render video tracks")))
{
this->format = format; 
}
FormatVideo::~FormatVideo() {}
int FormatVideo::handle_event()
{
	format->asset->video_data = get_value();
}




FormatFormat::FormatFormat(int x, 
	int y, 
	FormatTools *format)
 : FormatPopup(format->plugindb, 
 	x, 
	y,
	format->use_brender)
{ 
	this->format = format; 
}
FormatFormat::~FormatFormat() 
{
}
int FormatFormat::handle_event()
{
	if(get_selection(0, 0) >= 0)
	{
		int new_format = File::strtoformat(format->plugindb, get_selection(0, 0)->get_text());
//		if(new_format != format->asset->format)
		{
			format->asset->format = new_format;
			format->format_text->update(get_selection(0, 0)->get_text());
			format->update_extension();
			format->close_format_windows();
		}
	}
	return 1;
}



FormatChannels::FormatChannels(int x, int y, FormatTools *format)
 : BC_TextBox(x, y, DP(100), 1, format->asset->channels) 
{ 
 	this->format = format; 
}
FormatChannels::~FormatChannels() 
{
}
int FormatChannels::handle_event() 
{
	format->asset->channels = atol(get_text());
	return 1;
}

FormatToTracks::FormatToTracks(int x, int y, int *output)
 : BC_CheckBox(x, y, *output, _("Overwrite project with output"))
{ 
	this->output = output; 
}
FormatToTracks::~FormatToTracks() 
{
}
int FormatToTracks::handle_event()
{
	*output = get_value();
	return 1;
}


FormatMultiple::FormatMultiple(MWindow *mwindow, int x, int y, int *output)
 : BC_CheckBox(x, 
 	y, 
	(*output == FILE_PER_LABEL) || (*output == FILE_PER_LABEL_FARM), 
	_("Create new file at each label"))
{ 
	this->output = output;
	this->mwindow = mwindow;
}
FormatMultiple::~FormatMultiple() 
{
}
int FormatMultiple::handle_event()
{
	if(get_value())
	{
		if(mwindow->preferences->use_renderfarm)
			*output = FILE_PER_LABEL_FARM;
		else
			*output = FILE_PER_LABEL;
	}
	else
	{
		if(mwindow->preferences->use_renderfarm)
			*output = SINGLE_PASS_FARM;
		else
			*output = SINGLE_PASS;
	}
	return 1;
}

void FormatMultiple::update(int *output)
{
	this->output = output;
	if(*output == FILE_PER_LABEL_FARM ||
		*output ==FILE_PER_LABEL)
		set_value(1);
	else
		set_value(0);
}


