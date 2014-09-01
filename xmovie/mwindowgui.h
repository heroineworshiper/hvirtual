#ifndef MWINDOWGUI_H
#define MWINDOWGUI_H

#include "guicast.h"
#include "mainmenu.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"


#ifdef HAVE_DVS
#ifdef __cplusplus
extern "C" {
#endif

#include "dvs_clib.h"
#include "dvs_fifo.h"

#ifdef __cplusplus
}
#endif
#endif

#define DEFAULTW  320
#define DEFAULTH  70
#define SCROLLBAR_LENGTH 1000

#define TOTAL_VDEVICES 3
#define VDEVICE_X11 0
#define VDEVICE_DVS 1
#define VDEVICE_XF86VM 2

class ErrorThread : public Thread
{
public:
	ErrorThread(MWindowGUI *gui);
	~ErrorThread();

	int show_error(char *text);
	void run();
	Mutex startup_lock;
	char text[BCTEXTLEN];
	MWindowGUI *gui;
	int x, y;
};

class MWindowGUI : public BC_Window
{
public:
	MWindowGUI(MWindow *mwindow, int x, int y, int w, int h);
	~MWindowGUI();

	int create_objects();
	void create_canvas();
	int close_event();
	void delete_output_frame();
	void bitmap_dimensions(int &w, int &h);
	void reset();
// Use size of canvas from loaded movie file.
	int resize_canvas(int w, int h);
// Use size of window from resize event.
	int resize_event(int w, int h);
	int translation_event();
// Fix size of scrollbar for movie length and position.
	int resize_scrollbar();
// Make scrollbar reflect current position
	int update_position(double percentage, double seconds, int use_scrollbar);
// Initialize for video
	int start_video();
	int stop_video();
	void start_dvs();
	void start_x11();
	void stop_dvs();
	void stop_x11();
	VFrame *get_output_frame();
	VFrame* get_output_frame_x11();
	VFrame* get_output_frame_dvs();
	void write_output();
	void write_x11();
	void write_dvs();
	static char* vdevice_to_text(int number);
	static int text_to_vdevice(char *text);
	static void decode_sign_change(int w, int h, unsigned char **row_pointers);

	MWindow *mwindow;
	MainMenu *menu;
// Canvas is either a subwindow or a popup if fullscreen
	BC_WindowBase *canvas;
	MainScrollbar *scrollbar;
	PlayButton *playbutton;
	FrameBackButton *backbutton;
	FrameForwardButton *forwardbutton;
	BC_Title *time_title;
	int playback_colormodel;

	int window_resized;
	int resize_widgets();
	VFrame *output_frame;
	BC_Bitmap *primary_surface;
// VDevice started in the last start_video or -1 for none
	int active_vdevice;

#ifdef HAVE_DVS
	sv_handle *dvs_output;
	sv_fifo *pfifo;
	sv_fifo_info status;
	sv_fifo_buffer *pbuffer;
#endif
	VFrame *dvs_frame;
};


class MainCanvasSubWindow : public BC_SubWindow
{
public:
	MainCanvasSubWindow(MWindow *mwindow, int x, int y, int w, int h);
	~MainCanvasSubWindow();

	MWindow *mwindow;
};

class MainCanvasPopup : public BC_Popup
{
public:
	MainCanvasPopup(MWindow *mwindow, int x, int y, int w, int h);
	~MainCanvasPopup();

    MWindow *mwindow;
};

class MainCanvasFullScreen : public BC_FullScreen
{
public:
    MainCanvasFullScreen(MWindow *mwindow, int w, int h, int vm_scale);
    ~MainCanvasFullScreen();

	MWindow *mwindow;
};

class MainScrollbar : public BC_PercentageSlider
{
public:
	MainScrollbar(MWindow *mwindow, int x, int y, int w);
	~MainScrollbar();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

class PlayButton : public BC_Button
{
public:
	PlayButton(MWindow *mwindow, int x, int y);
	~PlayButton();
	int handle_event();
	int keypress_event();
	int set_mode(int mode);

	MWindow *mwindow;
	int mode;
};

class FrameBackButton : public BC_Button
{
public:
	FrameBackButton(MWindow *mwindow, int x, int y);
	~FrameBackButton();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

class FrameForwardButton : public BC_Button
{
public:
	FrameForwardButton(MWindow *mwindow, int x, int y);
	~FrameForwardButton();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};




#endif
