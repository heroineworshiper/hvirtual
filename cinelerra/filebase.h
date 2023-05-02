/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILEBASE_H
#define FILEBASE_H

#include "asset.inc"
#include "assets.inc"
#include "colormodels.h"
#include "edit.inc"
#include "guicast.h"
#include "file.inc"
#include "filebase.inc"
#include "filelist.inc"
#include "overlayframe.inc"
#include "strategies.inc"
#include "vframe.inc"

#include <sys/types.h>

// Number of samples saved before the current read position
#define HISTORY_MAX 0x100000

// inherited by every file interpreter
class FileBase
{
public:
	FileBase(Asset *asset, File *file);
// table entry
	FileBase();
	virtual ~FileBase();


	friend class File;
	friend class FileList;
	friend class FrameWriter;


// table functions for file handler
    virtual int check_sig(File *file, const uint8_t *test_data);
    virtual FileBase* create(File *file);
    virtual int get_best_colormodel(Asset *asset, int driver);
    virtual void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int option_type,
		const char *locked_compressor);
// must not translate languages here, since it's written to the EDL
    virtual const char* formattostr(int format);
    virtual const char* get_tag(int format);


// convert the read & write permissions into a stdio code
	int get_mode(char *mode, int rd, int wr);

//	virtual int get_index(char *index_path) { return 1; };
// reset_parameters_derived is not available until after the constructor
	virtual int reset_parameters_derived();
	virtual int open_file(int rd, int wr) { return 0; };
	virtual int close_file();
	virtual int close_file_derived() { return 0; };
	int set_dither();
	virtual int seek_end() { return 0; };
	virtual int64_t get_video_position() { return 0; };
	virtual int64_t get_audio_position() { return 0; };
	virtual int set_video_position(int64_t x) { return 0; };
	virtual int set_audio_position(int64_t x) { return 0; };

// Subclass should call this to add the base class allocation.
// Only used in read mode.
	virtual int64_t get_memory_usage();
// delete the oldest frame & return the bytes freed
    virtual int64_t purge_cache();

	virtual int write_samples(double **buffer, 
		int64_t len) { return 0; };
	virtual int write_frames(VFrame ***frames, int len) { return 0; };
	virtual int read_compressed_frame(VFrame *buffer) { return 0; };
	virtual int write_compressed_frame(VFrame *buffers) { return 0; };
	virtual int64_t compressed_frame_size() { return 0; };
// Doubles are used to allow resampling
	virtual int read_samples(double *buffer, int64_t len) { return 0; };

	virtual int read_frame(VFrame *frame) { return 1; };

// Return either the argument or another colormodel which read_frame should
// use.
	virtual int colormodel_supported(int colormodel) { return BC_RGB888; };
// This file can copy compressed frames directly from the asset
	virtual int can_copy_from(Asset *asset, int64_t position) { return 0; }; 
	virtual int get_render_strategy(ArrayList<int>* render_strategies) { return VRENDER_VPIXEL; };

// Manages an audio history buffer
	void update_pcm_history(int64_t len);
// Returns history_start + history_size
	int64_t get_history_sample();
// contiguous float
	void append_history(float **new_data, int len);
// Interleaved short
	void append_history(short *new_data, int len);
	void read_history(double *dst,
		int64_t start_sample, 
		int channel,
		int64_t len);
	void allocate_history(int len);

// For static functions to access it
	Asset *asset;

// Table data for file handler.
// These fields are only defined by the default constructor.
    ArrayList<int> ids;
// Supports for format for writing
    int has_audio;
    int has_video;
    int has_wrapper;
// supports writing or reading
    int has_wr;
    int has_rd;

protected:
// convert samples into file format
	int64_t samples_to_raw(uint8_t *out_buffer, 
							double **in_buffer,
							int input_len, 
							int bits, 
							int channels,
							int byte_order,
							int signed_);

// overwrites the buffer from PCM data depending on feather.
	int raw_to_samples(float *out_buffer, char *in_buffer, 
		int64_t samples, int bits, int channels, int channel, int feather, 
		float lfeather_len, float lfeather_gain, float lfeather_slope);

// Overwrite the buffer from float data using feather.
	int overlay_float_buffer(float *out_buffer, float *in_buffer, 
		int64_t samples, 
		float lfeather_len, float lfeather_gain, float lfeather_slope);

// convert a frame to and from file format

	int64_t frame_to_raw(unsigned char *out_buffer,
					VFrame *in_frame,
					int w,
					int h,
					int use_alpha,
					int use_float,
					int color_model);

// allocate a buffer for translating int to float
	int get_audio_buffer(char **buffer, int64_t len, int64_t bits, int64_t channels); // audio

// Allocate a buffer for feathering floats
	int get_float_buffer(float **buffer, int64_t len);

// allocate a buffer for translating video to VFrame
	int get_video_buffer(unsigned char **buffer, int depth); // video
	int get_row_pointers(unsigned char *buffer, unsigned char ***pointers, int depth);
	static int match4(const char *in, const char *out);   // match 4 bytes for a quicktime type

	int64_t ima4_samples_to_bytes(int64_t samples, int channels);
	int64_t ima4_bytes_to_samples(int64_t bytes, int channels);

	float *float_buffer;          // for floating point feathering
	unsigned char **row_pointers_in, **row_pointers_out;
	int64_t prev_buffer_position;  // for audio determines if reading raw data is necessary
	int64_t prev_frame_position;   // for video determines if reading raw video data is necessary
	int64_t prev_bytes; // determines if new raw buffer is needed and used for getting memory usage
	int64_t prev_len;
	int prev_track;
	int prev_layer;
	int dither;
	int internal_byte_order;
	File *file;

// ================================= Audio compression
	double **pcm_history;
	int64_t history_allocated;
	int64_t history_size;
	int64_t history_start;
	int history_channels;
// Range to decode to fill history buffer.  Maintained by FileBase.
	int64_t decode_start;
	int64_t decode_len;
// End of last decoded sample.  Maintained by user for seeking.
	int64_t decode_end;


private:

	int reset_parameters();



// ULAW
	float ulawtofloat(char ulaw);
	char floattoulaw(float value);
	int generate_ulaw_tables();
	int delete_ulaw_tables();
	float *ulawtofloat_table, *ulawtofloat_ptr;
	unsigned char *floattoulaw_table, *floattoulaw_ptr;

// IMA4
	int init_ima4();
	int delete_ima4();
	int ima4_decode_block(int16_t *output, unsigned char *input);
	int ima4_decode_sample(int *predictor, int nibble, int *index, int *step);
	int ima4_encode_block(unsigned char *output, int16_t *input, int step, int channel);
	int ima4_encode_sample(int *last_sample, int *last_index, int *nibble, int next_sample);

	static int ima4_step[89];
	static int ima4_index[16];
	int *last_ima4_samples;
	int *last_ima4_indexes;
	int ima4_block_size;
	int ima4_block_samples;
	OverlayFrame *overlayer;
};

#endif
