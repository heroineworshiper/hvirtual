/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

#include "adeviceprefs.h"
#include "audioalsa.h"
#include "audiodevice.inc"
#include "bcsignals.h"
#include "bitspopup.h"
#include "edl.h"
#include "language.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "recordconfig.h"
#include "theme.h"
#include <string.h>

//#define DEVICE_H 50

ADevicePrefs::ADevicePrefs(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PreferencesDialog *dialog, 
	AudioOutConfig *out_config, 
	AudioInConfig *in_config, 
	int mode)
{
	reset();
	this->pwindow = pwindow;
	this->dialog = dialog;
	this->driver = -1;
	this->mode = mode;
	this->out_config = out_config;
	this->in_config = in_config;
	this->x = x;
	this->y = y;
}

ADevicePrefs::~ADevicePrefs()
{
	delete_objects();
	if(menu) delete menu;
}

void ADevicePrefs::reset()
{
	menu = 0;
	firewire_path = 0;
	firewire_syt = 0;
    channel_title = 0;
    firewire_port = 0;
    firewire_channel = 0;
	syt_title = 0;
	path_title = 0;

	alsa_drivers = 0;
	path_title = 0;
	bits_title = 0;
	alsa_device = 0;
	alsa_bits = 0;
	alsa_workaround = 0;

	oss_bits = 0;


	oss_path = 0;

	cine_bits = 0;
	cine_path = 0;
    
    server_title = 0;
    server = 0;
    port_title = 0;
    esound_port = 0;
}

int ADevicePrefs::initialize(int creation)
{
	int *driver;
	delete_objects();

	switch(mode)
	{
		case MODEPLAY:
			driver = &out_config->driver;
			break;
		case MODERECORD:
			driver = &in_config->driver;
			break;
		case MODEDUPLEX:
			driver = &out_config->driver;
			break;
	}
	this->driver = *driver;


	if(!menu)
	{
		dialog->add_subwindow(menu = new ADriverMenu(x, 
			y, 
			this, 
			(mode == MODERECORD),
			driver));
		menu->create_objects();
	}

	switch(*driver)
	{
		case AUDIO_OSS:
		case AUDIO_OSS_ENVY24:
			create_oss_objs();
			break;
		case AUDIO_ALSA:
			create_alsa_objs();
			break;
		case AUDIO_ESOUND:
			create_esound_objs();
			break;
		case AUDIO_1394:
		case AUDIO_DV1394:
		case AUDIO_IEC61883:
			create_firewire_objs();
			break;
		case AUDIO_CINE:
			create_cine_objs();
			break;
        case AUDIO_PULSE:
            create_pulse_objs();
            break;
	}
	
	return 0;
}

int ADevicePrefs::get_h(int recording)
{
	int margin = pwindow->mwindow->theme->widget_border;
	int result = BC_Title::calculate_h(dialog, 
			"XXX", 
			MEDIUMFONT) + 
		margin +
		BC_TextBox::calculate_h(dialog, 
			MEDIUMFONT, 
			1, 
			1);
	if(!recording)
	{
		result += BC_CheckBox::calculate_h(dialog) + margin;
	}

	return result;
}

int ADevicePrefs::delete_objects()
{
	switch(driver)
	{
		case AUDIO_OSS:
		case AUDIO_OSS_ENVY24:
			delete_oss_objs();
			break;
		case AUDIO_ALSA:
			delete_alsa_objs();
			break;
		case AUDIO_ESOUND:
			delete_esound_objs();
			break;
		case AUDIO_1394:
		case AUDIO_DV1394:
		case AUDIO_IEC61883:
			delete_firewire_objs();
			break;
        case AUDIO_PULSE:
            delete_pulse_objs();
            break;
	}

	delete cine_bits;
	delete cine_path;

	reset();
	driver = -1;
	return 0;
}

int ADevicePrefs::delete_oss_objs()
{
	delete path_title;
 	delete bits_title;
 	delete oss_bits;
	delete oss_path;
	return 0;
}

int ADevicePrefs::delete_esound_objs()
{
	delete server_title;
	delete port_title;
	delete server;
	delete esound_port;
	return 0;
}

int ADevicePrefs::delete_firewire_objs()
{
	delete port_title;
	delete channel_title;
	delete firewire_port;
	delete firewire_channel;
	if(firewire_path)
	{
		delete path_title;
		delete firewire_path;
	}
	firewire_path = 0;
	if(firewire_syt)
	{
		delete firewire_syt;
		delete syt_title;
	}
	firewire_syt = 0;
	return 0;
}

int ADevicePrefs::delete_alsa_objs()
{
#ifdef HAVE_ALSA
	if(alsa_drivers) alsa_drivers->remove_all_objects();
	delete alsa_drivers;
	delete path_title;
	delete bits_title;
	delete alsa_device;
	delete alsa_bits;
	delete alsa_workaround;
#endif
	return 0;
}


int ADevicePrefs::create_oss_objs()
{
	char *output_char;
	int *output_int;
	int margin = pwindow->mwindow->theme->widget_border;
	int y1 = y;
	BC_Resources *resources = BC_WindowBase::get_resources();

	int x1 = x + menu->get_w() + margin;
	switch(mode)
	{
		case MODEPLAY: 
//				output_int = &out_config->oss_enable;
			break;
		case MODERECORD:
//				output_int = &in_config->oss_enable;
			break;
		case MODEDUPLEX:
//				output_int = &out_config->oss_enable;
			break;
	}
//		dialog->add_subwindow(oss_enable = new OSSEnable(x1, y1 + 20, output_int));
//		x1 += oss_enable->get_w() + margin;
	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->oss_out_device;
			break;
		case MODERECORD:
			output_char = in_config->oss_in_device;
			break;
		case MODEDUPLEX:
			output_char = out_config->oss_out_device;
			break;
	}
	dialog->add_subwindow(path_title = new BC_Title(x1, 
		y, 
		_("Device path:"), 
		MEDIUMFONT, 
		resources->text_default));
	dialog->add_subwindow(oss_path = new ADeviceTextBox(x1, 
		y1 + path_title->get_h() + margin, 
		output_char));

	x1 += oss_path->get_w() + margin;
	switch(mode)
	{
		case MODEPLAY: 
			output_int = &out_config->oss_out_bits;
			break;
		case MODERECORD:
			output_int = &in_config->oss_in_bits;
			break;
		case MODEDUPLEX:
			output_int = &out_config->oss_out_bits;
			break;
	}
	dialog->add_subwindow(bits_title = new BC_Title(x1, y, _("Bits:"), MEDIUMFONT, resources->text_default));
	oss_bits = new BitsPopup(dialog, 
		x1, 
		y1 + bits_title->get_h() + margin, 
		output_int, 
		0, 
		0, 
		0,
		0,
		1);
	oss_bits->create_objects();

	x1 += oss_bits->get_w() + margin;
//		y1 += DEVICE_H;

	return 0;
}

int ADevicePrefs::create_alsa_objs()
{
#ifdef HAVE_ALSA
	char *output_char;
	int *output_int;
	int margin = pwindow->mwindow->theme->widget_border;
	BC_Resources *resources = BC_WindowBase::get_resources();

	int x1 = x + menu->get_w() + margin;
	int y1 = y;

	ArrayList<char*> *alsa_titles = new ArrayList<char*>;
	AudioALSA::list_devices(alsa_titles, 0, mode);


	alsa_drivers = new ArrayList<BC_ListBoxItem*>;
	for(int i = 0; i < alsa_titles->total; i++)
		alsa_drivers->append(new BC_ListBoxItem(alsa_titles->values[i]));
	alsa_titles->remove_all_objects();
	delete alsa_titles;

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->alsa_out_device;
			break;
		case MODERECORD:
			output_char = in_config->alsa_in_device;
			break;
		case MODEDUPLEX:
			output_char = out_config->alsa_out_device;
			break;
	}

	dialog->add_subwindow(path_title = new BC_Title(x1, y, _("Device:"), MEDIUMFONT, resources->text_default));
	y1 += path_title->get_h() + margin;
	alsa_device = new ALSADevice(dialog,
		x1, 
		y1, 
		output_char,
		alsa_drivers);
	alsa_device->create_objects();
	int x2 = x1;

	x1 += alsa_device->get_w() + 5;
	switch(mode)
	{
		case MODEPLAY: 
			output_int = &out_config->alsa_out_bits;
			break;
		case MODERECORD:
			output_int = &in_config->alsa_in_bits;
			break;
		case MODEDUPLEX:
			output_int = &out_config->alsa_out_bits;
			break;
	}
	
	dialog->add_subwindow(bits_title = new BC_Title(x1, y, _("Bits:"), MEDIUMFONT, resources->text_default));
	y1 = y + bits_title->get_h() + margin;
	alsa_bits = new BitsPopup(dialog, 
		x1, 
		y1, 
		output_int, 
		0, 
		0, 
		0,
		0,
		1);
	alsa_bits->create_objects();

	y1 += alsa_bits->get_h();
	x1 = x2;

	if(mode == MODEPLAY)
	{
		dialog->add_subwindow(alsa_workaround = 
			new BC_CheckBox(x1, 
				y1, 
				&out_config->interrupt_workaround,
				_("Stop playback locks up.")));
	}

#endif

	return 0;
}

int ADevicePrefs::create_esound_objs()
{
	int x1 = x + menu->get_w() + 5;
	int margin = pwindow->mwindow->theme->widget_border;
	char *output_char;
	int *output_int;
	BC_Resources *resources = BC_WindowBase::get_resources();

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->esound_out_server;
			break;
		case MODERECORD:
			output_char = in_config->esound_in_server;
			break;
		case MODEDUPLEX:
			output_char = out_config->esound_out_server;
			break;
	}
	dialog->add_subwindow(server_title = new BC_Title(x1, y, _("Server:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(server = new ADeviceTextBox(x1, y + DP(20), output_char));

	switch(mode)
	{
		case MODEPLAY: 
			output_int = &out_config->esound_out_port;
			break;
		case MODERECORD:
			output_int = &in_config->esound_in_port;
			break;
		case MODEDUPLEX:
			output_int = &out_config->esound_out_port;
			break;
	}
	x1 += server->get_w() + margin;
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(esound_port = new ADeviceIntBox(x1, y + DP(20), output_int));
	return 0;
}

int ADevicePrefs::create_firewire_objs()
{
	int margin = pwindow->mwindow->theme->widget_border;
	int x1 = x + menu->get_w() + margin;
	int *output_int = 0;
	char *output_char = 0;
	BC_Resources *resources = BC_WindowBase::get_resources();

// Firewire path
	switch(mode)
	{
		case MODEPLAY:
			if(driver == AUDIO_DV1394)
				output_char = out_config->dv1394_path;
			else
			if(driver == AUDIO_1394)
				output_char = out_config->firewire_path;
			break;
// Our version of raw1394 doesn't support changing the path
		case MODERECORD:
			output_char = 0;
			break;
	}

	if(output_char)
	{
		dialog->add_subwindow(path_title = new BC_Title(x1, y, _("Device Path:"), MEDIUMFONT, resources->text_default));
		dialog->add_subwindow(firewire_path = new ADeviceTextBox(x1, y + DP(20), output_char));
		x1 += firewire_path->get_w() + margin;
	}

// Firewire port
	switch(mode)
	{
		case MODEPLAY: 
			if(driver == AUDIO_DV1394)
				output_int = &out_config->dv1394_port;
			else
				output_int = &out_config->firewire_port;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_port;
			break;
		case MODEDUPLEX:
//			output_int = &out_config->afirewire_out_port;
			break;
	}
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(firewire_port = new ADeviceIntBox(x1, y + DP(20), output_int));

	x1 += firewire_port->get_w() + margin;

// Firewire channel
	switch(mode)
	{
		case MODEPLAY: 
			if(driver == AUDIO_DV1394)
				output_int = &out_config->dv1394_channel;
			else
				output_int = &out_config->firewire_channel;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_channel;
			break;
	}
	dialog->add_subwindow(channel_title = new BC_Title(x1, y, _("Channel:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(firewire_channel = new ADeviceIntBox(x1, y + DP(20), output_int));
	x1 += firewire_channel->get_w() + margin;

// Syt offset
	switch(mode)
	{
		case MODEPLAY:
			if(driver == AUDIO_DV1394)
				output_int = &out_config->dv1394_syt;
			else
			if(driver == AUDIO_1394)
				output_int = &out_config->firewire_syt;
			else
				output_int = 0;
			break;
		case MODERECORD:
			output_int = 0;
			break;
	}

	if(output_int)
	{
		dialog->add_subwindow(syt_title = new BC_Title(x1, y, _("Syt Offset:"), MEDIUMFONT, resources->text_default));
		dialog->add_subwindow(firewire_syt = new ADeviceIntBox(x1, y + DP(20), output_int));
		x1 += firewire_syt->get_w() + margin;
	}

	return 0;
}



int ADevicePrefs::create_cine_objs()
{
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
    return 0;
	
}


void ADevicePrefs::create_pulse_objs()
{
	int x1 = x + menu->get_w() + 5;
	int margin = pwindow->mwindow->theme->widget_border;
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->pulse_out_server;
			break;
		case MODERECORD:
			output_char = in_config->pulse_in_server;
			break;
	}
	dialog->add_subwindow(server_title = new BC_Title(x1, y, _("Server (blank for default):"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(server = new ADeviceTextBox(x1, y + DP(20), output_char));
}

void ADevicePrefs::delete_pulse_objs()
{
    delete server_title;
    delete server;
}



ADriverMenu::ADriverMenu(int x, 
	int y, 
	ADevicePrefs *device_prefs, 
	int do_input,
	int *output)
 : BC_PopupMenu(x, y, DP(150), adriver_to_string(*output), 1)
{
	this->output = output;
	this->do_input = do_input;
	this->device_prefs = device_prefs;
}

ADriverMenu::~ADriverMenu()
{
}

void ADriverMenu::create_objects()
{
	add_item(new ADriverItem(this, AUDIO_OSS_TITLE, AUDIO_OSS));
	add_item(new ADriverItem(this, AUDIO_OSS_ENVY24_TITLE, AUDIO_OSS_ENVY24));

#ifdef HAVE_ALSA
	add_item(new ADriverItem(this, AUDIO_ALSA_TITLE, AUDIO_ALSA));
#endif

#ifdef HAVE_PULSE
	add_item(new ADriverItem(this, AUDIO_PULSE_TITLE, AUDIO_PULSE));
#endif

	/* if(!do_input) */
    {
        add_item(new ADriverItem(this, AUDIO_ESOUND_TITLE, AUDIO_ESOUND));
    }
//	add_item(new ADriverItem(this, AUDIO_NAS_TITLE, AUDIO_NAS));
	if(!do_input) add_item(new ADriverItem(this, AUDIO_1394_TITLE, AUDIO_1394));
	add_item(new ADriverItem(this, AUDIO_DV1394_TITLE, AUDIO_DV1394));
	add_item(new ADriverItem(this, AUDIO_IEC61883_TITLE, AUDIO_IEC61883));
//	add_item(new ADriverItem(this, AUDIO_DVB_TITLE, AUDIO_DVB));
}

char* ADriverMenu::adriver_to_string(int driver)
{
	switch(driver)
	{
		case AUDIO_OSS:
			sprintf(string, AUDIO_OSS_TITLE);
			break;
		case AUDIO_OSS_ENVY24:
			sprintf(string, AUDIO_OSS_ENVY24_TITLE);
			break;
		case AUDIO_ESOUND:
			sprintf(string, AUDIO_ESOUND_TITLE);
			break;
		case AUDIO_NAS:
			sprintf(string, AUDIO_NAS_TITLE);
			break;
		case AUDIO_ALSA:
			sprintf(string, AUDIO_ALSA_TITLE);
			break;
		case AUDIO_PULSE:
			sprintf(string, AUDIO_PULSE_TITLE);
			break;
		case AUDIO_1394:
			sprintf(string, AUDIO_1394_TITLE);
			break;
		case AUDIO_DV1394:
			sprintf(string, AUDIO_DV1394_TITLE);
			break;
		case AUDIO_IEC61883:
			sprintf(string, AUDIO_IEC61883_TITLE);
			break;
		case AUDIO_DVB:
			sprintf(string, AUDIO_DVB_TITLE);
			break;
	}
	return string;
}

ADriverItem::ADriverItem(ADriverMenu *popup, const char *text, int driver)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->driver = driver;
}

ADriverItem::~ADriverItem()
{
}

int ADriverItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = driver;
	popup->device_prefs->initialize(0);
	popup->device_prefs->pwindow->show_window();
	return 1;
}




OSSEnable::OSSEnable(int x, int y, int *output)
 : BC_CheckBox(x, y, *output)
{
	this->output = output;
}
int OSSEnable::handle_event()
{
	*output = get_value();
	return 1;
}




ADeviceTextBox::ADeviceTextBox(int x, int y, char *output)
 : BC_TextBox(x, y, DP(150), 1, output)
{ 
	this->output = output; 
}

int ADeviceTextBox::handle_event() 
{
	strcpy(output, get_text());
	return 1;
}

ADeviceIntBox::ADeviceIntBox(int x, int y, int *output)
 : BC_TextBox(x, y, DP(80), 1, *output)
{ 
	this->output = output;
}

int ADeviceIntBox::handle_event() 
{ 
	*output = atol(get_text()); 
	return 1;
}


ALSADevice::ALSADevice(PreferencesDialog *dialog, 
	int x, 
	int y, 
	char *output, 
	ArrayList<BC_ListBoxItem*> *devices)
 : BC_PopupTextBox(dialog,
 	devices,
	output,
	x, 
	y, 
	DP(200),
	DP(200))
{
	this->output = output;
}

ALSADevice::~ALSADevice()
{
}

int ALSADevice::handle_event()
{
	strcpy(output, get_text());
	return 1;
}

