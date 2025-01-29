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
#include "edits.h"
#include "edl.h"
#include "edlfactory.h"
#include "edlsession.h"
#include "file.h"
#include "filepreviewer.h"
#include "filesystem.h"
#include "filexml.h"
#include "indexfile.h"
#include "localsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginset.h"
#include "preferences.h"
#include "theme.h"
#include "tracks.h"
#include "transportque.h"

#include <sys/stat.h>

#define MAX_WIDTH 10000
#define MAX_HEIGHT 10000



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
            int is_edl = 0;
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
                if(new_asset->audio_length >= 0 && 
                    new_asset->video_length >= 0) 
                    previewer->seekable = 1;
                if(new_asset->video_data && 
                    (new_asset->width > MAX_WIDTH ||
                    new_asset->height > MAX_HEIGHT))
                    preview_it = 0;
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
                    previewer->seekable = 1;
			    }
            }
            else
            if(result == FILE_IS_XML)
            {
// load the EDL & test conditions for previewing
                preview_it = 1;
                is_edl = 1;

                int debug = 1;
                FileXML xml_file;
                result = xml_file.read_from_file(current_path.c_str());
// load failed but fall through to keep indentation reasonable
                if(result) preview_it = 0;


                previewer->previewer_lock->lock("FilePreviewerThread::run");
// delete the previous EDL
                if(previewer->edl) 
                {
                    previewer->edl->remove_user();
                    previewer->edl = 0;
                }

                previewer->edl = new EDL;
                previewer->edl->create_objects();
                result = previewer->edl->load_xml(&xml_file, LOAD_ALL);
                previewer->edl->set_path(current_path.c_str());
// match the selection to the preview scroll bar
                previewer->edl->local_session->set_selectionstart(0);
                previewer->edl->local_session->set_selectionend(0);

// load failed
                if(result) preview_it = 0;


// output must be within frame size limit
                if(preview_it && 
                    (previewer->edl->session->output_w > MAX_WIDTH ||
                    previewer->edl->session->output_h > MAX_HEIGHT)) 
                {
                    if(debug) 
                    {
                        printf("FilePreviewerThread::run %d: output too big\n", __LINE__);
                        debug = 0;
                    }
                    preview_it = 0;
                }

// no more than 1 playable video track
                if(preview_it && 
                    previewer->edl->tracks->total_playable_tracks(TRACK_VIDEO) > 1)
                {
                    if(debug) 
                    {
                        printf("FilePreviewerThread::run %d: multiple video tracks not supported\n", __LINE__);
                        debug = 0;
                    }
                    preview_it = 0;
                }

                Track *track = previewer->edl->tracks->first;

// get the video track
                while(track && 
                    (track->data_type != TRACK_VIDEO || 
                    !track->play)) track = track->next;

                if(track)
                {
// track must be within frame size limit
                    if(track->track_w > MAX_WIDTH ||
                        track->track_h > MAX_HEIGHT)
                    {
                        if(debug) 
                        {
                            printf("FilePreviewerThread::run %d: track too big\n", __LINE__);
                            debug = 0;
                        }
                        preview_it = 0;
                    }

// no more than 1 asset in the track
                    Edit *edit = track->edits->first;
                    Asset *got_asset = 0;
                    while(edit)
                    {
                        if(edit->asset)
                        {
                            if(got_asset && 
                                edit->asset && 
                                edit->asset != got_asset)
                            {
                                if(debug) 
                                {
                                    printf("FilePreviewerThread::run %d: multiple assets not supported\n", __LINE__);
                                    debug = 0;
                                }
                                preview_it = 0;
                            }
                            got_asset = edit->asset;
                        }

// no nested EDLs
                        if(edit->nested_edl) preview_it = 0;

// fall through
                        edit = edit->next;
                    }

// no plugins in the track, in case of a memory hog
                    if(track->plugin_set.size() > 0)
                    {
                        for(int i = 0; i < track->plugin_set.size(); i++)
                        {
                            PluginSet *plugin_set = track->plugin_set.get(i);
                            Plugin *plugin = plugin_set->get_first_plugin();
                            while(plugin)
                            {
                                if(plugin->plugin_type != PLUGIN_NONE && 
                                    plugin->on) 
                                {
                                    if(debug) 
                                    {
                                        printf("FilePreviewerThread::run %d: plugins not supported\n", __LINE__);
                                        debug = 0;
                                    }
                                    preview_it = 0;
                                }
                                plugin = (Plugin*)plugin->next;
                            }
                        }
                    }

// asset dimensions outside size limit
                    if(got_asset &&
                        (got_asset->width > MAX_WIDTH ||
                        got_asset->height > MAX_HEIGHT))
                        preview_it = 0;

// test if asset exists & is seekable by opening it
                    if(preview_it)
                    {
                        File *test_file = new File;
                        Asset *test_asset = new Asset(got_asset->path);
                        test_file->set_disable_toc_creation(1);
                        result = test_file->open_file(MWindow::preferences, 
                            test_asset, 
                            1, 
                            0);
                        if(result == FILE_OK)
                        {
                            if(test_asset->audio_length >= 0 && 
                                test_asset->video_length >= 0) 
                                previewer->seekable = 1;
                        }
                        else
                        {
                            if(debug) 
                            {
                                printf("FilePreviewerThread::run %d: asset not found\n", __LINE__);
                                debug = 0;
                            }
                            preview_it = 0;
                        }
                        delete test_file;
                        test_asset->remove_user();
                    }
                }

// failed
                if(!preview_it)
                {
                    previewer->edl->remove_user();
                    previewer->edl = 0;
                }

                previewer->previewer_lock->unlock();
            }

            if(!preview_it)
            {
                previewer->preview_unavailable();
            }
            else
            {
                if(!is_edl)
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
                    previewer->edl->set_path(current_path.c_str());
// copy session breaks previews, but we want certain bits 
// from the mane project
                    previewer->edl->session->audio_channels = MWindow::instance->edl->session->audio_channels;
                    EDLFactory::asset_to_edl(previewer->edl, 
                        new_asset, 
                        0, 
                        1,  // conform
                        1); // auto aspect
                    previewer->previewer_lock->unlock();
                }


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
//printf("FilePreviewer::submit_file %d\n", __LINE__);
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
                    int margin = BC_Resources::theme->widget_border;
                    Asset *asset = previewer->edl->assets->first;
// length in seconds
                    double total = -1;
                    
//printf("FilePreviewer::create_preview %d asset=%p\n", __LINE__, asset);
//asset->dump();
//previewer->edl->dump();
// create the widgets based on the EDL contents
                    int canvas_w = filebox->preview_w /* - margin */;
                    int canvas_h = canvas_w;
                    previewer->play_position = 0;
                    filebox->preview_status->hide_window();
                    if(previewer->edl->tracks->total_playable_tracks(TRACK_VIDEO))
                    {
                        canvas_h = canvas_w * 
                            previewer->edl->session->output_h / 
                            previewer->edl->session->output_w;
                    }
                    else
                    {
                        canvas_h = canvas_w * 
                            previewer->speaker_image->get_h() / 
                            previewer->speaker_image->get_w();
                    }

                    int x = filebox->preview_x /* + margin */;
                    int y = filebox->preview_center_y - 
                        canvas_h / 2 - 
                        BC_Title::calculate_h(filebox, 
                            "X", 
                            SMALLFONT) * 3 -
                        previewer->rewind_images[0]->get_h();
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
//                     if(asset && 
//                         asset->audio_length >= 0 && 
//                         asset->video_length >= 0)
                    if(previewer->seekable)
                    {
                        total = previewer->edl->tracks->total_playable_length();
                        if(previewer->scroll)
                        {
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
                    if(asset && asset->video_length != STILL_PHOTO_LENGTH)
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
                        y += previewer->play->get_h();
                    }

                    if(!previewer->edl->tracks->total_playable_tracks(TRACK_VIDEO))
                    {
                        previewer->canvas->draw_vframe(previewer->speaker_image,
                            0,
                            0,
                            previewer->canvas->get_w(),
                            previewer->canvas->get_h());
                        previewer->canvas->flash(0);
                    }

                    y += margin;
                    previewer->create_info(filebox, 
                        filebox->preview_x, 
                        y,
                        total);

                    filebox->flush();
// set up the playback engine
                    previewer->playback_engine->que->send_command(CURRENT_FRAME, 
                        1.0, // speed
			            CHANGE_ALL,
			            previewer->edl,
			            1, // realtime
                        0, // resume,
                        0); // use_inout
                }
                previewer->previewer_lock->unlock();
            },
            gui);
    }
    previewer_lock->unlock();
}

void FilePreviewer::create_info(BC_FileBox *filebox, 
    int x, 
    int y, 
    double length)
{

// always show info about the file
// must handle the case of clear_preview not being called before this
    struct stat ostat;
    if(!stat(edl->path, &ostat))
    {
        char string[BCTEXTLEN];
        char string2[BCTEXTLEN];
        char string3[BCTEXTLEN];
        FileSystem fs;
        fs.extract_name(string, edl->path);
        strcat(string, "\n");

        sprintf(string2, "%ld", (long)ostat.st_size);
        Units::punctuate(string2);
        strcat(string2, " bytes\n");
        strcat(string, string2);


        struct tm *mod_time;
        mod_time = localtime(&(ostat.st_mtime));
        int month = mod_time->tm_mon + 1;
        int day = mod_time->tm_mday;
        int year = mod_time->tm_year + 1900;
        int hour = mod_time->tm_hour;
        int minute = mod_time->tm_min;
        int second = mod_time->tm_sec;
		static const char *month_text[13] = 
		{
			"Null",
			"Jan",
			"Feb",
			"Mar",
			"Apr",
			"May",
			"Jun",
			"Jul",
			"Aug",
			"Sep",
			"Oct",
			"Nov",
			"Dec"
		};
        sprintf(string2, 
			"Date: %s %d, %04d\nTime: %d:%02d:%02d\n", 
			month_text[month],
			day,
			year,
            hour,
            minute,
            second);
        strcat(string, string2);


        if(length > 0)
            Units::totext(string2, length, TIME_HMS);
        else
            sprintf(string2, _("Unknown"));
        sprintf(string3, "Length: %s\n", string2);
        strcat(string, string3);

        string2[0] = 0;
        if(edl->tracks->total_playable_tracks(TRACK_VIDEO))
        {
            sprintf(string3, 
                "%dx%d", 
                edl->session->output_w,
                edl->session->output_h);
            strcat(string2, string3);
        }
        
        if(edl->tracks->playable_audio_tracks())
        {
            if(strlen(string2) > 0) strcat(string2, " ");
            if((edl->session->sample_rate % 1000) == 0)
                sprintf(string3, 
                    "%dkhz", 
                    (int)edl->session->sample_rate / 1000);
            else
                sprintf(string3, 
                    "%dHz", 
                    (int)edl->session->sample_rate);
            strcat(string2, string3);
        }
        
        if(strlen(string3) > 0)
        {
            strcat(string3, "\n");
            strcat(string, string2);
        }


        delete info_text;
        filebox->add_subwindow(info_text = new BC_Title(x, 
            y, 
            string,
            SMALLFONT,
            -1,
            0,
            filebox->preview_w));
        y += info_text->get_h();
    }
    else
    {
        printf("FilePreviewer::create_info %d: edl->path=%s stat failed '%s'\n", 
            __LINE__, edl->path, strerror(errno));
    }
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
        int y = filebox->preview_center_y - 
            canvas_h / 2 -
            BC_Title::calculate_h(filebox, 
                "X", 
                SMALLFONT) * 3 -
            rewind_images[0]->get_h();
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
            y += play->get_h();
        }

        x = filebox->preview_x;
        y += margin;
        if(info_text) 
        {
            info_text->reposition(x, y, filebox->preview_w);
            y += info_text->get_h();
        }
    }
    
    previewer_lock->unlock();
}


