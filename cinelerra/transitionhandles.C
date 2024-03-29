/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
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

#include "cursors.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "mwindow.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "transitionhandles.h"

TransitionHandle::TransitionHandle(MWindow *mwindow, 
	TrackCanvas *trackcanvas,
	Edit *edit,
	int x,
	int y)
 : CanvasTool(mwindow, 
		trackcanvas,
		edit,
		x,
		y,
		0)
//		mwindow->theme->transitionhandle_data)
{
}

TransitionHandle::~TransitionHandle()
{
}

int TransitionHandle::handle_event()
{
	return 0;
}




TransitionHandles::TransitionHandles(MWindow *mwindow,
		TrackCanvas *trackcanvas)
 : CanvasTools(mwindow, trackcanvas)
{
}

TransitionHandles::~TransitionHandles()
{
}


void TransitionHandles::update()
{
	decrease_visible();

	for(Track *current = mwindow->edl->tracks->first;
		current;
		current = NEXT)
	{
		for(Edit *edit = current->edits->first; edit; edit = edit->next)
		{
			if(edit->transition)
			{
				int64_t edit_x, edit_y, edit_w, edit_h;
				trackcanvas->edit_dimensions(edit, 
                    edit_x, 
                    edit_y, 
                    edit_w, 
                    edit_h);
				trackcanvas->get_transition_coords(
                    edit->transition,
                    edit->transition->title,
                    edit_x, 
                    edit_y, 
                    edit_w, 
                    edit_h);
				
				if(visible(edit_x, edit_w, edit_y, edit_h))
				{
					int exists = 0;

					for(int i = 0; i < total; i++)
					{
						TransitionHandle *handle = (TransitionHandle*)values[i];
						if(handle->edit->id == edit->id)
						{
							handle->reposition_window(edit_x, edit_y);
							handle->visible = 1;
							exists = 1;
							break;
						}
					}

					if(!exists)
					{
						TransitionHandle *handle = new TransitionHandle(mwindow,
							trackcanvas,
							edit,
							edit_x,
							edit_y);
						trackcanvas->add_subwindow(handle);
						handle->set_cursor(ARROW_CURSOR, 0, 0);
						append(handle);
					}
				}
			}
		}
	}

	delete_invisible();
}
