#ifndef MWINDOW_H
#define MWINDOW_H

#include "arraylist.h"
#include "settings.inc"
#include "asset.inc"
#include "defaults.inc"
#include "file.inc"
#include "mainmenu.inc"
#include "mwindow.inc"
#include "mutex.h"
#include "mwindowgui.inc"
#include "playbackengine.inc"
#include "playbackscroll.inc"
#include "playlist.inc"
#include "theme.inc"
#include "thread.h"
#include "vframe.inc"

#include <stdint.h>

class Load;
class LoadPrevious;
class LoadThread;


class MWindow
{
public:
	MWindow(ArrayList<char*> *init_playlist);
	~MWindow();

	void load_defaults();
	void save_defaults();
	int create_objects();
	int run_program();
	int quit();
	int reset_parameters();
	int get_canvas_sizes(int width_given);
	int get_full_size(int &full_w, int &full_h);
	int get_cropping(int &y1, int &y2);
	int set_audio_stream(int stream_number);
	int set_video_stream(int stream_number);
	float get_aspect_ratio();
	int calculate_smp();

// =================== all operations start here
	int load_file(char *path, int use_locking);
	int close_file();
	int original_size(float percent);

	int every_frame;
	int smp;
	int use_mmx;
	int use_deblocking;
	int software_sync;
	int64_t prebuffer_size;
// Streams to read from a multi-stream file
	int video_stream;
	int audio_stream;

	ErrorThread *error_thread;
	Defaults *defaults;
	ArrayList<char*> *init_playlist;
	MWindowGUI *gui;
	Asset *asset;   // description of file or 0 if no file loaded
// 0 when no file loaded otherwise pointer to open file
	File *video_file;
	File *audio_file;
	Playlist *playlist;
	PlaybackEngine *engine;
	PlaybackScroll *playback_scroll;
	SettingsMenu *settingsmenu;
	char default_path[1024];
	int fullscreen;
// Current position must be stored here since the position in playback_engine
// is after the frame is rendered.
	double current_position;
// Window dimensions
	int mwindow_x, mwindow_y, mwindow_w, mwindow_h;
// Aspect ratio of display
	float aspect_w, aspect_h;   
// Letterbox aspect ratio
	float letter_w, letter_h;   
// Size of source to read
	int input_w, input_h;     
// The frame is expanded to the size of the display aspect ratio
// then cropped to the size of the letterbox aspect ratio.
	int square_pixels; // Ignore aspect ratio if 1
	int crop_letterbox;   // Crop letterbox if 1
	int convert_601;     // Convert luminance
	int audio_priority;
	int mix_strategy;
	int video_device;
	float actual_framerate; // Actual framerate being achieved.
	Theme *theme;

private:
// Default buffer for frame flashes
	VFrame *frame;
};

#endif
