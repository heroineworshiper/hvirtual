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

#ifndef BLOBTRACKER_H
#define BLOBTRACKER_H

#include "overlayframe.inc"
#include "pluginvclient.h"

#include <vector>

class BlobConfig
{
public:
    BlobConfig();
	int equivalent(BlobConfig &that);
	void copy_from(BlobConfig &that);
	void interpolate(BlobConfig &prev, 
		BlobConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();

    float targ_x;
    float targ_y;
// sqrt of the total pixels in the smallest blob
    int min_size;
    int window_size;
#define TOP 0
#define BOTTOM 1
    int ref_layer;
    int targ_layer;
    int draw_guides;
#define LEFTMOST_BLOB 0 // too many false positives
#define LARGEST_BLOB 1
#define BRIGHTEST_WINDOW 2 // vulnerable to larger area of dimmer pixels
#define BRIGHTEST_BLOB 3
    int mode;
    int stabilize;
};

class BlobTracker : public PluginVClient
{
public:
    BlobTracker(PluginServer *server);
	~BlobTracker();
    int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
    int is_multichannel();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
    void render_gui(void *data, int size);

    int test_blob(std::vector<int> *current_x, 
        std::vector<int> *current_y, 
        int *blob_x, 
        int *blob_y,
        int *blob_size,
        VFrame *targ_frame,
        float *brightest);
    PLUGIN_CLASS_MEMBERS2(BlobConfig)
    OverlayFrame *translator;
    VFrame *mask;
    int shift_x;
    int shift_y;
};


#endif




