#ifdef USE_AVI
#ifndef FILEAVI_H
#define FILEAVI_H

#include "file.inc"
#include "filebase.h"

class IAviWriteFile;
class IAviReadFile;
class IAviReadStream;
#define MAX_STREAMS 256

class FileAVI : public FileBase
{
public:
	FileAVI(Asset *asset, File *file);
	~FileAVI();

	static int check_sig(Asset *asset);
	int open_file();
	int close_file();
	int reset();
	int read_header();
	long get_audio_length();
	int get_position(double *percentage, double *seconds);
	int end_of_audio();
	int end_of_video();
	int set_position(double percentage);
	int set_audio_position(long x);
	int set_video_stream(int stream);
	int set_audio_stream(int stream);
	int drop_frames(int frames);
	int frame_back();

	int colormodel_supported(int color_model);
	int read_frame(VFrame *frame, int in_y1, int in_y2);

	int read_audio(char *buffer, long len);
	int load_into_ram();

private:
	IAviReadFile *in_fd;
	IAviReadStream *astream[MAX_STREAMS];
	IAviReadStream *vstream[MAX_STREAMS];
	unsigned char *temp_audio;
// Units are 
	long temp_size;
	long temp_allocated;
	long temp_position;
	int source_cmodel;
};





#endif
#endif
