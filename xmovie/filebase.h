#ifndef FILEBASE_H
#define FILEBASE_H

#include "asset.inc"
#include "file.inc"
#include "mwindow.inc"
#include "sizes.h"
#include "vframe.inc"

// inherited by every file interpreter
class FileBase
{
public:
	FileBase(Asset *asset, File *file);
	virtual ~FileBase();

	virtual int close_file();
	virtual int reset();

	virtual int reset_parameters_derived() { return 0; };
	virtual int open_file() { return 0; };
	virtual int close_file_derived() { return 0; };
	virtual long get_audio_length() { return 0; };
	virtual int get_position(double *percentage, double *seconds) { return 0; };
	virtual int set_position(double percentage) { return 1; };
	virtual int end_of_audio() { return 0; };
	virtual int end_of_video() { return 0; };
	virtual int set_audio_position(long x) { return 0; };
	virtual int set_video_stream(int stream) { return 0; };
	virtual int set_audio_stream(int stream) { return 0; };
	virtual int set_cpus(int cpus) { return 0; };
	virtual int set_mmx(int use_mmx) { return 0; };
	virtual int colormodel_supported(int color_model) { return 0; };
	virtual void synchronize_position(File *src) { };

// Skip a certain number of frames forward.
	virtual int drop_frames(int frames) { return 1; };
// Step back a frame
	virtual int frame_back() { return 1; };

// Read video
	virtual int read_frame(VFrame *frame, int in_y1, int in_y2) { return 0; };

// Read audio
	virtual int read_audio(char *buffer, long len) { return 0; };
	
	File *file;
	MWindow *mwindow;

protected:
	int get_read_buffer(long len);

	int internal_byte_order;
	int match4(char *in, char *out);
	Asset *asset;
	int16_t *read_buffer; // Temporary storage for audio output
	long read_size;   // Sample size of temporary storage
};

#endif
