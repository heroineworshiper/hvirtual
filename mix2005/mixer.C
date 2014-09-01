#include "audioalsa.h"
#include "audiodriver.h"
#include "audiooss.h"
#include "clip.h"
#include "configure.h"
#include "keys.h"
#include "mixer.h"
#include "mixergui.h"
#include "mixertree.h"
#include "theme.h"
#include "vframe.h"


Mixer* MixerResources::mixer = 0;

MixerResources::MixerResources(Mixer *mixer)
 : BC_Signals()
{
	this->mixer = mixer;
}

void MixerResources::signal_handler(int signum)
{
	mixer->save_defaults();
	exit(0);
}

Mixer::Mixer(char *display)
{
	reset();
	defaults = new BC_Hash("~/.mix2005rc");
	defaults->load();
	this->channels = channels;
	this->display = display;
}

Mixer::~Mixer()
{
}

void Mixer::reset()
{
	audio = 0;
	tree = 0;
	defaults = 0;
}

int Mixer::create_objects()
{

	init_driver();
	load_defaults();
	theme = new Theme(this);
	theme->initialize();
	save_defaults();
	audio->write_parameters();
	init_display();
	return 0;
}

void Mixer::read_hardware()
{
	audio->read_parameters();
	gui->lock_window("Mixer::read_hardware");
	gui->update_values();
	gui->unlock_window();
}

void Mixer::configure()
{
	gui->unlock_window();
	configure_thread->start();
	gui->lock_window("Mixer::configure");
}

void Mixer::zero()
{
}

void Mixer::do_gui()
{
	gui->run_window();
	delete gui;
}


void Mixer::init_driver()
{
	if(audio)
	{
		delete audio;
		audio = 0;
	}

	if(tree)
	{
		delete tree;
		tree = 0;
	}

	tree = new MixerTree(this);
	audio = new AudioALSA(this);
	audio->initialize();
}

void Mixer::init_display()
{
	configure_thread = new ConfigureThread(this);
	gui = new MixerGUI(this, x, y, w, h);
	gui->create_objects();
}


int Mixer::load_defaults()
{
	int i, j;

	tree->load_defaults(defaults);

	x = defaults->get("X", 0);
	y = defaults->get("Y", 0);
	w = defaults->get("W", 640);
	h = defaults->get("H", 480);
	x_position = defaults->get("X_POSITION", 0);
	y_position = defaults->get("Y_POSITION", 0);
	lock_channels = defaults->get("LOCK_CHANNELS", 1);
	lock_elements = defaults->get("LOCK_ELEMENTS", 0);
	
	CLAMP(w, 50, 4096);
	CLAMP(h, 50, 4096);
	return 0;
}

int Mixer::save_defaults()
{
	tree->save_defaults(defaults);

	defaults->update("X", x);
	defaults->update("Y", y);
	defaults->update("W", w);
	defaults->update("H", h);
	defaults->update("X_POSITION", x_position);
	defaults->update("Y_POSITION", y_position);
	defaults->update("LOCK_CHANNELS", lock_channels);
	defaults->update("LOCK_ELEMENTS", lock_elements);
	defaults->update("BRIGHT_THEME", theme->is_bright);

	defaults->save();
	return 0;
}









