#ifndef CONFIGURE_H
#define CONFIGURE_H


#include "bcdialog.h"
#include "mixer.inc"
#include "mixertree.inc"

class ConfigureGUI;
class ConfigureCheckBox;
class ConfigureVScroll;
class ConfigureHScroll;

class ConfigureThread : public BC_DialogThread
{
public:
	ConfigureThread(Mixer *mixer);
	virtual ~ConfigureThread();
	
	void handle_close_event(int result);
	BC_Window* new_gui();
	
	Mixer *mixer;
	ConfigureGUI *gui;
	int x_position;
	int y_position;
	int control_h;
	int control_w;
	int w, h;
};

class ConfigureGUI : public BC_Window
{
public:
	ConfigureGUI(Mixer *mixer, 
		ConfigureThread *thread, 
		int x, 
		int y, 
		int w, 
		int h);
	void create_objects();
	int resize_event(int w, int h);
	int button_press_event();
	void reposition_controls(int new_x, int new_y);
	int cursor_motion_event();

	ArrayList<ConfigureCheckBox*> checkboxes;
	ConfigureVScroll *vscroll;
	ConfigureHScroll *hscroll;
	ConfigureThread *thread;
	Mixer *mixer;
	BC_SubWindow *control_window;
};

class ConfigureCheckBox : public BC_CheckBox
{
public:
	ConfigureCheckBox(Mixer *mixer, 
		int x, 
		int y, 
		char *text,
		MixerNode *node);
	
	int handle_event();
	Mixer *mixer;
	MixerNode *node;
};


class ConfigureVScroll : public BC_ScrollBar
{
public:
	ConfigureVScroll(Mixer *mixer,
		ConfigureThread *thread,
		int x, 
		int y, 
		int h);
	int handle_event();
	Mixer *mixer;
	ConfigureThread *thread;
};

class ConfigureHScroll : public BC_ScrollBar
{
public:
	ConfigureHScroll(Mixer *mixer,
		ConfigureThread *thread,
		int x, 
		int y, 
		int w);
	int handle_event();
	Mixer *mixer;
	ConfigureThread *thread;
};

#endif
