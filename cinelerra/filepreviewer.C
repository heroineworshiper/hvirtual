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
#include "mutex.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "tracks.h"
#include "transportque.h"






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

FilePreviewer::FilePreviewer() : Previewer()
{
}

void FilePreviewer::initialize()
{
    Previewer::initialize();

    file_waiter = new Condition(0, "FilePreviewer::file_waiter");
    analyzer_lock = new Mutex("FilePreviewer::analyzer_lock");
    thread = new FilePreviewerThread;
    thread->start();
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
    if(gui)
    {
//printf("FilePreviewer::create_preview %d\n", __LINE__);
        gui->put_event([](void *ptr)
            {
                BC_FileBox *filebox = (BC_FileBox*)ptr;
                FilePreviewer *previewer = &FilePreviewer::instance;
                previewer->previewer_lock->lock("FilePreviewer::create_preview");

//printf("FilePreviewer::create_preview %d edl=%p\n", __LINE__, previewer->edl);
                if(previewer->edl)
                {
//                    int margin = BC_Resources::theme->widget_border;
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
                                previewer->play_position * 
                                    previewer->scroll->get_length() / 
                                    total,
                                0,
                                previewer->scroll->get_length());
                        }
                        else
                            filebox->add_subwindow(previewer->scroll = new PreviewerScroll(previewer, 
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
                            filebox->add_subwindow(previewer->rewind = new PreviewerRewind(previewer, 
                                x,
                                y));
                        x += previewer->rewind->get_w();
                        
                        if(previewer->play)
                            previewer->play->reposition_window(x,
                                y);
                        else
                            filebox->add_subwindow(previewer->play = new PreviewerPlay(previewer, 
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
            gui);
    }
    previewer_lock->unlock();
}



void FilePreviewer::handle_resize(int w, int h)
{
    previewer_lock->lock("FilePreviewer::handle_resize");
    if(edl)
    {
        BC_FileBox *filebox = (BC_FileBox*)gui;
        int margin = BC_Resources::theme->widget_border;
        int canvas_w = filebox->preview_w /* - margin */;
        int canvas_h = canvas_w;
        if(edl->tracks->playable_video_tracks())
        {
            canvas_h = canvas_w * edl->session->output_h / edl->session->output_w;
        }
        else
        if(edl->tracks->playable_audio_tracks())
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
            if(!edl->tracks->playable_video_tracks())
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
                play_position * scroll->get_length() / total,
                0,
                scroll->get_length());
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


