
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

extern "C" 
{
#include "avcodec.h"
}

#include "clip.h"
#include "file.h"
#include "fileac3.h"
#include "language.h"
#include "mwindow.inc"




#include <string.h>

FileAC3::FileAC3(Asset *asset, File *file)
 : FileBase(asset, file)
{
    reset_parameters_derived();
}

FileAC3::~FileAC3()
{
	close_file();
}


FileAC3::FileAC3()
 : FileBase()
{
    reset_parameters_derived();
    ids.append(FILE_AC3);
    has_audio = 1;
    has_wr = 1;
}

FileBase* FileAC3::create(File *file)
{
    return new FileAC3(file->asset, file);
}

const char* FileAC3::formattostr(int format)
{
    switch(format)
    {
		case FILE_AC3:
			return AC3_NAME;
			break;
    }
    return 0;
}

const char* FileAC3::get_tag(int format)
{
    switch(format)
    {
		case FILE_AC3:
            return "ac3";
    }
    return 0;
}



int FileAC3::reset_parameters_derived()
{
	codec = 0;
	codec_context = 0;
	fd = 0;
	temp_raw = 0;
	temp_raw_size = 0;
	temp_raw_allocated = 0;
	temp_compressed = 0;
	compressed_allocated = 0;
    return 0;
}

void FileAC3::get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int option_type,
	    const char *locked_compressor)
{
	if(option_type == AUDIO_PARAMS)
	{

		AC3ConfigAudio *window = new AC3ConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileAC3::open_file(int rd, int wr)
{


	if(wr)
	{
//  		avcodec_init();
//		avcodec_register_all();
		codec = avcodec_find_encoder(AV_CODEC_ID_AC3);
		if(!codec)
		{
			fprintf(stderr, 
				"FileAC3::open_file codec not found.\n");
			return 1;
		}
		codec_context = avcodec_alloc_context3((AVCodec*)codec);
		((AVCodecContext*)codec_context)->bit_rate = asset->ac3_bitrate * 1000;
		((AVCodecContext*)codec_context)->sample_rate = asset->sample_rate;
		((AVCodecContext*)codec_context)->channels = asset->channels;
		if(avcodec_open2(((AVCodecContext*)codec_context), ((AVCodec*)codec), 0))
		{
			fprintf(stderr, 
				"FileAC3::open_file failed to open codec.\n");
			return 1;
		}

		if(!(fd = fopen(asset->path, "w")))
		{
			perror("FileAC3::open_file");
			return 1;
		}
	}
	else
	{
		if(!(fd = fopen(asset->path, "r")))
		{
			perror("FileAC3::open_file");
			return 1;
		}
	}




	return 0;
}

int FileAC3::close_file()
{
	if(codec_context)
	{
		avcodec_close(((AVCodecContext*)codec_context));
		free(codec_context);
		codec_context = 0;
		codec = 0;
	}
	if(fd)
	{
		fclose(fd);
		fd = 0;
	}
	if(temp_raw)
	{
		delete [] temp_raw;
		temp_raw = 0;
	}
	if(temp_compressed)
	{
		delete [] temp_compressed;
		temp_compressed = 0;
	}
	FileBase::close_file();
    return 0;
}

// Channel conversion matrices because ffmpeg encodes a
// different channel order than liba52 decodes.
// Each row is an output channel.
// Each column is an input channel.
// static int channels5[] = 
// {
// 	{ }
// };
// 
// static int channels6[] = 
// {
// 	{ }
// };


int FileAC3::write_samples(double **buffer, int64_t len)
{
// Convert buffer to encoder format
	if(temp_raw_size + len > temp_raw_allocated)
	{
		int new_allocated = temp_raw_size + len;
		int16_t *new_raw = new int16_t[new_allocated * asset->channels];
		if(temp_raw)
		{
			memcpy(new_raw, 
				temp_raw, 
				sizeof(int16_t) * temp_raw_size * asset->channels);
			delete [] temp_raw;
		}
		temp_raw = new_raw;
		temp_raw_allocated = new_allocated;
	}

// Allocate compressed data buffer
	if(temp_raw_allocated * asset->channels * 2 > compressed_allocated)
	{
		compressed_allocated = temp_raw_allocated * asset->channels * 2;
		delete [] temp_compressed;
		temp_compressed = new unsigned char[compressed_allocated];
	}

// Append buffer to temp raw
	int16_t *out_ptr = temp_raw + temp_raw_size * asset->channels;
	for(int i = 0; i < len; i++)
	{
		for(int j = 0; j < asset->channels; j++)
		{
			int sample = (int)(buffer[j][i] * 32767);
			CLAMP(sample, -32768, 32767);
			*out_ptr++ = sample;
		}
	}
	temp_raw_size += len;

	int frame_size = ((AVCodecContext*)codec_context)->frame_size;
	int output_size = 0;
	int current_sample = 0;
	for(current_sample = 0; 
		current_sample + frame_size <= temp_raw_size; 
		current_sample += frame_size)
	{
		AVFrame *frame = av_frame_alloc();
		frame->format = AV_SAMPLE_FMT_S16;
		frame->nb_samples = frame_size;
		for(int i = 0; i < asset->channels; i++)
		{
			frame->data[i] = new unsigned char[frame->nb_samples * sizeof(int16_t)];
			int16_t *out = (int16_t*)frame->data[i];
			for(int j = 0; j < frame->nb_samples; j++)
			{
				out[j] = *(temp_raw + (current_sample + j) * asset->channels + i);
			}
		}
		
        
        int result = avcodec_send_frame((AVCodecContext*)codec_context, frame);
        
// 		avcodec_encode_audio2(((AVCodecContext*)codec_context), 
// 			&packet,
//         	frame, 
// 			&got_packet);


// 		avcodec_encode_audio(
// 			((AVCodecContext*)codec_context), 
// 			temp_compressed + output_size, 
// 			compressed_allocated - output_size, 
//             temp_raw + current_sample * asset->channels);


		for(int i = 0; i < asset->channels; i++)
		{
			delete [] frame->data[i];
		}
		av_frame_free(&frame);


		AVPacket *packet = av_packet_alloc();
        while(result >= 0)
        {
            result = avcodec_receive_packet((AVCodecContext*)codec_context, packet);
            if(result >= 0)
            {
                memcpy(temp_compressed + output_size, packet->data, packet->size);
                output_size += packet->size;
                av_packet_unref(packet);
            }
//		packet.data = temp_compressed + output_size;
//		packet.size = compressed_allocated - output_size;
//		int got_packet = 0;
//		output_size += packet.size;
        }
        av_packet_free(&packet);
	}

// Shift buffer back
	memcpy(temp_raw,
		temp_raw + current_sample * asset->channels,
		(temp_raw_size - current_sample) * sizeof(int16_t) * asset->channels);
	temp_raw_size -= current_sample;

	int bytes_written = fwrite(temp_compressed, 1, output_size, fd);
	if(bytes_written < output_size)
	{
		perror("FileAC3::write_samples");
		return 1;
	}
	return 0;
}







AC3ConfigAudio::AC3ConfigAudio(BC_WindowBase *parent_window,
	Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	DP(500),
	BC_OKButton::calculate_h() + DP(100),
	DP(500),
	BC_OKButton::calculate_h() + DP(100),
	0,
	0,
	1)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

void AC3ConfigAudio::create_objects()
{
	int x = DP(10), y = DP(10);
	int x1 = DP(150);
	lock_window("AC3ConfigAudio::create_objects");
	add_tool(new BC_Title(x, y, "Bitrate (kbps):"));
	AC3ConfigAudioBitrate *bitrate;
	add_tool(bitrate = 
		new AC3ConfigAudioBitrate(this,
			x1, 
			y));
	bitrate->create_objects();

	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

int AC3ConfigAudio::close_event()
{
	set_done(0);
	return 1;
}






AC3ConfigAudioBitrate::AC3ConfigAudioBitrate(AC3ConfigAudio *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x,
 	y,
	DP(150),
	AC3ConfigAudioBitrate::bitrate_to_string(gui->string, gui->asset->ac3_bitrate))
{
	this->gui = gui;
}

char* AC3ConfigAudioBitrate::bitrate_to_string(char *string, int bitrate)
{
	sprintf(string, "%d", bitrate);
	return string;
}

void AC3ConfigAudioBitrate::create_objects()
{
	add_item(new BC_MenuItem("32"));
	add_item(new BC_MenuItem("40"));
	add_item(new BC_MenuItem("48"));
	add_item(new BC_MenuItem("56"));
	add_item(new BC_MenuItem("64"));
	add_item(new BC_MenuItem("80"));
	add_item(new BC_MenuItem("96"));
	add_item(new BC_MenuItem("112"));
	add_item(new BC_MenuItem("128"));
	add_item(new BC_MenuItem("160"));
	add_item(new BC_MenuItem("192"));
	add_item(new BC_MenuItem("224"));
	add_item(new BC_MenuItem("256"));
	add_item(new BC_MenuItem("320"));
	add_item(new BC_MenuItem("384"));
	add_item(new BC_MenuItem("448"));
	add_item(new BC_MenuItem("512"));
	add_item(new BC_MenuItem("576"));
	add_item(new BC_MenuItem("640"));
}

int AC3ConfigAudioBitrate::handle_event()
{
	gui->asset->ac3_bitrate = atol(get_text());
	return 1;
}



