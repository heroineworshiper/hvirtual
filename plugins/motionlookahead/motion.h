/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef MOTION_H
#define MOTION_H

#include "affine.inc"
#include "motionscan.inc"
#include "overlayframe.inc"
#include "pluginvclient.h"

// Limits of global range in percent
#define MIN_RADIUS 1
#define MAX_RADIUS 100

// Limits of rotation range in degrees
#define MIN_ROTATION 1
#define MAX_ROTATION 25

// Limits of block size in percent.
#define MIN_BLOCK 1
#define MAX_BLOCK 100

// Precision of rotation
#define MIN_ANGLE 0.0001

// too slow to be practical with any more frames
#define MIN_LOOKAHEAD 1
#define MAX_LOOKAHEAD 100

class MotionLookaheadConfig
{
public:
	MotionLookaheadConfig();

	int equivalent(MotionLookaheadConfig &that);
	void copy_from(MotionLookaheadConfig &that);
	void interpolate(MotionLookaheadConfig &prev, 
		MotionLookaheadConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();

// Block position as percentage 0 - 100
	float block_x;
	float block_y;
// Block size as percent of image size
	int block_w;
	int block_h;
// Translation search range as percent of image size
	int range_w;
	int range_h;

    int enable;
	int do_rotate;
// rotation search in degrees
	int rotation_range;

	int draw_vectors;
// frames to look ahead
    int frames;
    int tracking_type;
};

class MotionVector
{
public:
// Change between previous frame and current frame multiplied by 
// OVERSAMPLE
	int dx_result;
	int dy_result;
	float angle_result;
};

class LeastSquares
{
public:
    LeastSquares(float y);
    void append(float y);
    float get_b();
    float get_m();

    float sum_x;
    float sum_y;
    float sum_xy;
    float sum_x2;
    int x;
    int n;
};

class MotionLookahead : public PluginVClient
{
public:
	MotionLookahead(PluginServer *server);
	~MotionLookahead();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	void draw_vectors(VFrame *frame);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	PLUGIN_CLASS_MEMBERS2(MotionLookaheadConfig)

// lookahead buffer
    ArrayList<VFrame*> frames;
// instantaneous vector for each frame
    ArrayList<MotionVector*> vectors;
// starting position of the lookahead buffer
    int64_t frames_start;
// lookahead frames read
    int frames_read;
// lookahead frames scanned can be less than frames_read
    int frames_scanned;

	MotionScan *engine;
	AffineEngine *rotate_engine;
    OverlayFrame *translate_engine;
// total accumulated vectors multiplied by OVERSAMPLE
    int total_dx;
    int total_dy;
    float total_angle;
// total regressed center position multiplied by OVERSAMPLE
    float center_dx;
    float center_dy;
    float center_angle;

// final offsets
    float current_dx;
    float current_dy;
    float current_angle;

// previous scanning options to know if we have to reset the lookahead vectors
    float prev_block_x;
    float prev_block_y;
    int prev_block_w;
    int prev_block_h;
    int prev_range_w;
    int prev_range_h;
    int prev_do_rotate;
    int prev_rotation_range;
    int prev_direction;
};




#endif



