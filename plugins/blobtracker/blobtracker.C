/*
 * CINELERRA
 * Copyright (C) 2026 Adam Williams <broadcast at earthling dot net>
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

// Track blob created by a color key plugin

#include "blobtracker.h"
#include "blobwindow.h"
#include "cicolors.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "overlayframe.h"

#include <vector>

REGISTER_PLUGIN(BlobTracker)

BlobConfig::BlobConfig()
{
    targ_x = 50;
    targ_y = 50;
    min_size = 7;
    window_size = 20;
    draw_guides = 0;
    mode = BRIGHTEST_BLOB;
//    mode = LARGEST_BLOB;
    ref_layer = TOP;
    targ_layer = BOTTOM;
    stabilize = 1;
}

int BlobConfig::equivalent(BlobConfig &that)
{
    return mode == that.mode &&
        ref_layer == that.ref_layer &&
        targ_layer == that.targ_layer &&
        draw_guides == that.draw_guides &&
        min_size == that.min_size &&
        stabilize == that.stabilize;
}

void BlobConfig::copy_from(BlobConfig &that)
{
    mode = that.mode;
    ref_layer = that.ref_layer;
    targ_layer = that.targ_layer;
    draw_guides = that.draw_guides;
    min_size = that.min_size;
    stabilize = that.stabilize;
}

void BlobConfig::interpolate(BlobConfig &prev, 
	BlobConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
    mode = prev.mode;
    ref_layer = prev.ref_layer;
    targ_layer = prev.targ_layer;
    draw_guides = prev.draw_guides;
    min_size = prev.min_size;
    stabilize = prev.stabilize;
}

void BlobConfig::boundaries()
{
    if(
//      mode != LEFTMOST_BLOB &&
        mode != LARGEST_BLOB &&
//        mode != BRIGHTEST_WINDOW &&
        mode != BRIGHTEST_BLOB)
        mode = BRIGHTEST_BLOB;
    if(ref_layer != TOP && ref_layer != BOTTOM) ref_layer = TOP;
    if(targ_layer != TOP && targ_layer != BOTTOM) targ_layer = BOTTOM;
    CLAMP(min_size, 1, MAX_BLOB_SIZE);
}







BlobTracker::BlobTracker(PluginServer *server)
 : PluginVClient(server)
{
    mask = 0;
    shift_x = 0;
    shift_y = 0;
    translator = 0;
}

BlobTracker::~BlobTracker()
{
    delete mask;
    delete translator;
}

const char* BlobTracker::plugin_title() { return N_("Blob Tracker"); }
int BlobTracker::is_realtime() { return 1; }
int BlobTracker::is_multichannel() { return 1; }

NEW_WINDOW_MACRO(BlobTracker, BlobWindow)

LOAD_CONFIGURATION_MACRO(BlobTracker, BlobConfig)

int BlobTracker::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
    int ref_layer = config.ref_layer ? PluginClient::total_in_buffers - 1 : 0;
    int targ_layer = config.targ_layer ? PluginClient::total_in_buffers - 1 : 0;
    VFrame *ref_frame = frame[ref_layer];
    VFrame *targ_frame = frame[targ_layer];
	int h = ref_frame->get_h();
	int w = ref_frame->get_w();
	int color_model = ref_frame->get_color_model();
	int need_reconfigure = load_configuration();
    int targ_x = (int)(w * config.targ_x / 100);
    int targ_y = (int)(h * config.targ_y / 100);
    int min_x = 0x7fffffff;
    int max_x = -1;
    int min_y = 0x7fffffff;
    int max_y = -1;
// blob tracker result
    std::vector<int> best_x;
    std::vector<int> best_y;

    VFrame *tmp = new_temp(w, h, color_model);
// targ layer is always modified
    read_frame(tmp, 
		targ_layer, 
		start_position, 
		frame_rate,
		0);
// ref frame is not modified
    if(ref_layer != targ_layer)
    {
        read_frame(ref_frame, 
		    ref_layer, 
		    start_position, 
		    frame_rate,
		    0);
    }
    else
    {
        ref_frame = tmp;
    }

//     if(config.mode == LEFTMOST_BLOB ||
//         config.mode == LARGEST_BLOB ||
//         config.mode == BRIGHTEST_BLOB)
//     {
    if(!mask) 
    {
        mask = new VFrame;
        mask->set_use_shm(0);
        mask->reallocate(0, // data
	  	    -1, // shmid
            0, // Y
            0, // U
            0, // V
	  	    w, // w
		    h, // h
            BC_A8, // color_model
            -1); // bytes_per_line
    }
    mask->clear_frame();
// best hit
// left center pixel
    int blob_x = 0x7fffffff;
    int blob_y = -1;
    int blob_size = 0;
    float blob_brightness = 0;
    best_x.clear();
    best_y.clear();


// pixels defining the current blob
    std::vector<int> current_x;
    std::vector<int> current_y;
    int done = 0;

#define TEST_KEY(x, y, components, threshold) \
    if(x >= 0 && x < w && y >= 0 && y < h) \
    { \
        if(mask_rows[y][x] == 0 && \
            ((components == 4 && src_rows[y][(x) * 4 + 3] < threshold) || \
            (components == 3 && src_rows[y][(x) * 3 + 0] < threshold))) \
        { \
            done = 0; \
            mask_rows[y][x] = 1; \
            current_x.push_back(x); \
            current_y.push_back(y); \
        } \
    }


#define PROCESS_BLOB(type, components, threshold) \
{ \
    type **src_rows = (type**)ref_frame->get_rows(); \
    uint8_t **mask_rows = (uint8_t**)mask->get_rows(); \
    for(int i = 0; i < h; i++) \
    { \
        type *src_row = (type*)src_rows[i]; \
        uint8_t *mask_row = (uint8_t*)mask_rows[i]; \
        for(int j = 0; j < w; j++) \
        { \
            done = 1; \
            TEST_KEY(j, i, components, threshold) \
/* new blob */ \
            while(!done) \
            { \
                done = 1; \
                for(int k = 0; k < current_x.size(); k++) \
                { \
                    int x = current_x[k]; \
                    int y = current_y[k]; \
                    TEST_KEY(x - 1, y, components, threshold) \
                    TEST_KEY(x + 1, y, components, threshold) \
                    TEST_KEY(x, y - 1, components, threshold) \
                    TEST_KEY(x, y + 1, components, threshold) \
                } \
            } \
            if(current_x.size()) \
            { \
/* printf("BlobTracker::process_buffer %d: size=%d\n", __LINE__, (int)current_x.size()); */ \
                if(test_blob(&current_x, \
                    &current_y, \
                    &blob_x, \
                    &blob_y, \
                    &blob_size, \
                    tmp, \
                    &blob_brightness) && \
                    config.draw_guides) \
                { \
                    best_x = current_x; \
                    best_y = current_y; \
                } \
                current_x.clear(); \
                current_y.clear(); \
            } \
        } \
    } \
}

    switch(color_model)
    {
        case BC_RGB888:
        case BC_YUV888:
            PROCESS_BLOB(uint8_t, 3, 0x80)
            break;
        case BC_RGBA8888:
        case BC_YUVA8888:
            PROCESS_BLOB(uint8_t, 4, 0x80)
            break;
        case BC_RGB_FLOAT:
            PROCESS_BLOB(float, 3, 0.5)
            break;
        case BC_RGBA_FLOAT:
            PROCESS_BLOB(float, 4, 0.5)
            break;
    }
//printf("BlobTracker::process_buffer %d: %d %d %d\n", 
//__LINE__, blob_x, blob_y, blob_size);

// update the shift
    if(blob_y >= 0)
    {
        shift_x = targ_x - blob_x;
        shift_y = targ_y - blob_y;
    }
//    }
//     else
//     if(config.mode == BRIGHTEST_WINDOW)
//     {
//         int window_w = MIN(config.window_size, w);
//         int window_h = MIN(config.window_size, h);
// 
// #define PROCESS_BRIGHTEST(type, accum_type, components, is_yuv) \
// { \
//     accum_type greatest = 0; \
//     type kernal[window_w * window_h * 3]; \
//     type **src_rows = (type**)tmp->get_rows(); \
//     for(int j = 0; j < w - window_w; j++) \
//     { \
//         accum_type accum = 0; \
// /* initialize the kernal on the top row */ \
//         for(int i = 0; i < window_h; i++) \
//         { \
//             type *src_row = (type*)src_rows[i] + j * components; \
//             type *dst_row = kernal + i * window_w * 3; \
//             for(int j2 = 0; j2 < window_w; j2++) \
//             { \
//                 dst_row[0] = src_row[0]; \
//                 dst_row[1] = src_row[1]; \
//                 dst_row[2] = src_row[2]; \
//                 if(is_yuv) \
//                     accum += dst_row[0]; \
//                 else \
//                     accum += dst_row[0] + dst_row[1] + dst_row[2]; \
//                 src_row += components; \
//                 dst_row += 3; \
//             } \
//         } \
// /* advance the kernal down the column */ \
//         for(int i = 0; i < h - window_h; i++) \
//         { \
//             if(i > 0) \
//             { \
//                 int kernal_row = (i - 1) % window_h; \
//                 type *dst_row = &kernal[kernal_row * window_w * 3]; \
//                 type *src_row = (type*)src_rows[i + window_h - 1] + j * components; \
//                 for(int j2 = 0; j2 < window_w; j2++) \
//                 { \
// /* subtract top row of kernal */ \
//                     if(is_yuv) \
//                         accum -= dst_row[0]; \
//                     else \
//                         accum -= dst_row[0] + dst_row[1] + dst_row[2]; \
// /* add bottom row of kernal */ \
//                     dst_row[0] = src_row[0]; \
//                     dst_row[1] = src_row[1]; \
//                     dst_row[2] = src_row[2]; \
//                     if(is_yuv) \
//                         accum += dst_row[0]; \
//                     else \
//                         accum += dst_row[0] + dst_row[1] + dst_row[2]; \
//                     dst_row += 3; \
//                     src_row += components; \
//                 } \
//             } \
//             if(accum > greatest) \
//             { \
//                 min_x = j; \
//                 max_x = j + window_w; \
//                 min_y = i; \
//                 max_y = i + window_h; \
//                 greatest = accum; \
//             } \
//             else \
//             if(accum == greatest) \
//             { \
//                 min_x = MIN(j, min_x); \
//                 min_y = MIN(i, min_y); \
//                 max_x = MAX(j + window_w, max_x); \
//                 max_y = MAX(i + window_h, max_y); \
//             } \
//         } \
//     } \
// }
// 
// 
//         switch(color_model)
//         {
//             case BC_RGB888:
//                 PROCESS_BRIGHTEST(uint8_t, uint32_t, 3, 0)
//                 break;
//             case BC_YUV888:
//                 PROCESS_BRIGHTEST(uint8_t, uint32_t, 3, 1)
//                 break;
//         }
// 
// // update the shift
//         if(max_x >= 0)
//         {
// // center of x range
// //            shift_x = targ_x - (min_x + max_x) / 2;
// // left edge
//             shift_x = targ_x - min_x;
// // center of y range
//             shift_y = targ_y - (min_y + max_y) / 2;
// // top edge
// //            shift_y = targ_y - min_y;
// // bottom edge
// //            shift_y = targ_y - max_y;
// //printf("BlobTracker::process_buffer %d: x=%d %d y=%d %d\n", 
// //__LINE__, min_x, max_x, min_y, max_y);
//         }
//     }


    if(!config.stabilize)
    {
        shift_x = 0;
        shift_y = 0;
    }

// shift the targ frame
	if(!translator) 
		translator = new OverlayFrame(PluginClient::get_project_smp() + 1);
//printf("BlobTracker::process_buffer %d: total_in_buffers=%d ref_layer=%d targ_layer=%d\n", 
    targ_frame->clear_frame();
    translator->overlay(targ_frame,
		tmp,
		0,
		0,
		w,
		h,
		shift_x,
		shift_y,
		(float)w + shift_x,
		(float)h + shift_y,
		1,
		TRANSFER_REPLACE,
		NEAREST_NEIGHBOR);

// diagnostics
    send_render_gui(&blob_size, sizeof(blob_size));

    if(config.draw_guides)
    {
// printf("BlobTracker::process_buffer %d %p %d %d %d %d\n", 
// __LINE__, 
// frame[targ_layer],
// min_x + shift_x, 
// min_y + shift_y,
// max_x + shift_x,
// max_y + shift_y);
//         if(config.mode == BRIGHTEST_WINDOW)
//             frame[targ_layer]->draw_rect(min_x + shift_x, 
//                 min_y + shift_y,
//                 max_x + shift_x,
//                 max_y + shift_y);
//         else
//         {
        uint8_t **mask_rows = (uint8_t**)mask->get_rows();
        for(int i = 0; i < best_x.size(); i++)
        {
            mask_rows[best_y[i]][best_x[i]] = 2;
        }
        for(int i = 0; i < best_x.size(); i++)
        {
// mark border pixels
            int x = best_x[i];
            int y = best_y[i];
            if(x == 0 || 
                x == w - 1 || 
                y == 0 || 
                y == h - 1 ||
                mask_rows[y][x - 1] != 2 ||
                mask_rows[y][x + 1] != 2 ||
                mask_rows[y - 1][x] != 2 ||
                mask_rows[y + 1][x] != 2)
                frame[targ_layer]->draw_pixel(x + shift_x, y + shift_y);
        }
//        }
// printf("BlobTracker::process_buffer %d\n", 
// __LINE__);
    }

	return 0;
}

int BlobTracker::test_blob(std::vector<int> *current_x, 
    std::vector<int> *current_y, 
    int *blob_x, 
    int *blob_y,
    int *blob_size,
    VFrame *targ_frame,
    float *brightest)
{
	int h = targ_frame->get_h();
	int w = targ_frame->get_w();
	int color_model = targ_frame->get_color_model();
    int min_x = 0x7fffffff;
    int min_y = 0x7fffffff;
    int max_y = -1;
    float brightness = 0;
    int n = current_x->size();

// too small
    if(n < config.min_size * config.min_size)
    {
// update biggest detected for diagnostics
        if(n > *blob_size) *blob_size = n;
        return 0;
    }

    int result = 0;
// test size of blob
    if(config.mode == LARGEST_BLOB)
    {
        if(n > *blob_size)
        {
            result = 1;
        }
    }
    else
    if(config.mode == BRIGHTEST_BLOB)
    {
#define TEST_BRIGHTEST(type, accum_type, components, is_yuv) \
{ \
    type **src_rows = (type**)targ_frame->get_rows(); \
    accum_type accum = 0; \
    for(int i = 0; i < n; i++) \
    { \
        type *src_pixel = src_rows[(*current_y)[i]] + (*current_x)[i] * components; \
        if(is_yuv) \
            accum = MAX(src_pixel[0], accum); \
        else \
        { \
            accum_type value = src_pixel[0] + src_pixel[1] + src_pixel[2]; \
            accum = MAX(value, accum); \
        } \
    } \
    brightness = accum; \
}
        switch(color_model)
        {
            case BC_RGB888:
                TEST_BRIGHTEST(uint8_t, uint32_t, 3, 0)
                break;
            case BC_YUV888:
                TEST_BRIGHTEST(uint8_t, uint32_t, 3, 1)
                break;
        }

        if(brightness > *brightest ||
            (brightness == *brightest && n > *blob_size))
        {
            result = 1;
        }
    }

    if(result)
    {
        for(int i = 0; i < n; i++)
        {
            if((*current_x)[i] < min_x)
            {
// recompute center of left edge
                min_x = (*current_x)[i];
                min_y = max_y = (*current_y)[i];
            }
            else
            if((*current_x)[i] == min_x)
            {
// expand Y of left edge
                if((*current_y)[i] < min_y) min_y = (*current_y)[i];
                else
                if((*current_y)[i] > max_y) max_y = (*current_y)[i];
            }
        }

        *blob_x = min_x;
        *blob_y = (min_y + max_y) / 2;
        *brightest = brightness;
        *blob_size = n;
//        printf("BlobTracker::test_blob %d brightness=%d *brightest=%d n=%d\n",
//            __LINE__, (int)brightness,  (int)*brightest, n);
    }
    return result;
}


void BlobTracker::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->get_data());
	output.tag.set_title("BLOBTRACKER");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("REF_LAYER", config.ref_layer);
	output.tag.set_property("TARG_LAYER", config.targ_layer);
	output.tag.set_property("DRAW_GUIDES", config.draw_guides);
	output.tag.set_property("STABILIZE", config.stabilize);
	output.tag.set_property("MIN_SIZE", config.min_size);
	output.append_tag();
	output.terminate_string();
}

void BlobTracker::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->get_data());
    while(!input.read_tag())
    {
        if(input.tag.title_is("BLOBTRACKER"))
        {
            config.mode = input.tag.get_property("MODE", config.mode);
	        config.ref_layer = input.tag.get_property("REF_LAYER", config.ref_layer);
	        config.targ_layer = input.tag.get_property("TARG_LAYER", config.targ_layer);
	        config.draw_guides = input.tag.get_property("DRAW_GUIDES", config.draw_guides);
	        config.stabilize = input.tag.get_property("STABILIZE", config.stabilize);
	        config.min_size = input.tag.get_property("MIN_SIZE", config.min_size);
        }
    }
    config.boundaries();
}

void BlobTracker::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("BlobTracker::update_gui");
            BlobWindow *win = (BlobWindow*)thread->window;
            win->update_mode();
            win->draw_guides->update(config.draw_guides);
            win->stabilize->update(config.stabilize);
            win->min_size->update(config.min_size);
            thread->window->unlock_window();
        }
    }
}

void BlobTracker::render_gui(void *data, int size)
{
	if(thread)
	{
        BlobWindow *win = (BlobWindow*)thread->window;
        win->lock_window("BlobTracker::render_gui");
        char string[BCTEXTLEN];
        sprintf(string, "%d", (int)sqrt(*(int*)data));
        win->detected_size->update(string);
        win->unlock_window();
    }
}








