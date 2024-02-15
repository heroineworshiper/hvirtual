/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILEPNG_H
#define FILEPNG_H


#include "file.inc"
#include "filebase.h"
#include "filelist.h"
#include "vframe.inc"

class FilePNG : public FileList
{
public:
	FilePNG(Asset *asset, File *file);
	~FilePNG();

// table functions
    FilePNG();
	int check_sig(File *file, const uint8_t *test_data);
    FileBase* create(File *file);
	void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int option_type,
	    const char *locked_compressor);
	int get_best_colormodel(Asset *asset, int driver);
    const char* formattostr(int format);
    const char* get_tag(int format);

//	int colormodel_supported(int colormodel);
	int read_frame(VFrame *frame, VFrame *data);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	int can_copy_from(Asset *asset, int64_t position);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);

	int read_frame_header(char *path);



// For decoding
	VFrame *temp;
};


class PNGUnit : public FrameWriterUnit
{
public:
	PNGUnit(FilePNG *file, FrameWriter *writer);
	~PNGUnit();
	
	FilePNG *file;
	VFrame *temp_frame;
};

class PNGConfigVideo : public BC_Window
{
public:
	PNGConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~PNGConfigVideo();

	void create_objects();
	int close_event();

	BC_WindowBase *parent_window;
	Asset *asset;
};


class PNGUseAlpha : public BC_CheckBox
{
public:
	PNGUseAlpha(PNGConfigVideo *gui, int x, int y);
	int handle_event();
	PNGConfigVideo *gui;
};


#endif
