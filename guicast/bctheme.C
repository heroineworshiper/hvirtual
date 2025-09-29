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

#include "bcresources.h"
#include "bctheme.h"
#include "bcwindowbase.h"
#include "clip.h"
#include "language.h"
#include "vframe.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

BC_Theme::BC_Theme()
{
    BC_Resources::theme = this;
	last_image = 0;
	last_pointer = 0;

    widget_border = DP(5);
}

BC_Theme::~BC_Theme()
{
    for(auto i = image_sets.begin(); i != image_sets.end(); i++)
        delete i->second;
}

void BC_Theme::dump()
{
	printf("BC_Theme::dump %d image_sets=%d images=%d\n", 
        __LINE__,
		(int)image_sets.size(), 
		(int)images.size());
	for(auto i = images.begin(); i != images.end(); i++)
		printf("    %s %p\n", i->first.c_str(), i->second);
}

BC_Resources* BC_Theme::get_resources()
{
	return BC_WindowBase::get_resources();
}


// These create single images for storage in the image_sets table.
VFrame* BC_Theme::new_image(const char *title, const char *path)
{
//printf("BC_Theme::new_image %d: %s %s\n", __LINE__, title, path);
// 	VFrame *existing_image = title[0] ? get_image(title, 0) : 0;
// 	if(existing_image) return existing_image;

	BC_ThemeSet *result = new BC_ThemeSet(1, 0, title);
//	result->data[0] = new VFrame(get_image_data(path));
	result->data[0] = new VFrame();
    result->data[0]->set_use_shm(0);
	result->data[0]->read_png(get_image_data(path), BC_Resources::dpi);

// delete previous image set
    if(image_sets.find(title) != image_sets.end())
    {
//         printf("BC_Theme::new_image %d: deleting previous %s %p\n", 
//             __LINE__, 
//             title,
//             image_sets[title]);
        delete image_sets[title];
//printf("BC_Theme::new_image %d %s new=%p previous=%p\n",
//__LINE__, title, result, image_sets[title]);
    }

	image_sets[title] = result;

	return result->data[0];
}

VFrame* BC_Theme::new_image(const char *path)
{
    VFrame *result = new VFrame;
    result->read_png(get_image_data(path), BC_Resources::dpi);
    result->set_use_shm(0);
    return result;
//	return new_image("", path);
}



// These create image sets which are stored in the image_sets table.
VFrame** BC_Theme::new_image_set(const char *title, int total, va_list *args)
{
	if(!total)
	{
		printf("BC_Theme::new_image_set %d: %s zero number of images\n",
			__LINE__,
			title);
	}

	VFrame **existing_image_set = title[0] ? get_image_set(title, 0) : 0;
	if(existing_image_set) return existing_image_set;

	BC_ThemeSet *result = new BC_ThemeSet(total, 1, title);
    image_sets[title] = result;
	for(int i = 0; i < total; i++)
	{
		char *path = va_arg(*args, char*);
		result->data[i] = new_image(path);
	}
	return result->data;
}

VFrame** BC_Theme::new_image_set_images(const char *title, int total, ...)
{
	va_list list;
	va_start(list, total);
	BC_ThemeSet *existing_image_set = title[0] ? get_image_set_object(title) : 0;
	if(existing_image_set)
	{
		delete existing_image_set;
	}

	BC_ThemeSet *result = new BC_ThemeSet(total, 0, title);
	image_sets[title] = result;
	for(int i = 0; i < total; i++)
	{
		result->data[i] = va_arg(list, VFrame*);
	}
	va_end(list);
	return result->data;
}

VFrame** BC_Theme::new_image_set(const char *title, int total, ...)
{
	va_list list;
	va_start(list, total);
	VFrame **result = new_image_set(title, total, &list);
	va_end(list);

	return result;
}

VFrame** BC_Theme::new_image_set(int total, ...)
{
	va_list list;
	va_start(list, total);
	VFrame **result = new_image_set("", total, &list);
	va_end(list);

	return result;
}

// get 1 image from an image set
VFrame* BC_Theme::get_image(const char *title, int use_default)
{
    auto i = image_sets.find(title);
    if(i != image_sets.end())
        return i->second->data[0];


// Return the first image if nothing found.
	if(use_default)
	{
		printf("BC_Theme::get_image %d: image \"%s\" not found.\n",
            __LINE__,
			title);
		if(image_sets.begin() != image_sets.end())
		{
            i = image_sets.begin();
			return i->second->data[0];
		}
	}

// Give up and go to a movie.
	return 0;
}

VFrame** BC_Theme::get_image_set(const char *title, int use_default)
{
    auto i = image_sets.find(title);
    if(i != image_sets.end())
        return i->second->data;

// Get the image set with the largest number of images.
	if(use_default)
	{
		printf("BC_Theme::get_image_set %d: image set \"%s\" not found.\n",
            __LINE__,
			title);
		int max_total = 0;
		auto max_i = image_sets.end();
		for(i = image_sets.begin(); i != image_sets.end(); i++)
		{
            BC_ThemeSet *image_set = i->second;
			if(image_set->total > max_total)
			{
				max_total = image_set->total;
				max_i = i;
			}
		}

		if(max_i != image_sets.end())
			return max_i->second->data;
	}

// Give up and go to a movie
	return 0;
}

BC_ThemeSet* BC_Theme::get_image_set_object(const char *title)
{
    return image_sets[title];
}

void BC_Theme::set_image_set(const char *title, BC_ThemeSet *image_set)
{
    auto i = image_sets.find(title);
    if(i != image_sets.end()) delete i->second;
    image_sets[title] = image_set;
}








VFrame** BC_Theme::new_button(const char *overlay_path, 
	const char *up_path, 
	const char *hi_path, 
	const char *dn_path,
	const char *title)
{
//	VFrame default_data(get_image_data(overlay_path));
	VFrame default_data;
	default_data.read_png(get_image_data(overlay_path), BC_Resources::dpi);

	BC_ThemeSet *result = new BC_ThemeSet(3, 1, title ? title : (char*)"");
	if(title) set_image_set(title, result);

	result->data[0] = new_image(up_path);
	result->data[1] = new_image(hi_path);
	result->data[2] = new_image(dn_path);
	for(int i = 0; i < 3; i++)
	{
		overlay(result->data[i], &default_data, -1, -1, (i == 2));
	}
	return result->data;
}


VFrame** BC_Theme::new_button(const char *overlay_path, 
	VFrame *up,
	VFrame *hi,
	VFrame *dn,
	const char *title)
{
//	VFrame default_data(get_image_data(overlay_path));
	VFrame default_data;
	default_data.read_png(get_image_data(overlay_path), BC_Resources::dpi);
	BC_ThemeSet *result = new BC_ThemeSet(3, 0, title ? title : (char*)"");
	if(title) set_image_set(title, result);

	result->data[0] = new VFrame(*up);
	result->data[1] = new VFrame(*hi);
	result->data[2] = new VFrame(*dn);
	for(int i = 0; i < 3; i++)
		overlay(result->data[i], &default_data, -1, -1, (i == 2));
	return result->data;
}


VFrame** BC_Theme::new_toggle(const char *overlay_path, 
	const char *up_path,
	const char *hi_path,
	const char *checked_path,
	const char *dn_path,
	const char *checkedhi_path,
	const char *title)
{
//	VFrame default_data(get_image_data(overlay_path));
	VFrame default_data;
	default_data.read_png(get_image_data(overlay_path), BC_Resources::dpi);
	BC_ThemeSet *result = new BC_ThemeSet(5, 1, title ? title : (char*)"");
	if(title) set_image_set(title, result);

	result->data[0] = new_image(up_path);
	result->data[1] = new_image(hi_path);
	result->data[2] = new_image(checked_path);
	result->data[3] = new_image(dn_path);
	result->data[4] = new_image(checkedhi_path);
	for(int i = 0; i < 5; i++)
		overlay(result->data[i], &default_data, -1, -1, (i == 3));
	return result->data;
}

VFrame** BC_Theme::new_toggle(const char *overlay_path, 
	VFrame *up,
	VFrame *hi,
	VFrame *checked,
	VFrame *dn,
	VFrame *checkedhi,
	const char *title)
{
//	VFrame default_data(get_image_data(overlay_path));
	VFrame default_data;
	default_data.read_png(get_image_data(overlay_path), BC_Resources::dpi);

	BC_ThemeSet *result = new BC_ThemeSet(5, 0, title ? title : (char*)"");
	if(title) set_image_set(title, result);

	result->data[0] = new VFrame(*up);
	result->data[1] = new VFrame(*hi);
	result->data[2] = new VFrame(*checked);
	result->data[3] = new VFrame(*dn);
	result->data[4] = new VFrame(*checkedhi);
	for(int i = 0; i < 5; i++)
		overlay(result->data[i], &default_data, -1, -1, (i == 3));
	return result->data;
}

void BC_Theme::overlay(VFrame *dst, VFrame *src, int in_x1, int in_x2, int shift)
{
	int w;
	int h;
	unsigned char **in_rows;
	unsigned char **out_rows;

	if(in_x1 < 0)
	{
		w = MIN(src->get_w(), dst->get_w());
		h = MIN(dst->get_h(), src->get_h());
		in_x1 = 0;
		in_x2 = w;
	}
	else
	{
		w = in_x2 - in_x1;
		h = MIN(dst->get_h(), src->get_h());
	}
	in_rows = src->get_rows();
	out_rows = dst->get_rows();

	switch(src->get_color_model())
	{
		case BC_RGBA8888:
			switch(dst->get_color_model())
			{
				case BC_RGBA8888:
					for(int i = shift; i < h; i++)
					{
						unsigned char *in_row = 0;
						unsigned char *out_row;

						if(!shift)
						{
							in_row = in_rows[i] + in_x1 * 4;
							out_row = out_rows[i];
						}
						else
						{
							in_row = in_rows[i - 1] + in_x1 * 4;
							out_row = out_rows[i] + 4;
						}

						for(int j = shift; j < w; j++)
						{
							int opacity = in_row[3];
							int transparency = out_row[3] * (0xff - opacity) / 0xff;

							out_row[0] = (in_row[0] * opacity + out_row[0] * transparency) / 0xff;
							out_row[1] = (in_row[1] * opacity + out_row[1] * transparency) / 0xff;
							out_row[2] = (in_row[2] * opacity + out_row[2] * transparency) / 0xff;
							out_row[3] = MAX(in_row[3], out_row[3]);
							out_row += 4;
							in_row += 4;
						}
					}
					break;

				case BC_RGB888:
					for(int i = shift; i < h; i++)
					{
						unsigned char *in_row;
						unsigned char *out_row = out_rows[i];

						if(!shift)
						{
							in_row = in_rows[i] + in_x1 * 3;
							out_row = out_rows[i];
						}
						else
						{
							in_row = in_rows[i - 1] + in_x1 * 3;
							out_row = out_rows[i] + 3;
						}

						for(int j = shift; j < w; j++)
						{
							int opacity = in_row[3];
							int transparency = 0xff - opacity;
							out_row[0] = (in_row[0] * opacity + out_row[0] * transparency) / 0xff;
							out_row[1] = (in_row[1] * opacity + out_row[1] * transparency) / 0xff;
							out_row[2] = (in_row[2] * opacity + out_row[2] * transparency) / 0xff;
							out_row += 3;
							in_row += 4;
						}
					}
					break;
			}
			break;
	}
}

void BC_Theme::swap_color(VFrame *dst, VFrame *src, uint32_t src_color, uint32_t dst_color)
{
	int w;
	int h;
	unsigned char **in_rows;
	unsigned char **out_rows;
	w = MIN(src->get_w(), dst->get_w());
	h = MIN(dst->get_h(), src->get_h());
	in_rows = src->get_rows();
	out_rows = dst->get_rows();
    uint8_t in_r = (src_color >> 16) & 0xff;
    uint8_t in_g = (src_color >> 8) & 0xff;
    uint8_t in_b = src_color & 0xff;
    uint8_t out_r = (dst_color >> 16) & 0xff;
    uint8_t out_g = (dst_color >> 8) & 0xff;
    uint8_t out_b = dst_color & 0xff;

    switch(src->get_color_model())
    {
        case BC_RGBA8888:
            for(int i = 0; i < h; i++)
            {
                uint8_t *in_row = in_rows[i];
                uint8_t *out_row = out_rows[i];
                for(int j = 0; j < w; j++)
                {
                    if(in_row[0] == in_r &&
                        in_row[1] == in_g &&
                        in_row[2] == in_b)
                    {
                        out_row[0] = out_r;
                        out_row[1] = out_g;
                        out_row[2] = out_b;
                    }
                    else
                    {
                        out_row[0] = in_row[0];
                        out_row[1] = in_row[1];
                        out_row[2] = in_row[2];
                    }
                    out_row[3] = in_row[3];

                    in_row += 4;
                    out_row += 4;
                }
            }
            break;

        default:
            printf("BC_Theme::swap_color %d: unsupported colormodel %d", 
                __LINE__, 
                src->get_color_model());
            break;
    }
}




void BC_Theme::set_data(unsigned char *ptr)
{
// pointers to statically linked objects
	char *contents_ptr = (char*)(ptr + sizeof(int));
	int contents_size = *(int*)ptr - sizeof(int);
	uint8_t *data_ptr = (uint8_t*)contents_ptr + contents_size;

	for(int i = 0; i < contents_size; )
	{
// load the key
        char *key = contents_ptr + i;
		while(contents_ptr[i] && i < contents_size)
			i++;

// load the image data
		if(i < contents_size)
		{
			i++;
            int image_offset = *(unsigned int*)(contents_ptr + i);
			i += 4;
            images[key] = (uint8_t*)data_ptr + image_offset;
//printf("BC_Theme::set_data %d key=%s\n", __LINE__, key);
		}
		else
		{
// point to 1st image data if not found
			images[key] = (uint8_t*)data_ptr;
			break;
		}
	}
}

unsigned char* BC_Theme::get_image_data(const char *title)
{
	if(images.begin() == images.end())
	{
		fprintf(stderr, "BC_Theme::get_image_data %d: no data set\n", __LINE__);
		return 0;
	}

// Image is the same as the last one
	if(last_image && !strcasecmp(last_image, title))
	{
		return last_pointer;
	}
	else
// Search for image anew.
	{
        auto i = images.find(title);
        if(i != images.end())
        {
            last_image = i->first.c_str();
            last_pointer = i->second;
            return last_pointer;
        }
	}

	fprintf(stderr, _("BC_Theme::get_image_data %d: %s not found.\n"), 
        __LINE__,
        title);
	return 0;
}












BC_ThemeSet::BC_ThemeSet(int total, int is_reference, const char *title)
{
	this->total = total;
	this->title = new char[strlen(title) + 1];
	strcpy(this->title, title);
	this->is_reference = is_reference;
	data = new VFrame*[total];
}

BC_ThemeSet::~BC_ThemeSet()
{
	if(data) 
	{
		if(!is_reference)
		{
			for(int i = 0; i < total; i++)
				delete data[i];
		}

		delete [] data;
	}

	delete [] title;
}





