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

#include "bccapture.h"
#include "file.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "recordconfig.h"
#include "textengine.h"
#include "vdevicescreencap.h"
#include "vframe.h"
#include "videodevice.h"

VDeviceScreencap::VDeviceScreencap(VideoDevice *device)
 : VDeviceBase(device)
{
	reset_parameters();
}


VDeviceScreencap::~VDeviceScreencap()
{
	close_all();
}

int VDeviceScreencap::reset_parameters()
{
	capture_bitmap = 0;
    text_engine = 0;
    mouse_text_engine = 0;
    mouse_mask = 0;
    mouse_outline = 0;
	for(int i = 0; i < SCREENCAP_BORDERS; i++)
	{
		screencap_border[i] = 0;
	}
    return 0;
}

int VDeviceScreencap::open_input()
{
//printf("VDeviceX11::open_input 1\n");
	capture_bitmap = new BC_Capture(device->in_config->w, 
		device->in_config->h,
		device->in_config->screencapture_display);
//printf("VDeviceX11::open_input %d %p\n", __LINE__, device->mwindow);

	if(device->mwindow)
	{
// create overlay
		device->mwindow->gui->lock_window("VDeviceScreencap::close_all");

		screencap_border[0] = new BC_Popup(device->mwindow->gui, 
				device->input_x - SCREENCAP_PIXELS,
				device->input_y - SCREENCAP_PIXELS,
				device->in_config->w + SCREENCAP_PIXELS * 2,
				SCREENCAP_PIXELS,
				SCREENCAP_COLOR);
		screencap_border[1] = new BC_Popup(device->mwindow->gui, 
				device->input_x - SCREENCAP_PIXELS,
				device->input_y,
				SCREENCAP_PIXELS,
				device->in_config->h,
				SCREENCAP_COLOR);
		screencap_border[2] = new BC_Popup(device->mwindow->gui, 
				device->input_x - SCREENCAP_PIXELS,
				device->input_y + device->in_config->h,
				device->in_config->w + SCREENCAP_PIXELS * 2,
				SCREENCAP_PIXELS,
				SCREENCAP_COLOR);
		screencap_border[3] = new BC_Popup(device->mwindow->gui, 
				device->input_x + device->in_config->w,
				device->input_y,
				SCREENCAP_PIXELS,
				device->in_config->h,
				SCREENCAP_COLOR);
		device->mwindow->gui->unlock_window();
	}

	return 0;
}

int VDeviceScreencap::get_best_colormodel(Asset *asset)
{
	return File::get_best_colormodel(asset, device->in_config, 0 /* SCREENCAPTURE */ );

//	return BC_RGB888;
}

int VDeviceScreencap::close_all()
{
    delete capture_bitmap;
    delete text_engine;
    delete mouse_text_engine;
    delete mouse_mask;
    delete mouse_outline;

	if(device->mwindow)
	{
		device->mwindow->gui->lock_window("VDeviceScreencap::close_all");
		for(int i = 0; i < SCREENCAP_BORDERS; i++)
		{
			delete screencap_border[i];
			screencap_border[i] = 0;
		}
		device->mwindow->gui->unlock_window();
	}
	reset_parameters();
	return 0;
}

int VDeviceScreencap::read_buffer(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();
    int bitmap_color_model = capture_bitmap->bitmap_color_model;
    int pixel_size = cmodel_calculate_pixelsize(bitmap_color_model);

//printf("VDeviceX11::read_buffer %d colormodel=%d\n", __LINE__, frame->get_color_model());
	if(device->mwindow)
	{
		device->mwindow->gui->lock_window("VDeviceX11::close_all");

		screencap_border[0]->reposition_window(device->input_x - SCREENCAP_PIXELS,
				device->input_y - SCREENCAP_PIXELS);
		screencap_border[1]->reposition_window(device->input_x - SCREENCAP_PIXELS,
				device->input_y);
		screencap_border[2]->reposition_window(device->input_x - SCREENCAP_PIXELS,
				device->input_y + device->in_config->h);
		screencap_border[3]->reposition_window(device->input_x + device->in_config->w,
				device->input_y);
		device->mwindow->gui->flush();
		device->mwindow->gui->unlock_window();
	}

	capture_bitmap->capture_frame(device->input_x, 
		device->input_y,
		device->cursor_scale,
        device->keypress_size);

    const int outline_sizes[] = 
    {
        1, 1,
        18, 2,
        48, 4
    };

    int outline_size = 1;
    for(int i = sizeof(outline_sizes) / sizeof(int) - 2; i >= 0; i -= 2)
    {
        if(outline_sizes[i] <= device->keypress_size) 
        {
            outline_size = outline_sizes[i + 1];
            break;
        }
    }

    char font_path[BCTEXTLEN];
    sprintf(font_path, 
        "%s/%s", 
        MWindow::preferences->plugin_dir, 
        "fonts/arial.ttf");

// have to draw keypresses here to avoid a freetype dependency in guicast
    if(device->keypress_size && capture_bitmap->key_text[0])
    {
// update the keypress bitmap
//printf("VDeviceScreencap::read_buffer %d %s\n", __LINE__, capture_bitmap->key_text);
        if(!text_engine ||
            (text_engine->text.compare(capture_bitmap->key_text) ||
            device->keypress_size != text_engine->size))
        {
            if(text_engine && 
                device->keypress_size != text_engine->size)
            {
                delete text_engine;
                text_engine = 0;
            }

            if(!text_engine) text_engine = new TextEngine;
            text_engine->load(font_path, device->keypress_size);
            text_engine->set_text(capture_bitmap->key_text);
            text_engine->set_outline(outline_size);
            text_engine->draw();
// printf("VDeviceScreencap::read_buffer %d %s w=%d h=%d\n", 
// __LINE__, 
// capture_bitmap->key_text,
// text_engine->text_w,
// text_engine->text_h);
        }

// Get destination coordinates of the text
        int bottom = h - h / 10;
        int top = bottom - text_engine->text_h;
        int left = w / 2 - text_engine->text_w / 2;
        draw_mask(text_engine->text_mask,
            text_engine->outline_mask,
            left,
            top);
    }


    if(device->keypress_size && capture_bitmap->button.id >= 0)
    {
        int mouse_x = capture_bitmap->cursor_x;
        int mouse_y = capture_bitmap->cursor_y + capture_bitmap->cursor_h;
        int mouse_w = device->keypress_size + outline_size * 2;
        int mouse_h = mouse_w;
        if(mouse_y + mouse_h > h) mouse_y = capture_bitmap->cursor_y - mouse_h;
        if(mouse_x + mouse_w > w) mouse_x = w - mouse_w;
        if(mouse_mask && mouse_mask->get_h() != mouse_w + mouse_h)
        {
            delete mouse_mask;
            delete mouse_outline;
            mouse_mask = 0;
            mouse_outline = 0;
        }

        if(!mouse_mask)
        {
            mouse_mask = new VFrame;
            mouse_mask->set_use_shm(0);
            mouse_mask->reallocate(0,
			    -1,
			    0,
			    0,
			    0,
			    mouse_w,
			    mouse_h,
			    BC_A8,
			    -1);
            mouse_outline = new VFrame;
            mouse_outline->set_use_shm(0);
            mouse_outline->reallocate(0,
			    -1,
			    0,
			    0,
			    0,
			    mouse_w,
			    mouse_h,
			    BC_A8,
			    -1);
        }

        int line_thickness = 1;
        if(device->keypress_size > 10)
            line_thickness = device->keypress_size / 10;
// printf("VDeviceScreencap::read_buffer %d %d %d\n", 
// __LINE__,
// mouse_mask->get_w(),
// mouse_mask->get_h());
        mouse_mask->clear_frame();
        mouse_outline->clear_frame();
        draw_mouse(mouse_mask, line_thickness, outline_size);
        TextEngine::do_outline(mouse_mask, mouse_outline, outline_size);

        char mouse_text[BCTEXTLEN];
        mouse_text[0] = 0;
//printf("VDeviceScreencap::read_buffer %d %d\n", __LINE__, capture_bitmap->button.times);
        if(capture_bitmap->button.times > 1)
            sprintf(mouse_text, "x%d", capture_bitmap->button.times);

        if(mouse_text[0] != 0)
        {
            if(!mouse_text_engine || 
                (mouse_text_engine->text.compare(mouse_text) ||
                mouse_text_engine->size != device->keypress_size))
            {
                if(mouse_text_engine &&
                    device->keypress_size != mouse_text_engine->size)
                {
                    delete mouse_text_engine;
                    mouse_text_engine = 0;
                }

                if(!mouse_text_engine) mouse_text_engine = new TextEngine;
                mouse_text_engine->load(font_path, device->keypress_size);
                mouse_text_engine->set_text(mouse_text);
                mouse_text_engine->set_outline(outline_size);
                mouse_text_engine->draw();
            }

//printf("VDeviceScreencap::read_buffer %d %s\n", __LINE__, mouse_text);
            if(mouse_x + mouse_w + mouse_mask->get_w() > w)
                mouse_x = w - mouse_w - mouse_mask->get_w();
            draw_mask(mouse_text_engine->text_mask,
                mouse_text_engine->outline_mask,
                mouse_x + mouse_mask->get_w(),
                mouse_y);
        }

        draw_mask(mouse_mask, 
            mouse_outline,
            mouse_x, 
            mouse_y);
    }




	cmodel_transfer(frame->get_rows(), 
		capture_bitmap->row_data,
		frame->get_y(),
		frame->get_u(),
		frame->get_v(),
        0, // out_a_plane
		0,
		0,
		0,
        0, // in_a_plane
		0, // in_x
		0, // in_y
		w, 
		h,
		0, 
		0, 
		frame->get_w(), 
		frame->get_h(),
		capture_bitmap->bitmap_color_model, 
		frame->get_color_model(),
		0,
		frame->get_w(),
		w);


	return 0;
}

// convert a mask value into a blended color
void VDeviceScreencap::draw_pixel(uint8_t *dst, int color_model, int alpha, int r, int g, int b)
{
    int inverse = 0xff - alpha;
    switch(color_model)
    {
        case BC_BGR8888:
            dst[0] = (r * alpha + inverse * dst[0]) / 0xff;
            dst[1] = (g * alpha + inverse * dst[1]) / 0xff;
            dst[2] = (b * alpha + inverse * dst[2]) / 0xff;
            break;

/*        default: */
/*            printf("VDeviceScreencap::read_buffer %d unsupported color model %d\n", */
/*                __LINE__, */
/*                bitmap_color_model); */
/*            break; */
    }
}

// draw on the mask
void VDeviceScreencap::fill_rect(VFrame *mask, int x, int y, int w, int h)
{
    uint8_t **rows = mask->get_rows();
    int mask_w = mask->get_w();
    int mask_h = mask->get_h();
    int x1 = x;
    int x2 = x + w;
    if(x1 < 0) x1 = 0;
    if(x2 > mask_w) x2 = mask_w;
    int w1 = x2 - x1;
    if(x2 > x1)
    {
        for(int i = 0; i < h; i++)
        {
            int y1 = i + y;
            if(y1 >= 0 && y1 < mask_h)
            {
                uint8_t *row = rows[y1] + x1;
                memset(row, 0xff, w1);
            }
        }
    }
}

void VDeviceScreencap::fill_tri(VFrame *mask, int x, int y, int w, int h, int up)
{
    uint8_t **rows = mask->get_rows();
    int mask_w = mask->get_w();
    int mask_h = mask->get_h();
    int center_x = x + w / 2;
    for(int i = 0; i < h; i++)
    {
        int y1 = i + y;
        if(y1 >= 0 && y1 < mask_h)
        {
            int w1;
            if(up) 
                w1 = w * (i + 1) / h;
            else
                w1 = w * (h - i) / h;
            int x1 = center_x - w1 / 2;
            int x2 = x1 + w1;
            if(x1 < 0) x1 = 0;
            if(x2 > mask_w) x2 = mask_w;
            if(x2 > x1)
            {
                w1 = x2 - x1;
                uint8_t *row = rows[y1] + x1;
                memset(row, 0xff, w1);
            }
        }
    }
}

void VDeviceScreencap::draw_mouse(VFrame *mask, 
    int line_thickness,
    int outline_size)
{
    int w = mask->get_w() - outline_size * 2;
    int h = mask->get_h() - outline_size * 2;
    int y = outline_size;
    int button_h = h / 3;
    int button1 = outline_size + w / 3 - line_thickness / 2;
    int button2 = outline_size + w * 2 / 3 - line_thickness / 2;

    fill_rect(mask, outline_size, y, w, line_thickness);
    fill_rect(mask, outline_size, y, line_thickness, h);
    fill_rect(mask, 
        outline_size + w - line_thickness, 
        y, 
        line_thickness, 
        h);
    fill_rect(mask, 
        outline_size, 
        outline_size + h - line_thickness, 
        w, 
        line_thickness);
    if(capture_bitmap->button.id == LEFT_BUTTON ||
        capture_bitmap->button.id == MIDDLE_BUTTON ||
        capture_bitmap->button.id == RIGHT_BUTTON)
    {
        fill_rect(mask, 
            button1, 
            y, 
            line_thickness, 
            button_h);
        fill_rect(mask, 
            button2, 
            y, 
            line_thickness, 
            button_h);
    }

    switch(capture_bitmap->button.id)
    {
        case LEFT_BUTTON:
            fill_rect(mask, outline_size, y, button1 - outline_size, button_h);
            break;
        case MIDDLE_BUTTON:
            fill_rect(mask, button1, y, button2 - button1, button_h);
            break;
        case RIGHT_BUTTON:
            fill_rect(mask, button2, y, w - button2, button_h);
            break;
        case WHEEL_UP:
            fill_tri(mask, 
                outline_size + w / 3, 
                y + line_thickness + outline_size, 
                w / 3, 
                h / 2 - line_thickness - outline_size, 
                1);
            fill_rect(mask, \
                outline_size + w / 2 - line_thickness / 2, 
                y + h / 2, 
                line_thickness, 
                h / 2 - line_thickness - outline_size);
            break;
        case WHEEL_DOWN:
            fill_tri(mask, 
                outline_size + w / 3, 
                y + h / 2, 
                w / 3, 
                h / 2 - line_thickness - outline_size, 
                0);
            fill_rect(mask, 
                outline_size + w / 2 - line_thickness / 2, 
                y + line_thickness + outline_size, 
                line_thickness, 
                h / 2 - line_thickness - outline_size);
            break;
    }
}

void VDeviceScreencap::draw_mask(VFrame *text,
    VFrame *outline,
    int x, 
    int y)
{
    int w = capture_bitmap->w;
    int h = capture_bitmap->h;
    int text_w = text->get_w();
    int text_h = text->get_h();
    int bitmap_color_model = capture_bitmap->bitmap_color_model;
    int pixel_size = cmodel_calculate_pixelsize(bitmap_color_model);

//printf("VDeviceScreencap::draw_mask %d\n", __LINE__);
// fuse the outline & foreground layers
    for(int i = 0; i < text_h; i++)
    {
        int y1 = i + y;
        if(y1 >= 0 && y1 < h)
        {
            uint8_t *text_row = text->get_rows()[i];
            uint8_t *outline_row = outline->get_rows()[i];
//printf("VDeviceScreencap::read_buffer %d i=%d top=%d pixel_size=%d left=%d\n", 
//__LINE__, (int)i, (int)top, (int)pixel_size, (int)left);

            uint8_t *dst_row = capture_bitmap->row_data[y1] + pixel_size * x;

            for(int j = 0; j < text_w; j++)
            {
                int x1 = j + x;
                if(x1 >= 0 && x1 < w)
                {
                    if(*text_row > 0)
                    {
                        draw_pixel(dst_row, bitmap_color_model, 0xff, 0xff, 0xff, 0xff);
                    }
                    else
                    if(*outline_row > 0)
                    {
                        draw_pixel(dst_row, bitmap_color_model, *outline_row, 0x00, 0x00, 0x00);
                    }
                }
                dst_row += pixel_size;
                text_row++;
                outline_row++;
            }
        }
    }
}



