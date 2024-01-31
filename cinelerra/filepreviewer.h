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



// generate previews for the file dialogs

#ifndef FILEPREVIEWER_H
#define FILEPREVIEWER_H

#include "canvas.inc"
#include "condition.inc"
#include "filepreviewer.inc"
#include "guicast.h"
#include "mutex.inc"
#include "playbackengine.h"
#include "thread.h"

#include <string>

using std::string;


class FilePreviewerScroll : public BC_ISlider
{
public:
    FilePreviewerScroll(FilePreviewer *previewer, 
        int x,
        int y,
        int w);
    int handle_event();
    FilePreviewer *previewer;
};

class FilePreviewerPlay : public BC_Button
{
public:
    FilePreviewerPlay(FilePreviewer *previewer, 
        int x,
        int y);
    int handle_event();
    FilePreviewer *previewer;
    int is_play;
};

// required by non seekable files
class FilePreviewerRewind : public BC_Button
{
public:
    FilePreviewerRewind(FilePreviewer *previewer, 
        int x,
        int y);
    int handle_event();
    FilePreviewer *previewer;
};

class PreviewCanvas : public BC_SubWindow
{
public:
    PreviewCanvas(FilePreviewer *previewer, 
        int x,
        int y,
        int w,
        int h);
    int button_release_event();
    FilePreviewer *previewer;
};

class PreviewPlayback : public PlaybackEngine
{
public:
    PreviewPlayback(FilePreviewer *previewer);
	void init_cursor();
	void stop_cursor();
    void update_tracker(double position);
    FilePreviewer *previewer;
};


// analyzes the file
class FilePreviewerThread : public Thread
{
public:
    FilePreviewerThread();
    void run();

// file currently being analyzed
    string current_path;
};

// BC_Filebox calls into this to get previews
class FilePreviewer : public BC_FileBoxPreviewer
{
public:
    FilePreviewer();


    static FilePreviewer instance;

    void initialize();
    void submit_file(const char *path);
    void clear_preview();
    void handle_resize(int w, int h);

// create the widgets
    void create_preview();

    void start_playback();
    void stop_playback();
    void interrupt_playback();
    void rewind_playback();
    void seek_playback();
    void write_frame(VFrame *frame);
    void update_scrollbar(int lock_it);
    void reset_play_button();
// do it in the window thread
    void reset_play_async();

// wait for a new file
    Condition *file_waiter;
    Mutex *analyzer_lock;
// next file to analyze
    string next_path;
    int have_next;


    FilePreviewerThread *thread;
    PreviewPlayback *playback_engine;

// widgets in the file dialog
    PreviewCanvas *canvas;
    FilePreviewerPlay *play;
    FilePreviewerRewind *rewind;
    FilePreviewerScroll *scroll;
// bits for creating the widgets
    EDL *edl;
    VFrame **play_images;
    VFrame **rewind_images;
    VFrame **stop_images;
    VFrame *speaker_image;
// video output from the playback engine
    VFrame *output_frame;
    double play_position;
// only update the play_position when this is 1
    int is_playing;
};



#endif
