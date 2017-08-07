
/*
 * CINELERRA
 * Copyright (C) 2016 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "asset.h" 

extern "C" 
{
#include "avcodec.h"
#include "avformat.h"
}
#include "bcsignals.h"
#include "clip.h"
#include "file.h"
#include "fileffmpeg.h"
#include "mpegaudio.h"
#include "mutex.h"
#include <unistd.h>
#include "videodevice.inc"

#include <string.h>

// Different ffmpeg versions
#define FFMPEG_2010

Mutex* FileFFMPEG::ffmpeg_lock = new Mutex("FileFFMPEG::ffmpeg_lock");






FileFFMPEGStream::FileFFMPEGStream()
{
	ffmpeg_file_context = 0;

	first_frame = 1;
	current_frame = 0;
	decoded_frame = 0;

// Interleaved samples
	pcm_history = 0;
	history_allocated = 0;
	history_size = 0;
	history_start = 0;
	decode_start = 0;
	decode_len = 0;
	decode_end = 0;
	channels = 0;
	index = 0;
	current_sample = 0;
	decoded_sample = 0;
}

FileFFMPEGStream::~FileFFMPEGStream()
{
	if(pcm_history)
	{
		for(int i = 0; i < channels; i++)
			delete [] pcm_history[i];
	}
	delete [] pcm_history;

	if(ffmpeg_file_context)
	{
		avformat_close_input((AVFormatContext**)&ffmpeg_file_context);
	}
	
	ffmpeg_file_context = 0;
}

void FileFFMPEGStream::update_pcm_history(int64_t current_sample, int64_t len)
{
	decode_start = 0;
	decode_len = 0;

	if(!pcm_history)
	{
		pcm_history = new double*[channels];
		for(int i = 0; i < channels; i++)
			pcm_history[i] = new double[HISTORY_MAX];
		history_start = 0;
		history_size = 0;
		history_allocated = HISTORY_MAX;
	}
	

//printf("FileBase::update_pcm_history current_sample=%lld history_start=%lld history_size=%lld\n",
//file->current_sample,
//history_start,
//history_size);
// Restart history.  Don't bother shifting history back.
	if(current_sample < history_start ||
		current_sample > history_start + history_size)
	{
		history_size = 0;
		history_start = current_sample;
		decode_start = current_sample;
		decode_len = len;
	}
	else
// Shift history forward to make room for new samples
	if(current_sample > history_start + HISTORY_MAX)
	{
		int diff = current_sample - (history_start + HISTORY_MAX);
		for(int i = 0; i < channels; i++)
		{
			double *temp = pcm_history[i];
			memcpy(temp, temp + diff, (history_size - diff) * sizeof(double));
		}

		history_start += diff;
		history_size -= diff;

// Decode more data
		decode_start = history_start + history_size;
		decode_len = current_sample + len - (history_start + history_size);
	}
	else
// Starting somewhere in the buffer
	{
		decode_start = history_start + history_size;
		decode_len = current_sample + len - (history_start + history_size);
	}
}


void FileFFMPEGStream::allocate_history(int len)
{
	if(history_size + len > history_allocated)
	{
		double **temp = new double*[channels];

		for(int i = 0; i < channels; i++)
		{
			temp[i] = new double[history_size + len];
			memcpy(temp[i], pcm_history[i], history_size * sizeof(double));
			delete [] pcm_history[i];
		}

		delete [] pcm_history;
		pcm_history = temp;
		history_allocated = history_size + len;
	}
}

void FileFFMPEGStream::append_history(void *frame2, int len)
{
// printf("FileFFMPEGStream::append_history %d len=%d format=%d\n", 
// __LINE__,
// len,
// frame->format);
	allocate_history(len);
	AVFrame *frame = (AVFrame*)frame2;

	for(int i = 0; i < channels; i++)
	{
		switch(frame->format)
		{
			case AV_SAMPLE_FMT_S16P:
			{
				double *output = pcm_history[i] + history_size;
				int16_t *input = (int16_t*)frame->data[i];
				for(int j = 0; j < len; j++)
				{
					*output++ = (double)*input / 32767;
					input++;
				}
				break;
			}
			
			case AV_SAMPLE_FMT_FLTP:
			{
				double *output = pcm_history[i] + history_size;
				float *input = (float*)frame->data[i];
				for(int j = 0; j < len; j++)
				{
					*output++ = *input;
					input++;
				}
				break;
			}
			
			default:
				printf("FileFFMPEGStream::append_history %d unsupported audio format %d\n", 
					__LINE__,
					frame->format);
				break;
		}
	}

	history_size += len;
	decode_end += len;
}


void FileFFMPEGStream::read_history(double *dst,
	int64_t start_sample, 
	int channel,
	int64_t len)
{
	if(start_sample - history_start + len > history_size)
		len = history_size - (start_sample - history_start);
// printf("FileBase::read_history %d start_sample=%lld history_start=%lld history_size=%lld len=%lld\n", 
// __LINE__, 
// start_sample, 
// history_start, 
// history_size, 
// len);
	double *input = pcm_history[channel] + start_sample - history_start;
	for(int i = 0; i < len; i++)
	{
		*dst++ = *input++;
	}
// printf("FileBase::read_history %d\n", 
// __LINE__);
}






FileFFMPEG::FileFFMPEG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_FFMPEG;
}

FileFFMPEG::~FileFFMPEG()
{
	close_file();
}

void FileFFMPEG::reset()
{
//	ffmpeg_format = 0;
//	ffmpeg_frame = 0;
//	ffmpeg_samples = 0;
}

char* FileFFMPEG::get_format_string(Asset *asset)
{
	unsigned char test[16];
	const int debug = 0;
if(debug) printf("FileFFMPEG::get_format_string %d\n", __LINE__);
	FILE *in = fopen(asset->path, "r");
if(debug) printf("FileFFMPEG::get_format_string %d\n", __LINE__);
	char *format_string = 0;
if(debug) printf("FileFFMPEG::get_format_string %d\n", __LINE__);

	if(in)
	{
if(debug) printf("FileFFMPEG::get_format_string %d\n", __LINE__);
		int temp = fread(test, sizeof(test), 1, in);
// printf("FileFFMPEG %d %02x %02x %02x %02x \n", 
// __LINE__, 
// test[0], 
// test[1], 
// test[2], 
// test[3]);
// Matroska
		if(test[0] == 0x1a &&
			test[1] == 0x45 &&
			test[2] == 0xdf &&
			test[3] == 0xa3)
		{
if(debug) printf("FileFFMPEG::get_format_string %d\n", __LINE__);
			format_string = (char*)"matroska";
		}
if(debug) printf("FileFFMPEG::get_format_string %d\n", __LINE__);

		fclose(in);
if(debug) printf("FileFFMPEG::get_format_string %d\n", __LINE__);
		return format_string;
	}

	return 0;
}

int FileFFMPEG::check_sig(Asset *asset)
{
	char *ptr = strstr(asset->path, ".pcm");
	if(ptr) return 0;


	ffmpeg_lock->lock("FileFFMPEG::check_sig");
//	avcodec_init();
    avcodec_register_all();
    av_register_all();

	AVFormatContext *ffmpeg_file_context = 0;
//    AVFormatParameters params;
//	bzero(&params, sizeof(params));
// 	int result = av_open_input_file(
// 		&ffmpeg_file_context, 
// 		asset->path, 
// 		0, 
// 		0, 
// 		&params);
	int result = avformat_open_input(
		&ffmpeg_file_context, 
		asset->path, 
		0, 
		0);

//printf("FileFFMPEG::check_sig %d result=%d\n", __LINE__, result);
	if(result >= 0)
	{
		result = avformat_find_stream_info(ffmpeg_file_context, 0);

		
		if(result >= 0)
		{
			avformat_close_input(&ffmpeg_file_context);
			ffmpeg_lock->unlock();
			return 1;
		}
		
		ffmpeg_lock->unlock();
		return 0;
	}
	else
	{
		ffmpeg_lock->unlock();
		return 0;
	}
	
	
// 	char *format_string = get_format_string(asset);
// 	AVInputFormat *ffmpeg_format = 0;
// 
// 
// 	if(format_string)
// 	{
//     	ffmpeg_format = av_find_input_format(format_string);
// // printf("FileFFMPEG::check_sig path=%s ffmpeg_format=%p\n",
// // asset->path,
// // ffmpeg_format);
// 	}
// 
// 	ffmpeg_lock->unlock();
// 	if(!ffmpeg_format) 
// 		return 0;
// 	else
// 		return 1;
}

int FileFFMPEG::open_file(int rd, int wr)
{
	const int debug = 0;
	int result = 0;
//    AVFormatParameters params;
//	bzero(&params, sizeof(params));

	ffmpeg_lock->lock("FileFFMPEG::open_file");
//	avcodec_init();
    avcodec_register_all();
    av_register_all();

	if(rd)
	{
		AVFormatContext *ffmpeg_file_context = 0;

if(debug) printf("FileFFMPEG::open_file %d\n", __LINE__);
// 		result = av_open_input_file(
// 			(AVFormatContext**)&ffmpeg_file_context, 
// 			asset->path, 
// 			0,
// 			0, 
// 			&params);
		result = avformat_open_input(
			&ffmpeg_file_context, 
			asset->path, 
			0,
			0);

		if(debug) printf("FileFFMPEG::open_file %d result=%d\n", __LINE__, result);

		if(result >= 0)
		{
			if(debug) printf("FileFFMPEG::open_file %d this=%p result=%d ffmpeg_file_context=%p\n", __LINE__, this, result, ffmpeg_file_context);
			result = avformat_find_stream_info(ffmpeg_file_context, 0);
			if(debug) printf("FileFFMPEG::open_file %d this=%p result=%d\n", __LINE__, this, result);
		}
		else
		{
			ffmpeg_lock->unlock();
			if(debug) printf("FileFFMPEG::open_file %d\n", __LINE__);
			return 1;
		}
		if(debug) printf("FileFFMPEG::open_file %d result=%d\n", __LINE__, result);

// Convert format to asset
		if(result >= 0)
		{
			result = 0;
			asset->format = FILE_FFMPEG;
			asset->channels = 0;
			asset->audio_data = 0;

if(debug) printf("FileFFMPEG::open_file %d streams=%d\n", __LINE__, ((AVFormatContext*)ffmpeg_file_context)->nb_streams);
			for(int i = 0; i < ((AVFormatContext*)ffmpeg_file_context)->nb_streams; i++)
			{
				AVStream *ffmpeg_stream = ((AVFormatContext*)ffmpeg_file_context)->streams[i];
       			AVCodecContext *decoder_context = ffmpeg_stream->codec;
        		switch(decoder_context->codec_type) 
				{
        			case AVMEDIA_TYPE_AUDIO:
					{
if(debug) printf("FileFFMPEG::open_file %d i=%d CODEC_TYPE_AUDIO\n", __LINE__, i);
if(debug) printf("FileFFMPEG::open_file %d decoder_context->codec_id=%d\n", __LINE__, decoder_context->codec_id);
						AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
						if(!codec)
						{
							printf("FileFFMPEG::open_file: audio codec 0x%x not found.\n", 
								decoder_context->codec_id);
						}
						else
						{
							FileFFMPEGStream *new_stream = new FileFFMPEGStream;
							audio_streams.append(new_stream);
							new_stream->index = i;
							new_stream->channels = decoder_context->channels;


// Open a new FFMPEG file for the stream
							result = avformat_open_input(
								(AVFormatContext**)&new_stream->ffmpeg_file_context, 
								asset->path, 
								0,
								0);
							avformat_find_stream_info((AVFormatContext*)new_stream->ffmpeg_file_context, 0);
							ffmpeg_stream = ((AVFormatContext*)new_stream->ffmpeg_file_context)->streams[i];
							decoder_context = ffmpeg_stream->codec;
							codec = avcodec_find_decoder(decoder_context->codec_id);

							//avcodec_thread_init(decoder_context, file->cpus);
							decoder_context->thread_count = file->cpus;
							avcodec_open2(decoder_context, codec, 0);

							asset->channels += new_stream->channels;
							asset->bits = 16;
							asset->audio_data = 1;
							asset->sample_rate = decoder_context->sample_rate;
							int64_t audio_length = (int64_t)(((AVFormatContext*)new_stream->ffmpeg_file_context)->duration * 
								asset->sample_rate / 
								AV_TIME_BASE);
if(debug) printf("FileFFMPEG::open_file %d audio_length=%lld\n", __LINE__, (long long)audio_length);
							asset->audio_length = MAX(asset->audio_length, audio_length);
						}
            			break;
					}

        			case AVMEDIA_TYPE_VIDEO:
if(debug) printf("FileFFMPEG::open_file %d i=%d CODEC_TYPE_VIDEO\n", __LINE__, i);
            			if(video_streams.size() == 0)
						{
							FileFFMPEGStream *new_stream = new FileFFMPEGStream;
							video_streams.append(new_stream);
							new_stream->index = i;

					
							asset->video_data = 1;
							asset->layers = 1;

// Open a new FFMPEG file for the stream
							result = avformat_open_input(
								(AVFormatContext**)&new_stream->ffmpeg_file_context, 
								asset->path, 
								0,
								0);
							avformat_find_stream_info((AVFormatContext*)new_stream->ffmpeg_file_context, 0);
							ffmpeg_stream = ((AVFormatContext*)new_stream->ffmpeg_file_context)->streams[i];
							decoder_context = ffmpeg_stream->codec;
							AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
//							avcodec_thread_init(decoder_context, file->cpus);
							decoder_context->thread_count = file->cpus;
							avcodec_open2(decoder_context, codec, 0);

							asset->width = decoder_context->width;
							asset->height = decoder_context->height;
							if(EQUIV(asset->frame_rate, 0))
								asset->frame_rate = 
									(double)ffmpeg_stream->r_frame_rate.num /
									(double)ffmpeg_stream->r_frame_rate.den;
// 								(double)decoder_context->time_base.den / 
// 								decoder_context->time_base.num;
							asset->video_length = (int64_t)(((AVFormatContext*)new_stream->ffmpeg_file_context)->duration *
								asset->frame_rate / 
								AV_TIME_BASE);
							asset->aspect_ratio = 
								(double)decoder_context->sample_aspect_ratio.num / 
								decoder_context->sample_aspect_ratio.den;
if(debug) printf("FileFFMPEG::open_file %d decoder_context->codec_id=%d\n", 
__LINE__, 
decoder_context->codec_id);

						}
            			break;

        			default:
            			break;
        		}
			}

			if(debug) 
			{
				printf("FileFFMPEG::open_file %d audio_streams=%d video_streams=%d\n",
					__LINE__,
					audio_streams.size(),
					audio_streams.size());
				asset->dump();
			}
		}
		else
		{
			ffmpeg_lock->unlock();
			if(ffmpeg_file_context)
			{
				avformat_close_input((AVFormatContext**)&ffmpeg_file_context);
			}
if(debug) printf("FileFFMPEG::open_file %d\n", __LINE__);
			return 1;
		}


		if(ffmpeg_file_context)
		{
			avformat_close_input((AVFormatContext**)&ffmpeg_file_context);
		}

	}

if(debug) printf("FileFFMPEG::open_file %d result=%d\n", __LINE__, result);
	ffmpeg_lock->unlock();
	return result;
}

int FileFFMPEG::close_file()
{
	const int debug = 0;
	if(debug) printf("FileFFMPEG::close_file %d\n", __LINE__);
	ffmpeg_lock->lock("FileFFMPEG::close_file");
//	if(ffmpeg_frame) av_frame_free(ffmpeg_frame);
	if(debug) printf("FileFFMPEG::close_file %d\n", __LINE__);
//	if(ffmpeg_samples) av_frame_free(ffmpeg_samples);
	if(debug) printf("FileFFMPEG::close_file %d\n", __LINE__);
	audio_streams.remove_all_objects();
	if(debug) printf("FileFFMPEG::close_file %d\n", __LINE__);
	video_streams.remove_all_objects();

	if(debug) printf("FileFFMPEG::close_file %d\n", __LINE__);

	reset();
	if(debug) printf("FileFFMPEG::close_file %d\n", __LINE__);
	ffmpeg_lock->unlock();
}


int64_t FileFFMPEG::get_memory_usage()
{
	return 0;
}


int FileFFMPEG::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileFFMPEG::get_best_colormodel(Asset *asset, int driver)
{
//printf("FileFFMPEG::get_best_colormodel %d driver=%d\n", __LINE__, driver);
	switch(driver)
	{
		case PLAYBACK_X11:
//			return BC_RGB888;
// the direct X11 color model requires scaling in the codec
			return BC_BGR8888;
			
		case PLAYBACK_X11_XV:
		case PLAYBACK_ASYNCHRONOUS:
			return BC_YUV420P;
			
		case PLAYBACK_X11_GL:
			return BC_YUV888;
			
		default:
			return BC_YUV420P;
	}
}

void FileFFMPEG::dump_context(void *ptr)
{
	AVCodecContext *context = (AVCodecContext*)ptr;

// 	printf("FileFFMPEG::dump_context %d\n", __LINE__);
// 	printf("    bit_rate=%d\n", context->bit_rate);
// 	printf("    bit_rate_tolerance=%d\n", context->bit_rate_tolerance);
// 	printf("    flags=%d\n", context->flags);
// 	printf("    sub_id=%d\n", context->sub_id);
// 	printf("    me_method=%d\n", context->me_method);
// 	printf("    extradata_size=%d\n", context->extradata_size);
// 	printf("    time_base.num=%d\n", context->time_base.num);
// 	printf("    time_base.den=%d\n", context->time_base.den);
// 	printf("    width=%d\n", context->width);
// 	printf("    height=%d\n", context->height);
// 	printf("    gop_size=%d\n", context->gop_size);
// 	printf("    pix_fmt=%d\n", context->pix_fmt);
// 	printf("    rate_emu=%d\n", context->rate_emu);
// 	printf("    sample_rate=%d\n", context->sample_rate);
// 	printf("    channels=%d\n", context->channels);
// 	printf("    sample_fmt=%d\n", context->sample_fmt);
// 	printf("    frame_size=%d\n", context->frame_size);
// 	printf("    frame_number=%d\n", context->frame_number);
// 	printf("    real_pict_num=%d\n", context->real_pict_num);
// 	printf("    delay=%d\n", context->delay);
// 	printf("    qcompress=%f\n", context->qcompress);
// 	printf("    qblur=%f\n", context->qblur);
// 	printf("    qmin=%d\n", context->qmin);
// 	printf("    qmax=%d\n", context->qmax);
// 	printf("    max_qdiff=%d\n", context->max_qdiff);
// 	printf("    max_b_frames=%d\n", context->max_b_frames);
// 	printf("    b_quant_factor=%f\n", context->b_quant_factor);
// 	printf("    b_frame_strategy=%d\n", context->b_frame_strategy);
// 	printf("    hurry_up=%d\n", context->hurry_up);
// 	printf("    rtp_payload_size=%d\n", context->rtp_payload_size);
// 	printf("    codec_id=%d\n", context->codec_id);
// 	printf("    codec_tag=%d\n", context->codec_tag);
// 	printf("    workaround_bugs=%d\n", context->workaround_bugs);
// 	printf("    has_b_frames=%d\n", context->has_b_frames);
// 	printf("    block_align=%d\n", context->block_align);
// 	printf("    parse_only=%d\n", context->parse_only);
// 	printf("    idct_algo=%d\n", context->idct_algo);
// 	printf("    slice_count=%d\n", context->slice_count);
// 	printf("    slice_offset=%p\n", context->slice_offset);
// 	printf("    error_concealment=%d\n", context->error_concealment);
// 	printf("    dsp_mask=%x\n", context->dsp_mask);
// 	printf("    slice_flags=%d\n", context->slice_flags);
// 	printf("    xvmc_acceleration=%d\n", context->xvmc_acceleration);
// 	printf("    antialias_algo=%d\n", context->antialias_algo);
// 	printf("    thread_count=%d\n", context->thread_count);
// 	printf("    skip_top=%d\n", context->skip_top);
// 	printf("    profile=%d\n", context->profile);
// 	printf("    level=%d\n", context->level);
// 	printf("    lowres=%d\n", context->lowres);
// 	printf("    coded_width=%d\n", context->coded_width);
// 	printf("    coded_height=%d\n", context->coded_height);
}


int FileFFMPEG::read_frame(VFrame *frame)
{
	int error = 0;
	const int debug = 0;

	ffmpeg_lock->lock("FileFFMPEG::read_frame");
	
	
	FileFFMPEGStream *stream = video_streams.get(0);
	if(debug) printf("FileFFMPEG::read_frame %d stream=%p stream->ffmpeg_file_contex=%p\n", 
		__LINE__, 
		stream,
		stream->ffmpeg_file_context);
	AVStream *ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->index];
	AVCodecContext *decoder_context = ffmpeg_stream->codec;
	AVFrame *ffmpeg_frame = av_frame_alloc();

	if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);

// if(file->current_frame == 100)
// {
// printf("FileFFMPEG::read_frame %d fake crash\n", __LINE__);
// 	exit(1);
// }


//dump_context(stream->codec);
	if(stream->first_frame)
	{
		stream->first_frame = 0;
		int got_it = 0;

		while(!got_it && !error)
		{
			AVPacket *packet = av_packet_alloc();
			if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);
			error = av_read_frame((AVFormatContext*)stream->ffmpeg_file_context, 
				packet);
			if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);

			if(!error && packet->size > 0)
			{
				if(packet->stream_index == stream->index)
				{
					int got_picture = 0;

					if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);

//                	avcodec_get_frame_defaults((AVFrame*)ffmpeg_frame);
					if(debug) printf("FileFFMPEG::read_frame %d decoder_context=%p ffmpeg_frame=%p\n", 
						__LINE__,
						decoder_context,
						ffmpeg_frame);

printf("FileFFMPEG::read_frame %d buf=%p bufsize=%d data=%p size=%d side_data_elems=%d\n", 
__LINE__,
packet->buf,
packet->buf->size,
packet->data,
packet->size,
packet->side_data_elems);
printf("FileFFMPEG::read_frame %d data=\n", __LINE__);
for(int i = 0; i < 256; i++)
{
	printf("%02x", packet->data[i]);
	if((i + 1) % 16 == 0)
	{
		printf("\n");
	}
	else
	{
		printf(" ");
	}
}
printf("\n");

		        	int result = avcodec_decode_video2(
						decoder_context,
                    	ffmpeg_frame, 
						&got_picture,
                    	packet);
					if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);
					if(ffmpeg_frame->data[0] && got_picture) got_it = 1;
					if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);
				}
			}

			av_packet_free(&packet);
		}

		error = 0;
	}
	if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);

#define SEEK_THRESHOLD 16

// printf("FileFFMPEG::read_frame %d current_frame=%lld file->current_frame=%lld\n", 
// __LINE__, 
// current_frame,
// file->current_frame);
	if(stream->current_frame != file->current_frame &&
		(file->current_frame < stream->current_frame ||
		file->current_frame > stream->current_frame + SEEK_THRESHOLD))
	{
		if(debug) printf("FileFFMPEG::read_frame %d stream->current_frame=%lld file->current_frame=%lld\n", 
		__LINE__, 
		(long long)stream->current_frame, 
		(long long)file->current_frame);

		int64_t timestamp = (int64_t)((double)file->current_frame * 
			ffmpeg_stream->time_base.den /
			ffmpeg_stream->time_base.num /
			asset->frame_rate);
// Want to seek to the nearest keyframe and read up to the current frame
// but ffmpeg doesn't support that kind of precision.
// Also, basing all the seeking on the same stream seems to be required for synchronization.
		av_seek_frame((AVFormatContext*)stream->ffmpeg_file_context, 
			/* stream->index */ 0, 
			timestamp, 
			AVSEEK_FLAG_ANY);
		stream->current_frame = file->current_frame - 1;
	}
	if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);

	int got_it = 0;
// Read frames until we catch up to the current position.
// 	if(current_frame >= file->current_frame - SEEK_THRESHOLD &&
// 		current_frame < file->current_frame - 1)
// 	{
// 		printf("FileFFMPEG::read_frame %d current_frame=%lld file->current_frame=%lld\n", 
// 			__LINE__,
// 			current_frame,
// 			file->current_frame);
// 	}



	while(stream->current_frame < file->current_frame && !error)
	{
		got_it = 0;
		if(debug) printf("FileFFMPEG::read_frame %d stream->current_frame=%lld file->current_frame=%lld\n", 
			__LINE__,
			(long long)stream->current_frame,
			(long long)file->current_frame);

		while(!got_it && !error)
		{
			AVPacket *packet = av_packet_alloc();

			error = av_read_frame((AVFormatContext*)stream->ffmpeg_file_context, 
				packet);

			if(!error && packet->size > 0)
			{
				if(packet->stream_index == stream->index)
				{
					int got_picture = 0;
//                	avcodec_get_frame_defaults((AVFrame*)ffmpeg_frame);


// printf("FileFFMPEG::read_frame %d this=%p buf=%p data=%p size=%d side_data_elems=%d\n", 
// __LINE__,
// this,
// packet->buf,
// packet->data,
// packet->size,
// packet->side_data_elems);
// 
// if(file->current_frame >= 200 && file->current_frame < 280)
// {
// char string[1024];
// sprintf(string, "/tmp/debug%03lld", file->current_frame);
// FILE *out = fopen(string, "w");
// fwrite(packet->data, packet->size, 1, out);
// fclose(out);
// }


		        	int result = avcodec_decode_video2(
						decoder_context,
                    	(AVFrame*)ffmpeg_frame, 
						&got_picture,
                    	packet);


//printf("FileFFMPEG::read_frame %d result=%d\n", __LINE__, result);
					if(((AVFrame*)ffmpeg_frame)->data[0] && got_picture) got_it = 1;
//printf("FileFFMPEG::read_frame %d result=%d got_it=%d\n", __LINE__, result, got_it);
				}
			}
			
			
			av_packet_free(&packet);
		}

		if(got_it) stream->current_frame++;
	}

//PRINT_TRACE
// printf("FileFFMPEG::read_frame %d current_frame=%lld file->current_frame=%lld got_it=%d\n", 
// __LINE__, 
// current_frame,
// file->current_frame,
// got_it);
	if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);

// Convert colormodel
	if(got_it)
	{
		int input_cmodel;
		AVFrame *input_frame = (AVFrame*)ffmpeg_frame;


// printf("FileFFMPEG::read_frame %d pix_fmt=%d output_cmodel=%d %02x %02x %02x %02x %02x %02x %02x %02x\n", 
// __LINE__, 
// decoder_context->pix_fmt,
// frame->get_color_model(),
// input_frame->data[1][0],
// input_frame->data[1][1],
// input_frame->data[1][2],
// input_frame->data[1][3],
// input_frame->data[1][4],
// input_frame->data[1][5],
// input_frame->data[1][6],
// input_frame->data[1][7]);

		switch(decoder_context->pix_fmt)
		{
			case AV_PIX_FMT_YUV420P10LE:
				input_cmodel = BC_YUV420P10LE;
				break;
		
			case AV_PIX_FMT_YUV420P:
				input_cmodel = BC_YUV420P;
				break;
#ifndef FFMPEG_2010
			case AV_PIX_FMT_YUV422:
				input_cmodel = BC_YUV422;
				break;
#endif

			case AV_PIX_FMT_YUV422P:
				input_cmodel = BC_YUV422P;
				break;
			case AV_PIX_FMT_YUV410P:
				input_cmodel = BC_YUV9P;
				break;
			default:
				fprintf(stderr, 
					"quicktime_ffmpeg_decode: unrecognized color model %d\n", 
					decoder_context->pix_fmt);
				input_cmodel = BC_YUV420P;
				break;
		}



		unsigned char **input_rows = 
			(unsigned char**)malloc(sizeof(unsigned char*) * 
			decoder_context->height);


		for(int i = 0; i < decoder_context->height; i++)
		{
			input_rows[i] = input_frame->data[0] + 
				i * 
				decoder_context->width * 
				cmodel_calculate_pixelsize(input_cmodel);
		}


		cmodel_transfer(frame->get_rows(), /* Leave NULL if non existent */
			input_rows,
			frame->get_y(), /* Leave NULL if non existent */
			frame->get_u(),
			frame->get_v(),
			input_frame->data[0], /* Leave NULL if non existent */
			input_frame->data[1],
			input_frame->data[2],
			0,        /* Dimensions to capture from input frame */
			0, 
			decoder_context->width, 
			decoder_context->height,
			0,       /* Dimensions to project on output frame */
			0, 
			frame->get_w(), 
			frame->get_h(),
			input_cmodel, 
			frame->get_color_model(),
			0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			input_frame->linesize[0],       /* For planar use the luma rowspan */
			frame->get_w());
		free(input_rows);
	}
//PRINT_TRACE

	av_frame_free(&ffmpeg_frame);

	ffmpeg_lock->unlock();
	if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);
	return error;
}

int FileFFMPEG::read_samples(double *buffer, int64_t len)
{
	const int debug = 0;
	int error = 0;
	ffmpeg_lock->lock("FileFFMPEG::read_samples");

	if(debug) printf("FileFFMPEG::read_samples %d\n", __LINE__);
// Compute stream & stream channel from global channel
	int audio_channel = file->current_channel;
	int audio_index = -1;
	FileFFMPEGStream *stream = 0;
	for(int i = 0; i < audio_streams.size(); i++)
	{
		if(audio_channel < audio_streams.get(i)->channels)
		{
			stream = audio_streams.get(i);
			audio_index = stream->index;
			break;
		}
		audio_channel -= audio_streams.get(i)->channels;
	}
	if(debug) printf("FileFFMPEG::read_samples %d\n", __LINE__);

	AVStream *ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->index];
	AVCodecContext *decoder_context = ffmpeg_stream->codec;

	stream->update_pcm_history(file->current_sample, len);
	if(debug) printf("FileFFMPEG::read_samples %d len=%d\n", __LINE__, (int)len);



	if(debug) printf("FileFFMPEG::read_samples %d decode_start=%lld decode_end=%lld\n",
		__LINE__,
		(long long)stream->decode_start,
		(long long)stream->decode_end);

// Seek occurred
	if(stream->decode_start != stream->decode_end)
	{
		int64_t timestamp = (int64_t)((double)file->current_sample * 
			ffmpeg_stream->time_base.den /
			ffmpeg_stream->time_base.num /
			asset->sample_rate);
// Want to seek to the nearest keyframe and read up to the current frame
// but ffmpeg doesn't support that kind of precision.
// Also, basing all the seeking on the same stream seems to be required for synchronization.
		if(debug) printf("FileFFMPEG::read_samples %d\n",
			__LINE__);
		av_seek_frame((AVFormatContext*)stream->ffmpeg_file_context, 
			/* stream->index */ 0, 
			timestamp, 
			AVSEEK_FLAG_ANY);
		if(debug) printf("FileFFMPEG::read_samples %d\n",
			__LINE__);
		stream->current_sample = file->current_sample;
		stream->decode_end = stream->decode_start;
	}

	if(debug) printf("FileFFMPEG::read_samples %d stream->decode_len=%d\n", 
		__LINE__,
		(int)stream->decode_len);




	int got_it = 0;
	int accumulation = 0;
// Read frames until the requested range is decoded.
	while(accumulation < stream->decode_len && !error)
	{
//printf("FileFFMPEG::read_samples %d accumulation=%d\n", __LINE__, accumulation);
		AVPacket *packet = av_packet_alloc();
		
		error = av_read_frame((AVFormatContext*)stream->ffmpeg_file_context, 
			packet);
		unsigned char *packet_ptr = packet->data;
		int packet_len = packet->size;
		if(debug) printf("FileFFMPEG::read_samples %d error=%d packet_len=%d\n", 
		__LINE__, 
		error, 
		packet_len);

		if(packet->stream_index == stream->index)
		{
			while(packet_len > 0 && !error)
			{
//				int data_size = MPA_MAX_CODED_FRAME_SIZE;
				int got_frame;
                AVFrame *ffmpeg_samples = av_frame_alloc();
if(debug) printf("FileFFMPEG::read_samples %d decoder_context=%p ffmpeg_samples=%p packet.size=%d packet.data=%p codec_id=%d\n", 
__LINE__, 
decoder_context,
ffmpeg_samples,
packet_len,
packet_ptr,
decoder_context->codec_id);
//av_log_set_level(AV_LOG_DEBUG);


				int bytes_decoded = avcodec_decode_audio4(decoder_context, 
					ffmpeg_samples, 
					&got_frame,
                    packet);


if(debug) PRINT_TRACE
//				if(bytes_decoded < 0) error = 1;
				if(bytes_decoded == -1) error = 1;
				packet_ptr += bytes_decoded;
				packet_len -= bytes_decoded;
if(debug) printf("FileFFMPEG::read_samples %d bytes_decoded=%d\n", 
__LINE__, 
bytes_decoded);
//				if(data_size <= 0)
//					break;
				if(!got_frame) break;
				int samples_decoded = ffmpeg_samples->nb_samples;
// Transfer decoded samples to ring buffer
				stream->append_history(ffmpeg_samples, samples_decoded);
// static FILE *fd = 0;
// if(!fd) fd = fopen("/tmp/test.pcm", "w");
// fwrite(ffmpeg_samples, data_size, 1, fd);
				
				av_frame_free(&ffmpeg_samples);
				accumulation += samples_decoded;
			}
		}
		if(debug) PRINT_TRACE
		
		av_packet_free(&packet);
	}
	if(debug) printf("FileFFMPEG::read_samples %d\n", __LINE__);

	stream->read_history(buffer, 
		file->current_sample, 
		audio_channel,
		len);
	if(debug) printf("FileFFMPEG::read_samples %d\n", __LINE__);

	if(debug) printf("FileFFMPEG::read_samples %d %d\n", __LINE__, error);
	stream->current_sample += len;
	ffmpeg_lock->unlock();
	return error;
}






