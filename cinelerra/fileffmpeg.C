/*
 * CINELERRA
 * Copyright (C) 2016-2024 Adam Williams <broadcast at earthling dot net>
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

void quicktime_print_buffer(char *desc, uint8_t *input, int len);

}

#include "filesystem.h"
#include "bcsignals.h"
#include "clip.h"
#include "file.h"
#include "fileffmpeg.h"
#include "filefork.h"
#include "framecache.h"
#include "indexfile.h"
//#include "mpegaudio.h"
#include "mutex.h"
#include "mwindow.h"
#include "preferences.h"
#include "quicktime.h"
#include "videodevice.inc"

#include <string.h>
#include <string>
#include <unistd.h>

using std::string;


// Different ffmpeg versions
#define FFMPEG_2010


// stuff
#define FFMPEG_TOC_SIG "FFMPEGTOC03"

// hacks for seeking which depend on the codec
#define AUDIO_REWIND_SECS 0
//#define AUDIO_REWIND_SECS 1
//#define AUDIO_REWIND_SECS 10
#define VIDEO_REWIND_KEYFRAMES 2
#define VP9_REWIND_KEYFRAMES 1
//#define VIDEO_REWIND_KEYFRAMES 4



// MKV/WEBM doesn't have the required information for frame accurate
// seeking. It stores only a table of offsets where a packet is guaranteed
// to start, every few seconds, while each packet contains a more accurate
// PTS. MPlayer just jumps & reads forward to figure out where it is, with
// no prior knowledge of where the video keyframes are.
// 
// 
// 1 option is searching backwards with MKV's table. Jump to the nearest
// table offset, decode video forward to find the time of the 1st frame it
// can decode. If it's too high, restart decoding from the previous table
// entry. This is very slow.
// 
// The other option is making a better table of contents when opening the
// file, reading the entire file, tabulating every frame offset, tabulating
// whether it's a keyframe, & tabulating every audio packet offset.
// Cinelerra does this for every file anyway to draw the audio
// waveform, so it's just keeping more of the data it already reads.


// If different audio tracks have different sample rates, the tracks are
// different lengths.  The user has to select the right samplerate in the 
// asset edit window.



// encoding H265 directly with ffmpeg:
// ffmpeg -i test.mp4 -c:v libx265 -crf 28 -tag:v hvc1 test2.mp4


Mutex* FileFFMPEG::ffmpeg_lock = new Mutex("FileFFMPEG::ffmpeg_lock");






FileFFMPEGStream::FileFFMPEGStream()
{
	ffmpeg_file_context = 0;
    decoder_context = 0;

	current_frame = -1;
    is_video = 0;

// Interleaved samples
	pcm_history = 0;
	history_size = 0;
	history_start = 0;
    write_offset = 0;
	channels = 0;
	ffmpeg_id = 0;
    is_audio = 0;
    
    index_data = 0;
    index_allocated = 0;
    index_size = 0;
    index_zoom = 0;
    next_index_max = 0;
    next_index_min = 0;
    next_index_size = 0;
    next_index_allocated = 0;
}

FileFFMPEGStream::~FileFFMPEGStream()
{
	if(pcm_history)
	{
		for(int i = 0; i < channels; i++)
			delete [] pcm_history[i];
	}
	delete [] pcm_history;

// AVStream owns the decoder context in 3.3.3
#if LIBAVCODEC_VERSION_MAJOR >= 58
    if(decoder_context)
    {
        avcodec_free_context((AVCodecContext**)&decoder_context);
        decoder_context = 0;
    }
#endif

	if(ffmpeg_file_context)
	{
		avformat_close_input((AVFormatContext**)&ffmpeg_file_context);
	}
	
	ffmpeg_file_context = 0;
    
    if(next_index_max)
    {
        delete [] next_index_max;
    }
    
    if(next_index_min)
    {
        delete [] next_index_min;
    }
    
    delete_index();
}

void FileFFMPEGStream::delete_index()
{
    if(index_data)
    {
        for(int i = 0; i < channels; i++)
        {
            if(index_data[i])
            {
                delete [] index_data[i];
            }
        }
        delete [] index_data;
    }
    
    index_data = 0;
}


// Reset the history buffer if a seek happened.  
void FileFFMPEGStream::update_pcm_history(int64_t current_sample)
{

// Restart history.  Don't bother shifting it.
	if(current_sample < history_start ||
		current_sample > history_start + history_size)
	{
// printf("FileBase::update_pcm_history %d: current_sample=%ld history=%ld - %ld\n",
// __LINE__,
// current_sample,
// history_start,
// history_start + history_size);
		history_size = 0;
		history_start = current_sample;
		write_offset = 0;
	}
}


void FileFFMPEGStream::append_index(float *data, 
    int samples_decoded,
    int channels2,
    Asset *asset, 
    Preferences *preferences)
{
    int i, j, k;
    
// Allocate new index buffer
	if(!index_data)
	{
// Calculate the required zoom for the index based on ffmpeg's estimated file length
        int max_frames = preferences->index_size / // bytes
            sizeof(float) / 
            asset->channels /
            2; // high/low pair
        for(index_zoom = 1; 1; index_zoom *= 2)
        {
            if(asset->audio_length / index_zoom <= max_frames)
            {
                break;
            }
        }

// double the estimated size in case it was off
		index_allocated = asset->audio_length * 2 / index_zoom + 1;
		index_data = new float*[channels];
// printf("FileFFMPEGStream::append_index %d index_zoom=%d asset->audio_length=%ld index_allocated=%d\n",
// __LINE__,
// index_zoom,
// asset->audio_length,
// index_allocated);

// Allocate enough high and low pairs for all frames
		for(i = 0; i < channels; i++)
		{
        	index_data[i] = new float[index_allocated * 2];
        }
	}

    if(!next_index_max)
    {
        next_index_max = new float[channels];
        next_index_min = new float[channels];
    }

// in case the number of channels in the source changed
    int current_channels = channels;
    if(channels2 < current_channels)
    {
        current_channels = channels2;
    }

    for(int j = 0; j < samples_decoded; j++)
	{

// add a sample to the next frame
	    for(i = 0; i < current_channels; i++)
	    {
            float value = data[j * channels2 + i];
            
            if(value > next_index_max[i] || next_index_size == 0)
            {
                next_index_max[i] = value;
            }
            
            if(value < next_index_min[i] || next_index_size == 0)
            {
                next_index_min[i] = value;
            }
        }
//printf("FileFFMPEGStream::append_index %d\n", __LINE__);

        next_index_size++;

// index frame is ready to be added to the index
// will never have more samples in the buffer than index_zoom
        if(next_index_size >= index_zoom)
        {
            flush_index();
        }
	}

}


// add the next high/low pair
void FileFFMPEGStream::flush_index()
{
    int i, k;
    if(next_index_size > 0)
    {
        if(index_size < index_allocated)
        {
            for(i = 0; i < channels; i++)
            {
                index_data[i][index_size * 2] = next_index_max[i];
                index_data[i][index_size * 2 + 1] = next_index_min[i];
            }
            index_size++;
        }

// reset the next index frame
        next_index_size = 0;
    }
}

void FileFFMPEGStream::append_history(void *frame2, int len)
{
	AVFrame *frame = (AVFrame*)frame2;
// printf("FileFFMPEGStream::append_history %d len=%d channels=%d write_offset=%d\n", 
// __LINE__,
// len,
// channels,
// write_offset);
	if(!pcm_history)
	{
		pcm_history = new double*[channels];
		for(int i = 0; i < channels; i++)
			pcm_history[i] = new double[HISTORY_MAX];
	}


	for(int i = 0; i < channels; i++)
	{
		double *output = pcm_history[i] + write_offset;
        double *output_end = pcm_history[i] + HISTORY_MAX;
		switch(frame->format)
		{
			case AV_SAMPLE_FMT_S16:
			{
				int16_t *input = (int16_t*)frame->data[0];
				for(int j = 0; j < len; j++)
				{
					*output++ = (double)input[j * channels + i] / 32767;
                    if(output >= output_end)
                    {
                        output = pcm_history[i];
                    }
				}
				break;
			}
			
			case AV_SAMPLE_FMT_S16P:
			{
				int16_t *input = (int16_t*)frame->data[i];
				for(int j = 0; j < len; j++)
				{
					*output++ = (double)*input / 32767;
                    if(output >= output_end)
                    {
                        output = pcm_history[i];
                    }
					input++;
				}
				break;
			}
			
			case AV_SAMPLE_FMT_S32P:
			{
				int32_t *input = (int32_t*)frame->data[i];
				for(int j = 0; j < len; j++)
				{
					*output++ = (double)*input / 0x7fffffff;
                    if(output >= output_end)
                    {
                        output = pcm_history[i];
                    }
					input++;
				}
				break;
			}
			
			case AV_SAMPLE_FMT_FLTP:
			{
				float *input = (float*)frame->data[i];
				for(int j = 0; j < len; j++)
				{
					*output++ = *input;
                    if(output >= output_end)
                    {
                        output = pcm_history[i];
                    }
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
    
    write_offset += len;
    if(write_offset >= HISTORY_MAX)
    {
        write_offset -= HISTORY_MAX;
    }

	history_size += len;
    if(history_size >= HISTORY_MAX)
    {
        int64_t diff = history_size - HISTORY_MAX;
// advance start of history in the source
        history_start += diff;
        history_size = HISTORY_MAX;
    }
}


void FileFFMPEGStream::read_history(double *dst,
	int64_t start_sample, 
	int channel,
	int64_t len)
{
// truncate the length to the history size
	if(start_sample - history_start + len > history_size)
		len = history_size - (start_sample - history_start);

// calculate the read offset
    int read_offset = (write_offset - history_size) + 
        (start_sample - history_start);
    if(read_offset < 0)
    {
        read_offset += HISTORY_MAX;
    }

// printf("FileBase::read_history %d start_sample=%lld history_start=%lld history_size=%lld len=%lld\n", 
// __LINE__, 
// start_sample, 
// history_start, 
// history_size, 
// len);
	double *input = pcm_history[channel] + read_offset;
    double *output_end = pcm_history[channel] + HISTORY_MAX;
	for(int i = 0; i < len; i++)
	{
		*dst++ = *input++;
        if(input >= output_end)
        {
            input = pcm_history[channel];
        }
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

FileFFMPEG::FileFFMPEG()
 : FileBase()
{
    ids.append(FILE_FFMPEG);
    has_audio = 1;
    has_video = 1;
    has_rd = 1;
}

FileBase* FileFFMPEG::create(File *file)
{
    return new FileFFMPEG(file->asset, file);
}

const char* FileFFMPEG::formattostr(int format)
{
    switch(format)
    {
		case FILE_FFMPEG:
			return FFMPEG_NAME;
			break;
    }
    return 0;
}





void FileFFMPEG::reset()
{
    has_toc = 0;
    ffmpeg_frame = 0;
    got_frame = 0;
    need_restart = 0;
    last_pts = -1;
#ifdef USE_FFMPEG_OUTPUT
    ffmpeg_output = 0;
#endif
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

int FileFFMPEG::check_sig(File *file, const uint8_t *test_data)
{
    Asset *asset = file->asset;
	char *ptr = strstr(asset->path, ".pcm");
	if(ptr) return 0;


	ffmpeg_lock->lock("FileFFMPEG::check_sig");
#if LIBAVCODEC_VERSION_MAJOR < 58
    avcodec_register_all();
    av_register_all();
#endif

//printf("FileFFMPEG::check_sig %d\n", __LINE__);
	AVFormatContext *ffmpeg_file_context = 0;
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
	
	
}

int FileFFMPEG::open_file(int rd, int wr)
{
	const int debug = 0;
	int result = 0;
//    AVFormatParameters params;
//	bzero(&params, sizeof(params));
if(debug) printf("FileFFMPEG::open_file %d result=%d\n", __LINE__, result);

#if LIBAVCODEC_VERSION_MAJOR < 58
	ffmpeg_lock->lock("FileFFMPEG::open_file");
    avcodec_register_all();
    av_register_all();
	ffmpeg_lock->unlock();
#endif
if(debug) printf("FileFFMPEG::open_file %d result=%d\n", __LINE__, result);

	if(rd)
	{
        result = open_ffmpeg();
//printf("FileFFMPEG::open_file %d result=%d\n", __LINE__, result);
        if(result != FILE_OK)
        {
            return result;
        }
	}

if(debug) printf("FileFFMPEG::open_file %d result=%d\n", __LINE__, result);



#ifdef USE_FFMPEG_OUTPUT
    if(wr)
    {
    	ffmpeg_lock->lock("FileFFMPEG::open_file");
// only generating quicktime/mp4 for now
        avformat_alloc_output_context2(&ffmpeg_output, 
            NULL, 
            "mp4", 
            asset->path);
        
        int current_id = 0;
        if(asset->video_data)
	    {
            FileFFMPEGStream *new_stream = new FileFFMPEGStream;
            AVStream *ffmpeg_stream = avformat_new_stream(ffmpeg_output, NULL);
            video_streams.append(new_stream);
            new_stream->ffmpeg_id = current_id++;
            new_stream->is_video = 1;
            ffmpeg_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
            
            
        }
        
        
        if(asset->audio_data)
        {
            FileFFMPEGStream *new_stream = new FileFFMPEGStream;
            AVStream *st = avformat_new_stream(ffmpeg_output, NULL);
            video_streams.append(new_stream);
            new_stream->ffmpeg_id = current_id++;
            new_stream->is_video = 0;
            ffmpeg_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        }
        
        ffmpeg_lock->unlock();
    }
#endif // USE_FFMPEG_OUTPUT



if(debug) printf("FileFFMPEG::open_file %d result=%d\n", __LINE__, result);
	return result;
}


int FileFFMPEG::open_ffmpeg()
{
    const int debug = 0;
    int result = 0;
	AVFormatContext *ffmpeg_file_context = 0;
    need_restart = 0;
    ffmpeg_lock->lock("FileFFMPEG::open_file");

    if(debug) printf("FileFFMPEG::open_ffmpeg %d asset=%p\n", __LINE__, asset);
	result = avformat_open_input(
		&ffmpeg_file_context, 
		asset->path, 
		0,
		0);

	if(debug) printf("FileFFMPEG::open_ffmpeg %d result=%d\n", __LINE__, result);

	if(result >= 0)
	{
		if(debug) printf("FileFFMPEG::open_ffmpeg %d this=%p result=%d ffmpeg_file_context=%p\n", __LINE__, this, result, ffmpeg_file_context);
		result = avformat_find_stream_info(ffmpeg_file_context, 0);
		if(debug) printf("FileFFMPEG::open_ffmpeg %d this=%p result=%d\n", __LINE__, this, result);
	}
	else
	{
		ffmpeg_lock->unlock();
		if(debug) printf("FileFFMPEG::open_ffmpeg %d\n", __LINE__);
		return 1;
	}
	if(debug) printf("FileFFMPEG::open_ffmpeg %d result=%d\n", __LINE__, result);

	if(result >= 0)
	{
		result = 0;



// Convert format to asset & create stream objects
		asset->format = FILE_FFMPEG;
		asset->channels = 0;
		asset->audio_data = 0;

//printf("FileFFMPEG::open_ffmpeg %d streams=%d\n", __LINE__, ((AVFormatContext*)ffmpeg_file_context)->nb_streams);
		for(int i = 0; i < ((AVFormatContext*)ffmpeg_file_context)->nb_streams; i++)
		{
			AVStream *ffmpeg_stream = ((AVFormatContext*)ffmpeg_file_context)->streams[i];
#if LIBAVCODEC_VERSION_MAJOR < 58
      		AVCodecContext *decoder_context = ffmpeg_stream->codec;
#endif
        	enum AVMediaType type = ffmpeg_stream->codecpar->codec_type;
//            switch(decoder_context->codec_type) 
            switch(type) 
			{
        		case AVMEDIA_TYPE_AUDIO:
				{
//printf("FileFFMPEG::open_ffmpeg %d i=%d CODEC_TYPE_AUDIO\n", __LINE__, i);
//if(debug) printf("FileFFMPEG::open_ffmpeg %d decoder_context->codec_id=%d\n", __LINE__, decoder_context->codec_id);
#if LIBAVCODEC_VERSION_MAJOR >= 58
					AVCodecContext *decoder_context = avcodec_alloc_context3(NULL);
                    avcodec_parameters_to_context(decoder_context, ffmpeg_stream->codecpar);
#endif
                    const AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
					if(!codec)
					{
						printf("FileFFMPEG::open_ffmpeg %d: audio codec 0x%x not found.\n", 
                            __LINE__,
							decoder_context->codec_id);
#if LIBAVCODEC_VERSION_MAJOR >= 58
                        avcodec_free_context(&decoder_context);
#endif
					}
					else
					{
						FileFFMPEGStream *new_stream = new FileFFMPEGStream;
						audio_streams.append(new_stream);
                        new_stream->is_audio = 1;
						new_stream->ffmpeg_id = i;
						new_stream->channels = decoder_context->channels;


// Open a new FFMPEG file for the stream
						result = avformat_open_input(
							(AVFormatContext**)&new_stream->ffmpeg_file_context, 
							asset->path, 
							0,
							0);
						avformat_find_stream_info((AVFormatContext*)new_stream->ffmpeg_file_context, 0);
						ffmpeg_stream = ((AVFormatContext*)new_stream->ffmpeg_file_context)->streams[i];

#if LIBAVCODEC_VERSION_MAJOR < 58
						decoder_context = ffmpeg_stream->codec;
//						codec = avcodec_find_decoder(decoder_context->codec_id);
#endif
//                        ((AVFormatContext*)new_stream->ffmpeg_file_context)->seek2any = 1;
                        new_stream->decoder_context = decoder_context;
						//avcodec_thread_init(decoder_context, file->cpus);
                        decoder_context->pkt_timebase = ffmpeg_stream->time_base;
                        decoder_context->codec_id = codec->id;
                        decoder_context->thread_count = file->cpus;
#if LIBAVCODEC_VERSION_MAJOR >= 58
                        decoder_context->thread_type = FF_THREAD_SLICE;
#endif
						avcodec_open2(decoder_context, codec, 0);

                        switch(decoder_context->codec_id)
                        {
                            case AV_CODEC_ID_AAC:
                                strcpy (asset->acodec, QUICKTIME_MP4A);
                                break;
                            case AV_CODEC_ID_AC3:
                                strcpy (asset->acodec, "AC3");
                                break;
                            case AV_CODEC_ID_OPUS:
                                strcpy (asset->acodec, "OPUS");
                                break;
                            default:
                                asset->acodec[0] = 0;
                                break;
                        }

						asset->channels += new_stream->channels;
						asset->bits = BITSFLOAT;
						asset->audio_data = 1;
						asset->sample_rate = decoder_context->sample_rate;

// printf("FileFFMPEG::open_ffmpeg %d audio codec_id=%d profile=%d\n", 
// __LINE__, 
// decoder_context->codec_id,
// decoder_context->profile);

						int64_t audio_length = (int64_t)(((AVFormatContext*)new_stream->ffmpeg_file_context)->duration * 
							asset->sample_rate / 
							AV_TIME_BASE);
if(debug) printf("FileFFMPEG::open_ffmpeg %d audio_length=%lld\n", __LINE__, (long long)audio_length);
						asset->audio_length = MAX(asset->audio_length, audio_length);
                    }
            		break;
				}

        		case AVMEDIA_TYPE_VIDEO:
// printf("FileFFMPEG::open_ffmpeg %d i=%d CODEC_TYPE_VIDEO decoder_context=%p codec_id=0x%x\n", 
// __LINE__, 
// i, 
// decoder_context, 
// decoder_context->codec_id);
// only 1 video track supported for ffmpeg
            		if(video_streams.size() == 0)
					{
   						FileFFMPEGStream *new_stream = new FileFFMPEGStream;
                        if(!open_codec(new_stream, ffmpeg_file_context, i))
                        {
						    video_streams.append(new_stream);
                            AVCodecContext *decoder_context = (AVCodecContext*)new_stream->decoder_context;
						    new_stream->ffmpeg_id = i;
                            new_stream->is_video = 1;


						    asset->video_data = 1;
						    asset->layers = 1;
                            switch(decoder_context->codec_id)
                            {
                                case AV_CODEC_ID_H264:
                                    strcpy (asset->vcodec, QUICKTIME_H264);
                                    break;
                                case AV_CODEC_ID_H265:
                                    strcpy (asset->vcodec, QUICKTIME_H265);
                                    break;
                                case AV_CODEC_ID_VP9:
                                    strcpy (asset->vcodec, QUICKTIME_VP09);
                                    break;
							    case AV_CODEC_ID_VP8:
								    strcpy (asset->vcodec, QUICKTIME_VP08);
								    break;
                                default:
                                    asset->vcodec[0] = 0;
                                    break;
                            }
						    asset->width = decoder_context->width;
						    asset->height = decoder_context->height;
						    if(EQUIV(asset->frame_rate, 0))
							    asset->frame_rate = 
								    (double)ffmpeg_stream->r_frame_rate.num /
								    (double)ffmpeg_stream->r_frame_rate.den;
    // 								(double)decoder_context->time_base.den / 
    // 								decoder_context->time_base.num;
                            int64_t ffmpeg_duration = ((AVFormatContext*)new_stream->ffmpeg_file_context)->duration;
						    if(ffmpeg_duration == AV_NOPTS_VALUE)
                                asset->video_length = STILL_PHOTO_LENGTH;
                            else
                                asset->video_length = (int64_t)(ffmpeg_duration *
							        asset->frame_rate / 
							        AV_TIME_BASE);
						    asset->aspect_ratio = 
							    (double)decoder_context->sample_aspect_ratio.num / 
							    decoder_context->sample_aspect_ratio.den;
if(debug) printf("FileFFMPEG::open_ffmpeg %d decoder_context->codec_id=%d duration=%ld video_length=%ld frame_rate=%f\n", 
__LINE__, 
decoder_context->codec_id,
(long)((AVFormatContext*)new_stream->ffmpeg_file_context)->duration,
(long)asset->video_length,
asset->frame_rate);
                        }
                        else
                        {
                            delete new_stream;
                        }
					}
            		break;

        		default:
//printf("FileFFMPEG::open_ffmpeg %d i=%d codec_type=%d\n", __LINE__, i, type);
            		break;
        	}
		}


//printf("FileFFMPEG::open_ffmpeg %d\n", __LINE__);
        if(debug) printf("FileFFMPEG::open_ffmpeg %d\n", __LINE__);

// does the format need a table of contents?
// TODO: do it for all MKV by iformat name
        if(asset->video_data &&
            (!strcmp(asset->vcodec, QUICKTIME_H264) ||
            !strcmp(asset->vcodec, QUICKTIME_H265) ||
            !strcmp(asset->vcodec, QUICKTIME_VP09) ||
// all AVI files
            !strcmp(((AVFormatContext*)ffmpeg_file_context)->iformat->name, "avi")))
        {
            if(debug) printf("FileFFMPEG::open_ffmpeg %d vcodec=%s\n", __LINE__, asset->vcodec);
            result = create_toc(ffmpeg_file_context);
        }
        if(debug) printf("FileFFMPEG::open_ffmpeg %d\n", __LINE__);


		if(debug) 
		{
			printf("FileFFMPEG::open_ffmpeg %d audio_streams=%d video_streams=%d\n",
				__LINE__,
				audio_streams.size(),
				audio_streams.size());
			//asset->dump();
		}
	}
	else
	{
		ffmpeg_lock->unlock();
		if(ffmpeg_file_context)
		{
			avformat_close_input((AVFormatContext**)&ffmpeg_file_context);
		}
		return 1;
	}
    if(debug) printf("FileFFMPEG::open_ffmpeg %d\n", __LINE__);


	if(ffmpeg_file_context)
	{
		avformat_close_input((AVFormatContext**)&ffmpeg_file_context);
	}
	ffmpeg_lock->unlock();
    if(debug) printf("FileFFMPEG::open_ffmpeg %d result=%d\n", __LINE__, result);

    return result;
}

int FileFFMPEG::open_codec(FileFFMPEGStream *stream, void *ptr, int id)
{
    AVFormatContext *ffmpeg_file_context = (AVFormatContext*)ptr;
    AVStream *ffmpeg_stream = (AVStream*)((AVFormatContext*)ffmpeg_file_context)->streams[id];

#if LIBAVCODEC_VERSION_MAJOR >= 58
    AVCodecContext *decoder_context = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(decoder_context, ffmpeg_stream->codecpar);
#else
    AVCodecContext *decoder_context = ffmpeg_stream->codec;
#endif

    const AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
// Append _cuvid to the name to use hardware decoding
    if(codec && MWindow::preferences->use_hardware_decoding)
    {
        char string[BCTEXTLEN];
        sprintf(string, "%s_cuvid", codec->name);
        const AVCodec *hw_codec = avcodec_find_decoder_by_name(string);
        if(hw_codec)
        {
            codec = hw_codec;
        }
    }

	if(!codec)
	{
		printf("FileFFMPEG::open_codec %d: video codec 0x%x not found.\n", 
            __LINE__,
			decoder_context->codec_id);
#if LIBAVCODEC_VERSION_MAJOR >= 58
        avcodec_free_context(&decoder_context);
#endif
        return 1;
	}


// Open a new FFMPEG file for the stream if we're opening the codec for the 1st time
    if(!stream->ffmpeg_file_context)
    {
		int result = avformat_open_input(
			(AVFormatContext**)&stream->ffmpeg_file_context, 
			asset->path, 
			0,
			0);
	    avformat_find_stream_info((AVFormatContext*)stream->ffmpeg_file_context, 0);
    }

// initialize the codec
// replace the ffmpeg_stream with the newly created one
	ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[id];
#if LIBAVCODEC_VERSION_MAJOR < 58
    decoder_context = ffmpeg_stream->codec;
#endif
//  ((AVFormatContext*)new_stream->ffmpeg_file_context)->seek2any = 1;

    stream->decoder_context = decoder_context;
    decoder_context->pkt_timebase = ffmpeg_stream->time_base;
    decoder_context->thread_count = file->cpus;

#if LIBAVCODEC_VERSION_MAJOR >= 58
    decoder_context->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
#endif


//  decoder_context->flags2 = AV_CODEC_FLAG2_FAST;
	int result = avcodec_open2(decoder_context, codec, 0);

printf("FileFFMPEG::open_codec %d cpus=%d codec=%s id=%d\n", 
__LINE__, file->cpus, codec->name, decoder_context->codec_id);


    return 0;
}

void FileFFMPEG::close_ffmpeg()
{
	ffmpeg_lock->lock("FileFFMPEG::close_file");
	if(ffmpeg_frame) av_frame_free((AVFrame**)&ffmpeg_frame);
	audio_streams.remove_all_objects();
	video_streams.remove_all_objects();
	ffmpeg_lock->unlock();
}




int FileFFMPEG::create_toc(void *ptr)
{
    AVFormatContext *ffmpeg = (AVFormatContext*)ptr;
	string index_filename;
	string source_filename;
	string string2;
    string path_string(asset->path);
    char string3[BCTEXTLEN];
    int debug = 0;
    if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);

	IndexFile::get_toc_filename(&source_filename, 
		&file->preferences->index_directory, 
		&index_filename, 
		&path_string);
    if(debug) printf("FileFFMPEG::create_toc %d index_filename='%s'\n", 
        __LINE__, 
        index_filename.c_str());

    int need_toc = 1;
    int result = 0;
    int i, j;

#define PUT_INT32(x) \
{ \
    uint32_t temp = x; \
    fwrite(&temp, 1, 4, fd); \
}

#define PUT_INT64(x) \
{ \
    uint64_t temp = x; \
    fwrite(&temp, 1, 8, fd); \
}

#define READ_INT32(fd) \
({ \
    uint32_t temp = 0; \
    int x = fread(&temp, 1, 4, fd); \
    temp; \
})

#define READ_INT64(fd) \
({ \
    uint64_t temp = 0; \
    int x = fread(&temp, 1, 8, fd); \
    temp; \
})

// test for existing TOC
    FILE *fd = fopen(index_filename.c_str(), "r");
    int64_t creation_date = FileSystem::get_date(asset->path);
    if(fd)
    {
        int sig_len = strlen(FFMPEG_TOC_SIG);
        if(fread(string3, 1, sig_len, fd) < sig_len)
        {
            result = 1;
        }

// test start code
        string3[sig_len] = 0;
        if(result || strcmp(string3, FFMPEG_TOC_SIG))
        {
            result = 1;
        }

// test creation date
        int64_t toc_creation_date = READ_INT64(fd);
        FileSystem fs;
        if(result || toc_creation_date < creation_date)
        {
            result = 1;
        }

// skip file size
        fseek(fd, sizeof(int64_t), SEEK_CUR);

        if(!result)
        {
// load the indexes
            int toc_audio_streams = READ_INT32(fd);
            if(debug) printf("FileFFMPEG::create_toc %d reading toc_audio_streams=%d\n", __LINE__, toc_audio_streams);
            for(i = 0; i < toc_audio_streams && i < audio_streams.size() && !result; i++)
            {
                FileFFMPEGStream *stream = audio_streams.get(i);
                stream->index_zoom = READ_INT32(fd);
// this is required to skip the table
                stream->index_size = READ_INT32(fd);
// channels of index data written
                int channels = READ_INT32(fd);
                stream->delete_index();
                if(debug) printf("FileFFMPEG::create_toc %d reading index_zoom=%d index_size=%d\n", __LINE__, stream->index_zoom, stream->index_size);
// skip the table
                fseek(fd, 
                    stream->index_size * sizeof(float) * 2 * channels, 
                    SEEK_CUR);
//                 stream->index_data = new float*[stream->channels];
//                 for(j = 0; j < channels && j < stream->channels; j++)
//                 {
//                     stream->index_data[j] = new float[stream->index_size * 2];
//                     if(fread(stream->index_data[j], sizeof(float) * 2, stream->index_size, fd) < stream->index_size)
//                     {
//                         result = 1;
//                         break;
//                     }
//                 }
            }
        }
        if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);

        if(!result)
        {
// read the tables

            int toc_audio_streams = READ_INT32(fd);
            if(debug) printf("FileFFMPEG::create_toc %d reading toc_audio_streams=%d\n", __LINE__, toc_audio_streams);
            int64_t max_samples = 0;
            for(i = 0; i < toc_audio_streams && i < audio_streams.size(); i++)
            {
                FileFFMPEGStream *stream = audio_streams.get(i);
                stream->total_samples = READ_INT64(fd);
                if(stream->total_samples > max_samples)
                {
                    max_samples = stream->total_samples;
                }
                int64_t chunks = READ_INT64(fd);
                if(debug) printf("FileFFMPEG::create_toc %d reading total_samples=%ld chunks=%d\n", 
                    __LINE__, 
                    (long)stream->total_samples, 
                    (int)chunks);
                stream->audio_offsets.allocate(chunks);
                stream->audio_offsets.total = chunks;
                if(fread(stream->audio_offsets.values, sizeof(int64_t), chunks, fd) < chunks)
                {
                    result = 1;
                    break;
                }


                stream->audio_samples.allocate(chunks);
                stream->audio_samples.total = chunks;
                if(fread(stream->audio_samples.values, sizeof(int32_t), chunks, fd) < chunks)
                {
                    result = 1;
                    break;
                }

// for(j = 0; j < chunks; j++)
// {
// printf("FileFFMPEG::create_toc %d offset=%p samples=%d\n",
// __LINE__,
// stream->audio_offsets.get(j),
// stream->audio_samples.get(i));
// }
            }
            
            if(!result)
            {
// replace the estimated total samples
                asset->audio_length = max_samples;
            }
        }



        if(!result)
        {
            int toc_video_streams = READ_INT32(fd);
            if(debug) printf("FileFFMPEG::create_toc %d reading toc_video_streams=%d\n", __LINE__, toc_video_streams);
            for(i = 0; i < toc_video_streams && i < video_streams.size(); i++)
            {
                FileFFMPEGStream *stream = video_streams.get(i);
                int total_frames = READ_INT32(fd);
// replace the estimated total frames
                asset->video_length = total_frames;
                stream->video_offsets.allocate(total_frames);
                stream->video_offsets.total = total_frames;
                if(fread(stream->video_offsets.values, sizeof(int64_t), total_frames, fd) < total_frames)
                {
                    result = 1;
                    break;
                }

                int total_keyframes = READ_INT32(fd);
                stream->video_keyframes.allocate(total_keyframes);
                stream->video_keyframes.total = total_keyframes;
                if(fread(stream->video_keyframes.values, sizeof(int32_t), total_keyframes, fd) < total_keyframes)
                {
                    result = 1;
                    break;
                }


                if(debug) printf("FileFFMPEG::create_toc %d reading stream=%p total_frames=%d total_keyframes=%d\n", __LINE__, stream, total_frames, total_keyframes);
            }
            
            
        }


        if(!result)
        {
            if(debug) printf("FileFFMPEG::create_toc %d opened\n", __LINE__);
            need_toc = 0;
            has_toc = 1;
        }
        fclose(fd);
        fd = 0;
    }
    else
    {
        if(debug) printf("FileFFMPEG::create_toc %d couldn't open TOC\n", __LINE__);
    }


    if(debug) printf("FileFFMPEG::create_toc %d need_toc=%d\n", __LINE__, need_toc);


    if(need_toc)
    {
// use length codes if TOC creation is disabled
        if(file->disable_toc_creation) 
        {
            if(asset->audio_data) asset->audio_length = NOSEEK_LENGTH;
            if(asset->video_data) asset->video_length = NOSEEK_LENGTH;
            return 0;
        }
        
    
        result = 0;

        Timer prev_time;
        Timer fast_progress_time;
        Timer current_time;
        int64_t total_bytes = FileSystem::get_size(asset->path);

// make a table of ffmpeg stream ID's to FileFFMPEGStream objects
        int total_streams = audio_streams.size() + video_streams.size();
        ArrayList<FileFFMPEGStream*> stream_map;
        int i;
        int current_astream = 0;
        int current_vstream = 0;
        for(i = 0; i < ffmpeg->nb_streams; i++)
        {
            AVStream *ffmpeg_stream = ffmpeg->streams[i];
//            AVCodecContext *decoder_context = ffmpeg_stream->codec;
            enum AVMediaType type = ffmpeg_stream->codecpar->codec_type;

            if(type == AVMEDIA_TYPE_AUDIO)
            {
//printf("FileFFMPEG::create_toc %d i=%i AVMEDIA_TYPE_AUDIO\n", __LINE__, i);
                FileFFMPEGStream *dst = audio_streams.get(current_astream++);
// force it to update this
                dst->next_frame_offset = -1;
                dst->index_zoom = 1;
                dst->total_samples = 0;
                dst->delete_index();
                stream_map.append(dst);
            }
            else
            if(type == AVMEDIA_TYPE_VIDEO)
            {
//printf("FileFFMPEG::create_toc %d i=%i AVMEDIA_TYPE_VIDEO\n", __LINE__, i);
// only 1 video track supported
                if(current_vstream == 0)
                {
                    FileFFMPEGStream *dst = video_streams.get(current_vstream++);
                    dst->next_frame_offset = -1;
                    dst->is_keyframe = 0;
                    stream_map.append(dst);
                }
                else
                {
                    stream_map.append(0);
                }
            }
            else
            {
// unsupported stream
                stream_map.append(0);
            }
        }

        av_seek_frame(ffmpeg, 
			0, 
			0, 
			AVSEEK_FLAG_ANY);

        string progress_title;
        progress_title.assign("Creating ");
        progress_title.append(index_filename);
        progress_title.append("\n");
        if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);
        file->start_progress(progress_title.c_str(), total_bytes);
        if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);


        while(1)
        {
            AVPacket *packet = av_packet_alloc();
// starting offset of the packet
// offsets are aligned to whatever packet structure it is
            int64_t offset = avio_tell(ffmpeg->pb);
            int error = av_read_frame(ffmpeg, 
				packet);
            if(error)
            {
                break;
            }
            int64_t dts = packet->dts;
            if(file->progress_canceled()) 
			{
				result = FILE_USER_CANCELED;
//                printf("FileFFMPEG::create_toc %d result=%d\n", __LINE__, result);
				break;
			}

// update the progress bar            
            if(fast_progress_time.get_difference() >= 50 && offset > 0)
            {
                fast_progress_time.update();
                file->update_progress(offset);
            }

            if(packet->size > 0)
            {
//                 printf("FileFFMPEG::create_toc %d: offset=0x%lx dts=%ld size=%d stream=%d\n", 
//                     __LINE__, 
//                     offset,
//                     dts,
//                     packet->size,
//                     packet->stream_index);

                FileFFMPEGStream *stream = stream_map.get(packet->stream_index);
                if(stream)
                {
                    if(stream->is_audio)
                    {
// next samples to be decoded will come after this offset
                        if(stream->next_frame_offset < 0)
                        {
                            stream->next_frame_offset = offset;
                        }

// decode the audio samples
                        AVFrame *ffmpeg_samples = av_frame_alloc();
// we need the AVStream & AVCodecContext corresponding to 
// the FileFFMPEGStream instance of AVFormatContext
                        AVStream *ffmpeg_stream = 
                            ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->ffmpeg_id];

                        AVCodecContext *decoder_context = (AVCodecContext*)stream->decoder_context;
//printf("FileFFMPEG::create_toc %d decoder_context=%p\n", __LINE__, decoder_context);
                        int result = avcodec_send_packet(decoder_context, packet);
// store interleaved audio samples in a temporary
                        const int MAX_SAMPLES = 4096;
                        float temp_audio[MAX_SAMPLES * MAX_CHANNELS];
                        int samples_decoded = 0;
                        int channels = 0;

                        while(result >= 0)
                        {
                            result = avcodec_receive_frame(decoder_context, 
                                ffmpeg_samples);
                            if(result >= 0)
                            {
//                                 printf("FileFFMPEG::create_toc %d: audio offset=0x%lx size=%d dts=%ld samples=%d\n", 
//                                     __LINE__,
//                                     stream->next_frame_offset,
//                                     packet->size,
//                                     ffmpeg_samples->pkt_dts,
//                                     ffmpeg_samples->nb_samples);

// Store in temp buffer.  
// Assume number of channels doesn't change in a single send_packet
                                channels = ffmpeg_samples->channels;
                                for(j = 0; j < ffmpeg_samples->nb_samples; j++)
                                {
                                    for(i = 0; i < channels; i++)
                                    {
                                        float value = 0;
                                        switch(ffmpeg_samples->format)
                                        {
                                            case AV_SAMPLE_FMT_S16:
                                            {
                                                int16_t *input = (int16_t*)ffmpeg_samples->data[0];
                                                value = (float)input[j * channels + i] / 32767;
                                                break;
                                            }

                                            case AV_SAMPLE_FMT_S16P:
			                                {
                                                int16_t *input = (int16_t*)ffmpeg_samples->data[i];
                                                value = (float)input[j] / 32767;
                                            }
                                            break;

                                            case AV_SAMPLE_FMT_FLTP:
			                                {
                                                float *input = (float*)ffmpeg_samples->data[i];
                                                value = input[j];
                                            }
                                            break;

                                            case AV_SAMPLE_FMT_S32P:
                                            {
                                                int32_t *input = (int32_t*)ffmpeg_samples->data[i];
                                                value = (float)input[j] / 0x7fffffff;
                                            }
                                            break;

                                            default:
				                                printf("FileFFMPEGStream::append_index %d: unsupported audio format %d\n", 
					                                __LINE__,
					                                ffmpeg_samples->format);
				                                break;
                                        }

                                        temp_audio[(samples_decoded + j) * channels + 
                                            i] = value;
                                    }
                                }

                                samples_decoded += ffmpeg_samples->nb_samples;
                            }
                        }

                        if(samples_decoded > 0)
                        {
//                             printf("FileFFMPEG::create_toc %d: audio offset=0x%lx size=%d flags=0x%x samples=%d\n", 
//                                 __LINE__,
//                                 stream->next_frame_offset,
//                                 packet->size,
//                                 packet->flags,
//                                 samples_decoded);
                            stream->audio_offsets.append(stream->next_frame_offset);
                            stream->audio_samples.append(samples_decoded);
                            stream->total_samples += samples_decoded;
// next samples to be decoded will come after the next offset
                            stream->next_frame_offset = -1;


//printf("FileFFMPEG::create_toc %d samples_decoded=%d\n", __LINE__, samples_decoded);
                            stream->append_index(temp_audio, 
                                samples_decoded,
                                channels,
                                asset, 
                                file->preferences);
//printf("FileFFMPEG::create_toc %d\n", __LINE__);
                        }


                        av_frame_free(&ffmpeg_samples);
                    }
                    else
                    {
// get the keyframes from video
//                         printf("FileFFMPEG::create_toc %d: video offset=0x%lx size=%d flags=0x%x\n", 
//                             __LINE__,
//                             offset,
//                             packet->size,
//                             packet->flags);


// next frame to be decoded will come after this offset
                        if(stream->next_frame_offset < 0)
                        {
                            stream->next_frame_offset = offset;
                        }

                        if(packet->flags)
                        {
                            stream->is_keyframe = 1;
                        }
                        
// fudge for different codecs
                        int got_frame = 1;


#if LIBAVCODEC_VERSION_MAJOR < 58
                        if(!strcmp(asset->vcodec, QUICKTIME_VP09))
                        {
// VP9 skips some.  0x84 doesn't denote this is a skipped frame, 
// but a frame somewhere else is skipped for every 0x84 code & no more than 1
// 0x84 occurs for every skipped frame.
                            if(packet->data[0] == 0x84)
                            {
                                got_frame = 0;
                            }
                        }
#endif


// store a frame
                        if(got_frame)
                        {
                            if(stream->is_keyframe)
                            {
                                stream->video_keyframes.append(stream->video_offsets.size());
                            }
                            stream->video_offsets.append(stream->next_frame_offset);

                            stream->next_frame_offset = -1;
                            stream->is_keyframe = 0;
                            if(debug) printf("FileFFMPEG::create_toc %d: total frames=%d\n", 
                                __LINE__,
                                stream->video_offsets.size());
                        }

                    }
                }
                
                
            }
            
            
            av_packet_free(&packet);
        }


        if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);
        file->stop_progress("done creating table of contents");
        if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);

        av_seek_frame(ffmpeg, 
			0, 
			0, 
			AVSEEK_FLAG_ANY);

//printf("FileFFMPEG::create_toc %d result=%d %s\n", __LINE__, result, index_filename.c_str());

// write the last incomplete high/low pairs to the indexes
        if(!result)
        {
            for(i = 0; i < audio_streams.size(); i++)
            {
                audio_streams.get(i)->flush_index();
            }
        
        
// write the index file
            fd = fopen(index_filename.c_str(), "w");
            if(!fd)
            {
                printf("FileFFMPEG::create_toc %d: can't open \"%s\".  %s\n",
			        __LINE__,
                    index_filename.c_str(),
			        strerror(errno));
                result = 1;
            }
        }
        
        

        if(!result)
        {
            fwrite(FFMPEG_TOC_SIG, strlen(FFMPEG_TOC_SIG), 1, fd);

// store the date of the source file
            PUT_INT64(creation_date);


// store the size of the source file to handle removable media
            PUT_INT64(total_bytes);

// put the audio indexes first so they can be drawn quickly
            PUT_INT32(audio_streams.size());
            for(i = 0; i < audio_streams.size(); i++)
            {
                FileFFMPEGStream *stream = audio_streams.get(i);
                PUT_INT32(stream->index_zoom);
                PUT_INT32(stream->index_size);
                PUT_INT32(stream->channels);
                if(debug) printf("FileFFMPEG::create_toc %d writing index_zoom=%d index_size=%d\n", __LINE__, stream->index_zoom, stream->index_size);
                if(stream->index_size > 0)
                {
// write the channels sequentially
                    for(j = 0; j < stream->channels; j++)
                    {
                        fwrite(stream->index_data[j], 
                            sizeof(float) * 2,
                            stream->index_size,
                            fd);
                    }
                }
            }
        }

// then come the audio chunks
        if(!result)
        {
            PUT_INT32(audio_streams.size());
            int64_t max_samples = 0;
            for(i = 0; i < audio_streams.size(); i++)
            {
                FileFFMPEGStream *stream = audio_streams.get(i);
                PUT_INT64(stream->total_samples);
                if(stream->total_samples > max_samples)
                {
                    max_samples = stream->total_samples;
                }
// total number of chunks detected
                int64_t chunks = stream->audio_offsets.size();
                PUT_INT64(chunks);
                if(fwrite(stream->audio_offsets.values, sizeof(int64_t), chunks, fd) < chunks)
                {
                    result = 1;
                    break;
                }

// samples in each chunk
                if(fwrite(stream->audio_samples.values, sizeof(int32_t), chunks, fd) < chunks)
                {
                    result = 1;
                    break;
                }
                if(debug) printf("FileFFMPEG::create_toc %d writing total_samples=%d chunks=%ld\n", __LINE__, stream->total_samples, chunks);
            }
// replace the estimated total samples
            asset->audio_length = max_samples;
        }

        if(!result)
        {
            PUT_INT32(video_streams.size());
            for(i = 0; i < video_streams.size(); i++)
            {
                FileFFMPEGStream *stream = video_streams.get(i);
                int total_frames = stream->video_offsets.size();
// total number of frames detected
                PUT_INT32(total_frames);
                if(fwrite(stream->video_offsets.values, sizeof(int64_t), total_frames, fd) < total_frames)
                {
                    result = 1;
                    break;
                }
// number of each keyframe
                int total_keyframes = stream->video_keyframes.size();
                PUT_INT32(total_keyframes);
                if(fwrite(stream->video_keyframes.values, sizeof(int32_t), total_keyframes, fd) < total_keyframes)
                {
                    result = 1;
                    break;
                }
                if(debug) printf("FileFFMPEG::create_toc %d writing total_frames=%d total_keyframes=%d\n", __LINE__, total_frames, total_keyframes);

                asset->video_length = total_frames;
            }
        }


        if(!result)
        {
            has_toc = 1;
        }
        
        if(fd)
        {
            fclose(fd);
// set the creation date to the file's creation date to handle future times.
// breaks index file deletion
//            FileSystem::set_date(index_filename.c_str(), creation_date);
        }
    }
    
    
    if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);


    return result;
}



int FileFFMPEG::read_index_state(FILE *fd, Indexable *dst)
{
    char string[BCTEXTLEN];
    int i, j;
    int debug = 0;
    IndexState *index_state = dst->index_state;

// test signature
    fseek(fd, 0, SEEK_SET);
    int sig_len = strlen(FFMPEG_TOC_SIG);
    if(fread(string, 1, sig_len, fd) < sig_len)
    {
//        printf("FileFFMPEG::read_index_state %d: failed to read the TOC\n", __LINE__);
        return 1;
    }

    string[sig_len] = 0;
// not a TOC belonging to this format
    if(strcmp(string, FFMPEG_TOC_SIG))
    {
//        printf("FileFFMPEG::read_index_state %d: failed to read the TOC\n", __LINE__);
        return 1;
    }
    
// the source date
    READ_INT64(fd);

    index_state->index_bytes = READ_INT64(fd);

// offsets of the tables in bytes
    ArrayList<int> offsets;
// sizes of the tables in floats
    ArrayList<int> sizes;

    int toc_audio_streams = READ_INT32(fd);
    for(i = 0; i < toc_audio_streams; i++)
    {
// the same for all audio streams
        index_state->index_zoom = READ_INT32(fd);
// number of high/low pairs per channel in this stream
        int index_size = READ_INT32(fd);
// number of channels in this stream
        int channels = READ_INT32(fd);
        if(debug) printf("FileFFMPEG::read_index_state %d: index_zoom=%d index_size=%d channels=%d\n",
            __LINE__,
            (int)index_state->index_zoom,
            (int)index_size,
            (int)channels);
// store the offsets & sizes of the index tables
        for(j = 0; j < channels ; j++)
        {
            offsets.append(ftell(fd));
            sizes.append(index_size * 2);
            if(debug) printf("FileFFMPEG::read_index_state %d: offset=%ld size=%d\n",
                __LINE__,
                ftell(fd),
                index_size * 2);
// skip the table to get to the next stream
            fseek(fd, 
                index_size * sizeof(float) * 2, 
                SEEK_CUR);
        }
    }

// offsets are used instead of this when drawing from a TOC
    index_state->index_start = 0;
// copy the offsets & sizes to the index state
    delete [] index_state->index_offsets;
    delete [] index_state->index_sizes;
    index_state->channels = offsets.size();
    index_state->index_offsets = new int64_t[offsets.size()];
    index_state->index_sizes = new int64_t[offsets.size()];
    for(i = 0; i < offsets.size(); i++)
    {
        index_state->index_offsets[i] = offsets.get(i);
        index_state->index_sizes[i] = sizes.get(i);
    }
    
    return 0;
}
            
            



int FileFFMPEG::close_file()
{
	const int debug = 0;
    close_ffmpeg();


#ifdef USE_FFMPEG_OUTPUT
    if(ffmpeg_output)
    {
	    ffmpeg_lock->lock("FileFFMPEG::close_file");
        avformat_free_context(ffmpeg_output);
	    ffmpeg_lock->unlock();
    }
#endif



	reset();
    return 0;
}


int64_t FileFFMPEG::get_memory_usage()
{
// estimate the frame size, to avoid a complicated color space dependent calculation
    if(ffmpeg_frame)
    {
        return asset->width * asset->height * 3;
    }
	return 0;
}


// int FileFFMPEG::colormodel_supported(int colormodel)
// {
// 	return colormodel;
// }

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


int FileFFMPEG::get_seek_stream()
{
    if(video_streams.size() > 0)
    {
        FileFFMPEGStream *stream = video_streams.get(0);
        return stream->ffmpeg_id;
    }
    else
    {
        return 0;
    }
}

// seeking in ffmpeg-5.1 requires reading backwards with av_seek_frame
// then reading forwards with av_read_frame to the desired byte offset
int FileFFMPEG::seek_5(FileFFMPEGStream *stream, 
    int chunk, 
    double seconds)
{
    AVStream *ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->ffmpeg_id];
	AVCodecContext *decoder_context = (AVCodecContext*)stream->decoder_context;
    ArrayList<int64_t> *offsets;
    int64_t timestamp = (int64_t)(seconds * 
        ffmpeg_stream->time_base.den /
        ffmpeg_stream->time_base.num);
    int need_seek = 1;

// don't do anything if rewinding
    if(chunk == 0)
    {
        av_seek_frame((AVFormatContext*)stream->ffmpeg_file_context, 
            stream->ffmpeg_id, 
            0,
            SEEK_SET | AVSEEK_FLAG_BACKWARD);
        return 0;
    }

    if(stream->is_audio)
    {
        offsets = &stream->audio_offsets;
        int64_t want_offset = offsets->get(chunk);
        
        if(video_streams.size() > 0)
        {
// if it's an audio track, seek to the nearest keyframe in
// the video track to work around a bug
// It seems to lock up if the timestamp isn't on a video keyframe.
            FileFFMPEGStream *vstream = video_streams.get(0);
            AVStream *ffmpeg_vstream = ((AVFormatContext*)vstream->ffmpeg_file_context)->streams[vstream->ffmpeg_id];
            int keyframe = -1;
            int64_t offset = -1;
            int got_it = 0;
            for(int i = vstream->video_keyframes.size() - 1; 
                i >= 0; i--)
            {
                keyframe = vstream->video_keyframes.get(i);
                offset = vstream->video_offsets.get(keyframe);

// take the video keyframe before the desired byte offset
                if(offset <= want_offset)
                {
                    got_it = 1;
                    break;
                }
            }

// default to the start of the file if no video keyframe comes before 
// the audio position
            if(!got_it)
            {
                keyframe = 0;
                got_it = 1;
            }

            if(got_it)
            {
                int64_t timestamp2 = (int64_t)keyframe * 
                    ffmpeg_vstream->time_base.den /
                    ffmpeg_vstream->time_base.num /
                    asset->frame_rate;
//printf("FileFFMPEG::seek_5 %d\n", __LINE__);

// Seek the audio stream's file context using the video stream ID so the
// video timestamp can be found.
                av_seek_frame((AVFormatContext*)stream->ffmpeg_file_context, 
                    vstream->ffmpeg_id, 
                    timestamp2,
                    SEEK_SET | AVSEEK_FLAG_BACKWARD);
//printf("FileFFMPEG::seek_5 %d\n", __LINE__);
                need_seek = 0;
            }
        }
    }
    else
    if(stream->is_video)
    {
        offsets = &stream->video_offsets;
        
        
    }


//printf("FileFFMPEG::seek_5 %d\n", __LINE__);
    if(need_seek)
    {
// this seeks to a certain point before the desired timestamp but not after it
        av_seek_frame((AVFormatContext*)stream->ffmpeg_file_context, 
            stream->ffmpeg_id, 
            timestamp,
            SEEK_SET | AVSEEK_FLAG_BACKWARD);
    }

// printf("FileFFMPEG::seek_5 %d got=%ld want=%ld\n", 
// __LINE__,
// avio_tell(((AVFormatContext*)stream->ffmpeg_file_context)->pb),
// offsets->get(chunk));
// read up to the desired position so we don't spend all day decoding
// hundreds of frames.
// The 1st read always skips many chunks
    while(avio_tell(((AVFormatContext*)stream->ffmpeg_file_context)->pb) < offsets->get(chunk))
    {
        AVPacket *packet = av_packet_alloc();
       	int result = av_read_frame((AVFormatContext*)stream->ffmpeg_file_context, packet);
        av_packet_free(&packet);
    }

//printf("FileFFMPEG::seek_5 %d\n", __LINE__);

    int64_t real_offset = avio_tell(((AVFormatContext*)stream->ffmpeg_file_context)->pb);
    int real_chunk = 0;

    avcodec_flush_buffers(decoder_context);

// Find the true chunk we're on
//     for(int i = offsets->size() - 1; i >= 0; i--)
//     {
//         if(offsets->get(i) <= real_offset)
//         {
//             real_chunk = i;
//             break;
//         }
//     }

// Must read forward since multiple chunks seem to have the same offset.  It might
// be multiple output frames coming from a single input packet.
    for(int i = 0; i < offsets->size(); i++)
    {
        if(offsets->get(i) >= real_offset)
        {
            real_chunk = i;
            break;
        }
    }

//     printf("FileFFMPEG::seek_5 %d want offset=%ld got offset=%ld want chunk=%d got chunk=%d\n",
//         __LINE__,
//         offsets->get(chunk),
//         real_offset,
//         chunk,
//         real_chunk);
    return real_chunk;
}

static int ffmpeg_to_cmodel(int pix_fmt)
{
	switch(pix_fmt)
	{
		case AV_PIX_FMT_YUV420P10LE:
			return BC_YUV420P10LE;
			break;

		case AV_PIX_FMT_YUV420P:
			return BC_YUV420P;
			break;
#ifndef FFMPEG_2010
		case AV_PIX_FMT_YUV422:
			return BC_YUV422;
			break;
#endif

		case AV_PIX_FMT_YUV422P:
			return BC_YUV422P;
			break;
		case AV_PIX_FMT_YUV410P:
			return BC_YUV9P;
			break;

        case AV_PIX_FMT_YUV411P:
            return BC_YUV411P;
            break;

        case AV_PIX_FMT_NV12:
			return BC_NV12;
//                 printf("FileFFMPEG::ffmpeg_to_cmodel %d: AV_PIX_FMT_NV12 -> %d\n", 
//                     __LINE__,
//                     frame->get_color_model());
            break;

// webp requires convert_unsupported()
        case AV_PIX_FMT_ARGB:
            return BC_RGBA8888;
            break;

//             case AV_PIX_FMT_NV21:
// 				return BC_NV21;
//                 printf("FileFFMPEG::ffmpeg_to_cmodel %d: AV_PIX_FMT_NV21\n", __LINE__);
//                 break;

		default:
			fprintf(stderr, 
				"FileFFMPEG::ffmpeg_to_cmodel %d: unrecognized color model %d\n", 
                __LINE__,
				pix_fmt);
//printf("AV_PIX_FMT_P010LE=%d\n", AV_PIX_FMT_P010LE);
			return BC_YUV420P;
			break;
	}

}


// convert unsupported colormodels to result of ffmpeg_to_cmodel
static void convert_unsupported(AVFrame *ffmpeg_frame, 
    int dst_cmodel, 
    int ffmpeg_cmodel,
    int w,
    int h)
{
    switch(ffmpeg_cmodel)
    {
        case AV_PIX_FMT_ARGB:
// convert to RGBA8888
            for(int i = 0; i < h; i++)
            {
                uint8_t *row = ffmpeg_frame->data[0] + i * ffmpeg_frame->linesize[0];
                for(int j = 0; j < w; j++)
                {
                    uint8_t r = row[1];
                    uint8_t g = row[2];
                    uint8_t b = row[3];
                    uint8_t a = row[0];
                    row[0] = r;
                    row[1] = g;
                    row[2] = b;
                    row[3] = a;
                    row += 4;
                }
            }
            break;
    }
    
}

int FileFFMPEG::read_frame(VFrame *frame)
{
	int error = 0;
	const int debug = 0;

	ffmpeg_lock->lock("FileFFMPEG::read_frame 1");
	
	
	FileFFMPEGStream *stream = video_streams.get(0);
	if(debug) printf("FileFFMPEG::read_frame %d stream=%p stream->ffmpeg_file_contex=%p\n", 
		__LINE__, 
		stream,
		stream->ffmpeg_file_context);
	AVStream *ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->ffmpeg_id];
	AVCodecContext *decoder_context = (AVCodecContext*)stream->decoder_context;


    if(debug) printf("FileFFMPEG::read_frame %d file->current_frame=%ld\n", __LINE__, file->current_frame);


// seek if reading ahead this many frames
#define SEEK_THRESHOLD 16
// Faster for HD timelapse
//#define SEEK_THRESHOLD 300

// printf("FileFFMPEG::read_frame %d current_frame=%lld file->current_frame=%lld\n", 
// __LINE__, 
// current_frame,
// file->current_frame);
	if(!ffmpeg_frame)
    {
        ffmpeg_frame = av_frame_alloc();
    }

	if(stream->current_frame != file->current_frame &&
		(file->current_frame < stream->current_frame ||
		file->current_frame > stream->current_frame + SEEK_THRESHOLD))
	{
		if(debug) printf("FileFFMPEG::read_frame %d stream->current_frame=%lld file->current_frame=%lld\n", 
		    __LINE__, 
		    (long long)stream->current_frame, 
		    (long long)file->current_frame);

        if(need_restart)
        {
            ffmpeg_lock->unlock();
            close_ffmpeg();
            open_ffmpeg();
	        ffmpeg_lock->lock("FileFFMPEG::read_frame 2");

	        stream = video_streams.get(0);
	        ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->ffmpeg_id];
	        decoder_context = (AVCodecContext*)stream->decoder_context;
        }
//printf("FileFFMPEG::read_frame %d\n", __LINE__);

// Want to seek to the nearest keyframe and read up to the current frame
// but ffmpeg seeks to the next keyframe.
// The best workaround was basing all the seeking on the video stream.

        if(!has_toc)
        {
            AVStream *seek_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[get_seek_stream()];

		    int64_t timestamp = (int64_t)((double)file->current_frame * 
			    seek_stream->time_base.den /
			    seek_stream->time_base.num /
			    asset->frame_rate);
		    av_seek_frame((AVFormatContext*)stream->ffmpeg_file_context, 
			    get_seek_stream(), 
			    timestamp, 
			    AVSEEK_FLAG_ANY);
		    stream->current_frame = file->current_frame - 1;
        }
        else
        {

// seek based on the TOC
            int i;
            int total_keyframes = stream->video_keyframes.size();
            if(debug) printf("FileFFMPEG::read_frame %d: stream=%p total_keyframes=%d video_offsets=%d\n", 
                __LINE__, 
                stream, 
                total_keyframes, 
                stream->video_offsets.size());
            
// rewind this many keyframes
            int rewind_count = VIDEO_REWIND_KEYFRAMES;
// Some codecs only require rewinding 1 keyframe
            if(decoder_context->codec_id == AV_CODEC_ID_VP9)
            {
                rewind_count = VP9_REWIND_KEYFRAMES;
            }

            if(debug) printf("FileFFMPEG::read_frame %d total_keyframes=%d\n", 
                __LINE__, 
                total_keyframes);

            int got_it = 0;
            for(i = 0; i < total_keyframes; i++)
            {
                if(stream->video_keyframes.get(i) > file->current_frame)
                {
                    got_it = 1;
// rewind the required number of keyframes
                    i -= rewind_count;
                    if(i < 0)
                    {
                        i = 0;
                    }
                    break;
                }
            }
//            if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);

// no keyframe found.  Go rewind_count before the last keyframe.
            if(!got_it)
            {
                i = total_keyframes - 1 - rewind_count;
                if(i < 0)
                {
                    i = 0;
                }
            }

            int keyframe = 0;
            if(stream->video_keyframes.size())
                keyframe = stream->video_keyframes.get(i);



#if LIBAVCODEC_VERSION_MAJOR >= 58
            if(keyframe > 0) keyframe--;
            keyframe = seek_5(stream, 
                keyframe, 
                (double)keyframe / asset->frame_rate);
// restart decoder for this case.  Might be easier to close_ffmpeg
            if(keyframe == 0)
            {
                avcodec_free_context(&decoder_context);
                open_codec(stream, stream->ffmpeg_file_context, stream->ffmpeg_id);
    	        decoder_context = (AVCodecContext*)stream->decoder_context;
            }
#else
            int64_t offset = stream->video_offsets.get(keyframe);
            avio_seek(((AVFormatContext*)stream->ffmpeg_file_context)->pb, 
                offset, 
                SEEK_SET);
            avcodec_flush_buffers(decoder_context);
#endif

            if(debug) printf("FileFFMPEG::read_frame %d context=%p offset=%ld keyframe=%d file->current_frame=%ld\n", 
                __LINE__, 
                stream->ffmpeg_file_context,
                (long)stream->video_offsets.get(keyframe),
                keyframe,
                file->current_frame);

		    stream->current_frame = keyframe;
// want 1st frame in the stream
            if(stream->current_frame == 0 &&
                file->current_frame == 0)
            {
                stream->current_frame = -1;
            }
        }

        got_frame = 0;
        last_pts = -1;
	}
// if(debug) printf("FileFFMPEG::read_frame %d stream->current_frame=%ld file->current_frame=%ld error=%d\n", 
// __LINE__,
// stream->current_frame,
// file->current_frame,
// error);




// Read frames until we catch up to the current position.
// 	if(current_frame >= file->current_frame - SEEK_THRESHOLD &&
// 		current_frame < file->current_frame - 1)
// 	{
// 		printf("FileFFMPEG::read_frame %d current_frame=%lld file->current_frame=%lld\n", 
// 			__LINE__,
// 			current_frame,
// 			file->current_frame);
// 	}


// match decoded frames with sent packets by DTS
// DTS codes are discontiguous so we can't buffer the entire file
    ArrayList<int64_t> dts_history;
// starting frame in the DTS history
    int dts_frame0 = stream->current_frame;
	while(stream->current_frame < file->current_frame && 
        !error)
	{
		got_frame = 0;
//         printf("FileFFMPEG::read_frame %d current_frame=%ld want_frame=%ld\n", 
//             __LINE__,
//             (long)stream->current_frame,
//             (long)file->current_frame);

		AVPacket *packet = av_packet_alloc();
        AVFrame *input_frame = (AVFrame*)ffmpeg_frame;

// check for a decoded frame from previous packets
// There's no way to peek at the buffer size before popping a frame.
#if LIBAVCODEC_VERSION_MAJOR >= 58
        int result = avcodec_receive_frame(decoder_context, input_frame);
        if(input_frame->data[0] && result >= 0)
            got_frame = 1;
#endif

// no frame decoded.  Read in a new packet
        if(!got_frame)
        {
		    error = av_read_frame((AVFormatContext*)stream->ffmpeg_file_context, 
			    packet);

            if(error)
            {
                printf("FileFFMPEG::read_frame %d error=%c%c%c%c offset=%ld stream->current_frame=%ld file->current_frame=%ld\n",
    		        __LINE__,
                    (-error) & 0xff,
                    ((-error) >> 8) & 0xff,
                    ((-error) >> 16) & 0xff,
                    ((-error) >> 24) & 0xff,
                    (long)avio_tell(((AVFormatContext*)stream->ffmpeg_file_context)->pb),
                    stream->current_frame,
                    file->current_frame);

// give up & reopen the ffmpeg objects.
// Still have frames buffered in the decoder, so reopen in the next seek.
// For ffmpeg 5.1 this no longer works & it only recovers if it doesn't restart.

#if LIBAVCODEC_VERSION_MAJOR < 58
                need_restart = 1;
#endif
            }
            else
            {
#if LIBAVCODEC_VERSION_MAJOR < 58
                av_packet_merge_side_data(packet);
#endif
            }


// printf("FileFFMPEG::read_frame %d error=%d want stream=%d got=%d\n", 
// __LINE__,
// error, 
// stream->ffmpeg_id,
// packet->stream_index);

			if(!error && 
                packet->size > 0 && 
                packet->stream_index == stream->ffmpeg_id)
			{
				int got_picture = 0;

//                 printf("FileFFMPEG::read_frame %d size=%d dts=%ld flags=0x%x\n", 
//                     __LINE__,
//                     packet->size,
//                     packet->dts,
//                     packet->flags);
                dts_history.append(packet->dts);


#if LIBAVCODEC_VERSION_MAJOR >= 58
                result = avcodec_send_packet(decoder_context, packet);
                if(debug) printf("FileFFMPEG::read_frame %d result=%d\n", 
                    __LINE__,
                    result);


                if(result >= 0)
                {
                    result = avcodec_receive_frame(decoder_context, input_frame);
//                     printf("FileFFMPEG::read_frame %d result=%d\n", 
//                         __LINE__,
//                         result);
                    if(result >= 0)
                        got_picture = 1;
                }
#else
		        int result = avcodec_decode_video2(
					decoder_context,
                    input_frame, 
					&got_picture,
                    packet);
#endif

				if(input_frame->data[0] && got_picture) 
                {
                    got_frame = 1;
                }
			}
            else
            if(error)
            {
// at the end of the file, we still have frames buffered in the decoder
                int got_picture = 0;


#if LIBAVCODEC_VERSION_MAJOR >= 58
// send an empty packet to flush the decoder
                int result = avcodec_send_packet(decoder_context, 0);
                if(result >= 0)
                {
                    result = avcodec_receive_frame(decoder_context, input_frame);
//                     printf("FileFFMPEG::read_frame %d result=%d\n", 
//                         __LINE__);
                    if(result >= 0)
                    {
                        got_picture = 1;
                    }
                }
#else
                int result = avcodec_decode_video2(
					decoder_context,
                    input_frame, 
					&got_picture,
                    packet);
#endif

				if(input_frame->data[0] && got_picture) 
                {
                    got_frame = 1;
                    error = 0;
                }
            }
		}

		av_packet_free(&packet);


		if(got_frame)
        {
// determine the current frame by looking up DTS code in packet history
            int got_it = 0;
            int i = -1;
// hardware decoders don't generate a DTS so we're screwed
            if(input_frame->pkt_dts != AV_NOPTS_VALUE)
            {
                for(i = 0; i < dts_history.size(); i++)
                {
                    if(input_frame->pkt_dts == dts_history.get(i))
                    {
                        stream->current_frame = dts_frame0 + i + 1;
                        got_it = 1;
                        break;
                    }
                }
            }

// didn't find a DTS so increment 1
            if(!got_it)
            {
                stream->current_frame++;
            }
//             printf("FileFFMPEG::read_frame %d got_it=%d dts_frame0=%d i=%d current_frame=%ld dts=0x%ld\n", 
//                 __LINE__, 
//                 got_it, 
//                 dts_frame0,
//                 i,
//                 stream->current_frame,
//                 input_frame->pkt_dts);

// store it in the cache if not returning it
            if(stream->current_frame < file->current_frame)
            {
                if(file->use_cache)
                {
		            AVFrame *input_frame = (AVFrame*)ffmpeg_frame;
		            int input_cmodel = ffmpeg_to_cmodel(decoder_context->pix_fmt);
                    VFrame *src = new VFrame;
// store it in a wrapper
                    src->reallocate(input_frame->data[0], 
	                    -1, // shmid
	                    input_frame->data[0],
	                    input_frame->data[1],
	                    input_frame->data[2],
	                    decoder_context->width, 
	                    decoder_context->height, 
	                    input_cmodel, 
	                    input_frame->linesize[0]);
// convert unsupported colormodels
                    convert_unsupported(input_frame, 
                        input_cmodel, 
                        decoder_context->pix_fmt,
                        decoder_context->width,
                        decoder_context->height);
                    file->get_frame_cache()->put_frame(
                        src,
		                stream->current_frame,
		                file->current_layer,
		                asset->frame_rate,
		                1, // use_copy
		                0);
                    delete src;
//printf("FileFFMPEG::read_frame %d %p %ld\n", 
//__LINE__, file->get_frame_cache(), file->get_frame_cache()->get_memory_usage());
                }
            }
        }
	}

// printf("FileFFMPEG::read_frame %d stream->current_frame=%lld file->current_frame=%lld got_it=%d\n", 
// __LINE__, 
// stream->current_frame,
// file->current_frame,
// got_frame);


// point the file class to the output
	if(got_frame)
	{
		AVFrame *input_frame = (AVFrame*)ffmpeg_frame;
		int input_cmodel = ffmpeg_to_cmodel(decoder_context->pix_fmt);

//         printf("FileFFMPEG::read_frame %d pix_fmt=%d\n", 
//             __LINE__, 
//             decoder_context->pix_fmt);



// File class will copy from the read pointer to the argument frame
// convert unsupported colormodels
        convert_unsupported(input_frame, 
            input_cmodel, 
            decoder_context->pix_fmt,
            decoder_context->width,
            decoder_context->height);
        file->set_read_pointer(input_cmodel, 
            input_frame->data[0], 
            input_frame->data[0], 
            input_frame->data[1], 
            input_frame->data[2],
            input_frame->linesize[0],
            decoder_context->width,
            decoder_context->height);
// printf("FileFFMPEG::read_frame %d u_offset=%d v_offset=%d\n", 
// __LINE__,
// (int)(input_frame->data[1] - input_frame->data[0]),
// (int)(input_frame->data[2] - input_frame->data[0]));
 	}
//PRINT_TRACE


	ffmpeg_lock->unlock();
	if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);
	return error;
}

int FileFFMPEG::read_samples(double *buffer, int64_t len)
{
	const int debug = 0;
	int error = 0;
	ffmpeg_lock->lock("FileFFMPEG::read_samples");


//printf("FileFFMPEG::read_samples %d\n", __LINE__);
// Compute stream & stream channel from global channel
	int audio_channel = file->current_channel;
	int audio_index = -1;
	FileFFMPEGStream *stream = 0;
	for(int i = 0; i < audio_streams.size(); i++)
	{
		if(audio_channel < audio_streams.get(i)->channels)
		{
			stream = audio_streams.get(i);
			audio_index = stream->ffmpeg_id;
			break;
		}
		audio_channel -= audio_streams.get(i)->channels;
	}

	AVStream *ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->ffmpeg_id];
	AVCodecContext *decoder_context = (AVCodecContext*)stream->decoder_context;




// printf("FileFFMPEG::read_samples %d: want=%ld-%ld history=%ld-%ld\n", 
// __LINE__, 
// file->current_sample,
// file->current_sample + len,
// stream->history_start,
// stream->history_start + stream->history_size);

// Seek occurred
	if(file->current_sample < stream->history_start ||
        file->current_sample > stream->history_start + stream->history_size)
	{
        if(!has_toc)
        {

// printf("FileFFMPEG::read_samples %d: %d %d\n", 
// __LINE__, 
// ffmpeg_stream->time_base.num,
// ffmpeg_stream->time_base.den);

// seeking based on a common track
            AVStream *seek_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[get_seek_stream()];
// printf("FileFFMPEG::read_samples %d want pts=%ld\n",
// __LINE__,
// timestamp);

		    int64_t timestamp = (int64_t)((double)file->current_sample * 
			    seek_stream->time_base.den /
			    seek_stream->time_base.num /
			    asset->sample_rate);
		    av_seek_frame((AVFormatContext*)stream->ffmpeg_file_context, 
			    get_seek_stream(), 
			    timestamp, 
			    AVSEEK_FLAG_ANY);
            stream->update_pcm_history(file->current_sample);
        }
        else
        {
// seeking based on the TOC
            int chunks = stream->audio_samples.size();
            int chunk = 0;
            int64_t aligned_sample = 0;
            int got_it = 0;
// rewind a certain amount before the desired sample for the decoder to warm up
            int64_t start_sample = file->current_sample - 
                asset->sample_rate * AUDIO_REWIND_SECS;
            if(start_sample < 0)
            {
                start_sample = 0;
            }



            while(chunk < chunks)
            {
// add the current chunk
                aligned_sample += stream->audio_samples.get(chunk);
// printf("FileFFMPEG::read_samples %d: chunk=%d aligned_sample=%ld\n",
// __LINE__,
// chunk,
// aligned_sample);



                if(aligned_sample > start_sample)
                {
// rewind the current chunk
                    aligned_sample -= stream->audio_samples.get(chunk);
                    if(chunk > 0) chunk--;
                    got_it = 1;
                    break;
                }
                chunk++;
            }

            if(!got_it)
            {
                chunk = chunks - 1;
                aligned_sample = stream->total_samples;
            }
// byte offset of the chunk
            int64_t audio_offset = stream->audio_offsets.get(chunk);

#if LIBAVCODEC_VERSION_MAJOR >= 58
            chunk = seek_5(stream, 
                chunk, 
                (double)aligned_sample / asset->sample_rate);
// recompute the true sample position
            aligned_sample = 0;
            for(int i = 0; i < chunk; i++)
            {
                aligned_sample += stream->audio_samples.get(i);
            }
// printf("FileFFMPEG::read_samples %d: want_sample=%ld got_sample=%ld\n",
// __LINE__,
// start_sample,
// aligned_sample);
#else
            avio_seek(((AVFormatContext*)stream->ffmpeg_file_context)->pb, 
                audio_offset, 
                SEEK_SET);
            avcodec_flush_buffers(decoder_context);
#endif



// set the true sample we're on
            stream->update_pcm_history(aligned_sample);
        }



	}





	int got_it = 0;
// DTS of packets sent to decoder 
    ArrayList<int64_t> dts_history;
// number of samples in packets sent to decoder
    ArrayList<int> samples_history;
// starting sample in the DTS history
    int dts_sample0 = stream->history_start + stream->history_size;
// Read samples until the requested range is decoded.
	while(stream->history_start + stream->history_size < 
        file->current_sample + len && !error)
	{
		AVPacket *packet = av_packet_alloc();




//        printf("FileFFMPEG::read_samples %d ftell=%ld\n", 
//            __LINE__, 
//            avio_tell(((AVFormatContext*)stream->ffmpeg_file_context)->pb));

		error = av_read_frame((AVFormatContext*)stream->ffmpeg_file_context, 
			packet);

//         printf("FileFFMPEG::read_samples %d ftell=%ld size=%d id=%d\n", 
//             __LINE__, 
//             avio_tell(((AVFormatContext*)stream->ffmpeg_file_context)->pb),
//             packet->size,
//             packet->stream_index);

        if(error)
        {
            printf("FileFFMPEG::read_samples %d error=%c%c%c%c current_sample=%ld\n",
    			__LINE__,
                (-error) & 0xff,
                ((-error) >> 8) & 0xff,
                ((-error) >> 16) & 0xff,
                ((-error) >> 24) & 0xff,
                file->current_sample);
// give up & reopen the ffmpeg objects
            av_packet_free(&packet);
            ffmpeg_lock->unlock();

            close_ffmpeg();
            open_ffmpeg();
            return 1;
        }
        

		unsigned char *packet_ptr = packet->data;
		int packet_len = packet->size;

		if(packet->stream_index == stream->ffmpeg_id)
		{
			int got_frame = 0;
            AVFrame *ffmpeg_samples = av_frame_alloc();



            int result = avcodec_send_packet(decoder_context, packet);

// 		    printf("FileFFMPEG::read_samples %d result=%c%c%c%c\n", 
// 		        __LINE__, 
// 		        (-result) & 0xff,
//                 ((-result) >> 8) & 0xff,
//                 ((-result) >> 16) & 0xff,
//                 ((-result) >> 24) & 0xff);
//             quicktime_print_buffer("", packet->data, packet->size);

//             dts_history.append(packet->dts);
// // compute the number of samples in the packet
//             int got_it = 0;
//             int packet_samples = 0;
//             for(int i = 0; i < stream->audio_offsets.size(); i++)
//             {
//                 if(stream->audio_offsets.get(i) >= offset)
//                 {
//                     packet_samples = stream->audio_samples.get(i);
//                     samples_history.append(stream->audio_samples.get(i));
//                     break;
//                 }
//             }
//             if(!got_it) samples_history.append(0);
            


// decode samples until the buffer is empty
			while(result >= 0)
            {
                result = avcodec_receive_frame(decoder_context, ffmpeg_samples);
                if(result >= 0)
                {
    				int samples_decoded = ffmpeg_samples->nb_samples;
                    got_frame = 1;
// Transfer decoded samples to ring buffer
    				stream->append_history(ffmpeg_samples, samples_decoded);

// printf("FileFFMPEG::read_samples %d pts=%ld samples_decoded=%d current_sample=%ld\n", 
// __LINE__,
// ffmpeg_samples->pts,
// samples_decoded,
// stream->history_start + stream->history_size);
                }
                else
                    break;

            }

			av_frame_free(&ffmpeg_samples);
		}
		
		av_packet_free(&packet);
	}

	stream->read_history(buffer, 
		file->current_sample, 
		audio_channel,
		len);
// printf("FileFFMPEG::read_samples %d\n", 
// __LINE__);

	ffmpeg_lock->unlock();
	return error;
}






