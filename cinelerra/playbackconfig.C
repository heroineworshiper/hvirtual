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

#include "clip.h"
#include "bchash.h"
#include "playbackconfig.h"
#include "videodevice.inc"
#include <string.h>

AudioOutConfig::AudioOutConfig()
{
	fragment_size = 16384;
	driver = AUDIO_OSS;

	audio_offset = 0.0;

	oss_out_bits = 16;
	oss_enable = 1;
	sprintf(oss_out_device, "/dev/dsp");

	esound_out_server[0] = 0;
	pulse_out_server[0] = 0;
	esound_out_port = 0;

	sprintf(alsa_out_device, "default");
	alsa_out_bits = 16;
	interrupt_workaround = 0;

	firewire_channel = 63;
	firewire_port = 0;
	strcpy(firewire_path, "/dev/video1394");
	firewire_syt = 30000;

	dv1394_channel = 63;
	dv1394_port = 0;
	strcpy(dv1394_path, "/dev/dv1394");
	dv1394_syt = 30000;

}

AudioOutConfig::~AudioOutConfig()
{
}



int AudioOutConfig::operator!=(AudioOutConfig &that)
{
	return !(*this == that);
}

int AudioOutConfig::operator==(AudioOutConfig &that)
{
	return 
		fragment_size == that.fragment_size &&
		driver == that.driver &&
		EQUIV(audio_offset, that.audio_offset) &&


		!strcmp(oss_out_device, that.oss_out_device) && 
		(oss_out_bits == that.oss_out_bits) && 



		!strcmp(esound_out_server, that.esound_out_server) && 
		!strcmp(pulse_out_server, that.pulse_out_server) && 
		(esound_out_port == that.esound_out_port) && 



		!strcmp(alsa_out_device, that.alsa_out_device) &&
		(alsa_out_bits == that.alsa_out_bits) &&
		(interrupt_workaround == that.interrupt_workaround) &&

		firewire_channel == that.firewire_channel &&
		firewire_port == that.firewire_port &&
		firewire_syt == that.firewire_syt &&
		!strcmp(firewire_path, that.firewire_path) &&

		dv1394_channel == that.dv1394_channel &&
		dv1394_port == that.dv1394_port &&
		dv1394_syt == that.dv1394_syt &&
		!strcmp(dv1394_path, that.dv1394_path);
}



AudioOutConfig& AudioOutConfig::operator=(AudioOutConfig &that)
{
	copy_from(&that);
	return *this;
}

void AudioOutConfig::dump()
{
    printf("AudioOutConfig::dump %d: driver=%d\n", 
        __LINE__, 
        driver);
}

void AudioOutConfig::copy_from(AudioOutConfig *src)
{
	fragment_size = src->fragment_size;
	driver = src->driver;
	audio_offset = src->audio_offset;

	strcpy(esound_out_server, src->esound_out_server);
	strcpy(pulse_out_server, src->pulse_out_server);
	esound_out_port = src->esound_out_port;
	oss_enable = src->oss_enable;
	strcpy(oss_out_device, src->oss_out_device);
	oss_out_bits = src->oss_out_bits;

	strcpy(alsa_out_device, src->alsa_out_device);
	alsa_out_bits = src->alsa_out_bits;
	interrupt_workaround = src->interrupt_workaround;

	firewire_channel = src->firewire_channel;
	firewire_port = src->firewire_port;
	firewire_syt = src->firewire_syt;
	strcpy(firewire_path, src->firewire_path);

	dv1394_channel = src->dv1394_channel;
	dv1394_port = src->dv1394_port;
	dv1394_syt = src->dv1394_syt;
	strcpy(dv1394_path, src->dv1394_path);

}

int AudioOutConfig::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	fragment_size = defaults->get("FRAGMENT_SIZE", fragment_size);
	audio_offset = defaults->get("AUDIO_OFFSET", audio_offset);
	driver = defaults->get("AUDIO_OUT_DRIVER", driver);

	oss_enable = defaults->get("OSS_ENABLE", oss_enable);
	defaults->get("OSS_OUT_DEVICE", oss_out_device);

	oss_out_bits = defaults->get("OSS_OUT_BITS", oss_out_bits);

	defaults->get("ALSA_OUT_DEVICE", alsa_out_device);
	alsa_out_bits = defaults->get("ALSA_OUT_BITS", alsa_out_bits);
	interrupt_workaround = defaults->get("ALSA_INTERRUPT_WORKAROUND", interrupt_workaround);

	defaults->get("ESOUND_OUT_SERVER", esound_out_server);
	defaults->get("PULSE_OUT_SERVER", pulse_out_server);
	esound_out_port = defaults->get("ESOUND_OUT_PORT", esound_out_port);

	sprintf(string, "AFIREWIRE_OUT_CHANNEL");
	firewire_channel = defaults->get(string, firewire_channel);
	sprintf(string, "AFIREWIRE_OUT_PORT");
	firewire_port = defaults->get(string, firewire_port);
	sprintf(string, "AFIREWIRE_OUT_PATH");
	defaults->get(string, firewire_path);
	sprintf(string, "AFIREWIRE_OUT_SYT");
	firewire_syt = defaults->get(string, firewire_syt);

	sprintf(string, "ADV1394_OUT_CHANNEL");
	dv1394_channel = defaults->get(string, dv1394_channel);
	sprintf(string, "ADV1394_OUT_PORT");
	dv1394_port = defaults->get(string, dv1394_port);
	sprintf(string, "ADV1394_OUT_PATH");
	defaults->get(string, dv1394_path);
	sprintf(string, "ADV1394_OUT_SYT");
	dv1394_syt = defaults->get(string, dv1394_syt);

	return 0;
}

int AudioOutConfig::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	defaults->update("FRAGMENT_SIZE", fragment_size);
	defaults->update("AUDIO_OFFSET", audio_offset);

	defaults->update("AUDIO_OUT_DRIVER", driver);

	defaults->update("OSS_ENABLE", oss_enable);
	defaults->update("OSS_OUT_DEVICE", oss_out_device);
	defaults->update("OSS_OUT_BITS", oss_out_bits);


	defaults->update("ALSA_OUT_DEVICE", alsa_out_device);
	defaults->update("ALSA_OUT_BITS", alsa_out_bits);
	defaults->update("ALSA_INTERRUPT_WORKAROUND", interrupt_workaround);

	defaults->update("ESOUND_OUT_SERVER", esound_out_server);
	defaults->update("PULSE_OUT_SERVER", pulse_out_server);
	defaults->update("ESOUND_OUT_PORT", esound_out_port);

	sprintf(string, "AFIREWIRE_OUT_CHANNEL");
	defaults->update(string, firewire_channel);
	sprintf(string, "AFIREWIRE_OUT_PORT");
	defaults->update(string, firewire_port);
	sprintf(string, "AFIREWIRE_OUT_PATH");
	defaults->update(string, firewire_path);
	sprintf(string, "AFIREWIRE_OUT_SYT");
	defaults->update(string, firewire_syt);


	sprintf(string, "ADV1394_OUT_CHANNEL");
	defaults->update(string, dv1394_channel);
	sprintf(string, "ADV1394_OUT_PORT");
	defaults->update(string, dv1394_port);
	sprintf(string, "ADV1394_OUT_PATH");
	defaults->update(string, dv1394_path);
	sprintf(string, "ADV1394_OUT_SYT");
	defaults->update(string, dv1394_syt);

	return 0;
}










VideoOutConfig::VideoOutConfig()
{
	sprintf(lml_out_device, "/dev/mvideo/stream");
	sprintf(buz_out_device, "/dev/video0");
	driver = PLAYBACK_X11;
	buz_out_channel = 0;
	buz_swap_fields = 0;
	x11_host[0] = 0;
	x11_use_fields = USE_NO_FIELDS;

	firewire_channel = 63;
	firewire_port = 0;
	strcpy(firewire_path, "/dev/video1394");
	firewire_syt = 30000;

	dv1394_channel = 63;
	dv1394_port = 0;
	strcpy(dv1394_path, "/dev/dv1394");
	dv1394_syt = 30000;

	brightness = 32768;
	hue = 32768;
	color = 32768;
	contrast = 32768;
	whiteness = 32768;
}

VideoOutConfig::~VideoOutConfig()
{
}


int VideoOutConfig::operator!=(VideoOutConfig &that)
{
	return !(*this == that);
}

int VideoOutConfig::operator==(VideoOutConfig &that)
{
	return (driver == that.driver) &&
		!strcmp(lml_out_device, that.lml_out_device) && 
		!strcmp(buz_out_device, that.buz_out_device) && 
		(buz_out_channel == that.buz_out_channel) && 
		(buz_swap_fields == that.buz_swap_fields) &&
		!strcmp(x11_host, that.x11_host) && 
		(x11_use_fields == that.x11_use_fields) &&
		(brightness == that.brightness) && 
		(hue == that.hue) && 
		(color == that.color) && 
		(contrast == that.contrast) && 
		(whiteness == that.whiteness) &&

		(firewire_channel == that.firewire_channel) &&
		(firewire_port == that.firewire_port) &&
		!strcmp(firewire_path, that.firewire_path) &&
		(firewire_syt == that.firewire_syt) &&

		(dv1394_channel == that.dv1394_channel) &&
		(dv1394_port == that.dv1394_port) &&
		!strcmp(dv1394_path, that.dv1394_path) &&
		(dv1394_syt == that.dv1394_syt);
}



void VideoOutConfig::dump()
{
    printf("VideoOutConfig::dump %d: driver=%d\n", 
        __LINE__, 
        driver);
}



VideoOutConfig& VideoOutConfig::operator=(VideoOutConfig &that)
{
	copy_from(&that);
	return *this;
}

void VideoOutConfig::copy_from(VideoOutConfig *src)
{
	this->driver = src->driver;
	strcpy(this->lml_out_device, src->lml_out_device);
	strcpy(this->buz_out_device, src->buz_out_device);
	this->buz_out_channel = src->buz_out_channel;
	this->buz_swap_fields = src->buz_swap_fields;
	strcpy(this->x11_host, src->x11_host);
	this->x11_use_fields = src->x11_use_fields;

	firewire_channel = src->firewire_channel;
	firewire_port = src->firewire_port;
	strcpy(firewire_path, src->firewire_path);
	firewire_syt = src->firewire_syt;

	dv1394_channel = src->dv1394_channel;
	dv1394_port = src->dv1394_port;
	strcpy(dv1394_path, src->dv1394_path);
	dv1394_syt = src->dv1394_syt;
}

char* VideoOutConfig::get_path()
{
	switch(driver)
	{
		case PLAYBACK_BUZ:
			return buz_out_device;
			break;
		case PLAYBACK_X11:
		case PLAYBACK_X11_XV:
			return x11_host;
			break;
		case PLAYBACK_DV1394:
			return dv1394_path;
			break;
		case PLAYBACK_FIREWIRE:
			return firewire_path;
			break;
	};
	return buz_out_device;
}

int VideoOutConfig::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];
	sprintf(string, "VIDEO_OUT_DRIVER");
	driver = defaults->get(string, driver);
	sprintf(string, "LML_OUT_DEVICE");
	defaults->get(string, lml_out_device);
	sprintf(string, "BUZ_OUT_DEVICE");
	defaults->get(string, buz_out_device);
	sprintf(string, "BUZ_OUT_CHANNEL");
	buz_out_channel = defaults->get(string, buz_out_channel);
	sprintf(string, "BUZ_SWAP_FIELDS");
	buz_swap_fields = defaults->get(string, buz_swap_fields);
	sprintf(string, "X11_OUT_DEVICE");
	defaults->get(string, x11_host);
	x11_use_fields = defaults->get("X11_USE_FIELDS", x11_use_fields);



	sprintf(string, "VFIREWIRE_OUT_CHANNEL");
	firewire_channel = defaults->get(string, firewire_channel);
	sprintf(string, "VFIREWIRE_OUT_PORT");
	firewire_port = defaults->get(string, firewire_port);
	sprintf(string, "VFIREWIRE_OUT_PATH");
	defaults->get(string, firewire_path);
	sprintf(string, "VFIREWIRE_OUT_SYT");
	firewire_syt = defaults->get(string, firewire_syt);

	sprintf(string, "VDV1394_OUT_CHANNEL");
	dv1394_channel = defaults->get(string, dv1394_channel);
	sprintf(string, "VDV1394_OUT_PORT");
	dv1394_port = defaults->get(string, dv1394_port);
	sprintf(string, "VDV1394_OUT_PATH");
	defaults->get(string, dv1394_path);
	sprintf(string, "VDV1394_OUT_SYT");
	dv1394_syt = defaults->get(string, dv1394_syt);
	return 0;
}

int VideoOutConfig::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];
	sprintf(string, "VIDEO_OUT_DRIVER");
	defaults->update(string, driver);
	sprintf(string, "LML_OUT_DEVICE");
	defaults->update(string, lml_out_device);
	sprintf(string, "BUZ_OUT_DEVICE");
	defaults->update(string, buz_out_device);
	sprintf(string, "BUZ_OUT_CHANNEL");
	defaults->update(string, buz_out_channel);
	sprintf(string, "BUZ_SWAP_FIELDS");
	defaults->update(string, buz_swap_fields);
	sprintf(string, "X11_OUT_DEVICE");
	defaults->update(string, x11_host);
	defaults->update("X11_USE_FIELDS", x11_use_fields);

	sprintf(string, "VFIREWIRE_OUT_CHANNEL");
	defaults->update(string, firewire_channel);
	sprintf(string, "VFIREWIRE_OUT_PORT");
	defaults->update(string, firewire_port);
	sprintf(string, "VFIREWIRE_OUT_PATH");
	defaults->update(string, firewire_path);
	sprintf(string, "VFIREWIRE_OUT_SYT");
	defaults->update(string, firewire_syt);

	sprintf(string, "VDV1394_OUT_CHANNEL");
	defaults->update(string, dv1394_channel);
	sprintf(string, "VDV1394_OUT_PORT");
	defaults->update(string, dv1394_port);
	sprintf(string, "VDV1394_OUT_PATH");
	defaults->update(string, dv1394_path);
	sprintf(string, "VDV1394_OUT_SYT");
	defaults->update(string, dv1394_syt);
	return 0;
}









PlaybackConfig::PlaybackConfig()
{
	aconfig = new AudioOutConfig;
	vconfig = new VideoOutConfig;
	sprintf(hostname, "localhost");
	port = 23456;
}

PlaybackConfig::~PlaybackConfig()
{
	delete aconfig;
	delete vconfig;
}

PlaybackConfig& PlaybackConfig::operator=(PlaybackConfig &that)
{
	copy_from(&that);
	return *this;
}

void PlaybackConfig::copy_from(PlaybackConfig *src)
{
	aconfig->copy_from(src->aconfig);
	vconfig->copy_from(src->vconfig);
	strcpy(hostname, src->hostname);
	port = src->port;
}

int PlaybackConfig::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];
	sprintf(string, "PLAYBACK_HOSTNAME");
	defaults->get(string, hostname);
	sprintf(string, "PLAYBACK_PORT");
	port = defaults->get(string, port);
	aconfig->load_defaults(defaults);
	vconfig->load_defaults(defaults);
	return 0;
}

int PlaybackConfig::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];
	sprintf(string, "PLAYBACK_HOSTNAME");
	defaults->update(string, hostname);
	sprintf(string, "PLAYBACK_PORT");
	defaults->update(string, port);
	aconfig->save_defaults(defaults);
	vconfig->save_defaults(defaults);
	return 0;
}

void PlaybackConfig::dump()
{
    printf("PlaybackConfig::dump %d: %p\n", __LINE__, this);
    aconfig->dump();
    vconfig->dump();
}


