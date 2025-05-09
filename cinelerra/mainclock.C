/*
 * CINELERRA
 * Copyright (C) 1997-2024 Adam Williams <broadcast at earthling dot net>
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

#include "edl.h"
#include "edlsession.h"
#include "fonts.h"
#include "mainclock.h"
#include "mwindow.h"
#include "theme.h"

MainClock::MainClock(MWindow *mwindow, int x, int y, int w)
 : BC_Title(x,
 		y,
		"", 
//		MEDIUM_7SEGMENT,
		CLOCKFONT,
		mwindow->theme->clock_fg_color,
		0,
		w)
{
	this->mwindow = mwindow;
	set_bg_color(mwindow->theme->clock_bg_color);
}

MainClock::~MainClock()
{
}

void MainClock::update(double position)
{
	char string[BCTEXTLEN];
	Units::totext(string, 
		position,
		mwindow->edl->session->time_format,
		mwindow->edl->session->sample_rate,
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
// printf("MainClock::update %d position=%f %f %f %f %s\n", 
// __LINE__, 
// position, 
// position * mwindow->edl->session->frame_rate,
// (int)position * mwindow->edl->session->frame_rate,
// position * mwindow->edl->session->frame_rate - (int)position * mwindow->edl->session->frame_rate,
// string);
	BC_Title::update(string);
}





