
/*
 * CINELERRA
 * Copyright (C) 1997-2018 Adam Williams <broadcast at earthling dot net>
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

#include "arender.h"
#include "asset.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "cache.h"
#include "clip.h"
#include "condition.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "indexable.h"
#include "indexfile.h"
#include "indexstate.h"
#include "indexthread.h"
#include "language.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "preferences.h"
#include "renderengine.h"
#include "resourcepixmap.h"
#include "samples.h"
#include "theme.h"
#include "timelinepane.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "vframe.h"


#include <string.h>
#include <unistd.h>

// Use native sampling rates for files so the same index can be used in
// multiple projects.

IndexFile::IndexFile(MWindow *mwindow)
{
//printf("IndexFile::IndexFile 1\n");
	reset();
	this->mwindow = mwindow;
//printf("IndexFile::IndexFile 2\n");
	redraw_timer = new Timer;
}

IndexFile::IndexFile(MWindow *mwindow, 
	Indexable *indexable)
{
//printf("IndexFile::IndexFile 2\n");
	reset();
	this->mwindow = mwindow;
	this->indexable = indexable;
	redraw_timer = new Timer;

	if(indexable)
	{
		indexable->add_user();
		source_channels = indexable->get_audio_channels();
		source_samplerate = indexable->get_sample_rate();
		source_length = indexable->get_audio_samples();
	}
}

IndexFile::~IndexFile()
{
//printf("IndexFile::~IndexFile 1\n");
	delete redraw_timer;
	if(indexable) indexable->remove_user();
	close_source();
}

void IndexFile::reset()
{
	fd = 0;
	source = 0;
	interrupt_flag = 0;
	source_length = 0;
	source_channels = 0;
	indexable = 0;
	render_engine = 0;
	cache = 0;
}

IndexState* IndexFile::get_state()
{
	IndexState *index_state = 0;
	if(indexable) index_state = indexable->index_state;
	return index_state;
}



int IndexFile::open_index()
{
	IndexState *index_state = 0;
	int result = 0;

// use buffer if being built
	index_state = get_state();

	if(index_state->index_status == INDEX_BUILDING)
	{
// use buffer
		result = 0;
	}
	else
	if(!(result = open_file()))
	{
// opened existing file
		if(read_info())
		{
			result = 1;
			close_index();
		}
		else
		{
			index_state->index_status = INDEX_READY;
		}
	}
	else
	{
		result = 1;
	}

	return result;
}

void IndexFile::delete_index(Preferences *preferences, 
	Indexable *indexable)
{
	char index_filename[BCTEXTLEN];
	char source_filename[BCTEXTLEN];
	const char *path = indexable->path;

	get_index_filename(source_filename, 
		preferences->index_directory,
		index_filename, 
		path);
//printf("IndexFile::delete_index %s %s\n", source_filename, index_filename);
	remove(index_filename);
}

int IndexFile::open_file()
{
	int result = 0;
	const int debug = 0;
	const char *path = indexable->path;


//printf("IndexFile::open_file %f\n", indexable->get_frame_rate());

	get_index_filename(source_filename, 
		mwindow->preferences->index_directory,
		index_filename, 
		path);

	if(debug) printf("IndexFile::open_file %d index_filename=%s\n", 
		__LINE__,
		index_filename);
	if(fd = fopen(index_filename, "rb"))
	{
// Index file already exists.
// Get its last date without changing the real asset status.
		Indexable *test_indexable = new Indexable(0);
		if(indexable)
		{
			test_indexable->copy_indexable(indexable);
		}

		read_info(test_indexable);

		FileSystem fs;
		if(fs.get_date(index_filename) < fs.get_date(test_indexable->path))
		{
			if(debug) printf("IndexFile::open_file %d index_date=%lld source_date=%lld\n",
				__LINE__,
				(long long)fs.get_date(index_filename),
				(long long)fs.get_date(test_indexable->path));

// index older than source
			result = 2;
			fclose(fd);
			fd = 0;
		}
		else
		if(fs.get_size(test_indexable->path) != test_indexable->index_state->index_bytes)
		{
// source file is a different size than index source file
			if(debug) printf("IndexFile::open_file %d index_size=%lld source_size=%lld\n",
				__LINE__,
				(long long)test_indexable->index_state->index_bytes,
				(long long)fs.get_size(test_indexable->path));
			result = 2;
			fclose(fd);	
			fd = 0;
		}
		else
		{
			if(debug) printf("IndexFile::open_file %d\n",
				__LINE__);
			fseek(fd, 0, SEEK_END);
			file_length = ftell(fd);
			fseek(fd, 0, SEEK_SET);
			result = 0;
		}
		test_indexable->Garbage::remove_user();
	}
	else
	{
// doesn't exist
		if(debug) printf("IndexFile::open_file %d index_filename=%s doesn't exist\n", 
			__LINE__,
			index_filename);
		result = 1;
	}

	return result;
}

int IndexFile::open_source()
{
//printf("IndexFile::open_source %p %s\n", asset, asset->path);
	int result = 0;
	if(indexable && indexable->is_asset)
	{
		if(!source) source = new File;

		Asset *asset = (Asset*)indexable;
		if(source->open_file(mwindow->preferences, 
			asset, 
			1, 
			0))
		{
			//printf("IndexFile::open_source() Couldn't open %s.\n", asset->path);
			result = 1;
		}
		else
		{
			FileSystem fs;
			asset->index_state->index_bytes = fs.get_size(asset->path);
			result = 0;
			source_length = source->get_audio_length();
		}
	}
	else
	{
		TransportCommand command;
		command.command = NORMAL_FWD;
		command.get_edl()->copy_all((EDL*)indexable);
		command.change_type = CHANGE_ALL;
		command.realtime = 0;
		cache = new CICache(mwindow->preferences);
		render_engine = new RenderEngine(0,
			mwindow->preferences,
			0,
			0,
			0);
		render_engine->set_acache(cache);
		render_engine->arm_command(&command);
		FileSystem fs;
		indexable->index_state->index_bytes = fs.get_size(indexable->path);
	}

	return 0;
}

void IndexFile::close_source()
{
	delete source;
	source = 0;

	delete render_engine;
	render_engine = 0;

	delete cache;
	cache = 0;
}

int64_t IndexFile::get_required_scale()
{
	int64_t result = 1;
	

// get scale of index file
// Total peaks which may be stored in buffer
	int64_t peak_count = mwindow->preferences->index_size / 
		(2 * sizeof(float) * source_channels);
	for(result = 1; 
		source_length / result > peak_count; 
		result *= 2)
		;

// Takes too long to draw from source on a CDROM.  Make indexes for
// everything.

	return result;
}

int IndexFile::get_index_filename(char *source_filename, 
	char *index_directory, 
	char *index_filename, 
	const char *input_filename)
{
// Replace slashes and dots
	int i, j;
	int len = strlen(input_filename);
	for(i = 0, j = 0; i < len; i++)
	{
		if(input_filename[i] != '/' &&
			input_filename[i] != '.')
			source_filename[j++] = input_filename[i];
		else
		{
			if(i > 0)
				source_filename[j++] = '_';
		}
	}
	source_filename[j] = 0;
	FileSystem fs;
	fs.join_names(index_filename, index_directory, source_filename);
	strcat(index_filename, ".idx");
	return 0;
}


int IndexFile::get_toc_filename(string *source_filename, 
	string *index_directory, 
	string *index_filename, 
	const string *input_filename)
{
// Replace slashes and dots
	int i;
	int len = input_filename->length();
	for(i = 0; i < len; i++)
	{
		if(input_filename->at(i) != '/' &&
			input_filename->at(i) != '.')
        {
			source_filename->push_back(input_filename->at(i));
		}
        else
		{
			if(i > 0)
            {
				source_filename->push_back('_');
            }
		}
	}

	FileSystem fs;
	fs.join_names(index_filename, index_directory, source_filename);
	index_filename->append(".toc");
	return 0;
}



int IndexFile::interrupt_index()
{
	interrupt_flag = 1;
	return 0;
}

// Read data into buffers

int IndexFile::create_index(MainProgressBar *progress)
{
	int result = 0;

SET_TRACE

	IndexState *index_state = get_state();

	interrupt_flag = 0;

// open the source file
	if(open_source()) return 1;

SET_TRACE

	get_index_filename(source_filename, 
		mwindow->preferences->index_directory, 
		index_filename, 
		indexable->path);

SET_TRACE

// Some file formats have their own sample index.
// Test for index in stream table of contents
	if(source && !source->get_index(index_filename))
	{
		redraw_edits(1);
	}
	else
// Build index from scratch
	{




		index_state->index_zoom = get_required_scale();
SET_TRACE

// Indexes are now built for everything since it takes too long to draw
// from CDROM source.

// get amount to read at a time in floats
		int64_t buffersize = 65536;
		char string[BCTEXTLEN];
		sprintf(string, _("Creating %s."), index_filename);

		progress->update_title(string);
		progress->update_length(source_length);
		redraw_timer->update();
SET_TRACE

// create the former index thread
		IndexThread *index_thread = new IndexThread(mwindow, 
			this, 
			index_filename, 
			buffersize, 
			source_length);

// current sample in source file
		int64_t position = 0;
		int64_t fragment_size = buffersize;


// pass through file once
// printf("IndexFile::create_index %d source_length=%lld source=%p progress=%p\n", 
// __LINE__, 
// source_length,
// source,
// progress);
SET_TRACE
		while(position < source_length && !result)
		{
SET_TRACE
			if(source_length - position < fragment_size && fragment_size == buffersize) fragment_size = source_length - position;

SET_TRACE
			int cancelled = progress->update(position);
//printf("IndexFile::create_index cancelled=%d\n", cancelled);
SET_TRACE
			if(cancelled || interrupt_flag)
			{
				result = 3;
			}


SET_TRACE
			if(source && !result)
			{
SET_TRACE
				for(int channel = 0; 
					!result && channel < source_channels; 
					channel++)
				{
// Read from source file
					source->set_audio_position(position);
					source->set_channel(channel);

					if(source->read_samples(
						index_thread->buffer_in[channel],
						fragment_size)) 
						result = 1;
				}
SET_TRACE
			}
			else
			if(render_engine && !result)
			{
SET_TRACE
				if(render_engine->arender)
				{
					result = render_engine->arender->process_buffer(
						index_thread->buffer_in, 
						fragment_size,
						position);
				}
				else
				{
					for(int i = 0; i < source_channels; i++)
					{
						bzero(index_thread->buffer_in[i]->get_data(),
							fragment_size * sizeof(double));
					}
				}
SET_TRACE
			}
SET_TRACE

// Release buffer to thread
			if(!result)
			{
				index_thread->process(fragment_size);
				position += fragment_size;
			}
SET_TRACE
		}

		progress->update(position);

		delete index_thread;

	}



	close_source();



	open_index();

	close_index();

	mwindow->edl->set_index_file(indexable);
	return 0;
}



int IndexFile::redraw_edits(int force)
{
	int64_t difference = redraw_timer->get_scaled_difference(1000);

	if(difference > 16 || force)
	{
		IndexState *index_state = get_state();
		redraw_timer->update();
// Can't lock window here since the window is only redrawn when the pixel
// count changes.
		mwindow->gui->lock_window("IndexFile::redraw_edits");
		mwindow->edl->set_index_file(indexable);
		mwindow->gui->draw_indexes(indexable);
		index_state->old_index_end = index_state->index_end;
		mwindow->gui->unlock_window();
	}
	return 0;
}




int IndexFile::draw_index(
	TrackCanvas *canvas,
	ResourcePixmap *pixmap, 
	Edit *edit, 
	int x, 
	int w)
{
	const int debug = 0;
	IndexState *index_state = get_state();
	int pane_number = canvas->pane->number;
//index_state->dump();

SET_TRACE
	if(debug) printf("IndexFile::draw_index %d\n", __LINE__);
// check against index_end when being built
	if(index_state->index_zoom == 0)
	{
		printf(_("IndexFile::draw_index: index has 0 zoom\n"));
		return 0;
	}
	if(debug) printf("IndexFile::draw_index %d\n", __LINE__);

// test channel number
	if(edit->channel > source_channels) return 1;
	if(debug) printf("IndexFile::draw_index %d source_samplerate=%d w=%d samplerate=%lld zoom_sample=%lld\n", 
		__LINE__,
		(int)source_samplerate,
		w,
		(long long)mwindow->edl->session->sample_rate,
		(long long)mwindow->edl->local_session->zoom_sample);

// calculate a virtual x where the edit_x should be in floating point
	double virtual_edit_x = 1.0 * 
		edit->track->from_units(edit->startproject) * 
		mwindow->edl->session->sample_rate /
		mwindow->edl->local_session->zoom_sample - 
		mwindow->edl->local_session->view_start[pane_number];

// samples in segment to draw relative to asset
	double asset_over_session = (double)source_samplerate / 
		mwindow->edl->session->sample_rate;
	int64_t startsource = (int64_t)(((pixmap->pixmap_x - virtual_edit_x + x) * 
		mwindow->edl->local_session->zoom_sample + 
		edit->startsource) * 
		asset_over_session);
// just in case we get a numerical error 
	if (startsource < 0) startsource = 0;
	int64_t length = (int64_t)(w * 
		mwindow->edl->local_session->zoom_sample * 
		asset_over_session);

	if(index_state->index_status == INDEX_BUILDING)
	{
		if(startsource + length > index_state->index_end)
			length = index_state->index_end - startsource;
	}

// length of index to read in samples * 2
	int64_t lengthindex = length / index_state->index_zoom * 2;
// start of data in samples
	int64_t startindex = startsource / index_state->index_zoom * 2;  
// Clamp length of index to read by available data
	if(startindex + lengthindex > index_state->get_index_size(edit->channel))
		lengthindex = index_state->get_index_size(edit->channel) - startindex;


	if(debug) printf("IndexFile::draw_index %d length=%lld index_size=%lld lengthindex=%lld\n", 
		__LINE__,
		(long long)length,
		(long long)index_state->get_index_size(edit->channel),
		(long long)lengthindex);
	if(lengthindex <= 0) return 0;




// Actual length read from file in bytes
	int64_t length_read;   
// Start and length of fragment to read from file in bytes.
	int64_t startfile, lengthfile;
	float *buffer = 0;
	int buffer_shared = 0;
	int i;
	int center_pixel = mwindow->edl->local_session->zoom_track / 2;
	if(mwindow->edl->session->show_titles) center_pixel += mwindow->theme->get_image("title_bg_data")->get_h();
	int miny = center_pixel - mwindow->edl->local_session->zoom_track / 2;
	int maxy = center_pixel + mwindow->edl->local_session->zoom_track / 2;
	int x1 = 0, y1, y2;
// get zoom_sample relative to index zoomx
	double index_frames_per_pixel = mwindow->edl->local_session->zoom_sample / 
		index_state->index_zoom * 
		asset_over_session;

// get channel offset
	startindex += index_state->get_index_offset(edit->channel);


	if(index_state->index_status == INDEX_BUILDING)
	{
// index is in RAM, being built
		buffer = &(index_state->index_buffer[startindex]);
		buffer_shared = 1;
	}
	else
	{
// index is stored in a file
		buffer = new float[lengthindex + 1];
		buffer_shared = 0;
		startfile = index_state->index_start + startindex * sizeof(float);
		lengthfile = lengthindex * sizeof(float);
		length_read = 0;

		if(startfile < file_length)
		{
			fseek(fd, startfile, SEEK_SET);

			length_read = lengthfile;
			if(startfile + length_read > file_length)
				length_read = file_length - startfile;

			int temp = fread(buffer, length_read + sizeof(float), 1, fd);
		}

		if(length_read < lengthfile)
			for(i = length_read / sizeof(float); 
				i < lengthfile / sizeof(float); 
				i++)
				buffer[i] = 0;
	}



	canvas->set_color(mwindow->theme->audio_color);


	double current_frame = 0;
	float highsample = buffer[0];
	float lowsample = buffer[1];
	int prev_y1 = center_pixel;
	int prev_y2 = center_pixel;
	int first_frame = 1;
SET_TRACE

	for(int bufferposition = 0; 
		bufferposition < lengthindex; 
		bufferposition += 2)
	{
		if(current_frame >= index_frames_per_pixel)
		{
			int next_y1 = (int)(center_pixel - highsample * mwindow->edl->local_session->zoom_y / 2);
			int next_y2 = (int)(center_pixel - lowsample * mwindow->edl->local_session->zoom_y / 2);
			if(next_y1 < 0) next_y1 = 0;
			if(next_y1 > canvas->get_h()) next_y1 = canvas->get_h();
			if(next_y2 < 0) next_y2 = 0;
			if(next_y2 > canvas->get_h()) next_y2 = canvas->get_h();
			if(next_y2 > center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1) 
				next_y2 = center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1;
			if(next_y1 > center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1) 
				next_y1 = center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1;

			int y1 = next_y1;
			int y2 = next_y2;




//SET_TRACE
// A different algorithm has to be used if it's 1 sample per pixel and the
// index is used.  Now the min and max values are equal so we join the max samples.
			if(mwindow->edl->local_session->zoom_sample == 1)
			{
				canvas->draw_line(x1 + x - 1, prev_y1, x1 + x, y1, pixmap);
			}
			else
			{
// Extend line height if it doesn't connect to previous line
				if(!first_frame)
				{
					if(y1 > prev_y2) y1 = prev_y2 + 1;
					if(y2 < prev_y1) y2 = prev_y1 - 1;
				}
				else
				{
					first_frame = 0;
				}



				canvas->draw_line(x1 + x, y1, x1 + x, y2, pixmap);
			}
			current_frame -= index_frames_per_pixel;
			x1++;
			prev_y1 = next_y1;
			prev_y2 = next_y2;
			highsample = buffer[bufferposition];
			lowsample = buffer[bufferposition + 1];
		}

		current_frame++;
		highsample = MAX(highsample, buffer[bufferposition]);
		lowsample = MIN(lowsample, buffer[bufferposition + 1]);
	}
SET_TRACE

// Get last column
	if(current_frame)
	{
		y1 = (int)(center_pixel - highsample * mwindow->edl->local_session->zoom_y / 2);
		y2 = (int)(center_pixel - lowsample * mwindow->edl->local_session->zoom_y / 2);
		canvas->draw_line(x1 + x, y1, x1 + x, y2, pixmap);
	}

SET_TRACE



	if(!buffer_shared) delete [] buffer;
SET_TRACE
	if(debug) printf("IndexFile::draw_index %d\n", __LINE__);
	return 0;
}

int IndexFile::close_index()
{
	if(fd)
	{
		fclose(fd);
		fd = 0;
	}
}

int IndexFile::remove_index()
{
	IndexState *index_state = get_state();
	if(index_state->index_status == INDEX_READY || 
		index_state->index_status == INDEX_NOTTESTED)
	{
		close_index();
		remove(index_filename);
	}
}

int IndexFile::read_info(Indexable *test_indexable)
{
	const int debug = 0;

// Store format in actual asset.
// If it's a nested EDL, we never want the format, just the index info.
	if(!test_indexable) test_indexable = indexable;


	IndexState *index_state = 0;
	if(test_indexable) 
		index_state = test_indexable->index_state;
	else
		return 1;
	

	if(index_state->index_status == INDEX_NOTTESTED)
	{
// read start of index data
		int temp = fread((char*)&(index_state->index_start), sizeof(int64_t), 1, fd);
//printf("IndexFile::read_info %d %f\n", __LINE__, test_indexable->get_frame_rate());

		if(!temp) return 1;
// read test_indexable info from index
		char *data;
		
		data = new char[index_state->index_start];
		temp = fread(data, index_state->index_start - sizeof(int64_t), 1, fd);
		if(!temp) return 1;

		data[index_state->index_start - sizeof(int64_t)] = 0;
		FileXML xml;
		xml.read_from_string(data);
		delete [] data;



// Read the file format & index state.
		if(test_indexable->is_asset)
		{
			Asset *asset = (Asset*)test_indexable;
			asset->read(&xml);

//printf("IndexFile::read_info %d %f\n", __LINE__, asset->get_frame_rate());

			if(asset->format == FILE_UNKNOWN)
			{
if(debug) printf("IndexFile::read_info %d\n", __LINE__);
				return 1;
			}
		}
		else
		{
// Read only the index state for a nested EDL
			int result = 0;
if(debug) printf("IndexFile::read_info %d\n", __LINE__);
			while(!result)
			{
				result = xml.read_tag();
				if(!result)
				{
					if(xml.tag.title_is("INDEX"))
					{
						index_state->read_xml(&xml, source_channels);
if(debug) printf("IndexFile::read_info %d\n", __LINE__);
if(debug) index_state->dump();
						result = 1;
					}
				}
			}
		}
	}

	return 0;
}








