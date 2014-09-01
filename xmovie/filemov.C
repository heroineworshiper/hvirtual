#include "asset.h"
#include "colormodels.h"
#include "file.h"
#include "filemov.h"
#include "mwindow.h"
#include "vframe.h"
#include <string.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))

FileMOV::FileMOV(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset();
	asset->format = FILE_MOV;
	asset->byte_order = 0;
}

FileMOV::~FileMOV()
{
	close_file();
}

int FileMOV::reset()
{
	FileBase::reset();
	fd = 0;
	return 0;
}

int FileMOV::open_file()
{
	if(!(fd = quicktime_open(asset->path, 1, 0)))
	{
		perror("FileMOV::open_file");
		return 1;
	}

	quicktime_set_preload(fd, FileBase::file->prebuffer_size);
	quicktime_set_cpus(fd, FileBase::file->cpus);
	read_header();
	
	if(!asset->video_data && !asset->audio_data)
	{
		quicktime_close(fd);
		return 3;
	}
	return 0;
}

int FileMOV::close_file()
{
	FileBase::close_file();
	if(fd) quicktime_close(fd);
	fd = 0;
	return 0;
}

int FileMOV::read_header()
{
// determine if the audio can be read before declaring audio data
	if(quicktime_has_audio(fd))
		if(quicktime_supported_audio(fd, 0))
				asset->audio_data = 1;
			else
				printf("FileMOV::read_header: unsupported audio codec\n");


	if(asset->audio_data)
	{
		asset->audio_streams = 1;
		file->audio_stream = 0;
		asset->channels = 0;
		asset->channels = quicktime_track_channels(fd, 0);
		asset->rate = quicktime_sample_rate(fd, 0);
		asset->bits = quicktime_audio_bits(fd, 0);
		char *compressor = quicktime_audio_compressor(fd, 0);

		if(quicktime_supported_audio(fd, 0))
		{
			asset->bits = 16;
			asset->signed_ = 1;
			asset->byte_order = internal_byte_order;
		}
		else
		if(match4(compressor, "raw ")) asset->signed_ = 0;
		else
		if(match4(compressor, "twos")) asset->signed_ = 1;
		else
		printf("FileMOV::read_header: unsupported audio codec\n");
	}

// determine if the video can be read before declaring video data
	if(quicktime_has_video(fd))
		if(quicktime_supported_video(fd, 0))
				asset->video_data = 1;
			else
				printf("FileMOV::read_header: unsupported video codec\n");

	if(asset->video_data)
	{
		asset->video_streams = 1;
		file->video_stream = 0;
		asset->width = quicktime_video_width(fd, 0);
		asset->height = quicktime_video_height(fd, 0);
		asset->frame_rate = quicktime_frame_rate(fd, 0);
		
		char *compressor = quicktime_video_compressor(fd, 0);
		asset->compression[0] = compressor[0];
		asset->compression[1] = compressor[1];
		asset->compression[2] = compressor[2];
		asset->compression[3] = compressor[3];
	}
	return 0;
}

long FileMOV::get_audio_length()
{
	long result = quicktime_audio_length(fd, 0);
	return result;
}

int FileMOV::get_position(double *percentage, double *seconds)
{
	float apercentage = 0, aseconds = 0;
	float vpercentage = 0, vseconds = 0;

	if(fd)
	{
		if(asset->audio_data)
		{
			apercentage = (double)get_audio_position() / 
				get_audio_length();
			aseconds = (double)get_audio_position() / 
				asset->rate;
		}

		if(asset->video_data)
		{
			vpercentage = (double)quicktime_video_position(fd, 0) / 
				quicktime_video_length(fd, 0);
			vseconds = (double)quicktime_video_position(fd, 0) / 
				asset->frame_rate;
		}

		*percentage = MAX(apercentage, vpercentage);
		*seconds = MAX(aseconds, vseconds);
	}

//printf("FileMOV::get_position %f\n", seconds);
	return 0;
}

long FileMOV::get_audio_position()
{
	return quicktime_audio_position(fd, 0);
}

int FileMOV::end_of_audio()
{
	return quicktime_audio_position(fd, 0) >= quicktime_audio_length(fd, file->audio_stream);
}


int FileMOV::end_of_video()
{
	return quicktime_video_position(fd, 0) >= quicktime_video_length(fd, file->video_stream);
}

int FileMOV::set_position(double percentage)
{
	int result = 0;

// Make audio synchronize to video
	if(asset->video_data)
	{
		file->set_audio_position((long)(percentage * 
			quicktime_video_length(fd, file->video_stream) / 
			asset->frame_rate *
			asset->rate));
		result |= quicktime_set_video_position(fd, 
			(long)(percentage * quicktime_video_length(fd, file->video_stream)),
			file->video_stream);
	}
	else
	{
		file->set_audio_position((long)(percentage * get_audio_length()));
	}
	return result;
}

int FileMOV::set_audio_position(long x)
{
	return quicktime_set_audio_position(fd, x, file->audio_stream);
}

int FileMOV::drop_frames(int frames)
{
	return quicktime_set_video_position(fd, 
		quicktime_video_position(fd, file->video_stream) + frames, 
		file->video_stream);
}

int FileMOV::frame_back()
{
	if(quicktime_video_position(fd, file->video_stream) > 1)
		quicktime_set_video_position(fd, 
			quicktime_video_position(fd, file->video_stream) - 2, 
			file->video_stream);
	else
		quicktime_set_video_position(fd, 
			0, 
			file->video_stream);
	quicktime_set_audio_position(fd, 
		file->audio_position - asset->rate, 
		0);
	return  0;
}


int FileMOV::colormodel_supported(int color_model)
{
	return quicktime_reads_cmodel(fd, 
		color_model, 
		file->video_stream);
}


int FileMOV::read_frame(VFrame *frame, int in_y1, int in_y2)
{
	int result = 0;


	if(!end_of_video())
	{
		quicktime_set_parameter(fd, "divx_use_deblocking", &mwindow->use_deblocking);
		switch(frame->get_color_model())
		{
			case BC_YUV420P:
			case BC_YUV422P:
			{
				unsigned char *temp_rows[3];
				temp_rows[0] = frame->get_y();
				temp_rows[1] = frame->get_u();
				temp_rows[2] = frame->get_v();
				quicktime_set_cmodel(fd, frame->get_color_model());
				quicktime_set_window(fd,
					0,                    /* Location of input frame to take picture */
					in_y1,
					asset->width,
					in_y2 - in_y1,
					frame->get_w(),                   /* Dimensions of output frame */
					frame->get_h());
				result = quicktime_decode_video(fd, 
					temp_rows, 
					file->video_stream);
				break;
			}

			default:
				quicktime_set_cmodel(fd, frame->get_color_model());
				quicktime_set_window(fd,
					0,                    /* Location of input frame to take picture */
					in_y1,
					asset->width,
					in_y2 - in_y1,
					frame->get_w(),                   /* Dimensions of output frame */
					frame->get_h());
				result = quicktime_decode_video(fd, 
					frame->get_rows(), 
					file->video_stream);
				break;
		}
	}

	return result;
}

int FileMOV::read_audio(char *buffer, long len)
{
	if(quicktime_supported_audio(fd, 0))
	{
		int channel;
		int i, j;
		int result = 0;

		get_read_buffer(len);

		for(channel = 0; channel < asset->channels && !result; channel++)
		{
//printf("FileMOV::read_audio %d %d\n", channel, file->audio_position);
			bzero(read_buffer, sizeof(int16_t) * len);

			quicktime_set_audio_position(fd, file->audio_position, file->audio_stream);
			result = quicktime_decode_audio(fd, read_buffer, 0, len, channel);

			int16_t *output_ptr = (int16_t*)buffer + channel;
			int16_t *input_ptr = (int16_t*)read_buffer;
			int16_t *input_end = input_ptr + len;

// Interlace the audio for the sound driver.
			while(input_ptr < input_end)
			{
				*output_ptr = *input_ptr++;
				output_ptr += asset->channels;
			}
		}
	}

	return 0;
}
