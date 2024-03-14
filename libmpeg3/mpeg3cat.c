/* Concatenate elementary streams */
/* Mpeg3cat is useful for extracting elementary streams from program streams. */


#include "libmpeg3.h"
#include "mpeg3protos.h"
#include "mpeg3css.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define AC3_START_CODE 0x0b77
#define BUFFER_SIZE            0x100000

unsigned char *output_buffer = 0;
int64_t output_buffer_size = 0;
int64_t output_scan_offset = 0;
FILE *out = 0;
int got_start = 0;
int do_audio = 0;
int do_video = 0;

// Check for first start code before writing out
static int write_output(unsigned char *data, int size, mpeg3_t *fd)
{
// Already got start code so write data directly
// Or user doesn't want to extract an elementary stream
	if(got_start || (!do_audio && !do_video) || fd->is_bd)
	{
		return fwrite(data, size, 1, out);
	}
	else
// Buffer until start code
	{
		uint32_t code = 0xffffffff;
		output_buffer = realloc(output_buffer, output_buffer_size + size);
		memcpy(output_buffer + output_buffer_size, data, size);
		output_buffer_size += size;

		if(output_buffer_size >= 4)
		{
			if(do_video)
			{
				while(output_scan_offset < output_buffer_size && !got_start)
				{
					code = (code << 8) | output_buffer[output_scan_offset++];
					if(code == MPEG3_SEQUENCE_START_CODE)
					{
						got_start = 1;
					}
				}

				output_scan_offset -= 4;
			}
			else
// Only scan for AC3 start code since we can't scan for mp2 start codes.
// It must occur in the first 2048 bytes or we give up.
			{
				while(output_scan_offset < output_buffer_size && 
					output_scan_offset < 2048 &&
					!got_start)
				{
					code = ((code & 0xff) << 8) | output_buffer[output_scan_offset++];
					if(code == AC3_START_CODE)
					{
						got_start = 1;
					}
				}

				if(got_start)
					output_scan_offset -= 2;
				else
				if(output_scan_offset >= 2048)
				{
					output_scan_offset = 0;
					got_start = 1;
				}
				else
					output_scan_offset -= 2;
			}

			if(got_start)
			{
    		   return fwrite(output_buffer + output_scan_offset, 
					output_buffer_size - output_scan_offset, 1, out);
			}
		}
	}

	return 1;
}

int main(int argc, char *argv[])
{
	char inpath[1024], outpath[1024], newpath[1024];
	char *inpaths[argc];
	int total_infiles = 0;
	mpeg3_t *in;
	int out_counter = 0;
	int current_file, current_output_file = 0, i;
	unsigned int bits;
	unsigned char *buffer = 0;
	long output_size;
	int result = 0;
	int64_t total_frames = 0;
	int stream = 0;
	int64_t total_written = 0;

	buffer = malloc(BUFFER_SIZE);

	if(argc < 2)
	{
		fprintf(stderr, "Concatenate elementary streams or demultiplex a program stream.\n"
			"Usage: mpeg3cat -[av0123456789] <infile> [infile...] > <outfile>\n\n"
			"Example: Concatenate 2 video files: mpeg3cat xena1.m2v xena2.m2v > xena.m2v\n"
			"         Extract audio stream 0: mpeg3cat -a0 xena.vob > war_cry.ac3\n");
		exit(1);
	}

	outpath[0] = 0;
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(argv[i][1] != 'a' && argv[i][1] != 'v' && argv[i][1] != 'o')
			{
				fprintf(stderr, "invalid option %s\n", argv[i]);
				exit(1);
			}
			else
			if(argv[i][1] == 'o')
			{
// Check for filename
				if(i < argc - 1)
				{
					strcpy(outpath, argv[++i]);
				}
				else
				{
					fprintf(stderr, "-o requires an output file\n");
					exit(1);
				}

// Check if file exists
				if(out = fopen(outpath, "r"))
				{
					fprintf(stderr, "%s exists.\n", outpath);
					exit(1);
				}

			}
			else
			{
				if(argv[i][1] == 'a') do_audio = 1;
				else
				if(argv[i][1] == 'v') do_video = 1;
				
				if(argv[i][2] != 0)
				{
					stream = atoi(argv[i] + 2);
				}
			}
		}
		else
		{
			inpaths[total_infiles++] = argv[i];
		}
	}

	if(outpath[0])
	{
		if(!(out = fopen(outpath, "wb")))
		{
			fprintf(stderr, "Failed to open %s for writing\n", outpath);
			exit(1);
		}
	}
	else
		out = stdout;

	for(current_file = 0; current_file < total_infiles; current_file++)
	{
		strcpy(inpath, inpaths[current_file]);

		int error = 0;
		if(!(in = mpeg3_open(inpath, &error)))
		{
			fprintf(stderr, "Skipping %s\n", inpath);
			continue;
		}





/* output elementary audio stream */
		if((mpeg3_has_audio(in) && in->is_audio_stream) || 
			(do_audio && !in->is_audio_stream && !in->is_video_stream))
		{
			do_audio = 1;
/* Add audio stream to end */
			if(stream >= in->total_astreams)
			{
				fprintf(stderr, "No audio stream %d\n", stream);
				exit(1);
			}

			mpeg3demux_seek_byte(in->atrack[stream]->demuxer, MPEG3_START_BYTE);
//			mpeg3bits_refill(in->atrack[stream]->audio->astream);
//printf("mpeg3cat 1\n");
			while(!mpeg3_read_audio_chunk(in, 
				buffer, 
				&output_size, 
				BUFFER_SIZE,
				stream))
			{
//printf("mpeg3cat 2 0x%x\n", output_size);
				result = !write_output(buffer, output_size, in);
				if(result)
				{
					perror("write audio chunk");
					break;
				}
			}
//printf("mpeg3cat 3\n");
		}
		else
/* Output elementary video stream */
		if((mpeg3_has_video(in) && in->is_video_stream) ||
			(do_video && !in->is_video_stream && !in->is_audio_stream))
		{
/* Add video stream to end */
			int64_t hour, minute, second, frame;
			int64_t gop_frame;
			uint32_t code;
			float carry;
			int i, offset;
			
			if(stream >= in->total_vstreams)
			{
				fprintf(stderr, "No video stream %d\n", stream);
				exit(1);
			}

			mpeg3demux_seek_byte(in->vtrack[stream]->demuxer, MPEG3_START_BYTE);
			mpeg3bits_refill(in->vtrack[stream]->video->vstream);
			do_video = 1;
			while(!mpeg3_read_video_chunk(in, 
				buffer, 
				&output_size, 
				BUFFER_SIZE,
				stream) &&
				output_size >= 4)
			{
				code = (uint32_t)buffer[output_size - 4] << 24; 
				code |= (uint32_t)buffer[output_size - 3] << 16; 
				code |= (uint32_t)buffer[output_size - 2] << 8; 
				code |= (uint32_t)buffer[output_size - 1]; 

/* Got a frame at the end of this buffer. */
				if(code == MPEG3_PICTURE_START_CODE)
				{
					total_frames++;
				}
				else
				if(code == MPEG3_SEQUENCE_END_CODE)
				{
/* Got a sequence end code at the end of this buffer. */
					output_size -= 4;
				}

				code = (uint32_t)buffer[0] << 24;
				code |= (uint32_t)buffer[1] << 16;
				code |= (uint32_t)buffer[2] << 8;
				code |= buffer[3];

				i = 0;
				offset = 0;
				if(code == MPEG3_SEQUENCE_START_CODE && current_output_file > 0)
				{
/* Skip the sequence start code */
					i += 4;
					while(i < output_size && 
						code != MPEG3_GOP_START_CODE)
					{
						code <<= 8;
						code |= buffer[i++];
					}
					i -= 4;
					offset = i;
				}

/* Search for GOP header to fix */
				code = (uint32_t)buffer[i++] << 24;
				code |= (uint32_t)buffer[i++] << 16;
				code |= (uint32_t)buffer[i++] << 8;
				code |= buffer[i++];
				while(i < output_size &&
					code != MPEG3_GOP_START_CODE)
				{
					code <<= 8;
					code |= buffer[i++];
				}

				if(code == MPEG3_GOP_START_CODE)
				{
/* Get the time code */
					code = (uint32_t)buffer[i] << 24;
					code |= (uint32_t)buffer[i + 1] << 16;
					code |= (uint32_t)buffer[i + 2] << 8;
					code |= (uint32_t)buffer[i + 3];

					hour = code >> 26 & 0x1f;
					minute = code >> 20 & 0x3f;
					second = code >> 13 & 0x3f;
					frame = code >> 7 & 0x3f;

					gop_frame = (int64_t)(hour * 3600 * mpeg3_frame_rate(in, stream) +
							minute * 60 * mpeg3_frame_rate(in, stream) +
							second * mpeg3_frame_rate(in, stream) + 
							frame);
/* fprintf(stderr, "old: %02d:%02d:%02d:%02d ", hour, minute, second, frame); */
/* Write a new time code */
					hour = (int64_t)((float)(total_frames - 1) / mpeg3_frame_rate(in, stream) / 3600);
					carry = hour * 3600 * mpeg3_frame_rate(in, stream);
					minute = (int64_t)((float)(total_frames - 1 - carry) / mpeg3_frame_rate(in, stream) / 60);
					carry += minute * 60 * mpeg3_frame_rate(in, stream);
					second = (int64_t)((float)(total_frames - 1 - carry) / mpeg3_frame_rate(in, stream));
					carry += second * mpeg3_frame_rate(in, stream);
					frame = (total_frames - 1 - carry);

					buffer[i] = ((code >> 24) & 0x80) | (hour << 2) | (minute >> 4);
					buffer[i + 1] = ((code >> 16) & 0x08) | ((minute & 0xf) << 4) | (second >> 3);
					buffer[i + 2] = ((second & 0x7) << 5) | (frame >> 1);
					buffer[i + 3] = (code & 0x7f) | ((frame & 0x1) << 7);
/* fprintf(stderr, "new: %02d:%02d:%02d:%02d\n", hour, minute, second, frame); */
				}


/* Test 32 bit overflow */
				if(outpath[0])
				{
					if(ftell(out) > 0x7f000000)
					{
						fclose(out);
						out_counter++;
						sprintf(newpath, "%s%03d", outpath, out_counter);
						if(!(out = fopen(newpath, "wb")))
						{
							fprintf(stderr, "Couldn't open %s for writing.\n", newpath);
							exit(1);
						}
					}
				}
/*
 * fprintf(stderr, "mpeg3cat 5 %02x %02x %02x %02x\n", 
 * 	(buffer + offset)[0], 
 * 	(buffer + offset)[1],
 * 	(buffer + offset)[2], 
 * 	(buffer + offset)[3]);
 */

/* Write the frame */
				result = !write_output(buffer + offset, output_size - offset, in);
				if(result)
				{
					perror("write video chunk");
					break;
				}
			}
		}
		else
/* Output program stream */
/* In real life, program streams ended up having discontinuities in time codes */
/* so this isn't being maintained anymore. */
		if(in->is_program_stream)
		{
			mpeg3_demuxer_t *demuxer = in->vtrack[0]->demuxer;
			result = 0;

/* Append program stream with no changes */
			demuxer->read_all = 1;
			mpeg3demux_seek_byte(demuxer, MPEG3_START_BYTE);


			while(!result)
			{
				result = mpeg3_seek_phys(demuxer);


				if(!result) 
				{
					demuxer->data_size = 0;
					demuxer->video_size = 0;
					demuxer->audio_size = 0;
					result = mpeg3demux_read_program(demuxer);
					if(result)
						fprintf(stderr, "Hit end of data in %s\n", inpath);
				}


// Read again and decrypt it
				unsigned char *raw_data = malloc(0x10000);
				int raw_size = 0;
				if(!result)
				{
					mpeg3_title_t *title = demuxer->titles[demuxer->current_title];
					int64_t temp_offset = mpeg3io_tell(title->fs);
					int64_t decryption_offset = demuxer->last_packet_decryption - demuxer->last_packet_start;
					raw_size = demuxer->last_packet_end - demuxer->last_packet_start;

					mpeg3io_seek(title->fs, demuxer->last_packet_start);
					mpeg3io_read_data(raw_data, raw_size, title->fs);
					mpeg3io_seek(title->fs, temp_offset);


					if(decryption_offset > 0 && 
						decryption_offset < raw_size &&
						raw_data[decryption_offset] & 0x30)
					{
						if(mpeg3_decrypt_packet(title->fs->css, 
							raw_data,
							0))
						{
							fprintf(stderr, "get_ps_pes_packet: Decryption not available\n");
							return 1;
						}
						raw_data[decryption_offset] &= 0xcf;
					}
				}

// Write it
				if(!result)
				{
					result = !write_output(raw_data, raw_size, in);
					total_written += raw_size;
					if(result) fprintf(stderr, "write program stream: %s\n", strerror(errno));
				}

				free(raw_data);
			}
		}
		else
/* No transport stream support, since these can be catted */
		{
			fprintf(stderr, "No catting of transport streams.\n");
			mpeg3_close(in);
			in = 0;
			continue;
		}

		mpeg3_close(in);
		in = 0;
		current_output_file++;
	}

#ifdef TODO
/* Terminate output */
	if(current_output_file > 0 && do_video)
	{
/* Write new end of sequence */
/* Not very useful */
		buffer[0] = MPEG3_SEQUENCE_END_CODE >> 24;
		buffer[1] = (MPEG3_SEQUENCE_END_CODE >> 16) & 0xff;
		buffer[2] = (MPEG3_SEQUENCE_END_CODE >> 8) & 0xff;
		buffer[3] = MPEG3_SEQUENCE_END_CODE & 0xff;
		result = !write_output(buffer, 4, in);
	}
#endif


	if(outpath[0]) fclose(out);

	exit(0);
}
