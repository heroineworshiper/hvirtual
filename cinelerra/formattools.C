/*
 * CINELERRA
 * Copyright (C) 2010-2022 Adam Williams <broadcast at earthling dot net>
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
#include "recordconfig.h"
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

    audio_title = 0;
    video_title = 0;
    mplex_title = 0;

    audio_switch = 0;
    video_switch = 0;
    mplex_switch = 0;

	aparams_button = 0;
	vparams_button = 0;
    mplexparams_button = 0;

	aparams_thread = 0;
	vparams_thread = 0;
    mplexparams_thread = 0;

	channels_tumbler = 0;
	path_textbox = 0;
	path_button = 0;
	w = window->get_w() - mwindow->theme->widget_border;
    user_w = -1;
	file_entries = 0;
}

FormatTools::~FormatTools()
{
	delete path_button;
	delete path_textbox;
	delete format_button;

    if(audio_title) delete audio_title;
    if(video_title) delete video_title;
    if(mplex_title) delete mplex_title;

    if(audio_switch) delete audio_switch;
    if(video_switch) delete video_switch;
    if(mplex_switch) delete mplex_switch;


	if(aparams_button) delete aparams_button;
	if(vparams_button) delete vparams_button;
	if(mplexparams_button) delete mplexparams_button;

	if(aparams_thread) delete aparams_thread;
	if(vparams_thread) delete vparams_thread;
    if(mplexparams_thread) delete mplexparams_thread;

	if(channels_tumbler) delete channels_tumbler;
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
						int prompt_video_compression,
                        int prompt_wrapper,
						char *locked_compressor,
						int recording,
						int *strategy,
						int brender)
{
	int x = init_x;
	int y = init_y;
	int margin = mwindow->theme->widget_border;
//printf("FormatTools::create_objects %d y=%d\n", __LINE__, y);

	this->locked_compressor = locked_compressor;
	this->recording = recording;
	this->use_brender = brender;
	this->do_audio = do_audio;
	this->do_video = do_video;
	this->prompt_audio = prompt_audio;
//	this->prompt_audio_channels = prompt_audio_channels;
	this->prompt_video = prompt_video;
	this->prompt_video_compression = prompt_video_compression;
    this->prompt_wrapper = prompt_wrapper;
	this->strategy = strategy;

    audio_checked = asset->audio_data;
    video_checked = asset->video_data;
    mplex_checked = asset->do_wrapper;

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
		x += path_textbox->get_w() /* + margin */;
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
        if(user_w > 0)
            w = user_w;
        else
            w = window->get_w() - init_x;
//		w = MAX(w, DP(305));
//		w = x + path_button->get_w() + margin;
		x -= path_textbox->get_w() + margin;
		y += DP(35);
	}
	else
	{
        if(user_w > 0)
            w = user_w;
        else
            w = window->get_w() - init_x;
//		w = x + DP(305);
//		w = DP(305);
	}

	window->add_subwindow(format_title = new BC_Title(x, y, _("File Format:")));
	x += format_title->get_w() + margin;
	window->add_subwindow(format_text = new BC_TextBox(x, 
		y, 
		DP(200), 
		1, 
		File::formattostr(asset->format)));
    format_text->set_read_only(1);
	x += format_text->get_w() + margin;

//printf("FormatTools::create_objects %d %p\n", __LINE__, window);
	window->add_subwindow(format_button = new FormatFormat(x, 
		y, 
		this));
	format_button->create_objects();

	x = init_x;
	y += format_button->get_h() + margin;
	if(do_audio)
	{
//printf("FormatTools::create_objects %d\n", __LINE__);
//		window->add_subwindow(audio_title = new BC_Title(x, y, _("Audio:"), LARGEFONT, RED));
//		x += audio_title->get_w() + margin;
		window->add_subwindow(aparams_button = new FormatAParams(mwindow, this, x, y));
		x += aparams_button->get_w() + margin;
		if(prompt_audio) 
		{
			window->add_subwindow(audio_switch = new FormatAudio(x, y, this, asset->audio_data));
		}
        else
        {
            window->add_subwindow(audio_title = new BC_Title(x, y, _("Configure encoding")));
        }
		x = init_x;
		y += aparams_button->get_h() + margin;

// Audio channels only used for recording.
// 		if(prompt_audio_channels)
// 		{
// 			window->add_subwindow(channels_title = new BC_Title(x, y, _("Number of audio channels to record:")));
// 			x += 260;
// 			window->add_subwindow(channels_button = new FormatChannels(x, y, this));
// 			x += channels_button->get_w() + margin;
// 			window->add_subwindow(channels_tumbler = new BC_ITumbler(channels_button, 1, MAXCHANNELS, x, y));
// 			y += channels_button->get_h() + margin;
// 			x = init_x;
// 		}

		aparams_thread = new FormatAThread(this);
	}

//printf("FormatTools::create_objects 7\n");
	if(do_video)
	{

//		window->add_subwindow(video_title = new BC_Title(x, y, _("Video:"), LARGEFONT, RED));
//		x += video_title->get_w() + margin;
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
		if(prompt_video_compression)
		{
            window->add_subwindow(video_title = new BC_Title(x, y, _("Configure encoding")));
			y += vparams_button->get_h();
		}

//printf("FormatTools::create_objects 10\n");
		y += margin;
		vparams_thread = new FormatVThread(this);
	}

    if((do_audio && do_video) || prompt_wrapper)
    {
// wrapper
 	    x = init_x;
    //    window->add_subwindow(mplex_title = new BC_Title(x, y, _("Wrapper:"), LARGEFONT, RED));
    //	x += mplex_title->get_w() + margin;
	    window->add_subwindow(mplexparams_button = new FormatMplexParams(mwindow, this, x, y));
	    x += mplexparams_button->get_w() + margin;
	    window->add_subwindow(mplex_switch = new FormatMplex(x, y, this, asset->do_wrapper));
	    y += mplex_switch->get_h() + margin;
	    mplexparams_thread = new FormatMplexThread(this);
    }
    

//printf("FormatTools::create_objects 11\n");

	if(strategy)
	{
        x = init_x;
        window->add_subwindow(bar = new BC_Bar(x, y, w - margin));
        y += margin;
		window->add_subwindow(multiple_files = new FormatMultiple(mwindow, x, y, strategy));
		y += multiple_files->get_h() + margin;
	}

//printf("FormatTools::create_objects %d y=%d\n", __LINE__, y);

	init_y = y;
    update_prompts();
}

void FormatTools::update_driver(VideoInConfig *in_config)
{
//	this->video_driver = driver;

	switch(in_config->driver)
	{
        case VIDEO4LINUX2:
            switch(in_config->v4l2_format)
            {
            case CAPTURE_JPEG:
            case CAPTURE_JPEG_NOHEAD:
            case CAPTURE_MJPG_1FIELD:
				format_text->update(MOV_NAME);
				asset->format = FILE_MOV;
				locked_compressor = (char*)QUICKTIME_JPEG;
				strcpy(asset->vcodec, QUICKTIME_JPEG);
                break;
            case CAPTURE_MJPG:
				format_text->update(MOV_NAME);
				asset->format = FILE_MOV;
				locked_compressor = (char*)QUICKTIME_MJPA;
				strcpy(asset->vcodec, QUICKTIME_MJPA);
                break;
            default:
				format_text->update(File::formattostr(asset->format));
                locked_compressor = 0;
                break;
            }
			audio_switch->update(asset->audio_data);
			video_switch->update(asset->video_data);
            break;

		case CAPTURE_DVB:
//		case CAPTURE_MPEG:
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
//		case VIDEO4LINUX2JPEG:
//		case VIDEO4LINUX2MJPG:
//		case CAPTURE_JPEG_WEBCAM:
			if(asset->format != FILE_AVI &&
				asset->format != FILE_MOV)
			{
				format_text->update(MOV_NAME);
				asset->format = FILE_MOV;
			}
			else
            {
				format_text->update(File::formattostr(asset->format));
            }

			switch(in_config->driver)
			{
				case CAPTURE_IEC61883:
				case CAPTURE_FIREWIRE:
					locked_compressor = (char*)QUICKTIME_DVSD;
					strcpy(asset->vcodec, QUICKTIME_DVSD);
					break;

				case CAPTURE_BUZ:
//				case VIDEO4LINUX2MJPG:
					locked_compressor = (char*)QUICKTIME_MJPA;
					strcpy(asset->vcodec, QUICKTIME_MJPA);
					break;

// 				case VIDEO4LINUX2JPEG:
// 					locked_compressor = (char*)QUICKTIME_JPEG;
// 					strcpy(asset->vcodec, QUICKTIME_JPEG);
// 					break;
// 
// 				case CAPTURE_JPEG_WEBCAM:
// 					locked_compressor = (char*)QUICKTIME_JPEG;
// 					strcpy(asset->vcodec, QUICKTIME_JPEG);
// 					break;
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
    update_prompts();
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
    
    if(!extension)
    {
        return;
    }
    
    
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
//printf("FormatTools::update_extension %d asset->path=%s\n", 
//__LINE__, asset->path);
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

void FormatTools::update_prompts()
{
    if(video_switch)
    {
        if(File::supports_video(asset->format))
        {
            asset->video_data = video_checked;
            video_switch->enable();
            video_switch->update(asset->video_data);
        }
        else
        {
            asset->video_data = 0;
            video_switch->disable();
            video_switch->update(0);
        }
    }

    if(audio_switch)
    {
        if(File::supports_audio(asset->format))
        {
            asset->audio_data = audio_checked;
            audio_switch->enable();
            audio_switch->update(asset->audio_data);
        }
        else
        {
            asset->audio_data = 0;
            audio_switch->disable();
            audio_switch->update(0);
        }
    }


    if(mplex_switch)
    {
        if(File::supports_wrapper(asset->format))
        {
            asset->do_wrapper = mplex_checked;
            mplex_switch->enable();
            mplex_switch->update(asset->do_wrapper);
        }
        else
        {
            asset->do_wrapper = 0;
            mplex_switch->disable();
            mplex_switch->update(0);
        }
    }
}

void FormatTools::update(Asset *asset, int *strategy)
{
	this->asset = asset;
	this->strategy = strategy;

	if(path_textbox) 
	{
    	path_textbox->update(asset->path);
	}
    format_text->update(File::formattostr(asset->format));
	if(do_audio && audio_switch) audio_switch->update(asset->audio_data);
	if(do_video && video_switch) video_switch->update(asset->video_data);
    if(mplex_switch) mplex_switch->update(asset->do_wrapper);
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
    if(mplexparams_thread && mplexparams_thread->running())
    {
        mplexparams_thread->file->close_window();
        mplexparams_thread->join();
    }
}

int FormatTools::get_w()
{
	return w;
}

void FormatTools::set_w(int w)
{
	this->user_w = this->w = w;
}

void FormatTools::reposition_window(int &init_x, int &init_y)
{
	int x = init_x;
	int y = init_y;
	int margin = mwindow->theme->widget_border;

	if(path_textbox) 
	{
//printf("FormatTools::reposition_window %d %d\n", __LINE__, w);
		path_textbox->reposition_window(x, 
			y, 
			w - mwindow->theme->get_image_set("wrench")[0]->get_w() - margin);
		x += path_textbox->get_w() /* + margin */;
		path_button->reposition_window(x, y);
		x -= path_textbox->get_w() + margin;
		y += DP(35);
	}

	format_title->reposition_window(x, y);
	x += format_title->get_w() + margin;
	format_text->reposition_window(x, y);
	x += format_text->get_w() + margin;
	format_button->reposition_window(x, y);

	x = init_x;
	y += format_button->get_h() + margin;

	if(do_audio)
	{
//		audio_title->reposition_window(x, y);
//		x += audio_title->get_w() + margin;
		aparams_button->reposition_window(x, y);
		x += aparams_button->get_w() + margin;
		if(audio_switch) 
        {
            audio_switch->reposition_window(x, y);
        }

        if(audio_title)
        {
            audio_title->reposition_window(x, y);
        }

		x = init_x;
		y += aparams_button->get_h() + margin;
// 		if(prompt_audio_channels)
// 		{
// 			channels_title->reposition_window(x, y);
// 			x += DP(260);
// 			channels_button->reposition_window(x, y);
// 			x += channels_button->get_w() + margin;
// 			channels_tumbler->reposition_window(x, y);
// 			y += channels_button->get_h() + margin;
// 			x = init_x;
// 		}
	}


	if(do_video)
	{
//		video_title->reposition_window(x, y);
//		x += video_title->get_w() + margin;
		if(prompt_video_compression)
		{
			vparams_button->reposition_window(x, y);
			x += vparams_button->get_w() + margin;
		}

		if(video_switch)
		{
			video_switch->reposition_window(x, y);
			y += video_switch->get_h();
		}
		else
        if(prompt_video_compression)
		{
            if(video_title)
            {
                video_title->reposition_window(x, y);
            }
			y += vparams_button->get_h();
		}

		y += margin;
		x = init_x;
	}
    
    if(mplexparams_button)
    {
        mplexparams_button->reposition_window(x, y);
        x += mplexparams_button->get_w() + margin;
        mplex_switch->reposition_window(x, y);
        y += mplex_switch->get_h() + margin;
        x = init_x;
    }

	if(strategy)
	{
        bar->reposition_window(x, y, get_w() - margin);
        y += margin;
		multiple_files->reposition_window(x, y);
		y += multiple_files->get_h() + margin;
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



int FormatTools::set_mplex_options()
{
	if(!mplexparams_thread->running())
	{
		mplexparams_thread->start();
	}
	else
	{
		mplexparams_thread->file->raise_window();
	}

	return 0;
}




FormatAParams::FormatAParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{
	this->format = format;
	set_tooltip(_("Configure audio compression"));
}
int FormatAParams::handle_event() 
{
	format->set_audio_options(); 
    return 0;
}





FormatVParams::FormatVParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{ 
	this->format = format; 
	set_tooltip(_("Configure video compression"));
}
int FormatVParams::handle_event() 
{ 
	format->set_video_options(); 
    return 0;
}


FormatMplexParams::FormatMplexParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{ 
	this->format = format; 
	set_tooltip(_("Configure wrapper"));
}
int FormatMplexParams::handle_event() 
{ 
	format->set_mplex_options(); 
    return 0;
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
	file->get_parameters(format->window, 
		format->plugindb, 
		format->asset, 
		AUDIO_PARAMS,
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
		join();
		delete file;
	}
	else
	{
		delete file;
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
	file->get_parameters(format->window, 
		format->plugindb, 
		format->asset, 
		VIDEO_PARAMS, 
		format->locked_compressor);
}







FormatMplexThread::FormatMplexThread(FormatTools *format)
 : Thread(1, 0, 0)
{
	this->format = format;
	file = new File;
	joined = 1;
}

FormatMplexThread::~FormatMplexThread() 
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

void FormatMplexThread::start()
{
	if(!joined)
	{
		join();
	}

	joined = 0;
	Thread::start();
}

void FormatMplexThread::run()
{
	file->get_parameters(format->window, 
		format->plugindb, 
		format->asset, 
		MPLEX_PARAMS, 
		0);
}





FormatPathText::FormatPathText(int x, int y, FormatTools *format)
 : BC_TextBox(x, 
 	y, 
	format->w - 
		format->mwindow->theme->get_image_set("wrench")[0]->get_w() - 
		MWindow::theme->widget_border, 
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
	format->asset->audio_data = format->audio_checked = get_value();
    return 0;
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
	format->asset->video_data = format->video_checked = get_value();
    return 0;
}



FormatMplex::FormatMplex(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x, 
 	y, 
	default_, 
	(char*)(_("Create wrapper")))
{
    this->format = format; 
}
int FormatMplex::handle_event()
{
    format->asset->do_wrapper = format->mplex_checked = get_value();
    return 0;
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
	if(get_selection(0, 0) != 0)
	{
        const char *text = get_selection(0, 0)->get_text();
		int new_format = File::strtoformat(text);

// printf("FormatFormat::handle_event %d name='%s' new_format=%d\n", 
// __LINE__,
// text,
// new_format);

//		if(new_format != format->asset->format)
		{
			format->asset->format = new_format;
			format->format_text->update(get_selection(0, 0)->get_text());
			format->update_extension();
            format->update_prompts();
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


