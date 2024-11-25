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

// wrapper for freetype
// taken from a subset of the titler

#ifndef TEXTENGINE_H
#define TEXTENGINE_H

#include "arraylist.h"
#include "bcwindowbase.inc"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <sys/types.h>
#include <string>
#include "vframe.inc"

using std::string;

// A single character in the glyph cache
class FreetypeGlyph
{
public:
    FreetypeGlyph();
    ~FreetypeGlyph();
// character
	int c;
	int width, height, pitch, advance_w, left, top, freetype_index;
	VFrame *data;
};

// Position of each character relative to total text extents
typedef struct
{
	int x, y, w, h;
// hints for computing the extents
    int line_w;
    int end_of_line;
} GlyphPosition;

class TextEngine
{
public:
    TextEngine();
    ~TextEngine();

// load a font file
    int load(const char *path, int size);
    void set_text(const char *text);
    void set_outline(int outline_size);
// draw it
    void draw();
    static void do_outline(VFrame *src, VFrame *dst, int outline_size);


    int size;
    int outline_size;
// Text to display
	string text;
    int text_w;
    int text_h;
    VFrame *text_mask;
    VFrame *outline_mask;

private:
    void draw_glyphs();
    void get_total_extents();
    FreetypeGlyph* get_glyph(int c);
    int get_char_advance(int current, int next);
    void draw_glyph(FreetypeGlyph *glyph, int x, int y);

    char font_path[BCTEXTLEN];
	FT_Library freetype_library;      	// Freetype library
	FT_Face freetype_face;
    ArrayList<FreetypeGlyph*> glyphs;
    GlyphPosition* positions;
};



#endif
