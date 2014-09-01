#ifndef FILE_H
#define FILE_H

#include <stdio.h>


#include "asset.inc"
#include "file.inc"
#include "filebase.inc"
#include "mwindow.inc"
#include "sema.h"
#include "vframe.inc"


// ======================================= include file types here

// generic file opened by user
class File
{
public:
	File(MWindow *mwindow);
	~File();

	void reset();

// ===================================== start here
	int set_processors(int cpus);
	int set_mmx(int use_mmx);
	int set_prebuffer(long size);

// return 0 if success/found format
// return 1 if failure/file not found
// return 2 if need format
// return 3 if project
// format may be preset by set_format
	int open_file(Asset *asset);

// close file
	int close_file();

// get length of file
	long get_audio_length();

// get positions
	int get_position(double *percentage, double *seconds);
	long get_audio_position();
// Return 1 if position is at the end
	int end_of_audio();
	int end_of_video();

// set positions
	int set_audio_position(long x);
// set position in percentage
	int set_position(double percentage);
// Synchronize position of this file with another file.
// Used for synchronizing MPEG audio and video while using percentage seeking.
	void synchronize_position(File *src);
// Skip a certain number of frames forward.
	int drop_frames(int frames);
// Step back a frame
	int frame_back();

// Set layer
	int set_audio_stream(int stream);
	int set_video_stream(int stream);

// read the number of samples of raw data for all audio channels
// advances file pointer
// return 1 if failed
	int read_audio(char *buffer, long len);

// Test if hardware accelerated colormodels are supported.
// All software colormodels must be supported
	int colormodel_supported(int color_model);

	int read_frame(VFrame *frame, int in_y1, int in_y2);

	int lock_read();
	int unlock_read();

	Asset *asset;
	FileBase *file; // virtual class for file type
	MWindow *mwindow;
//	Sema read_lock, write_lock;
	int cpus;
	int use_mmx;
	long prebuffer_size;
	int video_stream;
	int audio_stream;
	long audio_position;
};

#endif
