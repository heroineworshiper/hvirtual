#include "asset.h"
#include "file.h"
#include "fileavi.h"
#include "filemov.h"
#include "filempeg.h"
#include "filesndfile.h"
#include "mwindow.h"

File::File(MWindow *mwindow)
{
	this->mwindow = mwindow;
	reset();
}

File::~File()
{
	close_file();
}

void File::reset()
{
	file = 0;
	asset = 0;
	cpus = 1;
	use_mmx = 1;
	audio_position = 0;
	video_stream = mwindow->audio_stream;
	audio_stream = mwindow->video_stream;
	prebuffer_size = 0;
}

int File::set_processors(int cpus)
{
	this->cpus = cpus;
	if(file) file->set_cpus(cpus);
	return 0;
}

int File::set_mmx(int use_mmx)
{
	this->use_mmx = use_mmx;
	if(file) file->set_mmx(use_mmx);
	return 0;
}

int File::set_prebuffer(long size)
{
	this->prebuffer_size = size;
	return 0;
}

int File::open_file(Asset *asset)
{
	this->asset = asset;
	this->mwindow = mwindow;

// get the format now
	FILE *stream;
	if(!(stream = fopen(asset->path, "rb")))
	{
// file not found
		return 1;
	}

//printf("File::open_file 1\n");
// ============================== Check formats
// add new formats here
#ifdef USE_AVI
	if(FileAVI::check_sig(asset))
	{
//printf("File::open_file 2\n");
		fclose(stream);
		file = new FileAVI(asset, this);
	}
	else
#endif
	if(FileSndFile::check_sig(asset))
	{
//printf("File::open_file 3\n");
		fclose(stream);
		file = new FileSndFile(asset, this);
	}
	else
	if(mpeg3_check_sig(asset->path))
	{
//printf("File::open_file 4\n");
		fclose(stream);
		file = new FileMPEG(asset, this);
	}
	else
	if(quicktime_check_sig(asset->path))
	{
//printf("File::open_file 5\n");
// MOV file
// should be last because quicktime lacks a magic number
		fclose(stream);
		file = new FileMOV(asset, this);
	}
	else
	{
		fclose(stream);
		return 2;
	}

//printf("File::open_file 7\n");
	int result = file->open_file();
	if(result)
	{
// failure
		delete file;
		file = 0;
	}
//printf("File::open_file 8 %p\n", file);
	return result;
}

int File::close_file()
{
	if(file) 
	{
		file->close_file();
		delete file;
	}

	file = 0;
	asset = 0;
	return 0;
}

long File::get_audio_length() 
{ 
	return file->get_audio_length(); 
}

long File::get_audio_position() 
{ 
	return audio_position; 
}

int File::get_position(double *percentage, double *seconds)
{
	return file->get_position(percentage, seconds);
}

int File::end_of_audio()
{
	return file->end_of_audio();
}

int File::end_of_video()
{
	return file->end_of_video();
}

int File::set_position(double percentage)
{
	return file->set_position(percentage);
}

void File::synchronize_position(File *src)
{
	return file->synchronize_position(src);
}

int File::set_audio_position(long x) 
{
	if(x >= 0 & x < get_audio_length())
	{
		this->audio_position = x;
		file->set_audio_position(x); 
	}
	return 0;
}

int File::set_audio_stream(int stream)
{
	if(stream >= 0 && stream < asset->audio_streams)
	{
		this->audio_stream = stream;
		return file->set_audio_stream(stream);
	}
	return -1;
}

int File::set_video_stream(int stream)
{
	if(stream >= 0 && stream < asset->video_streams)
	{
		this->video_stream = stream;
		return file->set_video_stream(stream);
	}
	return -1;
}

int File::drop_frames(int frames)
{
	file->drop_frames(frames);
	return 0;
}

int File::frame_back()
{
	file->frame_back();
	if(!asset->video_data)
	{
		audio_position -= asset->rate;
	}
	else
		audio_position = 0;

	return 0;
}


int File::lock_read()
{
//	read_lock.lock();
	return 0;
}

int File::unlock_read()
{
//	read_lock.unlock();
	return 0;
}

int File::read_audio(char *buffer, long len)
{
	int result = 0;
	if(file) result = file->read_audio(buffer, len);
	audio_position += len;
	return result;
}

int File::colormodel_supported(int color_model)
{
	if(file) return file->colormodel_supported(color_model);
	return 0;
}

int File::read_frame(VFrame *frame, int in_y1, int in_y2)
{
	int result = 1;
	if(file) result = file->read_frame(frame, in_y1, in_y2);
	return result;
}
