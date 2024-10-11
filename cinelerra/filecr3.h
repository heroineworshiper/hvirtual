/*
 * CINELERRA
 * Copyright (C) 2020-2022 Adam Williams <broadcast at earthling dot net>
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


// This uses bits of libraw to do the decoding.
// Changes were commented with // CINELERRA

#ifndef FILECR3_H
#define FILECR3_H


#include "filelist.h"

class FileCR3 : public FileList
{
public:
	FileCR3(Asset *asset, File *file);
	~FileCR3();

// table functions
    FileCR3();
	int check_sig(File *file, const uint8_t *test_data);
    FileBase* create(File *file);
	int get_best_colormodel(Asset *asset, VideoInConfig *in_config, VideoOutConfig *out_config);
    const char* formattostr(int format);

	void reset();
	int use_path();


// Open file and set asset properties but don't decode.
//	int open_file(int rd, int wr);
//	int close_file();
// Open file and decode.
	int read_frame(VFrame *frame, char *path);
// Get best colormodel for decoding.
//	int colormodel_supported(int colormodel);
//	int64_t get_memory_usage();
	int read_frame_header(char *path);

private:
};


#endif
