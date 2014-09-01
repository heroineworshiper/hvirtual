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

#ifndef MOTIONSCAN_H
#define MOTIONSCAN_H


#include "affine.inc"
#include "arraylist.h"
#include "loadbalance.h"
#include "opencvwrapper.inc"
#include "vframe.inc"
#include <stdint.h>




class MotionScan;

#define OVERSAMPLE 4
#define MOTION_FILE "/tmp/motion"
// Precision of rotation
#define MIN_ANGLE 0.0001
#define MIN_VARIANCE 0.20





class MotionScanStep
{
public:
	MotionScanStep();


// For multiple blocks
// Position of stationary block
	int block_x1, block_y1, block_x2, block_y2;
// Range of positions to scan
	int scan_x1, scan_y1, scan_x2, scan_y2;
	int dx;
	int dy;
	int64_t max_difference;
	int64_t min_difference;
	int64_t min_pixel;
	int is_border;
	int valid;
// For single block
	int step;
// 2 differences are calculated for each subpixel package
	int64_t difference1;
	int64_t difference2;
// Search position to nearest pixel
	int search_x;
	int search_y;
// Subpixel of search position
	int sub_x;
	int sub_y;
// Optimum angle for each difference computed during the search
	float angle1;
	float angle2;
};




class MotionScanPackage : public LoadPackage
{
public:
	MotionScanPackage();
	int step0;
	int step1;
};

class MotionScanCache
{
public:
	MotionScanCache(int x, int y, float angle, int64_t difference);
	int x, y;
	float angle;
	int64_t difference;
};

class MotionScanUnit : public LoadClient
{
public:
	MotionScanUnit(MotionScan *server);
	~MotionScanUnit();

	void process_package(LoadPackage *package);
/*
 * 	int64_t get_cache(int x, int y, float angle);
 * 	void put_cache(int x, int y, float angle, int64_t difference);
 */

	MotionScan *server;

	ArrayList<MotionScanCache*> cache;
};

class MotionScan : public LoadServer
{
public:
	MotionScan(int total_clients, 
		int total_packages);
	~MotionScan();

	friend class MotionScanUnit;

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();


	float get_macroblock_angle(int number);
// Test for identical frames before scanning
	void set_test_match(int value);
// Test for enough detail
	void set_test_variance(int value);


	enum
	{
// action_type
		TRACK,
		STABILIZE,
		TRACK_PIXEL,
		STABILIZE_PIXEL,
		NOTHING
	};
	
	enum
	{
// tracking_type
		CALCULATE_ABSDIFF,
		CALCULATE_SURF,
		SAVE_ABSDIFF,
		SAVE_SURF,
		LOAD,
		NO_CALCULATE
	};
	
	enum
	{
// frame_type
		TRACK_SINGLE,
		TRACK_PREVIOUS,
		PREVIOUS_SAME_BLOCK
	};


// Invoke the motion engine for a search
// Frame before motion
	void scan_frame(VFrame *previous_frame,
// Frame after motion
		VFrame *current_frame,
		int global_range_w,
		int global_range_h,
		int global_block_w,
		int global_block_h,
		double block_x,
		double block_y,
		int frame_type,
		int tracking_type,
		int action_type,
		int horizontal_only,
		int vertical_only,
		int source_position,
		int total_steps,
		int total_dx,
		int total_dy,
		int global_origin_x,
		int global_origin_y,
// Degrees from center to maximum angle
		float angle_range, 
// Accumulated angle from previous frames
		float total_angle,
// Total number of angles to test in each pass
		int total_angle_steps,
		float global_origin_angle);
	int64_t get_cache(int x, int y, float angle);
	void put_cache(int x, int y, float angle, int64_t difference);

	float calculate_variance(VFrame *frame,
		int x1,
		int y1,
		int x2,
		int y2);


	static int64_t abs_diff(unsigned char *prev_ptr,
		unsigned char *current_ptr,
		int prev_row_bytes,
		int current_row_bytes,
		int w,
		int h,
		int color_model);
	static int64_t abs_diff_sub(unsigned char *prev_ptr,
		unsigned char *current_ptr,
		int prev_row_bytes,
		int current_row_bytes,
		int w,
		int h,
		int color_model,
		int sub_x,
		int sub_y);


	static void clamp_scan(int w, 
		int h, 
		int *block_x1,
		int *block_y1,
		int *block_x2,
		int *block_y2,
		int *scan_x1,
		int *scan_y1,
		int *scan_x2,
		int *scan_y2,
		int use_absolute);


	void do_surf();
	double turn_direction(double target, double current);


// Change between previous frame and current frame multiplied by 
// OVERSAMPLE
	int dx_result;
	int dy_result;

	float dangle_result;

// If result was thrown out by the variance filter
	int result_valid;





private:
// OpenCV variables
	OpenCVWrapper *opencv;

// Dimensions of input frames
	int w;
	int h;
// Pointer to frame before motion
	VFrame *previous_frame;
// Pointer to frame after motion
	VFrame *current_frame;
// Macroblocks to compare from previous frame with rotation applied
	VFrame **macroblocks;
	int total_macroblocks;
// Frames passed from user
	VFrame *previous_frame_arg;
	VFrame *current_frame_arg;
	int skip;
// Test for identical frames before processing
// Faster to skip it if the frames are usually different
	int test_match;
// Test for enough detail 
	int test_variance;
// For single block
// Coordinates of object to search for
	int block_w;
	int block_h;
	int block_x1;
	int block_x2;
	int block_y1;
	int block_y2;
// Temporary storage of current best position of object
// Sometimes oversampled & sometimes single pixel.
	int x_result;
	int y_result;
// Region to scan
	int scan_w;
	int scan_h;
	int scan_x1;
	int scan_y1;
	int scan_x2;
	int scan_y2;
	int total_pixels;
	int total_steps;
	int edge_steps;
	int y_steps;
	int x_steps;
	int subpixel;
	int horizontal_only;
	int vertical_only;
	int global_origin_x;
	int global_origin_y;
// Range of angles to compare
	float scan_angle1, scan_angle2;
	float total_angle;
	int total_angle_steps;
	float global_origin_angle;
	AffineEngine *rotate_engine;

	ArrayList<MotionScanCache*> cache;
	ArrayList<MotionScanStep*> steps;
	Mutex *cache_lock;
};



#endif


