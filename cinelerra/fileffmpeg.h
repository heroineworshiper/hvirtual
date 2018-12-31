
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

// Decoding for all FFMPEG formats

#include "asset.inc" 
#include "filebase.h"
#include "file.inc"
#include "mutex.inc"
#include "preferences.inc"



// Handler for audio streams
class FileFFMPEGStream
{
public:
	FileFFMPEGStream();
	~FileFFMPEGStream();

	void update_pcm_history(int64_t current_sample);
	void append_history(void *frame2, int len);
	void read_history(double *dst,
		int64_t start_sample, 
		int channel,
		int64_t len);

    void append_index(void *ptr, Asset *asset, Preferences *preferences);
    void flush_index();
    void delete_index();

	void *ffmpeg_file_context;

// Video
// Next read positions
	int64_t current_frame;
// Last decoded positions
	int64_t decoded_frame;
	int first_frame;
    int is_video;
	

// Audio
// 1 buffer for each channel
	double **pcm_history;
	int64_t history_size;
// start of the history buffer in the source
	int64_t history_start;
// position in the history buffer of history_start + history_size
    int write_offset;
	int channels;
// total samples detected during toc creation
    int total_samples;
    int is_audio;



// table of contents data
// offset vs chunk
    ArrayList<int64_t> audio_offsets;
// samples in each chunk.  Uses less space than absolute sample numbers.
    ArrayList<int32_t> audio_samples;
// next offset to be stored when audio is decoded
    int64_t next_audio_offset;
// offset vs frame
    ArrayList<int64_t> video_offsets;
// keyframe numbers
    ArrayList<int32_t> video_keyframes;
/* Buffer of frames for index.  A frame is a high/low pair of audio samples. */
	float **index_data;
/* Number of high/low pairs allocated in each index channel. */
	int index_allocated;
/* Number of high/low pairs per index channel */
	int index_size;
/* Downsampling of index buffers when constructing index */
	int index_zoom;
// frames yet to be added to the index_data
    float **next_index_frame;
    int next_index_size;
    int next_index_allocated;


// Number of the ffmpeg stream
	int ffmpeg_id;
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
    int create_toc(void *ptr);
//    int get_index(char *index_path);
    static int read_index_state(FILE *fd, Indexable *dst);

	int64_t get_memory_usage();
	int colormodel_supported(int colormodel);
	static int get_best_colormodel(Asset *asset, int driver);
	int read_frame(VFrame *frame);
	int read_samples(double *buffer, int64_t len);

// get the stream for seeking
    int get_seek_stream();
	void dump_context(void *ptr);

	ArrayList<FileFFMPEGStream*> audio_streams;
	ArrayList<FileFFMPEGStream*> video_streams;

// last decoded video frame for redisplay
    void *ffmpeg_frame;
    int got_frame;
	static Mutex *ffmpeg_lock;
    int has_toc;
};





#endif
