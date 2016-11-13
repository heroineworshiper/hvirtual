
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILEFFMPEG_H
#define FILEFFMPEG_H


#include "asset.inc" 
#include "filebase.h"
#include "file.inc"
#include "mutex.inc"


// Decoding for all FFMPEG formats


// Handler for audio streams
class FileFFMPEGStream
{
public:
	FileFFMPEGStream();
	~FileFFMPEGStream();

	void update_pcm_history(int64_t current_sample, int64_t len);
	void append_history(void *frame2, int len);
	void read_history(double *dst,
		int64_t start_sample, 
		int channel,
		int64_t len);
	void allocate_history(int len);


	void *ffmpeg_file_context;

// Video
// Next read positions
	int64_t current_frame;
// Last decoded positions
	int64_t decoded_frame;
	int first_frame;
	

// Audio
// Interleaved samples
	double **pcm_history;
	int64_t history_allocated;
	int64_t history_size;
	int64_t history_start;
	int64_t decode_start;
	int64_t decode_len;
	int64_t decode_end;
	int channels;
	int64_t current_sample;
	int64_t decoded_sample;

// Number of the stream in the ffmpeg array
	int index;
};


class FileFFMPEG : public FileBase
{
public:
	FileFFMPEG(Asset *asset, File *file);
	~FileFFMPEG();

// Get format string for ffmpeg
	static char* get_format_string(Asset *asset);
	static int check_sig(Asset *asset);
	void reset();
	int open_file(int rd, int wr);
	int close_file();

	int64_t get_memory_usage();
	int colormodel_supported(int colormodel);
	static int get_best_colormodel(Asset *asset, int driver);
	int read_frame(VFrame *frame);
	int read_samples(double *buffer, int64_t len);

	void dump_context(void *ptr);

	ArrayList<FileFFMPEGStream*> audio_streams;
	ArrayList<FileFFMPEGStream*> video_streams;

	static Mutex *ffmpeg_lock;
};





#endif
