#include <stdint.h>
#include <string.h>

#include "faac.h"

// The FAAD includes redefine the same symbols as if they're not intended to
// be used by the same program.
#undef MAIN
#undef SSR
#undef LTP


#include "funcprotos.h"
#include "faad.h"
#include "quicktime.h"


// Attempts to read more samples than this will crash
#define OUTPUT_ALLOCATION 0x100000
#define CLAMP(x, y, z) ((x) = ((x) <  (y) ? (y) : ((x) > (z) ? (z) : (x))))


typedef struct
{
// Decoder objects
    faacDecHandle decoder_handle;
    faacDecFrameInfo frame_info;
    faacDecConfigurationPtr decoder_config;
	int decoder_initialized;


	faacEncHandle encoder_handle;
	faacEncConfigurationPtr encoder_params;
// encoded channels always disagrees with MOOV channels
	int channels;
// Number of frames
	int frame_size;
	int max_frame_bytes;
// Interleaved samples
	float *input_buffer;
// Number of frames
	int input_size;
// Number of samples allocated
	int input_allocated;
	unsigned char *compressed_buffer;

// Encoding objects
	int bitrate;
	int quantizer_quality;
	int encoder_initialized;
} quicktime_mp4a_codec_t;






static int delete_codec(quicktime_audio_map_t *atrack)
{
	quicktime_mp4a_codec_t *codec = 
		((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->decoder_initialized)
	{
		faacDecClose(codec->decoder_handle);
	}

	if(codec->encoder_initialized)
	{
		faacEncClose(codec->encoder_handle);
		if(codec->compressed_buffer) free(codec->compressed_buffer);
		if(codec->input_buffer) free(codec->input_buffer);
	}

	free(codec);
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
	quicktime_mp4a_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int64_t current_position = track_map->current_position;
	int64_t end_position = current_position + samples;
	quicktime_vbr_t *vbr = &track_map->vbr;


// Initialize decoder
	if(!codec->decoder_initialized)
	{
		uint32_t samplerate = quicktime_sample_rate(file, track);
		quicktime_esds_t *esds = &trak->mdia.minf.stbl.stsd.table[0].esds;
// FAAD needs unsigned char here
		uint8_t channels = codec->channels = track_map->channels;
		quicktime_init_vbr(vbr, channels);
		codec->decoder_handle = faacDecOpen();
		codec->decoder_config = faacDecGetCurrentConfiguration(codec->decoder_handle);
		codec->decoder_config->outputFormat = FAAD_FMT_FLOAT;
//		codec->decoder_config->defSampleRate = 
//			trak->mdia.minf.stbl.stsd.table[0].sample_rate;

		faacDecSetConfiguration(codec->decoder_handle, codec->decoder_config);

// Always initialize from sample 0
		
		int done = 0;
		int total_read = 0;
		while(!done && total_read < 65536)
		{
//printf("decode %d vbr->sample=%ld\n", __LINE__, vbr->sample);
			if(quicktime_read_vbr(file, track_map))
			{
				break;
			}
		
			int result = 0;
			if(!codec->decoder_initialized)
			{
// always fails to decode any header
				result = faacDecInit(codec->decoder_handle,
					quicktime_vbr_input(vbr), 
					quicktime_vbr_input_size(vbr),
					&samplerate,
					&channels);
				codec->decoder_initialized = 1;
			}
			
			faacDecDecode(codec->decoder_handle, 
				&codec->frame_info,
            	quicktime_vbr_input(vbr), 
				quicktime_vbr_input_size(vbr));
			
//printf("decode %d size=%d\n", __LINE__, quicktime_vbr_input_size(vbr));
//quicktime_print_buffer("", quicktime_vbr_input(vbr), quicktime_vbr_input_size(vbr));

			if(codec->frame_info.error > 0)
			{
            	printf("decode %d: %s\n",
					__LINE__,
                	faacDecGetErrorMessage(codec->frame_info.error));
			}

			quicktime_shift_vbr(track_map, quicktime_vbr_input_size(vbr));
			total_read += quicktime_vbr_input_size(vbr);
			if(codec->frame_info.samples > 0)
			{
// reinitialize with encoded channel count
				if(codec->frame_info.channels != track_map->channels)
				{
printf("decode %d: AAC channel count=%d MPEG4 channel count=%d\n", 
__LINE__, 
codec->frame_info.channels,
track_map->channels);
					codec->channels = codec->frame_info.channels;
					quicktime_init_vbr(vbr, codec->channels);
				}
				
				if(codec->frame_info.samplerate != quicktime_sample_rate(file, track))
				{
printf("decode %d: AAC samplerate=%d MPEG4 samplerate=%ld\n", 
__LINE__, 
codec->frame_info.samplerate,
quicktime_sample_rate(file, track));
				}
				break;
			}


		}

	}

	if(quicktime_align_vbr(track_map, 
		samples))
	{
		return 1;
	}
	else
	{
// Decode until buffer is full
//printf("decode %d vbr=%p end=%d end_position=%d\n", __LINE__, vbr, quicktime_vbr_end(vbr), end_position);
		while(quicktime_vbr_end(vbr) < end_position)
		{
// Fill until min buffer size reached or EOF
/*
 * 			while(quicktime_vbr_input_size(vbr) <
 * 				FAAD_MIN_STREAMSIZE * track_map->channels)
 * 			{
 * 				if(quicktime_read_vbr(file, track_map)) break;
 * 			}
 */

			if(quicktime_read_vbr(file, track_map)) break;

			bzero(&codec->frame_info, sizeof(faacDecFrameInfo));
			float *sample_buffer = faacDecDecode(codec->decoder_handle, 
				&codec->frame_info,
            	quicktime_vbr_input(vbr), 
				quicktime_vbr_input_size(vbr));

        	if (codec->frame_info.error > 0)
        	{
//            	printf("decode mp4a: %s\n",
//                	faacDecGetErrorMessage(codec->frame_info.error));
        	}

// printf("decode %d channels=%d samplerate=%d samples=%d\n",
// __LINE__,
// codec->frame_info.channels,
// codec->frame_info.samplerate,
// codec->frame_info.samples);

/*
 * 
 * static FILE *test = 0;
 * if(!test) test = fopen("/tmp/test.aac", "w");
 * fwrite(quicktime_vbr_input(vbr), quicktime_vbr_input_size(vbr), 1, test);
 */


/*
 * static FILE *test = 0;
 * if(!test) test = fopen("/hmov/test.pcm", "w");
 * int i;
 * for(i = 0; i < codec->frame_info.samples; i++)
 * {
 * int16_t sample = (int)(sample_buffer[i] * 32767);
 * fwrite(&sample, 2, 1, test);
 * }
 * fflush(test);
 */

			quicktime_shift_vbr(track_map, quicktime_vbr_input_size(vbr));
			quicktime_store_vbr_float(track_map,
				sample_buffer,
				codec->frame_info.samples / codec->channels);
		}


// Transfer from buffer to output
		if(output_i)
			quicktime_copy_vbr_int16(vbr, 
				current_position, 
				samples, 
				output_i, 
				channel);
		else
		if(output_f)
			quicktime_copy_vbr_float(vbr, 
				current_position, 
				samples,
				output_f, 
				channel);
	}
	return 0;
}


static int encode(quicktime_t *file, 
	int16_t **input_i, 
	float **input_f, 
	int track, 
	long samples)
{
	int result = 0;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_mp4a_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int channels = quicktime_track_channels(file, track);
	int i, j, k;

	if(!codec->encoder_initialized)
	{
		unsigned long input_samples;
		unsigned long max_output_bytes;
		int sample_rate = quicktime_sample_rate(file, track);
		codec->encoder_initialized = 1;
		codec->encoder_handle = faacEncOpen(quicktime_sample_rate(file, track), 
			channels,
    	    &input_samples, 
			&max_output_bytes);

		codec->frame_size = input_samples / channels;
		codec->max_frame_bytes = max_output_bytes;
		codec->compressed_buffer = calloc(1, max_output_bytes);
		codec->encoder_params = faacEncGetCurrentConfiguration(codec->encoder_handle);

// Parameters from faac demo
		codec->encoder_params->aacObjectType = LOW;
		codec->encoder_params->mpegVersion = MPEG4;
		codec->encoder_params->useTns = 0;
		
		
// this doesn't work with 44.1
		if(sample_rate != 44100)
		{
			codec->encoder_params->allowMidside = 1;
		}
		else
		{
			codec->encoder_params->allowMidside = 0;
		}
		
		
		codec->encoder_params->inputFormat = FAAC_INPUT_FLOAT;
		codec->encoder_params->outputFormat = 0;
		codec->encoder_params->bitRate = codec->bitrate / channels;
//printf("mp4a encode %d codec->quantizer_quality=%d\n", __LINE__, codec->encoder_params->quantqual);
//		codec->encoder_params->quantqual = codec->quantizer_quality;
//		codec->encoder_params->quantqual = 0;
		codec->encoder_params->bandWidth = sample_rate / 2;
//		codec->encoder_params->bandWidth = 0;

		if(!faacEncSetConfiguration(codec->encoder_handle, codec->encoder_params))
		{
			fprintf(stderr, "encode: unsupported MPEG-4 Audio configuration!@#!@#\n");
			return 1;
		}



// Create esds header
		unsigned char *buffer;
		unsigned long buffer_size;
		faacEncGetDecoderSpecificInfo(codec->encoder_handle, 
			&buffer,
			&buffer_size);
 		quicktime_set_mpeg4_header(&trak->mdia.minf.stbl.stsd.table[0],
			buffer, 
			buffer_size);
// Quicktime player needs this.  But it doesn't work on IOS player.
//		trak->mdia.minf.stbl.stsd.table[0].version = 1;
//		trak->mdia.minf.stbl.stsd.table[0].compression_id = 0xfffe;
	}


// Stack new audio at end of old audio
	int new_allocation = codec->input_size + samples;
	if(new_allocation > codec->input_allocated)
	{
		codec->input_buffer = realloc(codec->input_buffer, 
			new_allocation * 
			sizeof(float) * 
			channels);
		codec->input_allocated = new_allocation;
	}

	float *output = (float*)codec->input_buffer + codec->input_size * channels;
	if(input_f)
	{
		for(i = 0; i < samples; i++)
		{
			for(j = 0; j < channels; j++)
			{
				*output++ = input_f[j][i] * 32767;
			}
		}
	}
	else
	if(input_i)
	{
		for(i = 0; i < samples; i++)
		{
			for(j = 0; j < channels; j++)
			{
				*output++ = (float)input_i[j][i];
			}
		}
	}

	codec->input_size += samples;


	for(i = 0; i + codec->frame_size < codec->input_size; i += codec->frame_size)
	{

// static FILE *fd = 0;
// if(!fd)
// {
// fd = fopen("/tmp/x.pcm", "w");
// }
// for(j = 0; j < codec->frame_size * channels; j++)
// {
// float *sample = codec->input_buffer + i * channels + j;
// int16_t temp = *sample;
// fwrite(&temp, 2, 1, fd);
// }

		int bytes = faacEncEncode(codec->encoder_handle,
                (int32_t*)(codec->input_buffer + i * channels),
                codec->frame_size * channels,
                codec->compressed_buffer,
                codec->max_frame_bytes);

// printf("encode 1 %lld %d %d\n", 
// track_map->current_position,
// codec->frame_size, 
// bytes);

// Write out the packet
		if(bytes)
		{
			quicktime_write_vbr_frame(file, 
				track,
				codec->compressed_buffer,
				bytes,
				codec->frame_size);
		}
	}

	for(j = i * channels, k = 0; j < codec->input_size * channels; j++, k++)
	{
		codec->input_buffer[k] = codec->input_buffer[j];
	}
	codec->input_size -= i;  

	return result;
}



static void flush(quicktime_t *file, int track)
{
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_mp4a_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int channels = quicktime_track_channels(file, track);
	int i;
	if(codec->encoder_initialized)
	{
		for(i = 0; 
			i < codec->input_size && 
				i + codec->frame_size < codec->input_allocated; 
			i += codec->frame_size)
		{
			int bytes = faacEncEncode(codec->encoder_handle,
                	(int32_t*)(codec->input_buffer + i * channels),
                	codec->frame_size * channels,
                	codec->compressed_buffer,
                	codec->max_frame_bytes);

/*
 * printf("flush 1 %d %d %d\n", 
 * codec->encoder_params->bitRate, 
 * bytes, 
 * codec->max_frame_bytes);
 */
// Write out the packet
			if(bytes)
			{
				quicktime_write_vbr_frame(file, 
					track,
					codec->compressed_buffer,
					bytes,
					codec->frame_size);
			}
		}
	}
}


static int set_parameter(quicktime_t *file, 
	int track, 
	char *key, 
	void *value)
{
	quicktime_audio_map_t *atrack = &(file->atracks[track]);
	char *compressor = quicktime_compressor(atrack->track);

	if(quicktime_match_32(compressor, QUICKTIME_MP4A))
	{
		quicktime_mp4a_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
		if(!strcasecmp(key, "mp4a_bitrate"))
		{
			codec->bitrate = *(int*)value;
		}
		else
		if(!strcasecmp(key, "mp4a_quantqual"))
		{
			codec->quantizer_quality = *(int*)value;
		}
	}
	return 0;
}



void quicktime_init_codec_mp4a(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_mp4a_codec_t *codec;
	codec_base->priv = calloc(1, sizeof(quicktime_mp4a_codec_t));
	codec_base->delete_acodec = delete_codec;
	codec_base->decode_audio = decode;
	codec_base->encode_audio = encode;
	codec_base->set_parameter = set_parameter;
	codec_base->flush = flush;
	codec_base->fourcc = QUICKTIME_MP4A;
	codec_base->title = "MPEG4 audio";
	codec_base->desc = "Audio section of MPEG4 standard";

	codec = (quicktime_mp4a_codec_t*)codec_base->priv;
// Default encoding parameters here
	codec->bitrate = 256000;
	codec->quantizer_quality = 100;
}




