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

#include "confirmsave.h"
#include "filepreviewer.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "save.h"
#include <string.h>

SaveBackupItem::SaveBackupItem()
 : BC_MenuItem(_("Save backup"))
{
}
int SaveBackupItem::handle_event()
{
	MWindow::instance->save_backup();
	MWindow::instance->gui->show_message(_("Saved backup."));
	return 1;
}






SaveItem::SaveItem()
 : BC_MenuItem(_("Save"), "s", 's')
{
}


int SaveItem::handle_event()
{
// get path from user
	if(MWindow::instance->session->filename[0] == 0) 
	{
		MWindow::save_thread->start();
	}
	else
	{
// save it
        MWindow::instance->save_xml(MWindow::instance->session->filename, 
            1, 
            MWindow::save_thread->quit_now);
	}
	return 1;
}

int SaveItem::save_before_quit()
{
	MWindow::save_thread->quit_now = 1;
	handle_event();
	return 0;
}




SaveAsItem::SaveAsItem()
 : BC_MenuItem(_("Save as..."), "Shift+S", 'S')
{
	set_shift(); 
}

int SaveAsItem::handle_event() 
{ 
	MWindow::save_thread->quit_now = 0;
	MWindow::save_thread->start();
	return 1;
}




SaveClipItem::SaveClipItem()
 : BC_MenuItem(_("Save selection..."))
{
}

int SaveClipItem::handle_event() 
{
	MWindow::save_thread->quit_now = 0;
    MWindow::save_thread->do_clip = 1;
	MWindow::save_thread->start();
	return 1;
}









SaveThread::SaveThread() 
 : BC_DialogThread()
{
    reset_flags();
    set_async_gui();
}

SaveThread::~SaveThread()
{
}

BC_Window* SaveThread::new_gui()
{
	char init_path[BCTEXTLEN];
	sprintf(init_path, "~");
	MWindow::defaults->get("DIRECTORY", init_path);
	window = new SaveWindow(init_path, do_clip);
	window->create_objects();
    return window;
}


void SaveThread::handle_done_event(int result)
{
	char path[BCTEXTLEN];
    MWindow::defaults->update("DIRECTORY", window->get_submitted_path());
    window->lock_window("SaveThread::handle_done_event");
    window->hide_window(1);
    window->unlock_window();

// user cancelled
	if(result == 1)
    {
        reset_flags();
        return;
    }


    strcpy(path, window->get_submitted_path());
// no filename given
	if(path[0] == 0)
    {
        reset_flags();
        return;
    }


// Extend the filename with .xml
	if(strlen(path) < 4 || 
		strcasecmp(&path[strlen(path) - 4], ".xml"))
	{
		strcat(path, ".xml");
	}

	result = ConfirmSave::test_file(MWindow::instance, path);

    if(result)
// file exists so restart the dialog
    {
        set_restart();
    }
    else
// save it
    {
        if(do_clip)
        {
            MWindow::instance->save_clip(path);
        }
        else
        {
            MWindow::instance->save_xml(path, 
                1, 
                quit_now);
        }
        reset_flags();
    }
}

void SaveThread::reset_flags()
{
    quit_now = 0;
    do_clip = 0;
}


SaveWindow::SaveWindow(char *init_path, int do_clip)
 : BC_FileBox(MWindow::instance->gui->get_abs_cursor_x(1),
 	MWindow::instance->gui->get_abs_cursor_y(1) - 
        BC_WindowBase::get_resources()->filebox_h / 2,
 	init_path, 
	do_clip ? PROGRAM_NAME ": Save selection" : PROGRAM_NAME ": Save", 
	_("Enter a filename to save as"))
{ 
    set_previewer(&FilePreviewer::instance);
}

SaveWindow::~SaveWindow() 
{
}














