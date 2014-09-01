#ifndef MIXER_H
#define MIXER_H

#include "audiodriver.inc"
#include "bcsignals.h"
#include "configure.inc"
#include "bchash.h"
#include "mixer.inc"
#include "mixergui.inc"
#include "mixertree.inc"
#include "theme.inc"

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
	Mixer(char *display);
	~Mixer();

// for startup
	void reset();
	int load_defaults();
	int save_defaults();
	int create_objects();
	int calculate_width();
	int calculate_height();
	int reread_parameters();
	int reconfigure_visible(int item, int new_value);
	void init_driver();
	void init_display();
	void configure();

	void read_hardware();
	void zero();
	void do_gui();

	int audio_driver;
	enum
	{
		AUDIO_NONE,
		AUDIO_OSS,
		AUDIO_ALSA
	};

	AudioDriver *audio;
	MixerTree *tree;
	int commandline_setting[DEFINEDPOTS];
	float values[DEFINEDPOTS][MAXCHANNELS];
	int visible[DEFINEDPOTS];
	static char *captions[DEFINEDPOTS];
	static char *default_headers[DEFINEDPOTS];
	static int device_parameters[DEFINEDPOTS];
	int channels;
	char *display;
// Toggles
	int lock_channels;
	int lock_elements;
// Window size
	int x, y, w, h;
// Position of upper left
	int x_position;
	int y_position;
	BC_Hash *defaults;
	MixerGUI *gui;
	ConfigureThread *configure_thread;
	Theme *theme;
};




#endif
