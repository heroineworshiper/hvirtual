
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


#include "asset.h"
#include "filesystem.h"
#include "filexml.h"
#include "indexstate.h"
#include "indexstate.inc"
#include "language.h"

#include <stdio.h>


IndexState::IndexState()
{
	reset();
}

IndexState::~IndexState()
{
	delete [] index_offsets;
	delete [] index_sizes;
// Don't delete index buffer since it is shared with the index thread.
}

void IndexState::reset()
{
	index_status = INDEX_NOTTESTED;
	index_start = old_index_end = index_end = 0;
	index_offsets = 0;
	index_sizes = 0;
	index_zoom = 0;
	index_bytes = 0;
	index_buffer = 0;
	channels = 0;
}

void IndexState::dump()
{
	printf("IndexState::dump this=%p\n", this);
	printf("    channels=%d index_status=%d index_zoom=%lld index_bytes=%lld index_offsets=%p\n",
		channels,
		index_status,
		(long long)index_zoom,
		(long long)index_bytes,
		(void*)index_offsets);
	if(index_sizes)
	{
		printf("    index_sizes=");
		for(int i = 0; i < channels; i++)
			printf("%lld ", (long long)index_sizes[i]);
		printf("\n");
	}
}

void IndexState::copy_from(IndexState *src)
{
	if(this == src) return;

	delete [] index_offsets;
	delete [] index_sizes;
	index_offsets = 0;
	index_sizes = 0;

//printf("Asset::update_index 1 %d\n", index_status);
	index_status = src->index_status;
	index_zoom = src->index_zoom; 	 // zoom factor of index data
	index_start = src->index_start;	 // byte start of index data in the index file
	index_bytes = src->index_bytes;	 // Total bytes in source file for comparison before rebuilding the index
	index_end = src->index_end;
	old_index_end = src->old_index_end;	 // values for index build
	channels = src->channels;

	if(src->index_offsets)
	{
		index_offsets = new int64_t[channels];
		index_sizes = new int64_t[channels];

		int i;
		for(i = 0; i < channels; i++)
		{
// offsets of channels in index file in floats
			index_offsets[i] = src->index_offsets[i];  
			index_sizes[i] = src->index_sizes[i];
		}
	}

// pointer
	index_buffer = src->index_buffer;
}

void IndexState::write_xml(FileXML *file)
{
	file->tag.set_title("INDEX");
	file->tag.set_property("ZOOM", index_zoom);
	file->tag.set_property("BYTES", index_bytes);
	file->append_tag();
	file->append_newline();

	if(index_offsets)
	{
		for(int i = 0; i < channels; i++)
		{
			file->tag.set_title("OFFSET");
			file->tag.set_property("FLOAT", index_offsets[i]);
			file->append_tag();
			file->tag.set_title("SIZE");
			file->tag.set_property("FLOAT", index_sizes[i]);
			file->append_tag();
		}
	}

	file->append_newline();
	file->tag.set_title("/INDEX");
	file->append_tag();
	file->append_newline();
}

void IndexState::read_xml(FileXML *file, int channels)
{
	this->channels = channels;

	delete [] index_offsets;
	delete [] index_sizes;
	index_offsets = new int64_t[channels];
	index_sizes = new int64_t[channels];

	for(int i = 0; i < channels; i++) 
	{
		index_offsets[i] = 0;
		index_sizes[i] = 0;
	}

	int current_offset = 0;
	int current_size = 0;
	int result = 0;

	index_zoom = file->tag.get_property("ZOOM", 1);
	index_bytes = file->tag.get_property("BYTES", (int64_t)0);

	while(!result)
	{
		result = file->read_tag();
		if(!result)
		{
			if(file->tag.title_is("/INDEX"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("OFFSET"))
			{
				if(current_offset < channels)
				{
					index_offsets[current_offset++] = file->tag.get_property("FLOAT", 0);
//printf("Asset::read_xml %d %d\n", current_offset - 1, index_offsets[current_offset - 1]);
				}
			}
			else
			if(file->tag.title_is("SIZE"))
			{
				if(current_size < channels)
				{
					index_sizes[current_size++] = file->tag.get_property("FLOAT", 0);
				}
			}
		}
	}
}

int IndexState::write_index(const char *path, 
	int data_bytes, 
	Asset *asset,
	int64_t length_source)
{

	FILE *file;
	if(!(file = fopen(path, "wb")))
	{
// failed to create it
		printf(_("IndexState::write_index Couldn't write index file %s to disk.\n"), 
			path);
	}
	else
	{
		FileXML xml;
// Pad index start position
		fwrite((char*)&(index_start), sizeof(int64_t), 1, file);

		index_status = INDEX_READY;

// Write asset encoding information in index file.
// This also calls back into index_state to write it.
		if(asset)
		{
			asset->write(&xml, 
				1, 
				"");
		}
		else
		{
// Must write index_state directly.
			write_xml(&xml);
		}

		xml.write_to_file(file);
		index_start = ftell(file);
		fseek(file, 0, SEEK_SET);
// Write index start
		fwrite((char*)&(index_start), sizeof(int64_t), 1, file);
		fseek(file, index_start, SEEK_SET);

// Write index data
		fwrite(index_buffer, 
			data_bytes, 
			1, 
			file);
		fclose(file);

// if the source is newer than the index, fake the index date
		if(asset)
		{
			FileSystem fs;
			int64_t source_date = fs.get_date(asset->path);

			if(fs.get_date(path) < source_date)
			{
				fs.set_date(path, source_date + 1);
			}
		}
	}

// Force reread of header
	index_status = INDEX_NOTTESTED;
	index_end = length_source;
	old_index_end = 0;
	index_start = 0;
}

int64_t IndexState::get_index_offset(int channel)
{
// printf("IndexState::get_index_offset %d channels=%d index_offsets=%p\n",
// __LINE__,
// channels,
// index_offsets);

	if(channel < channels && index_offsets)
		return index_offsets[channel];
	else
		return 0;
}

int64_t IndexState::get_index_size(int channel)
{
	if(channel < channels && index_sizes)
		return index_sizes[channel];
	else
		return 0;
}


