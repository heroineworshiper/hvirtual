
/*
 * CINELERRA
 * Copyright (C) 2020 Adam Williams <broadcast at earthling dot net>
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

	void reset();
	static int check_sig(Asset *asset);
	int use_path();


// Open file and set asset properties but don't decode.
//	int open_file(int rd, int wr);
//	int close_file();
// Open file and decode.
	int read_frame(VFrame *frame, char *path);
// Get best colormodel for decoding.
	int colormodel_supported(int colormodel);
	static int get_best_colormodel(Asset *asset, int driver);
//	int64_t get_memory_usage();
	int read_frame_header(char *path);

private:
};


#endif
