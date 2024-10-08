// New version.
// Very basic table of contents utility since most of the time it's going to be
// built inside a graphical program.
#if 1


#include "libmpeg3.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>





int main(int argc, char *argv[])
{
	int i, j, l;
    char **src = 0;
    int total_src = 0;
	char *dst = 0;
    
	int verbose = 0;

	if(argc < 2)
	{
		fprintf(stderr, "Table of contents generator version %d.%d.%d\n"
			"Create a table of contents for a DVD or mpeg stream.\n"
			"Usage: mpeg3toc <path> [-o output]\n"
			"\n"
            "-o specify output filename if processing 1 file only\n"
            "    Default is to append .toc to the input\n"
			"-v Print tracking information\n"
			"\n"
			"The path should be absolute unless you plan\n"
			"to always run your movie editor from the same directory\n"
			"as the filename.  For renderfarms the filesystem prefix\n"
			"should be / and the movie directory mounted under the same\n"
			"directory on each node.\n\n"
			"Example: mpeg3toc -v /cdrom/video_ts/vts_01_0.ifo -o titanic.toc\n"
			"Example: mpeg3toc -v a.mp3 b.mp3 c.mp3\n",
			mpeg3_major(),
			mpeg3_minor(),
			mpeg3_release());
		exit(1);
	}

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-v"))
		{
			verbose = 1;
		}
		else
        if(!strcmp(argv[i], "-o"))
        {
            dst = argv[i + 1];
            i++;
        }
        else
		if(argv[i][0] == '-')
		{
			fprintf(stderr, "Unrecognized command %s\n", argv[i]);
			exit(1);
		}
		else
		{
// source filename
            total_src++;
            src = realloc(src, sizeof(char*) * total_src);
			src[total_src - 1] = argv[i];
		}
	}

	if(!src)
	{
		fprintf(stderr, "source path not supplied.\n");
		exit(1);
	}

    if(total_src > 1 && dst)
    {
        fprintf(stderr, "can't supply more than 1 source when also supplying a destination path.\n");
		exit(1);
    }

	int64_t total_bytes;
    for(i = 0; i < total_src; i++)
    {
        char *dst_path;
        if(dst)
        {
            dst_path = strdup(dst);
        }
        else
        {
            dst_path = malloc(strlen(src[i]) + 5);
            strcpy(dst_path, src[i]);
            strcat(dst_path, ".toc");
        }

        if(verbose)
            fprintf(stderr,
                "%s -> %s\n",
                src[i],
                dst_path);
	    mpeg3_t *file = mpeg3_start_toc(src[i], dst_path, &total_bytes);

	    if(!file) exit(1);
	    struct timeval new_time;
	    struct timeval prev_time;
	    struct timeval start_time;
	    struct timeval current_time;
	    gettimeofday(&prev_time, 0);
	    gettimeofday(&start_time, 0);


	    while(1)
	    {
		    int64_t bytes_processed = 0;
		    mpeg3_do_toc(file, &bytes_processed);

		    gettimeofday(&new_time, 0);
		    if(verbose && new_time.tv_sec - prev_time.tv_sec > 1)
		    {
			    gettimeofday(&current_time, 0);
			    int64_t elapsed_seconds = current_time.tv_sec - start_time.tv_sec;
			    int64_t total_seconds = elapsed_seconds * total_bytes / bytes_processed;
			    int64_t eta = total_seconds - elapsed_seconds;
			    fprintf(stderr, "%jd%% ETA: %jdm%jds        \r", 
				    bytes_processed * 100 / total_bytes,
				    eta / 60,
				    eta % 60);
			    fflush(stdout);
			    prev_time = new_time;
		    }

		    if(bytes_processed >= total_bytes) break;
	    }

	    mpeg3_stop_toc(file);
        free(dst_path);

	    gettimeofday(&current_time, 0);
	    int64_t elapsed = current_time.tv_sec - start_time.tv_sec;
	    if(verbose)
	    {
		    fprintf(stderr, "%jdm%jds elapsed           \n", 
			    elapsed / 60,
			    elapsed % 60);
	    }
    }

	return 0;
}







#else





#include "libmpeg3.h"
#include "mpeg3protos.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>


/*
 * Generate table of frame and sample offsets for editing.
 */


#define INIT_VECTORS(data, size, allocation, tracks) \
{ \
	int k; \
	data = calloc(1, sizeof(int64_t*) * tracks); \
	size = calloc(1, sizeof(int*) * tracks); \
	allocation = calloc(1, sizeof(int*) * tracks); \
 \
	for(k = 0; k < tracks; k++) \
	{ \
		allocation[k] = 0x100; \
		data[k] = calloc(1, sizeof(int64_t) * allocation[k]); \
	} \
}

#if 1
#define APPEND_VECTOR(data, size, allocation, track, value) \
{ \
	uint64_t **track_data = &(data)[(track)]; \
	int *track_allocation = &(allocation)[(track)]; \
	int *track_size = &(size)[(track)]; \
 \
	if(!(*track_data) || (*track_allocation) <= (*track_size)) \
	{ \
		int64_t *new_data = calloc(1, sizeof(int64_t) * (*track_allocation) * 2); \
 \
		if((*track_data)) \
		{ \
			memcpy(new_data, (*track_data), sizeof(int64_t) * (*track_allocation)); \
			free((*track_data)); \
		} \
		(*track_allocation) *= 2; \
		(*track_data) = new_data; \
	} \
 \
	(*track_data)[(*track_size)++] = (value); \
}
#else
#define APPEND_VECTOR(data, size, allocation, track, value) \
	;
#endif

#define DELETE_VECTORS(data, size, allocation, tracks) \
{ \
	int k; \
	for(k = 0; k < tracks; k++) if(data[k]) free(data[k]); \
	free(data); \
}






#define PUT_INT32(x) \
{ \
	if(MPEG3_LITTLE_ENDIAN) \
	{ \
		fputc(((unsigned char*)&x)[3], output); \
		fputc(((unsigned char*)&x)[2], output); \
		fputc(((unsigned char*)&x)[1], output); \
		fputc(((unsigned char*)&x)[0], output); \
	} \
	else \
	{ \
		fputc(((unsigned char*)&x)[0], output); \
		fputc(((unsigned char*)&x)[1], output); \
		fputc(((unsigned char*)&x)[2], output); \
		fputc(((unsigned char*)&x)[3], output); \
	} \
}




#define PUT_INT64(x) \
{ \
	if(MPEG3_LITTLE_ENDIAN) \
	{ \
		fputc(((unsigned char*)&x)[7], output); \
		fputc(((unsigned char*)&x)[6], output); \
		fputc(((unsigned char*)&x)[5], output); \
		fputc(((unsigned char*)&x)[4], output); \
		fputc(((unsigned char*)&x)[3], output); \
		fputc(((unsigned char*)&x)[2], output); \
		fputc(((unsigned char*)&x)[1], output); \
		fputc(((unsigned char*)&x)[0], output); \
	} \
	else \
	{ \
		fwrite(&x, 1, 8, output); \
	} \
}





int main(int argc, char *argv[])
{
	struct stat st;
	int i, j, l;
	char *src = 0, *dst = 0;
	int astream_override = -1;

	if(argc < 3)
	{
		fprintf(stderr, "Create a table of contents for a DVD or mpeg stream.\n"
			"	Usage: [-a audio streams] mpeg3toc <path> <output>\n"
			"\n"
			" -a override the number of audio streams to scan.  Must be less than\n"
			"the total number of audio streams.\n"
			"\n"
			"	The path should be absolute unless you plan\n"
			"	to always run your movie editor from the same directory\n"
			"	as the filename.  For renderfarms the filesystem prefix\n"
			"	should be / and the movie directory mounted under the same\n"
			"	directory on each node.\n"
			"Example: mpeg3toc /cd2/video_ts/vts_01_0.ifo titanic.toc\n");
		exit(1);
	}

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-a"))
		{
			if(i < argc - 1)
			{
				astream_override = atoi(argv[i + 1]);
				if(astream_override < 0)
				{
					fprintf(stderr, "Total audio streams may not be negative\n");
					exit(1);
				}
				else
				{
					fprintf(stderr, 
						"Using first %d audio streams.\n",
						astream_override);
				}
				i++;
			}
			else
			{
				fprintf(stderr, "-a requires an argument.\n");
				exit(1);
			}
		}
		else
		if(!src)
		{
			src = argv[i];
		}
		else
		if(!dst)
		{
			dst = argv[i];
		}
		else
		{
			fprintf(stderr, "Ignoring argument \"%s\"\n", argv[i]);
		}
	}

	if(!src)
	{
		fprintf(stderr, "source path not supplied.\n");
		exit(1);
	}

	if(!dst)
	{
		fprintf(stderr, "source path not supplied.\n");
		exit(1);
	}

	stat(src, &st);

	if(!st.st_size)
	{
		fprintf(stderr, "%s is 0 length.  Skipping\n", src);
	}
	else
	{
		int64_t size;
		int vtracks;
		int atracks;
		mpeg3_t *input;
		uint64_t **frame_offsets;
		uint64_t **keyframe_numbers;
		uint64_t **sample_offsets;
		int *total_frame_offsets;
		int *total_sample_offsets;
		int *total_keyframe_numbers;
		int *frame_offset_allocation;
		int *sample_offset_allocation;
		int *keyframe_numbers_allocation;
		int done = 0;
		double sample_rate;
		double frame_rate;
		FILE *output;
		int total_samples = 0;
		int total_frames = 0;
		int rewind = 1;

//printf(__FUNCTION__ " 1\n");
		input = mpeg3_open(src);

//printf(__FUNCTION__ " 2\n");
		vtracks = mpeg3_total_vstreams(input);
		atracks = mpeg3_total_astreams(input);
		if(astream_override >= 0) atracks = astream_override;

		if(atracks) sample_rate = mpeg3_sample_rate(input, 0);
		if(vtracks) frame_rate = mpeg3_frame_rate(input, 0);

//printf(__FUNCTION__ " 3\n");
// Handle titles
		INIT_VECTORS(frame_offsets, total_frame_offsets, frame_offset_allocation, vtracks);
		INIT_VECTORS(keyframe_numbers, total_keyframe_numbers, keyframe_numbers_allocation, vtracks);
		INIT_VECTORS(sample_offsets, total_sample_offsets, sample_offset_allocation, atracks);

//printf(__FUNCTION__ " 4\n");
		while(!done)
		{
			int sample_count = MPEG3_AUDIO_CHUNKSIZE;
			int frame_count;
			int have_audio = 0;
			int have_video = 0;
			int64_t title_number = 0;






// Audio section
// Store current position and read sample_count from each atrack
			for(j = 0; j < atracks; j++)
			{
//printf(__FUNCTION__ " 3 %d\n", total_sample_offsets[j]);
				if(rewind)
				{
					mpeg3_demuxer_t *demuxer = input->atrack[j]->demuxer;
					mpeg3demux_seek_byte(demuxer, 0);
//demuxer->dump = 1;
				}

				if(!mpeg3_end_of_audio(input, j))
				{
// Don't want to maintain separate vectors for offset and title.
					int64_t position = mpeg3demux_tell_byte(input->atrack[j]->demuxer);
					int64_t result;

					if(position < MPEG3_IO_SIZE) position = MPEG3_IO_SIZE;
					result = position;

					have_audio = 1;
					APPEND_VECTOR(sample_offsets, 
						total_sample_offsets,
						sample_offset_allocation,
						j,
						result);


//printf(__FUNCTION__ " 6 %d\n", j);
// Throw away samples
					mpeg3_read_audio(input, 
						0,
						0,
						0,
						sample_count,         /* Number of samples to decode */
						j);

printf("\n%lld %lld\n", 
mpeg3demux_tell_byte(input->atrack[j]->demuxer),
mpeg3demux_movie_size(input->atrack[j]->demuxer));
//printf(__FUNCTION__ " 7 %d\n", total_sample_offsets[j]);
				}

				if(j == atracks - 1)
				{
					total_samples += sample_count;
					fprintf(stderr, "Audio: title=%lld total_samples=%d ", title_number, total_samples);
				}
			}

//printf(__FUNCTION__ " 8\n");
			if(have_audio)
			{
				frame_count = 
					(int)((double)total_samples / sample_rate * frame_rate + 0.5) - 
					total_frames;
			}
			else
			{
				frame_count = 1;
			}

//printf(__FUNCTION__ " 9 %d\n", vtracks);














// Video section
			for(j = 0; j < vtracks; j++)
			{
				mpeg3video_t *video = input->vtrack[j]->video;
				mpeg3_demuxer_t *demuxer = input->vtrack[j]->demuxer;
				if(rewind)
				{
					mpeg3demux_seek_byte(demuxer, 0);
				}


				for(l = 0; l < frame_count; l++)
				{
					if(!mpeg3_end_of_video(input, j))
					{
// Transport streams always return one packet after the start of the frame.
						int64_t position = mpeg3demux_tell_byte(demuxer) - 2048;
						int64_t result;
						uint32_t code = 0;
						int got_top = 0;
						int got_bottom = 0;
						int got_keyframe = 0;
						int fields = 0;

						if(position < MPEG3_IO_SIZE) position = MPEG3_IO_SIZE;
						result = position;
						have_video = 1;


//printf("%llx\n", position);
// Store offset of every frame in table
						APPEND_VECTOR(frame_offsets, 
							total_frame_offsets,
							frame_offset_allocation,
							j,
							result);



// Search for next frame start.
						if(total_frame_offsets[j] == 1)
						{
// Assume first frame is an I-frame and put its number in the keyframe number
// table.
							APPEND_VECTOR(keyframe_numbers,
								total_keyframe_numbers,
								keyframe_numbers_allocation,
								j,
								0);



// Skip the first frame.
							mpeg3video_get_header(video, 0);
							video->current_repeat += 100;
						}




// Get next frame
						do
						{
							mpeg3video_get_header(video, 0);
							video->current_repeat += 100;

							if(video->pict_struct == TOP_FIELD)
							{
								got_top = 1;
							}
							else
							if(video->pict_struct == BOTTOM_FIELD)
							{
								got_bottom = 1;
							}
							else
							if(video->pict_struct == FRAME_PICTURE)
							{
								got_top = got_bottom = 1;
							}
							fields++;

// The way we do it, the I frames have the top field but both the I frame and
// subsequent P frame make the keyframe.
							if(video->pict_type == I_TYPE)
								got_keyframe = 1;
						}while(!mpeg3_end_of_video(input, j) && 
							!got_bottom && 
							total_frame_offsets[j] > 1);




// Store number of a keyframe in the keyframe number table
						if(got_keyframe)
							APPEND_VECTOR(keyframe_numbers,
								total_keyframe_numbers,
								keyframe_numbers_allocation,
								j,
								total_frame_offsets[j] - 1);

						if(j == vtracks - 1 && l == frame_count - 1)
						{
							total_frames += frame_count;
							fprintf(stderr, "Video: title=%lld total_frames=%d ", title_number, total_frames);
						}
					}
				}
			}

			if(!have_audio && !have_video) done = 1;

			fprintf(stderr, "\r");
			fflush(stderr);
/*
 * if(total_frames > 10000) 
 * {
 * printf("\n");
 * return 0;
 * }
 */



			rewind = 0;
		}


		output = fopen(dst, "w");



// Write file type
		fputc('T', output);
		fputc('O', output);
		fputc('C', output);
		fputc(' ', output);

// Write version
		fputc(MPEG3_TOC_VERSION, output);

		if(input->is_program_stream)
		{
			fputc(FILE_TYPE_PROGRAM, output);
		}
		else
		if(input->is_transport_stream)
		{
			fputc(FILE_TYPE_TRANSPORT, output);
		}
		else
		if(input->is_audio_stream)
		{
			fputc(FILE_TYPE_AUDIO, output);
		}
		else
		if(input->is_video_stream)
		{
			fputc(FILE_TYPE_VIDEO, output);
		}

// Write stream ID's
// Only program and transport streams have these
		for(i = 0; i < MPEG3_MAX_STREAMS; i++)
		{
			if(input->demuxer->astream_table[i])
			{
				fputc(STREAM_AUDIO, output);
				PUT_INT32(i);
				PUT_INT32(input->demuxer->astream_table[i]);
			}

			if(input->demuxer->vstream_table[i])
			{
				fputc(STREAM_VIDEO, output);
				PUT_INT32(i);
				PUT_INT32(input->demuxer->vstream_table[i]);
			}
		}


// Write titles
		for(i = 0; i < input->demuxer->total_titles; i++)
		{
			mpeg3_title_t *title = input->demuxer->titles[i];
// Path
			fputc(TITLE_PATH, output);
			fprintf(output, title->fs->path);
			fputc(0, output);
// Total bytes
			PUT_INT64(title->total_bytes);
// Byte offsets of cells
			PUT_INT32(input->demuxer->titles[i]->cell_table_size);
			for(j = 0; j < title->cell_table_size; j++)
			{
				mpeg3_cell_t *cell = &title->cell_table[j];
				PUT_INT64(cell->title_start);
				PUT_INT64(cell->title_end);
				PUT_INT64(cell->program_start);
				PUT_INT64(cell->program_end);
				PUT_INT32(cell->program);
			}
		}








		fputc(ATRACK_COUNT, output);
		PUT_INT32(atracks);

		fputc(VTRACK_COUNT, output);
		PUT_INT32(vtracks);

// Audio streams
		for(j = 0; j < atracks; j++)
		{
			int channels = mpeg3_audio_channels(input, j);
			PUT_INT32(channels);
			PUT_INT32(total_sample_offsets[j]);
			for(i = 0; i < total_sample_offsets[j]; i++)
			{
				PUT_INT64(sample_offsets[j][i]);
//printf("Audio: offset=%016llx\n", sample_offsets[j][i]);
			}
		}

// Video streams
		for(j = 0; j < vtracks; j++)
		{
			PUT_INT32(total_frame_offsets[j]);
			for(i = 0; i < total_frame_offsets[j]; i++)
			{
				PUT_INT64(frame_offsets[j][i]);
//printf("Video: offset=%016llx\n", frame_offsets[j][i]);
			}

			PUT_INT32(total_keyframe_numbers[j]);
			for(i = 0; i < total_keyframe_numbers[j]; i++)
			{
				PUT_INT64(keyframe_numbers[j][i]);
//printf("Video: keyframe=%lld\n", keyframe_numbers[j][i]);
			}
		}




		DELETE_VECTORS(frame_offsets, total_frame_offsets, frame_offset_allocation, vtracks);
		DELETE_VECTORS(keyframe_numbers, total_keyframe_numbers, keyframe_numbers_allocation, vtracks);
		DELETE_VECTORS(sample_offsets, total_sample_offsets, sample_offset_allocation, atracks);


		mpeg3_close(input);
		fclose(output);
	}




	return 0;
}






#endif
