
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


#ifndef INDEXSTATE_H
#define INDEXSTATE_H

#include "asset.inc"
#include "filexml.inc"

#include <stdint.h>

// Assets & nested EDLs store this information to have their indexes built
class IndexState
{
public:
	IndexState();
	~IndexState();

	void reset();
	void copy_from(IndexState *src);
// Supply asset to include encoding information in index file.
	void write_xml(FileXML *file);
	void read_xml(FileXML *file, int channels);
	int write_index(const char *path, 
		int data_bytes, 
		Asset *asset,
		int64_t length_source);
	int64_t get_index_offset(int channel);
	int64_t get_index_size(int channel);
	void dump();

// index info
	int index_status;     // Macro from assets.inc
	int64_t index_zoom;      // zoom factor of index data
	int64_t index_start;     // byte start of index data in the index file
// Total bytes in the source file when the index was buillt
	int64_t index_bytes;
	int64_t index_end, old_index_end;    // values for index build
// offsets of channels in index buffer in floats
	int64_t *index_offsets;
// Sizes of channels in index buffer in floats.  This allows
// variable channel size.
	int64_t *index_sizes;
// [ index channel      ][ index channel      ]
// [high][low][high][low][high][low][high][low]
	float *index_buffer;  
// Number of channels our buffers were allocated for
	int channels;
};




#endif

