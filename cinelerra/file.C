/*
 * CINELERRA
 * Copyright (C) 2010-2019 Adam Williams <broadcast at earthling dot net>
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
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "byteorder.h"
#include "cache.inc"
#include "condition.h"
#include "errorbox.h"
#include "fileac3.h"
#include "fileavi.h"
#include "filebase.h"
#include "filecr2.h"
#include "fileexr.h"
#include "fileffmpeg.h"
#include "fileflac.h"
#include "filefork.h"
#include "file.h"
#include "filegif.h"
#include "filejpeg.h"
#include "filemkv.h"
#include "filemov.h"
#include "filempeg.h"
#include "fileogg.h"
#include "fileogg.h"
#include "filepng.h"
#include "filescene.h"
#include "fileserver.h"
#include "filesndfile.h"
#include "filetga.h"
#include "filethread.h"
#include "filetiff.h"
#include "filevorbis.h"
#include "filexml.h"
#include "formatwindow.h"
#include "framecache.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "samples.h"
#include "stringfile.h"
#include "vframe.h"

//static int temp_debug = 0;


File::File()
{
	cpus = 1;
	asset = new Asset;
	format_completion = new Condition(1, "File::format_completion");
	write_lock = new Condition(1, "File::write_lock");
	frame_cache = new FrameCache;
	reset_parameters();
}

File::~File()
{
	if(getting_options)
	{
		if(format_window) format_window->set_done(0);
		format_completion->lock("File::~File");
		format_completion->unlock();
	}

	if(temp_frame) 
	{
//temp_debug--;
//printf("File::~File %d temp_debug=%d\n", __LINE__, temp_debug);
		delete temp_frame;
	}


	close_file(0);

	asset->Garbage::remove_user();

	delete format_completion;

	delete write_lock;

	if(frame_cache) delete frame_cache;
}

void File::reset_parameters()
{
#ifdef USE_FILEFORK
	file_fork = 0;
	is_fork = 0;
#endif

	file = 0;
	audio_thread = 0;
	video_thread = 0;
	getting_options = 0;
	format_window = 0;
	temp_frame = 0;
	current_sample = 0;
	current_frame = 0;
	current_channel = 0;
	current_layer = 0;
	normalized_sample = 0;
//	normalized_sample_rate = 0;
	use_cache = 0;
	preferences = 0;
	playback_subtitle = -1;
	interpolate_raw = 1;


	temp_samples_buffer = 0;
	temp_frame_buffer = 0;
	current_frame_buffer = 0;
	audio_ring_buffers = 0;
	video_ring_buffers = 0;
	video_buffer_size = 0;
	cache_size = 0;
	memory_usage = 0;
}

int File::raise_window()
{
	if(getting_options && format_window)
	{
		format_window->raise_window();
		format_window->flush();
	}
	return 0;
}

void File::close_window()
{
	if(getting_options)
	{
		format_window->lock_window("File::close_window");
		format_window->set_done(1);
		format_window->unlock_window();
		getting_options = 0;
	}
}

int File::get_options(BC_WindowBase *parent_window, 
	ArrayList<PluginServer*> *plugindb, 
	Asset *asset, 
	int audio_options, 
	int video_options,
	char *locked_compressor)
{
	getting_options = 1;
	format_completion->lock("File::get_options");
	switch(asset->format)
	{
		case FILE_AC3:
			FileAC3::get_parameters(parent_window,
				asset,
				format_window,
				audio_options,
				video_options);
			break;
		case FILE_PCM:
		case FILE_WAV:
		case FILE_AU:
		case FILE_AIFF:
		case FILE_SND:
			FileSndFile::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_MOV:
			FileMOV::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options,
				locked_compressor);
			break;
		case FILE_AMPEG:
		case FILE_VMPEG:
			FileMPEG::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_AVI:
			FileMOV::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options,
				locked_compressor);
			break;
		case FILE_AVI_LAVTOOLS:
		case FILE_AVI_ARNE2:
		case FILE_AVI_ARNE1:
		case FILE_AVI_AVIFILE:
			FileAVI::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options,
				locked_compressor);
			break;
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			FileJPEG::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_EXR:
		case FILE_EXR_LIST:
			FileEXR::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_FLAC:
			FileFLAC::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_PNG:
		case FILE_PNG_LIST:
			FilePNG::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_TGA:
		case FILE_TGA_LIST:
			FileTGA::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_TIFF:
		case FILE_TIFF_LIST:
			FileTIFF::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_OGG:
			FileOGG::get_parameters(parent_window,
				asset,
				format_window,
				audio_options,
				video_options);
			break;
		default:
			break;
	}

	if(!format_window)
	{
		ErrorBox *errorbox = new ErrorBox(PROGRAM_NAME ": Error",
			parent_window->get_abs_cursor_x(1),
			parent_window->get_abs_cursor_y(1));
		format_window = errorbox;
		getting_options = 1;
		if(audio_options)
			errorbox->create_objects(_("This format doesn't support audio."));
		else
		if(video_options)
			errorbox->create_objects(_("This format doesn't support video."));
		errorbox->run_window();
		delete errorbox;
	}

	getting_options = 0;
	format_window = 0;
	format_completion->unlock();
	return 0;
}










int File::set_processors(int cpus)   // Set the number of cpus for certain codecs
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::SET_PROCESSORS, (unsigned char*)&cpus, sizeof(cpus));
		file_fork->read_result();
	}
#endif

// Set all instances so gets work.
	this->cpus = cpus;

	return 0;
}

void File::set_cache(int bytes)
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::SET_CACHE, 
			(unsigned char*)&bytes, 
			sizeof(bytes));
		file_fork->read_result();
	}
#endif

// Set all instances so gets work.
	this->cache_size = bytes;
//printf("File::set_cache %d %d\n", __LINE__, cache_size);
}

int File::set_preload(int64_t size)
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::SET_PRELOAD, (unsigned char*)&size, sizeof(size));
		file_fork->read_result();
	}

#endif

	this->playback_preload = size;
	return 0;
}

void File::set_subtitle(int value)
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::SET_SUBTITLE, (unsigned char*)&value, sizeof(value));
		file_fork->read_result();
	}

#endif
	this->playback_subtitle = value;
}

void File::set_interpolate_raw(int value)
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::SET_INTERPOLATE_RAW, (unsigned char*)&value, sizeof(value));
		file_fork->read_result();
	}

#endif

	this->interpolate_raw = value;
}

// void File::set_white_balance_raw(int value)
// {
// #ifdef USE_FILEFORK
// 	if(!is_fork && file_fork)
// 	{
// 		file_fork->send_command(FileFork::SET_WHITE_BALANCE_RAW, (unsigned char*)&value, sizeof(value));
// 		file_fork->read_result();
// 	}
// #endif
// 
// 	this->white_balance_raw = value;
// }

void File::set_cache_frames(int value)
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::SET_CACHE_FRAMES, (unsigned char*)&value, sizeof(value));
		file_fork->read_result();
	}
#endif
	

	if(!video_thread)
		use_cache = value;
}

int File::purge_cache()
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
//printf("File::purge_cache %d\n", __LINE__);
		file_fork->send_command(FileFork::PURGE_CACHE, 0, 0);
//printf("File::purge_cache %d\n", __LINE__);
		int result = file_fork->read_result();

// update the precalculated memory usage
		memory_usage -= result;
// sleeping causes CICache::check_out to lock up without polling
//sleep(1);
//printf("File::purge_cache %d\n", __LINE__);
		return result;
	}
#endif


//printf("File::purge_cache %d memory_usage=%d\n", __LINE__, get_memory_usage());
	int result = frame_cache->delete_oldest();
// return the number of bytes freed
	return result;
}











int File::open_file(Preferences *preferences, 
	Asset *asset, 
	int rd, 
	int wr)
{
	int result = 0;
	const int debug = 0;

	this->preferences = preferences;
	this->asset->copy_from(asset, 1);
	this->rd = rd;
	this->wr = wr;
	file = 0;

	if(debug) printf("File::open_file %d\n", __LINE__);

#ifdef USE_FILEFORK
	if(!is_fork)
	{
// printf("File::open_file %d file_server=%p rd=%d wr=%d %d\n", 
// __LINE__, 
// MWindow::file_server,
// rd, 
// wr, 
// asset->ms_quantization);




		file_fork = MWindow::file_server->new_filefork();

// Send the asset
// Convert to hash table
		BC_Hash table;
		asset->save_defaults(&table, "", 1, 1, 1, 1, 1);
// Convert to string
		char *string = 0;
		table.save_string(string);
		int buffer_size = sizeof(int) * 6 + strlen(string) + 1;
		unsigned char *buffer = new unsigned char[buffer_size];
		int offset = 0;
		*(int*)(buffer + offset) = rd;
		offset += sizeof(int);
		*(int*)(buffer + offset) = wr;
		offset += sizeof(int);
		*(int*)(buffer + offset) = cpus;
		offset += sizeof(int);
		*(int*)(buffer + offset) = cache_size;
		offset += sizeof(int);
//		*(int*)(buffer + offset) = white_balance_raw;
		*(int*)(buffer + offset) = 0;
		offset += sizeof(int);
		*(int*)(buffer + offset) = interpolate_raw;
		offset += sizeof(int);
		memcpy(buffer + offset, string, strlen(string) + 1);
//printf("File::open_file %d\n", __LINE__);
		file_fork->send_command(FileFork::OPEN_FILE, 
			buffer, 
			buffer_size);
		delete [] buffer;
		delete [] string;



// get progress & completion from the fork when building a table of contents
        int done = 0;
        while(!done)
        {
            result = file_fork->read_result();

            switch(result)
            {
// done loading
		        case 0:
		        {
// Get the updated asset from the fork
			        table.load_string((char*)file_fork->result_data);

			        asset->load_defaults(&table, "", 1, 1, 1, 1, 1);
			        this->asset->load_defaults(&table, "", 1, 1, 1, 1, 1);
                    done = 1;
//this->asset->dump();
                    break;
		        }

// progress bar commands sent by the fork
                case FileFork::START_PROGRESS:
                {
                    int64_t total = *(int64_t*)file_fork->result_data;
                    const char *title = (const char *)file_fork->result_data + sizeof(int64_t);
                    start_progress(title, total);
                    break;
                }
                
                case FileFork::UPDATE_PROGRESS:
                {
                    int64_t value = *(int64_t*)file_fork->result_data;
                    update_progress(value);
                    break;
                }

                case FileFork::UPDATE_PROGRESS_TITLE:
                {
                    const char *title = (const char *)file_fork->result_data;
                    update_progress_title(title);
                    break;
                }

                case FileFork::PROGRESS_CANCELED:
                {
                    int result2 = progress_canceled();
                    file_fork->send_command(result2, 
		                0,
		                0);
                    break;
                }

                case FileFork::STOP_PROGRESS:
                {
                    const char *title = (const char *)file_fork->result_data;
                    stop_progress(title);
                    break;
                }
            }
        }




// If it's a scene renderer, close it & reopen it locally to get the 
// full OpenGL support.
// Just doing 2D for now.  Should be forked in case Festival crashes.
// 		if(rd && this->asset->format == FILE_SCENE)
// 		{
// //printf("File::open_file %p %d\n", this, __LINE__);
// 			close_file(0);
// // Lie to get it to work properly
// 			is_fork = 1;
// 		}
// 		else
		{
			return result;
		}
	}
#endif // USE_FILEFORK


	if(debug) printf("File::open_file %p %d\n", this, __LINE__);

	switch(this->asset->format)
	{
// get the format now
// If you add another format to case 0, you also need to add another case for the
// file format #define.
		case FILE_UNKNOWN:
			FILE *stream;
			if(!(stream = fopen(this->asset->path, "rb")))
			{
// file not found
				return 1;
			}

			char test[16];
			result = fread(test, 16, 1, stream);

			if(FileScene::check_sig(this->asset, test))
			{
// libsndfile
				fclose(stream);
				file = new FileScene(this->asset, this);
			}
			else
			if(FileSndFile::check_sig(this->asset))
			{
// libsndfile
				fclose(stream);
				file = new FileSndFile(this->asset, this);
			}
			else
			if(FilePNG::check_sig(this->asset))
			{
// PNG file
				fclose(stream);
				file = new FilePNG(this->asset, this);
			}
			else
			if(FileJPEG::check_sig(this->asset))
			{
// JPEG file
				fclose(stream);
				file = new FileJPEG(this->asset, this);
			}
			else
			if(FileGIF::check_sig(this->asset))
			{
// GIF file
				fclose(stream);
				file = new FileGIF(this->asset, this);
			}
			else
			if(FileEXR::check_sig(this->asset, test))
			{
// EXR file
				fclose(stream);
				file = new FileEXR(this->asset, this);
			}
			else
			if(FileFLAC::check_sig(this->asset, test))
			{
// FLAC file
				fclose(stream);
				file = new FileFLAC(this->asset, this);
			}
			else
			if(FileCR2::check_sig(this->asset))
			{
// CR2 file
				fclose(stream);
				file = new FileCR2(this->asset, this);
			}
			else
			if(FileTGA::check_sig(this->asset))
			{
// TGA file
				fclose(stream);
				file = new FileTGA(this->asset, this);
			}
			else
			if(FileTIFF::check_sig(this->asset))
			{
// TIFF file
				fclose(stream);
				file = new FileTIFF(this->asset, this);
			}
			else
			if(FileVorbis::check_sig(this->asset))
			{
// VorbisFile file
				fclose(stream);
				file = new FileVorbis(this->asset, this);
			}
			else
			if(FileOGG::check_sig(this->asset))
			{
// OGG file.  Doesn't always work with pure audio files.
				fclose(stream);
				file = new FileOGG(this->asset, this);
			}
			else
			if(FileMPEG::check_sig(this->asset))
			{
// MPEG file
				fclose(stream);
				file = new FileMPEG(this->asset, this);
			}
			else
			if(test[0] == '<' && test[1] == 'E' && test[2] == 'D' && test[3] == 'L' && test[4] == '>' ||
				test[0] == '<' && test[1] == 'H' && test[2] == 'T' && test[3] == 'A' && test[4] == 'L' && test[5] == '>' ||
				test[0] == '<' && test[1] == '?' && test[2] == 'x' && test[3] == 'm' && test[4] == 'l')
			{
// XML file
				fclose(stream);
				return FILE_IS_XML;
			}    // can't load project file
			else
			if(FileMOV::check_sig(this->asset))
			{
// MOV file
// should be last because quicktime lacks a magic number
				fclose(stream);
				file = new FileMOV(this->asset, this);
			}
			else
// 			if(FileMKV::check_sig(this->asset))
// 			{
// 				fclose(stream);
// 				file = new FileMKV(this->asset, this);
// 			}
// 			else
// FFMPEG last because it sux
			if(FileFFMPEG::check_sig(this->asset))
			{
				fclose(stream);
				file = new FileFFMPEG(this->asset, this);
			}
			else
			{
// PCM file
				fclose(stream);
				return FILE_UNRECOGNIZED_CODEC;
			}   // need more info
			break;

// format already determined
		case FILE_AC3:
			file = new FileAC3(this->asset, this);
			break;

		case FILE_SCENE:
			file = new FileScene(this->asset, this);
			break;

		case FILE_FFMPEG:
			file = new FileFFMPEG(this->asset, this);
			break;

		case FILE_PCM:
		case FILE_WAV:
		case FILE_AU:
		case FILE_AIFF:
		case FILE_SND:
//printf("File::open_file 1\n");
			file = new FileSndFile(this->asset, this);
			break;

		case FILE_PNG:
		case FILE_PNG_LIST:
			file = new FilePNG(this->asset, this);
			break;

		case FILE_JPEG:
		case FILE_JPEG_LIST:
			file = new FileJPEG(this->asset, this);
			break;

		case FILE_GIF:
		case FILE_GIF_LIST:
			file = new FileGIF(this->asset, this);
			break;

		case FILE_EXR:
		case FILE_EXR_LIST:
			file = new FileEXR(this->asset, this);
			break;

		case FILE_FLAC:
			file = new FileFLAC(this->asset, this);
			break;

		case FILE_CR2:
		case FILE_CR2_LIST:
			file = new FileCR2(this->asset, this);
			break;

		case FILE_TGA_LIST:
		case FILE_TGA:
			file = new FileTGA(this->asset, this);
			break;

		case FILE_TIFF:
		case FILE_TIFF_LIST:
			file = new FileTIFF(this->asset, this);
			break;

		case FILE_MOV:
#ifdef USE_FFMPEG_OUTPUT
// use ffmpeg if a MOV & writing to it
            if(wr)
            {
                file = new FileFFMPEG(this->asset, this);
            }
            else
#endif // USE_FFMPEG_OUTPUT
            {
			    file = new FileMOV(this->asset, this);
            }
			break;

		case FILE_MPEG:
		case FILE_AMPEG:
		case FILE_VMPEG:
			file = new FileMPEG(this->asset, this);
			break;

		case FILE_OGG:
			file = new FileOGG(this->asset, this);
			break;

		case FILE_VORBIS:
			file = new FileVorbis(this->asset, this);
			break;

		case FILE_AVI:
			file = new FileMOV(this->asset, this);
			break;

		case FILE_AVI_LAVTOOLS:
		case FILE_AVI_ARNE2:
		case FILE_AVI_ARNE1:
		case FILE_AVI_AVIFILE:
			file = new FileAVI(this->asset, this);
			break;

// try plugins
		default:
			return 1;
			break;
	}


// Reopen file with correct parser and get header.
	if(file->open_file(rd, wr))
	{
		delete file;
		file = 0;
	}



// Set extra writing parameters to mandatory settings.
	if(file && wr)
	{
		if(this->asset->dither) file->set_dither();
	}



// Synchronize header parameters
	if(file)
	{
		asset->copy_from(this->asset, 1);
//asset->dump();
	}

	if(debug) printf("File::open_file %d file=%p\n", __LINE__, file);
// sleep(1);

	if(file)
		return FILE_OK;
	else
		return FILE_NOT_FOUND;
}



// called by the fork to show a progress bar when building a table of contents
void File::start_progress(const char *title, int64_t total)
{
    if(is_fork)
	{
		int data_len = sizeof(int64_t) + strlen(title) + 1;
		unsigned char buffer[data_len];
		*(int64_t*)buffer = total;
		memcpy(buffer + sizeof(int64_t), title, strlen(title) + 1);
		file_fork->send_result(FileFork::START_PROGRESS, buffer, data_len);
        return;
    }
    
// show progress only if there's a GUI
    if(BC_WindowBase::get_resources()->initialized)
    {
        if(!MWindow::file_progress)
        {
// Always going to be a dedicated window, even if we use MainProgress
            MWindow::file_progress = new BC_ProgressBox(-1, 
			    -1, 
			    title, 
			    total);
            MWindow::file_progress->start();
        }
    }
    else
    {
        printf("File::start_progress %d: %s\n", __LINE__, title);
    }
}

void File::update_progress(int64_t value)
{
    if(is_fork)
	{
		file_fork->send_result(FileFork::UPDATE_PROGRESS, 
            (unsigned char*)&value, 
            sizeof(int64_t));
        return;
    }
    
    if(MWindow::file_progress)
    {
        MWindow::file_progress->update(value, 1);
    }
}

void File::update_progress_title(const char *title)
{
    if(is_fork)
	{
		file_fork->send_result(FileFork::UPDATE_PROGRESS_TITLE, 
            (unsigned char*)title, 
            strlen(title) + 1);
        return;
    }
    
    if(MWindow::file_progress)
    {
        MWindow::file_progress->update_title(title, 1);
    }
}



// returns 1 if the user cancelled
int File::progress_canceled()
{
    if(is_fork)
	{
		file_fork->send_result(FileFork::PROGRESS_CANCELED, 0, 0);
		file_fork->read_command();
        return file_fork->command_token;
    }
    
    if(MWindow::file_progress && MWindow::file_progress->is_cancelled())
    {
        return 1;
    }
    
    return 0;
}

void File::stop_progress(const char *title)
{
    if(is_fork)
	{
		file_fork->send_result(FileFork::STOP_PROGRESS, 
            (unsigned char*)title, 
            strlen(title) + 1);
        return;
    }
    
    if(MWindow::file_progress)
    {
		if(!MWindow::is_loading)
		{
        	MWindow::file_progress->stop_progress();
        	delete MWindow::file_progress;
        	MWindow::file_progress = 0;
		}
    }
    else
    {
        printf("File::stop_progress %d: %s\n", __LINE__, title);
    }
}


void File::delete_temp_samples_buffer()
{

	if(temp_samples_buffer)
	{
		for(int j = 0; j < audio_ring_buffers; j++)
		{
			for(int i = 0; i < asset->channels; i++)
			{
				delete temp_samples_buffer[j][i];
			}
			delete [] temp_samples_buffer[j];
		}

		delete [] temp_samples_buffer;
		temp_samples_buffer = 0;
		audio_ring_buffers = 0;
	}
}

void File::delete_temp_frame_buffer()
{
	
	if(temp_frame_buffer)
	{
		for(int k = 0; k < video_ring_buffers; k++)
		{
			for(int i = 0; i < asset->layers; i++)
			{
				for(int j = 0; j < video_buffer_size; j++)
				{
					delete temp_frame_buffer[k][i][j];
				}
				delete [] temp_frame_buffer[k][i];
			}
			delete [] temp_frame_buffer[k];
		}

		delete [] temp_frame_buffer;
		temp_frame_buffer = 0;
		video_ring_buffers = 0;
		video_buffer_size = 0;
	}
}

int File::close_file(int ignore_thread)
{
	const int debug = 0;

#ifdef USE_FILEFORK
	if(debug) printf("File::close_file %d: file=%p file_fork=%p\n", __LINE__, file, file_fork);


	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::CLOSE_FILE, 0, 0);
		file_fork->read_result();

		if(asset && wr)
		{
			asset->audio_length = current_sample = *(int64_t*)file_fork->result_data;
			asset->video_length = current_frame = *(int64_t*)(file_fork->result_data + sizeof(int64_t));
		}

		if(debug) printf("File::close_file %d current_sample=%lld current_frame=%lld\n", 
			__LINE__,
			(long long)current_sample,
			(long long)current_frame);

		delete file_fork;
		file_fork = 0;
		
	}

#endif

	if(debug) printf("File::close_file file=%p %d\n", file, __LINE__);

	if(!ignore_thread)
	{

		stop_audio_thread();

		stop_video_thread();

	}


	if(debug) printf("File::close_file file=%p %d\n", file, __LINE__);
	if(file) 
	{
// The file's asset is a copy of the argument passed to open_file so the
// user must copy lengths from the file's asset.
		if(asset && wr)
		{
			asset->audio_length = current_sample;
			asset->video_length = current_frame;
		}

		file->close_file();

		delete file;

	}
	if(debug) printf("File::close_file file=%p %d\n", file, __LINE__);

	delete_temp_samples_buffer();
	delete_temp_frame_buffer();
	if(debug) printf("File::close_file file=%p %d\n", file, __LINE__);

#ifdef USE_FILEFORK
    if(!is_fork && file_fork)
    {
    	delete file_fork;
        file_fork = 0;
    }
#endif


	if(debug) printf("File::close_file file=%p %d\n", file, __LINE__);

	reset_parameters();
	if(debug) printf("File::close_file file=%p %d\n", file, __LINE__);

	return 0;
}



// int File::get_index(char *index_path)
// {
// #ifdef USE_FILEFORK
// 	if(!is_fork && file_fork)
// 	{
// 		file_fork->send_command(FileFork::GET_INDEX, (unsigned char*)index_path, strlen(index_path) + 1);
// 		int result = file_fork->read_result();
// 		return result;
// 	}
// #endif
// 
// 	if(file)
// 	{
// 		return file->get_index(index_path);
// 	}
// 	return 1;
// }



int File::start_audio_thread(int buffer_size, int ring_buffers)
{
	this->audio_ring_buffers = ring_buffers;

#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		unsigned char buffer[sizeof(int) * 2];
		*(int*)(buffer) = buffer_size;
		*(int*)(buffer + sizeof(int)) = audio_ring_buffers;
		file_fork->send_command(FileFork::START_AUDIO_THREAD, buffer, sizeof(buffer));
		int result = file_fork->read_result();


//printf("File::start_audio_thread %d file_fork->result_data=%p\n", __LINE__, file_fork->result_data);
// Create server copy of buffer
		delete_temp_samples_buffer();
//printf("File::start_audio_thread %d\n", __LINE__);
		temp_samples_buffer = new Samples**[audio_ring_buffers];
//printf("File::start_audio_thread %d\n", __LINE__);
		for(int i = 0; i < audio_ring_buffers; i++)
		{
//printf("File::start_audio_thread %d\n", __LINE__);
			temp_samples_buffer[i] = new Samples*[asset->channels];
//printf("File::start_audio_thread %d\n", __LINE__);
			for(int j = 0; j < asset->channels; j++)
			{
				int offset = i * Samples::filefork_size() * asset->channels +
					j * Samples::filefork_size();
//printf("File::start_audio_thread %d j=%d offset=%d\n", __LINE__, j, offset);
				temp_samples_buffer[i][j] = new Samples;
				temp_samples_buffer[i][j]->from_filefork(
					file_fork->result_data +
					offset);
//printf("File::start_audio_thread %d\n", __LINE__);
			}
		}
		
		return result;
	}
#endif

	
	if(!audio_thread)
	{
		audio_thread = new FileThread(this, 1, 0);
		audio_thread->start_writing(buffer_size, 0, ring_buffers, 0);
	}
	return 0;
}

int File::start_video_thread(int buffer_size, 
	int color_model, 
	int ring_buffers, 
	int compressed)
{
	this->video_ring_buffers = ring_buffers;
	this->video_buffer_size = buffer_size;

#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
// This resets variables
		delete_temp_frame_buffer();

		this->video_ring_buffers = ring_buffers;
		this->video_buffer_size = buffer_size;

		unsigned char buffer[sizeof(int) * 4];
		*(int*)(buffer) = buffer_size;
		*(int*)(buffer + sizeof(int)) = color_model;
		*(int*)(buffer + sizeof(int) * 2) = video_ring_buffers;
		*(int*)(buffer + sizeof(int) * 3) = compressed;
// Buffers are allocated
		file_fork->send_command(FileFork::START_VIDEO_THREAD, 
			buffer, 
			sizeof(buffer));
		int result = file_fork->read_result();


// Create server copy of buffer
// printf("File::start_video_thread %d video_ring_buffers=%d color_model=%d\n", 
// __LINE__, 
// video_ring_buffers,
// color_model);
		temp_frame_buffer = new VFrame***[video_ring_buffers];
		for(int i = 0; i < video_ring_buffers; i++)
		{
			temp_frame_buffer[i] = new VFrame**[asset->layers];
			for(int j = 0; j < asset->layers; j++)
			{
				temp_frame_buffer[i][j] = new VFrame*[video_buffer_size];
//printf("File::start_video_thread %d %p\n", __LINE__, temp_frame_buffer[i][j]);
				for(int k = 0; k < video_buffer_size; k++)
				{
					temp_frame_buffer[i][j][k] = new VFrame;
					temp_frame_buffer[i][j][k]->from_filefork(file_fork->result_data + 
						i * asset->layers * video_buffer_size * VFrame::filefork_size() + 
						j * video_buffer_size * VFrame::filefork_size() +
						k * VFrame::filefork_size());
				}
			}
		}


		return result;
	}
#endif



	if(!video_thread)
	{
		video_thread = new FileThread(this, 0, 1);
		video_thread->start_writing(buffer_size, 
			color_model, 
			ring_buffers, 
			compressed);
	}
	return 0;
}

int File::start_video_decode_thread()
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::START_VIDEO_DECODE_THREAD, 0, 0);
		file_fork->read_result();
		return 0;
	}
#endif


// Currently, CR2 is the only one which won't work asynchronously, so
// we're not using a virtual function yet.
	if(!video_thread /* && asset->format != FILE_CR2 */)
	{
		video_thread = new FileThread(this, 0, 1);
		video_thread->start_reading();
		use_cache = 0;
	}
	return 0;
}

int File::stop_audio_thread()
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::STOP_AUDIO_THREAD, 0, 0);
		file_fork->read_result();
		return 0;
	}
#endif

	if(audio_thread)
	{
		audio_thread->stop_writing();
		delete audio_thread;
		audio_thread = 0;
	}
	return 0;
}

int File::stop_video_thread()
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::STOP_VIDEO_THREAD, 0, 0);
		file_fork->read_result();
		return 0;
	}
#endif

	if(video_thread)
	{
		video_thread->stop_reading();
		video_thread->stop_writing();
		delete video_thread;
		video_thread = 0;
	}
	return 0;
}

FileThread* File::get_video_thread()
{
	return video_thread;
}

int File::set_channel(int channel) 
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
// Set it locally for get_channel
		current_channel = channel;
		file_fork->send_command(FileFork::SET_CHANNEL, (unsigned char*)&channel, sizeof(channel));
		int result = file_fork->read_result();
		return result;
	}
#endif

	if(file && channel < asset->channels)
	{
		current_channel = channel;
		return 0;
	}
	else
		return 1;
}

int File::get_channel()
{
	return current_channel;
}

int File::set_layer(int layer, int is_thread) 
{
#ifdef USE_FILEFORK
// thread should only call in the fork
	if(!is_fork && !is_thread)
	{
		file_fork->send_command(FileFork::SET_LAYER, (unsigned char*)&layer, sizeof(layer));
		int result = file_fork->read_result();
		current_layer = layer;
		return result;
	}
#endif

	if(file && layer < asset->layers)
	{
		if(!is_thread && video_thread)
		{
			video_thread->set_layer(layer);
		}
		else
		{
			current_layer = layer;
		}
		return 0; 
	}
	else
		return 1;
}

int64_t File::get_audio_length()
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::GET_AUDIO_LENGTH, 0, 0);
		int64_t result = file_fork->read_result();
		return result;
	}
#endif

	int64_t result = asset->audio_length;
	int64_t base_samplerate = -1;
	if(result > 0)
	{
		if(base_samplerate > 0)
			return (int64_t)((double)result / asset->sample_rate * base_samplerate + 0.5);
		else
			return result;
	}
	else
		return -1;
}

int64_t File::get_video_length()
{ 
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::GET_VIDEO_LENGTH, 0, 0);
		int64_t result = file_fork->read_result();
		return result;
	}
#endif


	int64_t result = asset->video_length;
	float base_framerate = -1;
	if(result > 0)
	{
		if(base_framerate > 0)
			return (int64_t)((double)result / asset->frame_rate * base_framerate + 0.5); 
		else
			return result;
	}
	else
		return -1;  // infinity
}


int64_t File::get_video_position() 
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::GET_VIDEO_POSITION, 0, 0);
		int64_t result = file_fork->read_result();
		return result;
	}
#endif

	float base_framerate = -1;
	if(base_framerate > 0)
		return (int64_t)((double)current_frame / asset->frame_rate * base_framerate + 0.5);
	else
		return current_frame;
}

int64_t File::get_audio_position() 
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::GET_AUDIO_POSITION, 0, 0);
		int64_t result = file_fork->read_result();
		return result;
	}
#endif


// 	int64_t base_samplerate = -1;
// 	if(base_samplerate > 0)
// 	{
// 		if(normalized_sample_rate == base_samplerate)
// 			return normalized_sample;
// 		else
// 			return (int64_t)((double)current_sample / 
// 				asset->sample_rate * 
// 				base_samplerate + 
// 				0.5);
// 	}
// 	else
		return current_sample;
}



// The base samplerate must be nonzero if the base samplerate in the calling
// function is expected to change as this forces the resampler to reset.

int File::set_audio_position(int64_t position) 
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::SET_AUDIO_POSITION, 
			(unsigned char*)&position, 
			sizeof(position));
		int result = file_fork->read_result();
		return result;
	}
#endif

	int result = 0;

	if(!file) return 1;

#define REPOSITION(x, y) \
	(labs((x) - (y)) > 1)


	float base_samplerate = asset->sample_rate;

// printf("File::set_audio_position %d base_samplerate=%f normalized_sample=%ld current_sample=%ld position=%ld\n", 
// __LINE__, 
// base_samplerate, 
// normalized_sample,
// current_sample, 
// position);

	if((base_samplerate && REPOSITION(normalized_sample, position)) ||
		(!base_samplerate && REPOSITION(current_sample, position)))
	{
// Can't reset resampler since one seek operation is done 
// for every channel to be read at the same position.

// Use a conditional reset for just the case of different base_samplerates
// 		if(base_samplerate > 0)
// 		{
// 			if(normalized_sample_rate &&
// 				normalized_sample_rate != base_samplerate && 
// 				resample)
// 				resample->reset(-1);
// 
// 			normalized_sample = position;
// 			normalized_sample_rate = (int64_t)((base_samplerate > 0) ? 
// 				base_samplerate : 
// 				asset->sample_rate);
// 
// // Convert position to file's rate
// 			if(base_samplerate > 0)
// 				current_sample = Units::round((double)position / 
// 					base_samplerate * 
// 					asset->sample_rate);
// 		}
// 		else
		{
// Resampling is now done in AModule
			normalized_sample = current_sample = position;
// 			normalized_sample = Units::round((double)position / 
// 					asset->sample_rate * 
// 					normalized_sample_rate);
// Can not set the normalized sample rate since this would reset the resampler.
		}


// printf("File::set_audio_position %d normalized_sample=%ld\n", 
// __LINE__, 
// normalized_sample);
		result = file->set_audio_position(current_sample);

		if(result)
			printf("File::set_audio_position position=%d base_samplerate=%f asset=%p asset->sample_rate=%d\n",
				(int)position, 
				base_samplerate, 
				asset, 
				(int)asset->sample_rate);
	}

//printf("File::set_audio_position %d %d %d\n", current_channel, current_sample, position);

	return result;
}

int File::set_video_position(int64_t position, 
	int is_thread) 
{
#ifdef USE_FILEFORK
// Thread should only call in the fork
	if(!is_fork && !is_thread)
	{
//printf("File::set_video_position %d %lld\n", __LINE__, position);
		file_fork->send_command(FileFork::SET_VIDEO_POSITION, (unsigned char*)&position, sizeof(position));
		int result = file_fork->read_result();
		return result;
	}
#endif

	int result = 0;
	if(!file) return 0;

// Convert to file's rate
// 	if(base_framerate > 0)
// 		position = (int64_t)((double)position / 
// 			base_framerate * 
// 			asset->frame_rate + 
// 			0.5);


	if(video_thread && !is_thread)
	{
// Call thread.  Thread calls this again to set the file state.
		video_thread->set_video_position(position);
	}
	else
	if(current_frame != position)
	{
		if(file)
		{
			current_frame = position;
			result = file->set_video_position(current_frame);
		}
	}

	return result;
}

// No resampling here.
int File::write_samples(Samples **buffer, int64_t len)
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		int entry_size = Samples::filefork_size();
		int buffer_size = entry_size * asset->channels + sizeof(int64_t);
		unsigned char fork_buffer[buffer_size];
		for(int i = 0; i < asset->channels; i++)
		{
			buffer[i]->to_filefork(fork_buffer + entry_size * i);
		}

		*(int64_t*)(fork_buffer + 
			entry_size * asset->channels) = len;

		file_fork->send_command(FileFork::WRITE_SAMPLES, 
			fork_buffer, 
			buffer_size);
		int result = file_fork->read_result();
		return result;
	}
#endif




	int result = 1;

	if(file)
	{
		write_lock->lock("File::write_samples");

// Convert to arrays for backwards compatability
		double *temp[asset->channels];
		for(int i = 0; i < asset->channels; i++)
		{
			temp[i] = buffer[i]->get_data();
		}

		result = file->write_samples(temp, len);
		current_sample += len;
		normalized_sample += len;
		asset->audio_length += len;
		write_lock->unlock();
	}
	return result;
}





// Can't put any cmodel abstraction here because the filebase couldn't be
// parallel.
int File::write_frames(VFrame ***frames, int len)
{
//printf("File::write_frames %d\n", __LINE__);
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
//printf("File::write_frames %d\n", __LINE__);
		int entry_size = frames[0][0]->filefork_size();
		unsigned char fork_buffer[entry_size * asset->layers * len + sizeof(int)];
		for(int i = 0; i < asset->layers; i++)
		{
			for(int j = 0; j < len; j++)
			{
// printf("File::write_frames %d %lld %d\n", 
// __LINE__, 
// frames[i][j]->get_number(), 
// frames[i][j]->get_keyframe());
				frames[i][j]->to_filefork(fork_buffer + 
					sizeof(int) +
					entry_size * len * i +
					entry_size * j);
			}
		}

		
//PRINT_TRACE
// Frames per layer
		*(int*)fork_buffer = len;

//PRINT_TRACE

		file_fork->send_command(FileFork::WRITE_FRAMES, 
			fork_buffer, 
			sizeof(fork_buffer));
//PRINT_TRACE
		int result = file_fork->read_result();


//printf("File::write_frames %d\n", __LINE__);
		return result;
	}


#endif // USE_FILEFORK


//PRINT_TRACE
// Store the counters in temps so the filebase can choose to overwrite them.
	int result;
	int current_frame_temp = current_frame;
	int video_length_temp = asset->video_length;

	write_lock->lock("File::write_frames");

//PRINT_TRACE
	result = file->write_frames(frames, len);
//PRINT_TRACE

	current_frame = current_frame_temp + len;
	asset->video_length = video_length_temp + len;
	write_lock->unlock();
//PRINT_TRACE
	return result;
}

// Only called by FileThread
int File::write_compressed_frame(VFrame *buffer)
{
	int result = 0;
	write_lock->lock("File::write_compressed_frame");
	result = file->write_compressed_frame(buffer);
	current_frame++;
	asset->video_length++;
	write_lock->unlock();
	return result;
}


int File::write_audio_buffer(int64_t len)
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::WRITE_AUDIO_BUFFER, (unsigned char*)&len, sizeof(len));
		int result = file_fork->read_result();
		return result;
	}
#endif

	int result = 0;
	if(audio_thread)
	{
		result = audio_thread->write_buffer(len);
	}
	return result;
}

int File::write_video_buffer(int64_t len)
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
// Copy over sequence numbers for background rendering
// frame sizes for direct copy
//printf("File::write_video_buffer %d\n", __LINE__);
		int fork_buffer_size = sizeof(int64_t) +
			VFrame::filefork_size() * asset->layers * len;
		unsigned char fork_buffer[fork_buffer_size];
		*(int64_t*)(fork_buffer) = len;
		for(int i = 0; i < asset->layers; i++)
		{
			for(int j = 0; j < len; j++)
			{
// Send memory state
				current_frame_buffer[i][j]->to_filefork(fork_buffer + 
					sizeof(int64_t) +
					VFrame::filefork_size() * (len * i + j));
// printf("File::write_video_buffer %d size=%d %d %02x %02x %02x %02x %02x %02x %02x %02x\n", 
// __LINE__, 
// current_frame_buffer[i][j]->get_shmid(),
// current_frame_buffer[i][j]->get_compressed_size(),
// current_frame_buffer[i][j]->get_data()[0],
// current_frame_buffer[i][j]->get_data()[1],
// current_frame_buffer[i][j]->get_data()[2],
// current_frame_buffer[i][j]->get_data()[3],
// current_frame_buffer[i][j]->get_data()[4],
// current_frame_buffer[i][j]->get_data()[5],
// current_frame_buffer[i][j]->get_data()[6],
// current_frame_buffer[i][j]->get_data()[7]);
			}
		}

//printf("File::write_video_buffer %d\n", __LINE__);
		file_fork->send_command(FileFork::WRITE_VIDEO_BUFFER, 
			fork_buffer, 
			fork_buffer_size);
//printf("File::write_video_buffer %d\n", __LINE__);
		int result = file_fork->read_result();
//printf("File::write_video_buffer %d\n", __LINE__);
		return result;
	}
#endif

	int result = 0;
	if(video_thread)
	{
		result = video_thread->write_buffer(len);
	}

	return result;
}

Samples** File::get_audio_buffer()
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		file_fork->send_command(FileFork::GET_AUDIO_BUFFER, 0, 0);
		int result = file_fork->read_result();

// Read parameters for a Samples buffer & create it in File
//		delete_temp_samples_buffer();
// 		if(!temp_samples_buffer) 
// 		{
// 			temp_samples_buffer = new Samples**[ring_buffers];
// 			for(int i = 0; i < ring_buffers; i++) temp_samples_buffer[i] = 0;
// 		}
// 		
// 		
// 		temp_samples_buffer  = new Samples*[asset->channels];
// 		for(int i = 0; i < asset->channels; i++)
// 		{
// 			temp_samples_buffer[i] = new Samples;
// 			temp_samples_buffer[i]->from_filefork(file_fork->result_data + 
// 				i * Samples::filefork_size());
// 		}

		return temp_samples_buffer[result];
	}
#endif

	if(audio_thread) return audio_thread->get_audio_buffer();
	return 0;
}

VFrame*** File::get_video_buffer()
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{

		file_fork->send_command(FileFork::GET_VIDEO_BUFFER, 0, 0);
		int result = file_fork->read_result();

// Read parameters for a VFrame buffer & create it in File
//		delete_temp_frame_buffer();


// 		temp_frame_size = *(int*)(file_fork->result_data + 
// 			file_fork->result_bytes - 
// 			sizeof(int));
// 
// //printf("File::get_video_buffer %d %p %d\n", __LINE__, this, asset->layers);
// 		temp_frame_buffer = new VFrame**[asset->layers];
// 
// 		for(int i = 0; i < asset->layers; i++)
// 		{
// 
// 			temp_frame_buffer[i] = new VFrame*[temp_frame_size];
// 
// 			for(int j = 0; j < temp_frame_size; j++)
// 			{
// 
// 				temp_frame_buffer[i][j] = new VFrame;
// printf("File::get_video_buffer %d %p\n", __LINE__, temp_frame_buffer[i][j]);
// 
// 				temp_frame_buffer[i][j]->from_filefork(file_fork->result_data + 
// 					i * temp_frame_size * VFrame::filefork_size() +
// 					j * VFrame::filefork_size());
// 
// 			}
// 		}
// 

		current_frame_buffer = temp_frame_buffer[result];

		return current_frame_buffer;
	}
#endif

	if(video_thread) 
	{
		VFrame*** result = video_thread->get_video_buffer();

		return result;
	}

	return 0;
}


int File::read_samples(Samples *samples, int64_t len)
{
	if(len < 0) return 0;

	int result = 0;
	const int debug = 0;
	if(debug) PRINT_TRACE

#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		int buffer_bytes = Samples::filefork_size() + sizeof(int64_t);
		unsigned char buffer[buffer_bytes];
		samples->to_filefork(buffer);
		*(int64_t*)(buffer + Samples::filefork_size()) = len;
		if(debug) PRINT_TRACE
		file_fork->send_command(FileFork::READ_SAMPLES, 
			buffer, 
			buffer_bytes);
		if(debug) PRINT_TRACE
		int result = file_fork->read_result();

// Crashed
		if(result && !file_fork->child_running())
		{
			delete file_fork;
			result = open_file(preferences, asset, rd, wr);
		}


 		file_fork->send_command(FileFork::GET_MEMORY_USAGE, 
 			0, 
 			0);
 		memory_usage = file_fork->read_result();

//printf("File::read_samples %lld\n", memory_usage);


		return result;
	}
#endif

	if(debug) PRINT_TRACE

	double *buffer = samples->get_data();

	int64_t base_samplerate = asset->sample_rate;

	if(file)
	{
// Resample recursively calls this with the asset sample rate
		if(base_samplerate == 0) base_samplerate = asset->sample_rate;

//printf("File::read_samples %d %lld %lld\n", __LINE__, current_sample, len);
// Load with resampling	
// 		if(base_samplerate != asset->sample_rate)
// 		{
// //printf("File::read_samples 3\n");
// 			if(!resample)
// 			{
// //printf("File::read_samples 4\n");
// 				resample = new Resample(this, asset->channels);
// 			}
// 
// //printf("File::read_samples 5\n");
// 			current_sample += resample->resample(buffer, 
// 				len, 
// 				asset->sample_rate, 
// 				base_samplerate,
// 				current_channel,
// 				current_sample,
// 				normalized_sample);
// //printf("File::read_samples 6\n");
// 		}
// 		else
// // Load directly
		{
			if(debug) PRINT_TRACE
			result = file->read_samples(buffer, len);

			if(debug) PRINT_TRACE
			current_sample += len;
		}

		normalized_sample += len;
	}
	if(debug) PRINT_TRACE

	return result;
}

// int File::read_compressed_frame(VFrame *buffer)
// {
// #ifdef USE_FILEFORK
// 	if(!is_fork && file_fork)
// 	{
// 		unsigned char fork_buffer[buffer->filefork_size()];
// 		buffer->to_filefork(fork_buffer);
// 		file_fork->send_command(FileFork::READ_COMPRESSED_FRAME, 
// 			fork_buffer, 
// 			buffer->filefork_size());
// // TODO: need to read back new frame parameters & reallocate
// 		int result = file_fork->read_result();
// 		return result;
// 	}
// #endif
// 
// 
// 
// 
// 	int result = 1;
// 	if(file)
// 		result = file->read_compressed_frame(buffer);
// 	current_frame++;
// 	return result;
// }

// int64_t File::compressed_frame_size()
// {
// #ifdef USE_FILEFORK
// 	if(!is_fork && file_fork)
// 	{
// 		file_fork->send_command(FileFork::COMPRESSED_FRAME_SIZE, 
// 			0, 
// 			0);
// 		int64_t result = file_fork->read_result();
// 		return result;
// 	}
// #endif
// 
// 
// 	if(file)
// 		return file->compressed_frame_size();
// 	else 
// 		return 0;
// }




int File::read_frame(VFrame *frame, int is_thread)
{
	const int debug = 0;

	if(debug) PRINT_TRACE

#ifdef USE_FILEFORK
// is_thread is only true in the fork
	if(!is_fork && !is_thread)
	{
		unsigned char fork_buffer[VFrame::filefork_size()];
		if(debug) PRINT_TRACE

		frame->to_filefork(fork_buffer);
		file_fork->send_command(FileFork::READ_FRAME, 
			fork_buffer, 
			VFrame::filefork_size());

		int result = file_fork->read_result();


// Crashed
		if(result && !file_fork->child_running())
		{
			delete file_fork;
			result = open_file(preferences, asset, rd, wr);
		}
		else
		if(!result && 
			frame->get_color_model() == BC_COMPRESSED)
		{
// Get compressed data from socket
//printf("File::read_frame %d %d\n", __LINE__, file_fork->result_bytes);
			if(file_fork->result_bytes > sizeof(int) * 2)
			{
//printf("File::read_frame %d %d\n", __LINE__, file_fork->result_bytes);
				int header_size = sizeof(int) * 2;
				frame->allocate_compressed_data(file_fork->result_bytes - header_size);
				frame->set_compressed_size(file_fork->result_bytes - header_size);
				frame->set_keyframe(*(int*)(file_fork->result_data + sizeof(int)));
				memcpy(frame->get_data(), 
					file_fork->result_data + header_size,
					file_fork->result_bytes - header_size);
			}
			else
// Get compressed data size
			{
				frame->set_compressed_size(*(int*)file_fork->result_data);
				frame->set_keyframe(*(int*)(file_fork->result_data + sizeof(int)));
//printf("File::read_frame %d %d\n", __LINE__, *(int*)(file_fork->result_data + sizeof(int)));
			}
		}
		else
		if(!result)
		{
// get the params if not compressed
			if(file_fork->result_bytes > sizeof(int) * 2)
			{
				StringFile params((long)0);
				params.read_from_string((char*)(file_fork->result_data + sizeof(int) * 2));
//printf("File::read_frame %d result_data=%s\n", __LINE__, file_fork->result_data + sizeof(int) * 2);
				frame->get_params()->load_stringfile(&params, 1);
			}
		}

		file_fork->send_command(FileFork::GET_MEMORY_USAGE, 
			0, 
			0);
		memory_usage = file_fork->read_result();
		
		if(debug) PRINT_TRACE

//printf("File::read_frame %d frame=%p\n", __LINE__, frame);
//frame->dump_params();

		return result;
	}
#endif


//printf("File::read_frame %d\n", __LINE__);

	if(video_thread && !is_thread) return video_thread->read_frame(frame);

//printf("File::read_frame %d\n", __LINE__);
	if(debug) PRINT_TRACE
	if(file)
	{
		if(debug) PRINT_TRACE
		int supported_colormodel = colormodel_supported(frame->get_color_model());
		int advance_position = 1;

// Test cache
		if(use_cache &&
			frame_cache->get_frame(frame,
				current_frame,
				current_layer,
				asset->frame_rate))
		{
// Can't advance position if cache used.
//printf("File::read_frame %d\n", __LINE__);
			advance_position = 0;
		}
		else
// Need temp
		if(frame->get_color_model() != BC_COMPRESSED &&
			(supported_colormodel != frame->get_color_model() 
			 ||
				(frame->get_color_model() != BC_BGR8888 && 
					(frame->get_w() != asset->width ||
					frame->get_h() != asset->height))))
		{

//printf("File::read_frame %d using temp\n", __LINE__);
// Can't advance position here because it needs to be added to cache
			if(temp_frame)
			{
				if(!temp_frame->params_match(asset->width, asset->height, supported_colormodel))
				{
//temp_debug--;
//printf("File::read_frame %d temp_debug=%d\n", __LINE__, temp_debug);
					delete temp_frame;
					temp_frame = 0;
				}
			}

			if(!temp_frame)
			{
//temp_debug++;
//printf("File::read_frame %d temp_debug=%d\n", __LINE__, temp_debug);
				temp_frame = new VFrame(0,
					-1,
					asset->width,
					asset->height,
					supported_colormodel,
					-1);
			}

//			printf("File::read_frame %d\n", __LINE__);
			temp_frame->copy_stacks(frame);
			file->read_frame(temp_frame);
//for(int i = 0; i < 1000 * 1000; i++) ((float*)temp_frame->get_rows()[0])[i] = 1.0;
// printf("File::read_frame %d %d %d %d %d %d\n", 
// temp_frame->get_color_model(), 
// temp_frame->get_w(),
// temp_frame->get_h(),
// frame->get_color_model(),
// frame->get_w(),
// frame->get_h());
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
				temp_frame->get_w(), 
				temp_frame->get_h(),
				0, 
				0, 
				frame->get_w(), 
				frame->get_h(),
				temp_frame->get_color_model(), 
				frame->get_color_model(),
				0,
				temp_frame->get_w(),
				frame->get_w());
//printf("File::read_frame %d file -> temp -> frame\n", __LINE__);
		}
		else
		{
// Can't advance position here because it needs to be added to cache
			file->read_frame(frame);

// printf("File::read_frame %d reading directly frame=%p colormodel=%d w=%d h=%d\n", 
// __LINE__, 
// frame,
// frame->get_color_model(), 
// frame->get_w(), 
// frame->get_h());
//for(int i = 0; i < 100 * 1000; i++) ((float*)frame->get_rows()[0])[i] = 1.0;
		}

//printf("File::read_frame %d use_cache=%d\n", __LINE__, use_cache);
		if(use_cache) frame_cache->put_frame(frame,
			current_frame,
			current_layer,
			asset->frame_rate,
			1,
			0);
//printf("File::read_frame %d\n", __LINE__);

		if(advance_position) current_frame++;
		if(debug) PRINT_TRACE
		return 0;
	}
	else
		return 1;
}

int File::can_copy_from(Asset *asset, 
	int64_t position, 
	int output_w, 
	int output_h)
{
	if(!asset) return 0;

#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		FileXML xml;
		asset->write(&xml, 1, "");
		xml.terminate_string();
		int buffer_size = strlen(xml.string) + 1 + 
			sizeof(int64_t) +
			sizeof(int) + 
			sizeof(int);
		unsigned char *buffer = new unsigned char[buffer_size];
		*(int64_t*)(buffer) = position;
		*(int*)(buffer + sizeof(int64_t)) = output_w;
		*(int*)(buffer + sizeof(int64_t) + sizeof(int)) = output_h;
		memcpy(buffer + 
			sizeof(int64_t) +
			sizeof(int) + 
			sizeof(int), 
			xml.string, 
			strlen(xml.string) + 1);

		file_fork->send_command(FileFork::CAN_COPY_FROM, 
			buffer, 
			buffer_size);
		int result = file_fork->read_result();
		return result;
	}
#endif


	if(file)
	{
		return asset->width == output_w &&
			asset->height == output_h &&
			file->can_copy_from(asset, position);
	}
	else
		return 0;
}

// Fill in queries about formats when adding formats here.


int File::strtoformat(char *format)
{
	return strtoformat(0, format);
}

int File::strtoformat(ArrayList<PluginServer*> *plugindb, char *format)
{
	if(!strcasecmp(format, _(AC3_NAME))) return FILE_AC3;
	else
	if(!strcasecmp(format, _(SCENE_NAME))) return FILE_SCENE;
	else
	if(!strcasecmp(format, _(WAV_NAME))) return FILE_WAV;
	else
	if(!strcasecmp(format, _(PCM_NAME))) return FILE_PCM;
	else
	if(!strcasecmp(format, _(AU_NAME))) return FILE_AU;
	else
	if(!strcasecmp(format, _(AIFF_NAME))) return FILE_AIFF;
	else
	if(!strcasecmp(format, _(SND_NAME))) return FILE_SND;
	else
	if(!strcasecmp(format, _(PNG_NAME))) return FILE_PNG;
	else
	if(!strcasecmp(format, _(PNG_LIST_NAME))) return FILE_PNG_LIST;
	else
	if(!strcasecmp(format, _(TIFF_NAME))) return FILE_TIFF;
	else
	if(!strcasecmp(format, _(TIFF_LIST_NAME))) return FILE_TIFF_LIST;
	else
	if(!strcasecmp(format, _(JPEG_NAME))) return FILE_JPEG;
	else
	if(!strcasecmp(format, _(JPEG_LIST_NAME))) return FILE_JPEG_LIST;
	else
	if(!strcasecmp(format, _(EXR_NAME))) return FILE_EXR;
	else
	if(!strcasecmp(format, _(EXR_LIST_NAME))) return FILE_EXR_LIST;
	else
	if(!strcasecmp(format, _(FLAC_NAME))) return FILE_FLAC;
	else
	if(!strcasecmp(format, _(CR2_NAME))) return FILE_CR2;
	else
	if(!strcasecmp(format, _(CR2_LIST_NAME))) return FILE_CR2_LIST;
	else
	if(!strcasecmp(format, _(MPEG_NAME))) return FILE_MPEG;
	else
	if(!strcasecmp(format, _(AMPEG_NAME))) return FILE_AMPEG;
	else
	if(!strcasecmp(format, _(VMPEG_NAME))) return FILE_VMPEG;
	else
	if(!strcasecmp(format, _(TGA_NAME))) return FILE_TGA;
	else
	if(!strcasecmp(format, _(TGA_LIST_NAME))) return FILE_TGA_LIST;
	else
	if(!strcasecmp(format, _(MOV_NAME))) return FILE_MOV;
	else
	if(!strcasecmp(format, _(AVI_NAME))) return FILE_AVI;
	else
	if(!strcasecmp(format, _(AVI_LAVTOOLS_NAME))) return FILE_AVI_LAVTOOLS;
	else
	if(!strcasecmp(format, _(AVI_ARNE2_NAME))) return FILE_AVI_ARNE2;
	else
	if(!strcasecmp(format, _(AVI_ARNE1_NAME))) return FILE_AVI_ARNE1;
	else
	if(!strcasecmp(format, _(AVI_AVIFILE_NAME))) return FILE_AVI_AVIFILE;
	else
	if(!strcasecmp(format, _(OGG_NAME))) return FILE_OGG;
	else
	if(!strcasecmp(format, _(VORBIS_NAME))) return FILE_VORBIS;
	else
	if(!strcasecmp(format, _(FFMPEG_NAME))) return FILE_FFMPEG;

	return 0;
}

const char* File::formattostr(int format)
{
	return formattostr(0, format);
}

const char* File::formattostr(ArrayList<PluginServer*> *plugindb, int format)
{
	switch(format)
	{
		case FILE_SCENE:
			return _(SCENE_NAME);
			break;
		case FILE_AC3:
			return _(AC3_NAME);
			break;
		case FILE_WAV:
			return _(WAV_NAME);
			break;
		case FILE_PCM:
			return _(PCM_NAME);
			break;
		case FILE_AU:
			return _(AU_NAME);
			break;
		case FILE_AIFF:
			return _(AIFF_NAME);
			break;
		case FILE_SND:
			return _(SND_NAME);
			break;
		case FILE_PNG:
			return _(PNG_NAME);
			break;
		case FILE_PNG_LIST:
			return _(PNG_LIST_NAME);
			break;
		case FILE_JPEG:
			return _(JPEG_NAME);
			break;
		case FILE_JPEG_LIST:
			return _(JPEG_LIST_NAME);
			break;
		case FILE_CR2:
			return _(CR2_NAME);
			break;
		case FILE_CR2_LIST:
			return _(CR2_LIST_NAME);
			break;
		case FILE_EXR:
			return _(EXR_NAME);
			break;
		case FILE_FLAC:
			return _(FLAC_NAME);
			break;
		case FILE_EXR_LIST:
			return _(EXR_LIST_NAME);
			break;
		case FILE_MPEG:
			return _(MPEG_NAME);
			break;
		case FILE_AMPEG:
			return _(AMPEG_NAME);
			break;
		case FILE_VMPEG:
			return _(VMPEG_NAME);
			break;
		case FILE_TGA:
			return _(TGA_NAME);
			break;
		case FILE_TGA_LIST:
			return _(TGA_LIST_NAME);
			break;
		case FILE_TIFF:
			return _(TIFF_NAME);
			break;
		case FILE_TIFF_LIST:
			return _(TIFF_LIST_NAME);
			break;
		case FILE_MOV:
			return _(MOV_NAME);
			break;
		case FILE_AVI_LAVTOOLS:
			return _(AVI_LAVTOOLS_NAME);
			break;
		case FILE_AVI:
			return _(AVI_NAME);
			break;
		case FILE_AVI_ARNE2:
			return _(AVI_ARNE2_NAME);
			break;
		case FILE_AVI_ARNE1:
			return _(AVI_ARNE1_NAME);
			break;
		case FILE_AVI_AVIFILE:
			return _(AVI_AVIFILE_NAME);
			break;
		case FILE_OGG:
			return _(OGG_NAME);
			break;
		case FILE_VORBIS:
			return _(VORBIS_NAME);
			break;
		case FILE_FFMPEG:
			return _(FFMPEG_NAME);
			break;

		default:
			return _("Unknown");
			break;
	}
	return "Unknown";
}

int File::strtobits(const char *bits)
{
	if(!strcasecmp(bits, _(NAME_8BIT))) return BITSLINEAR8;
	if(!strcasecmp(bits, _(NAME_16BIT))) return BITSLINEAR16;
	if(!strcasecmp(bits, _(NAME_24BIT))) return BITSLINEAR24;
	if(!strcasecmp(bits, _(NAME_32BIT))) return BITSLINEAR32;
	if(!strcasecmp(bits, _(NAME_ULAW))) return BITSULAW;
	if(!strcasecmp(bits, _(NAME_ADPCM))) return BITS_ADPCM;
	if(!strcasecmp(bits, _(NAME_FLOAT))) return BITSFLOAT;
	if(!strcasecmp(bits, _(NAME_IMA4))) return BITSIMA4;
	return BITSLINEAR16;
}

const char* File::bitstostr(int bits)
{
//printf("File::bitstostr\n");
	switch(bits)
	{
		case BITSLINEAR8:
			return (NAME_8BIT);
			break;
		case BITSLINEAR16:
			return (NAME_16BIT);
			break;
		case BITSLINEAR24:
			return (NAME_24BIT);
			break;
		case BITSLINEAR32:
			return (NAME_32BIT);
			break;
		case BITSULAW:
			return (NAME_ULAW);
			break;
		case BITS_ADPCM:
			return (NAME_ADPCM);
			break;
		case BITSFLOAT:
			return (NAME_FLOAT);
			break;
		case BITSIMA4:
			return (NAME_IMA4);
			break;
	}
	return "Unknown";
}



int File::str_to_byteorder(const char *string)
{
	if(!strcasecmp(string, _("Lo Hi"))) return 1;
	return 0;
}

const char* File::byteorder_to_str(int byte_order)
{
	if(byte_order) return _("Lo Hi");
	return _("Hi Lo");
}

int File::bytes_per_sample(int bits)
{
	switch(bits)
	{
		case BITSLINEAR8:
			return 1;
			break;
		case BITSLINEAR16:
			return 2;
			break;
		case BITSLINEAR24:
			return 3;
			break;
		case BITSLINEAR32:
			return 4;
			break;
		case BITSULAW:
			return 1;
			break;
		case BITSIMA4:
			return 1;
			break;
	}
	return 1;
}





int File::get_best_colormodel(int driver)
{
	return get_best_colormodel(asset, driver);
}

int File::get_best_colormodel(Asset *asset, int driver)
{
	switch(asset->format)
	{
		case FILE_MOV:
			return FileMOV::get_best_colormodel(asset, driver);
			break;
		
        case FILE_AVI:
			return FileMOV::get_best_colormodel(asset, driver);
			break;

		case FILE_MPEG:
			return FileMPEG::get_best_colormodel(asset, driver);
			break;

		case FILE_FFMPEG:
			return FileFFMPEG::get_best_colormodel(asset, driver);
			break;
		
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			return FileJPEG::get_best_colormodel(asset, driver);
			break;

		case FILE_EXR:
		case FILE_EXR_LIST:
			return FileEXR::get_best_colormodel(asset, driver);
			break;
		
		case FILE_PNG:
		case FILE_PNG_LIST:
			return FilePNG::get_best_colormodel(asset, driver);
			break;
		
		case FILE_TGA:
		case FILE_TGA_LIST:
			return FileTGA::get_best_colormodel(asset, driver);
			break;
		
		case FILE_CR2:
		case FILE_CR2_LIST:
			return FileCR2::get_best_colormodel(asset, driver);
			break;
	}

	return BC_RGB888;
}


int File::colormodel_supported(int colormodel)
{
#ifdef USE_FILEFORK
	if(!is_fork && file_fork)
	{
		unsigned char buffer[sizeof(int)];
		*(int*)buffer = colormodel;
		file_fork->send_command(FileFork::COLORMODEL_SUPPORTED, 
			buffer, 
			sizeof(int));
		int result = file_fork->read_result();
		return result;
	}
#endif


	if(file)
		return file->colormodel_supported(colormodel);

	return BC_RGB888;
}


int64_t File::get_memory_usage() 
{
	int64_t result = 0;
#ifdef USE_FILEFORK


//printf("File::get_memory_usage %d this=%p is_fork=%d file_fork=%p memory_usage=%lld\n", __LINE__, this, is_fork, file_fork, memory_usage);
	if(!is_fork && file_fork)
 	{
// Return a precalculated value so it doesn't block.
		return memory_usage;

// 		file_fork->send_command(FileFork::GET_MEMORY_USAGE, 
// 			0, 
// 			0);
// 		result = file_fork->read_result();
 	}
#endif



	if(temp_frame) result += temp_frame->get_data_size();
	if(file) result += file->get_memory_usage();
	result += frame_cache->get_memory_usage();
	if(video_thread) result += video_thread->get_memory_usage();

	if(result < MIN_CACHEITEM_SIZE) result = MIN_CACHEITEM_SIZE;
//printf("File::get_memory_usage %d this=%p is_fork=%d file_fork=%p memory_usage=%lld\n", __LINE__, this, is_fork, file_fork, memory_usage);
	return result;
}

FrameCache* File::get_frame_cache()
{
	return frame_cache;
}

int File::supports_video(ArrayList<PluginServer*> *plugindb, char *format)
{
	int i, format_i = strtoformat(plugindb, format);
	
	return supports_video(format_i);
	return 0;
}

int File::supports_audio(ArrayList<PluginServer*> *plugindb, char *format)
{
	int i, format_i = strtoformat(plugindb, format);

	return supports_audio(format_i);
	return 0;
}


int File::supports_video(int format)
{
//printf("File::supports_video %d\n", format);
	switch(format)
	{
		case FILE_OGG:
		case FILE_MOV:
		case FILE_JPEG:
		case FILE_JPEG_LIST:
		case FILE_CR2:
		case FILE_CR2_LIST:
		case FILE_EXR:
		case FILE_EXR_LIST:
		case FILE_PNG:
		case FILE_PNG_LIST:
		case FILE_TGA:
		case FILE_TGA_LIST:
		case FILE_TIFF:
		case FILE_TIFF_LIST:
		case FILE_VMPEG:
		case FILE_AVI_LAVTOOLS:
		case FILE_AVI_ARNE2:
		case FILE_AVI:
		case FILE_AVI_ARNE1:
		case FILE_AVI_AVIFILE:
			return 1;
			break;

		default:
			return 0;
			break;
	}
}

int File::supports_audio(int format)
{
	switch(format)
	{
		case FILE_AC3:
		case FILE_FLAC:
		case FILE_PCM:
		case FILE_WAV:
		case FILE_MOV:
		case FILE_OGG:
		case FILE_VORBIS:
		case FILE_AMPEG:
		case FILE_AU:
		case FILE_AIFF:
		case FILE_SND:
		case FILE_AVI:
		case FILE_AVI_LAVTOOLS:
		case FILE_AVI_ARNE2:
		case FILE_AVI_ARNE1:
		case FILE_AVI_AVIFILE:
			return 1;
		
		default:
			return 0;
			break;
	}
}

const char* File::get_tag(int format)
{
	switch(format)
	{
		case FILE_AC3:          return "ac3";
		case FILE_AIFF:         return "aif";
		case FILE_AMPEG:        return "mp3";
		case FILE_AU:           return "au";
		case FILE_AVI:          return "avi";
		case FILE_EXR:          return "exr";
		case FILE_EXR_LIST:     return "exr";
		case FILE_FLAC:         return "flac";
		case FILE_JPEG:         return "jpg";
		case FILE_JPEG_LIST:    return "jpg";
		case FILE_MOV:          return "mov/mp4";
		case FILE_OGG:          return "ogg";
		case FILE_PCM:          return "pcm";
		case FILE_PNG:          return "png";
		case FILE_PNG_LIST:     return "png";
		case FILE_TGA:          return "tga";
		case FILE_TGA_LIST:     return "tga";
		case FILE_TIFF:         return "tif";
		case FILE_TIFF_LIST:    return "tif";
		case FILE_VMPEG:        return "m2v";
		case FILE_VORBIS:       return "ogg";
		case FILE_WAV:          return "wav";
	}
	return 0;
}









