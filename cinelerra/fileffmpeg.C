
/*
 * CINELERRA
 * Copyright (C) 2016-2019 Adam Williams <broadcast at earthling dot net>
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

#include "filesystem.h"
#include "bcsignals.h"
#include "clip.h"
#include "file.h"
#include "fileffmpeg.h"
#include "indexfile.h"
#include "mpegaudio.h"
#include "mutex.h"
#include "preferences.h"
#include "quicktime.h"
#include <unistd.h>
#include "videodevice.inc"

#include <string.h>
#include <string>

using std::string;


// Different ffmpeg versions
#define FFMPEG_2010


// stuff
#define QUICKTIME_VP9 "VP9"
#define QUICKTIME_VP8 "VP8"

#define FFMPEG_TOC_SIG "FFMPEGTOC02"

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


void FileFFMPEGStream::append_index(void *ptr, 
    Asset *asset, 
    Preferences *preferences)
{
    AVFrame *frame = (AVFrame*)ptr;
    int len = frame->nb_samples;
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
    if(frame->channels < current_channels)
    {
        current_channels = frame->channels;
    }

    for(int j = 0; j < len; j++)
	{

// add a sample to the next frame
	    for(i = 0; i < current_channels; i++)
	    {
            float value = 0;
            switch(frame->format)
            {
                case AV_SAMPLE_FMT_S16P:
			    {
                    int16_t *input = (int16_t*)frame->data[i];
                    value = (float)input[j] / 32767;
                }
                break;

                case AV_SAMPLE_FMT_FLTP:
			    {
                    float *input = (float*)frame->data[i];
                    value = input[j];
                }
                break;

                default:
				    printf("FileFFMPEGStream::append_index %d: unsupported audio format %d\n", 
					    __LINE__,
					    frame->format);
				    break;
            }
            
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

int FileFFMPEG::check_sig(Asset *asset)
{
	char *ptr = strstr(asset->path, ".pcm");
	if(ptr) return 0;


	ffmpeg_lock->lock("FileFFMPEG::check_sig");
    avcodec_register_all();
    av_register_all();

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

	ffmpeg_lock->lock("FileFFMPEG::open_file");
    avcodec_register_all();
    av_register_all();
	ffmpeg_lock->unlock();

	if(rd)
	{
        if(open_ffmpeg())
        {
            return 1;
        }
	}




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

    if(debug) printf("FileFFMPEG::open_ffmpeg %d\n", __LINE__);
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
       		AVCodecContext *decoder_context = ffmpeg_stream->codec;
        	switch(decoder_context->codec_type) 
			{
        		case AVMEDIA_TYPE_AUDIO:
				{
//printf("FileFFMPEG::open_ffmpeg %d i=%d CODEC_TYPE_AUDIO\n", __LINE__, i);
if(debug) printf("FileFFMPEG::open_ffmpeg %d decoder_context->codec_id=%d\n", __LINE__, decoder_context->codec_id);
					AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
					if(!codec)
					{
						printf("FileFFMPEG::open_ffmpeg: audio codec 0x%x not found.\n", 
							decoder_context->codec_id);
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
						decoder_context = ffmpeg_stream->codec;
						codec = avcodec_find_decoder(decoder_context->codec_id);

						//avcodec_thread_init(decoder_context, file->cpus);
						decoder_context->thread_count = file->cpus;
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
						asset->bits = 16;
						asset->audio_data = 1;
						asset->sample_rate = decoder_context->sample_rate;

//printf("FileFFMPEG::open_ffmpeg %d codec_id=%d\n", __LINE__, decoder_context->codec_id);

						int64_t audio_length = (int64_t)(((AVFormatContext*)new_stream->ffmpeg_file_context)->duration * 
							asset->sample_rate / 
							AV_TIME_BASE);
if(debug) printf("FileFFMPEG::open_ffmpeg %d audio_length=%lld\n", __LINE__, (long long)audio_length);
						asset->audio_length = MAX(asset->audio_length, audio_length);
					}
            		break;
				}

        		case AVMEDIA_TYPE_VIDEO:
//printf("FileFFMPEG::open_ffmpeg %d i=%d CODEC_TYPE_VIDEO\n", __LINE__, i);
// only 1 video track supported
            		if(video_streams.size() == 0)
					{
						FileFFMPEGStream *new_stream = new FileFFMPEGStream;
						video_streams.append(new_stream);
						new_stream->ffmpeg_id = i;
                        new_stream->is_video = 1;


						asset->video_data = 1;
						asset->layers = 1;

// Open a new FFMPEG file for the stream
						result = avformat_open_input(
							(AVFormatContext**)&new_stream->ffmpeg_file_context, 
							asset->path, 
							0,
							0);

// initialize the codec
						avformat_find_stream_info((AVFormatContext*)new_stream->ffmpeg_file_context, 0);
						ffmpeg_stream = ((AVFormatContext*)new_stream->ffmpeg_file_context)->streams[i];
						decoder_context = ffmpeg_stream->codec;
						AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
//							avcodec_thread_init(decoder_context, file->cpus);
						decoder_context->thread_count = file->cpus;
						avcodec_open2(decoder_context, codec, 0);

//printf("FileFFMPEG::open_ffmpeg %d codec_id=%d %d\n", 
//__LINE__, decoder_context->codec_id, AV_CODEC_ID_VP8);
                        switch(decoder_context->codec_id)
                        {
                            case AV_CODEC_ID_H264:
                                strcpy (asset->vcodec, QUICKTIME_H264);
                                break;
                            case AV_CODEC_ID_H265:
                                strcpy (asset->vcodec, QUICKTIME_H265);
                                break;
                            case AV_CODEC_ID_VP9:
                                strcpy (asset->vcodec, QUICKTIME_VP9);
                                break;
							case AV_CODEC_ID_VP8:
								strcpy (asset->vcodec, QUICKTIME_VP8);
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
						asset->video_length = (int64_t)(((AVFormatContext*)new_stream->ffmpeg_file_context)->duration *
							asset->frame_rate / 
							AV_TIME_BASE);
						asset->aspect_ratio = 
							(double)decoder_context->sample_aspect_ratio.num / 
							decoder_context->sample_aspect_ratio.den;
if(debug) printf("FileFFMPEG::open_ffmpeg %d decoder_context->codec_id=%d\n", 
__LINE__, 
decoder_context->codec_id);

					}
            		break;

        		default:
printf("FileFFMPEG::open_ffmpeg %d i=%d codec_type=%d\n", __LINE__, i, decoder_context->codec_type);
            		break;
        	}
		}


//printf("FileFFMPEG::open_ffmpeg %d: %s %d %d\n", __LINE__, asset->vcodec, asset->video_data, strcmp(asset->vcodec, QUICKTIME_H264));
        if(debug) printf("FileFFMPEG::open_ffmpeg %d\n", __LINE__);

// does the format need a table of contents?
        if(asset->video_data &&
            (!strcmp(asset->vcodec, QUICKTIME_H264) ||
            !strcmp(asset->vcodec, QUICKTIME_H265) ||
            !strcmp(asset->vcodec, QUICKTIME_VP9)))
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
    if(debug) printf("FileFFMPEG::open_ffmpeg %d\n", __LINE__);

    return result;
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
                if(debug) printf("FileFFMPEG::create_toc %d reading total_samples=%ld chunks=%d\n", __LINE__, stream->total_samples, chunks);
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
        result = 0;

        Timer prev_time;
        Timer new_time;
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
            AVCodecContext *decoder_context = ffmpeg_stream->codec;

            if(decoder_context->codec_type == AVMEDIA_TYPE_AUDIO)
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
            if(decoder_context->codec_type == AVMEDIA_TYPE_VIDEO)
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
        if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);
        file->start_progress(progress_title.c_str(), total_bytes);
        if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);


        while(1)
        {
            AVPacket *packet = av_packet_alloc();
// starting offset of the packet
            int64_t offset = avio_tell(ffmpeg->pb);
            int error = av_read_frame(ffmpeg, 
				packet);
            if(error)
            {
                break;
            }
            if(file->progress_canceled()) 
			{
				result = 1;
				break;
			}

// update the progress bar            
            if(new_time.get_difference() >= 1000 && offset > 0)
            {
                new_time.update();
                
                
                int64_t elapsed_s = current_time.get_difference() / 1000;
                int64_t total_s = elapsed_s * total_bytes / offset;
                int64_t eta = total_s - elapsed_s;
                if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);
                file->update_progress(offset);
                if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);
                string2.assign(progress_title);
                sprintf(string3, 
					"\nETA: %ldm%lds",
					(int64_t)eta / 60,
					(int64_t)eta % 60);
                string2.append(string3);
                if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);
				file->update_progress_title(string2.c_str());
                if(debug) printf("FileFFMPEG::create_toc %d\n", __LINE__);
            }

            if(packet->size > 0)
            {
                if(debug) printf("FileFFMPEG::create_toc %d: offset=0x%lx size=%d stream=%d\n", 
                    __LINE__, 
                    offset,
                    packet->size,
                    packet->stream_index);

// DEBUG
//usleep(100000);
                
                
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
                        AVCodecContext *decoder_context = ffmpeg_stream->codec;

                        int got_frame = 0;
                        int bytes_decoded = avcodec_decode_audio4(decoder_context, 
					        ffmpeg_samples, 
					        &got_frame,
                            packet);


                        if(got_frame)
                        {
                            int samples_decoded = ffmpeg_samples->nb_samples;
//                             printf("FileFFMPEG::create_toc %d: audio offset=0x%lx size=%d samples=%d\n", 
//                                 __LINE__,
//                                 offset,
//                                 packet->size,
//                                 samples_decoded);
                            stream->audio_offsets.append(stream->next_frame_offset);
                            stream->audio_samples.append(samples_decoded);
                            stream->total_samples += samples_decoded;
// next samples to be decoded will come after the next offset
                            stream->next_frame_offset = -1;


//printf("FileFFMPEG::create_toc %d\n", __LINE__);
                            stream->append_index(ffmpeg_samples, 
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
                        if(!strcmp(asset->vcodec, QUICKTIME_VP9))
                        {
// VP9 skips some.  0x84 doesn't denote this is a skipped frame, 
// but a frame somewhere else is skipped for every 0x84 code & no more than 1
// 0x84 occurs for every skipped frame.
                            if(packet->data[0] == 0x84)
                            {
                                got_frame = 0;
                            }
                        }
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
            index_state->index_zoom,
            index_size,
            channels);
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
	AVCodecContext *decoder_context = ffmpeg_stream->codec;


    if(debug) printf("FileFFMPEG::read_frame %d file->current_frame=%ld\n", __LINE__, file->current_frame);


// seek if reading ahead this many frames
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

        if(need_restart)
        {
            ffmpeg_lock->unlock();
            close_ffmpeg();
            open_ffmpeg();
	        ffmpeg_lock->lock("FileFFMPEG::read_frame 2");

	        stream = video_streams.get(0);
	        ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->ffmpeg_id];
	        decoder_context = ffmpeg_stream->codec;
        }

        AVStream *seek_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[get_seek_stream()];
		int64_t timestamp = (int64_t)((double)file->current_frame * 
			seek_stream->time_base.den /
			seek_stream->time_base.num /
			asset->frame_rate);

// printf("FileFFMPEG::read_frame %d: %d %d\n", 
// __LINE__, 
// ffmpeg_stream->time_base.num,
// ffmpeg_stream->time_base.den);
//printf("FileFFMPEG::read_frame %d: want pts=%ld\n", __LINE__, timestamp);

// Want to seek to the nearest keyframe and read up to the current frame
// but ffmpeg seeks to the next keyframe.
// The best workaround was basing all the seeking on the video stream.

        if(!has_toc)
        {
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
            int rewind_count = 2;
// Some codecs only require rewinding 1 keyframe
            if(decoder_context->codec_id == AV_CODEC_ID_VP9)
            {
                rewind_count = 1;
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

// no keyframe found.  Go rewind_count before the last keyframe.
            if(!got_it)
            {
                i = total_keyframes - 1 - rewind_count;
                if(i < 0)
                {
                    i = 0;
                }
            }

            int keyframe = stream->video_keyframes.get(i);
//            avformat_close_input((AVFormatContext**)&stream->ffmpeg_file_context);
// Open a new FFMPEG file for the stream
//            avformat_open_input((AVFormatContext**)&stream->ffmpeg_file_context, 
//                asset->path, 
//                0,
//                0);

// reinitialize the codec
//			avformat_find_stream_info((AVFormatContext*)stream->ffmpeg_file_context, 0);
//			ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->ffmpeg_id];
//			decoder_context = ffmpeg_stream->codec;
//			AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
//			decoder_context->thread_count = file->cpus;
//			avcodec_open2(decoder_context, codec, 0);


            avio_seek(((AVFormatContext*)stream->ffmpeg_file_context)->pb, 
                stream->video_offsets.get(keyframe), 
                SEEK_SET);

// this doesn't work
            avcodec_flush_buffers(decoder_context);
            if(debug) printf("FileFFMPEG::read_frame %d offset=0x%x keyframe=%d file->current_frame=%ld\n", 
                __LINE__, 
                stream->video_offsets.get(keyframe),
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
if(debug) printf("FileFFMPEG::read_frame %d stream->current_frame=%ld file->current_frame=%ld error=%d\n", 
__LINE__,
stream->current_frame,
file->current_frame,
error);


	if(!ffmpeg_frame)
    {
        ffmpeg_frame = av_frame_alloc();
    }


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
		got_frame = 0;
if(debug) printf("FileFFMPEG::read_frame %d stream->current_frame=%lld file->current_frame=%lld\n", 
__LINE__,
(long long)stream->current_frame,
(long long)file->current_frame);

		while(!got_frame && !error)
		{
			AVPacket *packet = av_packet_alloc();



if(debug) printf("FileFFMPEG::read_frame %d ffmpeg_file_context=%p pb=%p ftell=%ld\n", 
__LINE__, 
stream->ffmpeg_file_context,
((AVFormatContext*)stream->ffmpeg_file_context)->pb,
avio_tell(((AVFormatContext*)stream->ffmpeg_file_context)->pb));
		    error = av_read_frame((AVFormatContext*)stream->ffmpeg_file_context, 
			    packet);

            if(error)
            {
                printf("FileFFMPEG::read_frame %d error=%d stream->current_frame=%ld file->current_frame=%ld\n",
    		        __LINE__,
                    error,
                    stream->current_frame,
                    file->current_frame);
// give up & reopen the ffmpeg objects.
// Still have frames buffered in the decoder, so reopen in the next seek.
                need_restart = 1;

//                 av_packet_free(&packet);
//                 ffmpeg_lock->unlock();
// 
//                 close_ffmpeg();
//                 open_ffmpeg();
//                 return 1;
            }
            else
            {

                av_packet_merge_side_data(packet);
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
                AVFrame *input_frame = (AVFrame*)ffmpeg_frame;

// if(file->current_frame >= 200 && file->current_frame < 280)
// {
// char string[1024];
// sprintf(string, "/tmp/debug%03lld", file->current_frame);
// FILE *out = fopen(string, "w");
// fwrite(packet->data, packet->size, 1, out);
// fclose(out);
// }

if(debug) printf("FileFFMPEG::read_frame %d decoder_context=%p ffmpeg_frame=%p\n", 
__LINE__,
decoder_context,
ffmpeg_frame);

		        int result = avcodec_decode_video2(
					decoder_context,
                    input_frame, 
					&got_picture,
                    packet);


if(debug) printf("FileFFMPEG::read_frame %d stream->current_frame=%ld result=%d got_picture=%d ptr=%p\n", 
__LINE__, 
stream->current_frame,
result,
got_picture,
((AVFrame*)ffmpeg_frame)->data[0]);
// check multiple ways it could glitch
				if(input_frame->data[0] && 
                    got_picture /* &&
                    (last_pts < 0 || input_frame->pts > last_pts) */) 
                {
// After seeking, the decoder sometimes rewinds which is shown by a 
// rewinding PTS, but sometimes it rewinds the PTS without rewinding 
// the decoder
                    if(last_pts > 0 && input_frame->pts <= last_pts)
                    {
                        printf("FileFFMPEG::read_frame %d current pts=%ld last_pts=%ld\n",
                            __LINE__,
                            input_frame->pts, 
                            last_pts);
                    }
                    got_frame = 1;
                    last_pts = input_frame->pts;
                }
// printf("FileFFMPEG::read_frame %d result=%d %02x %02x %02x %02x %02x %02x %02x %02x packet->flags=0x%x got_frame=%d\n", 
// __LINE__, 
// result, 
// packet->data[0],
// packet->data[1],
// packet->data[2],
// packet->data[3],
// packet->data[4],
// packet->data[5],
// packet->data[6],
// packet->data[7],
// packet->flags,
// got_frame);
			}
            else
            if(error)
            {
// at the end of the file, we still have frames buffered in the decoder
                int got_picture = 0;
                int result = avcodec_decode_video2(
					decoder_context,
                    (AVFrame*)ffmpeg_frame, 
					&got_picture,
                    packet);
				if(((AVFrame*)ffmpeg_frame)->data[0] && got_picture) 
                {
                    got_frame = 1;
                    error = 0;
                }
            }
			
			
			av_packet_free(&packet);

// printf("FileFFMPEG::read_frame %d got_frame=%d stream->current_frame=%ld file->current_frame=%ld pts=%ld\n", 
// __LINE__,
// got_frame,
// stream->current_frame,
// file->current_frame,
// ((AVFrame*)ffmpeg_frame)->pts);
		}

// only count frames which actually come out of the decoder
		if(got_frame) stream->current_frame++;
	}

// printf("FileFFMPEG::read_frame %d current_frame=%lld file->current_frame=%lld got_it=%d\n", 
// __LINE__, 
// current_frame,
// file->current_frame,
// got_frame);

// Convert colormodel
	if(got_frame)
	{
		int input_cmodel;
		AVFrame *input_frame = (AVFrame*)ffmpeg_frame;

// print a chksum for the frame
// int64_t chksum = 0;
// for(int i = 0; i < decoder_context->height * decoder_context->width; i++)
// {
//     chksum += input_frame->data[0][i];
// }
// printf("FileFFMPEG::read_frame %d decoded pts=%ld chksum=%ld\n", 
// __LINE__, 
// input_frame->pts,
// chksum);

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


	ffmpeg_lock->unlock();
	if(debug) printf("FileFFMPEG::read_frame %d\n", __LINE__);
	return error;
}


int FileFFMPEG::read_samples(double *buffer, int64_t len)
{
	const int debug = 0;
	int error = 0;
	ffmpeg_lock->lock("FileFFMPEG::read_samples");

//	if(debug) printf("FileFFMPEG::read_samples %d\n", __LINE__);
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
//	if(debug) printf("FileFFMPEG::read_samples %d\n", __LINE__);

	AVStream *ffmpeg_stream = ((AVFormatContext*)stream->ffmpeg_file_context)->streams[stream->ffmpeg_id];
	AVCodecContext *decoder_context = ffmpeg_stream->codec;

//	stream->update_pcm_history(file->current_sample);
	if(debug) printf("FileFFMPEG::read_samples %d len=%d\n", __LINE__, (int)len);



if(debug) printf("FileFFMPEG::read_samples %d: want=%ld - %ld history=%ld - %ld\n", 
__LINE__, 
file->current_sample,
file->current_sample + len,
stream->history_start,
stream->history_start + stream->history_size);

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
		    int64_t timestamp = (int64_t)((double)file->current_sample * 
			    seek_stream->time_base.den /
			    seek_stream->time_base.num /
			    asset->sample_rate);
// printf("FileFFMPEG::read_samples %d want pts=%ld\n",
// __LINE__,
// timestamp);

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
            int64_t sample_counter = 0;
            int got_it = 0;
            while(chunk < chunks)
            {
                sample_counter += stream->audio_samples.get(chunk);
// printf("FileFFMPEG::read_samples %d: chunk=%d sample_counter=%ld\n",
// __LINE__,
// chunk,
// sample_counter);


// start a certain amount before the desired sample for the decoder to warm up
                if(sample_counter > file->current_sample - asset->sample_rate)
                {
                    sample_counter -= stream->audio_samples.get(chunk);
                    got_it = 1;
                    break;
                }
                chunk++;
            }

            if(!got_it)
            {
                chunk = chunks - 1;
                sample_counter = stream->total_samples;
            }

if(debug) printf("FileFFMPEG::read_samples %d: chunk=%d sample=%ld offset=%p\n",
__LINE__,
chunk,
file->current_sample,
stream->audio_offsets.get(chunk));

            avio_seek(((AVFormatContext*)stream->ffmpeg_file_context)->pb, 
                    stream->audio_offsets.get(chunk), 
                    SEEK_SET);
// get the true sample we're on
            stream->update_pcm_history(sample_counter);
        }



//printf("FileFFMPEG::read_samples %d: timestamp=%ld\n", __LINE__, timestamp);
// av_index = ffmpeg_stream2->index_entries;
// for(int i = 0; i < ffmpeg_stream2->nb_index_entries; i++)
// {
//     AVIndexEntry *entry = &av_index[i];
//     printf("FileFFMPEG::read_samples %d: timestamp=%ld pos=%ld size=%d %x\n",
//         __LINE__,
//         entry->timestamp,
//         entry->pos,
//         entry->size,
//         entry->flags);
// }


//		if(debug) printf("FileFFMPEG::read_samples %d\n",
//			__LINE__);
	}





	int got_it = 0;
// Read frames until the requested range is decoded.
	while(stream->history_start + stream->history_size < 
        file->current_sample + len && !error)
	{
		AVPacket *packet = av_packet_alloc();
		
		error = av_read_frame((AVFormatContext*)stream->ffmpeg_file_context, 
			packet);
            
        if(error)
        {
            printf("FileFFMPEG::read_samples %d error=%d current_sample=%ld\n",
    			__LINE__,
                error,
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
		if(debug) printf("FileFFMPEG::read_samples %d error=%d packet_len=%d\n", 
		    __LINE__, 
		    error, 
		    packet_len);

		if(packet->stream_index == stream->ffmpeg_id)
		{
			while(packet_len > 0 && !error)
			{
//				int data_size = MPA_MAX_CODED_FRAME_SIZE;
				int got_frame;
                AVFrame *ffmpeg_samples = av_frame_alloc();


				int bytes_decoded = avcodec_decode_audio4(decoder_context, 
					ffmpeg_samples, 
					&got_frame,
                    packet);
// printf("FileFFMPEG::read_samples %d bytes_decoded=%d\n", 
// __LINE__, 
// bytes_decoded);


//				if(bytes_decoded < 0) error = 1;
				if(bytes_decoded == -1) error = 1;
				packet_ptr += bytes_decoded;
				packet_len -= bytes_decoded;
				if(!got_frame)
                {
                    av_frame_free(&ffmpeg_samples);
                    break;
                }
                else
                {

				    int samples_decoded = ffmpeg_samples->nb_samples;
// printf("FileFFMPEG::read_samples %d samples_decoded=%d\n", 
// __LINE__,
// samples_decoded);
// Transfer decoded samples to ring buffer
				    stream->append_history(ffmpeg_samples, samples_decoded);
// printf("FileFFMPEG::read_samples %d history=%ld...%ld want=%ld...%ld\n", 
// __LINE__, 
// stream->history_start,
// stream->history_start + stream->history_size,
// file->current_sample,
// file->current_sample + len);
                }

// static FILE *fd = 0;
// if(!fd) fd = fopen("/tmp/test.pcm", "w");
// fwrite(ffmpeg_samples, data_size, 1, fd);

				av_frame_free(&ffmpeg_samples);
			}
		}
		if(debug) PRINT_TRACE
		
		av_packet_free(&packet);
	}

	stream->read_history(buffer, 
		file->current_sample, 
		audio_channel,
		len);

	ffmpeg_lock->unlock();
	return error;
}






