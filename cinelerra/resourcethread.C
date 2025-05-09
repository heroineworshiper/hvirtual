/*
 * CINELERRA
 * Copyright (C) 1997-2022 Adam Williams <broadcast at earthling dot net>
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
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "framecache.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "renderengine.h"
#include "resourcethread.h"
#include "resourcepixmap.h"
#include "samples.h"
#include "timelinepane.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vframe.h"
#include "vrender.h"
#include "wavecache.h"


#include <unistd.h>

ResourceThreadItem::ResourceThreadItem(ResourcePixmap *pixmap, 
	int pane_number,
	Indexable *indexable,
	int data_type,
	int operation_count)
{
	this->pane_number = pane_number;
	this->data_type = data_type;
	this->pixmap = pixmap;
	this->indexable = indexable;

// Assets are garbage collected so they don't need to be replicated.
	this->operation_count = operation_count;
	indexable->Garbage::add_user();
	last = 0;
}

ResourceThreadItem::~ResourceThreadItem()
{
	indexable->Garbage::remove_user();
}







VResourceThreadItem::VResourceThreadItem(ResourcePixmap *pixmap, 
	int pane_number,
	int picon_x, 
	int picon_y, 
	int picon_w,
	int picon_h,
	double frame_rate,
	int64_t position,
	int layer,
	Indexable *indexable,
	int operation_count)
 : ResourceThreadItem(pixmap, 
 	pane_number,
 	indexable, 
	TRACK_VIDEO, 
	operation_count)
{
	this->picon_x = picon_x;
	this->picon_y = picon_y;
	this->picon_w = picon_w;
	this->picon_h = picon_h;
	this->frame_rate = frame_rate;
	this->position = position;
	this->layer = layer;
}

VResourceThreadItem::~VResourceThreadItem()
{
}








AResourceThreadItem::AResourceThreadItem(ResourcePixmap *pixmap, 
	int pane_number,
	Indexable *indexable,
	int x,
	int channel,
	int64_t start,
	int64_t end,
	int operation_count)
 : ResourceThreadItem(pixmap, 
 	pane_number,
 	indexable, 
	TRACK_AUDIO, 
	operation_count)
{
	this->x = x;
	this->channel = channel;
	this->start = start;
	this->end = end;
}

AResourceThreadItem::~AResourceThreadItem()
{
}

















ResourceThread::ResourceThread(MWindow *mwindow, MWindowGUI *gui)
 : Thread(1, 0, 0)
{
//printf("ResourceThread::ResourceThread %d %p\n", __LINE__, this);
	this->mwindow = mwindow;
	this->gui = gui;
	interrupted = 1;
	done = 0;
	temp_picon = 0;
	temp_picon2 = 0;
	draw_lock = new Condition(0, "ResourceThread::draw_lock", 0);
//	interrupted_lock = new Condition(0, "ResourceThread::interrupted_lock", 0);
	item_lock = new Mutex("ResourceThread::item_lock");
	audio_buffer = 0;
	for(int i = 0; i < MAXCHANNELS; i++)
		temp_buffer[i] = 0;
	timer = new Timer;
	prev_x = -1;
	prev_h = 0;
	prev_l = 0;
	operation_count = 0;
	render_engine = 0;
}

ResourceThread::~ResourceThread()
{
	done = 1;
	interrupted = 1;
	draw_lock->unlock();
	Thread::join();


	delete draw_lock;
//	delete interrupted_lock;
	delete item_lock;
	delete temp_picon;
	delete temp_picon2;
	delete audio_buffer;
	for(int i = 0; i < MAXCHANNELS; i++)
		delete temp_buffer[i];
	delete timer;
	delete render_engine;
}

void ResourceThread::create_objects()
{
	Thread::start();
}

void ResourceThread::add_picon(ResourcePixmap *pixmap, 
	int pane_number,
	int picon_x, 
	int picon_y, 
	int picon_w,
	int picon_h,
	double frame_rate,
	int64_t position,
	int layer,
	Indexable *indexable)
{
	item_lock->lock("ResourceThread::item_lock");

	items.append(new VResourceThreadItem(pixmap, 
		pane_number,
		picon_x, 
		picon_y, 
		picon_w,
		picon_h,
		frame_rate,
		position,
		layer,
		indexable,
		operation_count));
	item_lock->unlock();
}

void ResourceThread::add_wave(ResourcePixmap *pixmap,
	int pane_number,
	Indexable *indexable,
	int x,
	int channel,
	int64_t source_start,
	int64_t source_end)
{
	item_lock->lock("ResourceThread::item_lock");

	items.append(new AResourceThreadItem(pixmap, 
		pane_number,
		indexable,
		x,
		channel,
		source_start,
		source_end,
		operation_count));
	item_lock->unlock();
}











void ResourceThread::stop_draw(int reset)
{
	if(!interrupted)
	{
		interrupted = 1;
		item_lock->lock("ResourceThread::stop_draw");

//printf("ResourceThread::stop_draw %d %d\n", __LINE__, reset);
//BC_Signals::dump_stack();
		if(reset) items.remove_all_objects();
		operation_count++;
		item_lock->unlock();
		prev_x = -1;
		prev_h = 0;
		prev_l = 0;
	}
}

void ResourceThread::start_draw()
{
	interrupted = 0;
// Tag last audio item to cause refresh.
	for(int i = items.total - 1; i >= 0; i--)
	{
		ResourceThreadItem *item = items.values[i];
		if(item->data_type == TRACK_AUDIO)
		{
			item->last = 1;
			break;
		}
	}
	timer->update();
	draw_lock->unlock();
}

void ResourceThread::run()
{
	while(!done)
	{

		draw_lock->lock("ResourceThread::run");


		while(!interrupted)
		{
// Pull off item
			item_lock->lock("ResourceThread::run");
			int total_items = items.size();
			ResourceThreadItem *item = 0;
			if(items.size()) 
			{
				item = items.get(0);
				items.remove_number(0);
			}
			item_lock->unlock();

//printf("ResourceThread::run %d %d\n", __LINE__, total_items);
			if(!total_items) break;

			if(item->data_type == TRACK_VIDEO)
			{

				do_video((VResourceThreadItem*)item);
			}
			else
			if(item->data_type == TRACK_AUDIO)
			{
				do_audio((AResourceThreadItem*)item);
			}

			delete item;
		}

// only delete assets when not drawing
		mwindow->age_caches();
	}
}


void ResourceThread::open_render_engine(EDL *nested_edl, 
	int do_audio, 
	int do_video)
{
	if(render_engine && render_engine_id != nested_edl->id)
	{
		delete render_engine;
		render_engine = 0;
	}

	if(!render_engine)
	{
		TransportCommand command;
		if(do_audio)
			command.command = PLAY_FWD;
		else
			command.command = CURRENT_FRAME;
		command.get_edl()->copy_all(nested_edl);
		command.change_type = CHANGE_ALL;
		command.realtime = 0;
		render_engine = new RenderEngine(0, mwindow->preferences);
		render_engine_id == nested_edl->id;
		render_engine->set_vcache(mwindow->video_cache);
		render_engine->set_acache(mwindow->audio_cache);
		render_engine->arm_command(&command);
	}
}

void ResourceThread::do_video(VResourceThreadItem *item)
{
	int source_w = 0;
	int source_h = 0;
	int source_id = -1;
	int source_cmodel = -1;

	if(item->indexable->is_asset)
	{
		Asset *asset = (Asset*)item->indexable;
		source_w = asset->width;
		source_h = asset->height;
		source_id = asset->id;
		source_cmodel = BC_RGB888;
	}
	else
	{
		EDL *nested_edl = (EDL*)item->indexable;
		source_w = nested_edl->session->output_w;
		source_h = nested_edl->session->output_h;
		source_id = nested_edl->id;
		source_cmodel = nested_edl->session->color_model;
	}

// this frame is what the file is loaded into & must use shm
	if(temp_picon &&
		(temp_picon->get_w() != source_w ||
		temp_picon->get_h() != source_h ||
		temp_picon->get_color_model() != source_cmodel))
	{
		delete temp_picon;
		temp_picon = 0;
	}

	if(!temp_picon)
	{
		temp_picon = new VFrame(0, 
			-1,
			source_w, 
			source_h, 
			source_cmodel,
			-1);
	}

// Temporary to copy cached frame to.  This doesn't need shm.
	if(temp_picon2 &&
		(temp_picon2->get_w() != item->picon_w ||
		temp_picon2->get_h() != item->picon_h))
	{
		delete temp_picon2;
		temp_picon2 = 0;
	}

	if(!temp_picon2)
	{
// 		temp_picon2 = new VFrame(0, 
// 			-1,
// 			item->picon_w, 
// 			item->picon_h, 
// 			BC_RGB888,
// 			-1);
        temp_picon2 = new VFrame;
        temp_picon2->set_use_shm(0);
        temp_picon2->reallocate(0, 
	        -1,
	        0,
	        0,
	        0,
	        item->picon_w, 
	        item->picon_h, 
	        BC_RGB888, 
	        -1);
	}



// Search frame cache again.

	VFrame *picon_frame = 0;
	int need_conversion = 0;
	EDL *nested_edl = 0;
	Asset *asset = 0;

	if((picon_frame = mwindow->frame_cache->get_frame_ptr(item->position,
		item->layer,
		item->frame_rate,
		BC_RGB888,
		item->picon_w,
		item->picon_h,
		source_id)) != 0)
	{
		temp_picon2->copy_from(picon_frame);
// Unlock the get_frame_ptr command
		mwindow->frame_cache->unlock();
	}
	else
	if(!item->indexable->is_asset)
	{
		nested_edl = (EDL*)item->indexable;
		open_render_engine(nested_edl, 0, 1);

		int64_t source_position = (int64_t)(item->position *
			nested_edl->session->get_nested_frame_rate() /
			item->frame_rate);
// printf("ResourceThread::do_video %d item->position=%d nested frame rate=%f item frame rate=%f source_position=%d\n",
// __LINE__,
// (int)item->position,
// nested_edl->session->get_nested_frame_rate(),
// item->frame_rate,
// (int)source_position);
		if(render_engine->vrender)
			render_engine->vrender->process_buffer(
				temp_picon, 
				source_position,
				0);

		need_conversion = 1;
	}
	else
	{
		asset = (Asset*)item->indexable;
		File *source = mwindow->video_cache->check_out(asset,
			mwindow->edl);
		if(!source) 
		{
			return;
		}

 		source->set_layer(item->layer);
		int64_t normalized_position = (int64_t)(item->position *
			asset->frame_rate /
			item->frame_rate);
		source->set_video_position(normalized_position, 
			0);

		source->read_frame(temp_picon, 0, 0, 0);
		mwindow->video_cache->check_in(asset);

// don't delete assets after every picon	
//		mwindow->age_caches();
		
		need_conversion = 1;
	}

	if(need_conversion)
	{
// 		picon_frame = new VFrame(0, 
// 			-1,
// 			item->picon_w, 
// 			item->picon_h, 
// 			BC_RGB888,
// 			-1);
// number of shm segments is finite, so must use malloc
        picon_frame = new VFrame;
        picon_frame->set_use_shm(0);
        picon_frame->reallocate(0, 
	        -1,
	        0,
	        0,
	        0,
	        item->picon_w, 
	        item->picon_h, 
	        BC_RGB888, 
	        -1);
        
		cmodel_transfer(picon_frame->get_rows(),
			temp_picon->get_rows(),
			0,
			0,
			0,
            0, // out_a_plane
			0,
			0,
			0,
            0, // in_a_plane
			0, // in_x
			0, // in_y
			temp_picon->get_w(),
			temp_picon->get_h(),
			0,
			0,
			picon_frame->get_w(), 
			picon_frame->get_h(),
			source_cmodel,
			BC_RGB888,
			0,
			temp_picon->get_bytes_per_line(),
			picon_frame->get_bytes_per_line());
		temp_picon2->copy_from(picon_frame);
		mwindow->frame_cache->put_frame(picon_frame, 
			item->position,
			item->layer,
			mwindow->edl->session->frame_rate,
			0,
			item->indexable);
	}

// Allow escape here
	if(interrupted) 
	{
		return;
	}


// Draw the picon
	mwindow->gui->lock_window("ResourceThread::do_video");

// It was interrupted while waiting.
	if(interrupted)
	{
		mwindow->gui->unlock_window();
		return;
	}



// Test for pixmap existence first
	if(item->operation_count == operation_count)
	{
		int exists = 0;
		for(int i = 0; i < gui->resource_pixmaps.total; i++)
		{
			if(gui->resource_pixmaps.values[i] == item->pixmap)
				exists = 1;
		}
		if(exists)
		{
			item->pixmap->draw_vframe(temp_picon2, 
				item->picon_x, 
				item->picon_y, 
				item->picon_w, 
				item->picon_h, 
				0, 
				0);

			mwindow->gui->update(0, IGNORE_THREAD, 0, 0, 0, 0, 0);
		}
	}

	mwindow->gui->unlock_window();
}


#define BUFFERSIZE 65536
void ResourceThread::do_audio(AResourceThreadItem *item)
{
// Search again
	WaveCacheItem *wave_item;
	double high;
	double low;
	const int debug = 0;
	if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
	if((wave_item = mwindow->wave_cache->get_wave(item->indexable->id,
		item->channel,
		item->start,
		item->end)))
	{
		high = wave_item->high;
		low = wave_item->low;
		mwindow->wave_cache->unlock();
	}
	else
	{
		int first_sample = 1;
		int64_t start = item->start;
		int64_t end = item->end;
		if(start == end) end = start + 1;
		
		if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
		for(int64_t sample = start; sample < end; sample++)
		{
			double value;
// Get value from previous buffer
			if(audio_buffer && 
				item->channel == audio_channel &&
				item->indexable->id == audio_asset_id &&
				sample >= audio_start &&
				sample < audio_start + audio_samples)
			{
				;
			}
			else
// Load new buffer
			{
				if(!audio_buffer) audio_buffer = new Samples(BUFFERSIZE);


				int64_t total_samples = item->indexable->get_audio_samples();
				int fragment = BUFFERSIZE;
				if(fragment + sample > total_samples)
					fragment = total_samples - sample;

                if(fragment > 0)
                {
				    if(!item->indexable->is_asset)
				    {
					    if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
					    open_render_engine((EDL*)item->indexable, 1, 0);
					    if(debug) printf("ResourceThread::do_audio %d %p\n", __LINE__, render_engine);
					    if(render_engine->arender)
					    {
						    if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
						    int source_channels = item->indexable->get_audio_channels();
						    if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
						    for(int i = 0; i < MAXCHANNELS; i++)
						    {
							    if(i < source_channels &&
								    !temp_buffer[i])
							    {
								    temp_buffer[i] = new Samples(BUFFERSIZE);
							    }
							    else
							    if(i >= source_channels &&
								    temp_buffer[i])
							    {
								    delete temp_buffer[i];
								    temp_buffer[i] = 0;
							    }
						    }


						    if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
						    render_engine->arender->process_buffer(
							    temp_buffer, 
							    fragment,
							    sample);
						    if(debug) printf("ResourceThread::do_audio %d audio_buffer=%p temp_buffer=%p channel=%d total_samples=%ld sample=%ld fragment=%d\n", 
                                __LINE__, 
                                audio_buffer,
                                temp_buffer,
                                item->channel,
                                total_samples,
                                sample,
                                fragment);
						    memcpy(audio_buffer->get_data(), 
							    temp_buffer[item->channel]->get_data(),
							    fragment * sizeof(double));
						    if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
					    }
					    else
					    {
						    if(debug) printf("ResourceThread::do_audio %d %d\n", __LINE__, fragment);
						    if(fragment > 0) bzero(audio_buffer->get_data(), sizeof(double) * fragment);
						    if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);

					    }
				    } // !item->indexable->is_asset
				    else
				    {
					    File *source = mwindow->audio_cache->check_out(
						    (Asset*)item->indexable,
						    mwindow->edl);
					    if(!source)
						    return;

					    source->set_channel(item->channel);
					    source->set_audio_position(sample);
					    source->read_samples(audio_buffer, fragment);
					    mwindow->audio_cache->check_in((Asset*)item->indexable);
				    }
                } // fragment > 0

		    	if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
				audio_asset_id = item->indexable->id;
				audio_channel = item->channel;
				audio_start = sample;
				audio_samples = fragment;
		    	if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
			}


			value = audio_buffer->get_data()[sample - audio_start];
			if(first_sample)
			{
				high = low = value;
				first_sample = 0;
			}
			else
			{
				if(value > high) 
					high = value;
				else
				if(value < low)
					low = value;
			}
		}
		if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);

// If it's a nested EDL, store all the channels
		mwindow->wave_cache->put_wave(item->indexable,
			item->channel,
			item->start,
			item->end,
			high,
			low);
		if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
	}
	if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);

// Allow escape here
	if(interrupted)
		return;

	if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
// Draw the column
	mwindow->gui->lock_window("ResourceThread::do_audio");
	if(interrupted)
	{
		mwindow->gui->unlock_window();
		return;
	}

	if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);
	if(item->operation_count == operation_count)
	{

// Test for pixmap existence first
		int exists = 0;
		for(int i = 0; i < gui->resource_pixmaps.total; i++)
		{
			if(gui->resource_pixmaps.values[i] == item->pixmap)
				exists = 1;
		}

		if(exists)
		{
			if(prev_x == item->x - 1)
			{
				high = MAX(high, prev_l);
				low = MIN(low, prev_h);
			}
			prev_x = item->x;
			prev_h = high;
			prev_l = low;
			if(gui->pane[item->pane_number])
				item->pixmap->draw_wave(gui->pane[item->pane_number]->canvas, 
					item->x, 
					high, 
					low);
			if(timer->get_difference() > 250 || item->last)
			{
				mwindow->gui->update(0, 3, 0, 0, 0, 0, 0);
				timer->update();
			}
		}
	}
	if(debug) printf("ResourceThread::do_audio %d\n", __LINE__);

	mwindow->gui->unlock_window();

}




