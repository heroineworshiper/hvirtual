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

#include "condition.inc"
#include "filepreviewer.inc"
#include "mutex.inc"
#include "previewer.h"
#include "thread.h"

#include <string>

using std::string;



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
class FilePreviewer : public Previewer
{
public:
    FilePreviewer();


    static FilePreviewer instance;

    void initialize();
// kick off the background thread to analyze the file
    void submit_file(const char *path);
// create the widgets
    void create_preview();
    void create_info(BC_FileBox *filebox, int x, int y, double length);
    void handle_resize(int w, int h);

// wait for a new file
    Condition *file_waiter;
    Mutex *analyzer_lock;
// next file to analyze
    string next_path;
    int have_next;


    FilePreviewerThread *thread;

};



#endif
