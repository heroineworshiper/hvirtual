/*
 * Quicktime 4 Linux
 * Copyright (C) 1997-2022 Adam Williams <broadcast at earthling dot net>
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

#include "avcodec.h"
#include "colormodels.h"
#include "funcprotos.h"

#include "quicktime.h"
#include "wma.h"
#include "qtffmpeg.h"
#include <pthread.h>
#include <string.h>

/* McRowesoft media player audio */
/* WMA derivatives */

typedef struct
{
// Interleaved sample output
	float *work_buffer;
// Number of first sample in output relative to file
	int64_t output_position;
// Number of samples in output buffer
	long output_size;
// Number of samples allocated in output buffer
	long output_allocated;
	char *packet_buffer;
	int packet_allocated;
// Current reading position in file
	int64_t chunk;



	int ffmpeg_id;
    const AVCodec *decoder;
	AVCodecContext *decoder_context;
	int decode_initialized;
} quicktime_wma_codec_t;



// Default number of samples to allocate in work buffer
#define OUTPUT_ALLOCATION 0x100000

static int delete_codec(quicktime_audio_map_t *atrack)
{
	quicktime_wma_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->decode_initialized)
	{
		pthread_mutex_lock(&ffmpeg_lock);
		avcodec_close(codec->decoder_context);
		free(codec->decoder_context);
		pthread_mutex_unlock(&ffmpeg_lock);
		codec->decode_initialized = 0;
	}

	if(codec->work_buffer)
		free(codec->work_buffer);
	if(codec->packet_buffer)
		free(codec->packet_buffer);
	free(codec);
	return 0;
}



static int init_decode(quicktime_audio_map_t *track_map,
	quicktime_wma_codec_t *codec)
{
	if(!codec->decode_initialized)
	{
		quicktime_trak_t *trak = track_map->track;
		pthread_mutex_lock(&ffmpeg_lock);
		if(!ffmpeg_initialized)
		{
			ffmpeg_initialized = 1;
//			avcodec_init();
//			avcodec_register_all();
		}

		codec->decoder = avcodec_find_decoder(codec->ffmpeg_id);
		if(!codec->decoder)
		{
			printf("init_decode: avcodec_find_decoder returned NULL.\n");
			return 1;
		}
		codec->decoder_context = avcodec_alloc_context3(codec->decoder);
		codec->decoder_context->sample_rate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;
		codec->decoder_context->channels = track_map->channels;
		if(avcodec_open2(codec->decoder_context, codec->decoder, 0) < 0)
		{
			printf("init_decode: avcodec_open failed.\n");
			return 1;
		}
		pthread_mutex_unlock(&ffmpeg_lock);

		codec->work_buffer = malloc(sizeof(float) * 
			track_map->channels * 
			OUTPUT_ALLOCATION);
		codec->output_allocated = OUTPUT_ALLOCATION;
	}
	return 0;
}

static int decode(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_wma_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	quicktime_trak_t *trak = track_map->track;
	long current_position = track_map->current_position;
	long end_position = current_position + samples;
	int try = 0;
	int result = 0;
	int i, j;
	int sample_size = sizeof(float) * track_map->channels;

	if(output_i) bzero(output_i, sizeof(int16_t) * samples);
	if(output_f) bzero(output_f, sizeof(float) * samples);

	if(samples > OUTPUT_ALLOCATION)
		printf("decode: can't read more than %d samples at a time.\n", OUTPUT_ALLOCATION);

	result = init_decode(track_map, codec);
	if(result) return 1;

// Seeked outside output buffer's range or not initialized: restart
	if(current_position < codec->output_position ||
		current_position > codec->output_position + codec->output_size ||
		!codec->decode_initialized)
	{
		quicktime_chunk_of_sample(&codec->output_position, 
			&codec->chunk, 
			trak, 
			current_position);

//printf("decode 1 %lld %d\n", codec->output_position, codec->chunk);
// We know the first mp3 packet in the chunk has a pcm_offset from the encoding.
		codec->output_size = 0;
		codec->decode_initialized = 1;
	}

// Decode chunks until output buffer covers requested range
	while(codec->output_position + codec->output_size <
		current_position + samples &&
		try < 256)
	{
// Load chunk into work buffer
		int64_t chunk_offset = 0;
		int chunk_samples = quicktime_chunk_samples(trak, codec->chunk);
		int chunk_size = quicktime_chunk_bytes(file, 
			&chunk_offset,
			codec->chunk, 
			trak);
// Getting invalid numbers for this
//		int max_samples = chunk_samples * 2;
		int max_samples = 32768;
		int max_bytes = max_samples * sample_size;
		int bytes_decoded = 0;
//printf("decode 2 %x %llx %llx\n", chunk_size, chunk_offset, chunk_offset + chunk_size);

// Allocate packet buffer
		if(codec->packet_allocated < chunk_size && 
			codec->packet_buffer)
		{
			free(codec->packet_buffer);
			codec->packet_buffer = 0;
		}

		if(!codec->packet_buffer)
		{
			codec->packet_buffer = calloc(1, chunk_size);
			codec->packet_allocated = chunk_size;
		}

// Allocate work buffer
		if(max_bytes + codec->output_size * sample_size > codec->output_allocated * sample_size)
		{
			float *new_output = calloc(1, max_bytes + codec->output_size * sample_size);
			if(codec->work_buffer)
			{
				memcpy(new_output, codec->work_buffer, codec->output_size * sample_size);
				free(codec->work_buffer);
			}
			codec->work_buffer = new_output;
			codec->output_allocated = max_bytes + codec->output_size * sample_size;
		}

		quicktime_set_position(file, chunk_offset);
		result = !quicktime_read_data(file, codec->packet_buffer, chunk_size);
		if(result) break;

// Decode chunk into work buffer.
		pthread_mutex_lock(&ffmpeg_lock);
		AVFrame *frame = av_frame_alloc();
		AVPacket *packet = av_packet_alloc();
		packet->data = codec->packet_buffer;
		packet->size = chunk_size;
		int got_frame = 0;
// 		result = avcodec_decode_audio4(codec->decoder_context, 
// 			frame, 
//             &got_frame,
//             packet);
        result = avcodec_send_packet(codec->decoder_context, packet);
		av_packet_free(&packet);


        while(result >= 0)
        {
            result = avcodec_receive_frame(codec->decoder_context, frame);

// 		    if(got_frame)
// 		    {
// 			    bytes_decoded = frame->nb_samples * sample_size;
// 		    }
// 		    else
// 		    {
// 			    bytes_decoded = 0;
// 		    }

            if(result == 0)
            {
// transfer from frame to work buffer
		        int samples = quicktime_ffmpeg_get_audio(frame, 
    			    codec->work_buffer + 
	    			    (current_position - codec->output_position) * track_map->channels);



		        av_frame_free(&frame);
		        pthread_mutex_unlock(&ffmpeg_lock);



		        if(samples <= 0)
		        {
			        try++;
		        }
		        else
		        {
			        if(codec->output_size * sample_size + samples * sample_size > codec->output_allocated * sample_size)
			        {
				        printf("decode: FYI: bytes_decoded=%ld is greater than output_allocated=%ld\n",
					        codec->output_size * sample_size + bytes_decoded,
					        codec->output_allocated);
			        }
			        codec->output_size += samples;
			        try = 0;
		        }
            }
            else
            {
		        try++;
            }
        }
	    codec->chunk++;
	}

//printf("decode 15 %d %lld %d\n", try, codec->output_position, codec->output_size);
// Transfer to output
	if(output_i)
	{
		float *pcm_ptr = codec->work_buffer + 
			(current_position - codec->output_position) * track_map->channels +
			channel;
		for(i = current_position - codec->output_position, j = 0;
			j < samples && i < codec->output_size;
			j++, i++)
		{
			float value = *pcm_ptr;
			CLAMP(value, -1.0f, 1.0f);
			output_i[j] = (int16_t)(value * 32767);
			pcm_ptr += track_map->channels;
		}
	}
	else
	if(output_f)
	{
		float *pcm_ptr = codec->work_buffer + 
			(current_position - codec->output_position) * track_map->channels +
			channel;
		for(i = current_position - codec->output_position, j = 0;
			j < samples && i < codec->output_size;
			j++, i++)
		{
			output_i[j] = *pcm_ptr;
			pcm_ptr += track_map->channels;
		}
	}

// Delete excess output
	if(codec->output_size > OUTPUT_ALLOCATION)
	{
		int sample_diff = codec->output_size - OUTPUT_ALLOCATION;
		int byte_diff = sample_diff * sample_size;
		memmove(codec->work_buffer,
			codec->work_buffer + byte_diff,
			OUTPUT_ALLOCATION * sample_size);
		codec->output_size -= sample_diff;
		codec->output_position += sample_diff;
	}

	return 0;
}







static void init_codec_common(quicktime_audio_map_t *atrack)
{
	quicktime_wma_codec_t *codec;
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;

/* Init public items */
	codec_base->delete_acodec = delete_codec;
	codec_base->decode_audio = decode;
	

/* Init private items */
	codec = codec_base->priv = calloc(1, sizeof(quicktime_wma_codec_t));
}


void quicktime_init_codec_wmav1(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_wma_codec_t *codec;
	init_codec_common(atrack);

	codec = codec_base->priv;
	codec_base->fourcc = QUICKTIME_WMA;
	codec_base->title = "Win Media Audio 1";
	codec_base->desc = "Win Media Audio 1";
	codec_base->wav_id = 0x160;
	codec->ffmpeg_id = AV_CODEC_ID_WMAV1;
}


void quicktime_init_codec_wmav2(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_wma_codec_t *codec;
	init_codec_common(atrack);
	
	codec = codec_base->priv;
	codec_base->fourcc = QUICKTIME_WMA;
	codec_base->title = "Win Media Audio 2";
	codec_base->desc = "Win Media Audio 2";
	codec_base->wav_id = 0x161;
	codec->ffmpeg_id = AV_CODEC_ID_WMAV2;
}
