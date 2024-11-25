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



#include "textengine.h"
#include "vframe.h"



FreetypeGlyph::FreetypeGlyph()
{
	c = 0;
	data = 0;
	freetype_index = 0;
	height = 0;
	top = 0;
	width = 0;
	advance_w = 0;
	pitch = 0;
	left = 0;
}

FreetypeGlyph::~FreetypeGlyph()
{
    delete data;
}





TextEngine::TextEngine()
{
    freetype_library = 0;
    freetype_face = 0;
    positions = 0;
    outline_size = 0;
    text_mask = 0;
    outline_mask = 0;
    size = 0;
    outline_size = 0;
    font_path[0] = 0;
}

TextEngine::~TextEngine()
{
    if(freetype_library) FT_Done_FreeType(freetype_library);
    glyphs.remove_all_objects();
    delete [] positions;
    delete text_mask;
    delete outline_mask;
}

int TextEngine::load(const char *path, int size)
{
    if(strcmp(font_path, path) || size != this->size)
    {
        this->size = size;
        strcpy(font_path, path);
        glyphs.remove_all_objects();
        delete [] positions;
        positions = 0;

printf("TextEngine::load %d: loading %s\n", __LINE__, path);
        FT_Init_FreeType(&freetype_library);
        if(FT_New_Face(freetype_library, 
		    path, 
		    0,
		    &freetype_face))
	    {
		    fprintf(stderr, "TextEngine::load %s failed.\n", path);
		    freetype_face = 0;
		    return 1;
	    }
        else
	    {
            FT_Set_Pixel_Sizes(freetype_face, size, 0);
		    return 0;
	    }
    }
}

void TextEngine::set_text(const char *text)
{
    this->text.assign(text);
}

void TextEngine::set_outline(int outline_size)
{
    this->outline_size = outline_size;
}

FreetypeGlyph* TextEngine::get_glyph(int c)
{
	if(c == 0xa) return 0;

	for(int i = 0; i < glyphs.size(); i++)
	{
		if(glyphs.get(i)->c == c)
		{
			return glyphs.get(i);
		}
	}
	
	return 0;
}


int TextEngine::get_char_advance(int current, int next)
{
	FT_Vector kerning;
	int result = 0;
	FreetypeGlyph *current_glyph = 0;
	FreetypeGlyph *next_glyph = 0;

	if(current == 0xa) return 0;

	for(int i = 0; i < glyphs.size(); i++)
	{
		if(glyphs.get(i)->c == current)
		{
			current_glyph = glyphs.get(i);
			break;
		}
	}

	for(int i = 0; i < glyphs.size(); i++)
	{
		if(glyphs.get(i)->c == next)
		{
			next_glyph = glyphs.get(i);
			break;
		}
	}

	if(current_glyph)
		result = current_glyph->advance_w;

//printf("TitleMain::get_char_advance 1 %c %c %p %p\n", current, next, current_glyph, next_glyph);
	if(next_glyph)
		FT_Get_Kerning(freetype_face, 
				current_glyph->freetype_index,
				next_glyph->freetype_index,
				ft_kerning_default,
				&kerning);
	else
		kerning.x = 0;
//printf("TitleMain::get_char_advance 2 %d %d\n", result, kerning.x);

	return result + (kerning.x >> 6);
}

void TextEngine::draw_glyph(FreetypeGlyph *glyph, int x, int y)
{
	int glyph_w = glyph->data->get_w();
	int glyph_h = glyph->data->get_h();
	int output_w = text_w;
	int output_h = text_h;
	unsigned char **in_rows = glyph->data->get_rows();
	unsigned char **out_rows = text_mask->get_rows();

//printf("TitleUnit::draw_glyph 1 %c %d %d\n", glyph->c, x - 1, y - 1);
	for(int in_y = 0; in_y < glyph_h; in_y++)
	{
		int y_out = y + in_y /* + plugin->ascent - glyph->top */;

		if(y_out >= 0 && y_out < output_h)
		{
			unsigned char *in_row = in_rows[in_y];
			int x1 = x /* + glyph->left */;
			int y1 = y_out;

			unsigned char *out_row = out_rows[y1];
// Blend color value with shadow using glyph alpha.
			for(int in_x = 0; in_x < glyph_w; in_x++)
			{
				int x_out = x1 + in_x;
				if(x_out >= 0 && x_out < output_w)
				{
					int opacity = in_row[in_x];
					int transparency = 0xff - opacity;
					if(in_row[in_x] > 0)
					{
						out_row[x_out] = (opacity * 0xff + out_row[x_out] * transparency) / 0xff;
//if(opacity > 1) printf("draw_glyph %d\n", out_row[x_out]);
					}
				}
			}
		}
	}
}

void TextEngine::draw_glyphs()
{
    for(int i = 0; i < text.size(); i++)
    {
        int c = text.at(i);
        FreetypeGlyph *glyph = get_glyph(c);

        if(!glyph)
        {
            glyph = new FreetypeGlyph;
            glyphs.append(glyph);
			glyph->c = c;
            
            
            if(FT_Load_Char(freetype_face, glyph->c, FT_LOAD_RENDER))
            {
			    printf("GlyphUnit::process_package FT_Load_Char failed.\n");
    // Prevent a crash here
			    glyph->width = 8;
			    glyph->height = 8;
			    glyph->pitch = 8;
			    glyph->left = 9;
			    glyph->top = 9;
			    glyph->freetype_index = 0;
			    glyph->advance_w = 8;
			    glyph->data = new VFrame;
			    glyph->data->set_use_shm(0);
			    glyph->data->reallocate(0,
				    -1,
				    0,
				    0,
				    0,
				    8,
				    8,
				    BC_A8,
				    8);
		    }
		    else
		    {
			    glyph->width = freetype_face->glyph->bitmap.width;
			    glyph->height = freetype_face->glyph->bitmap.rows;

    // printf("GlyphUnit::process_package %d c=%c top=%d rows=%d height=%d\n", 
    // __LINE__,
    // glyph->char_code,
    // freetype_face->glyph->bitmap_top,
    // freetype_face->glyph->bitmap.rows,
    // glyph->height);

			    glyph->pitch = freetype_face->glyph->bitmap.pitch;
			    glyph->left = freetype_face->glyph->bitmap_left;
			    glyph->top = freetype_face->glyph->bitmap_top;
			    glyph->freetype_index = FT_Get_Char_Index(freetype_face, 
                    glyph->c);
			    glyph->advance_w = (freetype_face->glyph->advance.x >> 6);

    //printf("GlyphUnit::process_package 1 width=%d height=%d pitch=%d left=%d top=%d advance_w=%d freetype_index=%d\n", 
    //	glyph->width, glyph->height, glyph->pitch, glyph->left, glyph->top, glyph->advance_w, glyph->freetype_index);

    //printf("GlyphUnit::process_package 1\n");
			    glyph->data = new VFrame;
			    glyph->data->set_use_shm(0);
			    glyph->data->reallocate(0,
				    -1,
				    0,
				    0,
				    0,
				    glyph->width,
				    glyph->height,
				    BC_A8,
				    glyph->pitch);
			    glyph->data->clear_frame();

    //printf("GlyphUnit::process_package 2\n");
			    for(int i = 0; i < freetype_face->glyph->bitmap.rows; i++)
			    {
				    memcpy(glyph->data->get_rows()[i], 
					    freetype_face->glyph->bitmap.buffer + glyph->pitch * i,
					    glyph->pitch);
			    }
		    }
        }
    }
}


void TextEngine::get_total_extents()
{
// Determine extents of total text
	int current_x = 0;
    int min_x = 65536;
    int max_x = -65536;
    int min_y = 65536;
    int max_y = -65536;
	int row_start = 0;
    int line_spacing = size;
    int text_rows = 0;
    text_w = 0;

    delete [] positions;
    positions = new GlyphPosition[text.size()];
    
	for(int i = 0; i < text.size(); i++)
	{
        int current_y = text_rows * line_spacing;
        FreetypeGlyph *glyph = get_glyph(text.at(i));
        if(glyph)
        {
// printf("TitleMain::get_total_extents %d: %c left=%d top=%d w=%d h=%d\n", 
// __LINE__,
// (char)config.ucs4text[i],
// glyph->left,
// glyph->top,
// glyph->width,
// glyph->height);
            int top = current_y - glyph->top;
            if(top < min_y) min_y = top;
            int bottom = top + 
                glyph->height;
            if(bottom > max_y) max_y = bottom;
            int right = current_x + glyph->left + glyph->width;
            if(right > max_x) max_x = right;
            int left = current_x + glyph->left;
            if(left < min_x) min_x = left;
            positions[i].x = left;
		    positions[i].y = top;
		    positions[i].w = glyph->width;
		    positions[i].h = glyph->height;
        }
        else
        {
            positions[i].x = current_x;
		    positions[i].y = current_y;
		    positions[i].w = 0;
		    positions[i].h = 0;
        }

		int char_advance = 0;
        if(i < text.size() - 1) char_advance = get_char_advance(text.at(i), text.at(i + 1));
        current_x += char_advance;
        positions[i].line_w = 0;
        positions[i].end_of_line = 0;

// printf("TitleMain::get_total_extents 1 %c x=%d y=%d w=%d\n", 
// config.text[i], 
// char_positions[i].x, 
// char_positions[i].y, 
// char_positions[i].w);

		if(text.at(i) == 0xa || i == text.size() - 1)
		{
            int line_w = max_x - min_x;
            if(line_w > text_w) text_w = line_w;
// store the line_w for later
            positions[i].line_w = line_w;
            positions[i].end_of_line = 1;

//printf("TitleMain::get_total_extents %d i=%d min_x=%d max_x=%d line_w=%d\n", 
//__LINE__, i, min_x, max_x, line_w);

// shift line horizontally
            for(int j = row_start; j <= i; j++)
            {
                positions[j].x -= min_x;
            }

            current_y += line_spacing;
			text_rows++;
			current_x = 0;
            min_x = 65536;
            max_x = -65536;
            row_start = i + 1;
		}
    }    

    text_h = max_y - min_y;

// shift again for vertical offset
	for(int i = 0; i < text.size(); i++)
	{
		positions[i].y += -min_y;
        positions[i].x += outline_size;
        positions[i].y += outline_size;
//printf("TitleMain::get_total_extents %d x=%d\n", __LINE__, char_positions[i].x);
    }
    
    text_w += outline_size * 2;
    text_h += outline_size * 2;
}

void TextEngine::draw()
{
    draw_glyphs();
    get_total_extents();
   
	if(text_mask &&
		(text_mask->get_w() != text_w ||
		text_mask->get_h() != text_h))
	{
		delete text_mask;
		text_mask = 0;
	}

//printf("TextEngine::draw %d text_w=%d text_h=%d\n", __LINE__, text_w, text_h);
	if(!text_mask)
	{
		text_mask = new VFrame;
		text_mask->set_use_shm(0);
		text_mask->reallocate(0,
			-1,
			0,
			0,
			0,
			text_w,
			text_h,
			BC_A8,
			-1);
	}
    text_mask->clear_frame();


    for(int i = 0; i < text.size(); i++)
    {
        FreetypeGlyph *glyph = get_glyph(text.at(i));
        if(glyph)
        {
            GlyphPosition *position = &positions[i];
            draw_glyph(glyph, position->x, position->y);
        }
    }


    if(outline_size > 0)
    {
	    if(outline_mask &&
		    (outline_mask->get_w() != text_w ||
		    outline_mask->get_h() != text_h))
	    {
		    delete outline_mask;
		    outline_mask = 0;
	    }

	    if(!outline_mask)
	    {
		    outline_mask = new VFrame;
		    outline_mask->set_use_shm(0);
		    outline_mask->reallocate(0,
			    -1,
			    0,
			    0,
			    0,
			    text_w,
			    text_h,
			    BC_A8,
			    -1);
	    }
        outline_mask->clear_frame();
        
        do_outline(text_mask, outline_mask, outline_size);
    }
}


void TextEngine::do_outline(VFrame *src, VFrame *dst, int outline_size)
{
    int w = src->get_w();
    int h = src->get_h();
    for(int i = 0; i < h; i++)
	{
		unsigned char *out_row = dst->get_rows()[i];
		for(int j = 0; j < w; j++)
		{
			int x1 = j - outline_size;
			int x2 = j + outline_size;
			int y1 = i - outline_size;
			int y2 = i + outline_size;
			CLAMP(x1, 0, w - 1);
			CLAMP(x2, 0, w - 1);
			CLAMP(y1, 0, h - 1);
			CLAMP(y2, 0, h - 1);
			unsigned char *out_pixel = out_row + j;
            int max_a = 0;

			for(int k = y1; k <= y2; k++)
			{
				unsigned char *in_row = src->get_rows()[k];
				for(int l = x1; l <= x2; l++)
				{
					unsigned char value = in_row[l];

					if(value > max_a)
					{
						max_a = value;
					}
				}
			}

			*out_pixel = max_a;
		}
	}
}









