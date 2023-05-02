/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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

// NOT USED
// an attempt to get frame accurate seeking with MKV, which ended up
// showing MKV doesn't have the required information in the 1st place.



#ifndef FILEMKV_H
#define FILEMKV_H




#include "filebase.h"
#include "file.inc"

class FileMKVPriv;

class FileMKV : public FileBase
{
public:
	FileMKV(Asset *asset, File *file);
	~FileMKV();


	int reset_parameters_derived();
	static int check_sig(Asset *asset);
	int open_file(int rd, int wr);
	int close_file();
	void format_to_asset();
	int64_t get_video_length();
	int64_t get_audio_length();
	int set_video_position(int64_t x);
	int set_audio_position(int64_t x);
	int read_frame(VFrame *frame);
	int read_samples(double *buffer, int64_t len);
	int64_t get_memory_usage();
    
// create private bits without creating header file dependencies
    void *priv;
};



#endif




