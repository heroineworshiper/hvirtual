#ifndef MAINMENU_H
#define MAINMENU_H

#include "defaults.h"
#include "guicast.h"
#include "load.inc"
#include "loaddvd.inc"
#include "mwindow.inc"
#include "settings.inc"

#define TOTAL_LOADS 5

class Load;
class LoadPrevious;
class StreamMenuItem;

class MainMenu : public BC_MenuBar
{
public:
	MainMenu(MWindow *mwindow, int w);
	~MainMenu();

	int create_objects();
	int update_list_menu(BC_Menu *menu, 
			int total, 
			int do_audio, 
			int do_video,
			int &total_streams,
			StreamMenuItem **array);
	int update_audio_streams(int total_streams);
	int update_video_streams(int total_streams);

// most recent loads
	int add_load(char *path);
	int init_loads(Defaults *defaults);
	int save_loads(Defaults *defaults);

// for previous document loader
	Load *load_file;
	LoadPrevious *load_previous[TOTAL_LOADS];
	BC_Menu *filemenu, *videomenu, *audiomenu;
	StreamMenuItem *astreams[256], *vstreams[256];
	int total_astreams, total_vstreams;
	MWindow *mwindow;
	int total_loads;
};

class Quit : public BC_MenuItem
{
public:
	Quit(MWindow *mwindow);
	~Quit();
	int handle_event();
	MWindow *mwindow;
};

class Load : public BC_MenuItem
{
public:
	Load(MWindow *mwindow);
	~Load();
	int handle_event();
	MWindow *mwindow;
	LoadThread *thread;
};

class LoadDVD : public BC_MenuItem
{
public:
	LoadDVD(MWindow *mwindow);
	~LoadDVD();
	int handle_event();
	MWindow *mwindow;
	LoadDVDThread *thread;
};

class SoftwareSync : public BC_MenuItem
{
public:
	SoftwareSync(MWindow *mwindow);
	~SoftwareSync();
	int handle_event();
	MWindow *mwindow;
};

class OriginalSize : public BC_MenuItem
{
public:
	OriginalSize(MWindow *mwindow);
	~OriginalSize();
	int handle_event();
	MWindow *mwindow;
};

class HalfSize : public BC_MenuItem
{
public:
	HalfSize(MWindow *mwindow);
	~HalfSize();
	int handle_event();
	MWindow *mwindow;
};

class DoubleSize : public BC_MenuItem
{
public:
	DoubleSize(MWindow *mwindow);
	~DoubleSize();
	int handle_event();
	MWindow *mwindow;
};

class SettingsMenu : public BC_MenuItem
{
public:
	SettingsMenu(MWindow *mwindow);
	~SettingsMenu();
	int handle_event();
	MWindow *mwindow;
	SettingsThread *thread;
};

class EveryFrame : public BC_MenuItem
{
public:
	EveryFrame(MWindow *mwindow);
	~EveryFrame();
	int handle_event();
	MWindow *mwindow;
};

class FullScreen : public BC_MenuItem
{
public:
	FullScreen(MWindow *mwindow);
	~FullScreen();
	int handle_event();
	MWindow *mwindow;
};

class LoadPrevious : public BC_MenuItem
{
public:
	LoadPrevious(Load *loadfile);
	int handle_event();
	void run();

	int set_path(char *path);

	Load *loadfile;
	char path[1024];
};

class StreamMenuItem : public BC_MenuItem
{
public:
	StreamMenuItem(MWindow *mwindow, 
		int stream_number, 
		char *text,
		int do_audio, 
		int do_video);
	~StreamMenuItem();
	int handle_event();
	MWindow *mwindow;
	int stream_number;
	int do_audio;
	int do_video;
};

class Close : public BC_MenuItem
{
public:
	Close(MWindow *mwindow);
	~Close();
	int handle_event();
	MWindow *mwindow;
};


#endif
