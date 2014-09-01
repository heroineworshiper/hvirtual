#ifndef FILEMOV_H
#define FILEMOV_H

#include "file.inc"
#include "filebase.h"
#include "quicktime.h"

class FileMOV : public FileBase
{
public:
	FileMOV(Asset *asset, File *file);
	~FileMOV();

	int open_file();
	int reset();
	int close_file();
	int read_header();
	long get_audio_length();
	int get_position(double *percentage, double *seconds);
	long get_audio_position();
	int end_of_audio();
	int end_of_video();
	int set_position(double percentage);
	int set_audio_position(long x);
	int drop_frames(int frames);
	int frame_back();
	int colormodel_supported(int color_model);

	int read_audio(char *buffer, long len);
	int read_frame(VFrame *frame, int in_y1, int in_y2);

private:
	quicktime_t *fd;
};





#endif
