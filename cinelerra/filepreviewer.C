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
#include "bcsignals.h"
#include "canvas.h"
#include "condition.h"
#include "edl.h"
#include "edlfactory.h"
#include "edlsession.h"
#include "file.h"
#include "filepreviewer.h"
#include "indexfile.h"
#include "localsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "tracks.h"
#include "transportque.h"



#define SCROLL_MAX 100
FilePreviewerScroll::FilePreviewerScroll(FilePreviewer *previewer, 
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

int FilePreviewerScroll::handle_event()
{
    previewer->reset_play_button();
    previewer->interrupt_playback();
    previewer->seek_playback();
    return 1;
}


FilePreviewerPlay::FilePreviewerPlay(FilePreviewer *previewer, 
    int x,
    int y)
 : BC_Button(x, y, previewer->play_images)
{
    this->previewer = previewer;
    set_tooltip(_("Play"));
    is_play = 1;
}

int FilePreviewerPlay::handle_event()
{
//printf("FilePreviewerPlay::handle_event %d\n", __LINE__);
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


FilePreviewerRewind::FilePreviewerRewind(FilePreviewer *previewer, 
    int x,
    int y)
 : BC_Button(x, y, previewer->rewind_images)
{
    this->previewer = previewer;
    set_tooltip(_("Rewind"));
}


int FilePreviewerRewind::handle_event()
{
    previewer->reset_play_button();
    previewer->interrupt_playback();
    previewer->rewind_playback();
    return 1;
}


PreviewCanvas::PreviewCanvas(FilePreviewer *previewer, 
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


PreviewPlayback::PreviewPlayback(FilePreviewer *previewer)
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





FilePreviewerThread::FilePreviewerThread()
 : Thread(1, 0, 0)
{
}

void FilePreviewerThread::run()
{
    FilePreviewer *previewer = &FilePreviewer::instance;
    while(1)
    {
//printf("FilePreviewerThread::run %d waiting\n", __LINE__);
        int got_it = 0;
        previewer->file_waiter->lock("FilePreviewerThread::run");


//printf("FilePreviewerThread::run %d got it\n", __LINE__);
        previewer->analyzer_lock->lock("FilePreviewerThread::run");
        if(previewer->have_next)
        {
            previewer->have_next = 0;
            current_path.assign(previewer->next_path);
            got_it = 1;
//printf("FilePreviewerThread::run %d got %s\n", __LINE__, current_path.c_str());
        }
        previewer->analyzer_lock->unlock();

        if(got_it)
        {
// decide if the file can be previewed
            int preview_it = 0;
            File *new_file = new File;
// don't make TOCs for previews
            new_file->set_disable_toc_creation(1);

            Asset *new_asset = new Asset(current_path.c_str());
            int result = new_file->open_file(MWindow::preferences, 
                new_asset, 
                1, 
                0);
            if(result == FILE_OK)
            {
                if(new_asset->video_data || new_asset->audio_data)
                    preview_it = 1;
            }
            else
            if(result == FILE_UNRECOGNIZED_CODEC)
            {
// Test index file for PCM audio
			    IndexFile indexfile(0, new_asset);
			    result = indexfile.open_index();
			    if(!result)
			    {
				    indexfile.close_index();
                    preview_it = 1;
			    }
            }
            else
            {
    // preview not supported for EDLs

            }

            if(!preview_it)
            {
                previewer->preview_unavailable();
            }
            else
            {
// create the EDL to play back
                previewer->previewer_lock->lock("FilePreviewerThread::run");
// delete the objects for the previous file
                if(previewer->edl) 
                {
                    previewer->edl->remove_user();
                    previewer->edl = 0;
                }
                previewer->edl = new EDL;
                previewer->edl->create_objects();
// copy session breaks previews, but we want certain bits 
// from the mane project
                previewer->edl->session->audio_channels = MWindow::instance->edl->session->audio_channels;
                EDLFactory::asset_to_edl(previewer->edl, 
                    new_asset, 
                    0, 
                    1,  // conform
                    1); // auto aspect
                previewer->previewer_lock->unlock();



// continue in the filebox thread
                previewer->create_preview();
            }

// all the bits are copied into the EDL
            delete new_file;
            new_asset->remove_user();
        }
    }
}

FilePreviewer FilePreviewer::instance;

FilePreviewer::FilePreviewer()
 : BC_FileBoxPreviewer()
{
//printf("FilePreviewer::FilePreviewer %d\n", __LINE__);
    canvas = 0;
    play = 0;
    rewind = 0;
    scroll = 0;
    edl = 0;
    is_playing = 0;
}

void FilePreviewer::initialize()
{
    playback_engine = new PreviewPlayback(this);
    playback_engine->set_is_previewer(1);
    playback_engine->create_objects();
//    playback_engine->preferences->dump();

    file_waiter = new Condition(0, "FilePreviewer::file_waiter");
    analyzer_lock = new Mutex("FilePreviewer::analyzer_lock");
    play_images = MWindow::theme->get_image_set("play");
    rewind_images = MWindow::theme->get_image_set("rewind");
    stop_images = MWindow::theme->get_image_set("stop");
    speaker_image = MWindow::theme->get_image("speaker");
    thread = new FilePreviewerThread;
    thread->start();
}

void FilePreviewer::handle_resize(int w, int h)
{
    previewer_lock->lock("FilePreviewer::handle_resize");
    if(edl)
    {
        int margin = BC_Resources::theme->widget_border;
        Asset *asset = edl->assets->first;
        int canvas_w = filebox->preview_w /* - margin */;
        int canvas_h = canvas_w;
        if(asset->video_data)
        {
            canvas_h = canvas_w * asset->height / asset->width;
        }
        else
        if(asset->audio_data)
        {
            canvas_h = canvas_w * 
                speaker_image->get_h() / 
                speaker_image->get_w();
        }
        int x = filebox->preview_x /* + margin */;
        int y = filebox->preview_center_y - canvas_h / 2;
        if(canvas)
        {
            canvas->reposition_window(x,
                y,
                canvas_w,
                canvas_h);
            if(!asset->video_data)
            {
                canvas->draw_vframe(speaker_image,
                    0,
                    0,
                    canvas->get_w(),
                    canvas->get_h());
                canvas->flash(0);
            }
            else
            if(output_frame)
            {
                canvas->draw_vframe(output_frame,
                    0,
                    0,
                    canvas->get_w(),
                    canvas->get_h());
                canvas->flash(0);
            }
            y += canvas_h;
        }
        
        if(scroll)
        {
            double total = edl->tracks->total_playable_length();
            scroll->reposition_window(x,
                y,
                canvas_w);
            scroll->update(canvas_w,
                play_position * SCROLL_MAX / total,
                0,
                SCROLL_MAX);
            y += scroll->get_h();
        }

        if(rewind)
        {
            rewind->reposition_window(x,
                y);
            x += rewind->get_w();
        }
        
        if(play)
        {
            play->reposition_window(x,
                y);
        }
    }
    previewer_lock->unlock();
}

void FilePreviewer::submit_file(const char *path)
{
    clear_preview();

    analyzer_lock->lock("FilePreviewer::submit_file");
//printf("FilePreviewer::submit_file %d got %s\n", __LINE__, path);
    this->next_path.assign(path);
    this->have_next = 1;
    analyzer_lock->unlock();
    
    file_waiter->unlock();
}


// clear_preview is always getting called before this but sometimes
// multiple create_preview are called in a row
void FilePreviewer::create_preview()
{
    previewer_lock->lock("FilePreviewer::create_preview");
    if(filebox)
    {
//printf("FilePreviewer::create_preview %d\n", __LINE__);
        filebox->put_event([](void *ptr)
            {
                BC_FileBox *filebox = (BC_FileBox*)ptr;
                FilePreviewer *previewer = &FilePreviewer::instance;
                previewer->previewer_lock->lock("FilePreviewer::create_preview");

//printf("FilePreviewer::create_preview %d edl=%p\n", __LINE__, previewer->edl);
                if(previewer->edl)
                {
                    int margin = BC_Resources::theme->widget_border;
                    Asset *asset = previewer->edl->assets->first;
//printf("FilePreviewer::create_preview %d asset=%p\n", __LINE__, asset);
//asset->dump();
//previewer->edl->dump();
// create the widgets based on the EDL contents
                    int canvas_w = filebox->preview_w /* - margin */;
                    int canvas_h = canvas_w;
                    previewer->play_position = 0;
                    filebox->preview_status->hide_window();
                    if(asset->video_data)
                    {
                        canvas_h = canvas_w * asset->height / asset->width;
                    }
                    else
                    if(asset->audio_data)
                    {
                        canvas_h = canvas_w * 
                            previewer->speaker_image->get_h() / 
                            previewer->speaker_image->get_w();
                    }

                    int x = filebox->preview_x /* + margin */;
                    int y = filebox->preview_center_y - canvas_h / 2;
                    if(previewer->canvas)
                        previewer->canvas->reposition_window(x,
                            y,
                            canvas_w,
                            canvas_h);
                    else
                        filebox->add_subwindow(previewer->canvas = new PreviewCanvas(previewer, 
                            x,
                            y,
                            canvas_w,
                            canvas_h));
                    previewer->canvas->show_window(0);
                    y += canvas_h;

// something seekable & not a still photo
                    if(asset->audio_length >= 0 && asset->video_length >= 0)
                    {
                        if(previewer->scroll)
                        {
                            double total = previewer->edl->tracks->total_playable_length();
                            previewer->scroll->reposition_window(x,
                                y,
                                canvas_w);
                            previewer->scroll->update(canvas_w,
                                previewer->play_position * SCROLL_MAX / total,
                                0,
                                SCROLL_MAX);
                        }
                        else
                            filebox->add_subwindow(previewer->scroll = new FilePreviewerScroll(previewer, 
                                x,
                                y,
                                canvas_w));
                        y += previewer->scroll->get_h();
                    }

// can be played back
                    if(asset->video_length != STILL_PHOTO_LENGTH)
                    {
                        if(previewer->rewind)
                            previewer->rewind->reposition_window(x,
                                y);
                        else
                            filebox->add_subwindow(previewer->rewind = new FilePreviewerRewind(previewer, 
                                x,
                                y));
                        x += previewer->rewind->get_w();
                        
                        if(previewer->play)
                            previewer->play->reposition_window(x,
                                y);
                        else
                            filebox->add_subwindow(previewer->play = new FilePreviewerPlay(previewer, 
                                x,
                                y));
                    }

                    if(!asset->video_data)
                    {
                        previewer->canvas->draw_vframe(previewer->speaker_image,
                            0,
                            0,
                            previewer->canvas->get_w(),
                            previewer->canvas->get_h());
                        previewer->canvas->flash(0);
                    }

                    filebox->flush();
// set up the playback engine
                    previewer->playback_engine->que->send_command(CURRENT_FRAME, 
			            CHANGE_ALL,
			            previewer->edl,
			            1);
                }
                previewer->previewer_lock->unlock();
            },
            filebox);
    }
    previewer_lock->unlock();
}

void FilePreviewer::clear_preview()
{
    interrupt_playback();
    if(canvas) delete canvas;
    if(play) delete play;
    if(rewind) delete rewind;
    if(scroll) delete scroll;
    canvas = 0;
    play = 0;
    rewind = 0;
    scroll = 0;
}




void FilePreviewer::start_playback()
{
    previewer_lock->lock("FilePreviewer::start_playback");
    edl->local_session->set_selectionstart(play_position);
    edl->local_session->set_selectionend(play_position);
    playback_engine->que->send_command(NORMAL_FWD,
		CHANGE_NONE, 
		edl,
		1,
		1,
		0);
    is_playing = 1;
    previewer_lock->unlock();
}

void FilePreviewer::stop_playback()
{
    previewer_lock->lock("FilePreviewer::stop_playback");
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

void FilePreviewer::interrupt_playback()
{
// deadlocks with update_tracker if we have previewer_lock
//printf("FilePreviewer::interrupt_playback %d\n", __LINE__);
//    previewer_lock->lock("FilePreviewer::stop_playback");
//printf("FilePreviewer::interrupt_playback %d\n", __LINE__);

    is_playing = 0;


	playback_engine->interrupt_playback(0);
//printf("FilePreviewer::interrupt_playback %d\n", __LINE__);

//    previewer_lock->unlock();
}

void FilePreviewer::rewind_playback()
{
    previewer_lock->lock("FilePreviewer::rewind_playback");
    play_position = 0;

// rewind the playback engine
    if(edl)
    {
// set up the playback engine
        edl->local_session->set_selectionstart(0);
        edl->local_session->set_selectionend(0);
        play_position = 0;
        playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_ALL,
			edl,
			1);
    }

// rewind the slider
    if(filebox)
    {
        filebox->put_event([](void *ptr)
            {
                BC_FileBox *filebox = (BC_FileBox*)ptr;
                FilePreviewer *previewer = &FilePreviewer::instance;
                previewer->previewer_lock->lock("FilePreviewer::update_scrollbar 2");
                if(previewer->scroll)
                {
                    previewer->scroll->update(0);
                }
                previewer->previewer_lock->unlock();
            },
            filebox);
    }

    previewer_lock->unlock();
}

void FilePreviewer::seek_playback()
{
    previewer_lock->lock("FilePreviewer::rewind_playback");
    if(edl)
    {
        double total = edl->tracks->total_playable_length();
        play_position = (double)scroll->get_value() *
            total /
            scroll->get_length();
        edl->local_session->set_selectionstart(play_position);
        edl->local_session->set_selectionend(play_position);
        playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_ALL,
			edl,
			1);
    }
    previewer_lock->unlock();
}

void FilePreviewer::write_frame(VFrame *frame)
{
    previewer_lock->lock("FilePreviewer::write_frame 1");


    if(filebox)
    {
        filebox->put_event([](void *ptr)
            {
                BC_FileBox *filebox = (BC_FileBox*)ptr;
                FilePreviewer *previewer = &FilePreviewer::instance;
                previewer->previewer_lock->lock("FilePreviewer::write_frame 2");
                if(previewer->output_frame &&
                    previewer->canvas)
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
            filebox);
    }

    previewer_lock->unlock();
}


void FilePreviewer::update_scrollbar(int lock_it)
{
    if(lock_it) previewer_lock->lock("FilePreviewer::update_scrollbar 1");


    if(filebox)
    {
        filebox->put_event([](void *ptr)
            {
                BC_FileBox *filebox = (BC_FileBox*)ptr;
                FilePreviewer *previewer = &FilePreviewer::instance;
                previewer->previewer_lock->lock("FilePreviewer::update_scrollbar 2");
                if(previewer->edl)
                {
                    Asset *asset = previewer->edl->assets->first;
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
            filebox);
    }

    if(lock_it) previewer_lock->unlock();
}

void FilePreviewer::reset_play_button()
{
    play->set_images(play_images);
    play->draw_face();
    play->is_play = 1;
}

void FilePreviewer::reset_play_async()
{
    previewer_lock->lock("FilePreviewer::reset_play_async 1");


    if(filebox)
    {
        filebox->put_event([](void *ptr)
            {
                FilePreviewer *previewer = &FilePreviewer::instance;
                if(previewer->play) previewer->reset_play_button();
            },
            filebox);
    }

    previewer_lock->unlock();
}



