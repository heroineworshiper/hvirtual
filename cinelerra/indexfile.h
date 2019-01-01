
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef INDEXFILE_H
#define INDEXFILE_H

#include "cache.inc"
#include "edit.inc"
#include "file.inc"
#include "guicast.h"
#include "indexable.inc"
#include "indexstate.inc"
#include "indexthread.inc"
#include "mainprogress.inc"
#include "mwindow.inc"
#include "preferences.inc"
#include "renderengine.inc"
#include "resourcepixmap.inc"
#include "samples.inc"
#include "trackcanvas.inc"
#include "bctimer.inc"
#include "tracks.inc"

#include <string>


class IndexFile
{
public:
	IndexFile(MWindow *mwindow);
	IndexFile(MWindow *mwindow, Indexable *indexable);
	~IndexFile();

	void reset();

// Get the index state object from the asset or the nested EDL
	IndexState* get_state();
	static const char* get_source_path(Indexable *indexable);

	int open_index();
	int create_index(MainProgressBar *progress);
	int interrupt_index();
	static void delete_index(Preferences *preferences, 
		Indexable *indexable);
	static int get_index_filename(string *source_path, 
		string *index_directory, 
		string *index_path, 
		const string *input_path);
// get the TOC for formats which use it instead of an index file
	static int get_toc_filename(string *source_path, 
		string *index_directory, 
		string *index_path, 
		const string *input_path);
	void update_edl_asset();
	int redraw_edits(int force);
	int draw_index(TrackCanvas *canvas,
		ResourcePixmap *pixmap, 
		Edit *edit, 
		int x, 
		int w);
	int close_index();
	int remove_index();
	int read_info(Indexable *dst);
	int write_info();

	MWindow *mwindow;
	std::string index_path;
    std::string source_path;
// Object to create an index for
	Indexable *indexable;
	Timer *redraw_timer;

	void update_mainasset();

	int open_file();
	int open_source();
	void close_source();
	int64_t get_required_scale();
// File descriptor for index file.
	FILE *fd;
// what type of file the index is stored in.
    int is_index;
    int is_toc;
// the source file is the table of contents rather than the media
    int is_source_toc;
// File object for source if an asset
	File *source;
// Render engine for source if the source is a nested EDL
	RenderEngine *render_engine;
// Audio cache for render engine
	CICache *cache;
// Number of samples in source.  Calculated in open_source.
	int64_t source_length;
// Number of channels in source.  Calculated in open_source.
	int source_channels;
	int source_samplerate;
	int64_t file_length;   // Length of index file in bytes
	int interrupt_flag;    // Flag set when index building is interrupted
};

#endif
