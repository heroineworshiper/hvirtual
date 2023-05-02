/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "assets.h"
#include "bcsignals.h"
#include "bitspopup.h"
#include "clip.h"
#include "file.h"
#include "filesndfile.h"
#include "language.h"
#include "mwindow.inc"



FileSndFile::FileSndFile(Asset *asset, File *file)
 : FileBase(asset, file)
{
	temp_double = 0;
	temp_allocated = 0;
	fd_config.format = 0;
	fd = 0;
// 	if(asset->format == FILE_UNKNOWN)
// 		asset->format = FILE_PCM;
}

FileSndFile::~FileSndFile()
{
	if(temp_double) delete [] temp_double;
}


FileSndFile::FileSndFile()
 : FileBase()
{
    ids.append(FILE_PCM);
    ids.append(FILE_WAV);
    ids.append(FILE_AU);
    ids.append(FILE_AIFF);
    ids.append(FILE_SND);
    has_audio = 1;
    has_wr = 1;
    has_rd = 1;
}

FileBase* FileSndFile::create(File *file)
{
    return new FileSndFile(file->asset, file);
}

const char* FileSndFile::formattostr(int format)
{
    switch(format)
    {
		case FILE_WAV:
			return WAV_NAME;
			break;
		case FILE_PCM:
			return PCM_NAME;
			break;
		case FILE_AU:
			return AU_NAME;
			break;
		case FILE_AIFF:
			return AIFF_NAME;
			break;
		case FILE_SND:
			return SND_NAME;
			break;
    }
    return 0;
}

const char* FileSndFile::get_tag(int format)
{
    switch(format)
    {
		case FILE_AC3:  return "ac3";
		case FILE_AIFF: return "aif";
		case FILE_AU:   return "au";
		case FILE_PCM:  return "pcm";
		case FILE_WAV:  return "wav";
    }
    return 0;
}

int FileSndFile::check_sig(File *file, const uint8_t *test_data)
{
    Asset *asset = file->asset;
	int result = 0;
	SF_INFO fd_config;
	fd_config.format = 0;
	SNDFILE *fd = sf_open(asset->path, SFM_READ, &fd_config);
	if(fd)
	{
		sf_close(fd);
		result = 1;
	}

	return result;
}

void FileSndFile::asset_to_format()
{
	switch(asset->format)
	{
		case FILE_PCM:  fd_config.format = SF_FORMAT_RAW;  break;
		case FILE_WAV:  fd_config.format = SF_FORMAT_WAV;  break;
		case FILE_AU:   fd_config.format = SF_FORMAT_AU;   break;
		case FILE_AIFF: fd_config.format = SF_FORMAT_AIFF; break;
	}

// Not all of these are allowed in all sound formats.
// Raw can't be float.
	switch(asset->bits)
	{
		case BITSLINEAR8:
			if(asset->format == FILE_WAV)
				fd_config.format |= SF_FORMAT_PCM_U8;
			else
			if(asset->signed_)
				fd_config.format |= SF_FORMAT_PCM_S8;
			else
				fd_config.format |= SF_FORMAT_PCM_U8;
			break;

		case BITSLINEAR16:
// Only signed is supported
			fd_config.format |= SF_FORMAT_PCM_16;

			if(asset->byte_order || asset->format == FILE_WAV)
				fd_config.format |= SF_ENDIAN_LITTLE;
			else
				fd_config.format |= SF_ENDIAN_BIG;
			break;

		case BITSLINEAR24:
			fd_config.format |= SF_FORMAT_PCM_24;

			if(asset->byte_order || asset->format == FILE_WAV)
				fd_config.format |= SF_ENDIAN_LITTLE;
			else
				fd_config.format |= SF_ENDIAN_BIG;
			break;

		case BITSLINEAR32:
			fd_config.format |= SF_FORMAT_PCM_32;

			if(asset->byte_order || asset->format == FILE_WAV)
				fd_config.format |= SF_ENDIAN_LITTLE;
			else
				fd_config.format |= SF_ENDIAN_BIG;
			break;

		case BITSULAW: 
			fd_config.format |= SF_FORMAT_ULAW; 
			break;

		case BITSFLOAT: 
			fd_config.format |= SF_FORMAT_FLOAT; 
			break;

		case BITS_ADPCM: 
			if(fd_config.format == FILE_WAV)
				fd_config.format |= SF_FORMAT_MS_ADPCM;
			else
				fd_config.format |= SF_FORMAT_IMA_ADPCM; 
			fd_config.format |= SF_FORMAT_PCM_16;
			break;
	}

	fd_config.seekable = 1;
	fd_config.samplerate = asset->sample_rate;
	fd_config.channels  = asset->channels;
//printf("FileSndFile::asset_to_format %x %d %d\n", fd_config.format, fd_config.pcmbitwidth, fd_config.channels);
}

void FileSndFile::format_to_asset()
{
//printf("FileSndFile::format_to_asset 1\n");
// User supplies values if PCM
	if(asset->format == 0)
	{
		asset->byte_order = 0;
		asset->signed_ = 1;
		switch(fd_config.format & SF_FORMAT_TYPEMASK)
		{
			case SF_FORMAT_WAV:  
				asset->format = FILE_WAV;  
				asset->byte_order = 1;
				asset->header = 44;
				break;
			case SF_FORMAT_AIFF: asset->format = FILE_AIFF; break;
			case SF_FORMAT_AU:   asset->format = FILE_AU;   break;
			case SF_FORMAT_RAW:  asset->format = FILE_PCM;  break;
			case SF_FORMAT_PAF:  asset->format = FILE_SND;  break;
			case SF_FORMAT_SVX:  asset->format = FILE_SND;  break;
			case SF_FORMAT_NIST: asset->format = FILE_SND;  break;
		}

		switch(fd_config.format & SF_FORMAT_SUBMASK)
		{
			case SF_FORMAT_FLOAT: 
				asset->bits = BITSFLOAT; 
				break;
			case SF_FORMAT_ULAW: 
				asset->bits = BITSULAW; 
				break;
			case SF_FORMAT_IMA_ADPCM:
			case SF_FORMAT_MS_ADPCM:
				asset->bits = BITS_ADPCM;
				break;
			case SF_FORMAT_PCM_16:
				asset->signed_ = 1;
				asset->bits = 16;
				break;
			case SF_FORMAT_PCM_24:
				asset->signed_ = 1;
				asset->bits = 24;
				break;
			case SF_FORMAT_PCM_32:
				asset->signed_ = 1;
				asset->bits = 32;
				break;
			case SF_FORMAT_PCM_S8:
				asset->signed_ = 1;
				asset->bits = BITSLINEAR8;
				break;
			case SF_FORMAT_PCM_U8:
				asset->signed_ = 0;
				asset->bits = BITSLINEAR8;
				break;
		}

		switch(fd_config.format & SF_FORMAT_ENDMASK)
		{
			case SF_ENDIAN_LITTLE:
				asset->byte_order = 1;
				break;
			case SF_ENDIAN_BIG:
				asset->byte_order = 0;
				break;
		}

		asset->channels = fd_config.channels;
	}

	asset->audio_data = 1;
	asset->audio_length = fd_config.frames;
	if(!asset->sample_rate)
		asset->sample_rate = fd_config.samplerate;
//printf("FileSndFile::format_to_asset %x %d %d %x\n", fd_config.format & SF_FORMAT_TYPEMASK, fd_config.pcmbitwidth, fd_config.samples, fd_config.format & SF_FORMAT_SUBMASK);
//asset->dump();
}

int FileSndFile::open_file(int rd, int wr)
{
	int result = 0;

	if(rd)
	{
		if(asset->format == FILE_PCM)
		{
			asset_to_format();
			fd = sf_open(asset->path, SFM_READ, &fd_config);
// Already given by user
			if(fd) format_to_asset();
		}
		else
		{
			fd = sf_open(asset->path, SFM_READ, &fd_config);
// Doesn't calculate the length
			if(fd) format_to_asset();
		}
	}
	else
	if(wr)
	{
		asset_to_format();
		fd = sf_open(asset->path, SFM_WRITE, &fd_config);
	}

	if(!fd) 
	{
		result = 1;
		printf("FileSndFile::open_file: ");
		sf_perror(0);
	}

	return result;
}

int FileSndFile::close_file()
{
	if(fd) sf_close(fd);
	fd = 0;
	FileBase::close_file();
	fd_config.format = 0;
	return 0;
}

int FileSndFile::set_audio_position(int64_t sample)
{
// Commented out /* && psf->dataoffset */ in sndfile.c: 761
	if(sf_seek(fd, sample, SEEK_SET) < 0)
	{
		printf("FileSndFile::set_audio_position %lld: failed\n", (long long)sample);
		sf_perror(fd);
		return 1;
	}
	return 0;
}

int FileSndFile::read_samples(double *buffer, int64_t len)
{
	int result = 0;

// printf("FileSndFile::read_samples %d current_channel=%d current_sample=%ld len=%ld\n", 
// __LINE__, 
// file->current_channel, 
// file->current_sample, 
// len);

// Get temp buffer for interleaved channels
	if(len <= 0 || len > 1000000)
		printf("FileSndFile::read_samples len=%d\n", (int)len);

	if(!buffer)
		printf("FileSndFile::read_samples buffer=%p\n", buffer);

// printf("FileSndFile::read_samples %d\n", 
// __LINE__);

	if(temp_allocated && temp_allocated < len)
	{
		delete [] temp_double;
		temp_double = 0;
		temp_allocated = 0;
	}

// printf("FileSndFile::read_samples %d\n", 
// __LINE__);

	if(!temp_allocated)
	{
		temp_allocated = len;
		temp_double = new double[len * asset->channels];
	}

//printf("FileSndFile::read_samples %d\n", 
//__LINE__);

	result = !sf_read_double(fd, temp_double, len * asset->channels);

//printf("FileSndFile::read_samples %d\n", 
//__LINE__);

	if(result)
		printf("FileSndFile::read_samples fd=%p temp_double=%p len=%d asset=%p asset->channels=%d\n",
			fd, temp_double, (int)len, asset, asset->channels);

// Extract single channel
	for(int i = 0, j = file->current_channel; 
		i < len;
		i++, j += asset->channels)
	{
		buffer[i] = temp_double[j];
	}

// printf("FileSndFile::read_samples %d\n", 
// __LINE__);

	return result;
}

int FileSndFile::write_samples(double **buffer, int64_t len)
{
	int result = 0;

// Get temp buffer for interleaved channels
//printf("FileSndFile::write_samples %d\n", __LINE__);
	if(temp_allocated && temp_allocated < len)
	{
		temp_allocated = 0;
		delete [] temp_double;
		temp_double = 0;
	}

//printf("FileSndFile::read_samples 2\n");
	if(!temp_allocated)
	{
		temp_allocated = len;
		temp_double = new double[len * asset->channels];
	}

// Interleave channels
	for(int i = 0; i < asset->channels; i++)
	{
		for(int j = 0; j < len; j++)
		{
			double sample = buffer[i][j];
// Libsndfile does not limit values
//if(sample > 1.0 || sample < -1.0) printf("FileSndFile::write_samples %f\n", sample);
//printf("FileSndFile::write_samples %d %d\n", asset->bits, BITSFLOAT);
			if(asset->bits != BITSFLOAT) CLAMP(sample, -1.0, (32767.0 / 32768.0));
			temp_double[j * asset->channels + i] = sample;
		}
	}

	result = !sf_writef_double(fd, temp_double, len);
//printf("FileSndFile::write_samples %d\n", __LINE__);

	return result;
}

void FileSndFile::get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int option_type,
	    const char *locked_compressor)
{
	if(option_type == AUDIO_PARAMS)
	{
		SndFileConfig *window = new SndFileConfig(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

SndFileConfig::SndFileConfig(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	DP(250),
	DP(250))
{
	this->parent_window = parent_window;
	this->asset = asset;
}

SndFileConfig::~SndFileConfig()
{
	if(bits_popup)
	{
		lock_window("SndFileConfig::~SndFileConfig");
		delete bits_popup;
		unlock_window();
	}
}

void SndFileConfig::create_objects()
{
	int x = DP(10), y = DP(10);

	lock_window("SndFileConfig::create_objects");
	bits_popup = 0;
	switch(asset->format)
	{
		case FILE_WAV:
		case FILE_PCM:
		case FILE_AIFF:
			add_tool(new BC_Title(x, y, _("Compression:")));
			y += DP(25);
			if(asset->format == FILE_WAV)
				bits_popup = new BitsPopup(this, x, y, &asset->bits, 0, 0, 1, 1, 0);
			else
				bits_popup = new BitsPopup(this, x, y, &asset->bits, 0, 0, 0, 0, 0);
			y += DP(40);
			bits_popup->create_objects();
			break;
	}

	x = DP(10);
	if(asset->format != FILE_AU)
	{
		add_subwindow(new BC_CheckBox(x, y, &asset->dither, _("Dither")));
	}
	
	y += DP(30);
	if(asset->format == FILE_PCM)
	{
		add_subwindow(new BC_CheckBox(x, y, &asset->signed_, _("Signed")));
		y += DP(35);
		add_subwindow(new BC_Title(x, y, _("Byte order:")));
		add_subwindow(hilo = new SndFileHILO(this, x + DP(100), y));
		add_subwindow(lohi = new SndFileLOHI(this, x + DP(170), y));
	}
	
	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

int SndFileConfig::close_event()
{
	set_done(0);
	return 1;
}



SndFileHILO::SndFileHILO(SndFileConfig *gui, int x, int y)
 : BC_Radial(x, y, gui->asset->byte_order == BYTE_ORDER_HILO, _("Hi Lo"))
{
	this->gui = gui;
}
int SndFileHILO::handle_event()
{
	gui->asset->byte_order = BYTE_ORDER_HILO;
	gui->lohi->update(0);
	return 1;
}




SndFileLOHI::SndFileLOHI(SndFileConfig *gui, int x, int y)
 : BC_Radial(x, y, gui->asset->byte_order == BYTE_ORDER_LOHI, _("Lo Hi"))
{
	this->gui = gui;
}
int SndFileLOHI::handle_event()
{
	gui->asset->byte_order = BYTE_ORDER_LOHI;
	gui->hilo->update(0);
	return 1;
}


