#include "funcprotos.h"
#include "quicktime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdint.h>









#define FSEEK fseeko64


#define WIDTH 1920
#define HEIGHT 1080

//#define FRAMERATE (double)30000/1001
#define FRAMERATE (double)30
//#define FRAMERATE (double)30000/3000


//#define STARTING_OFFSET 0x28
#define STARTING_OFFSET 0x0

#define CHANNELS 1
#define SAMPLERATE 32000
#define AUDIO_CHUNK 2048
#define BITS 16
#define TEMP_FILE "/tmp/temp.mov"

// Output files
#define AUDIO_FILE "/tmp/audio.pcm"
#define VIDEO_FILE "/tmp/video.mov"


//#define VCODEC QUICKTIME_MJPA
#define VCODEC QUICKTIME_JPEG
// Search for JFIF in JPEG header
//#define USE_JFIF
// Only 1 variation of this, recorded by 1 camcorder
//#define VCODEC QUICKTIME_H264
// H264 rendered by Cinelerra
//#define USE_X264


#define ACODEC QUICKTIME_MP4A

//#define READ_ONLY


#define SEARCH_FRAGMENT (int64_t)0x100000
//#define SEARCH_PAD 8
#define SEARCH_PAD 16


#define GOT_NOTHING 0
#define IN_FIELD1   1
#define GOT_FIELD1  2
#define IN_FIELD2   3
#define GOT_FIELD2  4
#define GOT_AUDIO   5
#define GOT_IMAGE_START 6
#define GOT_IMAGE_END   7



#ifdef USE_X264
unsigned char h264_desc[] = 
{

	0x01, 0x4d, 0x40, 0x1f, 0xff, 0xe1, 0x00, 0x16, 0x67, 0x4d, 0x40, 0x33, 0x9a,
	0x74, 0x07, 0x80, 0x8b, 0xf7, 0x08, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x07, 0x53, 0x04,
	0x78, 0xc1, 0x95, 0x01, 0x00, 0x04, 0x68, 0xee, 0x1f, 0x20

};

#else

// H264 description for cheap camcorder format
unsigned char h264_desc[] = 
{
	0x01, 0x4d, 0x00, 0x28, 0xff, 0xe1, 0x00, 0x30, 0x27, 0x4d, 0x00, 0x28,
	0x9a, 0x62, 0x80, 0xa0, 0x0b, 0x76, 0x02, 0x20, 0x00, 0x00, 0x7d, 0x20,
	0x00, 0x1d, 0x4c, 0x1d, 0x0c, 0x00, 0x26, 0x26, 0x00, 0x02, 0xae, 0xa9,
	0x77, 0x97, 0x1a, 0x18, 0x00, 0x4c, 0x4c, 0x00, 0x05, 0x5d, 0x52, 0xef,
	0x2e, 0x1f, 0x08, 0x84, 0x51, 0xe0, 0x00, 0x00, 0x01, 0x00, 0x04, 0x28,
	0xee, 0x3c, 0x80 
};

#endif

// Table utilities
#define NEW_TABLE(ptr, size, allocation) \
{ \
	(ptr) = 0; \
	(size) = 0; \
	(allocation) = 0; \
}

#define APPEND_TABLE(ptr, size, allocation, value) \
{ \
	if((allocation) <= (size)) \
	{ \
		if(!(allocation)) \
			(allocation) = 1024; \
		else \
			(allocation) *= 2; \
		int64_t *new_table = calloc(1, sizeof(int64_t) * (allocation)); \
		memcpy(new_table, (ptr), sizeof(int64_t) * (size)); \
		free((ptr)); \
		(ptr) = new_table; \
	} \
	(ptr)[(size)] = (value); \
	(size)++; \
}

int get_h264_size(unsigned char *frame_buffer, int frame_size)
{
	int result = frame_size;
	int offset = 0;

// walk NAL codes
	while(offset < frame_size)
	{
		int nal_size = ((frame_buffer[offset + 0] << 24) |
			(frame_buffer[offset + 1] << 16) |
			(frame_buffer[offset + 2] << 8) |
			(frame_buffer[offset + 3])) + 4;
//printf("get_h264_size %d %d %d\n", __LINE__, offset, nal_size);
		if(nal_size <= 0 || nal_size + offset >= frame_size)
		{
			return offset;
		}
		
		offset += nal_size;
	}
	
	return result;
}


int main(int argc, char *argv[])
{
	FILE *in = 0;
	FILE *temp = 0;
	FILE *audio_out = 0;
//	quicktime_t *out;
	quicktime_t *video_out;
	int64_t current_byte, ftell_byte;
	int64_t jpeg_end;
	int64_t audio_start = 0, audio_end = 0;
	unsigned char *search_buffer = calloc(1, SEARCH_FRAGMENT);
	unsigned char *frame_buffer = calloc(1, SEARCH_FRAGMENT);
	unsigned char *copy_buffer = 0;
	int i;
	int64_t file_size;
	struct stat status;
	unsigned char data[8];
	struct stat ostat;
	int fields = 1;
	int is_h264 = 0;
	int is_keyframe = 0;
	int next_is_keyframe = 0;
	time_t current_time = time(0);
	time_t prev_time = 0;
	int jpeg_header_offset;
	int64_t field1_offset = 0;
	int64_t field2_offset = 0;
	int64_t image_start = STARTING_OFFSET;
	int64_t image_end = STARTING_OFFSET;
	int update_time = 0;
	int state = GOT_NOTHING;
	char *in_path;
	char *audio_path = AUDIO_FILE;
	char *video_path = VIDEO_FILE;
	int audio_frame;
	int total_samples;
	int field;

// Value taken from Cinelerra preferences
	int audio_chunk = AUDIO_CHUNK;


	int64_t *start_table;
	int start_size;
	int start_allocation;
	int64_t *end_table;
	int end_size;
	int end_allocation;
	int64_t *field_table;
	int64_t field_size;
	int64_t field_allocation;
	int width = WIDTH;
	int height = HEIGHT;


	if(argc < 2)
	{
		printf("Recover JPEG and PCM audio in a corrupted movie.\n"
			"Usage: recover [options] <input>\n"
			"Options:\n"
			" -b samples     number of samples in an audio chunk (%d)\n"
			" -a filename    alternative audio output\n"
			" -v filename    alternative video output\n"
			"\n",
			audio_chunk);
		exit(1);
	}

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-b"))
		{
			if(i + 1 < argc)
			{
				audio_chunk = atol(argv[i + 1]);
				i++;
				if(audio_chunk <= 0)
				{
					printf("Sample count for -b is out of range.\n");
					exit(1);
				}
			}
			else
			{
				printf("-b needs a sample count.\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-v"))
		{
			if(i + 1 < argc)
			{
				video_path = argv[i + 1];
				i++;
			}
			else
			{
				printf("-v needs a filename.\n");
				exit(1);
			}
		}
		else
		if(!strcmp(argv[i], "-a"))
		{
			if(i + 1 < argc)
			{
				audio_path = argv[i + 1];
				i++;
			}
			else
			{
				printf("-a needs a filename.\n");
				exit(1);
			}
		}
		else
		{
			in_path = argv[i];
		}
	}

// Get the field count
	if(!memcmp(VCODEC, QUICKTIME_MJPA, 4))
	{
		fields = 2;
	}
	else
	{
		fields = 1;
	}

	if(!memcmp(VCODEC, QUICKTIME_H264, 4))
	{

#ifdef USE_X264
		printf("   X264\n");
#endif
		is_h264 = 1;
	}


// Dump codec settings
	printf("Codec settings:\n"
		"   WIDTH=%d HEIGHT=%d\n"
		"   FRAMERATE=%.2f\n"
		"   CHANNELS=%d\n"
		"   SAMPLERATE=%d\n"
		"   BITS=%d\n"
		"   AUDIO CHUNK=%d\n"
		"   VCODEC=\"%s\"\n"
		"   ACODEC=\"%s\"\n"
		"   AUDIO_FILE=\"%s\"\n"
		"   VIDEO_FILE=\"%s\"\n",
		WIDTH,
		HEIGHT,
		FRAMERATE,
		CHANNELS,
		SAMPLERATE,
		BITS,
		audio_chunk,
		VCODEC,
		ACODEC,
		audio_path,
		video_path);
#ifdef READ_ONLY
	printf("   READ ONLY\n");
#endif


	in = fopen(in_path, "rb+");
	if(!in)
	{
		perror("open input");
		exit(1);
	}
	
	fseek(in, STARTING_OFFSET, SEEK_SET);
	
	
	// try to get width & height
	if(!is_h264)
	{
		int bytes_read = fread(search_buffer, 1, SEARCH_FRAGMENT, in);
		int got_it = 0;
		for(i = 0; i < bytes_read - 0x20; i++)
		{
			if(search_buffer[i] == 0xff &&
				search_buffer[i + 1] == 0xd8 &&
				search_buffer[i + 2] == 0xff &&
				search_buffer[i + 3] == 0xc0
#ifdef USE_JFIF					
				&& search_buffer[i + 6] == 'J' &&
				search_buffer[i + 7] == 'F' &&
				search_buffer[i + 8] == 'I' &&
				search_buffer[i + 9] == 'F'
#endif
			)
			{
				for(i += 0x14; i < bytes_read - 0x9; i++)
				{
					if(search_buffer[i] == 0xff &&
						search_buffer[i + 1] == 0xc0)
					{
						height = (search_buffer[i + 5] << 8) |
							search_buffer[i + 6];
						width = (search_buffer[i + 7] << 8) |
							search_buffer[i + 8];
						got_it = 1;
						break;
					}
				}
				break;
			}
		}
		
		if(got_it)
		{
			printf("main %d: detected w=%d h=%d\n", __LINE__, width, height);
		}
	}


	fseek(in, STARTING_OFFSET, SEEK_SET);


#ifndef READ_ONLY
//	out = quicktime_open(TEMP_FILE, 0, 1);
// 	if(!out)
// 	{
// 		perror("open temp");
// 		exit(1);
// 	}

// 	quicktime_set_audio(out, 
// 		CHANNELS, 
// 		SAMPLERATE, 
// 		BITS, 
// 		QUICKTIME_TWOS);
// 	quicktime_set_video(out, 
// 		1, 
// 		WIDTH, 
// 		HEIGHT, 
// 		FRAMERATE, 
// 		VCODEC);

	audio_out = fopen(audio_path, "w");
	if(!audio_out)
	{
		perror("open audio output");
		exit(1);
	}

	video_out = quicktime_open(video_path, 0, 1);
		
	if(!video_out)
	{
		perror("open video out");
		exit(1);
	}

	quicktime_set_video(video_out, 
		1, 
		width, 
		height, 
		FRAMERATE, 
		VCODEC);
// 	quicktime_set_audio(video_out, 
// 		CHANNELS, 
// 		SAMPLERATE, 
// 		BITS, 
// 		ACODEC);


	if(is_h264)
	{
		quicktime_video_map_t *vtrack = &(video_out->vtracks[0]);
		quicktime_trak_t *trak = vtrack->track;
		quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;
		quicktime_set_avcc_header(avcc,
		  	h264_desc, 
		  	sizeof(h264_desc));
	}
	
#endif

	audio_start = (int64_t)0x10;
	ftell_byte = STARTING_OFFSET;

	if(fstat(fileno(in), &status))
		perror("get_file_length fstat:");
	file_size = status.st_size;


	NEW_TABLE(start_table, start_size, start_allocation)
	NEW_TABLE(end_table, end_size, end_allocation)
	NEW_TABLE(field_table, field_size, field_allocation)



	audio_frame = BITS * CHANNELS / 8;

// Tabulate the start and end of all the JPEG images.
// This search is intended to be as simple as possible, reserving more
// complicated operations for a table pass.
//printf("Pass 1 video only.\n");
	while(ftell_byte < file_size)
	{
		current_byte = ftell_byte;
		int temp = fread(search_buffer, SEARCH_FRAGMENT, 1, in);
		ftell_byte = current_byte + SEARCH_FRAGMENT - SEARCH_PAD;
		FSEEK(in, ftell_byte, SEEK_SET);

//printf("main %d\n", __LINE__);
		for(i = 0; i < SEARCH_FRAGMENT - SEARCH_PAD; i++)
		{
// Search for image start
			if(state == GOT_NOTHING)
			{
				if(is_h264)
				{
#ifdef USE_X264
					if(search_buffer[i] == 0x00 &&
						search_buffer[i + 1] == 0x00 &&
						search_buffer[i + 4] == 0x06 &&
						search_buffer[i + 5] == 0x05)
					{
//printf("main %d\n", __LINE__);
						state = GOT_IMAGE_START;
						image_end = current_byte + i;
						is_keyframe = next_is_keyframe;
						next_is_keyframe = 1;
					}
					else
					if(search_buffer[i] == 0x00 &&
						search_buffer[i + 1] == 0x00 &&
						search_buffer[i + 4] == 0x41 &&
						search_buffer[i + 5] == 0x9a)
					{
//printf("main %d\n", __LINE__);
						state = GOT_IMAGE_START;
						image_end = current_byte + i;
						is_keyframe = next_is_keyframe;
						next_is_keyframe = 0;
					}

					if(state == GOT_IMAGE_START)
					{
// end of previous frame
						if(start_size > 0) 
						{
							APPEND_TABLE(end_table, end_size, end_allocation, image_end)
// Copy frame
							int frame_size = image_end - image_start;
							FSEEK(in, image_start, SEEK_SET);
							fread(frame_buffer, frame_size, 1, in);
							FSEEK(in, ftell_byte, SEEK_SET);
							quicktime_write_frame(video_out, 
								frame_buffer, 
								frame_size, 
								0);
							if(is_keyframe)
							{
								quicktime_video_map_t *vtrack = &(video_out->vtracks[0]);
								quicktime_insert_keyframe(video_out, 
									vtrack->current_position - 1, 
									0);
							}
							image_start = image_end;
						}
// start of next frame
						APPEND_TABLE(start_table, start_size, start_allocation, image_start)
						state = GOT_NOTHING;


					}
					
#else // USE_X264
					if(search_buffer[i] == 0x00 &&
						search_buffer[i + 1] == 0x00 &&
						search_buffer[i + 2] == 0x00 &&
						search_buffer[i + 3] == 0x02 &&
						search_buffer[i + 4] == 0x09)
					{
						state = GOT_IMAGE_START;
						image_start = current_byte + i;
						if(search_buffer[i + 5] == 0x10)
							is_keyframe = 1;
						else
							is_keyframe = 0;
					}
#endif // !USE_X264
				}
				else
				if(search_buffer[i] == 0xff &&
					search_buffer[i + 1] == 0xd8 &&
					search_buffer[i + 2] == 0xff &&
					search_buffer[i + 3] == 0xe1 &&
					search_buffer[i + 10] == 'm' &&
					search_buffer[i + 11] == 'j' &&
					search_buffer[i + 12] == 'p' &&
					search_buffer[i + 13] == 'g')
				{
					state = GOT_IMAGE_START;
					image_start = current_byte + i;

// Determine the field
					if(fields == 2)
					{
// Next field offset is nonzero in first field
						if(search_buffer[i + 22] != 0 ||
							search_buffer[i + 23] != 0 ||
							search_buffer[i + 24] != 0 ||
							search_buffer[i + 25] != 0)
						{
							field = 0;
						}
						else
						{
							field = 1;
						}
						APPEND_TABLE(field_table, field_size, field_allocation, field)
					}
				}
				else
				if(search_buffer[i] == 0xff &&
					search_buffer[i + 1] == 0xd8 &&
					search_buffer[i + 2] == 0xff &&
					search_buffer[i + 3] == 0xc0

#ifdef USE_JFIF					
					&& search_buffer[i + 6] == 'J' &&
					search_buffer[i + 7] == 'F' &&
					search_buffer[i + 8] == 'I' &&
					search_buffer[i + 9] == 'F'
#endif
				)
				{
					state = GOT_IMAGE_START;
					image_start = current_byte + i;
//printf("main %d 0x%jx\n", __LINE__, image_start);
				}
			}
			else
// Search for image end
			if(state == GOT_IMAGE_START)
			{
				if(is_h264)
				{
// got next frame & end of previous frame or previous audio
// search 1 byte ahead so the loop doesn't skip the next frame
					if(search_buffer[i + 1] == 0x00 &&
						search_buffer[i + 2] == 0x00 &&
						search_buffer[i + 3] == 0x00 &&
						search_buffer[i + 4] == 0x02 &&
						search_buffer[i + 5] == 0x09)
					{
						state = GOT_NOTHING;
						image_end = current_byte + i + 1;

// Read entire frame & get length from NAL codes
						if(image_end - image_start <= SEARCH_FRAGMENT)
						{
							int frame_size = image_end - image_start;
							FSEEK(in, image_start, SEEK_SET);
							int temp = fread(frame_buffer, frame_size, 1, in);
							FSEEK(in, ftell_byte, SEEK_SET);

							int new_frame_size = get_h264_size(frame_buffer, frame_size);
/*
 * printf("%d: image_start=%lx image_end=%lx new_frame_size=%x\n",
 * __LINE__,
 * image_start,
 * image_end,
 * new_frame_size);
 */

							image_end = image_start + new_frame_size;

//printf("%d: image_start=0x%lx image_size=0x%x\n", __LINE__, image_start, new_frame_size);
						}
						else
						{
							printf("%d: Possibly lost image between %lx and %lx\n", 
								__LINE__,
								image_start,
								image_end);
						}


						APPEND_TABLE(start_table, start_size, start_allocation, image_start)
						APPEND_TABLE(end_table, end_size, end_allocation, image_end)

#ifndef READ_ONLY
// Write frame
						quicktime_write_frame(video_out, 
							frame_buffer, 
							image_end - image_start, 
							0);
						if(is_keyframe)
						{
							quicktime_video_map_t *vtrack = &(video_out->vtracks[0]);
							quicktime_insert_keyframe(video_out, 
								vtrack->current_position - 1, 
								0);
						}

// Write audio
						if(start_size > 1)
						{
							int64_t next_frame_start = start_table[start_size - 1];
							int64_t prev_frame_end = end_table[start_size - 2];
							int audio_size = next_frame_start - prev_frame_end;
							if(audio_size > SEARCH_FRAGMENT)
								audio_size = SEARCH_FRAGMENT;
							FSEEK(in, prev_frame_end, SEEK_SET);
							int temp = fread(frame_buffer, audio_size, 1, in);
							FSEEK(in, ftell_byte, SEEK_SET);
//							fwrite(frame_buffer, audio_size, 1, audio_out);

							quicktime_write_vbr_frame(video_out, 
								0,
								frame_buffer,
								audio_size,
								audio_chunk);
						}
#endif
					}
				}
				else
				if(search_buffer[i] == 0xff &&
					search_buffer[i + 1] == 0xd9)
				{
//printf("main %d 0x%jx\n", __LINE__, current_byte + i);
// ffd9 sometimes occurs inside the mjpg tag
					if(current_byte + i - image_start > 0x2a)
					{
						state = GOT_NOTHING;
// Put it in the table
						image_end = current_byte + i + 2;

// An image may have been lost due to encoding errors but we can't do anything
// because the audio may by misaligned.  Use the extract utility to get the audio.
						if(image_end - image_start > audio_chunk * audio_frame)
						{
/*
 * 							printf("%d: Possibly lost image between %llx and %llx\n", 
 * 								__LINE__,
 * 								image_start,
 * 								image_end);
 */
// Put in fake image
/*
 * 							APPEND_TABLE(start_table, start_size, start_allocation, image_start)
 * 							APPEND_TABLE(end_table, end_size, end_allocation, image_start + 1024)
 * 							APPEND_TABLE(start_table, start_size, start_allocation, image_end - 1024)
 * 							APPEND_TABLE(end_table, end_size, end_allocation, image_end)
 */
						}

						APPEND_TABLE(start_table, start_size, start_allocation, image_start)
						APPEND_TABLE(end_table, end_size, end_allocation, image_end)

						int frame_size = image_end - image_start;
						FSEEK(in, image_start, SEEK_SET);
						int temp = fread(frame_buffer, frame_size, 1, in);
						FSEEK(in, ftell_byte, SEEK_SET);
						quicktime_write_frame(video_out, 
							frame_buffer, 
							image_end - image_start, 
							0);
		

//printf("%d %llx - %llx\n", start_size, image_start, image_end - image_start);

if(!(start_size % 100))
{
printf("Got %d frames. %ld%%\n", 
start_size, 
current_byte * (int64_t)100 / file_size);
fflush(stdout);
}
					}
				}
			}
		}
	}



	printf("Got %d frames %d samples total.\n", start_size, total_samples);

// With the image table complete, 
// write chunk table from the gaps in the image table
// printf("Pass 2 audio table.\n");
// 	total_samples = 0;
// 	for(i = 1; i < start_size; i++)
// 	{
// 		int64_t next_image_start = start_table[i];
// 		int64_t prev_image_end = end_table[i - 1];
// 
// // Got a chunk
// 		if(next_image_start - prev_image_end >= audio_chunk * audio_frame)
// 		{
// 			long samples = (next_image_start - prev_image_end) / audio_frame;
// 			quicktime_atom_t chunk_atom;
// 
// 			quicktime_set_position(out, prev_image_end);
// 			quicktime_write_chunk_header(out, 
// 				out->atracks[0].track, 
// 				&chunk_atom);
// 			quicktime_set_position(out, next_image_start);
// 			quicktime_write_chunk_footer(out,
// 				out->atracks[0].track, 
// 				out->atracks[0].current_chunk, 
// 				&chunk_atom,
// 				samples);
// 			out->atracks[0].current_position += samples;
// 			out->atracks[0].current_chunk++;
// 			total_samples += samples;
// 		}
// 	}
// 
// 
// 
// 
// 
// // Put image table in movie
// 	for(i = 0; i < start_size - fields; i += fields)
// 	{
// // Got a field out of order.  Skip just 1 image instead of 2.
// 		if(fields == 2 && field_table[i] != 0)
// 		{
// 			printf("Got field out of order at 0x%llx\n", start_table[i]);
// 			i--;
// 		}
// 		else
// 		{
// 			quicktime_atom_t chunk_atom;
// 			quicktime_set_position(out, start_table[i]);
// 			quicktime_write_chunk_header(out, 
// 				out->vtracks[0].track,
// 				&chunk_atom);
// 			quicktime_set_position(out, end_table[i + fields - 1]);
// 			quicktime_write_chunk_footer(out,
// 				out->vtracks[0].track, 
// 				out->vtracks[0].current_chunk, 
// 				&chunk_atom,
// 				1);
// 			out->vtracks[0].current_position++;
// 			out->vtracks[0].current_chunk++;
// 		}
// 	}
// 
// 
// 
// 
// 
// 
// 
// // Force header out at beginning of temp file
// 	quicktime_set_position(out, 0x10);
// 	quicktime_close(out);
// 
// // Transfer header
// 	FSEEK(in, 0x8, SEEK_SET);
// 
// 	data[0] = (ftell_byte & 0xff00000000000000LL) >> 56;
// 	data[1] = (ftell_byte & 0xff000000000000LL) >> 48;
// 	data[2] = (ftell_byte & 0xff0000000000LL) >> 40;
// 	data[3] = (ftell_byte & 0xff00000000LL) >> 32;
// 	data[4] = (ftell_byte & 0xff000000LL) >> 24;
// 	data[5] = (ftell_byte & 0xff0000LL) >> 16;
// 	data[6] = (ftell_byte & 0xff00LL) >> 8;
// 	data[7] = ftell_byte & 0xff;
// 	fwrite(data, 8, 1, in);
// 
// 	FSEEK(in, ftell_byte, SEEK_SET);
// 	stat(TEMP_FILE, &ostat);
// 
// 	temp = fopen(TEMP_FILE, "rb");
// 	FSEEK(temp, 0x10, SEEK_SET);
// 
// 	copy_buffer = calloc(1, ostat.st_size);
// 	fread(copy_buffer, ostat.st_size, 1, temp);
// 	fclose(temp);
// 
// // Enable to alter the original file
// 	printf("%d: writing header to file\n", __LINE__);
// 	fwrite(copy_buffer, ostat.st_size, 1, in);


	fclose(in);

#ifndef READ_ONLY
	quicktime_close(video_out);
	fclose(audio_out);
#endif

}




