#include "colormodels.h"
#include "funcprotos.h"
#include "qtasf_codes.h"
#include "quicktime.h"
#include <string.h>
#include <sys/stat.h>
#include "workarounds.h"

int quicktime_make_streamable(char *in_path, char *out_path)
{
	quicktime_t file, *old_file, new_file;
	int moov_exists = 0, mdat_exists = 0, result, atoms = 1;
	int64_t mdat_start, mdat_size;
	quicktime_atom_t leaf_atom;
	int64_t moov_start, moov_end;
	int ftyp_exists = 0;
	int ftyp_size = 0;
	unsigned char *ftyp_data = 0;
	
	quicktime_init(&file);

/* find the moov atom in the old file */
	
	if(!(file.stream = fopen(in_path, "rb")))
	{
		perror("quicktime_make_streamable");
		return 1;
	}

	file.total_length = quicktime_get_file_length(in_path);

/* get the locations of moov and mdat atoms */
	do
	{
		result = quicktime_atom_read_header(&file, &leaf_atom);
//printf("0x%llx %s\n", quicktime_position(&file), leaf_atom.type);

		if(!result)
		{
			if(quicktime_atom_is(&leaf_atom, "ftyp"))
			{
				ftyp_exists = 1;
				ftyp_data = calloc(1, leaf_atom.size);
				ftyp_size = leaf_atom.size;
				quicktime_set_position(&file, 
					quicktime_position(&file) - HEADER_LENGTH);
				quicktime_read_data(&file, ftyp_data, ftyp_size);
			}
			else
			if(quicktime_atom_is(&leaf_atom, "moov"))
			{
				moov_exists = atoms;
			}
			else
			if(quicktime_atom_is(&leaf_atom, "mdat"))
			{
				mdat_start = quicktime_position(&file) - HEADER_LENGTH;
				mdat_size = leaf_atom.size;
				mdat_exists = atoms;
			}

			quicktime_atom_skip(&file, &leaf_atom);

			atoms++;
		}
	}while(!result && quicktime_position(&file) < file.total_length);

	fclose(file.stream);

	if(!moov_exists)
	{
		printf("quicktime_make_streamable: no moov atom\n");
		if(ftyp_data) free(ftyp_data);
		return 1;
	}

	if(!mdat_exists)
	{
		printf("quicktime_make_streamable: no mdat atom\n");
		if(ftyp_data) free(ftyp_data);
		return 1;
	}

/* copy the old file to the new file */
	if(moov_exists && mdat_exists)
	{
/* moov wasn't the first atom */
		if(moov_exists > 1)
		{
			char *buffer;
			int64_t buf_size = 1000000;

			result = 0;

/* read the header proper */
			if(!(old_file = quicktime_open(in_path, 1, 0)))
			{
				if(ftyp_data) free(ftyp_data);
				return 1;
			}


/* open the output file */
			if(!(new_file.stream = fopen(out_path, "wb")))
			{
				perror("quicktime_make_streamable");
				result =  1;
			}
			else
			{
/* set up some flags */
				new_file.wr = 1;
				new_file.rd = 0;
				
/* Write ftyp header */
				if(ftyp_exists) 
				{
					quicktime_write_data(&new_file, ftyp_data, ftyp_size);
				}
					

/* Write moov once to get final size with our substituted headers */
				moov_start = quicktime_position(&new_file);
				quicktime_write_moov(&new_file, &(old_file->moov), 0);
				moov_end = quicktime_position(&new_file);

printf("make_streamable 0x%llx 0x%llx\n", (long long)moov_end - moov_start, (long long)mdat_start);
				quicktime_shift_offsets(&(old_file->moov), 
					moov_end - moov_start - mdat_start + ftyp_size);

/* Write again with shifted offsets */
				quicktime_set_position(&new_file, moov_start);
				quicktime_write_moov(&new_file, &(old_file->moov), 0);
				quicktime_set_position(old_file, mdat_start);

				if(!(buffer = calloc(1, buf_size)))
				{
					result = 1;
					printf("quicktime_make_streamable: out of memory\n");
				}
				else
				{
					while(quicktime_position(old_file) < mdat_start + mdat_size && !result)
					{
						if(quicktime_position(old_file) + buf_size > mdat_start + mdat_size)
							buf_size = mdat_start + mdat_size - quicktime_position(old_file);

						if(!quicktime_read_data(old_file, buffer, buf_size)) result = 1;
						if(!result)
						{
							if(!quicktime_write_data(&new_file, buffer, buf_size)) result = 1;
						}
					}
					free(buffer);
				}
				fclose(new_file.stream);
			}
			quicktime_close(old_file);
		}
		else
		{
			printf("quicktime_make_streamable: header already at 0 offset\n");
			if(ftyp_data) free(ftyp_data);
			return 0;
		}
	}

	if(ftyp_data) free(ftyp_data);
	return 0;
}



void quicktime_set_copyright(quicktime_t *file, const char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.copyright), &(file->moov.udta.copyright_len), string);
}

void quicktime_set_name(quicktime_t *file, const char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.name), &(file->moov.udta.name_len), string);
}

void quicktime_set_info(quicktime_t *file, const char *string)
{
	quicktime_set_udta_string(&(file->moov.udta.info), &(file->moov.udta.info_len), string);
}

char* quicktime_get_copyright(quicktime_t *file)
{
	return file->moov.udta.copyright;
}

char* quicktime_get_name(quicktime_t *file)
{
	return file->moov.udta.name;
}

char* quicktime_get_info(quicktime_t *file)
{
	return file->moov.udta.info;
}


int quicktime_video_tracks(quicktime_t *file)
{
	int i, result = 0;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		if(file->moov.trak[i]->mdia.minf.is_video) result++;
	}
	return result;
}

int quicktime_audio_tracks(quicktime_t *file)
{
	int i, result = 0;
	quicktime_minf_t *minf;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		minf = &(file->moov.trak[i]->mdia.minf);
		if(minf->is_audio)
			result++;
	}
	return result;
}

int quicktime_set_audio(quicktime_t *file, 
						int channels,
						long sample_rate,
						int bits,
						char *compressor)
{
	quicktime_trak_t *trak;

/* allocate an arbitrary number of tracks */
	if(channels)
	{
/* Fake the bits parameter for some formats. */
		if(quicktime_match_32(compressor, QUICKTIME_ULAW) ||
			quicktime_match_32(compressor, QUICKTIME_IMA4)) bits = 16;

		file->atracks = (quicktime_audio_map_t*)calloc(1, sizeof(quicktime_audio_map_t));
		trak = quicktime_add_track(file);
		quicktime_trak_init_audio(file, 
			trak, 
			channels, 
			sample_rate, 
			bits, 
			compressor);
		quicktime_init_audio_map(&(file->atracks[0]), trak);
		file->atracks[file->total_atracks].track = trak;
		file->atracks[file->total_atracks].channels = channels;
		file->atracks[file->total_atracks].current_position = 0;
		file->atracks[file->total_atracks].current_chunk = 1;
		file->total_atracks++;
	}
	return 1;   /* Return the number of tracks created */
}

int quicktime_set_video(quicktime_t *file, 
						int tracks, 
						int frame_w, 
						int frame_h,
						double frame_rate,
						char *compressor)
{
	int i;
	quicktime_trak_t *trak;

	if(tracks)
	{
		quicktime_mhvd_init_video(file, &(file->moov.mvhd), frame_rate);
		file->total_vtracks = tracks;
		file->vtracks = (quicktime_video_map_t*)calloc(1, sizeof(quicktime_video_map_t) * file->total_vtracks);
		for(i = 0; i < tracks; i++)
		{
			trak = quicktime_add_track(file);
			quicktime_trak_init_video(file, trak, frame_w, frame_h, frame_rate, compressor);
			quicktime_init_video_map(&(file->vtracks[i]), trak);
		}
	}

	quicktime_set_cache_max(file, file->cache_size);

	return 0;
}

void quicktime_set_framerate(quicktime_t *file, double framerate)
{
	int i;
	int new_time_scale, new_sample_duration;

	if(!file->wr)
	{
		fprintf(stderr, "quicktime_set_framerate shouldn't be called in read mode.\n");
		return;
	}

	new_time_scale = quicktime_get_timescale(framerate);
	new_sample_duration = (int)((double)new_time_scale / framerate + 0.5);

	for(i = 0; i < file->total_vtracks; i++)
	{
		file->vtracks[i].track->mdia.mdhd.time_scale = new_time_scale;
		file->vtracks[i].track->mdia.minf.stbl.stts.table[0].sample_duration = new_sample_duration;
	}
}


// Used by quicktime_set_video when creating a new file
quicktime_trak_t* quicktime_add_track(quicktime_t *file)
{
	quicktime_moov_t *moov = &(file->moov);
	quicktime_trak_t *trak;
	int i;

	for(i = moov->total_tracks; i > 0; i--)
		moov->trak[i] = moov->trak[i - 1];

	trak = 
		moov->trak[0] = 
		calloc(1, sizeof(quicktime_trak_t));
	quicktime_trak_init(trak);
	moov->total_tracks++;

	for(i = 0; i < moov->total_tracks; i++)
		moov->trak[i]->tkhd.track_id = i + 1;
	moov->mvhd.next_track_id++;
	return trak;
}

/* ============================= Initialization functions */

int quicktime_init(quicktime_t *file)
{
	bzero(file, sizeof(quicktime_t));
	quicktime_moov_init(&(file->moov));
	file->cpus = 1;
	file->color_model = BC_RGB888;
	return 0;
}

int quicktime_delete(quicktime_t *file)
{
	int i;
	if(file->total_atracks) 
	{
		for(i = 0; i < file->total_atracks; i++)
			quicktime_delete_audio_map(&(file->atracks[i]));
		free(file->atracks);
	}

	if(file->total_vtracks)
	{
		for(i = 0; i < file->total_vtracks; i++)
			quicktime_delete_video_map(&(file->vtracks[i]));
		free(file->vtracks);
	}

	file->total_atracks = 0;
	file->total_vtracks = 0;

	if(file->moov_data)
		free(file->moov_data);

	if(file->preload_size)
	{
		free(file->preload_buffer);
		file->preload_size = 0;
	}

	if(file->presave_buffer)
	{
		free(file->presave_buffer);
	}

	for(i = 0; i < file->total_riffs; i++)
	{
		quicktime_delete_riff(file, file->riff[i]);
	}

	quicktime_moov_delete(&(file->moov));
	quicktime_mdat_delete(&(file->mdat));
	quicktime_delete_asf(file->asf);
	return 0;
}

/* =============================== Optimization functions */

int quicktime_set_cpus(quicktime_t *file, int cpus)
{
	if(cpus > 0) file->cpus = cpus;
	return 0;
}

void quicktime_set_preload(quicktime_t *file, int64_t preload)
{
	file->preload_size = preload;
	if(file->preload_buffer) free(file->preload_buffer);
	file->preload_buffer = 0;
	if(preload)
		file->preload_buffer = calloc(1, preload);
	file->preload_start = 0;
	file->preload_end = 0;
	file->preload_ptr = 0;
}


int quicktime_get_timescale(double frame_rate)
{
	int timescale = 600;
/* Encode the 29.97, 23.976, 59.94 framerates */
	if(frame_rate - (int)frame_rate != 0) 
		timescale = (int)(frame_rate * 1001 + 0.5);
	else
	if((600 / frame_rate) - (int)(600 / frame_rate) != 0) 
			timescale = (int)(frame_rate * 100 + 0.5);
//printf("quicktime_get_timescale %f %d\n", 600 / frame_rate, (int)(600 / frame_rate));
	return timescale;
}

int quicktime_seek_end(quicktime_t *file)
{
	quicktime_set_position(file, file->mdat.atom.size + file->mdat.atom.start + HEADER_LENGTH * 2);
/*printf("quicktime_seek_end %ld\n", file->mdat.atom.size + file->mdat.atom.start); */
	quicktime_update_positions(file);
	return 0;
}

int quicktime_seek_start(quicktime_t *file)
{
	quicktime_set_position(file, file->mdat.atom.start + HEADER_LENGTH * 2);
	quicktime_update_positions(file);
	return 0;
}

long quicktime_audio_length(quicktime_t *file, int track)
{
	if(file->total_atracks > 0) 
		return quicktime_track_samples(file, file->atracks[track].track);

	return 0;
}

long quicktime_video_length(quicktime_t *file, int track)
{
/*printf("quicktime_video_length %d %d\n", quicktime_track_samples(file, file->vtracks[track].track), track); */
	if(file->total_vtracks > 0)
		return quicktime_track_samples(file, file->vtracks[track].track);
	return 0;
}

long quicktime_audio_position(quicktime_t *file, int track)
{
	return file->atracks[track].current_position;
}

long quicktime_video_position(quicktime_t *file, int track)
{
	return file->vtracks[track].current_position;
}

int quicktime_update_positions(quicktime_t *file)
{
/* Get the sample position from the file offset */
/* for routines that change the positions of all tracks, like */
/* seek_end and seek_start but not for routines that reposition one track, like */
/* set_audio_position. */

	int64_t mdat_offset = quicktime_position(file) - file->mdat.atom.start;
	int64_t sample, chunk, chunk_offset;
	int i;

	if(file->total_atracks)
	{
		sample = quicktime_offset_to_sample(file->atracks[0].track, mdat_offset);
		chunk = quicktime_offset_to_chunk(&chunk_offset, file->atracks[0].track, mdat_offset);
		for(i = 0; i < file->total_atracks; i++)
		{
			file->atracks[i].current_position = sample;
			file->atracks[i].current_chunk = chunk;
		}
	}

	if(file->total_vtracks)
	{
		sample = quicktime_offset_to_sample(file->vtracks[0].track, mdat_offset);
		chunk = quicktime_offset_to_chunk(&chunk_offset, file->vtracks[0].track, mdat_offset);
		for(i = 0; i < file->total_vtracks; i++)
		{
			file->vtracks[i].current_position = sample;
			file->vtracks[i].current_chunk = chunk;
		}
	}
	return 0;
}

int quicktime_set_audio_position(quicktime_t *file, int64_t sample, int track)
{
	int64_t offset, chunk_sample, chunk;
	quicktime_trak_t *trak;

	if(track < file->total_atracks)
	{
		trak = file->atracks[track].track;
		file->atracks[track].current_position = sample;
		quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, sample);
		file->atracks[track].current_chunk = chunk;
		offset = quicktime_sample_to_offset(file, trak, sample);
		quicktime_set_position(file, offset);
	}
	else
		fprintf(stderr, "quicktime_set_audio_position: track >= file->total_atracks\n");

	return 0;
}

int quicktime_set_video_position(quicktime_t *file, int64_t frame, int track)
{
	int64_t offset, chunk_sample, chunk;
	quicktime_trak_t *trak;
	if(track >= file->total_vtracks)
	{
		fprintf(stderr, 
			"quicktime_set_video_position: frame=%lld track=%d >= file->total_vtracks %d\n", 
			(long long)frame,
			track,
			file->total_vtracks);
		track = file->total_vtracks - 1;
	}

	if(track < file->total_vtracks && track >= 0)
	{
		trak = file->vtracks[track].track;
		file->vtracks[track].current_position = frame;
		quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, frame);
		file->vtracks[track].current_chunk = chunk;
		offset = quicktime_sample_to_offset(file, trak, frame);
		quicktime_set_position(file, offset);
	}
	return 0;
}

int quicktime_has_audio(quicktime_t *file)
{
	if(quicktime_audio_tracks(file)) return 1;
	return 0;
}

long quicktime_sample_rate(quicktime_t *file, int track)
{
	if(file->total_atracks)
	{
		quicktime_trak_t *trak = file->atracks[track].track;
		return trak->mdia.minf.stbl.stsd.table[0].sample_rate;
	}
	return 0;
}

int quicktime_audio_bits(quicktime_t *file, int track)
{
	if(file->total_atracks)
		return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].sample_size;

	return 0;
}

char* quicktime_audio_compressor(quicktime_t *file, int track)
{
	return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].format;
}

int quicktime_track_channels(quicktime_t *file, int track)
{
	if(track < file->total_atracks)
		return file->atracks[track].channels;

	return 0;
}

int quicktime_channel_location(quicktime_t *file, int *quicktime_track, int *quicktime_channel, int channel)
{
	int current_channel = 0, current_track = 0;
	*quicktime_channel = 0;
	*quicktime_track = 0;
	for(current_channel = 0, current_track = 0; current_track < file->total_atracks; )
	{
		if(channel >= current_channel)
		{
			*quicktime_channel = channel - current_channel;
			*quicktime_track = current_track;
		}

		current_channel += file->atracks[current_track].channels;
		current_track++;
	}
	return 0;
}

int quicktime_has_video(quicktime_t *file)
{
	if(quicktime_video_tracks(file)) return 1;
	return 0;
}

int quicktime_video_width(quicktime_t *file, int track)
{
	if(file->total_vtracks)
		return file->vtracks[track].track->tkhd.track_width;
	return 0;
}

int quicktime_video_height(quicktime_t *file, int track)
{
	if(file->total_vtracks)
		return file->vtracks[track].track->tkhd.track_height;
	return 0;
}

int quicktime_video_depth(quicktime_t *file, int track)
{
	if(file->total_vtracks)
		return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].depth;
	return 0;
}

void quicktime_set_cmodel(quicktime_t *file, int colormodel)
{
	file->color_model = colormodel;
}

void quicktime_set_row_span(quicktime_t *file, int row_span)
{
	file->row_span = row_span;
}

void quicktime_set_window(quicktime_t *file,
	int in_x,                    /* Location of input frame to take picture */
	int in_y,
	int in_w,
	int in_h,
	int out_w,                   /* Dimensions of output frame */
	int out_h)
{
	if(in_x >= 0 && in_y >= 0 && in_w > 0 && in_h > 0 && out_w > 0 && out_h > 0)
	{
		file->do_scaling = 1;
		file->in_x = in_x;
		file->in_y = in_y;
		file->in_w = in_w;
		file->in_h = in_h;
		file->out_w = out_w;
		file->out_h = out_h;
	}
	else
	{
		file->do_scaling = 0;
/* quicktime_decode_video now sets the window for every frame based on the */
/* track dimensions */
	}
}

void quicktime_set_depth(quicktime_t *file, int depth, int track)
{
	int i;

	for(i = 0; i < file->total_vtracks; i++)
	{
		file->vtracks[i].track->mdia.minf.stbl.stsd.table[0].depth = depth;
	}
}

double quicktime_frame_rate(quicktime_t *file, int track)
{
	if(file->total_vtracks > track)
	{
		quicktime_trak_t *trak = file->vtracks[track].track;
		int time_scale = file->vtracks[track].track->mdia.mdhd.time_scale;
		int sample_duration = quicktime_sample_duration(trak);
		return (double)time_scale / sample_duration;
//		return (float)file->vtracks[track].track->mdia.mdhd.time_scale / 
//			file->vtracks[track].track->mdia.minf.stbl.stts.table[0].sample_duration;
	}
	return 0;
}

int quicktime_frame_rate_n(quicktime_t *file, int track)
{
	if(file->total_vtracks > track)
		return file->vtracks[track].track->mdia.mdhd.time_scale;
	return 0;
}

int quicktime_frame_rate_d(quicktime_t *file, int track)
{
	if(file->total_vtracks > track)
		return file->vtracks[track].track->mdia.minf.stbl.stts.table[0].sample_duration;
	return 0;
}

char* quicktime_video_compressor(quicktime_t *file, int track)
{
	return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].format;
}

int quicktime_write_audio(quicktime_t *file, 
	char *audio_buffer, 
	long samples, 
	int track)
{
	int result;
	int64_t bytes;
	quicktime_atom_t chunk_atom;
	quicktime_audio_map_t *track_map = &file->atracks[track];
	quicktime_trak_t *trak = track_map->track;

/* write chunk for 1 track */
	bytes = samples * quicktime_audio_bits(file, track) / 8 * file->atracks[track].channels;
	quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, audio_buffer, bytes);
	quicktime_write_chunk_footer(file, 
					trak,
					track_map->current_chunk,
					&chunk_atom, 
					samples);

/*	file->atracks[track].current_position += samples; */
	file->atracks[track].current_chunk++;
	return result;
}

int quicktime_write_frame(quicktime_t *file, 
	unsigned char *video_buffer, 
	int64_t bytes, 
	int track)
{
	int64_t offset = quicktime_position(file);
	int result = 0;
	quicktime_atom_t chunk_atom;
	quicktime_video_map_t *vtrack = &file->vtracks[track];
	quicktime_trak_t *trak = vtrack->track;

	if(!bytes) return 0;

	quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, video_buffer, bytes);
	quicktime_write_chunk_footer(file, 
					trak,
					vtrack->current_chunk,
					&chunk_atom, 
					1);
	file->vtracks[track].current_position++;
	file->vtracks[track].current_chunk++;
	return result;
}


long quicktime_read_audio(quicktime_t *file, 
	char *audio_buffer, 
	long samples, 
	int track)
{
	int64_t chunk_sample, chunk;
	int result = 0, track_num;
	quicktime_trak_t *trak = file->atracks[track].track;
	int64_t fragment_len, chunk_end;
	int64_t start_position = file->atracks[track].current_position;
	int64_t position = file->atracks[track].current_position;
	int64_t start = position, end = position + samples;
	int64_t bytes, total_bytes = 0;
	int64_t buffer_offset;

//printf("quicktime_read_audio 1\n");
	quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, position);
	buffer_offset = 0;

	while(position < end && !result)
	{
		quicktime_set_audio_position(file, position, track);
		fragment_len = quicktime_chunk_samples(trak, chunk);
		chunk_end = chunk_sample + fragment_len;
		fragment_len -= position - chunk_sample;
		if(position + fragment_len > chunk_end) fragment_len = chunk_end - position;
		if(position + fragment_len > end) fragment_len = end - position;

		bytes = quicktime_samples_to_bytes(trak, fragment_len);
/*
 * printf("quicktime_read_audio 2 %llx %llx %d\n", 
 * quicktime_position(file), 
 * quicktime_position(file) + bytes, 
 * samples);
 * sleep(1);
 */
		result = !quicktime_read_data(file, &audio_buffer[buffer_offset], bytes);
//printf("quicktime_read_audio 4\n");

		total_bytes += bytes;
		position += fragment_len;
		chunk_sample = position;
		buffer_offset += bytes;
		chunk++;
	}
//printf("quicktime_read_audio 5\n");

// Create illusion of track being advanced only by samples
	file->atracks[track].current_position = start_position + samples;
	if(result) return 0;
	return total_bytes;
}

int quicktime_read_chunk(quicktime_t *file, char *output, int track, int64_t chunk, int64_t byte_start, int64_t byte_len)
{
	quicktime_set_position(file, 
		quicktime_chunk_to_offset(file, file->atracks[track].track, chunk) + 
		byte_start);
	if(quicktime_read_data(file, output, byte_len)) return 0;
	else
	return 1;
}

long quicktime_frame_size(quicktime_t *file, long frame, int track)
{
	long bytes = 0;
	quicktime_trak_t *trak = file->vtracks[track].track;

	if(trak->mdia.minf.stbl.stsz.sample_size)
	{
		bytes = trak->mdia.minf.stbl.stsz.sample_size;
	}
	else
	{
		long total_frames = quicktime_track_samples(file, trak);
		if(frame < 0) frame = 0;
		else
		if(frame > total_frames - 1) frame = total_frames - 1;
		bytes = trak->mdia.minf.stbl.stsz.table[frame].size;
	}


	return bytes;
}


long quicktime_read_frame(quicktime_t *file, unsigned char *video_buffer, int track)
{
	int64_t bytes;
	int result = 0;

	quicktime_trak_t *trak = file->vtracks[track].track;
	bytes = quicktime_frame_size(file, file->vtracks[track].current_position, track);

	quicktime_set_video_position(file, file->vtracks[track].current_position, track);

/*
 * printf("quicktime_read_frame 0x%llx %lld\n", 
 * quicktime_ftell(file),
 * bytes);
 */
	result = quicktime_read_data(file, video_buffer, bytes);
	file->vtracks[track].current_position++;

	if(!result) return 0;
	return bytes;
}

int64_t quicktime_get_keyframe_before(quicktime_t *file, int64_t frame, int track)
{
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
	int i;





// Offset 1
	frame++;


	for(i = stss->total_entries - 1; i >= 0; i--)
	{
		if(stss->table[i].sample <= frame) return stss->table[i].sample - 1;
	}

	return 0;
}

int64_t quicktime_get_keyframe_after(quicktime_t *file, int64_t frame, int track)
{
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
	int i;





// Offset 1
	frame++;


	for(i = 0; i < stss->total_entries; i++)
	{
		if(stss->table[i].sample >= frame) return stss->table[i].sample - 1;
	}

	return 0;
}

void quicktime_insert_keyframe(quicktime_t *file, int64_t frame, int track)
{
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
	int i;

// Set keyframe flag in idx1 table.
// Only possible in the first RIFF.  After that, there's no keyframe support.
	if(file->use_avi && file->total_riffs == 1)
		quicktime_set_idx1_keyframe(file, 
			trak,
			frame);

// Offset 1
	frame++;


// Get the keyframe greater or equal to new frame
	for(i = 0; i < stss->total_entries; i++)
	{
		if(stss->table[i].sample >= frame) break;
	}

// Expand table
	if(stss->entries_allocated <= stss->total_entries)
	{
		stss->entries_allocated *= 2;
		stss->table = realloc(stss->table, sizeof(quicktime_stss_table_t) * stss->entries_allocated);
	}

// Insert before existing frame
	if(i < stss->total_entries)
	{
		if(stss->table[i].sample > frame)
		{
			int j, k;
			for(j = stss->total_entries, k = stss->total_entries - 1;
				k >= i;
				j--, k--)
			{
				stss->table[j] = stss->table[k];
			}
			stss->table[i].sample = frame;
		}
	}
	else
// Insert after last frame
		stss->table[i].sample = frame;

	stss->total_entries++;
}


int quicktime_has_keyframes(quicktime_t *file, int track)
{
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
	
	return stss->total_entries > 0;
}





int quicktime_read_frame_init(quicktime_t *file, int track)
{
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_set_video_position(file, file->vtracks[track].current_position, track);
	if(quicktime_ftell(file) != file->file_position) 
	{
		FSEEK(file->stream, file->file_position, SEEK_SET);
		file->ftell_position = file->file_position;
	}
	return 0;
}

int quicktime_read_frame_end(quicktime_t *file, int track)
{
	file->file_position = quicktime_ftell(file);
	file->vtracks[track].current_position++;
	return 0;
}

int quicktime_init_video_map(quicktime_video_map_t *vtrack, quicktime_trak_t *trak)
{
	vtrack->track = trak;
	vtrack->current_position = 0;
	vtrack->current_chunk = 1;
	quicktime_init_vcodec(vtrack);
	vtrack->frame_cache = quicktime_new_cache();
	return 0;
}

int quicktime_delete_video_map(quicktime_video_map_t *vtrack)
{
	int i;
	quicktime_delete_vcodec(vtrack);
	if(vtrack->frame_cache) quicktime_delete_cache(vtrack->frame_cache);
	vtrack->frame_cache = 0;
	return 0;
}

int64_t quicktime_memory_usage(quicktime_t *file)
{
	int i;
	int64_t result = 0;
//printf("quicktime_memory_usage %d\n", file->total_vtracks);
	for(i = 0; i < file->total_vtracks; i++)
	{
		result += quicktime_cache_usage(file->vtracks[i].frame_cache);
	}
	return result;
}

void quicktime_set_cache_max(quicktime_t *file, int bytes)
{
	int i;
	file->cache_size = bytes;


//printf("quicktime_set_cache_max %d %d %d\n", __LINE__, bytes, file->total_vtracks);
	for(i = 0; i < file->total_vtracks; i++)
	{
		quicktime_cache_max(file->vtracks[i].frame_cache, bytes);
	}
}





int quicktime_init_audio_map(quicktime_audio_map_t *atrack, quicktime_trak_t *trak)
{
	atrack->track = trak;
	atrack->channels = trak->mdia.minf.stbl.stsd.table[0].channels;
	atrack->current_position = 0;
	atrack->current_chunk = 1;
	quicktime_init_acodec(atrack);
	return 0;
}

int quicktime_delete_audio_map(quicktime_audio_map_t *atrack)
{
	int i;
	quicktime_delete_acodec(atrack);
	quicktime_clear_vbr(&atrack->vbr);
	return 0;
}

void quicktime_init_maps(quicktime_t *file)
{
	int i, track;
/* get tables for all the different tracks */
	file->total_atracks = quicktime_audio_tracks(file);
	file->atracks = (quicktime_audio_map_t*)calloc(1, sizeof(quicktime_audio_map_t) * file->total_atracks);

	for(i = 0, track = 0; i < file->total_atracks; i++)
	{
		while(!file->moov.trak[track]->mdia.minf.is_audio)
			track++;
		quicktime_init_audio_map(&(file->atracks[i]), file->moov.trak[track]);
	}

	file->total_vtracks = quicktime_video_tracks(file);
	file->vtracks = (quicktime_video_map_t*)calloc(1, sizeof(quicktime_video_map_t) * file->total_vtracks);

	for(track = 0, i = 0; i < file->total_vtracks; i++)
	{
		while(!file->moov.trak[track]->mdia.minf.is_video)
			track++;

		quicktime_init_video_map(&(file->vtracks[i]), file->moov.trak[track]);
	}

	quicktime_set_cache_max(file, file->cache_size);
}

int quicktime_read_info(quicktime_t *file)
{
	int result = 0, got_header = 0;
	int i, channel, trak_channel, track;
	int64_t start_position = quicktime_position(file);
	quicktime_atom_t leaf_atom;
	quicktime_guid_t guid;
	quicktime_trak_t *trak;
	char avi_avi[4];
	int got_avi = 0;
	int got_asf = 0;
	file->use_avi = 0;
	file->use_asf = 0;

	quicktime_set_position(file, 0LL);

/* Test for ASF */
	quicktime_read_guid(file, &guid);
	quicktime_set_position(file, 0LL);
	if(!memcmp(&guid, &asf_header, sizeof(guid)))
	{
		printf("quicktime_read_info: Got ASF\n");
		got_asf = 1;
		got_header = 1;
	}

/* Test file format */
	if(!got_asf)
	{
		quicktime_set_position(file, 0LL);
		do
		{
			file->use_avi = 1;
			result = quicktime_atom_read_header(file, &leaf_atom);

			if(!result && quicktime_atom_is(&leaf_atom, "RIFF"))
			{
				quicktime_read_data(file, avi_avi, 4);
				if(quicktime_match_32(avi_avi, "AVI "))
				{
					got_avi = 1;
				}
				else
				{
					file->use_avi = 0;
					result = 0;
					break;
				}
			}
			else
			{
				file->use_avi = 0;
				result = 0;
				break;
			}
		}while(1);
	}

	if(got_avi) file->use_avi = 1;
	else
	if(got_asf) file->use_asf = 1;

	quicktime_set_position(file, 0LL);


/* McRoweSoft AVI section */
	if(file->use_avi)
	{
/* Import first RIFF */
		do
		{
			result = quicktime_atom_read_header(file, &leaf_atom);
			if(!result)
			{
				if(quicktime_atom_is(&leaf_atom, "RIFF"))
				{
					quicktime_read_riff(file, &leaf_atom);
/* Return success */
					got_header = 1;
				}
			}
		}while(!result && 
			!got_header &&
			quicktime_position(file) < file->total_length);

/* Construct indexes. */
		quicktime_import_avi(file);
	}
/* Quicktime section */
	else
	if(file->use_asf)
	{
		result = quicktime_read_asf(file);
		if(result) got_header = 0;
		else
		quicktime_dump_asf(file->asf);
	}
	else
	if(!file->use_avi)
	{
		do
		{
			result = quicktime_atom_read_header(file, &leaf_atom);

			if(!result)
			{
				if(quicktime_atom_is(&leaf_atom, "mdat")) 
				{
					quicktime_read_mdat(file, &(file->mdat), &leaf_atom);
				}
				else
				if(quicktime_atom_is(&leaf_atom, "moov")) 
				{
/* Set preload and preload the moov atom here */
					int64_t start_position = quicktime_position(file);
					long temp_size = leaf_atom.end - start_position;
					unsigned char *temp = malloc(temp_size);
					quicktime_set_preload(file, 
						(temp_size < 0x100000) ? 0x100000 : temp_size);
					quicktime_read_data(file, temp, temp_size);
					quicktime_set_position(file, start_position);
					free(temp);

					if(quicktime_read_moov(file, &(file->moov), &leaf_atom))
						return 1;
					got_header = 1;
				}
				else
					quicktime_atom_skip(file, &leaf_atom);
			}
		}while(!result && quicktime_position(file) < file->total_length);










/* go back to the original position */
		quicktime_set_position(file, start_position);

	}

/* Initialize track map objects */
	if(got_header)
	{
		quicktime_init_maps(file);
	}

/* Shut down preload in case of an obsurdly high temp_size */
	quicktime_set_preload(file, 0);

	return !got_header;
}


int quicktime_dump(quicktime_t *file)
{
	printf("quicktime_dump\n");
	printf("movie data\n");
	printf(" size %ld\n", file->mdat.atom.size);
	printf(" start %ld\n", file->mdat.atom.start);
	quicktime_moov_dump(&(file->moov));
	return 0;
}





int quicktime_check_sig(char *path)
{
	quicktime_t file;
	quicktime_atom_t leaf_atom;
	int result = 0, result1 = 0, result2 = 0;
	char avi_test[12];

	quicktime_init(&file);
	result = quicktime_file_open(&file, path, 1, 0);

	if(!result)
	{
// Check for Microsoft AVI
		quicktime_read_data(&file, avi_test, 12);
		quicktime_set_position(&file, 0);
		if(quicktime_match_32(avi_test, "RIFF") && 
			quicktime_match_32(avi_test + 8, "AVI "))
		{
			result2 = 1;
		}



/*
 * 		if(!result2)
 * // Check for Microsoft ASF
 * 		{
 * 			quicktime_guid_t guid;
 * 			quicktime_read_guid(&file, &guid);
 * 			quicktime_set_position(&file, 0);
 * 			if(!memcmp(&guid, &asf_header, sizeof(guid)))
 * 			{
 * 				printf("quicktime_check_sig: Got ASF\n");
 * 				result2 = 1;
 * 			}
 * 		}
 */

		if(!result2)
		{
			do
			{
				result1 = quicktime_atom_read_header(&file, &leaf_atom);

				if(!result1)
				{
/* just want the "moov" atom */
					if(quicktime_atom_is(&leaf_atom, "moov"))
					{
						result2 = 1;
					}
					else
						quicktime_atom_skip(&file, &leaf_atom);
				}
			}while(!result1 && !result2 && quicktime_position(&file) < file.total_length);
		}
	}

//printf(__FUNCTION__ " 2 %d\n", result2);
	quicktime_file_close(&file);
	quicktime_delete(&file);
	return result2;
}

void quicktime_set_avi(quicktime_t *file, int value)
{
	file->use_avi = value;
	quicktime_set_position(file, 0);

// Write RIFF chunk
	quicktime_init_riff(file);
}

int quicktime_is_avi(quicktime_t *file)
{
	return file->use_avi;
}


void quicktime_set_asf(quicktime_t *file, int value)
{
	file->use_asf = value;
}


quicktime_t* quicktime_open(char *filename, int rd, int wr)
{
	quicktime_t *new_file = calloc(1, sizeof(quicktime_t));
	char flags[10];
	int result = 0;

//printf("quicktime_open 1\n");
	quicktime_init(new_file);
	new_file->wr = wr;
	new_file->rd = rd;
	new_file->mdat.atom.start = 0;

	result = quicktime_file_open(new_file, filename, rd, wr);

	if(!result)
	{
		if(rd)
		{
			if(quicktime_read_info(new_file))
			{
				quicktime_close(new_file);
				fprintf(stderr, "quicktime_open: error in header\n");
				new_file = 0;
			}
		}

/* start the data atom */
/* also don't want to do this if making a streamable file */
		if(wr)
		{
			quicktime_set_presave(new_file, 1);




// android requires the ftyp header
			const unsigned char ftyp_data[] = 
			{
				0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70, 0x6d, 0x70, 0x34, 0x32, 0x00, 0x00, 0x00, 0x01, 0x6d, 0x70, 0x34, 0x32, 0x61, 0x76, 0x63, 0x31
			};
			quicktime_write_data(new_file, (unsigned char*)ftyp_data, sizeof(ftyp_data));
			


			quicktime_atom_write_header64(new_file, 
				&new_file->mdat.atom, 
				"mdat");
			quicktime_set_presave(new_file, 0);
		}
	}
	else
	{
//printf("quicktime_open 10\n");
		quicktime_close(new_file);
//printf("quicktime_open 100\n");
		new_file = 0;
	}


	return new_file;
}

int quicktime_close(quicktime_t *file)
{
	int result = 0;
	if(file->wr)
	{
		quicktime_codecs_flush(file);

// Reenable buffer for quick header writing.
		quicktime_set_presave(file, 1);
		if(file->use_avi)
		{
			quicktime_atom_t junk_atom;
			int i;

// Finalize last header
			quicktime_finalize_riff(file, file->riff[file->total_riffs - 1]);

			int64_t position = quicktime_position(file);

// Finalize the odml header
			quicktime_finalize_odml(file, &file->riff[0]->hdrl);

// Finalize super indexes
			quicktime_finalize_indx(file);

// Pad ending
			quicktime_set_position(file, position);
			quicktime_atom_write_header(file, &junk_atom, "JUNK");
			for(i = 0; i < 0x406; i++)
				quicktime_write_int32_le(file, 0);
			quicktime_atom_write_footer(file, &junk_atom);
		}
		else
		{
// Atoms are only written here
			if(file->stream)
			{
				quicktime_write_moov(file, &(file->moov), 1);
				quicktime_atom_write_footer(file, &file->mdat.atom);
			}
		}
	}

	quicktime_file_close(file);

	quicktime_delete(file);
	free(file);
	return result;
}

int quicktime_major()
{
	return QUICKTIME_MAJOR;
}

int quicktime_minor()
{
	return QUICKTIME_MINOR;
}

int quicktime_release()
{
	return QUICKTIME_RELEASE;
}

