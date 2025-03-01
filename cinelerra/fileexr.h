/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILEEXR_H
#define FILEEXR_H


#include "file.inc"
#include "filelist.h"
#include "vframe.inc"

class FileEXR : public FileList
{
public:
	FileEXR(Asset *asset, File *file);
	~FileEXR();

// table functions
    FileEXR();
	int check_sig(File *file, const uint8_t *test_data);
    FileBase* create(File *file);
	void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int option_type,
	    const char *locked_compressor);
	int get_best_colormodel(Asset *asset, 
        VideoInConfig *in_config, 
        VideoOutConfig *out_config);
    const char* formattostr(int format);
    const char* get_tag(int format);

//	int colormodel_supported(int colormodel);
	int read_frame_header(char *path);
	int read_frame(VFrame *frame, VFrame *data);
	int64_t get_memory_usage();
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);

// exr_compression values
	enum
	{
		NONE,
		PIZ,
		ZIP,
		ZIPS,
		RLE,
		PXR24
	};

	static const char* compression_to_str(int compression);
	static int str_to_compression(char *string);
	static int compression_to_exr(int compression);

	int native_cmodel;
	int is_yuv;
// floating point YUV source is not supported by Cinelerra
	float *temp_y;
	float *temp_u;
	float *temp_v;
};

class EXRUnit : public FrameWriterUnit
{
public:
	EXRUnit(FileEXR *file, FrameWriter *writer);
	~EXRUnit();
	
	FileEXR *file;
	VFrame *temp_frame;
};



class EXRConfigVideo : public BC_Window
{
public:
	EXRConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~EXRConfigVideo();

	void create_objects();
	int close_event();

	BC_WindowBase *parent_window;
	Asset *asset;
};


class EXRUseAlpha : public BC_CheckBox
{
public:
	EXRUseAlpha(EXRConfigVideo *gui, int x, int y);
	int handle_event();
	EXRConfigVideo *gui;
};

class EXRCompression : public BC_PopupMenu
{
public:
	EXRCompression(EXRConfigVideo *gui, int x, int y, int w);
	void create_objects();
	int handle_event();
	EXRConfigVideo *gui;
};

class EXRCompressionItem : public BC_MenuItem
{
public:
	EXRCompressionItem(EXRConfigVideo *gui, int value);
	int handle_event();
	EXRConfigVideo *gui;
	int value;
};

#endif
