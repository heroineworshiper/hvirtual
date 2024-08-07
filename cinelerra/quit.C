
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "assets.h"
#include "mbuttons.h"
#include "confirmquit.h"
#include "errorbox.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playback3d.h"
#include "quit.h"
#include "record.h"
#include "render.h"
#include "save.h"
#include "mainsession.h"
#include "videowindow.h"
#include "videowindowgui.h"


Quit::Quit(MWindow *mwindow)
 : BC_MenuItem(_("Quit"), "q", 'q'), Thread() 
{ 
	this->mwindow = mwindow; 
}

void Quit::create_objects(SaveItem *save)
{ 
	this->save = save; 
}

int Quit::handle_event() 
{

//printf("Quit::handle_event 1 %d\n", mwindow->session->changes_made);
	if(mwindow->session->changes_made ||
		mwindow->gui->mainmenu->record->current_state ||
		mwindow->render->in_progress) 
	{
		start();
	}
	else 
	{        // quit
		mwindow->quit(1);
	}
	return 0;
}

void Quit::run()
{
	int result = 0;

// Test execution conditions
	if(mwindow->gui->mainmenu->record->current_state == RECORD_CAPTURING)
	{
		ErrorBox error(PROGRAM_NAME ": Error", 
			mwindow->gui->get_abs_cursor_x(1), 
			mwindow->gui->get_abs_cursor_y(1));
		error.create_objects(_("Can't quit while a recording is in progress."));
		error.run_window();
		return;
	}
	else
	if(mwindow->render->thread->running())
	{
		ErrorBox error(PROGRAM_NAME ": Error", 
			mwindow->gui->get_abs_cursor_x(1), 
			mwindow->gui->get_abs_cursor_y(1));
		error.create_objects(_("Can't quit while a render is in progress."));
		error.run_window();
		return;
	}


//printf("Quit::run 1\n");

// Quit
	{
		ConfirmQuitWindow confirm(mwindow);
		confirm.create_objects(_("Save edit list before exiting?"));
		result = confirm.run_window();
	}

	switch(result)
	{
		case 0:          // quit
			if(mwindow->gui)
			{
				mwindow->quit(0);
			}
			break;

		case 1:        // cancel
			return;
			break;

		case 2:           // save
			save->save_before_quit(); 
			return;
			break;
	}
}
