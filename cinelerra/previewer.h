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

// Bits shared between the filebox & transition previewers 
// Inherits from BC_FileBoxPreview to get previewer_lock
// but another guicast preview user might screw this up.

#ifndef PREVIEWER_H
#define PREVIEWER_H

#include "guicast.h"
#include "playbackengine.h"
#include "previewer.inc"

class PreviewerScroll : public BC_ISlider
{
public:
    PreviewerScroll(Previewer *previewer, 
        int x,
        int y,
        int w);
    int handle_event();
    Previewer *previewer;
};

class PreviewerPlay : public BC_Button
{
public:
    PreviewerPlay(Previewer *previewer, 
        int x,
        int y);
    int handle_event();
    Previewer *previewer;
    int is_play;
};

// required by non seekable files
class PreviewerRewind : public BC_Button
{
public:
    PreviewerRewind(Previewer *previewer, 
        int x,
        int y);
    int handle_event();
    Previewer *previewer;
};

class PreviewCanvas : public BC_SubWindow
{
public:
    PreviewCanvas(Previewer *previewer, 
        int x,
        int y,
        int w,
        int h);
    int button_release_event();
    Previewer *previewer;
};

class PreviewPlayback : public PlaybackEngine
{
public:
    PreviewPlayback(Previewer *previewer);
	void init_cursor();
	void stop_cursor();
    void update_tracker(double position);
    Previewer *previewer;
};

class Previewer : public BC_FileBoxPreviewer
{
public:
    Previewer();

    virtual void initialize();

    void clear_preview();

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


    PreviewPlayback *playback_engine;
// video output copied from the playback engine
    VFrame *output_frame;
    double play_position;
// only update the play_position when this is 1
    int is_playing;

// widgets
    PreviewCanvas *canvas;
    PreviewerPlay *play;
    PreviewerRewind *rewind;
    PreviewerScroll *scroll;
    BC_Title *info_text;
// bits for creating the widgets
    EDL *edl;
    VFrame **play_images;
    VFrame **rewind_images;
    VFrame **stop_images;
    VFrame *speaker_image;
    int seekable;
};





#endif



