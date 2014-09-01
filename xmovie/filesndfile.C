#include "asset.h"
#include "file.h"
#include "filesndfile.h"

#include <string.h>

FileSndFile::FileSndFile(Asset *asset, File *file)
 : FileBase(asset, file)
{
}

FileSndFile::~FileSndFile()
{
}

int FileSndFile::check_sig(Asset *asset)
{
	int result = 0;
	SF_INFO fd_config;
	SNDFILE *fd;
	
	bzero(&fd_config, sizeof(SF_INFO));
	fd = sf_open(asset->path, SFM_READ, &fd_config);

	if(fd)
	{
		sf_close(fd);
		result = 1;
	}
	else
		result = 0;
	return result;
}

void FileSndFile::format_to_asset()
{
	asset->format = FILE_SNDFILE;
	asset->audio_data = 1;
	asset->channels = fd_config.channels;
	asset->rate = fd_config.samplerate;
	asset->audio_streams = 1;
	asset->video_streams = 0;
	asset->bits = 16;
}

int FileSndFile::open_file()
{
//printf("FileSndFile::open_file 1\n");
	bzero(&fd_config, sizeof(SF_INFO));
//printf("FileSndFile::open_file 1\n");
	if(!(fd = sf_open(asset->path, SFM_READ, &fd_config)))
	{
		printf("FileSndFile::open_file\n");
		return 1;
	}
//printf("FileSndFile::open_file 1\n");

	format_to_asset();
//printf("FileSndFile::open_file 2\n");

	return 0;
}

int FileSndFile::close_file()
{
	FileBase::close_file();
	if(fd) sf_close(fd);
	return 0;
}

int FileSndFile::set_audio_position(long sample)
{
	sf_seek(fd, sample, SEEK_SET);
	return 0;
}

long FileSndFile::get_audio_length()
{
	return fd_config.frames;
}

int FileSndFile::end_of_audio()
{
	return file->audio_position >= fd_config.frames;
}

int FileSndFile::read_audio(char *buffer, long len)
{
	bzero(buffer, len * asset->channels * sizeof(int16_t));
	sf_read_short(fd, (short*)buffer, len * asset->channels);
	return 0;
}
