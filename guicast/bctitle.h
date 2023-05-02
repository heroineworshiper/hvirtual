
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef BCTITLE_H
#define BCTITLE_H

#include "bcsubwindow.h"
#include "colors.h"
#include "fonts.h"
#include <string>

using std::string;


class BC_Title : public BC_SubWindow
{
public:
	BC_Title(int x, 
		int y, 
		const char *text, 
		int font = MEDIUMFONT, 
		int color = -1, 
		int centered = 0,
		int fixed_w = 0, // causes ... to be printed when the width is exceeded
        int do_wrap = 0); // enable word wrapping
	virtual ~BC_Title();
	
	int initialize();
	static int calculate_w(BC_WindowBase *gui, 
        const char *text, 
        int font = MEDIUMFONT);
	static int calculate_h(BC_WindowBase *gui, 
        const char *text, 
        int font = MEDIUMFONT);
	int resize(int w, int h);
	int reposition(int x, int y, int fixed_w = 0);
	int set_color(int color);
	int update(const char *text, int flush = 1);
	void update(float value);
	const char* get_text();

private:
	int draw(int flush);
	static void get_size(BC_WindowBase *gui, 
        int font, 
        const char *text, 
        int fixed_w, 
        int &w, 
        int &h);
// insert newlines in src to wrap the text
    static void wrap_text(BC_WindowBase *gui, 
        int font,
        string *dst, 
        const char *src,
        int fixed_w);

// last text set by the user, after word wrapping
	string text;
	int color;
	int font;
	int centered;
// Width if fixed.  0 if variable width
	int fixed_w;
// word wrapping
    int do_wrap;
};

#endif
