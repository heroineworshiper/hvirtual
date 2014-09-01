#ifndef FILESNDFILE_H
#define FILESNDFILE_H

#include "filebase.h"
#include "sndfile.h"

class FileSndFile : public FileBase
{
public:
	FileSndFile(Asset *asset, File *file);
	~FileSndFile();

	static int check_sig(Asset *asset);
	int open_file();
	int close_file();
	int set_audio_position(long sample);
	int read_audio(char *buffer, long len);
	void format_to_asset();
	long get_audio_length();
	int end_of_audio();

	SNDFILE *fd;
	SF_INFO fd_config;
// Temp for interleaved channels
};

#endif
