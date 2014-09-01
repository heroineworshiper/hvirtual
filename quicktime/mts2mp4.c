#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>


#include "avcodec.h"
#include "libmpeg3.h"
#include "quicktime.h"

// This converts an AVC-HD transport stream to an mp4 Quicktime movie without
// re-encoding the video but does re-encode the audio.



// Testing the ffmpeg decoder:
// ffmpeg -i ~mov/saved/test.m2ts -an /tmp/test.mp4


// Override the video format
//#define OVERRIDE_FORMAT

//#define MAX_FRAMES 1000

// Use NAL_AUD to denote the start of frame
#define USE_NAL_AUD

// Encode the audio.  Better to use ffmpeg for this.
//#define ENCODE_AUDIO


double video_frame_rate = 24000.0 / 1001;
int video_width = 1920;
int video_height = 1080;
int audio_channels = 6;
int audio_samplerate = 48000;
int audio_bitrate = 768000;
int audio_track = 0;

#define AUDIO_BUFFER_SIZE       0x10000
#define BUFFER_SIZE             0x100000
#define MAX_FRAME_SIZE          0x400000
#define NAL_SLICE               0x01
#define NAL_DPA                 0x02
#define NAL_DPB                 0x03
#define NAL_DPC                 0x04
#define NAL_IDR_SLICE           0x05
#define NAL_SEI                 0x06
#define NAL_SPS                 0x07
#define NAL_PPS                 0x08
#define NAL_AUD                 0x09
#define NAL_END_SEQUENCE        0x0a
#define NAL_END_STREAM          0x0b
#define NAL_FILLER_DATA         0x0c
#define NAL_SPS_EXT             0x0d
#define NAL_AUXILIARY_SLICE     0x13

#define NAL_CODE_SIZE            4




unsigned char *buffer = 0;
unsigned char *video_in_buffer = 0;
unsigned char *frame_buffer = 0;
float **audio_buffer = 0;
int frame_buffer_size = 0;
int buffer_size = 0;
int64_t bytes_written = 0;
int frames_written = 0;
int64_t samples_written = 0;
// Last frame when we wrote audio
int next_audio_frame = 0;
unsigned char *ptr = 0;
unsigned char *end = 0;
unsigned char *frame_end = 0;
// Set to 1 after a sequence header is detected and after the previous frame is written.
int is_keyframe = 0;
// This tells it when it doesn't need to decode sequence headers anymore.
int got_header = 0;
// This tells it to not flush the frame at the next picture header
int got_seq_header = 0;
quicktime_t *quicktime_fd = 0;
// Use 2 libmpeg3 instances because while video is direct copied, audio must
// be decoded.
mpeg3_t *mpegv_fd = 0;
mpeg3_t *mpega_fd = 0;
int video_eof = 0;
int audio_eof = 0;
char *in_path = 0;
char *out_path = 0;
int pass = 0;
AVCodec *decoder = 0;
AVCodecContext *decoder_context = 0;









void write_frame()
{
	int result = 0;
	int i;

// Shift data into frame buffer
	frame_buffer_size = frame_end - buffer;
	memcpy(frame_buffer, buffer, frame_buffer_size);





/*
 * if(
 * buffer[0] == 0xc2 &&
 * buffer[1] == 0x5c &&
 * buffer[2] == 0x43 &&
 * buffer[3] == 0x49 &&
 * buffer[4] == 0x95 &&
 * buffer[5] == 0xc4 &&
 * buffer[6] == 0xa6 &&
 * buffer[7] == 0x2b)
 * printf("write_frame %d\n", __LINE__);
 */

// Shift input buffer
	buffer_size = end - frame_end;
	memcpy(buffer, frame_end, buffer_size);

// Seek to end of this NAL
	ptr -= (frame_end - buffer);
	end = buffer + buffer_size;

	if(!frame_buffer_size) return;




/*
 * static int debug_count = 0;
 * char string[1024];
 * sprintf(string, "test%06d", debug_count++);
 * printf("write_frame %d: %s\n", __LINE__, string);
 * FILE *test = fopen(string, "w");
 * fwrite(frame_buffer, frame_buffer_size, 1, test);
 * fclose(test);
 */


	
	int got_picture = 0;
	AVFrame picture;


// Skip frames until first keyframe
/*
 * 	if(!bytes_written && !is_keyframe)
 * 	{
 * 	}
 * 	else
 */
	if(pass == 0)
	{
// Decode header only

printf("write_frame %d pass=%d %d\n", __LINE__, pass, frame_buffer_size);

/*
* static FILE *test = 0;
* if(!test) test = fopen("test", "w");
* fwrite(buffer, output_size, 1, test);
*/

		avcodec_get_frame_defaults(&picture);
		avcodec_decode_video(decoder_context, 
			&picture, 
			&got_picture, 
			buffer, 
			frame_buffer_size);


printf("write_frame %d %d\n", __LINE__, frame_buffer_size);


		if(decoder_context->width > 0)
		{
			video_width = decoder_context->width;
			video_height = decoder_context->height;
			video_frame_rate = (double)decoder_context->time_base.den / 
				decoder_context->time_base.num / 
				2;
			got_header = 1;
printf("%s format:\nwidth=%d\nheight=%d\nframerate=%f\naudio_track=%d\nsamplerate=%d\nchannels=%d\n", 
in_path,
video_width, 
video_height,
video_frame_rate,
audio_track,
audio_samplerate,
audio_channels);
		}

	}
	else
	{
/*
* printf("write_frame: %s at offset %llx\n", 
* is_keyframe ? "Keyframe" : "Frame", 
* bytes_written);
*/
		if(!quicktime_fd)
		{
			quicktime_fd = quicktime_open(out_path, 0, 1);
			if(!quicktime_fd)
			{
				fprintf(stderr, 
					"write_frame: Failed to open %s for writing\n",
					out_path);
				exit(1);
			}

#ifdef ENCODE_AUDIO
			quicktime_set_audio(quicktime_fd, 
				audio_channels, 
				audio_samplerate, 
				16, 
				QUICKTIME_MP4A);
			quicktime_set_parameter(quicktime_fd, "mp4a_bitrate", &audio_bitrate);
#endif

			quicktime_set_video(quicktime_fd, 
				1, 
				video_width, 
				video_height,
				video_frame_rate,
				QUICKTIME_H264);
		}



// Convert NAL codes to AVC format
		unsigned char *ptr2 = frame_buffer + NAL_CODE_SIZE;
		unsigned char *end2 = frame_buffer + frame_buffer_size;
		unsigned char *last_start = frame_buffer;
		int nal_size;
		int total_nals = 1;
		while(ptr2 < end2 + NAL_CODE_SIZE)
		{
// Start of next NAL code
			if(ptr2[0] == 0x00 &&
				ptr2[1] == 0x00 &&
				ptr2[2] == 0x00 &&
				ptr2[3] == 0x01)
			{
				nal_size = ptr2 - last_start - 4;
				last_start[0] = (nal_size & 0xff000000) >> 24;
				last_start[1] = (nal_size & 0xff0000) >> 16;
				last_start[2] = (nal_size & 0xff00) >> 8;
				last_start[3] = (nal_size & 0xff);
				last_start = ptr2;
				ptr2 += 4;
				total_nals++;
			}
			else
				ptr2++;
		}

//printf("write_frame total_nals=%d\n", total_nals);
// Last NAL in frame
		nal_size = end2 - last_start - 4;
		last_start[0] = (nal_size & 0xff000000) >> 24;
		last_start[1] = (nal_size & 0xff0000) >> 16;
		last_start[2] = (nal_size & 0xff00) >> 8;
		last_start[3] = (nal_size & 0xff);


		if(is_keyframe)
			quicktime_insert_keyframe(quicktime_fd, frames_written, 0);

		result = quicktime_write_frame(quicktime_fd,
			frame_buffer,
			frame_buffer_size,
			0);

//printf("write_frame %d %d\n", __LINE__, frame_buffer_size);


// Test decode it
/*
 * avcodec_get_frame_defaults(&picture);
 * avcodec_decode_video(decoder_context, 
 * &picture, 
 * &got_picture, 
 * frame_buffer, 
 * frame_buffer_size);
 */



		if(result)
		{
			fprintf(stderr, "Error writing frame\n");
			exit(1);
		}

		bytes_written += frame_buffer_size;
		frames_written++;
		printf("write_frame %d: video_eof=%d frames_written=%d         \n", 
			__LINE__, 
			video_eof,
			frames_written);
		fflush(stdout);
	}

	frame_buffer_size = 0;
	is_keyframe = 0;

// Write audio up to current frame time
#ifdef ENCODE_AUDIO
	if((video_eof || frames_written >= next_audio_frame) &&
		pass > 0 &&
		!audio_eof)
	{
		int64_t next_audio_sample = (int64_t)next_audio_frame * 
			audio_samplerate /
			video_frame_rate;
		while((video_eof || samples_written < next_audio_sample) &&
			!audio_eof &&
			!mpeg3_end_of_audio(mpega_fd, audio_track))
		{
printf("write_frame %d: video_eof=%d samples_written=%lld\n", 
__LINE__, 
video_eof, 
samples_written);
			for(i = 0; i < audio_channels; i++)
			{
				if(i == 0)
					result = mpeg3_read_audio(mpega_fd, 
						audio_buffer[i], 
						0, 
						i, 
						AUDIO_BUFFER_SIZE, 
						audio_track);
				else
					result = mpeg3_reread_audio(mpega_fd, 
						audio_buffer[i],      /* Pointer to pre-allocated buffer of floats */
						0,      /* Pointer to pre-allocated buffer of int16's */
						i,          /* Channel to decode */
						AUDIO_BUFFER_SIZE,         /* Number of samples to decode */
						audio_track);
			}
//printf("write_frame %d\n", __LINE__);

			if(result || mpeg3_end_of_audio(mpega_fd, audio_track)) audio_eof = 1;

			result = quicktime_encode_audio(quicktime_fd, 
				0, 
				audio_buffer, 
				AUDIO_BUFFER_SIZE);
			samples_written += AUDIO_BUFFER_SIZE;
			if(result)
			{
				fprintf(stderr, "Error writing audio\n");
				exit(1);
			}
		}

		next_audio_frame = frames_written + (int)video_frame_rate;
	}
#endif // ENCODE_AUDIO
}

// Returns 1 and leaves ptr unchanged if more data needed
int decode_header()
{
	if(got_header)
	{
		if(ptr < end - NAL_CODE_SIZE)
		{
			ptr += NAL_CODE_SIZE;
			return 0;
		}
	}
	else
	{
// Extract encoding information here.
		if(ptr < end - NAL_CODE_SIZE)
		{
			ptr += NAL_CODE_SIZE;
			next_audio_frame = (int)video_frame_rate;
			return 0;
		}
	}

	return 1;
}

// Returns 1 and leaves ptr unchanged if more data needed
int decode_nal()
{
	if(ptr < end - NAL_CODE_SIZE - 2)
	{
		unsigned char nal_type = ptr[NAL_CODE_SIZE] & 0x1f;



/*
 * printf("decode_nal %d type=0x%02x 0x%02x offset=%lld video_eof=%d\n", 
 * __LINE__, 
 * nal_type, 
 * ptr[NAL_CODE_SIZE + 1], 
 * ptr - buffer,
 * video_eof);
 */



		switch(nal_type)
		{
#ifdef USE_NAL_AUD
			case NAL_AUD:
// End of frame is start of current NAL
				frame_end = ptr;
				write_frame();
				ptr += NAL_CODE_SIZE;
				break;


// sequence header
			case NAL_SPS:
/*
 * printf("decode_nal %d type=0x%02x 0x%02x offset=%lld\n", 
 * __LINE__, 
 * nal_type, 
 * ptr[NAL_CODE_SIZE + 1], 
 * ptr - buffer);
 */
// {
// int j;
// printf("decode_nal %d\n", __LINE__);
// for(j = 0; j < 66; j++)
// printf("%02x ", ptr[j]);
// printf("\n");
// }

				is_keyframe = 1;
				ptr += NAL_CODE_SIZE;
				break;
		

// picture header
			case NAL_PPS:
// printf("decode_nal %d type=0x%02x 0x%02x offset=%lld\n", 
// __LINE__, 
// nal_type, 
// ptr[NAL_CODE_SIZE + 1], 
// ptr - buffer);
				ptr += NAL_CODE_SIZE;
				break;
		

#endif // USE_NAL_AUD










#ifndef USE_NAL_AUD
// sequence header
			case NAL_SPS:
			{
// End of frame is start of sequence header
				frame_end = ptr;

// Decode header NAL
				if(decode_header()) return 1;

printf("decode_nal %d: Got SPS buffer offset=%lld\n", __LINE__, ptr - buffer - 4);

				write_frame();

// Set flags for next frame
				is_keyframe = 1;
				got_seq_header = 1;
				break;
			}

			case NAL_PPS:
printf("decode_nal %d: Got PPS buffer offset=%lld\n", __LINE__, ptr - buffer - 4);
// Picture 
				if(got_seq_header)
				{
// Skip if preceeded by sequence header
					got_seq_header = 0;
					ptr += NAL_CODE_SIZE;
				}
				else
				{
// End of frame is start of NAL
					frame_end = ptr;
					write_frame();
					ptr += NAL_CODE_SIZE;
				}
				break;
#endif // !USE_NAL_AUD





// Unrecognized NAL
			default:
// Skip NAL start code
				ptr += NAL_CODE_SIZE;
				break;




		}
	}
	else
		return 1;
	
	return 0;
}

int main(int argc, char *argv[])
{
	int error = 0;
	int i;

	buffer = malloc(BUFFER_SIZE);
	frame_buffer = malloc(MAX_FRAME_SIZE);
	ptr = buffer;
	end = buffer + buffer_size;
	next_audio_frame = 30;

	if(argc < 3)
	{
		printf("Usage: %s <input file> <output file>\n", argv[0]);
		exit(1);
	}


	in_path = argv[1];
	out_path = argv[2];

// Use the libmpeg3 internals to get at the demuxing support
	printf("main %d: Opening source %s\n", __LINE__, in_path);
	mpegv_fd = mpeg3_open(in_path, 0);
	if(!mpegv_fd)
	{
		fprintf(stderr, "Failed to open input file %s\n", in_path);
		exit(1);
	}

	if(!mpegv_fd->total_vstreams)
	{
		fprintf(stderr, "No video streams in %s\n", in_path);
		exit(1);
	}

	if(!mpegv_fd->total_astreams)
	{
		fprintf(stderr, "No audio streams in %s\n", in_path);
		exit(1);
	}

	mpega_fd = mpeg3_open_copy(in_path, mpegv_fd, 0);
	printf("main %d: Decoding encoding parameters\n", __LINE__);


// Get encoding parameters
	audio_channels = mpeg3_audio_channels(mpega_fd, audio_track);
	audio_samplerate = mpeg3_sample_rate(mpega_fd, audio_track);
	audio_bitrate = audio_channels * 128000;
	audio_buffer = calloc(sizeof(float*), audio_channels);
	for(i = 0; i < audio_channels; i++)
		audio_buffer[i] = calloc(sizeof(float), AUDIO_BUFFER_SIZE);


// Rewind streams
	mpeg3demux_seek_byte(mpegv_fd->vtrack[0]->demuxer, 0);
	mpeg3bits_refill(mpegv_fd->vtrack[0]->video->vstream);

	mpeg3demux_seek_byte(mpega_fd->atrack[0]->demuxer, 0);

// Initialize ffmpeg to decode headers
  	avcodec_init();
	avcodec_register_all();
	decoder = avcodec_find_decoder(CODEC_ID_H264);
//	decoder = avcodec_find_decoder(CODEC_ID_VC1);
	decoder_context = avcodec_alloc_context();
	error = avcodec_open(decoder_context, decoder);
	long output_size = 0;




//	printf("main %d: %d\n", __LINE__, decoder->id);

// Decode header on 1st pass.  Transcode on 2nd pass.
	decoder_context->width = 0;
	int64_t total_bytes = 0;



	printf("main %d: Transcoding\n", __LINE__);


// Audio EOF is handled when audio is written
	pass = 0;

#ifdef OVERRIDE_FORMAT
	pass++;
	got_header = 1;
#endif

	for( ; pass < 2; pass++)
	{
		while(!video_eof &&
			(pass > 0 || !got_header) 

#ifdef MAX_FRAMES
			&& frames_written < MAX_FRAMES
#endif
			)
		{
// Fill video input buffer
			while(!(error = mpeg3_read_video_chunk(mpegv_fd, 
					buffer + buffer_size, 
					&output_size, 
					BUFFER_SIZE - buffer_size,
					0)) &&
					output_size >= 4)
			{

/*
 * static FILE *test = 0;
 * if(!test) test = fopen("test", "w");
 * fwrite(buffer + buffer_size, output_size, 1, test);
 */

				buffer_size += output_size;
				if(BUFFER_SIZE - buffer_size < 4) break;
			}

			if(error)
			{
				video_eof = 1;
				buffer_size += output_size;
			}

			end = buffer + buffer_size;

//printf("main %d video_eof=%d size=%p\n", __LINE__, video_eof, end - NAL_CODE_SIZE - 2 - ptr);
// Search for start code in buffer
			while(ptr < end - NAL_CODE_SIZE - 2 &&
				(pass > 0 || !got_header))
			{
// Got a NAL
				if(ptr[0] == 0x00 &&
					ptr[1] == 0x00 &&
					ptr[2] == 0x00 &&
					ptr[3] == 0x01)
				{
// Need more data if it fails
					if(decode_nal()) break;
				}
				else
				{
					ptr++;
				}
			}
//printf("main %d\n", __LINE__);
		}

		if(pass == 0)
		{
// Rewind video stream
 			mpeg3demux_seek_byte(mpegv_fd->vtrack[0]->demuxer, 0);
			mpeg3bits_refill(mpegv_fd->vtrack[0]->video->vstream);
			buffer_size = 0;
			ptr = buffer;
			end = buffer;
		}
	}

// Flush
	frame_end = buffer + buffer_size;
	write_frame();


	if(quicktime_fd) quicktime_close(quicktime_fd);
	printf("main %d: Wrote %d frames\n", __LINE__, frames_written);
}




