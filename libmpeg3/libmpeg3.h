#ifndef LIBMPEG3_H
#define LIBMPEG3_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mpeg3private.h"


/* Supported color models for mpeg3_read_frame */
#define MPEG3_RGB565 2
#define MPEG3_BGR888 0
#define MPEG3_BGRA8888 1
#define MPEG3_RGB888 3
#define MPEG3_RGBA8888 4
#define MPEG3_RGBA16161616 5

/* Color models for the 601 to RGB conversion */
/* 601 not implemented for scalar code */
#define MPEG3_601_RGB565 11
#define MPEG3_601_BGR888 7
#define MPEG3_601_BGRA8888 8
#define MPEG3_601_RGB888 9
#define MPEG3_601_RGBA8888 10

/* Supported color models for mpeg3_read_yuvframe */ 
#define MPEG3_YUV420P 12
#define MPEG3_YUV422P 13


/* Error codes for the error_return variable */
#define MPEG3_UNDEFINED_ERROR 1
#define MPEG3_INVALID_TOC_VERSION 2
#define MPEG3_TOC_DATE_MISMATCH 3

/* Get version information */
int mpeg3_major();
int mpeg3_minor();
int mpeg3_release();


/* Check for file compatibility.  Return 1 if compatible. */
int mpeg3_check_sig(char *path);

/* Open the MPEG stream. */
/* An error code is put into *error_return if it fails and error_return is nonzero. */
mpeg3_t* mpeg3_open(char *path, int *error_return);

/* Open the MPEG stream and copy the tables from an already open stream. */
/* Eliminates some initial scanning and is used for opening audio streams. */
/* An error code is put into *error_return if it fails and error_return is nonzero. */
mpeg3_t* mpeg3_open_copy(char *path, mpeg3_t *old_file, int *error_return);
int mpeg3_close(mpeg3_t *file);




/* Performance */
int mpeg3_set_cpus(mpeg3_t *file, int cpus);

/* Query the MPEG3 stream about audio. */
int mpeg3_has_audio(mpeg3_t *file);
int mpeg3_total_astreams(mpeg3_t *file);             /* Number of multiplexed audio streams */
int mpeg3_audio_channels(mpeg3_t *file, int stream);
int mpeg3_sample_rate(mpeg3_t *file, int stream);
char* mpeg3_audio_format(mpeg3_t *file, int stream);

/* Total length obtained from the timecode. */
/* For DVD files, this is unreliable. */
long mpeg3_audio_samples(mpeg3_t *file, int stream); 
int mpeg3_set_sample(mpeg3_t *file, long sample, int stream);    /* Seek to a sample */
long mpeg3_get_sample(mpeg3_t *file, int stream);    /* Tell current position */

/* Read a PCM buffer of audio from 1 channel and advance the position. */
/* Return a 1 if error. */
/* Stream defines the number of the multiplexed stream to read. */
/* If both output arguments are null the audio is not rendered. */
int mpeg3_read_audio(mpeg3_t *file, 
		float *output_f,      /* Pointer to pre-allocated buffer of floats */
		short *output_i,      /* Pointer to pre-allocated buffer of int16's */
		int channel,          /* Channel to decode */
		long samples,         /* Number of samples to decode */
		int stream);          /* Stream containing the channel */

/* Reread the last PCM buffer from a different channel and advance the position */
int mpeg3_reread_audio(mpeg3_t *file, 
		float *output_f,      /* Pointer to pre-allocated buffer of floats */
		short *output_i,      /* Pointer to pre-allocated buffer of int16's */
		int channel,          /* Channel to decode */
		long samples,         /* Number of samples to decode */
		int stream);          /* Stream containing the channel */

/* Read the next compressed audio chunk.  Store the size in size and return a  */
/* 1 if error. */
/* Stream defines the number of the multiplexed stream to read. */
int mpeg3_read_audio_chunk(mpeg3_t *file, 
		unsigned char *output, 
		long *size, 
		long max_size,
		int stream);

/* Query the stream about video. */
int mpeg3_has_video(mpeg3_t *file);
int mpeg3_total_vstreams(mpeg3_t *file);            /* Number of multiplexed video streams */
int mpeg3_video_width(mpeg3_t *file, int stream);
int mpeg3_video_height(mpeg3_t *file, int stream);
float mpeg3_aspect_ratio(mpeg3_t *file, int stream); /* aspect ratio.  0 if none */
double mpeg3_frame_rate(mpeg3_t *file, int stream);  /* Frames/sec */


/* Total length.   */
/* This is meaningless except for TOC files. */
long mpeg3_video_frames(mpeg3_t *file, int stream);
int mpeg3_set_frame(mpeg3_t *file, long frame, int stream); /* Seek to a frame */
int mpeg3_skip_frames();
long mpeg3_get_frame(mpeg3_t *file, int stream);            /* Tell current position */

/* Total bytes.  Used for absolute byte seeking. */
int64_t mpeg3_get_bytes(mpeg3_t *file);

/* Seek all the tracks to the absolute byte in the  */
/* file.  This eliminates the need for tocs but doesn't  */
/* give frame accuracy. */
int mpeg3_seek_byte(mpeg3_t *file, int64_t byte);
int64_t mpeg3_tell_byte(mpeg3_t *file);


/* To synchronize audio and video in percentage seeking mode, these must */
/* be called after percentage seeking the video file and before */
/* percentage seeking the audio file.  Then when the audio file is percentage */
/* seeked it will search for the nearest pts to file->percentage_pts. */
/*
 * double mpeg3_get_percentage_pts(mpeg3_t *file);
 * void mpeg3_set_percentage_pts(mpeg3_t *file, double pts);
 */

int mpeg3_previous_frame(mpeg3_t *file, int stream);
int mpeg3_end_of_audio(mpeg3_t *file, int stream);
int mpeg3_end_of_video(mpeg3_t *file, int stream);

/* Give the seconds time in the last packet read */
double mpeg3_get_time(mpeg3_t *file);

/* Read a frame.  The dimensions of the input area and output frame must be supplied. */
/* The frame is taken from the input area and scaled to fit the output frame in 1 step. */
/* Stream defines the number of the multiplexed stream to read. */
/* The last row of **output_rows must contain 4 extra bytes for scratch work. */
int mpeg3_read_frame(mpeg3_t *file, 
		unsigned char **output_rows, /* Array of pointers to the start of each output row */
		int in_x,                    /* Location in input frame to take picture */
		int in_y, 
		int in_w, 
		int in_h, 
		int out_w,                   /* Dimensions of output_rows */
		int out_h, 
		int color_model,             /* One of the color model #defines */
		int stream);

/* Get the colormodel being used natively by the stream */
int mpeg3_colormodel(mpeg3_t *file, int stream);
/* Set the row stride to be used in mpeg3_read_yuvframe */
int mpeg3_set_rowspan(mpeg3_t *file, int bytes, int stream);

/* Read a frame in the native color model used by the stream.  */
/* The Y, U, and V planes are copied into the y, u, and v */
/* buffers provided. */
/* The input is cropped to the dimensions given but not scaled. */
int mpeg3_read_yuvframe(mpeg3_t *file,
		char *y_output,
		char *u_output,
		char *v_output,
		int in_x,
		int in_y,
		int in_w,
		int in_h,
		int stream);

/* Read a frame in the native color model used by the stream.  */
/* The Y, U, and V planes are not copied but the _output pointers */
/* are redirected to the frame buffer. */
int mpeg3_read_yuvframe_ptr(mpeg3_t *file,
		char **y_output,
		char **u_output,
		char **v_output,
		int stream);

/* Drop frames number of frames */
int mpeg3_drop_frames(mpeg3_t *file, long frames, int stream);

/* Read the next compressed frame including headers. */
/* Store the size in size and return a 1 if error. */
/* Stream defines the number of the multiplexed stream to read. */
int mpeg3_read_video_chunk(mpeg3_t *file, 
		unsigned char *output, 
		long *size, 
		long max_size,
		int stream);

/* Master control */
int mpeg3_total_programs();
int mpeg3_set_program(int program);

/* Memory used by video caches. */
int64_t mpeg3_memory_usage(mpeg3_t *file);







/* subtitle functions */
/* get number of subtitle tracks */
int mpeg3_subtitle_tracks(mpeg3_t *file);
/* Enable overlay of a subtitle track. */
/* track - the number of the subtitle track starting from 0 */
/* The same subtitle track is overlayed for all video tracks. */
/* Pass -1 to disable subtitles. */
void mpeg3_show_subtitle(mpeg3_t *file, int track);








/* Table of contents generation */
/* Begin constructing table of contents */
mpeg3_t* mpeg3_start_toc(char *path, char *toc_path, int64_t *total_bytes);
/* Set the maximum number of bytes per index track */
void mpeg3_set_index_bytes(mpeg3_t *file, int64_t bytes);
/* Process one packet */
int mpeg3_do_toc(mpeg3_t *file, int64_t *bytes_processed);
/* Write table of contents */
void mpeg3_stop_toc(mpeg3_t *file);

/* Get modification date of source file from table of contents. */
/* Used to compare DVD source file to table of contents source. */
int64_t mpeg3_get_source_date(mpeg3_t *file);
/* Get modification date of source file from source file. */
int64_t mpeg3_calculate_source_date(char *path);






/* Table of contents queries */
/* Return number of tracks in the table of contents */
int mpeg3_index_tracks(mpeg3_t *file);
/* Return number of channels in track */
int mpeg3_index_channels(mpeg3_t *file, int track);
/* Return zoom factor of index */
int mpeg3_index_zoom(mpeg3_t *file);
/* Number of high/low pairs in a channel of the track */
int mpeg3_index_size(mpeg3_t *file, int track);
/* Get data for one index channel */
float* mpeg3_index_data(mpeg3_t *file, int track, int channel);
/* Returns 1 if the file has a table of contents */
int mpeg3_has_toc(mpeg3_t *file);
/* Return the path of the title number or 0 if no more titles. */
char* mpeg3_title_path(mpeg3_t *file, int number);


#ifdef __cplusplus
}
#endif

#endif
