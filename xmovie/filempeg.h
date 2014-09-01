#ifndef FILEMPEG_H
#define FILEMPEG_H

#include "file.inc"
#include "filebase.h"
#include "libmpeg3.h"

class FileMPEG : public FileBase
{
public:
	FileMPEG(Asset *asset, File *file);
	~FileMPEG();

	int open_file();
	int close_file();
	int reset();
	int read_header();
	long get_audio_length();
	int get_position(double *percentage, double *seconds);
	long get_video_position();
	long get_audio_position();
	int end_of_audio();
	int end_of_video();
	int set_position(double percentage);
	int set_audio_position(long x);
	int set_audio_stream(int stream);
	int drop_frames(int frames);
	int frame_back();
	int set_cpus(int cpus);
	int set_mmx(int use_mmx);
	int colormodel_supported(int color_model);
	int read_frame(VFrame *frame, int in_y1, int in_y2);
	void synchronize_position(File *src);


	int read_audio(char *buffer, long len);

private:
	int translate_color_model(int color_model);
	mpeg3_t *fd;
// Temporary for 420 - 422 conversion
	VFrame *temp_frame;
};





#endif
