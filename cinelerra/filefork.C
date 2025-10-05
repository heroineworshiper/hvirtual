/*
 * CINELERRA
 * Copyright (C) 2009-2024 Adam Williams <broadcast at earthling dot net>
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
#include "bchash.h"
#include "bcsignals.h"
#include "file.h"
#include "filefork.h"
#include "fileserver.h"
#include "filesystem.h"
#include "filethread.h"
#include "filexml.h"
#include "framecache.h"
#include "mutex.h"
#include "mwindow.h"
#include "samples.h"
#include "string.h"
#include "stringfile.h"
#include "vframe.h"


#include <unistd.h>


#ifdef USE_FILEFORK


FileFork::FileFork() : ForkWrapper()
{
	file = 0;
    set_title("FileFork");
//printf("FileFork::FileFork %d\n", __LINE__);
}

FileFork::~FileFork()
{
	if(ForkWrapper::is_dummy)
	{
		MWindow::file_server->delete_filefork(ForkWrapper::real_fork);
	}
}

void FileFork::init_child()
{
//printf("FileFork::init_child %d\n", __LINE__);
}

int FileFork::handle_command()
{
	int64_t result = 0;
	const int debug = 0;

	if(debug) printf("FileFork::handle_command %d this=%p command=%d\n", 
		__LINE__, 
		this, 
		command_token);
	switch(command_token)
	{
		case OPEN_FILE:
		{
			file = new File;
			file->is_fork = 1;
            file->file_fork = this;
//printf("FileFork::handle_command %d OPEN_FILE\n", __LINE__);

// Read file modes
			int offset = 0;
			int rd = *(int*)(command_data + offset);
			offset += sizeof(int);
			int wr = *(int*)(command_data + offset);
			offset += sizeof(int);
			file->cpus = *(int*)(command_data + offset);
			offset += sizeof(int);
			file->cache_size = *(int*)(command_data + offset);
			offset += sizeof(int);
			file->white_balance_raw = *(int*)(command_data + offset);
			offset += sizeof(int);
			file->interpolate_raw = *(int*)(command_data + offset);
			offset += sizeof(int);
			file->disable_toc_creation = *(int*)(command_data + offset);
			offset += sizeof(int);
//printf("FileFork::handle_command %d OPEN_FILE %d\n", 
//__LINE__, file->disable_toc_creation);

            file->get_frame_cache()->set_max_size(file->cache_size);
// Read asset from socket
			BC_Hash table;
			table.load_string((char*)command_data + offset);
			Asset *new_asset = new Asset;
			new_asset->load_defaults(&table, "", 1, 1, 1, 1, 1);
			if(debug)
			{
				printf("FileFork::handle_command %d\n%s\n", 
				    __LINE__, 
				    command_data + offset);
				    new_asset->dump();
			}


// printf("FileFork::handle_command %d path=%s disable_toc_creation=%d\n",
// __LINE__, 
// command_data + offset,
// file->disable_toc_creation);
// table.dump();
//printf("FileFork::handle_command %d server=%p\n", __LINE__, server);
//printf("FileFork::handle_command %d server->preferences=%p\n", __LINE__, server->preferences);
			result = file->open_file(
				MWindow::preferences, 
				new_asset, 
				rd, 
				wr);
			new_asset->Garbage::remove_user();
//printf("FileFork::handle_command %d sending result=%d\n", __LINE__, (int)result);



// Send updated asset
			file->asset->save_defaults(&table, "", 1, 1, 1, 1, 1);
			char *string = 0;
			table.save_string(string);
			int buffer_size = strlen(string) + 1;
//printf("FileFork::handle_command %d OPEN_FILE result=%d\n", __LINE__, (int)result);
			send_result(result, (unsigned char*)string, buffer_size);
			delete [] string;
			break;
		}

		case SET_PROCESSORS:
			file->set_processors(*(int*)command_data);
			send_result(0, 0, 0);
			break;


		case SET_CACHE:
			file->set_cache(*(int*)command_data);
			send_result(0, 0, 0);
			break;

		case SET_PRELOAD:
			file->set_preload(*(int64_t*)command_data);
			send_result(0, 0, 0);
			break;

		case SET_SUBTITLE:
			file->set_subtitle(*(int*)command_data);
			send_result(0, 0, 0);
			break;

		case SET_INTERPOLATE_RAW:
			file->set_interpolate_raw(*(int*)command_data);
			send_result(0, 0, 0);
			break;

		case SET_WHITE_BALANCE_RAW:
			file->set_white_balance_raw(*(int*)command_data);
			send_result(0, 0, 0);
			break;

		case SET_CACHE_FRAMES:
			file->set_cache_frames(*(int*)command_data);
			send_result(0, 0, 0);
			break;

		case PURGE_CACHE:
			result = file->purge_cache();
			send_result(result, 0, 0);
//printf("FileFork::handle_command %d child_fd=%d\n", __LINE__, child_fd);
//			send_result(1234, 0, 0);
			break;

		case CLOSE_FILE:
		{
			unsigned char result_buffer[sizeof(int64_t) * 2];
			file->close_file(0);
			*(int64_t*)result_buffer = file->asset->audio_length;
			*(int64_t*)(result_buffer + sizeof(int64_t)) = file->asset->video_length;
// printf("FileFork::handle_command %d %lld %lld\n", 
// __LINE__, 
// file->asset->audio_length, 
// file->asset->video_length);
			send_result(0, result_buffer, sizeof(int64_t) * 2);
// exit the child loop here instead of using the stop command
			ForkWrapper::done = 1;
			break;
		}

// 		case GET_INDEX:
// 			result = file->get_index((char*)command_data);
// 			send_result(result, 0, 0);
// 			break;


		case START_AUDIO_THREAD:
		{
			int buffer_size = *(int*)command_data;
			int ring_buffers = *(int*)(command_data + sizeof(int));
			result = file->start_audio_thread(buffer_size, ring_buffers);
// Send buffer information back to server here
			int result_bytes = ring_buffers * 
				Samples::filefork_size() * 
				file->asset->channels;
			unsigned char result_buffer[result_bytes];
			for(int i = 0; i < ring_buffers; i++)
			{
				Samples **samples = file->audio_thread->audio_buffer[i];
				for(int j = 0; j < file->asset->channels; j++)
				{
					samples[j]->to_filefork(result_buffer +
						i * Samples::filefork_size() * file->asset->channels +
						j * Samples::filefork_size());
				}
			}

			send_result(result, result_buffer, result_bytes);
			break;
		}

		case START_VIDEO_THREAD:
		{
			int buffer_size = *(int*)command_data;
			int color_model = *(int*)(command_data + sizeof(int));
			int ring_buffers = *(int*)(command_data + sizeof(int) * 2);
			int compressed = *(int*)(command_data + sizeof(int) * 3);
// allocate buffers here
			result = file->start_video_thread(buffer_size, 
				color_model,
				ring_buffers,
				compressed);

// Send buffer information back to server here
			int result_bytes = ring_buffers *
				file->asset->layers *
				buffer_size *
				VFrame::filefork_size();
			unsigned char result_buffer[result_bytes];

			for(int i = 0; i < ring_buffers; i++)
			{
				VFrame ***frames = file->video_thread->video_buffer[i];
				for(int j = 0; j < file->asset->layers; j++)
				{
					for(int k = 0; k < buffer_size; k++)
					{
//printf("FileFork::handle_command %d j=%d k=%d %p %p\n", __LINE__, j, k, frames[j][k], frames[j][k]->get_shmid()));
						frames[j][k]->to_filefork(result_buffer +
							i * file->asset->layers *
								buffer_size *
								VFrame::filefork_size() +
							j * buffer_size *
								VFrame::filefork_size() +
							k * VFrame::filefork_size());
					}
				}
			}

			send_result(result, result_buffer, result_bytes);
			break;
		}


// 		case START_VIDEO_DECODE_THREAD:
// 			result = file->start_video_decode_thread();
// 			send_result(result, 0, 0);
// 			break;


		case STOP_AUDIO_THREAD:
			result = file->stop_audio_thread();
			send_result(result, 0, 0);
			break;

		case STOP_VIDEO_THREAD:
			result = file->stop_video_thread();
			send_result(result, 0, 0);
			break;

		case SET_CHANNEL:
			result = file->set_channel(*(int*)command_data);
			send_result(result, 0, 0);
			break;

		case SET_LAYER:
			result = file->set_layer(*(int*)command_data, 0);
			send_result(result, 0, 0);
			break;

		case GET_AUDIO_LENGTH:
			result = file->get_audio_length();
			send_result(result, 0, 0);
			break;

		case GET_VIDEO_LENGTH:
			result = file->get_video_length();
			send_result(result, 0, 0);
			break;

		case GET_VIDEO_POSITION:
			result = file->get_video_position();
			send_result(result, 0, 0);
			break;

		case GET_AUDIO_POSITION:
			result = file->get_audio_position();
			send_result(result, 0, 0);
			break;

		case SET_AUDIO_POSITION:
			result = file->set_audio_position(*(int64_t*)command_data);
			send_result(result, 0, 0);
			break;

		case SET_VIDEO_POSITION:
			result = file->set_video_position(*(int64_t*)command_data, 0);
			send_result(result, 0, 0);
			break;

		case WRITE_SAMPLES:
		{
			int entry_size = Samples::filefork_size();
			Samples **samples = new Samples*[file->asset->channels];
			for(int i = 0; i < file->asset->channels; i++)
			{
				samples[i] = new Samples;
				samples[i]->from_filefork(
					command_data + entry_size * i);
			}
			int64_t len = *(int64_t*)(command_data + 
				entry_size * file->asset->channels);

			result = file->write_samples(samples, len);
			send_result(result, 0, 0);

			for(int i = 0; i < file->asset->channels; i++)
			{
				delete samples[i];
			}
			delete [] samples;
			break;
		}

		case WRITE_FRAMES:
		{
//PRINT_TRACE
			int entry_size = VFrame::filefork_size();
//PRINT_TRACE
			VFrame ***frames = new VFrame**[file->asset->layers];
//printf("FileFork::handle_command %d %d\n", __LINE__, file->asset->layers);
			int len = *(int*)command_data;
//printf("FileFork::handle_command %d %d %d\n", __LINE__, file->asset->layers, len);

			for(int i = 0; i < file->asset->layers; i++)
			{
				frames[i] = new VFrame*[len];
				for(int j = 0; j < len; j++)
				{
					frames[i][j] = new VFrame;
//PRINT_TRACE
					frames[i][j]->from_filefork(command_data +
						sizeof(int) + 
						entry_size * len * i +
						entry_size * j);

//PRINT_TRACE
				}
			}

//PRINT_TRACE
			result = file->write_frames(frames, len);
//PRINT_TRACE

			send_result(result, 0, 0);
			for(int i = 0; i < file->asset->layers; i++)
			{
				for(int j = 0; j < len; j++)
				{
					delete frames[i][j];
				}
				delete [] frames[i];
			}
			delete [] frames;
			break;
		}


		case WRITE_AUDIO_BUFFER:
			result = file->write_audio_buffer(*(int64_t*)command_data);
			send_result(result, 0, 0);
			break;

		case WRITE_VIDEO_BUFFER:
		{
//printf("FileFork::handle_command %d\n", __LINE__);
			int len = *(int64_t*)command_data;
			VFrame ***video_buffer = file->video_thread->get_last_video_buffer();
			for(int i = 0; i < file->asset->layers; i++)
			{
				for(int j = 0; j < len; j++)
				{
// Copy memory state
//printf("FileFork::handle_command %d i=%d j=%d %p %p\n", __LINE__, i, j, video_buffer[i][j], video_buffer[i][j]->get_shmid());
					video_buffer[i][j]->from_filefork(command_data +
						sizeof(int64_t) +
						VFrame::filefork_size() * (len * i + j));
//printf("FileFork::handle_command %d %p %lld\n", __LINE__, video_buffer[i][j]->get_shmid(), video_buffer[i][j]->get_number());
				}
			}
		
			result = file->write_video_buffer(len);
			send_result(result, 0, 0);
//printf("FileFork::handle_command %d\n", __LINE__);
			break;
		}

		case GET_AUDIO_BUFFER:
		{
// 			int entry_size = Samples::filefork_size();
// 			int result_bytes = entry_size * file->asset->channels;
// 			unsigned char result_buffer[sizeof(int)];
// 			
// Make it swap buffers
			Samples **samples = file->get_audio_buffer();
// 			for(int i = 0; i < file->asset->channels; i++)
// 			{
// 				samples[i]->to_filefork(result_buffer + 
// 					i * Samples::filefork_size());
// 			}

			send_result(file->audio_thread->current_buffer, 0, 0);
			break;
		}

		case GET_VIDEO_BUFFER:
		{
// 			int entry_size = VFrame::filefork_size();
// 			int layers = file->asset->layers;
// 			int buffer_size = file->video_thread->buffer_size;
// 			int result_size = entry_size * 
// 				layers *
// 				buffer_size +
// 				sizeof(int);
// 			unsigned char result_buffer[result_size];
// 			*(int*)(result_buffer + entry_size * 
// 				layers *
// 				buffer_size) = buffer_size;
//printf("FileFork::handle_command %d layers=%d\n", __LINE__, layers);

			VFrame ***frames = file->get_video_buffer();
// 			for(int i = 0; i < layers; i++)
// 			{
// 				for(int j = 0; j < buffer_size; j++)
// 				{
// 					frames[i][j]->to_filefork(result_buffer +
// 						entry_size * i * buffer_size +
// 						entry_size * j);
// 				}
// 			}

			send_result(file->video_thread->current_buffer, 0, 0);
// printf("FileFork::handle_command %d GET_VIDEO_BUFFER %d\n", 
// __LINE__,
// file->video_thread->current_buffer);
			break;
		}

		case READ_SAMPLES:
		{
			if(debug) PRINT_TRACE
			int len = *(int64_t*)(command_data + Samples::filefork_size());
			if(debug) PRINT_TRACE
			Samples *samples = new Samples;
			samples->from_filefork(command_data);
			if(debug) PRINT_TRACE

			result = file->read_samples(samples, len);
			if(debug) PRINT_TRACE
			send_result(result, 0, 0);
			if(debug) PRINT_TRACE

			delete samples;
			if(debug) PRINT_TRACE
			break;
		}

		case READ_FRAME:
		{
			VFrame *frame = new VFrame;
            int use_opengl = command_data[0];
			frame->from_filefork(command_data + sizeof(int));
			int allocated_data = frame->get_compressed_allocated();
			
			
// printf("FileFork::handle_command %d file=%p\n", 
// __LINE__, 
// file);
// frame->dump();
			result = file->read_frame(frame, 0, use_opengl, 0);


// printf("FileFork::handle_command %d size=%d\n", 
// __LINE__, 
// frame->get_compressed_size());


// Send compressed data through socket only if data allocation changed.
			if(frame->get_color_model() == BC_COMPRESSED &&
				allocated_data != frame->get_compressed_allocated())
			{
				int result_size = sizeof(int) * 2 + frame->get_compressed_size();
				unsigned char *result_data = new unsigned char[result_size];
				*(int*)result_data = frame->get_compressed_size();
				*(int*)(result_data + sizeof(int)) = frame->get_keyframe();
				memcpy(result_data + sizeof(int) * 2, 
					frame->get_data(), 
					frame->get_compressed_size());
				send_result(result, 
					result_data, 
					result_size);
				delete [] result_data;
			}
			else
			{

// Serialize the params
				StringFile params((long)0);
				frame->get_params()->save_stringfile(&params);
				
				int result_size = sizeof(int) * 2 + params.get_length() + 1;
				unsigned char *result_data = new unsigned char[result_size];
				*(int*)result_data = frame->get_compressed_size();
				*(int*)(result_data + sizeof(int)) = frame->get_keyframe();
// send the params
				memcpy(result_data + sizeof(int) * 2, 
					params.string, 
					params.get_length() + 1);

//printf("FileFork::handle_command %d params=\n", __LINE__);
//frame->get_params()->dump();
//printf("FileFork::handle_command %d result_data=%s\n", __LINE__, params.string);

				send_result(result, result_data, result_size);
				delete [] result_data;
			}


//printf("FileFork::handle_command %d size=%d read_frame_dst=%p\n", 
//__LINE__, 
//frame->get_compressed_size(),
//frame);
//frame->dump_params();

			delete frame;

// printf("FileFork::handle_command %d size=%d\n", 
// __LINE__, 
// frame->get_compressed_size());
			break;
		}

		case CAN_COPY_FROM:
		{
			FileXML xml;
			int64_t position = *(int64_t*)(command_data);
			int output_w = *(int*)(command_data + sizeof(int64_t));
			int output_h = *(int*)(command_data + sizeof(int64_t) + sizeof(int));
			xml.read_from_string((char*)command_data + 
				sizeof(int64_t) + 
				sizeof(int) * 2);
			xml.read_tag();
// Asset doesn't read the XML path.
			Asset *new_asset = new Asset(xml.tag.get_value("SRC"));
			new_asset->read(&xml, 1);
			result = file->can_copy_from(new_asset, 
				position, 
				output_w, 
				output_h);
			send_result(result, 0, 0);
			new_asset->Garbage::remove_user();
			break;
		}

// 		case COLORMODEL_SUPPORTED:
// 		{
// 			int colormodel = *(int*)command_data;
// 			result = file->colormodel_supported(colormodel);
// 			send_result(result, 0, 0);
// 			break;
// 		}

		case GET_MEMORY_USAGE:
			result = file->get_memory_usage();
			send_result(result, 0, 0);
			break;
	}

	return result;
}


#endif // USE_FILEFORK




