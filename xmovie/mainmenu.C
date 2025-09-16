#include "asset.h"
#include "filesystem.h"
#include "load.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "settings.h"
#include "transportque.h"
#include "transportque.inc"

#include <string.h>
#include <unistd.h>

MainMenu::MainMenu(MWindow *mwindow, int w)
 : BC_MenuBar(0, 0, w)
{
	this->mwindow = mwindow;
}

MainMenu::~MainMenu()
{
}

int MainMenu::create_objects()
{
	add_menu(filemenu = new BC_Menu("File"));
	filemenu->add_item(load_file = new Load(mwindow));
	//filemenu->add_item(new LoadDVD(mwindow));
	//filemenu->add_item(new Close(mwindow));
	filemenu->add_item(new BC_MenuItem("-"));
	filemenu->add_item(new Quit(mwindow));
	init_loads(mwindow->defaults);

	BC_Menu *optionsmenu;
	add_menu(optionsmenu = new BC_Menu("Options"));
	optionsmenu->add_item(new EveryFrame(mwindow));
	optionsmenu->add_item(new FullScreen(mwindow));
	optionsmenu->add_item(new SoftwareSync(mwindow));
	optionsmenu->add_item(mwindow->settingsmenu = new SettingsMenu(mwindow));
	optionsmenu->add_item(new BC_MenuItem("-"));
	optionsmenu->add_item(new HalfSize(mwindow));
	optionsmenu->add_item(new OriginalSize(mwindow));
	optionsmenu->add_item(new DoubleSize(mwindow));

	add_menu(audiomenu = new BC_Menu("Audio"));
	add_menu(videomenu = new BC_Menu("Video"));
	total_astreams = total_vstreams = 0;
	return 0;
}

int MainMenu::update_list_menu(BC_Menu *menu, 
		int total, 
		int do_audio, 
		int do_video,
		int &total_streams,
		StreamMenuItem **array)
{
	if(total_streams < total)
	{
		char string[1024];
		while(total_streams < total)
		{
			sprintf(string, "Stream %d", total_streams + 1);
			menu->add_item(array[total_streams++] = new StreamMenuItem(mwindow, 
				menu->total_menuitems(), 
				string,
				do_audio,
				do_video));
		}
	}
	else
	if(menu->total_menuitems() > total)
	{
		while(total_streams > total)
		{
			menu->remove_item();
			total_streams--;
		}
	}

	for(int i = 0; i < total_streams; i++)
	{
		if(do_audio)
		{
			if(mwindow->audio_stream == i)
				array[i]->set_checked(1);
			else
				array[i]->set_checked(0);
		}

		if(do_video)
		{
			if(mwindow->video_stream == i)
				array[i]->set_checked(1);
			else
				array[i]->set_checked(0);
		}
	}
	return 0;
}

int MainMenu::update_audio_streams(int total)
{
	return update_list_menu(audiomenu, total, 1, 0, total_astreams, astreams);
}

int MainMenu::update_video_streams(int total)
{
	return update_list_menu(videomenu, total, 0, 1, total_vstreams, vstreams);
}

int MainMenu::init_loads(Defaults *defaults)
{
	total_loads = defaults->get("TOTAL_LOADS", 0);
	char string[1024], path[1024], filename[1024];
	FileSystem fs;
	if(total_loads > 0) filemenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_loads; i++)
	{
		sprintf(string, "LOADPREVIOUS%d", i);
		defaults->get(string, path);

		filemenu->add_item(load_previous[i] = new LoadPrevious(load_file));
		fs.extract_name(filename, path, 0);
		load_previous[i]->set_text(filename);
		load_previous[i]->set_path(path);
	}
	return 0;
}

int MainMenu::save_loads(Defaults *defaults)
{
	defaults->update("TOTAL_LOADS", total_loads);
	char string[1024];
	for(int i = 0; i < total_loads; i++)
	{
		sprintf(string, "LOADPREVIOUS%d", i);
		defaults->update(string, load_previous[i]->path);
	}
	return 0;
}

int MainMenu::add_load(char *path)
{
	if(total_loads == 0)
	{
		filemenu->add_item(new BC_MenuItem("-"));
	}
	
// test for existing copy
	FileSystem fs;
	char text[1024], new_path[1024];      // get text and path
	fs.extract_name(text, path, 0);
	strcpy(new_path, path);
	
	for(int i = 0; i < total_loads; i++)
	{
		if(!strcmp(load_previous[i]->get_text(), text))     // already exists
		{                                // swap for top load
			for(int j = i; j > 0; j--)   // move preceding loads down
			{
				load_previous[j]->set_text(load_previous[j - 1]->get_text());
				load_previous[j]->set_path(load_previous[j - 1]->path);
			}
			load_previous[0]->set_text(text);
			load_previous[0]->set_path(new_path);
			
			return 1;
		}
	}
	
// add another load
	if(total_loads < TOTAL_LOADS)
	{
		filemenu->add_item(load_previous[total_loads] = new LoadPrevious(load_file));
		total_loads++;
	}
	
// cycle loads down
	for(int i = total_loads - 1; i > 0; i--)
	{
// set menu item text
		load_previous[i]->set_text(load_previous[i - 1]->get_text());
// set filename
		load_previous[i]->set_path(load_previous[i - 1]->path);
	}

// set up the new load
	load_previous[0]->set_text(text);
	load_previous[0]->set_path(new_path);
	return 0;
}

LoadPrevious::LoadPrevious(Load *loadfile)
 : BC_MenuItem("")
{ this->loadfile = loadfile; }

int LoadPrevious::handle_event()
{
// run as thread to prompt users about missing files
// 	loadfile->mwindow->gui->unlock_window();
// 	loadfile->mwindow->stop_playback();
// 	loadfile->mwindow->gui->lock_window();
// 	loadfile->mwindow->close_file();
	loadfile->mwindow->load_file(path, 1);
	return 1;
}

int LoadPrevious::set_path(char *path)
{
	strcpy(this->path, path);
	return 0;
}

Quit::Quit(MWindow *mwindow)
 : BC_MenuItem("Quit", "q", 'q')
{
	this->mwindow = mwindow;
}

Quit::~Quit()
{
}

int Quit::handle_event()
{
	mwindow->quit();


// 	mwindow->gui->unlock_window();
// 	mwindow->engine->que->send_command(STOP_PLAYBACK, 1);
// 	mwindow->exit_cleanly();
// 	mwindow->gui->lock_window("Quit::handle_event");
// 	mwindow->gui->set_done(0);
	return 1;
}

Load::Load(MWindow *mwindow)
 : BC_MenuItem("Open...", "o", 'o')
{
	this->mwindow = mwindow;
	thread = new LoadThread(mwindow);
}

Load::~Load()
{
	delete thread;
}

int Load::handle_event()
{
// 	mwindow->gui->unlock_window();
// 	mwindow->stop_playback();
// 	mwindow->gui->lock_window();
	thread->start();
	return 1;
}

// LoadDVD::LoadDVD(MWindow *mwindow)
//  : BC_MenuItem("Open DVD...", "o", 'o')
// {
// 	this->mwindow = mwindow;
// 	thread = new LoadDVDThread(mwindow);
// }
// 
// LoadDVD::~LoadDVD()
// {
// 	delete thread;
// }
// 
// int LoadDVD::handle_event()
// {
// 	mwindow->gui->unlock_window();
// 	mwindow->stop_playback();
// 	mwindow->gui->lock_window();
// 	thread->start();
// }

SoftwareSync::SoftwareSync(MWindow *mwindow)
 : BC_MenuItem("Synchronize using software")
{
	set_checked(mwindow->software_sync);
	this->mwindow = mwindow;
}
SoftwareSync::~SoftwareSync()
{
}
int SoftwareSync::handle_event()
{
	mwindow->software_sync ^= 1;
	set_checked(mwindow->software_sync);
	return 1;
}

HalfSize::HalfSize(MWindow *mwindow)
 : BC_MenuItem("Half Size", "1", '1')
{
	this->mwindow = mwindow;
}
HalfSize::~HalfSize()
{
}
int HalfSize::handle_event()
{
	mwindow->original_size(.5);
	return 1;
}

OriginalSize::OriginalSize(MWindow *mwindow)
 : BC_MenuItem("Original Size", "2", '2')
{
	this->mwindow = mwindow;
}
OriginalSize::~OriginalSize()
{
}
int OriginalSize::handle_event()
{
	mwindow->original_size(1);
	return 1;
}

DoubleSize::DoubleSize(MWindow *mwindow)
 : BC_MenuItem("Double Size", "3", '3')
{
	this->mwindow = mwindow;
}
DoubleSize::~DoubleSize()
{
}
int DoubleSize::handle_event()
{
	mwindow->original_size(2);
	return 1;
}

FullScreen::FullScreen(MWindow *mwindow)
 : BC_MenuItem("Full screen", "f", 'f')
{
	this->mwindow = mwindow;
	set_checked(mwindow->fullscreen);
}
FullScreen::~FullScreen()
{
}
int FullScreen::handle_event()
{
	mwindow->fullscreen = !mwindow->fullscreen;
	set_checked(mwindow->fullscreen);
	mwindow->gui->create_canvas();
	return 1;
}

SettingsMenu::SettingsMenu(MWindow *mwindow)
 : BC_MenuItem("Settings...")
{
	this->mwindow = mwindow;
	thread = new SettingsThread(mwindow);
}
SettingsMenu::~SettingsMenu()
{
	delete thread;
}
int SettingsMenu::handle_event()
{
	thread->start();
	return 1;
}

EveryFrame::EveryFrame(MWindow *mwindow)
 : BC_MenuItem("Play every frame")
{
	this->mwindow = mwindow;
	set_checked(mwindow->every_frame);
}
EveryFrame::~EveryFrame()
{
}
int EveryFrame::handle_event()
{
	mwindow->every_frame ^= 1;
	set_checked(mwindow->every_frame);
	return 1;
}


StreamMenuItem::StreamMenuItem(MWindow *mwindow, 
	int stream_number, 
	char *text,
	int do_audio, 
	int do_video)
 : BC_MenuItem(text)
{
	this->mwindow = mwindow;
	this->stream_number = stream_number;
	this->do_audio = do_audio;
	this->do_video = do_video;
	if(do_video) set_checked(mwindow->video_stream == stream_number);
	else
	if(do_audio) set_checked(mwindow->audio_stream == stream_number);
}
StreamMenuItem::~StreamMenuItem()
{
}
int StreamMenuItem::handle_event()
{
	mwindow->gui->unlock_window();
	mwindow->engine->que->send_command(STOP_PLAYBACK, 0);
	mwindow->gui->lock_window("StreamMenuItem::handle_event");
	if(do_audio) mwindow->set_audio_stream(stream_number);
	if(do_video) mwindow->set_video_stream(stream_number);
	return 1;
}



Close::Close(MWindow *mwindow)
 : BC_MenuItem("Close", "w", 'w')
{
	this->mwindow = mwindow;
}

Close::~Close()
{
}

int Close::handle_event()
{
	mwindow->gui->unlock_window();
	mwindow->engine->que->send_command(STOP_PLAYBACK, 0);
	mwindow->gui->lock_window("Close::handle_event");
	mwindow->close_file();
	return 1;
}


