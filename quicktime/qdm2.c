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
//#include "mpegaudio.h"
#include "qtffmpeg.h"
#include "quicktime.h"

#include <pthread.h>
#include <stdint.h>
#include <string.h>


extern int ffmpeg_initialized;
extern pthread_mutex_t ffmpeg_lock;


typedef struct
{
	int decoder_initialized;
	const AVCodec *decoder;
	AVCodecContext *decoder_context;
	
// Number of frames
	int frame_size;
	int max_frame_bytes;
// Input samples interleaved
	float *input_buffer;
// Number of samples allocated
	int input_allocated;
// Last sample decoded in the input buffer + 1
	int64_t input_end;
// Total samples in input buffer
	int input_size;
// Current write offset in input buffer
	int input_ptr;
	
	unsigned char *compressed_buffer;
	int compressed_size;
	int compressed_allocated;
	float *temp_buffer;

// Next chunk to decode sequentially
	int64_t current_chunk;
} quicktime_qdm2_codec_t;

#define MAX(x, y) ((x) > (y) ? (x) : (y))
// Default number of samples to allocate in work buffer
#define MPA_MAX_CODED_FRAME_SIZE 2048
#define OUTPUT_ALLOCATION MAX(0x100000, MPA_MAX_CODED_FRAME_SIZE * 2 * 2)





static int delete_codec(quicktime_audio_map_t *atrack)
{
	quicktime_qdm2_codec_t *codec = 
		((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->decoder_initialized)
	{
		avcodec_close(codec->decoder_context);
		free(codec->decoder_context);
	}

	if(codec->input_buffer) free(codec->input_buffer);
	if(codec->compressed_buffer) free(codec->compressed_buffer);
	if(codec->temp_buffer) free(codec->temp_buffer);

	free(codec);
}

void allocate_compressed(quicktime_qdm2_codec_t *codec, int size)
{
	if(size > codec->compressed_allocated)
	{
		codec->compressed_buffer = realloc(codec->compressed_buffer, size);
		codec->compressed_allocated = size;
	}
}

static int decode(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_qdm2_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
	int64_t current_position = track_map->current_position;
	int64_t end_position = current_position + samples;
	quicktime_frma_t *frma = &stsd_table->frma;
	int channels = track_map->channels;
	int i, j;
	int debug = 0;
	int64_t chunk_sample;
	


// Initialize decoder
	if(!codec->decoder_initialized)
	{
		pthread_mutex_lock(&ffmpeg_lock);
		if(!ffmpeg_initialized)
		{
			ffmpeg_initialized = 1;
//  			avcodec_init();
//			avcodec_register_all();
		}

		codec->decoder = avcodec_find_decoder(AV_CODEC_ID_QDM2);
		if(!codec->decoder)
		{
			printf("qdm2.c: decode: no ffmpeg decoder found.\n");
			return 1;
		}

		uint32_t samplerate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;

// allocate the codec and fill in header
		AVCodecContext *context = 
			codec->decoder_context = 
			avcodec_alloc_context3(codec->decoder);
		codec->decoder_context->sample_rate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;
		codec->decoder_context->channels = track_map->channels;

		if(frma->data && frma->data_size)
		{
			context->extradata = frma->data + 4;
			context->extradata_size = frma->data_size - 4;
		}

		if(file->cpus > 1)
		{
//			avcodec_thread_init(context, file->cpus);
// Not exactly user friendly.
			context->thread_count = file->cpus;
		}
	
		if(avcodec_open2(context, codec->decoder, 0) < 0)
		{
			printf("qdm2.c: decode: avcodec_open failed.\n");
			return 1;
		}
		pthread_mutex_unlock(&ffmpeg_lock);
		
		codec->input_buffer = calloc(sizeof(float),
			track_map->channels * OUTPUT_ALLOCATION);
		codec->input_allocated = OUTPUT_ALLOCATION;

		codec->decoder_initialized = 1;
	}


	if(samples > OUTPUT_ALLOCATION)
	{
		printf("qdm2: decode: can't decode more than %d samples at a time.\n",
			OUTPUT_ALLOCATION);
		return 1;
	}



	if(debug)
	{
		printf("qdm2 decode: current_position=%ld end_position=%ld input_size=%d input_end=%ld\n",
			current_position,
			end_position,
			codec->input_size,
			codec->input_end);
	}

// printf("qdm2 decode: current_position=%lld end_position=%lld chunk_sample=%lld chunk=%lld\n", 
// current_position, 
// end_position,
// chunk_sample,
// chunk);


	if(current_position < codec->input_end - codec->input_size ||
		current_position > codec->input_end)
	{
// Desired start point is outside existing range.  Reposition buffer pointer
// to start time of nearest chunk and start over.
		quicktime_chunk_of_sample(&chunk_sample, 
			&codec->current_chunk, 
			trak, 
			current_position);
		codec->input_size = 0;
		codec->input_ptr = 0;
		codec->input_end = chunk_sample;
	}

// Decode complete chunks until samples is reached
	int total_chunks = trak->mdia.minf.stbl.stco.total_entries;
	while(codec->input_end < end_position)
	{
		int64_t offset = quicktime_chunk_to_offset(file, 
			trak, 
			codec->current_chunk);
		int64_t max_offset = quicktime_chunk_to_offset(file, 
			trak, 
			codec->current_chunk + 1);
		quicktime_set_position(file, offset);
		allocate_compressed(codec, 3);

		if(debug)
		{
			printf("qdm2 decode: input_end=%ld chunk=%ld offset=0x%lx\n", 
				codec->input_end, 
				codec->current_chunk, 
				offset);
		}

// Read fragments of chunk
		while(1)
		{
// Hit next chunk of audio
			if(max_offset > offset && quicktime_position(file) >= max_offset) break;
			if(!quicktime_read_data(file, 
				codec->compressed_buffer + codec->compressed_size, 
				3))
				break;
			if(codec->compressed_buffer[codec->compressed_size] != 0x82)
			{
//				printf("qdm2: decode: position=0x%llx\n", quicktime_position(file));
				break;
			}
			int fragment_size = 3 + ((codec->compressed_buffer[codec->compressed_size + 1] << 8) |
				codec->compressed_buffer[codec->compressed_size + 2]);
// Sanity check
			if(fragment_size > OUTPUT_ALLOCATION) break;
// Expand compressed buffer
			allocate_compressed(codec, 
				codec->compressed_size + fragment_size + 1024);
			if(!quicktime_read_data(file, 
				codec->compressed_buffer + codec->compressed_size + 3, 
				fragment_size - 3))
				break;

			codec->compressed_size += fragment_size;

// Repeat this sequence until ffmpeg stops outputting samples
			if(!codec->temp_buffer)
				codec->temp_buffer = calloc(sizeof(float), OUTPUT_ALLOCATION);
//				int bytes_decoded = MPA_MAX_CODED_FRAME_SIZE;
			AVFrame *frame = av_frame_alloc();
			AVPacket *packet = av_packet_alloc();
			int got_frame = 0;
			packet->data = codec->compressed_buffer;
			packet->size = codec->compressed_size;
//				int result = avcodec_decode_audio4(codec->decoder_context, 
//					frame,
//					&got_frame,
//            		packet);
            int result = avcodec_send_packet(codec->decoder_context, packet);
			av_packet_free(&packet);

// Shift compressed buffer
// 				if(result > 0)
// 				{
// 					memcpy(codec->compressed_buffer,
// 						codec->compressed_buffer + result,
// 						codec->compressed_size - result);
// 					codec->compressed_size -= result;
// 				}
            codec->compressed_size = 0;

//printf("avcodec_decode_audio result=%d bytes_decoded=%d fragment_size=%d codec->compressed_size=%d\n", 
//result, bytes_decoded, fragment_size, codec->compressed_size);
/*
* static FILE *test = 0;
* if(!test) test = fopen("/tmp/debug", "w");
* fwrite(codec->temp_buffer, 1, bytes_decoded, test);
* fflush(test);
*/

            while(result == 0)
            {
                result = avcodec_receive_frame(codec->decoder_context, frame);
// transfer from frames to temp buffer
				int samples = quicktime_ffmpeg_get_audio(frame, 
					codec->temp_buffer);

// transfer from temp to ring buffer
				for(i = 0; i < samples; i++)
				{
					for(j = 0; j < channels; j++)
						codec->input_buffer[codec->input_ptr * channels + j] =
							codec->temp_buffer[i * channels + j];
					codec->input_ptr++;
					if(codec->input_ptr >= codec->input_allocated)
						codec->input_ptr = 0;
				}

// update position in stream
				codec->input_end += samples;
				codec->input_size += samples;
            }

			av_frame_free(&frame);
		}

		codec->current_chunk++;
		if(codec->current_chunk >= total_chunks) break;
	}

// Transfer from buffer to output
	int input_ptr = codec->input_ptr - (codec->input_end - current_position);
	if(input_ptr < 0) input_ptr += codec->input_allocated;
	if(output_i)
	{
		for(i = 0; i < samples; i++)
		{
			float value = codec->input_buffer[input_ptr * channels + channel];
			CLAMP(value, -1.0f, 1.0f);
			output_i[i] = (int16_t)(value * 32767);
			input_ptr++;
			if(input_ptr >= codec->input_allocated) input_ptr = 0;
		}
	}
	else
	if(output_f)
	{
		for(i = 0; i < samples; i++)
		{
			output_f[i] = codec->input_buffer[input_ptr * channels + channel];
			input_ptr++;
			if(input_ptr >= codec->input_allocated) input_ptr = 0;
		}
	}
	return 0;
}





void quicktime_init_codec_qdm2(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	codec_base->priv = calloc(1, sizeof(quicktime_qdm2_codec_t));
	codec_base->delete_acodec = delete_codec;
	codec_base->decode_audio = decode;
	codec_base->encode_audio = 0;
	codec_base->set_parameter = 0;
	codec_base->flush = 0;
	codec_base->fourcc = "QDM2";
	codec_base->title = "QDesign Music 2";
	codec_base->desc = "QDesign Music 2";
}




