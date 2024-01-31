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

#include "audiodevice.inc"
#include "bchash.h"
#include "playbackconfig.h"
#include "recordconfig.h"
#include "videodevice.inc"
#include <string.h>





AudioInConfig::AudioInConfig()
{
	driver = AUDIO_OSS;
	oss_enable = 1;
	sprintf(oss_in_device, "/dev/dsp");
	oss_in_bits = 16;
	firewire_port = 0;
	firewire_channel = 63;
	strcpy(firewire_path, "/dev/raw1394");
	esound_in_server[0] = 0;
    pulse_in_server[0] = 0;
	esound_in_port = 0;

	sprintf(alsa_in_device, "default");
	alsa_in_bits = 16;
	in_samplerate = 48000;
	channels = 2;
}

AudioInConfig::~AudioInConfig()
{
}

int AudioInConfig::is_duplex(AudioInConfig *in, AudioOutConfig *out)
{
	if(in->driver == out->driver)
	{
		switch(in->driver)
		{
			case AUDIO_OSS:
			case AUDIO_OSS_ENVY24:
				return (!strcmp(in->oss_in_device, out->oss_out_device) &&
					in->oss_in_bits == out->oss_out_bits);
				break;

// ALSA always opens 2 devices
			case AUDIO_ALSA:
				return 0;
				break;
		}
	}

	return 0;
}


void AudioInConfig::copy_from(AudioInConfig *src)
{
	driver = src->driver;

	firewire_port = src->firewire_port;
	firewire_channel = src->firewire_channel;
	strcpy(firewire_path, src->firewire_path);

	strcpy(esound_in_server, src->esound_in_server);
	strcpy(pulse_in_server, src->pulse_in_server);
	esound_in_port = src->esound_in_port;

	oss_enable = src->oss_enable;
	strcpy(oss_in_device, src->oss_in_device);
	oss_in_bits = src->oss_in_bits;

	strcpy(alsa_in_device, src->alsa_in_device);
	alsa_in_bits = src->alsa_in_bits;
	in_samplerate = src->in_samplerate;
	channels = src->channels;
}

AudioInConfig& AudioInConfig::operator=(AudioInConfig &that)
{
	copy_from(&that);
	return *this;
}

int AudioInConfig::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];
	driver =                  defaults->get("AUDIOINDRIVER", driver);
	firewire_port =           defaults->get("AFIREWIRE_IN_PORT", firewire_port);
	firewire_channel =        defaults->get("AFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->get("AFIREWIRE_IN_PATH", firewire_path);
	oss_enable = defaults->get("OSS_ENABLE", oss_enable);
	defaults->get("OSS_IN_DEVICE", oss_in_device);
	sprintf(string, "OSS_IN_BITS");
	oss_in_bits = defaults->get(string, oss_in_bits);
	defaults->get("ESOUND_IN_SERVER", esound_in_server);
	defaults->get("PULSE_IN_SERVER", pulse_in_server);
	esound_in_port =              defaults->get("ESOUND_IN_PORT", esound_in_port);

	defaults->get("ALSA_IN_DEVICE", alsa_in_device);
	alsa_in_bits = defaults->get("ALSA_IN_BITS", alsa_in_bits);
	in_samplerate = defaults->get("IN_SAMPLERATE", in_samplerate);
	channels = defaults->get("IN_CHANNELS", channels);
	return 0;
}

int AudioInConfig::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];
	defaults->update("AUDIOINDRIVER", driver);
	defaults->update("AFIREWIRE_IN_PORT", firewire_port);
	defaults->update("AFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->update("AFIREWIRE_IN_PATH", firewire_path);

	defaults->update("OSS_ENABLE", oss_enable);
	defaults->update("OSS_IN_DEVICE", oss_in_device);


	sprintf(string, "OSS_IN_BITS");
	defaults->update(string, oss_in_bits);
	defaults->update("ESOUND_IN_SERVER", esound_in_server);
	defaults->update("PULSE_IN_SERVER", pulse_in_server);
	defaults->update("ESOUND_IN_PORT", esound_in_port);

	defaults->update("ALSA_IN_DEVICE", alsa_in_device);
	defaults->update("ALSA_IN_BITS", alsa_in_bits);
	defaults->update("IN_SAMPLERATE", in_samplerate);
	defaults->update("IN_CHANNELS", channels);
	return 0;
}







VideoInConfig::VideoInConfig()
{
//	driver = VIDEO4LINUX;
	driver = SCREENCAPTURE;
	sprintf(v4l_in_device, "/dev/video0");
	sprintf(v4l2_in_device, "/dev/video0");
	sprintf(lml_in_device, "/dev/mvideo/stream");
	sprintf(buz_in_device, "/dev/video0");
	screencapture_display[0] = 0;


// DVB
	sprintf(dvb_in_host, "echephyle");
	dvb_in_port = 400;
	dvb_in_number = 0;





	firewire_port = 0;
	firewire_channel = 63;
	sprintf(firewire_path, "/dev/raw1394");
// number of frames to read from device during video recording.
//	capture_length = 15;   
	capture_length = 2;   
	w = 720;
	h = 480;
	in_framerate = 29.97;
}

VideoInConfig::~VideoInConfig()
{
}

char* VideoInConfig::get_path()
{
	switch(driver)
	{
		case VIDEO4LINUX:
			return v4l_in_device;
			break;
		case VIDEO4LINUX2:
		case VIDEO4LINUX2JPEG:
		case VIDEO4LINUX2MJPG:
		case CAPTURE_JPEG_WEBCAM:
		case CAPTURE_YUYV_WEBCAM:
			return v4l2_in_device;
			break;
		case CAPTURE_BUZ:
			return buz_in_device;
			break;
		case CAPTURE_DVB:
			return dvb_in_host;
			break;
	}
	return v4l_in_device;
}

void VideoInConfig::copy_from(VideoInConfig *src)
{
	driver = src->driver;
	strcpy(v4l_in_device, src->v4l_in_device);
	strcpy(v4l2_in_device, src->v4l2_in_device);
	strcpy(lml_in_device, src->lml_in_device);
	strcpy(buz_in_device, src->buz_in_device);
	strcpy(screencapture_display, src->screencapture_display);





	strcmp(dvb_in_host, src->dvb_in_host);
	dvb_in_port = src->dvb_in_port;
	dvb_in_number = src->dvb_in_number;





	firewire_port = src->firewire_port;
	firewire_channel = src->firewire_channel;
	strcpy(firewire_path, src->firewire_path);
	capture_length = src->capture_length;
	w = src->w;
	h = src->h;
	in_framerate = src->in_framerate;
}

VideoInConfig& VideoInConfig::operator=(VideoInConfig &that)
{
	copy_from(&that);
	return *this;
}

int VideoInConfig::load_defaults(BC_Hash *defaults)
{
	driver = defaults->get("VIDEO_IN_DRIVER", driver);

// disable unsupported drivers
#ifndef HAVE_VIDEO4LINUX
    if(driver == VIDEO4LINUX || 
        driver == CAPTURE_BUZ)
    {
        driver = SCREENCAPTURE;
    }
#endif

#ifndef HAVE_VIDEO4LINUX2
    if(driver == VIDEO4LINUX2 || 
        driver == CAPTURE_JPEG_WEBCAM || 
        driver == CAPTURE_YUYV_WEBCAM || 
        driver == VIDEO4LINUX2JPEG || 
        driver == VIDEO4LINUX2MJPG || 
        driver == CAPTURE_MPEG)
    {
        driver = SCREENCAPTURE;
    }
#endif

	defaults->get("V4L_IN_DEVICE", v4l_in_device);
	defaults->get("V4L2_IN_DEVICE", v4l2_in_device);
	defaults->get("LML_IN_DEVICE", lml_in_device);
	defaults->get("BUZ_IN_DEVICE", buz_in_device);
	defaults->get("SCREENCAPTURE_DISPLAY", screencapture_display);

	defaults->get("DVB_IN_HOST", dvb_in_host);
	dvb_in_port = defaults->get("DVB_IN_PORT", dvb_in_port);
	dvb_in_number = defaults->get("DVB_IN_NUMBER", dvb_in_number);

	firewire_port = defaults->get("VFIREWIRE_IN_PORT", firewire_port);
	firewire_channel = defaults->get("VFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->get("VFIREWIRE_IN_PATH", firewire_path);
	capture_length = defaults->get("VIDEO_CAPTURE_LENGTH", capture_length);
	w = defaults->get("RECORD_W", w);
	h = defaults->get("RECORD_H", h);
	in_framerate = defaults->get("IN_FRAMERATE", in_framerate);
	return 0;
}

int VideoInConfig::save_defaults(BC_Hash *defaults)
{
	defaults->update("VIDEO_IN_DRIVER", driver);
	defaults->update("V4L_IN_DEVICE", v4l_in_device);
	defaults->update("V4L2_IN_DEVICE", v4l2_in_device);
	defaults->update("LML_IN_DEVICE", lml_in_device);
	defaults->update("BUZ_IN_DEVICE", buz_in_device);
	defaults->update("SCREENCAPTURE_DISPLAY", screencapture_display);




	defaults->update("DVB_IN_HOST", dvb_in_host);
	defaults->update("DVB_IN_PORT", dvb_in_port);
	defaults->update("DVB_IN_NUMBER", dvb_in_number);





	defaults->update("VFIREWIRE_IN_PORT", firewire_port);
	defaults->update("VFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->update("VFIREWIRE_IN_PATH", firewire_path);
	defaults->update("VIDEO_CAPTURE_LENGTH", capture_length);
	defaults->update("RECORD_W", w);
	defaults->update("RECORD_H", h);
	defaults->update("IN_FRAMERATE", in_framerate);
	return 0;
}

