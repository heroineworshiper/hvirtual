/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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


#ifndef FILEFORK_H
#define FILEFORK_H

#include "file.inc"
#include "fileserver.inc"
#include "forkwrapper.h"

// This is an object created by File which runs in a separate process to 
// actually interface the data.  File functions redirect to this process.
// This is to isolate file format crashes.



#ifdef USE_FILEFORK
class FileFork : public ForkWrapper
{
public:
	FileFork(FileServer *server);
	virtual ~FileFork();
	
	void init_child();
	int handle_command();

// Instance of file that does the actual work.	
	File *file;
// If this is a dummy filefork, the pointer of the real filefork in the fileserver
// memory space.
	FileFork *real_fork;
	FileServer *server;

// Command tokens
	enum
	{
		NO_COMMAND,
		OPEN_FILE,
		SET_PROCESSORS,
		SET_PRELOAD,
		SET_SUBTITLE,
		SET_INTERPOLATE_RAW,
		SET_WHITE_BALANCE_RAW,
		SET_CACHE_FRAMES,
		PURGE_CACHE,
		CLOSE_FILE,
		GET_INDEX,           // 10
		START_VIDEO_THREAD,
		START_AUDIO_THREAD,
		START_VIDEO_DECODE_THREAD,
		STOP_AUDIO_THREAD,
		STOP_VIDEO_THREAD,
		SET_CHANNEL,
		SET_LAYER,
		GET_AUDIO_LENGTH,
		GET_VIDEO_LENGTH,
		GET_AUDIO_POSITION,       // 20
		GET_VIDEO_POSITION,
		SET_AUDIO_POSITION,
		SET_VIDEO_POSITION,
		WRITE_SAMPLES,
		WRITE_FRAMES,
		WRITE_AUDIO_BUFFER,
		WRITE_VIDEO_BUFFER,
		GET_AUDIO_BUFFER,
		GET_VIDEO_BUFFER,
		READ_SAMPLES,              // 30
		READ_COMPRESSED_FRAME,
		COMPRESSED_FRAME_SIZE,
		READ_FRAME,
		CAN_COPY_FROM,
		COLORMODEL_SUPPORTED,
		GET_MEMORY_USAGE,
		SET_CACHE,
        
// progress bar commands that are packed into send_result by the file fork
        START_PROGRESS = 0x100,
        UPDATE_PROGRESS, 
        UPDATE_PROGRESS_TITLE,
        PROGRESS_CANCELED,
        STOP_PROGRESS,

// read_frame commands that are packed into send_result
        FILE_READ_TEMP // the read_frame requested a temporary
	};
};

#endif // USE_FILEFORK


#endif



