/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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
#include "bchash.h"
#include "edl.h"
#include "errorbox.h"
#include "file.h"
#include "filexml.h"
#include "fileformat.h"
#include "filepreviewer.h"
#include "indexfile.h"
#include "language.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playback3d.h"
#include "savefile.h"
#include "mainsession.h"

#include <string.h>









SaveBackup::SaveBackup(MWindow *mwindow)
 : BC_MenuItem(_("Save backup"))
{
	this->mwindow = mwindow;
}
int SaveBackup::handle_event()
{
	mwindow->save_backup();
	mwindow->gui->show_message(_("Saved backup."));
	return 1;
}











Save::Save(MWindow *mwindow) : BC_MenuItem(_("Save"), "s", 's')
{ 
	this->mwindow = mwindow; 
	quit_now = 0; 
}

void Save::create_objects(SaveAs *saveas)
{
	this->saveas = saveas;
}

int Save::handle_event()
{
	if(mwindow->session->filename[0] == 0) 
	{
		saveas->start();
	}
	else
	{
// save it
// TODO: Move this into mwindow.
		FileXML file;
		mwindow->edl->save_xml(&file, 
			mwindow->session->filename,
			0,
			0);
		file.terminate_string();

		if(file.write_to_file(mwindow->session->filename))
		{
			char string2[256];
			sprintf(string2, _("Couldn't open %s"), mwindow->session->filename);
			ErrorBox error(PROGRAM_NAME ": Error",
				mwindow->gui->get_abs_cursor_x(1),
				mwindow->gui->get_abs_cursor_y(1));
			error.create_objects(string2);
			error.raise_window();
			error.run_window();
			return 1;		
		}
		else
		{
			char string[BCTEXTLEN];
			sprintf(string, 
				_("\"%s\" %dC written"), 
				mwindow->session->filename, 
				(int)strlen(file.string));
			mwindow->gui->show_message(string);
		}
		mwindow->session->changes_made = 0;
// Last command in program
//		if(saveas->quit_now) mwindow->gui->set_done(0);
		if(saveas->quit_now) mwindow->playback_3d->quit();
	}
	return 1;
}

int Save::save_before_quit()
{
	saveas->quit_now = 1;
	handle_event();
	return 0;
}

SaveAs::SaveAs(MWindow *mwindow)
 : BC_MenuItem(_("Save as..."), "Shift+S", 'S'), Thread()
{ 
	this->mwindow = mwindow; 
	quit_now = 0;
	set_shift(); 
}

int SaveAs::set_mainmenu(MainMenu *mmenu)
{
	this->mmenu = mmenu;
	return 0;
}

int SaveAs::handle_event() 
{ 
	quit_now = 0;
	start();
	return 1;
}

void SaveAs::run()
{
// ======================================= get path from user
	int result;
//printf("SaveAs::run 1\n");
	char directory[1024], filename[1024];
	sprintf(directory, "~");
	mwindow->defaults->get("DIRECTORY", directory);

// Loop if file exists
	do{
		SaveFileWindow *window;

		window = new SaveFileWindow(mwindow, directory);
		window->lock_window("SaveAs::run");
		window->create_objects();
		window->unlock_window();
		result = window->run_window();
		mwindow->defaults->update("DIRECTORY", window->get_submitted_path());
		strcpy(filename, window->get_submitted_path());
		delete window;

// Extend the filename with .xml
		if(strlen(filename) < 4 || 
			strcasecmp(&filename[strlen(filename) - 4], ".xml"))
		{
			strcat(filename, ".xml");
		}

// ======================================= try to save it
		if(filename[0] == 0) return;              // no filename given
		if(result == 1) return;          // user cancelled
		result = ConfirmSave::test_file(mwindow, filename);
	}while(result);        // file exists so repeat

//printf("SaveAs::run 6 %s\n", filename);




// save it
	FileXML file;
	mwindow->gui->lock_window("SaveAs::run 1");
// update the project name
	mwindow->set_filename(filename);      
	mwindow->edl->save_xml(&file, 
		filename,
		0,
		0);
	mwindow->gui->unlock_window();
	file.terminate_string();

	if(file.write_to_file(filename))
	{
		char string2[256];
		mwindow->set_filename("");      // update the project name
		sprintf(string2, _("Couldn't open %s."), filename);
		ErrorBox error(PROGRAM_NAME ": Error",
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
		error.create_objects(string2);
		error.raise_window();
		error.run_window();
		return;		
	}
	else
	{
		char string[BCTEXTLEN];
		sprintf(string, _("\"%s\" %dC written"), filename, (int)strlen(file.string));
		mwindow->gui->lock_window("SaveAs::run 2");
		mwindow->gui->show_message(string);
		mwindow->gui->unlock_window();
	}


	mwindow->session->changes_made = 0;
	mmenu->add_load(filename);
// Last command in program
//	if(quit_now) mwindow->gui->set_done(0);
	if(quit_now) mwindow->playback_3d->quit();
	return;
}








SaveFileWindow::SaveFileWindow(MWindow *mwindow, char *init_directory)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(1),
 	mwindow->gui->get_abs_cursor_y(1) - BC_WindowBase::get_resources()->filebox_h / 2,
 	init_directory, 
	PROGRAM_NAME ": Save", 
	_("Enter a filename to save as"))
{ 
	this->mwindow = mwindow; 
    set_previewer(&FilePreviewer::instance);
}

SaveFileWindow::~SaveFileWindow() {}

