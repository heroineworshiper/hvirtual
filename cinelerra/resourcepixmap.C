
/*
 * CINELERRA
 * Copyright (C) 1997-2019 Adam Williams <broadcast at earthling dot net>
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
#include "aedit.h"
#include "asset.h"
#include "asset.inc"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "colormodels.h"
#include "datatype.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "framecache.h"
#include "indexfile.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "renderengine.h"
#include "resourcethread.h"
#include "resourcepixmap.h"
#include "samples.h"
#include "theme.h"
#include "timelinepane.h"
#include "track.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vedit.h"
#include "vframe.h"
#include "wavecache.h"


ResourcePixmap::ResourcePixmap(MWindow *mwindow, 
	MWindowGUI *gui, 
	Edit *edit, 
	int pane_number,
	int w, 
	int h)
 : BC_Pixmap(gui, w, h)
{
	reset();

	this->mwindow = mwindow;
	this->gui = gui;
	this->pane_number = pane_number;
	startsource = edit->startsource;
	data_type = edit->track->data_type;
	if(edit->asset)
	{
		source_framerate = edit->asset->frame_rate;
		source_samplerate = edit->asset->sample_rate;
	}
	else
	if(edit->nested_edl)
	{
		source_framerate = edit->nested_edl->session->frame_rate;
		source_samplerate = edit->nested_edl->session->sample_rate;
	}
	
	project_framerate = edit->edl->session->frame_rate;
	project_samplerate = edit->edl->session->sample_rate;
	edit_id = edit->id;
}

ResourcePixmap::~ResourcePixmap()
{
}


void ResourcePixmap::reset()
{
	edit_x = 0;
	pixmap_x = 0;
	pixmap_w = 0;
	pixmap_h = 0;
	zoom_sample = 0;
	zoom_track = 0;
	zoom_y = 0;
	visible = 1;
}
	
void ResourcePixmap::resize(int w, int h)
{
	int new_w = (w > get_w()) ? w : get_w();
	int new_h = (h > get_h()) ? h : get_h();

	BC_Pixmap::resize(new_w, new_h);
}


void ResourcePixmap::draw_data(TrackCanvas *canvas,
	Edit *edit,
	int64_t edit_x,
	int64_t edit_w, 
	int64_t pixmap_x, 
	int64_t pixmap_w,
	int64_t pixmap_h,
	int mode,
	int indexes_only)
{
// Get new areas to fill in relative to pixmap
// Area to redraw relative to pixmap
	int refresh_x = 0;
	int refresh_w = 0;

// Ignore if called by resourcethread.
//	if(mode == IGNORE_THREAD) return;

	int y = 0;
	if(mwindow->edl->session->show_titles) 
        y += mwindow->theme->get_image("title_bg_data")->get_h();
	Track *track = edit->edits->track;


// If want indexes only & index can't be drawn, don't do anything.
	int need_redraw = 0;
	int64_t index_zoom = 0;
	Indexable *indexable = 0;
	if(edit->asset) indexable = edit->asset;
	if(edit->nested_edl) indexable = edit->nested_edl;
	if(mwindow->edl->session->show_assets &&
        indexable && indexes_only)
	{
		IndexFile indexfile(mwindow, indexable);
		if(!indexfile.open_index())
		{
			index_zoom = indexable->index_state->index_zoom;
			indexfile.close_index();
		}

		if(index_zoom)
		{
			if(data_type == TRACK_AUDIO)
			{
				double asset_over_session = (double)indexable->get_sample_rate() / 
					mwindow->edl->session->sample_rate;
					asset_over_session;
				if(index_zoom <= mwindow->edl->local_session->zoom_sample *
					asset_over_session)
					need_redraw = 1;
			}
		}

		if(!need_redraw)
			return;
	}


// Redraw everything
/* Incremental drawing is not possible with resource thread */
// Redraw the whole thing.
	refresh_x = 0;
	refresh_w = pixmap_w;

// Update pixmap settings
	this->edit_id = edit->id;
	this->startsource = edit->startsource;

	if(edit->asset)
		this->source_framerate = edit->asset->frame_rate;
	else
	if(edit->nested_edl)
		this->source_framerate = edit->nested_edl->session->frame_rate;

	if(edit->asset)
		this->source_samplerate = edit->asset->sample_rate;
	else
	if(edit->nested_edl)
		this->source_samplerate = edit->nested_edl->session->sample_rate;

	this->project_framerate = edit->edl->session->frame_rate;
	this->project_samplerate = edit->edl->session->sample_rate;
	this->edit_x = edit_x;
	this->pixmap_x = pixmap_x;
	this->pixmap_w = pixmap_w;
	this->pixmap_h = pixmap_h;
	this->zoom_sample = mwindow->edl->local_session->zoom_sample;
	this->zoom_track = mwindow->edl->local_session->zoom_track;
	this->zoom_y = mwindow->edl->local_session->zoom_y;



    if(mwindow->edl->session->show_assets)
    {
// Draw background image
	    if(refresh_w > 0)
		    mwindow->theme->draw_resource_bg(canvas,
			    this, 
			    edit_x,
			    edit_w,
			    pixmap_x,
			    refresh_x, 
			    y,
			    refresh_x + refresh_w,
			    mwindow->edl->local_session->zoom_track + y);
//printf("ResourcePixmap::draw_data 70\n");


// Draw media which already exists
	    if(track->draw)
	    {
		    switch(track->data_type)
		    {
			    case TRACK_AUDIO:
				    draw_audio_resource(canvas,
					    edit, 
					    refresh_x, 
					    refresh_w);
				    break;

			    case TRACK_VIDEO:
				    draw_video_resource(canvas,
					    edit, 
					    edit_x, 
					    edit_w, 
					    pixmap_x,
					    pixmap_w,
					    refresh_x, 
					    refresh_w,
					    mode);
				    break;
		    }
	    }
    }

// Draw title
SET_TRACE
	if(mwindow->edl->session->show_titles)
		draw_title(canvas, 
			edit, 
			edit_x, 
			edit_w, 
			pixmap_x, 
			pixmap_w);
SET_TRACE
}

void ResourcePixmap::draw_title(TrackCanvas *canvas,
	Edit *edit,
	int64_t edit_x, 
	int64_t edit_w, 
	int64_t pixmap_x, 
	int64_t pixmap_w)
{
// coords relative to pixmap
	int64_t total_x = edit_x - pixmap_x, total_w = edit_w;
	int64_t x = total_x, w = total_w;
	int left_margin = 10;

	if(x < 0) 
	{
		w -= -x;
		x = 0;
	}
	if(w > pixmap_w) w -= w - pixmap_w;

	canvas->draw_3segmenth(x, 
		0, 
		w, 
		total_x,
		total_w,
		mwindow->theme->get_image("title_bg_data"),
		this);

//	if(total_x > -BC_INFINITY)
	{
		char title[BCTEXTLEN];
		char channel[BCTEXTLEN];
		title[0] = 0;
		channel[0] = 0;
		FileSystem fs;

		if(edit->user_title[0])
			strcpy(title, edit->user_title);
		else
		if(edit->nested_edl)
		{
//printf("ResourcePixmap::draw_title %s\n", edit->nested_edl->project_path);
			fs.extract_name(title, edit->nested_edl->path);

// EDLs only have 1 video output
			if(edit->track->data_type == TRACK_AUDIO)
			{
				sprintf(channel, " #%d", edit->channel + 1);
				strcat(title, channel);
			}
		}
		else
		if(edit->asset)
		{
			fs.extract_name(title, edit->asset->path);
			sprintf(channel, " #%d", edit->channel + 1);
			strcat(title, channel);
		}

		canvas->set_color(mwindow->theme->title_color);
		canvas->set_font(mwindow->theme->title_font);
		
// Justify the text on the left boundary of the edit if it is visible.
// Otherwise justify it on the left side of the screen.
		int text_x = total_x + left_margin;
		text_x = MAX(left_margin, text_x);
//printf("ResourcePixmap::draw_title 1 %d\n", text_x);
		canvas->draw_text(text_x, 
			canvas->get_text_ascent(mwindow->theme->title_font) + 2, 
			title,
			strlen(title),
			this);
	}
}


int ResourcePixmap::calculate_center_pixel()
{
   	int center_pixel = mwindow->edl->local_session->zoom_track / 2;
	if(mwindow->edl->session->show_titles) center_pixel += mwindow->theme->get_image("title_bg_data")->get_h();
    return center_pixel;
}

// Need to draw one more x
void ResourcePixmap::draw_audio_resource(TrackCanvas *canvas,
	Edit *edit, 
	int x, 
	int w)
{
	if(w <= 0) return;
	if(!edit->asset && !edit->nested_edl) return;
	Indexable *indexable = 0;
	if(edit->asset) indexable = edit->asset;
	if(edit->nested_edl) indexable = edit->nested_edl;
// printf("ResourcePixmap::draw_audio_resource %d x=%d w=%d\n",
// __LINE__,
// x,
// w);
SET_TRACE

	IndexState *index_state = indexable->index_state;
	double asset_over_session = (double)indexable->get_sample_rate() / 
		mwindow->edl->session->sample_rate;


// draw zero crossing
	int center_pixel = calculate_center_pixel();
// printf("ResourcePixmap::draw_audio_resource %d x=%d w=%d y=%d\n", 
// __LINE__,
// x,
// w,
// center_pixel);
    canvas->set_line_dashes(1);
    canvas->set_color(mwindow->theme->zero_crossing_color);
    canvas->draw_line(x, center_pixel, x + w, center_pixel, this);
    canvas->set_line_dashes(0);

// Develop strategy for drawing
// printf("ResourcePixmap::draw_audio_resource %d index_state=%p index_status=%d\n", 
// __LINE__, 
// index_state,
// index_state->index_status);
	switch(index_state->index_status)
	{
		case INDEX_NOTTESTED:
			return;
			break;
// Disabled.  All files have an index.
//		case INDEX_TOOSMALL:
//			draw_audio_source(canvas, edit, x, w);
//			break;
		case INDEX_BUILDING:
		case INDEX_READY:
		{
			IndexFile indexfile(mwindow, indexable);
			if(!indexfile.open_index())
			{
				if(index_state->index_zoom > 
						mwindow->edl->local_session->zoom_sample * 
						asset_over_session)
				{
//printf("ResourcePixmap::draw_audio_resource %d\n", __LINE__);

					draw_audio_source(canvas, edit, x, w);
				}
				else
				{
//printf("ResourcePixmap::draw_audio_resource %d\n", __LINE__);
					indexfile.draw_index(canvas, 
						this, 
						edit, 
						x, 
						w);
SET_TRACE
				}

				indexfile.close_index();
SET_TRACE
			}
			break;
		}
	}

}


















void ResourcePixmap::draw_audio_source(TrackCanvas *canvas,
	Edit *edit, 
	int x, 
	int w)
{
	w++;
	Indexable *indexable = edit->get_source();
	double asset_over_session = (double)indexable->get_sample_rate() / 
		mwindow->edl->session->sample_rate;
	int source_len = w * mwindow->edl->local_session->zoom_sample;
	int center_pixel = calculate_center_pixel();

// Single sample zoom
	if(mwindow->edl->local_session->zoom_sample == 1)
	{
		int64_t source_start = (int64_t)(((pixmap_x - edit_x + x) * 
			mwindow->edl->local_session->zoom_sample + edit->startsource) *
			asset_over_session);
		double oldsample, newsample;
		int total_source_samples = (int)((double)(source_len + 1) * 
			asset_over_session);
		Samples *buffer = new Samples(total_source_samples);
		int result = 0;
		canvas->set_color(mwindow->theme->audio_color);

		if(indexable->is_asset)
		{
			File *source = mwindow->audio_cache->check_out(edit->asset, mwindow->edl);

			if(!source)
			{
				printf(_("ResourcePixmap::draw_audio_source: failed to check out %s for drawing.\n"), edit->asset->path);
				return;
			}


			source->set_audio_position(source_start);
			source->set_channel(edit->channel);
			result = source->read_samples(buffer, total_source_samples);
			mwindow->audio_cache->check_in(edit->asset);
		}
		else
		{
			if(mwindow->gui->render_engine && 
				mwindow->gui->render_engine_id != indexable->id)
			{
				delete mwindow->gui->render_engine;
				mwindow->gui->render_engine = 0;
			}

			if(!mwindow->gui->render_engine)
			{
				TransportCommand command;
				command.command = NORMAL_FWD;
				command.get_edl()->copy_all(edit->nested_edl);
				command.change_type = CHANGE_ALL;
				command.realtime = 0;
				mwindow->gui->render_engine = new RenderEngine(0,
					mwindow->preferences);
				mwindow->gui->render_engine_id == edit->nested_edl->id;
				mwindow->gui->render_engine->set_acache(mwindow->audio_cache);
				mwindow->gui->render_engine->arm_command(&command);
			}

			Samples *temp_buffer[MAX_CHANNELS];
			bzero(temp_buffer, MAX_CHANNELS * sizeof(double*));
			for(int i = 0; i < indexable->get_audio_channels(); i++)
			{
				temp_buffer[i] = new Samples(total_source_samples);
			}

			if(mwindow->gui->render_engine->arender)
			{
				mwindow->gui->render_engine->arender->process_buffer(
					temp_buffer, 
					total_source_samples,
					source_start);
				memcpy(buffer->get_data(), 
					temp_buffer[edit->channel]->get_data(), 
					total_source_samples * sizeof(double));
			}

			for(int i = 0; i < indexable->get_audio_channels(); i++)
			{
				delete temp_buffer[i];
			}
		}
		
		
		
		if(!result)
		{
			oldsample = newsample = *buffer->get_data();
			for(int x1 = x, x2 = x + w, i = 0; 
				x1 < x2; 
				x1++, i++)
			{
				oldsample = newsample;
				newsample = buffer->get_data()[(int)(i * asset_over_session)];
				int y1 = (int)(center_pixel - oldsample * mwindow->edl->local_session->zoom_y / 2);
				int y2 = (int)(center_pixel - newsample * mwindow->edl->local_session->zoom_y / 2);
				if(y1 < 0) y1 = 0;
				if(y1 > canvas->get_h()) y1 = canvas->get_h();
				if(y2 < 0) y2 = 0;
				if(y2 > canvas->get_h()) y2 = canvas->get_h();
				if(y1 > center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1)
					y1 = center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1;
				if(y2 > center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1)
					y2 = center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1;

//printf("ResourcePixmap::draw_audio_source %d %d %d\n", __LINE__, y1, y2);
				
				canvas->draw_line(x1 - 1, 
					y1,
					x1,
					y2,
					this);
			}
		}

		delete buffer;
		canvas->test_timer();
	}
	else
// Multiple sample zoom
	{
		int first_pixel = 1;
		int prev_y1 = -1;
		int prev_y2 = -1;
		int y1;
		int y2;
		int x2 = x + w;

		canvas->set_color(mwindow->theme->audio_color);
// Draw each pixel from the cache
//printf("ResourcePixmap::draw_audio_source %d x=%d w=%d\n", __LINE__, x, w);
		while(x < x2)
		{
// Starting sample of pixel relative to asset rate.
			int64_t source_start = (int64_t)(((pixmap_x - edit_x + x) * 
				mwindow->edl->local_session->zoom_sample + edit->startsource) *
				asset_over_session);
			int64_t source_end = (int64_t)(((pixmap_x - edit_x + x + 1) * 
				mwindow->edl->local_session->zoom_sample + edit->startsource) *
				asset_over_session);
			WaveCacheItem *item = mwindow->wave_cache->get_wave(indexable->id,
					edit->channel,
					source_start,
					source_end);
			if(item)
			{
//printf("ResourcePixmap::draw_audio_source %d\n", __LINE__);
				y1 = (int)(center_pixel - 
					item->low * mwindow->edl->local_session->zoom_y / 2);
				y2 = (int)(center_pixel - 
					item->high * mwindow->edl->local_session->zoom_y / 2);
				if(y1 > center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1)
					y1 = center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1;
				if(y2 > center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1)
					y2 = center_pixel + mwindow->edl->local_session->zoom_y / 2 - 1;

				if(first_pixel)
				{
					canvas->draw_line(x, 
						y1,
						x,
						y2,
						this);
					first_pixel = 0;
				}
				else
				{
					canvas->draw_line(x, 
						MIN(y1, prev_y2),
						x,
						MAX(y2, prev_y1),
						this);
				}
				
				

//printf("ResourcePixmap::draw_audio_source %d %d %d %d\n", __LINE__, x, y1, y2);

				prev_y1 = y1;
				prev_y2 = y2;
				first_pixel = 0;
				mwindow->wave_cache->unlock();
			}
			else
			{
//printf("ResourcePixmap::draw_audio_source %d\n", __LINE__);
				first_pixel = 1;
				gui->resource_thread->add_wave(this,
					canvas->pane->number,
					indexable,
					x,
					edit->channel,
					source_start,
					source_end);
			}

			x++;
		}
	}

}



void ResourcePixmap::draw_wave(TrackCanvas *canvas,
	int x, 
	double high, 
	double low)
{
	int top_pixel = 0;
	if(mwindow->edl->session->show_titles) 
		top_pixel = mwindow->theme->get_image("title_bg_data")->get_h();
	int center_pixel = mwindow->edl->local_session->zoom_track / 2 + top_pixel;
	int bottom_pixel = top_pixel + mwindow->edl->local_session->zoom_track;
	int y1 = (int)(center_pixel - 
		low * mwindow->edl->local_session->zoom_y / 2);
	int y2 = (int)(center_pixel - 
		high * mwindow->edl->local_session->zoom_y / 2);
	CLAMP(y1, top_pixel, bottom_pixel);
	CLAMP(y2, top_pixel, bottom_pixel);
	canvas->set_color(mwindow->theme->audio_color);
	canvas->draw_line(x, 
		y1,
		x,
		y2,
		this);
}




















void ResourcePixmap::draw_video_resource(TrackCanvas *canvas,
	Edit *edit, 
	int64_t edit_x, 
	int64_t edit_w, 
	int64_t pixmap_x,
	int64_t pixmap_w,
	int refresh_x, 
	int refresh_w,
	int mode)
{
//PRINT_TRACE
//BC_Signals::dump_stack();

// pixels spanned by a picon
	int64_t picon_w = Units::round(edit->picon_w());
	int64_t picon_h = edit->picon_h();


// Don't draw video if picon is bigger than edit
	if(picon_w > edit_w) return;

// pixels spanned by a frame
	double frame_w = edit->frame_w();

// Frames spanned by a picon
	double frames_per_picon = edit->frames_per_picon();

// Current pixel relative to pixmap
	int x = 0;
	int y = 0;
	if(mwindow->edl->session->show_titles) 
		y += mwindow->theme->get_image("title_bg_data")->get_h();
// Frame in project touched by current pixel
	int64_t project_frame;

// Get first frame touched by x and fix x to start of frame
	if(frames_per_picon > 1)
	{
		int picon = Units::to_int64(
			(double)((int64_t)refresh_x + pixmap_x - edit_x) / 
			picon_w);
		x = picon_w * picon + edit_x - pixmap_x;
		project_frame = Units::to_int64((double)picon * frames_per_picon);
	}
	else
	{
		project_frame = Units::to_int64((double)((int64_t)refresh_x + pixmap_x - edit_x) / 
			frame_w);
		x = Units::round((double)project_frame * frame_w + edit_x - pixmap_x);
 	}


// Draw only cached frames
	while(x < refresh_x + refresh_w)
	{
		int64_t source_frame = project_frame + edit->startsource;
		VFrame *picon_frame = 0;
		Indexable *indexable = edit->get_source();
		int use_cache = 0;
		int id = -1;

		id = indexable->id;

		if(id >= 0)
		{
			picon_frame = mwindow->frame_cache->get_frame_ptr(source_frame,
				edit->channel,
				mwindow->edl->session->frame_rate,
				BC_RGB888,
				picon_w,
				picon_h,
				id);
		}
// printf("ResourcePixmap::draw_video_resource %d source_frame=%ld picon_frame=%p\n", 
// __LINE__, 
// source_frame,
// picon_frame);

		if(picon_frame != 0)
		{
			use_cache = 1;
		}
		else
		{
// Set picon thread to draw in background
			if(mode != IGNORE_THREAD)
			{
// printf("ResourcePixmap::draw_video_resource %d %d %lld\n", 
// __LINE__, 
// mwindow->frame_cache->total(),
// source_frame);
				gui->resource_thread->add_picon(this, 
					canvas->pane->number,
					x, 
					y, 
					picon_w,
					picon_h,
					mwindow->edl->session->frame_rate,
					source_frame,
					edit->channel,
					indexable);
			}
		}

		if(picon_frame)
        {
			draw_vframe(picon_frame, 
				x, 
				y, 
				picon_w, 
				picon_h, 
				0, 
				0);
        }


// Unlock the get_frame_ptr command
		if(use_cache)
        {
			mwindow->frame_cache->unlock();
		}
        
        
		if(frames_per_picon > 1)
		{
			x += Units::round(picon_w);
			project_frame = Units::to_int64(frames_per_picon * (int64_t)((double)(x + pixmap_x - edit_x) / picon_w));
		}
		else
		{
			x += Units::round(frame_w);
			project_frame = (int64_t)((double)(x + pixmap_x - edit_x) / frame_w);
		}


		canvas->test_timer();
	}
}


void ResourcePixmap::dump()
{
	printf("ResourcePixmap %p\n", this);
	printf(" edit 0x%llx edit_x %d pixmap_x %d pixmap_w %d visible %d\n", 
		(long long)edit_id, 
		(int)edit_x, 
		(int)pixmap_x, 
		(int)pixmap_w, 
		(int)visible);
}



