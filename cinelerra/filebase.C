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
#include "byteorder.h"
#include "colormodels.h"
#include "file.h"
#include "filebase.h"
#include "overlayframe.h"
#include "sizes.h"

#include <stdlib.h>
#include <string.h>

FileBase::FileBase(Asset *asset, File *file)
{
//printf("FileBase::FileBase %d this=%p\n", __LINE__, this);
// reset_parameters_derived is not available until after the constructor
	reset_parameters();
	this->file = file;
	this->asset = asset;
	internal_byte_order = get_byte_order();
	init_ima4();
	overlayer = new OverlayFrame;
}

FileBase::FileBase()
{
	reset_parameters();
    overlayer = 0;
}

FileBase::~FileBase()
{
	close_file();
	if(row_pointers_in) delete [] row_pointers_in;
	if(row_pointers_out) delete [] row_pointers_out;
	if(float_buffer) delete [] float_buffer;
	if(overlayer) delete overlayer;
	delete_ima4();
}

int FileBase::check_sig(File *file, const uint8_t *test_data)
{
    printf("FileBase::check_sig %d virtual function called\n", __LINE__);
    return 0;
}

FileBase* FileBase::create(File *file)
{
    printf("FileBase::create %d virtual function called\n", __LINE__);
    return 0;
}

int FileBase::get_best_colormodel(Asset *asset, int driver)
{
    return BC_RGB888;
}

void FileBase::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int option_type,
	const char *locked_compressor)
{
}

const char* FileBase::formattostr(int format)
{
    return 0;
}

const char* FileBase::get_tag(int format)
{
    return 0;
}



int FileBase::close_file()
{
	if(row_pointers_in) delete [] row_pointers_in;
	if(row_pointers_out) delete [] row_pointers_out;
	if(float_buffer) delete [] float_buffer;
	

	if(pcm_history)
	{
		for(int i = 0; i < history_channels; i++)
			delete [] pcm_history[i];
		delete [] pcm_history;
	}


	close_file_derived();
	reset_parameters();
	delete_ima4();
    return 0;
}

void FileBase::update_pcm_history(int64_t len)
{
	decode_start = 0;
	decode_len = 0;

    if(!asset) return;

	if(!pcm_history)
	{
		history_channels = asset->channels;
		pcm_history = new double*[history_channels];
		for(int i = 0; i < history_channels; i++)
			pcm_history[i] = new double[HISTORY_MAX];
		history_start = 0;
		history_size = 0;
		history_allocated = HISTORY_MAX;
	}
	

//printf("FileBase::update_pcm_history current_sample=%lld history_start=%lld history_size=%lld\n",
//file->current_sample,
//history_start,
//history_size);
// Restart history.  Don't bother shifting history back.
	if(file->current_sample < history_start ||
		file->current_sample > history_start + history_size)
	{
		history_size = 0;
		history_start = file->current_sample;
		decode_start = file->current_sample;
		decode_len = len;
	}
	else
// Shift history forward to make room for new samples
	if(file->current_sample > history_start + HISTORY_MAX)
	{
		int diff = file->current_sample - (history_start + HISTORY_MAX);
		for(int i = 0; i < asset->channels; i++)
		{
			double *temp = pcm_history[i];
			memmove(temp, temp + diff, (history_size - diff) * sizeof(double));
		}

		history_start += diff;
		history_size -= diff;

// Decode more data
		decode_start = history_start + history_size;
		decode_len = file->current_sample + len - (history_start + history_size);
	}
	else
// Starting somewhere in the buffer
	{
		decode_start = history_start + history_size;
		decode_len = file->current_sample + len - (history_start + history_size);
	}
}

void FileBase::append_history(float **new_data, int len)
{
	allocate_history(len);

	for(int i = 0; i < history_channels; i++)
	{
		double *output = pcm_history[i] + history_size;
		float *input = new_data[i];
		for(int j = 0; j < len; j++)
			*output++ = *input++;
	}

	history_size += len;
	decode_end += len;
}

void FileBase::append_history(short *new_data, int len)
{
	allocate_history(len);

	for(int i = 0; i < history_channels; i++)
	{
		double *output = pcm_history[i] + history_size;
		short *input = new_data + i;
		for(int j = 0; j < len; j++)
		{
			*output++ = (double)*input / 32768;
			input += history_channels;
		}
	}

	history_size += len;
	decode_end += len;
}

void FileBase::read_history(double *dst,
	int64_t start_sample, 
	int channel,
	int64_t len)
{
	if(start_sample - history_start + len > history_size)
		len = history_size - (start_sample - history_start);
//printf("FileBase::read_history start_sample=%lld history_start=%lld history_size=%lld len=%lld\n", 
//start_sample, history_start, history_size, len);
	double *input = pcm_history[channel] + start_sample - history_start;
	for(int i = 0; i < len; i++)
	{
		*dst++ = *input++;
	}
}

void FileBase::allocate_history(int len)
{
	if(history_size + len > history_allocated)
	{
		double **temp = new double*[history_channels];

		for(int i = 0; i < history_channels; i++)
		{
			temp[i] = new double[history_size + len];
			memcpy(temp[i], pcm_history[i], history_size * sizeof(double));
			delete [] pcm_history[i];
		}

		delete [] pcm_history;
		pcm_history = temp;
		history_allocated = history_size + len;
	}
}

int64_t FileBase::get_history_sample()
{
	return history_start + history_size;
}

int FileBase::set_dither()
{
	dither = 1;
	return 0;
}

int FileBase::reset_parameters()
{
    asset = 0;
    has_audio = 0;
    has_video = 0;
    has_wrapper = 0;
    has_wr = 0;
    has_rd = 0;

	dither = 0;
	float_buffer = 0;
	row_pointers_in = 0;
	row_pointers_out = 0;
	prev_buffer_position = -1;
	prev_frame_position = -1;
	prev_len = 0;
	prev_bytes = 0;
	prev_track = -1;
	prev_layer = -1;
	ulawtofloat_table = 0;
	floattoulaw_table = 0;
	pcm_history = 0;
	history_start = 0;
	history_size = 0;
	history_allocated = 0;
	history_channels = 0;
	decode_end = 0;

	delete_ulaw_tables();
// reset_parameters_derived is not available until after the constructor
	reset_parameters_derived();
    return 0;
}

int FileBase::reset_parameters_derived()
{
//    printf("FileBase::reset_parameters_derived %d\n", __LINE__);
    return 0;
}


int FileBase::get_mode(char *mode, int rd, int wr)
{
	if(rd && !wr) sprintf(mode, "rb");
	else
	if(!rd && wr) sprintf(mode, "wb");
	else
	if(rd && wr)
	{
		int exists = 0;
		FILE *stream;

		if(stream = fopen(asset->path, "rb")) 
		{
			exists = 1; 
			fclose(stream); 
		}

		if(exists) sprintf(mode, "rb+");
		else
		sprintf(mode, "wb+");
	}
    return 0;
}










// ======================================= audio codecs

int FileBase::get_video_buffer(unsigned char **buffer, int depth)
{
// get a raw video buffer for writing or compression by a library
	if(!*buffer)
	{
// Video compression is entirely done in the library.
		int64_t bytes = asset->width * asset->height * depth;
		*buffer = new unsigned char[bytes];
	}
	return 0;
}

int FileBase::get_row_pointers(unsigned char *buffer, unsigned char ***pointers, int depth)
{
// This might be fooled if a new VFrame is created at the same address with a different height.
	if(*pointers && (*pointers)[0] != &buffer[0])
	{
		delete [] *pointers;
		*pointers = 0;
	}

	if(!*pointers)
	{
		*pointers = new unsigned char*[asset->height];
		for(int i = 0; i < asset->height; i++)
		{
			(*pointers)[i] = &buffer[i * asset->width * depth / 8];
		}
	}
    return 0;
}

int FileBase::match4(const char *in, const char *out)
{
	if(in[0] == out[0] &&
		in[1] == out[1] &&
		in[2] == out[2] &&
		in[3] == out[3])
		return 1;
	else
		return 0;
}


int64_t FileBase::get_memory_usage()
{
//printf("FileBase::get_memory_usage %d\n", __LINE__);
	if(pcm_history) 
		return history_allocated * 
			history_channels * 
			sizeof(double);
	return 0;
}

int64_t FileBase::purge_cache()
{
    return 0;
}



