#ifdef USE_AVI
#include "asset.h"
#include "avifile.h"
#include "clip.h"
#include "colormodels.h"
#include "except.h"
#include "file.h"
#include "fileavi.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vframe.h"



FileAVI::FileAVI(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset();
	asset->format = FILE_AVI;
	asset->byte_order = 0;
}

FileAVI::~FileAVI()
{
	close_file();
}

int FileAVI::check_sig(Asset *asset)
{
	IAviReadFile *in_fd = 0;

	try
	{
		in_fd = CreateIAviReadFile(asset->path);
	}
	catch(FatalError& error)
	{
		if(in_fd) delete in_fd;
		return 0;
	}

	delete in_fd;
	return 1;
}



int FileAVI::open_file()
{
	try
	{
		in_fd = CreateIAviReadFile(asset->path);
	}
	catch(FatalError& error)
	{
		error.Print();
		close_file();
		return 1;
	}

	asset->video_streams = in_fd->VideoStreamCount();
//asset->video_streams = 0;
	if(asset->video_streams)
	{
		asset->video_data = 1;
		BITMAPINFOHEADER bitmapinfo;        // BITMAPINFOHEADER

		for(int i = 0; i < asset->video_streams && i < MAX_STREAMS; i++)
		{
			vstream[i] = in_fd->GetStream(i, IAviReadStream::Video);
			if(!vstream[i]->StartStreaming())
			{
// Avifile is not interactive about color models
// Get best color model
				IVideoDecoder::CAPS caps = vstream[i]->GetDecoder()->GetCapabilities();
				if(caps & IVideoDecoder::CAP_YUY2)
				{
					source_cmodel = BC_YUV422;
					vstream[i]->GetDecoder()->SetDestFmt(0, fccYUY2);
				}
				else
				if(caps & IVideoDecoder::CAP_YV12) 
				{
					source_cmodel = BC_YUV420P;
					vstream[i]->GetDecoder()->SetDestFmt(0, fccYV12);
				}
				else
				{
					source_cmodel = BC_RGB888;
					vstream[i]->GetDecoder()->SetDestFmt(24, 0);
				}
//printf("FileAVI::open_file: source_cmodel %d\n", source_cmodel);
				vstream[i]->SeekToTime(0);
			}
			else
			{
				asset->video_data = 0;
				break;
			}
		}

		if(asset->video_data)
		{
			asset->frame_rate = (double)1 / vstream[0]->GetFrameTime();
			vstream[0]->GetVideoFormatInfo(&bitmapinfo);
	    	asset->width = bitmapinfo.biWidth;
	    	asset->height = bitmapinfo.biHeight;

			asset->compression[0] = ((char*)&bitmapinfo.biCompression)[0];
			asset->compression[1] = ((char*)&bitmapinfo.biCompression)[1];
			asset->compression[2] = ((char*)&bitmapinfo.biCompression)[2];
			asset->compression[3] = ((char*)&bitmapinfo.biCompression)[3];
		}
	}


	asset->audio_streams = in_fd->AudioStreamCount();

	if(asset->audio_streams)
	{
		WAVEFORMATEX waveinfo;
		char *extinfo;

		asset->audio_data = 1;
		for(int i = 0; i < asset->audio_streams && i < MAX_STREAMS; i++)
		{
			astream[i] = in_fd->GetStream(i, IAviReadStream::Audio);
			if(!astream[i]->StartStreaming())
			{
			}
			else
			{
				asset->audio_data = 0;
				break;
			}
		}

		if(asset->audio_data)
		{
			astream[0]->GetAudioFormatInfo(&waveinfo, &extinfo);
			asset->channels = waveinfo.nChannels;
			asset->rate = waveinfo.nSamplesPerSec;
			asset->bits = MAX(16, waveinfo.wBitsPerSample);
		}
	}

	if(!asset->video_data && !asset->audio_data)
	{
		close_file();
		return 3;
	}
	return 0;
}

int FileAVI::reset()
{
	FileBase::reset();
	in_fd = 0;
	temp_audio = 0;
	temp_allocated = 0;
	return 0;
}

int FileAVI::close_file()
{
	FileBase::close_file();
	if(in_fd) delete in_fd;
	if(temp_audio) delete [] temp_audio;
	reset();
	return 0;
}

int FileAVI::set_video_stream(int stream)
{
	return 0;
}

int FileAVI::set_audio_stream(int stream)
{
	return 0;
}


long FileAVI::get_audio_length()
{
	long result = astream[0]->GetEndPos();
	return result;
}

int FileAVI::get_position(double *percentage, double *seconds)
{
	if(in_fd)
	{
		if(asset->video_data)
		{
			*percentage = (double)vstream[file->video_stream]->GetPos() / vstream[file->video_stream]->GetEndPos();
			*seconds = (double)vstream[file->video_stream]->GetPos() / asset->frame_rate;
		}
// Audio only
		else
		if(asset->audio_data)
		{
			*percentage = (double)file->audio_position / astream[file->audio_stream]->GetEndPos();
			*seconds = (double)file->audio_position / asset->rate;
		}
	}
	else
	{
		*percentage = *seconds = 0;
	}

	return 0;
}

int FileAVI::end_of_audio()
{
	return file->audio_position >= astream[file->audio_stream]->GetEndPos();
}

int FileAVI::end_of_video()
{
	return vstream[file->video_stream]->GetPos() >=
		vstream[file->video_stream]->GetEndPos();
}

int FileAVI::set_position(double percentage)
{
printf("FileAVI::set_position\n");
	if(in_fd)
	{
		if(asset->video_data)
		{
			long frame = (long)(percentage * vstream[file->video_stream]->GetEndPos());
			vstream[file->video_stream]->Seek(frame);
 		}

 		if(asset->audio_data)
 		{
 			long sample = (long)(percentage * astream[file->video_stream]->GetEndPos());
			astream[file->audio_stream]->Seek(sample);
 		}
	}

	return 0;
}

int FileAVI::set_audio_position(long x)
{
printf("FileAVI::set_audio_position\n");
	astream[file->audio_stream]->Seek(x);
	return 0;
}

int FileAVI::drop_frames(int frames)
{
	if(in_fd)
	{
		for(int i = 0; i < frames; i++)
			vstream[file->video_stream]->ReadFrame();
	}
	return 0;
}

int FileAVI::frame_back()
{
	if(in_fd)
	{
		long frame = vstream[file->video_stream]->GetPos();
		vstream[file->video_stream]->Seek(frame - 2);
	}
	return 0;
}


int FileAVI::colormodel_supported(int color_model)
{
	switch(color_model)
	{
		case BC_YUV420P:
			return source_cmodel == BC_YUV422 || source_cmodel == BC_YUV420P;
			break;
		case BC_YUV422:
			return source_cmodel == BC_YUV422;
			break;
	}
	return 0;
}

int FileAVI::read_frame(VFrame *frame, int in_y1, int in_y2)
{
//printf("FileAVI::read_frame 1 source_cmodel %d dest_cmodel %d\n", source_cmodel, frame->get_color_model());
	vstream[file->video_stream]->ReadFrame();
	unsigned char *data = vstream[file->video_stream]->GetFrame()->data();

//printf("FileAVI::read_frame 2 source_cmodel %d dest_cmodel %d\n", source_cmodel, frame->get_color_model());
	if(source_cmodel == frame->get_color_model())
	{
		bcopy(data, frame->get_data(), frame->get_data_size());
	}
	else
	{
		unsigned char **rows;
		rows = new unsigned char*[asset->height];
		for(int i = 0; i < asset->height; i++)
			rows[i] = data + 
				i * 
				asset->width * 
				VFrame::calculate_bytes_per_pixel(source_cmodel);

		unsigned char *y_data, *u_data, *v_data;
// Get planes from data
		switch(source_cmodel)
		{
			case BC_YUV420P:
				y_data = data;
				u_data = data + asset->width * asset->height;
				v_data = u_data + asset->width * asset->height / 4;
				break;
		}

		cmodel_transfer(frame->get_rows(), 
			rows,
			frame->get_y(),
			frame->get_u(),
			frame->get_v(),
			y_data,
			u_data,
			v_data, 
			0,
			in_y1, 
			asset->width, 
			in_y2 - in_y1,
			0, 
			0, 
			frame->get_w(), 
			frame->get_h(),
			source_cmodel, 
			frame->get_color_model(),
			0,
			asset->width,
			frame->get_w());
		delete [] rows;
	}
//printf("FileAVI::read_frame 7\n");
	return 0;
}

int FileAVI::read_audio(char *buffer, long len)
{
	Unsigned samples_read, bytes_read, bufsize;
	bufsize = len * asset->channels * asset->bits / 8;

printf("FileAVI::read_audio %d\n", astream[file->audio_stream]->GetFrameSize());

	astream[file->audio_stream]->ReadFrames(buffer, 
		bufsize,
		len,
		samples_read, 
		bytes_read);

printf("FileAVI::read_audio %d %d %d %d\n", bufsize,
		len,
		samples_read, 
		bytes_read);
	return 0;
}
#endif
