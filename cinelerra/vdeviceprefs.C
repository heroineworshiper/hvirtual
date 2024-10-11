/*
 * CINELERRA
 * Copyright (C) 2011-2024 Adam Williams <broadcast at earthling dot net>
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

#include "bcsignals.h"
#include "channeldb.h"
#include "channelpicker.h"
#include "edl.h"
#include "edlsession.h"
#include "formattools.h"
#include "../hvirtual_config.h"
#include "language.h"
#include "mwindow.h"
#include "vdeviceprefs.h"
#include "videoconfig.h"
#include "videodevice.inc"
#include "playbackconfig.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "recordconfig.h"
#include "recordprefs.h"
#include "theme.h"
#include <string.h>


VDevicePrefs::VDevicePrefs(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PreferencesDialog *dialog, 
	VideoOutConfig *out_config,
	VideoInConfig *in_config,
	int mode)
{
	this->pwindow = pwindow;
	this->dialog = dialog;
	this->driver = -1;
	this->mode = mode;
	this->out_config = out_config;
	this->in_config = in_config;
	this->x = x;
	this->y = y;
	menu = 0;
	reset_objects();

}

VDevicePrefs::~VDevicePrefs()
{
	delete_objects();
	if(menu) delete menu;
	pwindow->mwindow->channeldb_buz->save("channeldb_buz");
}


void VDevicePrefs::reset_objects()
{
	device_title = 0;
	device_text = 0;

	port_title = 0;
	device_port = 0;

	number_title = 0;
	device_number = 0;
	

	channel_title = 0;
	syt_title = 0;

	firewire_port = 0;
	firewire_channel = 0;
	firewire_channels = 0;
	firewire_syt = 0;
	firewire_path = 0;

	buz_swap_channels = 0;
	output_title = 0;
	channel_picker = 0;
    format_menu = 0;
    format_title = 0;
}

int VDevicePrefs::initialize(int creation)
{
	int *driver = 0;
	delete_objects();

	switch(mode)
	{
		case MODEPLAY:
			driver = &out_config->driver;
			break;

		case MODERECORD:
			driver = &in_config->driver;
			break;
	}
	this->driver = *driver;

	if(!menu)
	{
		dialog->add_subwindow(menu = new VDriverMenu(x,
			y,
			this, 
			(mode == MODERECORD), 
			driver));
		menu->create_objects();
	}

	switch(this->driver)
	{
		case VIDEO4LINUX:
			create_v4l_objs();
			break;
		case VIDEO4LINUX2:
// 		case CAPTURE_JPEG_WEBCAM:
// 		case CAPTURE_YUYV_WEBCAM:
// 		case VIDEO4LINUX2JPEG:
// 		case VIDEO4LINUX2MJPG:
// 		case CAPTURE_MPEG:
			create_v4l2_objs();
			break;
		case SCREENCAPTURE:
			create_screencap_objs();
			break;
		case CAPTURE_LML:
			create_lml_objs();
			break;
		case CAPTURE_BUZ:
		case PLAYBACK_BUZ:
			create_buz_objs();
			break;
		case PLAYBACK_X11:
		case PLAYBACK_X11_XV:
		case PLAYBACK_X11_GL:
			create_x11_objs();
			break;
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
		case PLAYBACK_IEC61883:
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			create_firewire_objs();
			break;
		case CAPTURE_DVB:
			create_dvb_objs();
			break;
	}



// Update driver dependancies in file format
	if(mode == MODERECORD && dialog && !creation)
	{
		RecordPrefs *record_prefs = (RecordPrefs*)dialog;
		record_prefs->recording_format->update_driver(in_config);
	}

	return 0;
}

int VDevicePrefs::delete_objects()
{
	delete output_title;
	delete channel_picker;
	delete buz_swap_channels;
	delete device_title;
	delete device_text;

	delete port_title;
	delete device_port;

	delete number_title;
	delete device_number;
    
    delete format_title;
    delete format_menu;

	if(firewire_port) delete firewire_port;
	if(channel_title) delete channel_title;
	if(firewire_channel) delete firewire_channel;
	if(firewire_path) delete firewire_path;
	if(syt_title) delete syt_title;
	if(firewire_syt) delete firewire_syt;

	reset_objects();
	driver = -1;
	return 0;
}

int VDevicePrefs::get_h()
{
	int margin = pwindow->mwindow->theme->widget_border;
	return BC_Title::calculate_h(dialog, 
			"XXX", 
			MEDIUMFONT) + 
		margin +
		BC_TextBox::calculate_h(dialog, 
			MEDIUMFONT, 
			1, 
			1);
}


int VDevicePrefs::create_dvb_objs()
{
	int x1 = x + menu->get_w() + DP(5);
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Host:")));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, in_config->dvb_in_host));
	x1 += device_text->get_w() + DP(10);
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:")));
	device_port = new VDeviceTumbleBox(this, x1, y + 20,  &in_config->dvb_in_port, 1, 65536);
	device_port->create_objects();
	x1 += device_port->get_w() + DP(10);
	dialog->add_subwindow(number_title = new BC_Title(x1, y, _("Adaptor:")));
	device_number = new VDeviceTumbleBox(this, x1, y + 20,  &in_config->dvb_in_number, 0, 16);
	device_number->create_objects();
    return 0;
}

int VDevicePrefs::create_lml_objs()
{
	char *output_char;
	int x1 = x + menu->get_w() + DP(5);
	BC_Resources *resources = BC_WindowBase::get_resources();

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->lml_out_device;
			break;
		case MODERECORD:
			output_char = in_config->lml_in_device;
			break;
	}
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	x1 += device_title->get_w() + DP(10);
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + DP(20), output_char));
	return 0;
}

int VDevicePrefs::create_buz_objs()
{
#ifdef HAVE_VIDEO4LINUX


	char *output_char;
	int x1 = x + menu->get_w() + DP(5);
	int x2 = x1 + DP(210);
	int y1 = y;
	BC_Resources *resources = BC_WindowBase::get_resources();

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->buz_out_device;
			break;
		case MODERECORD:
			output_char = in_config->buz_in_device;
			break;
	}
	dialog->add_subwindow(device_title = new BC_Title(x1, y1, _("Device path:"), MEDIUMFONT, resources->text_default));

	y1 += DP(20);
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y1, output_char));

	if(driver == PLAYBACK_BUZ)
	{
		dialog->add_subwindow(buz_swap_channels = 
			new VDeviceCheckBox(x2, y1, &out_config->buz_swap_fields, _("Swap fields")));
	}
	y1 += DP(30);
	if(driver == PLAYBACK_BUZ)
	{
		dialog->add_subwindow(output_title = new BC_Title(x1, y1, _("Output channel:")));
		y1 += DP(20);
		channel_picker = new PrefsChannelPicker(pwindow->mwindow, 
			this, 
			pwindow->mwindow->channeldb_buz, 
			x1,
			y1);
		channel_picker->create_objects();
	}
#endif // HAVE_VIDEO4LINUX

	return 0;
}

int VDevicePrefs::create_firewire_objs()
{
	int *output_int = 0;
	char *output_char = 0;
	int x1 = x + menu->get_w() + DP(5);
	BC_Resources *resources = BC_WindowBase::get_resources();

// Firewire path
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_char = out_config->dv1394_path;
			else
			if(driver == PLAYBACK_FIREWIRE)
				output_char = out_config->firewire_path;
			break;
		case MODERECORD:
// Our version of raw1394 doesn't support changing the input path
			output_char = 0;
			break;
	}

	if(output_char)
	{
		dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device Path:"), MEDIUMFONT, resources->text_default));
		dialog->add_subwindow(firewire_path = new VDeviceTextBox(x1, y + DP(20), output_char));
		x1 += firewire_path->get_w() + DP(5);
	}

// Firewire port
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_int = &out_config->dv1394_port;
			else
				output_int = &out_config->firewire_port;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_port;
			break;
	}
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(firewire_port = new VDeviceIntBox(x1, y + DP(20), output_int));
	x1 += firewire_port->get_w() + DP(5);

// Firewire channel
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_int = &out_config->dv1394_channel;
			else
				output_int = &out_config->firewire_channel;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_channel;
			break;
	}

	dialog->add_subwindow(channel_title = new BC_Title(x1, y, _("Channel:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(firewire_channel = new VDeviceIntBox(x1, y + DP(20), output_int));
	x1 += firewire_channel->get_w() + 5;


// Firewire syt
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_int = &out_config->dv1394_syt;
			else
			if(driver == PLAYBACK_FIREWIRE)
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
		dialog->add_subwindow(firewire_syt = new VDeviceIntBox(x1, y + DP(20), output_int));
	}

	return 0;
}

int VDevicePrefs::create_v4l_objs()
{
	int margin = MWindow::theme->widget_border;
#ifdef HAVE_VIDEO4LINUX


	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + margin;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:")));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, 
        y + device_title->get_h(), 
        output_char));

#endif // HAVE_VIDEO4LINUX
	return 0;
}

int VDevicePrefs::create_v4l2_objs()
{
	int margin = MWindow::theme->widget_border;
	BC_Resources *resources = BC_WindowBase::get_resources();
	char *output_char;
	int x1 = x + menu->get_w() + margin;
	output_char = pwindow->thread->preferences->vconfig_in->v4l2_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:")));
	int y2 = y + device_title->get_h();
    dialog->add_subwindow(device_text = new VDeviceTextBox(x1, 
        y2, 
        output_char));

    int x2 = x1 + device_text->get_w() + margin;
    dialog->add_subwindow(format_title = new BC_Title(x2, 
        y, 
        _("Codec:")));
	dialog->add_subwindow(format_menu = new VDriverFormat(x2, 
        y2, 
        this,
        &pwindow->thread->preferences->vconfig_in->v4l2_format));
    format_menu->create_objects();
    format_menu->show_window(1);
	return 0;
}



int VDevicePrefs::create_screencap_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + DP(5);
	output_char = pwindow->thread->preferences->vconfig_in->screencapture_display;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Display:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + DP(20), output_char));
	return 0;
}

int VDevicePrefs::create_x11_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + DP(5);
	output_char = out_config->x11_host;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Display for compositor:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + DP(20), output_char));
	return 0;
}




VDriverMenu::VDriverMenu(int x, 
	int y, 
	VDevicePrefs *device_prefs, 
	int do_input, 
	int *output)
 : BC_PopupMenu(x, y, DP(180), driver_to_string(*output))
{
	this->output = output;
	this->do_input = do_input;
	this->device_prefs = device_prefs;
}

const char* VDriverMenu::driver_to_string(int driver)
{
	switch(driver)
	{
		case VIDEO4LINUX:
			sprintf(string, VIDEO4LINUX_TITLE);
			break;
		case VIDEO4LINUX2:
			sprintf(string, VIDEO4LINUX2_TITLE);
			break;
// 		case VIDEO4LINUX2JPEG:
// 			return VIDEO4LINUX2JPEG_TITLE;
// 			break;
// 		case VIDEO4LINUX2MJPG:
// 			return VIDEO4LINUX2MJPG_TITLE;
// 			break;
// 		case CAPTURE_JPEG_WEBCAM:
// 			sprintf(string, CAPTURE_JPEG_WEBCAM_TITLE);
// 			break;
// 		case CAPTURE_YUYV_WEBCAM:
// 			sprintf(string, CAPTURE_YUYV_WEBCAM_TITLE);
// 			break;
		case SCREENCAPTURE:
			sprintf(string, SCREENCAPTURE_TITLE);
			break;
		case CAPTURE_BUZ:
			sprintf(string, CAPTURE_BUZ_TITLE);
			break;
		case CAPTURE_LML:
			sprintf(string, CAPTURE_LML_TITLE);
			break;
		case CAPTURE_FIREWIRE:
			sprintf(string, CAPTURE_FIREWIRE_TITLE);
			break;
		case CAPTURE_IEC61883:
			sprintf(string, CAPTURE_IEC61883_TITLE);
			break;
		case CAPTURE_DVB:
			sprintf(string, CAPTURE_DVB_TITLE);
			break;
// 		case CAPTURE_MPEG:
// 			sprintf(string, CAPTURE_MPEG_TITLE);
// 			break;
		case PLAYBACK_X11:
			sprintf(string, PLAYBACK_X11_TITLE);
			break;
		case PLAYBACK_X11_XV:
			sprintf(string, PLAYBACK_X11_XV_TITLE);
			break;
		case PLAYBACK_X11_GL:
			sprintf(string, PLAYBACK_X11_GL_TITLE);
			break;
		case PLAYBACK_LML:
			sprintf(string, PLAYBACK_LML_TITLE);
			break;
		case PLAYBACK_BUZ:
			sprintf(string, PLAYBACK_BUZ_TITLE);
			break;
		case PLAYBACK_FIREWIRE:
			sprintf(string, PLAYBACK_FIREWIRE_TITLE);
			break;
		case PLAYBACK_DV1394:
			sprintf(string, PLAYBACK_DV1394_TITLE);
			break;
		case PLAYBACK_IEC61883:
			sprintf(string, PLAYBACK_IEC61883_TITLE);
			break;
		default:
			string[0] = 0;
	}
	return string;
}

void VDriverMenu::create_objects()
{
	if(do_input)
	{
#ifdef HAVE_VIDEO4LINUX
		add_item(new VDriverItem(this, VIDEO4LINUX_TITLE, VIDEO4LINUX));
#endif

#ifdef HAVE_VIDEO4LINUX2
		add_item(new VDriverItem(this, VIDEO4LINUX2_TITLE, VIDEO4LINUX2));
// 		add_item(new VDriverItem(this, CAPTURE_JPEG_WEBCAM_TITLE, CAPTURE_JPEG_WEBCAM));
// 		add_item(new VDriverItem(this, CAPTURE_YUYV_WEBCAM_TITLE, CAPTURE_YUYV_WEBCAM));
// 		add_item(new VDriverItem(this, VIDEO4LINUX2JPEG_TITLE, VIDEO4LINUX2JPEG));
// 		add_item(new VDriverItem(this, VIDEO4LINUX2MJPG_TITLE, VIDEO4LINUX2MJPG));
// 		add_item(new VDriverItem(this, CAPTURE_MPEG_TITLE, CAPTURE_MPEG));
#endif

		add_item(new VDriverItem(this, SCREENCAPTURE_TITLE, SCREENCAPTURE));
#ifdef HAVE_VIDEO4LINUX
		add_item(new VDriverItem(this, CAPTURE_BUZ_TITLE, CAPTURE_BUZ));
#endif
		add_item(new VDriverItem(this, CAPTURE_FIREWIRE_TITLE, CAPTURE_FIREWIRE));
		add_item(new VDriverItem(this, CAPTURE_IEC61883_TITLE, CAPTURE_IEC61883));
//		add_item(new VDriverItem(this, CAPTURE_DVB_TITLE, CAPTURE_DVB));
	}
	else
	{
		add_item(new VDriverItem(this, PLAYBACK_X11_TITLE, PLAYBACK_X11));
		add_item(new VDriverItem(this, PLAYBACK_X11_XV_TITLE, PLAYBACK_X11_XV));
#ifdef HAVE_GL
		add_item(new VDriverItem(this, PLAYBACK_X11_GL_TITLE, PLAYBACK_X11_GL));
#endif
		add_item(new VDriverItem(this, PLAYBACK_BUZ_TITLE, PLAYBACK_BUZ));
		add_item(new VDriverItem(this, PLAYBACK_FIREWIRE_TITLE, PLAYBACK_FIREWIRE));
		add_item(new VDriverItem(this, PLAYBACK_DV1394_TITLE, PLAYBACK_DV1394));
		add_item(new VDriverItem(this, PLAYBACK_IEC61883_TITLE, PLAYBACK_IEC61883));
	}
}


VDriverItem::VDriverItem(VDriverMenu *popup, const char *text, int driver)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->driver = driver;
}

int VDriverItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = driver;
	popup->device_prefs->initialize(0);
	return 1;
}





VDriverFormat::VDriverFormat(int x, 
	int y, 
	VDevicePrefs *device_prefs, 
	int *output)
 : BC_PopupMenu(x, y, DP(200), format_to_string(*output))
{
	this->output = output;
	this->device_prefs = device_prefs;
}

const char* VDriverFormat::format_to_string(int format)
{
	switch(format)
	{
		case CAPTURE_RGB:
			sprintf(string, CAPTURE_RGB_TITLE);
			break;
		case CAPTURE_YUYV:
			sprintf(string, CAPTURE_YUYV_TITLE);
			break;
		case CAPTURE_JPEG:
			return CAPTURE_JPEG_TITLE;
			break;
		case CAPTURE_JPEG_NOHEAD:
			return CAPTURE_JPEG_NOHEAD_TITLE;
			break;
		case CAPTURE_MJPG:
			sprintf(string, CAPTURE_MJPG_TITLE);
			break;
		case CAPTURE_MJPG_1FIELD:
			sprintf(string, CAPTURE_MJPG_1FIELD_TITLE);
			break;
		default:
			sprintf(string, "Unknown");
	}
	return string;
}

void VDriverFormat::create_objects()
{
	add_item(new VDriverFormatItem(this, CAPTURE_RGB_TITLE, CAPTURE_RGB));
	add_item(new VDriverFormatItem(this, CAPTURE_YUYV_TITLE, CAPTURE_YUYV));
	add_item(new VDriverFormatItem(this, CAPTURE_JPEG_TITLE, CAPTURE_JPEG));
	add_item(new VDriverFormatItem(this, CAPTURE_JPEG_NOHEAD_TITLE, CAPTURE_JPEG_NOHEAD));
	add_item(new VDriverFormatItem(this, CAPTURE_MJPG_TITLE, CAPTURE_MJPG));
	add_item(new VDriverFormatItem(this, CAPTURE_MJPG_1FIELD_TITLE, CAPTURE_MJPG_1FIELD));
}

VDriverFormatItem::VDriverFormatItem(VDriverFormat *popup, const char *text, int format)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->format = format;
}

int VDriverFormatItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = format;
	popup->device_prefs->initialize(0);
	return 1;
}




VDeviceTextBox::VDeviceTextBox(int x, int y, char *output)
 : BC_TextBox(x, y, DP(200), 1, output)
{ 
	this->output = output; 
}

int VDeviceTextBox::handle_event() 
{ 
// Suggestions
	calculate_suggestions(0);

	strcpy(output, get_text()); 
    return 0;
}

VDeviceTumbleBox::VDeviceTumbleBox(VDevicePrefs *prefs, 
	int x, 
	int y, 
	int *output,
	int min,
	int max)
 : BC_TumbleTextBox(prefs->dialog,
	*output,
	min,
	max,
 	x, 
	y, 
	DP(60))
{ 
	this->output = output; 
}

int VDeviceTumbleBox::handle_event() 
{
	*output = atol(get_text()); 
	return 1;
}






VDeviceIntBox::VDeviceIntBox(int x, int y, int *output)
 : BC_TextBox(x, y, DP(60), 1, *output)
{ 
	this->output = output; 
}

int VDeviceIntBox::handle_event() 
{ 
	*output = atol(get_text()); 
	return 1;
}





VDeviceCheckBox::VDeviceCheckBox(int x, int y, int *output, char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
}
int VDeviceCheckBox::handle_event()
{
	*output = get_value();
	return 1;
}

