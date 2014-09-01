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

#include "affine.h"
#include "clip.h"
#include <math.h>
#include "motionscan.h"
#include "mutex.h"
#include "opencvwrapper.h"
#include "vframe.h"



// The module which does the actual scanning



MotionScanStep::MotionScanStep()
{
	valid = 1;
}




MotionScanPackage::MotionScanPackage()
 : LoadPackage()
{
}






MotionScanUnit::MotionScanUnit(MotionScan *server)
 : LoadClient(server)
{
	this->server = server;
}

MotionScanUnit::~MotionScanUnit()
{
}



void MotionScanUnit::process_package(LoadPackage *package)
{
	MotionScanPackage *pkg2 = (MotionScanPackage*)package;
	int w = server->current_frame->get_w();
	int h = server->current_frame->get_h();
	int color_model = server->current_frame->get_color_model();
	int pixel_size = BC_CModels::calculate_pixelsize(color_model);



	for(int step = pkg2->step0; step < pkg2->step1; step++)
	{
		MotionScanStep *pkg = server->steps.get(step);

		pkg->difference1 = -1;
		pkg->difference2 = -1;
		pkg->angle1 = 0;
		pkg->angle2 = 0;



// Do 1 pass for each macroblock angle
		for(int angle_number = 0; 
			angle_number < server->total_macroblocks; 
			angle_number++)
		{
			float angle = server->get_macroblock_angle(angle_number);
			VFrame *macroblock = server->macroblocks[angle_number];
			int prev_row_bytes = server->previous_frame->get_bytes_per_line();
			int current_row_bytes = macroblock->get_bytes_per_line();

// Single pixel
			if(!server->subpixel)
			{
// Try cache
				int64_t difference1 = server->get_cache(pkg->search_x, 
					pkg->search_y,
					angle);

				if(difference1 >= 0)
				{
					if(difference1 < pkg->difference1 || pkg->difference1 < 0)
					{
						pkg->difference1 = difference1;
						pkg->angle1 = angle;
					}
				}
				else
// Test variance.  Real slow & no benefit.
// 				if(server->test_variance && 
// 					server->calculate_variance(server->previous_frame,
// 						pkg->search_x,
// 						pkg->search_y,
// 						pkg->search_x + pkg->block_x2 - pkg->block_x1,
// 						pkg->search_y + pkg->block_y2 - pkg->block_y1) < 
// 						MIN_VARIANCE)
// 				{
// 					pkg->difference1 = 0x7fffffffffffffffLL;
// 					pkg->difference2 = 0x7fffffffffffffffLL;
// 				}
// 				else
// New scan
				{
//printf("MotionScanUnit::process_package 1 search_x=%d search_y=%d scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d x_steps=%d y_steps=%d\n", 
//pkg->search_x, pkg->search_y, pkg->scan_x1, pkg->scan_y1, pkg->scan_x2, pkg->scan_y2, server->x_steps, server->y_steps);


// Pointers to first pixel in each block
					unsigned char *prev_ptr = server->previous_frame->get_rows()[
						pkg->search_y] +	
						pkg->search_x * pixel_size;
// 					unsigned char *current_ptr = server->current_frame->get_rows()[
// 						pkg->block_y1] +
// 						pkg->block_x1 * pixel_size;
					unsigned char *current_ptr = macroblock->get_rows()[0];

// Scan block
					difference1 = MotionScan::abs_diff(prev_ptr,
						current_ptr,
						prev_row_bytes,
						current_row_bytes,
						pkg->block_x2 - pkg->block_x1,
						pkg->block_y2 - pkg->block_y1,
						color_model);

					server->put_cache(pkg->search_x, 
						pkg->search_y, 
						angle,
						difference1);

					if(difference1 < pkg->difference1 || pkg->difference1 < 0)
					{
						pkg->difference1 = difference1;
						pkg->angle1 = angle;
					}

	// printf("MotionScanUnit::process_package %d search_x=%d search_y=%d angle=%f diff=%lld\n",
	// __LINE__, 
	// server->block_x1 - pkg->search_x, 
	// server->block_y1 - pkg->search_y, 
	// angle,
	// pkg->difference1);
				}
			}




			else




	// Sub pixel
			{
				unsigned char *prev_ptr = server->previous_frame->get_rows()[
					pkg->search_y] +
					pkg->search_x * pixel_size;
	// 			unsigned char *current_ptr = server->current_frame->get_rows()[
	// 				pkg->block_y1] +
	// 				pkg->block_x1 * pixel_size;
				unsigned char *current_ptr = macroblock->get_rows()[0];

	// With subpixel, there are two ways to compare each position, one by shifting
	// the previous frame and two by shifting the current frame.
				int64_t difference1 = MotionScan::abs_diff_sub(prev_ptr,
					current_ptr,
					prev_row_bytes,
					current_row_bytes,
					pkg->block_x2 - pkg->block_x1,
					pkg->block_y2 - pkg->block_y1,
					color_model,
					pkg->sub_x,
					pkg->sub_y);
				int64_t difference2 = MotionScan::abs_diff_sub(current_ptr,
					prev_ptr,
					current_row_bytes,
					prev_row_bytes,
					pkg->block_x2 - pkg->block_x1,
					pkg->block_y2 - pkg->block_y1,
					color_model,
					pkg->sub_x,
					pkg->sub_y);


				if(difference1 < pkg->difference1 || pkg->difference1 < 0)
				{
					pkg->difference1 = difference1;
					pkg->angle1 = angle;
				}

				if(difference2 < pkg->difference2 || pkg->difference2 < 0)
				{
					pkg->difference2 = difference2;
					pkg->angle2 = angle;
				}
	// printf("MotionScanUnit::process_package sub_x=%d sub_y=%d search_x=%d search_y=%d angle=%f diff1=%lld diff2=%lld\n",
	// pkg->sub_x,
	// pkg->sub_y,
	// pkg->search_x,
	// pkg->search_y,
	// angle,
	// pkg->difference1,
	// pkg->difference2);
			}
		}

	}

}




















MotionScan::MotionScan(int total_clients,
	int total_packages)
 : LoadServer(
//1, 1 
total_clients, total_packages 
)
{
	cache_lock = new Mutex("MotionScan::cache_lock");
	macroblocks = 0;
	total_macroblocks = 0;
	rotate_engine = 0;
	test_match = 1;
	test_variance = 1;
	result_valid = 0;

	opencv = 0;
}

MotionScan::~MotionScan()
{
	delete cache_lock;
	for(int i = 0; i < total_macroblocks; i++)
		delete macroblocks[i];
	delete [] macroblocks;
	delete rotate_engine;
	steps.remove_all_objects();

	delete opencv;
}


void MotionScan::init_packages()
{
// Create steps
	steps.remove_all_objects();
	for(int i = 0; i < total_steps; i++)
	{
		MotionScanStep *pkg = new MotionScanStep;
		steps.append(pkg);

		pkg->block_x1 = block_x1;
		pkg->block_x2 = block_x2;
		pkg->block_y1 = block_y1;
		pkg->block_y2 = block_y2;
		pkg->scan_x1 = scan_x1;
		pkg->scan_x2 = scan_x2;
		pkg->scan_y1 = scan_y1;
		pkg->scan_y2 = scan_y2;
		pkg->step = i;
		pkg->difference1 = 0;
		pkg->difference2 = 0;
		pkg->dx = 0;
		pkg->dy = 0;
		pkg->valid = 1;
		
		if(!subpixel)
		{
			pkg->search_x = pkg->scan_x1 + (pkg->step % x_steps) *
				(scan_x2 - scan_x1) / x_steps;
			pkg->search_y = pkg->scan_y1 + (pkg->step / x_steps) *
				(scan_y2 - scan_y1) / y_steps;
			pkg->sub_x = 0;
			pkg->sub_y = 0;
		}
		else
		{
			pkg->sub_x = pkg->step % (OVERSAMPLE * 2);
			pkg->sub_y = pkg->step / (OVERSAMPLE * 2);

			if(horizontal_only)
			{
				pkg->sub_y = 0;
			}

			if(vertical_only)
			{
				pkg->sub_x = 0;
			}

			pkg->search_x = pkg->scan_x1 + pkg->sub_x / OVERSAMPLE + 1;
			pkg->search_y = pkg->scan_y1 + pkg->sub_y / OVERSAMPLE + 1;
			pkg->sub_x %= OVERSAMPLE;
			pkg->sub_y %= OVERSAMPLE;



// printf("MotionScan::init_packages %d i=%d search_x=%d search_y=%d sub_x=%d sub_y=%d\n", 
// __LINE__,
// i,
// pkg->search_x,
// pkg->search_y,
// pkg->sub_x,
// pkg->sub_y);
		}

// printf("MotionScan::init_packages %d %d,%d %d,%d %d,%d\n",
// __LINE__,
// scan_x1,
// scan_x2,
// scan_y1,
// scan_y2,
// pkg->search_x,
// pkg->search_y);
	}




// Set package coords
	for(int i = 0; i < get_total_packages(); i++)
	{
		MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);
		pkg->step0 = total_steps * i / get_total_packages();
		pkg->step1 = total_steps * (i + 1) / get_total_packages();
	}
}

LoadClient* MotionScan::new_client()
{
	return new MotionScanUnit(this);
}

LoadPackage* MotionScan::new_package()
{
	return new MotionScanPackage;
}

float MotionScan::get_macroblock_angle(int number)
{

	if(total_angle_steps == 0) return 0;

	float result = scan_angle1 +
		(scan_angle2 - scan_angle1) * 
		number / 
		total_angle_steps;
// printf("MotionScan::get_macroblock_angle %d number=%d total_angle_steps=%d result=%f\n",
// __LINE__,
// number,
// total_angle_steps,
// result);
	return result;
}



void MotionScan::set_test_match(int value)
{
	this->test_match = value;
}

void MotionScan::set_test_variance(int value)
{
	this->test_variance = value;
}


double MotionScan::turn_direction(double target, double current)
{
// For range of 0 - 180
	double result = 0;
	if( current > M_PI / 2 && target < -M_PI / 2 )
		result = 2.0 * M_PI + target - current;
	else
	if( current < -M_PI / 2 && target > M_PI / 2 )
		result = -2.0 * M_PI + target - current;
	else
		result = target - current;
	
	
	if(result < -M_PI) result += 2 * M_PI;
	else
	if(result > M_PI) result -= 2 * M_PI;
	return result;
}

void MotionScan::do_surf()
{
// Get scene dimensions
	scan_x1 = x_result - scan_w / 2;
	scan_y1 = y_result - scan_h / 2;
	scan_x2 = x_result + scan_w / 2;
	scan_y2 = y_result + scan_h / 2;

//printf("MotionScan::do_surf %d\n",  __LINE__);


	clamp_scan(w, 
		h, 
		&block_x1,
		&block_y1,
		&block_x2,
		&block_y2,
		&scan_x1,
		&scan_y1,
		&scan_x2,
		&scan_y2,
		0);


	if(!opencv) opencv = new OpenCVWrapper;
	if(opencv->scan(current_frame,
		previous_frame,
		block_x1, 
		block_y1,
		block_x2,
		block_y2,
		scan_x1,
		scan_y1,
		scan_x2,
		scan_y2))
	{
// Average corners to get center
		float dst_x_avg = 0;
		float dst_y_avg = 0;
		float src_x_avg = 0;
		float src_y_avg = 0;

		for(int i = 0; i < 4; i++)
		{
			dst_x_avg += opencv->get_dst_x(i);
			dst_y_avg += opencv->get_dst_y(i);
		}
		
		dst_x_avg /= 4;
		dst_y_avg /= 4;


		src_x_avg = (block_x2 + block_x1) / 2;
		src_y_avg = (block_y2 + block_y1) / 2;

// Get angles of edges in destination
		float dst_angles[4];
		for(int i = 0; i < 4; i++)
		{
			if(i == 0)
			{
				dst_angles[i] = atan2(opencv->get_dst_y(i) - opencv->get_dst_y(3), 
					opencv->get_dst_x(i) - opencv->get_dst_x(3));
			}
			else
			{
				dst_angles[i] = atan2(opencv->get_dst_y(i) - opencv->get_dst_y(i - 1), 
					opencv->get_dst_x(i) - opencv->get_dst_x(i - 1));
			}
			
			while(dst_angles[i] < 0) dst_angles[i] += 2 * M_PI;
// printf("MotionScan::do_surf %d i=%d angle=%f\n", 
// __LINE__,
// i,
// dst_angles[i] * 360 / 2 / M_PI);
		}

// Average change of all angles is the total angle change
		dangle_result = -(turn_direction(-M_PI / 2, dst_angles[0]) +
			turn_direction(0, dst_angles[1]) +
			turn_direction(M_PI / 2, dst_angles[2]) +
			turn_direction(M_PI, dst_angles[3])) / 4;
		dangle_result = dangle_result * 360 / 2 / M_PI;

// Get center of object
		dx_result = (int)((dst_x_avg - src_x_avg) * OVERSAMPLE);
		dy_result = (int)((dst_y_avg - src_y_avg) * OVERSAMPLE);

// printf("MotionScan::do_surf %d dx_result=%f dy_result=%f angle=%f\n", 
// __LINE__,
// (float)dx_result / OVERSAMPLE,
// (float)dy_result / OVERSAMPLE,
// dangle_result);

		result_valid = 1;
	}
	else
	{
//printf("MotionScan::do_surf %d\n",  __LINE__);
		dx_result = 0;
		dy_result = 0;
		dangle_result = 0;
		result_valid = 0;
	}

}


void MotionScan::scan_frame(VFrame *previous_frame,
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
	float angle_range, 
	float total_angle,
	int total_angle_steps,
	float global_origin_angle)
{
	const int debug = 0;

#if 0
printf("MotionScan::scan_frame %d %d %d %d %d block_x=%f block_y=%f %d %d %d %d %d %d %d total_dx=%d\n"
"total_dy=%d %d %d %f %f %d %f\n", 
__LINE__,
global_range_w,
global_range_h,
global_block_w,
global_block_h,
block_x,
block_y,
frame_type,
tracking_type,
action_type,
horizontal_only,
vertical_only,
source_position,
total_steps,
total_dx,
total_dy,
global_origin_x,
global_origin_y,
angle_range, 
total_angle,
total_angle_steps,
global_origin_angle);
#endif


	this->previous_frame_arg = previous_frame;
	this->current_frame_arg = current_frame;
	this->horizontal_only = horizontal_only;
	this->vertical_only = vertical_only;
	this->previous_frame = previous_frame_arg;
	this->current_frame = current_frame_arg;
	this->global_origin_x = global_origin_x;
	this->global_origin_y = global_origin_y;
	this->global_origin_angle = global_origin_angle;
	
	this->scan_angle1 = global_origin_angle - angle_range;
	this->scan_angle2 = global_origin_angle + angle_range;
	this->total_angle = total_angle;
	this->total_angle_steps = total_angle_steps;
	subpixel = 0;
	dx_result = 0;
	dy_result = 0;
	dangle_result = 0;

// Reset the cache
	cache.remove_all_objects();

// Single macroblock
	w = current_frame->get_w();
	h = current_frame->get_h();

// Initial search parameters
	scan_w = w * global_range_w / 100;
	scan_h = h * global_range_h / 100;
	block_w = w * global_block_w / 100;
	block_h = h * global_block_h / 100;

// Location of block in previous frame
	block_x1 = (int)(w * block_x / 100 - block_w / 2);
	block_y1 = (int)(h * block_y / 100 - block_h / 2);
	block_x2 = (int)(w * block_x / 100 + block_w / 2);
	block_y2 = (int)(h * block_y / 100 + block_h / 2);

// Offset to location of previous block.  This offset needn't be very accurate
// since it's the difference between the previous image and current image 
// we want.
	if(frame_type == MotionScan::TRACK_PREVIOUS)
	{
		block_x1 += total_dx / OVERSAMPLE;
		block_y1 += total_dy / OVERSAMPLE;
		block_x2 += total_dx / OVERSAMPLE;
		block_y2 += total_dy / OVERSAMPLE;
// Rotation is independant of accumulated angle
//		this->scan_angle1 += total_angle;
//		this->scan_angle2 += total_angle;
	}

	skip = 0;
	result_valid = 1;

	switch(tracking_type)
	{
// Don't calculate
		case MotionScan::NO_CALCULATE:
			dx_result = 0;
			dy_result = 0;
			dangle_result = 0;
			skip = 1;
			break;

		case MotionScan::LOAD:
		{
//printf("MotionScan::scan_frame %d\n", __LINE__);
// Load result from disk
			char string[BCTEXTLEN];
			sprintf(string, "%s%06d", 
				MOTION_FILE, 
				source_position);
			FILE *input = fopen(string, "r");
			if(input)
			{
				int temp = fscanf(input, 
					"%d %d %f", 
					&dx_result,
					&dy_result,
					&dangle_result);
				fclose(input);
				skip = 1;
			}
			break;
		}

// Scan from scratch
		default:
			skip = 0;
			break;
	}

	if(!skip && test_match)
	{
		if(previous_frame->data_matches(current_frame))
		{
printf("MotionScan::scan_frame: frames match. skipping.\n");
			dx_result = 0;
			dy_result = 0;
         	dangle_result = 0;
			skip = 1;
		}
	}


// Test origin frame detail	
	if(test_variance)
	{
		float variance = calculate_variance(current_frame,
			block_x1,
			block_y1,
			block_x2,
			block_y2);

// printf("MotionScan::scan_frame %d x1=%d y1=%d x2=%d y2=%d variance=%f\n", 
// __LINE__, 
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// variance);

		if(variance < MIN_VARIANCE) 
		{
			result_valid = 0;
			skip = 1;
		}
	}


// Perform scan
	if(!skip)
	{
// Location of block in current frame
		int origin_offset_x = this->global_origin_x * w / 100;
		int origin_offset_y = this->global_origin_y * h / 100;
		x_result = block_x1 + origin_offset_x;
		y_result = block_y1 + origin_offset_y;


		if(tracking_type == CALCULATE_SURF ||
			tracking_type == SAVE_SURF)
		{
			
			do_surf();
			
			
		}
		else
		{
// CALCULATE_ABSDIFF
// SAVE_ABSDIFF
//printf("MotionScan::scan_frame %d\n", __LINE__);
			float angle_result = 0;

// Determine min angle from size of block
			double min_angle1 = atan((double)(block_y2 - block_y1) / (block_x2 - block_x1));
			double min_angle2 = atan((double)(block_y2 - block_y1 - 1) / (block_x2 - block_x1 + 1));
			double min_angle = fabs(min_angle2 - min_angle1) / OVERSAMPLE;
			min_angle = MAX(min_angle, MIN_ANGLE);
// Convert to degrees
			min_angle = min_angle * 360 / 2 / M_PI;
			min_angle *= total_angle_steps;

// printf("MotionScan::scan_frame 1 %d %d %d %d %d %d %d %d\n",
// block_x1 + block_w / 2,
// block_y1 + block_h / 2,
// block_w,
// block_h,
// block_x1,
// block_y1,
// block_x2,
// block_y2);

			while(1)
			{
// Cache needs to be cleared if downsampling is used because the sums of 
// different downsamplings can't be compared.
// Subpixel never uses the cache.
//			cache.remove_all_objects();
// printf("MotionScan::scan_frame %d angle_range=%f scan_angle1=%f scan_angle2=%f min_angle=%f\n", 
// __LINE__, 
// angle_range,
// scan_angle1, 
// scan_angle2,
// min_angle * total_angle_steps);

				scan_x1 = x_result - scan_w / 2;
				scan_y1 = y_result - scan_h / 2;
				scan_x2 = x_result + scan_w / 2;
				scan_y2 = y_result + scan_h / 2;


// Zero out requested values
				if(horizontal_only)
				{
					scan_y1 = block_y1;
					scan_y2 = block_y1 + 1;
				}
				if(vertical_only)
				{
					scan_x1 = block_x1;
					scan_x2 = block_x1 + 1;
				}

// printf("MotionScan::scan_frame 1 %d %d %d %d %d %d %d %d\n",
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// scan_x1,
// scan_y1,
// scan_x2,
// scan_y2);

// Clamp the block coords before the scan so we get useful scan coords.
				clamp_scan(w, 
					h, 
					&block_x1,
					&block_y1,
					&block_x2,
					&block_y2,
					&scan_x1,
					&scan_y1,
					&scan_x2,
					&scan_y2,
					0);

// printf("MotionScan::scan_frame 1 %d block_x1=%d block_y1=%d block_x2=%d block_y2=%d\n	   scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d\n	x_result=%d y_result=%d\n", 
// __LINE__,
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// scan_x1, 
// scan_y1, 
// scan_x2, 
// scan_y2, 
// x_result, 
// y_result);

// Give up if invalid coords.
				if(scan_y2 <= scan_y1 ||
					scan_x2 <= scan_x1 ||
					block_x2 <= block_x1 ||
					block_y2 <= block_y1)
				{
//printf("MotionScan::scan_frame %d invalid coordinates\n", __LINE__);
					break;
				}




// Create origin frame macroblocks
				if(total_macroblocks != total_angle_steps + 1 ||
					(total_macroblocks && 
					(macroblocks[0]->get_w() != block_x2 - block_x1 ||
					macroblocks[0]->get_h() != block_y2 - block_y1 ||
					macroblocks[0]->get_color_model() != previous_frame->get_color_model())))
				{
					for(int i = 0; i < total_macroblocks; i++)
						delete macroblocks[i];
					delete [] macroblocks;

					total_macroblocks = total_angle_steps + 1;
					macroblocks = new VFrame*[total_macroblocks];
					for(int i = 0; i < total_macroblocks; i++)
					{
						macroblocks[i] = new VFrame;
						macroblocks[i]->set_use_shm(0);
						macroblocks[i]->reallocate(
							0,   // Data if shared
							-1,             // shmid if IPC
							0,         // plane offsets if shared YUV
							0,
							0,
							block_x2 - block_x1, 
							block_y2 - block_y1, 
							current_frame->get_color_model(), 
							-1);
					}
				}



				if(!rotate_engine)
				{
					rotate_engine = new AffineEngine(
						get_total_clients(),
						get_total_packages());
				}

				for(int i = 0; i < total_macroblocks; i++)
				{
					macroblocks[i]->clear_frame();
					rotate_engine->set_in_pivot((block_x1 + block_x2) / 2,
						(block_y1 + block_y2) / 2);
					rotate_engine->set_out_pivot((block_x2 - block_x1) / 2,
						(block_y2 - block_y1) / 2);

// Size of macroblock is output
					rotate_engine->set_out_viewport(0, 
						0, 
						block_x2 - block_x1,
						block_y2 - block_y1);

//printf("MotionScan::scan_frame %d i=%d angle=%f\n", __LINE__, i, get_macroblock_angle(i));
					rotate_engine->rotate(macroblocks[i], 
						current_frame, 
						get_macroblock_angle(i));
				}



// For subpixel, the top row and left column are skipped
				if(subpixel)
				{

//printf("MotionScan::scan_frame %d subpixel %d %d\n", __LINE__, x_result, y_result);
// Scan every subpixel in a 2 pixel * 2 pixel square
					total_pixels = (2 * OVERSAMPLE) * (2 * OVERSAMPLE);

					this->total_steps = total_pixels;
// These aren't used in subpixel
					this->x_steps = OVERSAMPLE * 2;
					this->y_steps = OVERSAMPLE * 2;

					process_packages();

// Get least difference
					int64_t min_difference = -1;
					for(int i = 0; i < this->total_steps; i++)
					{
						MotionScanStep *pkg = steps.get(i);
//printf("MotionScan::scan_frame %d search_x=%d search_y=%d sub_x=%d sub_y=%d diff1=%lld diff2=%lld\n", 
//__LINE__, pkg->search_x, pkg->search_y, pkg->sub_x, pkg->sub_y, pkg->difference1, pkg->difference2);
						if(pkg->difference1 < min_difference || 
							min_difference == -1)
						{
							min_difference = pkg->difference1;

// The sub coords are 1 pixel up & left of the block coords
							x_result = pkg->search_x * OVERSAMPLE + pkg->sub_x;
							y_result = pkg->search_y * OVERSAMPLE + pkg->sub_y;
							angle_result = pkg->angle1;

// Fill in results
							dx_result = block_x1 * OVERSAMPLE - x_result;
							dy_result = block_y1 * OVERSAMPLE - y_result;
							dangle_result = angle_result;
// printf("MotionScan::scan_frame %d dx_result=%d dy_result=%d dangle_result=%f\n", 
// __LINE__, dx_result, dy_result, dangle_result);
						}

						if(pkg->difference2 < min_difference)
						{
							min_difference = pkg->difference2;

							x_result = pkg->search_x * OVERSAMPLE - pkg->sub_x;
							y_result = pkg->search_y * OVERSAMPLE - pkg->sub_y;
							angle_result = pkg->angle2;

							dx_result = block_x1 * OVERSAMPLE - x_result;
							dy_result = block_y1 * OVERSAMPLE - y_result;
							dangle_result = angle_result;
// printf("MotionScan::scan_frame %d dx_result=%d dy_result=%d dangle_result=%f\n", 
// __LINE__, dx_result, dy_result, dangle_result);
						}
					}

					break;
				}
				else
// Single pixel
				{
					total_pixels = (scan_x2 - scan_x1) * (scan_y2 - scan_y1);
					this->total_steps = MIN(total_steps, total_pixels);

					if(this->total_steps == total_pixels)
					{
						x_steps = scan_x2 - scan_x1;
						y_steps = scan_y2 - scan_y1;
					}
					else
					{
						x_steps = (int)sqrt(this->total_steps);
						y_steps = (int)sqrt(this->total_steps);
					}






// printf("MotionScan::scan_frame %d this->total_steps=%d\n", 
// __LINE__, 
// this->total_steps);


					process_packages();

// Get least difference
					int64_t min_difference = -1;
					for(int i = 0; i < this->total_steps; i++)
					{
						MotionScanStep *pkg = steps.get(i);
//printf("MotionScan::scan_frame %d search_x=%d search_y=%d sub_x=%d sub_y=%d diff1=%lld diff2=%lld\n", 
//__LINE__, pkg->search_x, pkg->search_y, pkg->sub_x, pkg->sub_y, pkg->difference1, pkg->difference2);
						if(pkg->difference1 < min_difference || 
							min_difference == -1)
						{
							min_difference = pkg->difference1;
							x_result = pkg->search_x;
							y_result = pkg->search_y;
							x_result *= OVERSAMPLE;
							y_result *= OVERSAMPLE;
							angle_result = pkg->angle1;
// printf("MotionScan::scan_frame %d x_result=%d y_result=%d diff=%lld angle_result=%f\n", 
// __LINE__, block_x1 * OVERSAMPLE - x_result, block_y1 * OVERSAMPLE - y_result, pkg->difference1, angle_result);
						}
					}


// If a new search is required, rescale results back to pixels.
					if(this->total_steps >= total_pixels)
					{
// Single pixel accuracy reached.
// Continue at current granularity until minimum angle * 2 is reached.
						if(total_angle_steps > 0 &&
							angle_range > min_angle)
						{
							angle_range /= 2;
							scan_angle1 = angle_result - angle_range;
							scan_angle2 = angle_result + angle_range;
							x_result /= OVERSAMPLE;
							y_result /= OVERSAMPLE;
							if(debug) printf("MotionScan::scan_frame %d angle1=%f angle2=%f angle_result=%f\n",
								__LINE__,
								scan_angle1,
								scan_angle2,
								angle_result);
						}
						else
// Now do exhaustive subpixel search.
						if(action_type == MotionScan::STABILIZE ||
							action_type == MotionScan::TRACK ||
							action_type == MotionScan::NOTHING)
						{
							if(angle_range > min_angle) angle_range /= 2;
//								angle_range = 0;
							scan_angle1 = angle_result - angle_range;
							scan_angle2 = angle_result + angle_range;
							if(debug) printf("MotionScan::scan_frame %d angle1=%f angle2=%f angle_result=%f\n",
								__LINE__,
								scan_angle1,
								scan_angle2,
								angle_result);
							x_result /= OVERSAMPLE;
							y_result /= OVERSAMPLE;
							scan_w = 2;
							scan_h = 2;
							subpixel = 1;
						}
						else
						{
							if(debug) printf("MotionScan::scan_frame %d angle_range=%f angle_result=%f\n",
								__LINE__,
								angle_range,
								angle_result);
// Fill in results and quit
							dx_result = block_x1 * OVERSAMPLE - x_result;
							dy_result = block_y1 * OVERSAMPLE - y_result;
							dangle_result = angle_result;
//printf("MotionScan::scan_frame %d %d %d\n", __LINE__, dx_result, dy_result);
							break;
						}
					}
					else
// Reduce scan area and try again
					{
						scan_w = (scan_x2 - scan_x1) / 2;
						scan_h = (scan_y2 - scan_y1) / 2;
						x_result /= OVERSAMPLE;
						y_result /= OVERSAMPLE;
						if(angle_range > min_angle) angle_range /= 2;
						if(debug) printf("MotionScan::scan_frame %d angle1=%f angle2=%f angle_result=%f\n", 
							__LINE__, 
							scan_angle1,
							scan_angle2,
							angle_result);
						scan_angle1 = angle_result - angle_range;
						scan_angle2 = angle_result + angle_range;
					}
				}
			}

			dx_result *= -1;
			dy_result *= -1;
		}
	}
//printf("MotionScan::scan_frame %d\n", __LINE__);


	if(vertical_only) dx_result = 0;
	if(horizontal_only) dy_result = 0;



// Write results
	if(tracking_type == MotionScan::SAVE_ABSDIFF ||
		tracking_type == MotionScan::SAVE_SURF)
	{
		char string[BCTEXTLEN];
		sprintf(string, 
			"%s%06d", 
			MOTION_FILE, 
			source_position);
		FILE *output = fopen(string, "w");
		if(output)
		{
			fprintf(output, 
				"%d %d %f\n",
				dx_result,
				dy_result,
				dangle_result);
			fclose(output);
		}
		else
		{
			printf("MotionScan::scan_frame %d: save coordinate failed", __LINE__);
		}
	}

if(debug) printf("MotionScan::scan_frame %d dx=%.2f dy=%.2f dangle=%f\n", 
	__LINE__,
	(float)this->dx_result / OVERSAMPLE,
	(float)this->dy_result / OVERSAMPLE,
	this->dangle_result);
}










// math routines


float MotionScan::calculate_variance(VFrame *frame,
	int x1,
	int y1,
	int x2,
	int y2)
{
	int w = frame->get_w();
	int h = frame->get_h();
	float result;
	
	CLAMP(x1, 0, w - 1);
	CLAMP(x2, 0, w - 1);
	CLAMP(y1, 0, h - 1);
	CLAMP(y2, 0, h - 1);

#define VARIANCE(type, components, max) \
{ \
	type *row = (type*)frame->get_rows()[y1]; \
	type min_accum[3]; \
	type max_accum[3]; \
 \
	min_accum[0] = max_accum[0] = row[x1 * components + 0]; \
	min_accum[1] = max_accum[1] = row[x1 * components + 1]; \
	min_accum[2] = max_accum[2] = row[x1 * components + 2]; \
 \
	for(int i = y1; i < y2; i++) \
	{ \
		row = (type*)frame->get_rows()[i] + x1 * components; \
		for(int j = x1; j < x2; j++) \
		{ \
			for(int k = 0; k < 3; k++) \
			{ \
				max_accum[k] = MAX(*row, max_accum[k]); \
				min_accum[k] = MIN(*row, min_accum[k]); \
				row++; \
			} \
			if(components == 4) row++; \
		} \
	} \
 \
 \
	result = max_accum[0] - min_accum[0]; \
	result = MAX(result, max_accum[1] - min_accum[1]); \
	result = MAX(result, max_accum[2] - min_accum[2]); \
	result /= max; \
}




	for(int i = y1; i < y2; i++)
	{
		switch(frame->get_color_model())
		{
			case BC_RGB888:
				VARIANCE(unsigned char, 3, 0xff)
				break;
			case BC_RGBA8888:
				VARIANCE(unsigned char, 4, 0xff)
				break;
			case BC_RGB_FLOAT:
				VARIANCE(float, 3, 1.0)
				break;
			case BC_RGBA_FLOAT:
				VARIANCE(float, 4, 1.0)
				break;
			case BC_YUV888:
				VARIANCE(unsigned char, 3, 0xff)
				break;
			case BC_YUVA8888:
				VARIANCE(unsigned char, 4, 0xff)
				break;
		}
	}
	
	return result;
}







int64_t MotionScan::get_cache(int x, int y, float angle)
{
	int64_t result = -1;
	if(get_total_clients() > 1) cache_lock->lock("MotionScan::get_cache");
	for(int i = 0; i < cache.total; i++)
	{
		MotionScanCache *ptr = cache.values[i];
		if(ptr->x == x && ptr->y == y && EQUIV(angle, ptr->angle))
		{
			result = ptr->difference;
			break;
		}
	}
	if(get_total_clients() > 1) cache_lock->unlock();
	return result;
}

void MotionScan::put_cache(int x, int y, float angle, int64_t difference)
{
	MotionScanCache *ptr = new MotionScanCache(x, y, angle, difference);
	if(get_total_clients() > 1) cache_lock->lock("MotionScan::put_cache");
	cache.append(ptr);
	if(get_total_clients() > 1) cache_lock->unlock();
}



#define ABS_DIFF(type, temp_type, multiplier, components) \
{ \
	temp_type result_temp = 0; \
	for(int i = 0; i < h; i++) \
	{ \
		type *prev_row = (type*)prev_ptr; \
		type *current_row = (type*)current_ptr; \
		for(int j = 0; j < w; j++) \
		{ \
			for(int k = 0; k < 3; k++) \
			{ \
				temp_type difference; \
				difference = *prev_row++ - *current_row++; \
				if(difference < 0) \
					result_temp -= difference; \
				else \
					result_temp += difference; \
			} \
/* ignore alpha */ \
			if(components == 4) \
			{ \
				prev_row++; \
				current_row++; \
			} \
		} \
		prev_ptr += prev_row_bytes; \
		current_ptr += current_row_bytes; \
	} \
	result = (int64_t)(result_temp * multiplier); \
}

int64_t MotionScan::abs_diff(unsigned char *prev_ptr,
	unsigned char *current_ptr,
	int prev_row_bytes,
	int current_row_bytes,
	int w,
	int h,
	int color_model)
{
	int64_t result = 0;
	switch(color_model)
	{
		case BC_RGB888:
			ABS_DIFF(unsigned char, int, 1, 3)
			break;
		case BC_RGBA8888:
			ABS_DIFF(unsigned char, int, 1, 4)
			break;
		case BC_RGB_FLOAT:
			ABS_DIFF(float, float, 0x10000, 3)
			break;
		case BC_RGBA_FLOAT:
			ABS_DIFF(float, float, 0x10000, 4)
			break;
		case BC_YUV888:
			ABS_DIFF(unsigned char, int, 1, 3)
			break;
		case BC_YUVA8888:
			ABS_DIFF(unsigned char, int, 1, 4)
			break;
	}
	return result;
}



#define ABS_DIFF_SUB(type, temp_type, multiplier, components) \
{ \
	temp_type result_temp = 0; \
	temp_type y2_fraction = sub_y * 0x100 / OVERSAMPLE; \
	temp_type y1_fraction = 0x100 - y2_fraction; \
	temp_type x2_fraction = sub_x * 0x100 / OVERSAMPLE; \
	temp_type x1_fraction = 0x100 - x2_fraction; \
	for(int i = 0; i < h_sub; i++) \
	{ \
		type *prev_row1 = (type*)prev_ptr; \
		type *prev_row2 = (type*)prev_ptr + components; \
		type *prev_row3 = (type*)(prev_ptr + prev_row_bytes); \
		type *prev_row4 = (type*)(prev_ptr + prev_row_bytes) + components; \
		type *current_row = (type*)current_ptr; \
		for(int j = 0; j < w_sub; j++) \
		{ \
/* Scan each component */ \
			for(int k = 0; k < 3; k++) \
			{ \
				temp_type difference; \
				temp_type prev_value = \
					(*prev_row1++ * x1_fraction * y1_fraction + \
					*prev_row2++ * x2_fraction * y1_fraction + \
					*prev_row3++ * x1_fraction * y2_fraction + \
					*prev_row4++ * x2_fraction * y2_fraction) / \
					0x100 / 0x100; \
				temp_type current_value = *current_row++; \
				difference = prev_value - current_value; \
				if(difference < 0) \
					result_temp -= difference; \
				else \
					result_temp += difference; \
			} \
 \
/* skip alpha */ \
			if(components == 4) \
			{ \
				prev_row1++; \
				prev_row2++; \
				prev_row3++; \
				prev_row4++; \
				current_row++; \
			} \
		} \
		prev_ptr += prev_row_bytes; \
		current_ptr += current_row_bytes; \
	} \
	result = (int64_t)(result_temp * multiplier); \
}




int64_t MotionScan::abs_diff_sub(unsigned char *prev_ptr,
	unsigned char *current_ptr,
	int prev_row_bytes,
	int current_row_bytes,
	int w,
	int h,
	int color_model,
	int sub_x,
	int sub_y)
{
	int h_sub = h - 1;
	int w_sub = w - 1;
	int64_t result = 0;

	switch(color_model)
	{
		case BC_RGB888:
			ABS_DIFF_SUB(unsigned char, int, 1, 3)
			break;
		case BC_RGBA8888:
			ABS_DIFF_SUB(unsigned char, int, 1, 4)
			break;
		case BC_RGB_FLOAT:
			ABS_DIFF_SUB(float, float, 0x10000, 3)
			break;
		case BC_RGBA_FLOAT:
			ABS_DIFF_SUB(float, float, 0x10000, 4)
			break;
		case BC_YUV888:
			ABS_DIFF_SUB(unsigned char, int, 1, 3)
			break;
		case BC_YUVA8888:
			ABS_DIFF_SUB(unsigned char, int, 1, 4)
			break;
	}
	return result;
}





MotionScanCache::MotionScanCache(int x, int y, float angle, int64_t difference)
{
	this->x = x;
	this->y = y;
	this->angle = angle;
	this->difference = difference;
}



void MotionScan::clamp_scan(int w, 
	int h, 
	int *block_x1,
	int *block_y1,
	int *block_x2,
	int *block_y2,
	int *scan_x1,
	int *scan_y1,
	int *scan_x2,
	int *scan_y2,
	int use_absolute)
{
// printf("MotionMain::clamp_scan 1 w=%d h=%d block=%d %d %d %d scan=%d %d %d %d absolute=%d\n",
// w,
// h,
// *block_x1,
// *block_y1,
// *block_x2,
// *block_y2,
// *scan_x1,
// *scan_y1,
// *scan_x2,
// *scan_y2,
// use_absolute);


// Limit size of scan area
// Used for drawing vectors
	if(use_absolute)
	{
// scan is always out of range before block.
		if(*scan_x1 < 0)
		{
			int difference = -*scan_x1;
//			*block_x1 += difference;
			*scan_x1 = 0;
		}

		if(*scan_y1 < 0)
		{
			int difference = -*scan_y1;
//			*block_y1 += difference;
			*scan_y1 = 0;
		}

		if(*scan_x2 > w)
		{
			int difference = *scan_x2 - w;
//			*block_x2 -= difference;
			*scan_x2 -= difference;
		}

		if(*scan_y2 > h)
		{
			int difference = *scan_y2 - h;
//			*block_y2 -= difference;
			*scan_y2 -= difference;
		}


		CLAMP(*scan_x1, 0, w);
		CLAMP(*scan_y1, 0, h);
		CLAMP(*scan_x2, 0, w);
		CLAMP(*scan_y2, 0, h);
	}
	else
	{
// Limit range of upper left block coordinates
// Used for motion tracking
		CLAMP(*block_x1, 0, w);
		CLAMP(*block_x2, 0, w);
		CLAMP(*block_y1, 0, h);
		CLAMP(*block_y2, 0, h);

		if(*scan_x1 < 0)
		{
			int difference = -*scan_x1;
//			*block_x1 += difference;
			*scan_x2 += difference;
			*scan_x1 = 0;
		}

		if(*scan_y1 < 0)
		{
			int difference = -*scan_y1;
//			*block_y1 += difference;
			*scan_y2 += difference;
			*scan_y1 = 0;
		}

		if(*scan_x2 - *block_x1 + *block_x2 >= w)
		{
			int difference = *scan_x2 - *block_x1 + *block_x2 - w;
			*scan_x2 -= difference;
//			*block_x2 -= difference;
		}

		if(*scan_y2 - *block_y1 + *block_y2 >= h)
		{
			int difference = *scan_y2 - *block_y1 + *block_y2 - h;
			*scan_y2 -= difference;
//			*block_y2 -= difference;
		}


// 		CLAMP(*scan_x1, 0, w - (*block_x2 - *block_x1));
// 		CLAMP(*scan_y1, 0, h - (*block_y2 - *block_y1));
// 		CLAMP(*scan_x2, 0, w - (*block_x2 - *block_x1));
// 		CLAMP(*scan_y2, 0, h - (*block_y2 - *block_y1));
	}

// Sanity checks which break the calculation but should never happen if the
// center of the block is inside the frame.
	CLAMP(*block_x1, 0, w);
	CLAMP(*block_x2, 0, w);
	CLAMP(*block_y1, 0, h);
	CLAMP(*block_y2, 0, h);

// printf("MotionMain::clamp_scan 2 w=%d h=%d block=%d %d %d %d scan=%d %d %d %d absolute=%d\n",
// w,
// h,
// *block_x1,
// *block_y1,
// *block_x2,
// *block_y2,
// *scan_x1,
// *scan_y1,
// *scan_x2,
// *scan_y2,
// use_absolute);
}



