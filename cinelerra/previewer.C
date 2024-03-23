/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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
#include "assets.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "previewer.h"
#include "theme.h"
#include "tracks.h"
#include "transportque.h"



#define SCROLL_MAX 100
PreviewerScroll::PreviewerScroll(Previewer *previewer, 
    int x,
    int y,
    int w)
 : BC_ISlider(x, 
    y, 
    0,  // vertical
    w,  // pixels
    w,  // pointer_motion_range
    0,  // minvalue
    SCROLL_MAX,  // maxvalue
    0)  // value
{
    this->previewer = previewer;
}

int PreviewerScroll::handle_event()
{
    previewer->reset_play_button();
    previewer->interrupt_playback();
    previewer->seek_playback();
    return 1;
}


PreviewerPlay::PreviewerPlay(Previewer *previewer, 
    int x,
    int y)
 : BC_Button(x, y, previewer->play_images)
{
    this->previewer = previewer;
    set_tooltip(_("Play"));
    is_play = 1;
}

int PreviewerPlay::handle_event()
{
//printf("PreviewerPlay::handle_event %d\n", __LINE__);
    if(is_play)
    {
        previewer->start_playback();
        set_images(previewer->stop_images);
        draw_face();
        is_play = 0;
    }
    else
    {
        previewer->stop_playback();
        previewer->reset_play_button();
    }
    return 1;
}


PreviewerRewind::PreviewerRewind(Previewer *previewer, 
    int x,
    int y)
 : BC_Button(x, y, previewer->rewind_images)
{
    this->previewer = previewer;
    set_tooltip(_("Rewind"));
}


int PreviewerRewind::handle_event()
{
    previewer->reset_play_button();
    previewer->interrupt_playback();
    previewer->rewind_playback();
    return 1;
}


PreviewCanvas::PreviewCanvas(Previewer *previewer, 
    int x,
    int y,
    int w,
    int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
    this->previewer = previewer;
}

int PreviewCanvas::button_release_event()
{
    if(is_event_win())
    {
        return 1;
    }
    return 0;
}


PreviewPlayback::PreviewPlayback(Previewer *previewer)
 : PlaybackEngine()
{
    this->previewer = previewer;
}

void PreviewPlayback::init_cursor()
{
}

void PreviewPlayback::stop_cursor()
{
// can't draw anything in here, since it's called during interrupt_playback
    previewer->reset_play_async();
}

void PreviewPlayback::update_tracker(double position)
{
//printf("PreviewPlayback::update_tracker %d\n", __LINE__);
    previewer->previewer_lock->lock("PreviewPlayback::update_tracker");
//printf("PreviewPlayback::update_tracker %d\n", __LINE__);
    if(previewer->is_playing)
    {
        previewer->play_position = position;
        previewer->update_scrollbar(0);
    }
    previewer->previewer_lock->unlock();
}



Previewer::Previewer() : BC_FileBoxPreviewer()
{
    canvas = 0;
    play = 0;
    rewind = 0;
    scroll = 0;
    edl = 0;
    is_playing = 0;
    output_frame = 0;
    name_text = 0;
    date_text = 0;
    size_text = 0;
}


void Previewer::initialize()
{
    playback_engine = new PreviewPlayback(this);
    playback_engine->set_previewer(this);
    playback_engine->create_objects();
//    playback_engine->preferences->dump();
    play_images = MWindow::theme->get_image_set("play");
    rewind_images = MWindow::theme->get_image_set("rewind");
    stop_images = MWindow::theme->get_image_set("stop");
    speaker_image = MWindow::theme->get_image("speaker");
}

void Previewer::clear_preview()
{
    interrupt_playback();
    
    previewer_lock->lock("Previewer::clear_preview");
    delete canvas;
    delete play;
    delete rewind;
    delete scroll;
    delete output_frame;
    delete name_text;
    delete date_text;
    delete size_text;
    output_frame = 0;
    canvas = 0;
    play = 0;
    rewind = 0;
    scroll = 0;
    name_text = 0;
    date_text = 0;
    size_text = 0;
    seekable = 0;
    previewer_lock->unlock();
}


void Previewer::start_playback()
{
    previewer_lock->lock("Previewer::start_playback");
    if(edl)
    {
        edl->local_session->set_selectionstart(play_position);
        edl->local_session->set_selectionend(play_position);
        playback_engine->que->send_command(NORMAL_FWD,
		    CHANGE_NONE, 
		    edl,
		    1,
		    1,
		    0);
        is_playing = 1;
    }
    previewer_lock->unlock();
}

void Previewer::stop_playback()
{
    previewer_lock->lock("Previewer::stop_playback");
	playback_engine->que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0,
		0,
		0);
    is_playing = 0;
// avoid deadlock with update_tracker
    previewer_lock->unlock();

	interrupt_playback();
}

void Previewer::interrupt_playback()
{
// deadlocks with update_tracker if we have previewer_lock
//printf("Previewer::interrupt_playback %d\n", __LINE__);
//    previewer_lock->lock("Previewer::stop_playback");
//printf("Previewer::interrupt_playback %d\n", __LINE__);

    is_playing = 0;


	playback_engine->interrupt_playback(0);
//printf("Previewer::interrupt_playback %d\n", __LINE__);

//    previewer_lock->unlock();
}

void Previewer::rewind_playback()
{
    previewer_lock->lock("Previewer::rewind_playback");
    play_position = 0;

// rewind the playback engine
    if(edl)
    {
// set up the playback engine
        edl->local_session->set_selectionstart(0);
        edl->local_session->set_selectionend(0);
        play_position = 0;
// don't reload if seekable or not a file
        Asset *asset = edl->assets->first;

// can't know if it's seekable if it's in an EDL
// printf("Previewer::rewind_playback %d %d %d\n", 
// __LINE__,
// (int)asset->audio_length,
// (int)asset->video_length);
        playback_engine->que->send_command(CURRENT_FRAME, 
//			(!asset || (asset->audio_length >= 0 && asset->video_length >= 0)) ?
			seekable ? CHANGE_NONE : CHANGE_ALL,
			edl,
			1);
    }

// rewind the slider
    if(gui)
    {
        gui->put_event([](void *ptr)
            {
                Previewer *previewer = (Previewer*)ptr;
                previewer->previewer_lock->lock("Previewer::update_scrollbar 2");
                if(previewer->scroll)
                {
                    previewer->scroll->update(0);
                }
                previewer->previewer_lock->unlock();
            },
            this);
    }

    previewer_lock->unlock();
}

void Previewer::seek_playback()
{
    previewer_lock->lock("Previewer::seek_playback");
    if(edl)
    {
        double total = edl->tracks->total_playable_length();
// don't seek past last frame.  Important for transition previews.
        int64_t total_frames = total * edl->session->frame_rate;
        total_frames--;
        total = (double)total_frames / edl->session->frame_rate;

        play_position = (double)scroll->get_value() *
            total /
            scroll->get_length();
        edl->local_session->set_selectionstart(play_position);
        edl->local_session->set_selectionend(play_position);
        playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			edl,
			1);
    }
    previewer_lock->unlock();
}

void Previewer::write_frame(VFrame *frame)
{
    previewer_lock->lock("Previewer::write_frame 1");

    int output_cmodel = BC_RGB888;
// alpha checkers would be supported here but require heroic programming
// alpha checkers can only be drawn on the BGR destinations
//    if(cmodel_has_alpha(frame->get_color_model())) output_cmodel = BC_BGR8888;

    if(output_frame && 
        (output_cmodel != output_frame->get_color_model() ||
        output_frame->get_w() != frame->get_w() ||
        output_frame->get_h() != frame->get_h()))
    {
        delete output_frame;
        output_frame = 0;
    }
    if(!output_frame)
    {
        output_frame = new VFrame;
        output_frame->set_use_shm(0);
        output_frame->reallocate(
			0, 
			-1,
            0,
            0,
            0,
			frame->get_w(),
			frame->get_h(),
			output_cmodel,
			-1);
    }

// must copy it to avoid flickering, as it's being drawn in the GUI thread
//printf("Previewer::write_frame %d frame=%p output_frame=%p\n", 
//__LINE__, frame, output_frame);

    if(output_frame->equivalent(frame) /* && 
        !cmodel_has_alpha(frame->get_color_model()) */ )
    {
        output_frame->copy_from(frame);
    }
    else
//    if(!cmodel_has_alpha(frame->get_color_model()))
    {
        cmodel_transfer(output_frame->get_rows(), 
	        frame->get_rows(),
	        output_frame->get_y(),
	        output_frame->get_u(),
	        output_frame->get_v(),
	        frame->get_y(),
	        frame->get_u(),
	        frame->get_v(),
            0,        /* Dimensions to capture from input frame */
	        0, 
	        frame->get_w(), 
	        frame->get_h(),
	        0,       /* Dimensions to project on output frame */
	        0, 
	        output_frame->get_w(), 
	        output_frame->get_h(),
            frame->get_color_model(), 
	        output_frame->get_color_model(),
	        0,
            frame->get_bytes_per_line(),    // bytes per line
	        output_frame->get_bytes_per_line());
    }
//     else
//     {
//         cmodel_transfer_alpha(output_frame->get_rows(), /* Leave NULL if non existent */
// 	        frame->get_rows(),
//             0,        /* Dimensions to capture from input frame */
// 	        0, 
// 	        frame->get_w(), 
// 	        frame->get_h(),
// 	        0,       /* Dimensions to project on output frame */
// 	        0, 
// 	        output_frame->get_w(), 
// 	        output_frame->get_h(),
//             frame->get_color_model(), 
// 	        output_frame->get_color_model(),
//             frame->get_bytes_per_line(),    // bytes per line
// 	        output_frame->get_bytes_per_line(),
//             CHECKER_W,
//             CHECKER_H);
//     }

//printf("Previewer::write_frame %d\n", 
//__LINE__);

    if(gui)
    {
        gui->put_event([](void *ptr)
            {
                Previewer *previewer = (Previewer*)ptr;
                previewer->previewer_lock->lock("Previewer::write_frame 2");
//printf("Previewer::write_frame %d: output_frame=%p\n", 
//__LINE__, previewer->output_frame);
                if(previewer->output_frame &&
                    previewer->canvas &&
                    previewer->edl &&
                    previewer->edl->tracks->playable_video_tracks())
                {
                    previewer->canvas->draw_vframe(previewer->output_frame,
                        0,
                        0,
                        previewer->canvas->get_w(),
                        previewer->canvas->get_h());
                    previewer->canvas->flash(1);
                }
                previewer->previewer_lock->unlock();
            },
            this);
    }

    previewer_lock->unlock();
}


void Previewer::update_scrollbar(int lock_it)
{
    if(lock_it) previewer_lock->lock("Previewer::update_scrollbar 1");


    if(gui)
    {
        gui->put_event([](void *ptr)
            {
                Previewer *previewer = (Previewer*)ptr;
                previewer->previewer_lock->lock("Previewer::update_scrollbar 2");
                if(previewer->edl)
                {
                    if(previewer->scroll)
                    {
                        double total = previewer->edl->tracks->total_playable_length();
                        previewer->scroll->update(
                            (int64_t)(previewer->play_position * 
                                previewer->scroll->get_length() / 
                                total));
                    }
                }
                previewer->previewer_lock->unlock();
            },
            this);
    }

    if(lock_it) previewer_lock->unlock();
}

void Previewer::reset_play_button()
{
    play->set_images(play_images);
    play->draw_face();
    play->is_play = 1;
}

void Previewer::reset_play_async()
{
    previewer_lock->lock("Previewer::reset_play_async 1");


    if(gui)
    {
        gui->put_event([](void *ptr)
            {
                Previewer *previewer = (Previewer*)ptr;
                if(previewer->play) previewer->reset_play_button();
            },
            this);
    }

    previewer_lock->unlock();
}













