#ifndef MIXER_H
#define MIXER_H

#include "bcsignals.h"
#include "bchash.h"

#include <guicast.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <unistd.h>

#define MIXER "/dev/mixer"

#define MAXCHANNELS 4
#define DIVISION0 120
#define DIVISION1 40
#define DIVISION2 5
#define DIVISION3 40
#define DEFINEDPOTS 13

#define MIXER_X 110

#define MASTER_NUMBER 0
#define BASS_NUMBER 1
#define TREBLE_NUMBER 2
#define LINE_NUMBER 3
#define DSP_NUMBER 4
#define FM_NUMBER 5
#define CD_NUMBER 6
#define MIC_NUMBER 7
#define RECLEVEL_NUMBER 8
#define IGAIN_NUMBER 9
#define OGAIN_NUMBER 10
#define SPEAKER_NUMBER 11
#define PHONEOUT_NUMBER 12

template <class TYPE>
class MixerControl;
class MixerInputs;
class MixerSlider;
class MixerPot;
class MixerToggle;
class MixerLock;
class MixerDevice;
class MixerRefresh;
class MixerGUI;
class MixerMenu;
class Mixer;

class MixerResources : public BC_Signals
{
public:
	MixerResources(Mixer *mixer);
	void signal_handler(int signum);
	static Mixer *mixer;
};


class Mixer
{
public:
	Mixer(char *display, int channels);
	~Mixer();

// for startup
	int load_defaults();
	int save_defaults();
	int create_objects();
	int do_gui();
	int get_width();
	int get_height();
	int reread_parameters();
	int reconfigure_visible(int item, int new_value);

	int commandline_setting[DEFINEDPOTS];
	float values[DEFINEDPOTS][MAXCHANNELS];
	int visible[DEFINEDPOTS];
	static char *captions[DEFINEDPOTS];
	static char *default_headers[DEFINEDPOTS];
	static int device_parameters[DEFINEDPOTS];
	int channels;
	char *display;
	int line, mic, cd, lock;   // Toggles
	int x, y;
	BC_Hash *defaults;
	MixerDevice *device;
	MixerGUI *gui;
};

class MixerDevice
{
public:
	MixerDevice(Mixer *mixer);
	~MixerDevice();
	
	int read_parameters();
	int write_parameters(int visible_only, int commandline_only);

	Mixer *mixer;
};

class MixerGUI : public BC_Window
{
public:
	MixerGUI(Mixer *mixer, int x, int y, int w, int h);
	~MixerGUI();

	int create_objects();
	int create_controls();
	int close_event();
	int keypress_event();
	int button_press_event();
	int translation_event();

	int locked();

	MixerControl<MixerSlider> *master;
	MixerControl<MixerPot> *bass, *treble, *pots[DEFINEDPOTS];
	BC_Title *master_title, *bass_title, *treble_title, *titles[DEFINEDPOTS];
	MixerToggle *line, *mic, *cd;
	MixerRefresh *refresh;
	MixerLock *lock;
	Mixer *mixer;
	MixerMenu *menu;
};


template <class TYPE>
class MixerControl
{
public:
	MixerControl(Mixer *mixer, int parameter_number);
	~MixerControl();
	
	int add_control(TYPE* control);
	
	int update();        // store last_value and update everyone
	int change_value(int parameter_number, float offset, int omit_channel);           // adjust value of everyone by offset
	int reposition(int x);

	int parameter_number;
	int total_controls;
	Mixer *mixer;
	TYPE *controls[MAXCHANNELS];
};





class MixerSlider : public BC_PercentageSlider
{
public:
	MixerSlider(int x, int y, int w, int highlight, int color, Mixer *mixer, MixerControl<MixerSlider> *control, int channel);
	
	int handle_event();

	int channel;
	float last_value;
	
	Mixer *mixer;
	MixerControl<MixerSlider> *control;
};

class MixerPot : public BC_PercentagePot
{
public:
	MixerPot(int x, 
			int y, 
			VFrame **data, 
			Mixer *mixer, 
			MixerControl<MixerPot> *control, 
			int parameter_number, 
			int channel);

	int handle_event();

	int parameter_number;
	int channel;
	float last_value;

	Mixer *mixer;
	MixerControl<MixerPot> *control;
};

class MixerToggle : public BC_Radial
{
public:
	MixerToggle(int x, int y, int *output, Mixer *mixer, char *title);

	int handle_event();

	int *output;
	Mixer *mixer;
};



class MixerLock : public BC_CheckBox
{
public:
	MixerLock(int x, int y, Mixer *mixer);

	int handle_event();

	Mixer *mixer;
};


class MixerRefresh : public BC_Button
{
public:
	MixerRefresh(int x, int y, Mixer *mixer);
	int handle_event();
	
	Mixer *mixer;
};


class MixerMenu : public BC_PopupMenu
{
public:
	MixerMenu(Mixer *mixer);
	~MixerMenu();

	int create_objects();
	
	Mixer *mixer;
};

class MixerMenuItem : public BC_MenuItem
{
public:
	MixerMenuItem(Mixer *mixer, int item);
	~MixerMenuItem();

	int handle_event();
	
	Mixer *mixer;
	int item;
};

#endif
