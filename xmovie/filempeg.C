#include "asset.h"
#include "colormodels.h"
#include "file.h"
#include "filempeg.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vframe.h"
#include "workarounds.h"

#include <string.h>

FileMPEG::FileMPEG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset();
	asset->format = FILE_MPEG;
	asset->byte_order = 0;
	temp_frame = 0;
}

FileMPEG::~FileMPEG()
{
	close_file();
	if(temp_frame) delete temp_frame;
}

int FileMPEG::reset()
{
	FileBase::reset();
	fd = 0;
	return 0;
}

int FileMPEG::open_file()
{
// Copy tables if a file is already opened by MWindow
	if(mwindow->audio_file && mwindow->video_file)
	{
		if(!(fd = mpeg3_open_copy(asset->path, 
			((FileMPEG*)mwindow->audio_file->file)->fd)))
		{
			perror("FileMPEG::open_file");
			return 1;
		}
	}
	else
	if(!(fd = mpeg3_open(asset->path)))
	{
		perror("FileMPEG::open_file");
		return 1;
	}

	mpeg3_set_cpus(fd, FileBase::file->cpus);
	read_header();
	mpeg3_show_subtitle(fd, 1);

	if(!asset->video_data && !asset->audio_data)
	{
		mpeg3_close(fd);
		return 3;
	}
	return 0;
}

int FileMPEG::close_file()
{
	FileBase::close_file();
	if(fd) mpeg3_close(fd);
	reset();
	return 0;
}

int FileMPEG::set_cpus(int cpus)
{
	if(fd) mpeg3_set_cpus(fd, cpus);
	return 0;
}

int FileMPEG::set_mmx(int use_mmx)
{
//	if(fd) mpeg3_set_mmx(fd, use_mmx);
	return 0;
}


int FileMPEG::read_header()
{
	if(mpeg3_has_audio(fd)) asset->audio_data = 1;
//printf("FileMPEG::read_header %d %d\n",file->audio_stream , file->video_stream);

	if(asset->audio_data)
	{
		asset->audio_streams = mpeg3_total_astreams(fd);
		if(file->audio_stream >= asset->audio_streams) file->audio_stream = 0;
		asset->channels = mpeg3_audio_channels(fd, file->audio_stream);
		asset->rate = mpeg3_sample_rate(fd, file->audio_stream);
		asset->bits = 16;
		asset->signed_ = 1;
		asset->byte_order = internal_byte_order;
	}

	if(mpeg3_has_video(fd)) asset->video_data = 1;

	if(asset->video_data)
	{
		asset->video_streams = mpeg3_total_vstreams(fd);
		if(file->video_stream >= asset->video_streams) file->video_stream = 0;
		asset->width = mpeg3_video_width(fd, file->video_stream);
		asset->height = mpeg3_video_height(fd, file->video_stream);
		asset->frame_rate = mpeg3_frame_rate(fd, file->video_stream);
		asset->compression[0] = 0;
	}
//	if(asset->channels > 2) asset->channels = 2;
	return 0;
}


long FileMPEG::get_audio_length()
{
	long result = mpeg3_audio_samples(fd, file->audio_stream);
	return result;
}

int FileMPEG::get_position(double *percentage, double *seconds)
{
	if(fd)
	{
		if(asset->video_data)
		{
			*percentage = (double)mpeg3_tell_byte(fd) /
					(double)mpeg3_get_bytes(fd);
// printf("FileMPEG::get_position 1 %lld %lld %f\n", 
// mpeg3_tell_byte(fd),
// mpeg3_get_bytes(fd),
// *percentage);
			*seconds = mpeg3_get_time(fd);
		}
// Audio only
		else
		if(asset->audio_data)
		{
			*percentage =
				(double)mpeg3_get_sample(fd, 0) / 
					(double)mpeg3_audio_samples(fd, 0);
			*seconds = 
				(double)mpeg3_get_sample(fd, 0) / 
					(double)asset->rate;
		}
	}
	else
	{
		*percentage = *seconds = 0;
	}

	return 0;
}

long FileMPEG::get_audio_position()
{
	return mpeg3_get_sample(fd, file->audio_stream);
}

int FileMPEG::end_of_audio()
{
	return mpeg3_end_of_audio(fd, file->audio_stream);
}

int FileMPEG::end_of_video()
{
	return mpeg3_end_of_video(fd, file->video_stream);
}

int FileMPEG::set_position(double percentage)
{
	int result = 0;

	if(fd)
	{
		if(asset->video_data)
		{
			result = mpeg3_seek_byte(fd, (int64_t)(percentage * 
				mpeg3_get_bytes(fd)));
 		}
		else
 		if(asset->audio_data)
 		{
 			result = mpeg3_set_sample(fd, 
				(long)(percentage * mpeg3_audio_samples(fd, file->audio_stream)), 
				file->audio_stream);
 		}
	}

	return result;
}

void FileMPEG::synchronize_position(File *src)
{
	FileMPEG *src_file = (FileMPEG*)src->file;
	mpeg3_t *src_fd = src_file->fd;


// Assume src has already seeked.
// This doesn't work.  Needs some more referring to the spec.
//printf("FileMPEG::synchronize_position %p %f\n", src_fd, mpeg3_get_percentage_pts(src_fd));
//	mpeg3_set_percentage_pts(fd, mpeg3_get_percentage_pts(src_fd));
}

int FileMPEG::set_audio_position(long x)
{
	return mpeg3_set_sample(fd, x, file->audio_stream);
}

int FileMPEG::set_audio_stream(int stream)
{
	asset->channels = mpeg3_audio_channels(fd, file->audio_stream);
	return 0;
}

int FileMPEG::drop_frames(int frames)
{
	if(fd)
	{
		mpeg3_drop_frames(fd, frames, 0);
	}
	return 0;
}

int FileMPEG::frame_back()
{
	if(asset->video_data)
	{
		mpeg3_previous_frame(fd, 0);
		mpeg3_previous_frame(fd, 0);
	}
	else
		set_audio_position(file->audio_position - asset->rate);
	return 0;
}


// Optimization strategies
int FileMPEG::colormodel_supported(int color_model)
{
	switch(color_model)
	{
		case BC_YUV420P:
			return (mpeg3_colormodel(fd, 0) == MPEG3_YUV420P);
			break;

		case BC_YUV422:
			return (mpeg3_colormodel(fd, 0) == MPEG3_YUV422P);
			break;
	}
	return 0;
}

int FileMPEG::translate_color_model(int color_model)
{
	switch(color_model)
	{
		case BC_RGB565:
			return MPEG3_RGB565;
			break;
		case BC_BGR888:
			if(mwindow->convert_601) return MPEG3_601_BGR888;
			else return MPEG3_BGR888;
			break;
		case BC_BGR8888:
			if(mwindow->convert_601) return MPEG3_601_BGRA8888;
			else return MPEG3_BGRA8888;
			break;
	}
	return -1;
}

int FileMPEG::read_frame(VFrame *frame, int in_y1, int in_y2)
{
	switch(frame->get_color_model())
	{
		case BC_YUV420P:
			mpeg3_read_yuvframe(fd,
				(char*)frame->get_y(),
				(char*)frame->get_u(),
				(char*)frame->get_v(),
				0,
				in_y1,
				frame->get_w(),
				in_y2 - in_y1,
				file->video_stream);
			break;

		case BC_RGB565:
		case BC_BGR888:
		case BC_BGR8888:
		case BC_RGB888:
		case BC_RGBA8888:
			mpeg3_read_frame(fd, 
					frame->get_rows(), 
					0,
					in_y1,
					asset->width,
					in_y2 - in_y1,
					frame->get_w(), 
					frame->get_h(), 
					translate_color_model(frame->get_color_model()),
					file->video_stream);
			break;

		default:
		{
			int source_cmodel;
			switch(mpeg3_colormodel(fd, 0))
			{
				case MPEG3_YUV422P:
					source_cmodel = BC_YUV422P;
					break;
				case MPEG3_YUV420P:
					source_cmodel = BC_YUV420P;
					break;
			}
		
			if(!temp_frame) 
			{
				temp_frame = new VFrame(0, 
					asset->width, 
					asset->height, 
					source_cmodel);
			}

			mpeg3_read_yuvframe(fd,
				(char*)temp_frame->get_y(),
				(char*)temp_frame->get_u(),
				(char*)temp_frame->get_v(),
				0,
				0,
				frame->get_w(),
				asset->height,
				file->video_stream);
			cmodel_transfer(frame->get_rows(), 
				temp_frame->get_rows(),
				frame->get_y(),
				frame->get_u(),
				frame->get_v(),
				temp_frame->get_y(),
				temp_frame->get_u(),
				temp_frame->get_v(),
				0, 
				0, 
				asset->width, 
				asset->height,
				0, 
				0, 
				frame->get_w(), 
				frame->get_h(),
				source_cmodel, 
				frame->get_color_model(),
				0,
				temp_frame->get_w(),
				frame->get_w());
			break;
		}
	}

	return 0;
}

int FileMPEG::read_audio(char *buffer, long len)
{
	int channel;
	int i, j;
	int result = 0;
	int16_t *output_ptr;
	int16_t *output_end;
	int16_t *input_ptr;
	int16_t *input_end;

	get_read_buffer(len);
// Zero output channels
	output_ptr = (int16_t*)buffer + channel;
	input_ptr = (int16_t*)read_buffer;
	input_end = input_ptr + len;

	for(channel = 0; 
		channel < asset->channels && !result; 
		channel++)
	{
		bzero(read_buffer, sizeof(int16_t) * len);
	
		if(channel == 0)
			result = mpeg3_read_audio(fd, 
					0, 
					read_buffer, 
					channel, 
					len, 
					file->audio_stream);
		else
			result = mpeg3_reread_audio(fd, 
					0, 
					read_buffer, 
					channel, 
					len, 
					file->audio_stream);

// Interleave for output
		output_ptr = (int16_t*)buffer + channel;
		input_ptr = (int16_t*)read_buffer;
		input_end = input_ptr + len;

		while(input_ptr < input_end)
		{
			*output_ptr = *input_ptr++;
			output_ptr += asset->channels;
		}
	}
	return 0;
}
