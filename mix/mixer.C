#include "keys.h"
#include "mixer.h"
#include "vframe.h"


Mixer* MixerResources::mixer = 0;

MixerResources::MixerResources(Mixer *mixer)
 : BC_Signals()
{
	this->mixer = mixer;
}

void MixerResources::signal_handler(int signum)
{
printf("MixerResources::signal_handler saving defaults\n");
	mixer->save_defaults();
	exit(0);
};

#include "theme/reload_dn_png.h"
#include "theme/reload_hi_png.h"
#include "theme/reload_up_png.h"

VFrame *reload_data[] = 
{
	new VFrame(reload_up_png),
	new VFrame(reload_hi_png),
	new VFrame(reload_dn_png)
};

#include "theme/pot_red_dn_png.h"
#include "theme/pot_red_hi_png.h"
#include "theme/pot_red_up_png.h"

static VFrame *pot_red_data[] = 
{
	new VFrame(pot_red_up_png),
	new VFrame(pot_red_hi_png),
	new VFrame(pot_red_dn_png),
};

#include "theme/pot_grey_dn_png.h"
#include "theme/pot_grey_hi_png.h"
#include "theme/pot_grey_up_png.h"

static VFrame *pot_grey_data[] = 
{
	new VFrame(pot_grey_up_png),
	new VFrame(pot_grey_hi_png),
	new VFrame(pot_grey_dn_png),
};

#include "theme/hslider_fg_up_png.h"
#include "theme/hslider_fg_hi_png.h"
#include "theme/hslider_fg_dn_png.h"
#include "theme/hslider_bg_up_png.h"
#include "theme/hslider_bg_hi_png.h"
#include "theme/hslider_bg_dn_png.h"
static VFrame *slider_data[] = 
{
	new VFrame(hslider_fg_up_png),
	new VFrame(hslider_fg_hi_png),
	new VFrame(hslider_fg_dn_png),
	new VFrame(hslider_bg_up_png),
	new VFrame(hslider_bg_hi_png),
	new VFrame(hslider_bg_dn_png)
};

char* Mixer::captions[] =
{
	"Master",
	"Bass",
	"Treble",
	"Line",
	"Dsp",
	"FM",
	"CD",
	"Mic",
	"Rec",
	"IGain",
	"OGain",
	"Speaker",
	"PhoneOut"
};

char* Mixer::default_headers[] = 
{ 
	"MASTER", 
	"BASS", 
	"TREBLE", 
	"LINE", 
	"DSP", 
	"FM", 
	"CD", 
	"MIC",
	"RECLEVEL",
	"IGAIN",
	"OGAIN",
	"SPEAKER",
	"PHONEOUT"
};

int Mixer::device_parameters[] = 
{ 
	SOUND_MIXER_VOLUME, 
	SOUND_MIXER_BASS, 
	SOUND_MIXER_TREBLE, 
	SOUND_MIXER_LINE, 
	SOUND_MIXER_PCM, 
	SOUND_MIXER_SYNTH, 
	SOUND_MIXER_CD, 
	SOUND_MIXER_MIC,
	SOUND_MIXER_RECLEV,
	SOUND_MIXER_IGAIN,
	SOUND_MIXER_OGAIN,
	SOUND_MIXER_SPEAKER,
	SOUND_MIXER_PHONEOUT
};

Mixer::Mixer(char *display, int channels)
{
	this->channels = channels;
	this->display = display;
}

Mixer::~Mixer()
{
}

int Mixer::create_objects()
{
	device = new MixerDevice(this);
	device->read_parameters();
	for(int i = 0; i < DEFINEDPOTS; i++)
	{
		commandline_setting[i] = 0;
	}
	load_defaults();
	return 0;
}

int Mixer::load_defaults()
{
	int i, j;
	char string[1024];

	defaults = new BC_Hash("~/.mix2000rc");
	defaults->load();

// load all values
	for(i = 0; i < DEFINEDPOTS; i++)
	{
		sprintf(string, "%s_VISIBLE", default_headers[i]);
		visible[i] = defaults->get(string, 1);

		for(j = 0; j < channels; j++)
		{
			if(visible[i])
			{
				sprintf(string, "%s_%d", default_headers[i], j);
				values[i][j] = defaults->get(string, values[i][j]);
			}
			else
			{
// Default the values to 0.
				values[i][j] = 0;
			}
		}
	}

// Load toggles.
	line = defaults->get("LINE", line);
	mic = defaults->get("MIC", mic);
	cd = defaults->get("CD", cd);
	lock = defaults->get("LOCK", 1);
	x = defaults->get("X", 0);
	y = defaults->get("Y", 0);
	return 0;
}

int Mixer::save_defaults()
{
	int i, j;
	char string[1024];

// load all values
	for(i = 0; i < DEFINEDPOTS; i++)
	{
		sprintf(string, "%s_VISIBLE", default_headers[i]);
		defaults->update(string, visible[i]);

		for(j = 0; j < channels; j++)
		{
			sprintf(string, "%s_%d", default_headers[i], j);
			defaults->update(string, values[i][j]);
		}
	}

// Load toggles.
	defaults->update("LINE", line);
	defaults->update("MIC", mic);
	defaults->update("CD", cd);
	defaults->update("LOCK", lock);
	defaults->update("X", x);
	defaults->update("Y", y);

	defaults->save();
	return 0;
}

int Mixer::get_width()
{
	int w = MIXER_X, i;
	for(i = 0; i < DEFINEDPOTS; i++)
	{
		switch(i)
		{
			case 0:
				if(visible[i]) w += DIVISION0;
				break;
			case 1:
				if(visible[i]) w += DIVISION1;
				break;
			case 2:
				if(visible[i]) w += DIVISION1;
				break;
			case 3:
				w += DIVISION2;
			default:
				if(visible[i]) w += DIVISION3;
				break;
		}
	}
	return w;
}

int Mixer::get_height()
{
	int h = 20 + pot_red_data[0]->get_h() * channels;
	if(h < 20 + pot_red_data[0]->get_h() * 2) 
		h = 20 + pot_red_data[0]->get_h() * 2;
	return h;
}




int Mixer::do_gui()
{
	int w, h;
	w = get_width();
	h = get_height();

	gui = new MixerGUI(this, x, y, w, h);
// OpenGL may do something funny in the constructor so the signals need to be
// set here.
	BC_Resources::get_signals()->initialize2();
	gui->create_objects();

	gui->run_window();
	delete gui;
	return 0;
}

int Mixer::reread_parameters()
{
	device->read_parameters();
	gui->line->update(line);
	gui->mic->update(mic);
	gui->cd->update(cd);
	if(visible[0])
		gui->master->update();
	if(visible[1])
		gui->bass->update();
	if(visible[2])
		gui->treble->update();
	for(int i = 3; i < DEFINEDPOTS; i++)
	{
		if(visible[i])
			gui->pots[i]->update();
	}
	return 0;
}

int Mixer::reconfigure_visible(int item, int new_value)
{
	visible[item] = new_value;
	gui->resize_window(get_width(), gui->get_h());
	gui->create_controls();
	gui->flush();

	return 0;
}




MixerDevice::MixerDevice(Mixer *mixer)
{
	this->mixer = mixer;
}

MixerDevice::~MixerDevice()
{
}

int MixerDevice::read_parameters()
{
	int output, i, j;
	int mixer_fd;
	char *result;
	char string[1024];

	if((mixer_fd = open(MIXER, O_RDWR, 0)) == -1)
	{
		perror("Open " MIXER); 
		return 1;
	}

	for(i = 0; i < DEFINEDPOTS; i++)
	{
		if(mixer->visible[i])
		{
			if(ioctl(mixer_fd, MIXER_READ(mixer->device_parameters[i]), &output) < 0)
			{
				sprintf(string, "MixerDevice::read_parameters: MIXER_READ %s", mixer->captions[i]);
				//perror(string);
			}

			result = (char*)&output;

			for(j = 0; j < mixer->channels; j++)
			{
				mixer->values[i][j] = result[j];  // intel only
			}
		}
	}

	ioctl(mixer_fd, SOUND_MIXER_READ_RECSRC, &output);
	mixer->line = (output & SOUND_MASK_LINE) / SOUND_MASK_LINE;
	mixer->mic = (output & SOUND_MASK_MIC) / SOUND_MASK_MIC;
	mixer->cd = (output & SOUND_MASK_CD) / SOUND_MASK_CD;

	close(mixer_fd);
}

int MixerDevice::write_parameters(int visible_only, int commandline_only)
{
	char *input;
	int output;
	int i, j;
	int mixer_fd;
	char string[1024];

	if((mixer_fd = open(MIXER, O_RDWR, 0)) == -1)
	{
		perror("Open " MIXER); 
		return 1;
	}

	for(i = 0; i < DEFINEDPOTS; i++)
	{
		if((!commandline_only || mixer->commandline_setting[i]) &&
			(!visible_only || mixer->visible[i]))
		{
			input = (char*)&output;
			for(j = 0; j < mixer->channels; j++)
			{
				input[j] = (int)mixer->values[i][j];
			}
			if(ioctl(mixer_fd, MIXER_WRITE(mixer->device_parameters[i]), &output) < 0)
			{
				sprintf(string, "MixerDevice::write_parameters: MIXER_WRITE %s", mixer->captions[i]);
				//perror(string);
			}
		}
	}

// Set inputs
	output = SOUND_MASK_LINE * mixer->line;
	output += SOUND_MASK_MIC * mixer->mic;
	output += SOUND_MASK_CD * mixer->cd;

	ioctl(mixer_fd, SOUND_MIXER_WRITE_RECSRC, &output);

	close(mixer_fd);
	return 0;
}




MixerGUI::MixerGUI(Mixer *mixer, int x, int y, int w, int h)
 : BC_Window("Mix 2000", 
 	x, 
	y, 
	w, 
	h, 
	MIXER_X, 
	h,
	0,
	0,
	1,
	BLOND)
{
	this->mixer = mixer;
	master = 0;
	bass = 0;
	treble = 0;
	for(int i = 0, j; i < DEFINEDPOTS; i++)
	{
		pots[i] = 0;
	}
}

MixerGUI::~MixerGUI()
{
}

int MixerGUI::create_objects()
{


// build permanent controls
	int y = 5, x = 5, i, j;
	add_subwindow(new BC_Title(x, y, "Input", MEDIUMFONT_3D, WHITE));

	y += 20;
	add_subwindow(line = new MixerToggle(x, y, &(mixer->line), mixer, "Line"));
	y += 20;
	add_subwindow(mic = new MixerToggle(x, y, &(mixer->mic), mixer, "Mic"));
	y += 20;
	add_subwindow(cd = new MixerToggle(x, y, &(mixer->cd), mixer, "CD"));

	x += 60;
	y = 5;
	add_subwindow(new BC_Title(x, y, "Lock", MEDIUMFONT_3D, WHITE));
	y += 20;
	add_subwindow(lock = new MixerLock(x + 5, y, mixer));
	y += 30;
	add_subwindow(new MixerRefresh(x, y, mixer));

	create_controls();

	add_subwindow(menu = new MixerMenu(mixer));
	menu->create_objects();
	show_window();
	return 0;
}

int MixerGUI::create_controls()
{
	int i, j, x = MIXER_X, y = 5;

	for(j = 0; j < DEFINEDPOTS; j++)
	{
		switch(j)
		{
			case 0:
				if(mixer->visible[0])
				{
					if(!master)
					{
						master = new MixerControl<MixerSlider>(mixer, MASTER_NUMBER);
						add_subwindow(master_title = new BC_Title(x, 5, "Master", SMALLFONT_3D, WHITE));

						for(i = 0, y = 18; i < mixer->channels; i++, y += pot_red_data[0]->get_h())
						{
							master->add_control(new MixerSlider(x, 
								y + 5, 
								DIVISION0 - 10, 
								LTGREY, 
								MEGREY, 
								mixer, 
								master, 
								i));
						}
					}
					else
					{
						master->reposition(x);
						master_title->reposition_window(x, master_title->get_y());
					}
					x += DIVISION0;
				}
				else
				if(master)
				{
					delete master;
					delete master_title;
					master = 0;
				}
				break;
			
			case 1:
				if(mixer->visible[1])
				{
					if(!bass)
					{
						bass = new MixerControl<MixerPot>(mixer, BASS_NUMBER);
						add_subwindow(bass_title = new BC_Title(x, 5, "Bass", SMALLFONT_3D, WHITE));

						for(i = 0, y = 18; i < mixer->channels; i++, y += pot_red_data[0]->get_h())
						{
							bass->add_control(new MixerPot(x, 
								y, 
								pot_red_data, 
								mixer, 
								bass, 
								BASS_NUMBER, 
								i));
						}
					}
					else
					{
						bass->reposition(x);
						bass_title->reposition_window(x, bass_title->get_y());
					}
					x += DIVISION1;
				}
				else
				if(bass)
				{
					delete bass;
					delete bass_title;
					bass = 0;
				}
				break;

			case 2:
				if(mixer->visible[2])
				{
					if(!treble)
					{
						treble = new MixerControl<MixerPot>(mixer, TREBLE_NUMBER);
						add_subwindow(treble_title = new BC_Title(x, 5, "Treble", SMALLFONT_3D, WHITE));
						for(i = 0, y = 18; i < mixer->channels; i++, y += pot_red_data[0]->get_h())
						{
							treble->add_control(new MixerPot(x, 
								y, 
								pot_red_data, 
								mixer, 
								treble, 
								TREBLE_NUMBER, 
								i));
						}
					}
					else
					{
						treble->reposition(x);
						treble_title->reposition_window(x, treble_title->get_y());
					}
					x += DIVISION1;
				}
				else
				if(treble)
				{
					delete treble;
					delete treble_title;
					treble = 0;
				}
				break;
			
			case 3:
				x += DIVISION2;
			default:
				if(mixer->visible[j])
				{
					if(!pots[j])
					{
						pots[j] = new MixerControl<MixerPot>(mixer, j);
						add_subwindow(titles[j] = new BC_Title(x, 5, mixer->captions[j], SMALLFONT_3D, WHITE));
						for(i = 0, y = 18; i < mixer->channels; i++, y += pot_red_data[0]->get_h())
						{
							pots[j]->add_control(new MixerPot(x, 
								y, 
								pot_grey_data, 
								mixer, 
								pots[j], 
								j, 
								i));
						}
					}
					else
					{
						pots[j]->reposition(x);
						titles[j]->reposition_window(x, titles[j]->get_y());
					}
					x += DIVISION3;
				}
				else
				if(pots[j])
				{
					delete pots[j];
					delete titles[j];
					pots[j] = 0;
				}
				break;
		}
	}
	return 0;
}


int MixerGUI::close_event()
{
	set_done(0);
	return 1;
}

int MixerGUI::keypress_event()
{
	switch(get_keypress())
	{
		case 'q':
			set_done(0);
			return 1;
			break;
		case ESC:
			set_done(0);
			return 1;
			break;
	}
	return 0;
}



int MixerGUI::button_press_event()
{
	if(get_buttonpress() == 3)
	{
		menu->activate_menu();
		return 1;
	}
	return 0;
}

int MixerGUI::translation_event()
{
	mixer->x = get_x();
	mixer->y = get_y();
	return 0;
}












template <class TYPE>
MixerControl<TYPE>::MixerControl(Mixer *mixer, int parameter_number)
{
	this->mixer = mixer;
	this->parameter_number = parameter_number;
	total_controls = 0;
}

template <class TYPE>
MixerControl<TYPE>::~MixerControl()
{
	for(int i = 0; i < total_controls; i++)
	{
		delete controls[i];
	}
}

template <class TYPE>
int MixerControl<TYPE>::add_control(TYPE* control)
{
	controls[total_controls++] = control;
	mixer->gui->add_subwindow(control);
	return 0;
}

template <class TYPE>
int MixerControl<TYPE>::reposition(int x)
{
	for(int i = 0; i < total_controls; i++)
		controls[i]->reposition_window(x, controls[i]->get_y());
	return 0;
}

template <class TYPE>
int MixerControl<TYPE>::change_value(int parameter_number, 
	float offset, 
	int omit_channel)
{
	float new_value;
	int channel;

// fix controls
	for(channel = 0; channel < total_controls; channel++)
	{
		if(channel != omit_channel)
		{
			controls[channel]->last_value = controls[channel]->get_value();
			new_value = controls[channel]->get_value() + offset;

			if(new_value < 0 ) 
				new_value = 0; 
			else 
			if(new_value > 100) 
				new_value = 100;

			controls[channel]->update(new_value);
			controls[channel]->last_value = new_value;
			mixer->values[parameter_number][channel] = new_value;
		}
	}
	return 0;
}

template <class TYPE>
int MixerControl<TYPE>::update()
{
	int channel;
	
	for(channel = 0; channel < total_controls; channel++)
	{
		controls[channel]->update(mixer->values[parameter_number][channel]);
		controls[channel]->last_value = mixer->values[parameter_number][channel];
	}
	return 0;
}









MixerSlider::MixerSlider(int x, int y, int w, 
	int highlight, 
	int color, 
	Mixer *mixer, 
	MixerControl<MixerSlider> *control, 
	int channel)
 : BC_PercentageSlider(x, 
 	y, 
	0,
	w, 
	200, 
	0, 
	100, 
	mixer->values[MASTER_NUMBER][channel],
	0,
	slider_data)
{
	this->mixer = mixer;
	this->control = control;
	this->channel = channel;
	this->last_value = mixer->values[MASTER_NUMBER][channel];
}


int MixerSlider::handle_event()
{
	if(mixer->lock)
	{
//printf("MixerSlider::handle_event %f\n", get_value() - last_value);
		control->change_value(MASTER_NUMBER, 
			get_value() - last_value, 
			channel);
	}
	last_value = mixer->values[MASTER_NUMBER][channel] = get_value();
	mixer->device->write_parameters(1, 0);
	return 1;
}




MixerPot::MixerPot(int x, 
			int y, 
			VFrame **data, 
			Mixer *mixer, 
			MixerControl<MixerPot> *control, 
			int parameter_number, 
			int channel)
 : BC_PercentagePot(x, 
 	y, 
	mixer->values[parameter_number][channel], 
	0, 
	100,
	data)
{
	this->mixer = mixer;
	this->control = control;
	this->channel = channel;
	this->parameter_number = parameter_number;
	this->last_value = mixer->values[parameter_number][channel];
}


int MixerPot::handle_event()
{
	if(mixer->lock)
	{
//printf("MixerPot::handle_event %f\n", get_value() - last_value);
		control->change_value(parameter_number, 
			get_value() - last_value, channel);
	}
	last_value = mixer->values[parameter_number][channel] = get_value();
	mixer->device->write_parameters(1, 0);
	return 1;
}






MixerToggle::MixerToggle(int x, int y, int *output, Mixer *mixer, char *title)
 : BC_Radial(x, y, *output, title, MEDIUMFONT_3D, WHITE)
{
	this->mixer = mixer;
	this->output = output;
}

int MixerToggle::handle_event()
{
	*output = get_value();
	mixer->device->write_parameters(1, 0);
	return 1;
}





MixerLock::MixerLock(int x, int y, Mixer *mixer)
 : BC_CheckBox(x, y, mixer->lock)
{
	this->mixer = mixer; 
}

int MixerLock::handle_event()
{
	mixer->lock = get_value();
	return 1;
}


MixerRefresh::MixerRefresh(int x, int y, Mixer *mixer)
 : BC_Button(x, y, reload_data)
{
	this->mixer = mixer;
	set_tooltip("Read settings");
}

int MixerRefresh::handle_event()
{
	mixer->reread_parameters();
	return 1;
}




MixerMenu::MixerMenu(Mixer *mixer)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mixer = mixer;
}

MixerMenu::~MixerMenu()
{
}

int MixerMenu::create_objects()
{
	for(int i = 0; i < DEFINEDPOTS; i++)
	{
		add_item(new MixerMenuItem(mixer, i));
	}
	return 0;
}


MixerMenuItem::MixerMenuItem(Mixer *mixer, int item)
 : BC_MenuItem(Mixer::captions[item])
{
	this->mixer = mixer;
	this->item = item;
	if(mixer->visible[item]) set_checked(1);
}

MixerMenuItem::~MixerMenuItem()
{
}

int MixerMenuItem::handle_event()
{
	set_checked(!get_checked());
	mixer->reconfigure_visible(item, get_checked());
	return 1;
}
