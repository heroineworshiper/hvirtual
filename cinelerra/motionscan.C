/*
 * CINELERRA
 * Copyright (C) 2016-2024 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "clip.h"
#include "language.h"
#include "motioncache.h"
#include "motionscan.h"
#include "mutex.h"
#include "pluginclient.h"
#include "vframe.h"


#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// The module which does the actual scanning

// starting level of detail
#define STARTING_DOWNSAMPLE 16
// minimum size in each level of detail
#define MIN_DOWNSAMPLED_SIZE 16
// minimum scan range
#define MIN_DOWNSAMPLED_SCAN 4
// scan range for subpixel mode
#define SUBPIXEL_RANGE 4

MotionScanPackage::MotionScanPackage()
 : LoadPackage()
{
	valid = 1;
}






MotionScanUnit::MotionScanUnit(MotionScan *server)
 : LoadClient(server)
{
	this->server = server;
}

MotionScanUnit::~MotionScanUnit()
{
}


void MotionScanUnit::single_pixel(MotionScanPackage *pkg)
{
	int w = server->current_frame->get_w();
	int h = server->current_frame->get_h();
	int color_model = server->current_frame->get_color_model();
	int pixel_size = cmodel_calculate_pixelsize(color_model);

// printf("MotionScanUnit::process_package %d search_x=%d search_y=%d scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d x_steps=%d y_steps=%d\n", 
// __LINE__, 
// pkg->search_x, 
// pkg->search_y, 
// pkg->scan_x1, 
// pkg->scan_y1, 
// pkg->scan_x2, 
// pkg->scan_y2, 
// server->x_steps, 
// server->y_steps);

// Pointers to first pixel in each block
	unsigned char *prev_ptr = server->previous_frame->get_rows()[
		pkg->search_y] +
		pkg->search_x * pixel_size;
    int prev_row_bytes = server->previous_frame->get_bytes_per_line();
	unsigned char *current_ptr = 0;
	int current_row_bytes = 0;

	if(server->do_rotate)
	{
		current_ptr = server->rotated_current[pkg->angle_step]->get_rows()[
			pkg->block_y1] + 
			pkg->block_x1 * pixel_size;
        current_row_bytes = server->rotated_current[pkg->angle_step]->get_bytes_per_line();
	}
	else
	{
		current_ptr = server->current_frame->get_rows()[
			pkg->block_y1] +
			pkg->block_x1 * pixel_size;
        current_row_bytes = server->current_frame->get_bytes_per_line();
	}


// printf("MotionScanUnit::process_package %d row_bytes=%d %d\n", 
// __LINE__,
// prev_row_bytes,
// current_row_bytes);

// Scan block
	pkg->difference1 = MotionScan::abs_diff(prev_ptr,
		current_ptr,
        prev_row_bytes,
		current_row_bytes,
		pkg->block_x2 - pkg->block_x1,
		pkg->block_y2 - pkg->block_y1,
		color_model);

// printf("MotionScanUnit::process_package %d angle_step=%d diff=%d\n", 
// __LINE__, 
// pkg->angle_step,
// pkg->difference1);
// printf("MotionScanUnit::process_package %d search_x=%d search_y=%d diff=%lld\n",
// __LINE__, server->block_x1 - pkg->search_x, server->block_y1 - pkg->search_y, pkg->difference1);
}

void MotionScanUnit::subpixel(MotionScanPackage *pkg)
{
//PRINT_TRACE
	int w = server->current_frame->get_w();
	int h = server->current_frame->get_h();
	int color_model = server->current_frame->get_color_model();
	int pixel_size = cmodel_calculate_pixelsize(color_model);
	unsigned char *prev_ptr = server->previous_frame->get_rows()[
		pkg->search_y] +
		pkg->search_x * pixel_size;
    int prev_row_bytes = server->previous_frame->get_bytes_per_line();
	unsigned char *current_ptr = 0;
	int current_row_bytes = 0;

	if(server->do_rotate)
    {
        current_ptr = server->rotated_current[server->best_angle_step]->get_rows()[
			pkg->block_y1] + 
			pkg->block_x1 * pixel_size;
        current_row_bytes = server->rotated_current[server->best_angle_step]->get_bytes_per_line();
    }
    else
    {
        current_ptr = server->current_frame->get_rows()[
		    pkg->block_y1] +
		    pkg->block_x1 * pixel_size;
        current_row_bytes = server->current_frame->get_bytes_per_line();
    }

// With subpixel, there are two ways to compare each position, one by shifting
// the previous frame and two by shifting the current frame.
	pkg->difference1 = MotionScan::abs_diff_sub(prev_ptr,
		current_ptr,
        prev_row_bytes,
		current_row_bytes,
		pkg->block_x2 - pkg->block_x1,
		pkg->block_y2 - pkg->block_y1,
		color_model,
		pkg->sub_x,
		pkg->sub_y);
	pkg->difference2 = MotionScan::abs_diff_sub(current_ptr,
		prev_ptr,
		current_row_bytes,
        prev_row_bytes,
		pkg->block_x2 - pkg->block_x1,
		pkg->block_y2 - pkg->block_y1,
		color_model,
		pkg->sub_x,
		pkg->sub_y);
// printf("MotionScanUnit::process_package sub_x=%d sub_y=%d search_x=%d search_y=%d diff1=%lld diff2=%lld\n",
// pkg->sub_x,
// pkg->sub_y,
// pkg->search_x,
// pkg->search_y,
// pkg->difference1,
// pkg->difference2);
}

void MotionScanUnit::process_package(LoadPackage *package)
{
	MotionScanPackage *pkg = (MotionScanPackage*)package;


// Single pixel
	if(!server->subpixel)
	{
		single_pixel(pkg);
	}
	else
// Sub pixel
	{
		subpixel(pkg);
	}




}


















MotionScan::MotionScan(int total_clients,
	int total_packages)
 : LoadServer(
// DEBUG
//1, 1 
total_clients, total_packages 
)
{
	test_match = 1;
	rotated_current = 0;
	rotater = 0;
	downsample_cache = 0;
	shared_downsample = 0;
}

MotionScan::~MotionScan()
{
	if(downsample_cache && !shared_downsample)
	{
		delete downsample_cache;
		downsample_cache = 0;
		shared_downsample = 0;
	}

	if(rotated_current)
	{
		for(int i = 0; i < total_rotated; i++)
		{
			delete rotated_current[i];
		}
		
		delete [] rotated_current;
	}
	delete rotater;
}


void MotionScan::init_packages()
{
// Set package coords
// Total range of positions to scan with downsampling
	int downsampled_scan_x1 = scan_x1 / current_downsample;
	int downsampled_scan_x2 = scan_x2 / current_downsample;
	int downsampled_scan_y1 = scan_y1 / current_downsample;
	int downsampled_scan_y2 = scan_y2 / current_downsample;
	int downsampled_block_x1 = block_x1 / current_downsample;
	int downsampled_block_x2 = block_x2 / current_downsample;
	int downsampled_block_y1 = block_y1 / current_downsample;
	int downsampled_block_y2 = block_y2 / current_downsample;


//printf("MotionScan::init_packages %d %d\n", __LINE__, get_total_packages());
// printf("MotionScan::init_packages %d current_downsample=%d scan_x1=%d scan_x2=%d block_x1=%d block_x2=%d\n", 
// __LINE__, 
// current_downsample,
// downsampled_scan_x1,
// downsampled_scan_x2,
// downsampled_block_x1,
// downsampled_block_x2);
// if(current_downsample == 8 && downsampled_scan_x1 == 47)
// {
// downsampled_previous->write_png("/tmp/previous");
// downsampled_current->write_png("/tmp/current");
// }

	for(int i = 0; i < get_total_packages(); i++)
	{
		MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);

		pkg->block_x1 = downsampled_block_x1;
		pkg->block_x2 = downsampled_block_x2;
		pkg->block_y1 = downsampled_block_y1;
		pkg->block_y2 = downsampled_block_y2;
		pkg->difference1 = 0;
		pkg->difference2 = 0;
		pkg->dx = 0;
		pkg->dy = 0;
		pkg->valid = 1;
		pkg->angle_step = 0;
		
		if(!subpixel)
		{
			int current_y_step = (i / angle_steps) / x_steps;
			int current_x_step = (i / angle_steps) % x_steps;

//printf("MotionScan::init_packages %d i=%d x_step=%d y_step=%d angle_step=%d\n",
//__LINE__, i, current_x_step, current_y_step, current_angle_step);
			pkg->search_x = downsampled_scan_x1 + current_x_step *
				(scan_x2 - scan_x1) / current_downsample / x_steps;
			pkg->search_y = downsampled_scan_y1 + current_y_step *
				(scan_y2 - scan_y1) / current_downsample / y_steps;

			if(do_rotate)
			{
				pkg->angle_step = i % angle_steps;
			}
			else
			{
				pkg->angle_step = 0;
			}

			pkg->sub_x = 0;
			pkg->sub_y = 0;
		}
		else
		{
			pkg->sub_x = i % (OVERSAMPLE * SUBPIXEL_RANGE);
			pkg->sub_y = i / (OVERSAMPLE * SUBPIXEL_RANGE);

// 			if(horizontal_only)
// 			{
// 				pkg->sub_y = 0;
// 			}
// 
// 			if(vertical_only)
// 			{
// 				pkg->sub_x = 0;
// 			}

			pkg->search_x = scan_x1 + pkg->sub_x / OVERSAMPLE + 1;
			pkg->search_y = scan_y1 + pkg->sub_y / OVERSAMPLE + 1;
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
}

LoadClient* MotionScan::new_client()
{
	return new MotionScanUnit(this);
}

LoadPackage* MotionScan::new_package()
{
	return new MotionScanPackage;
}


void MotionScan::set_test_match(int value)
{
	this->test_match = value;
}

void MotionScan::set_cache(MotionCache *cache)
{
	this->downsample_cache = cache;
	shared_downsample = 1;
}





double MotionScan::step_to_angle(int step, double center)
{
	if(step < angle_steps / 2)
	{
		return center - angle_step * (angle_steps / 2 - step);
	}
	else
	if(step > angle_steps / 2)
	{
		return center + angle_step * (step - angle_steps / 2);
	}
	else
	{
		return center;
	}
}

static int compare(const void *p1, const void *p2)
{
	double value1 = *(double*)p1;
	double value2 = *(double*)p2;

//printf("compare %d value1=%f value2=%f\n", __LINE__, value1, value2);
	return value1 > value2;
}



// pixel accurate motion search
void MotionScan::pixel_search(int &x_result, int &y_result, double &r_result)
{
	int debug = 0;

// reduce level of detail until we have enough steps
	while(current_downsample > 1 &&
		((block_x2 - block_x1) / current_downsample < MIN_DOWNSAMPLED_SIZE ||
		(block_y2 - block_y1) / current_downsample < MIN_DOWNSAMPLED_SIZE 
		||
		 (scan_x2 - scan_x1) / current_downsample < MIN_DOWNSAMPLED_SCAN ||
		(scan_y2 - scan_y1) / current_downsample < MIN_DOWNSAMPLED_SCAN
		))
	{
		current_downsample /= 2;
	}



// create downsampled images.  
// Need to keep entire frame to search for rotation.
	int downsampled_prev_w = previous_frame_arg->get_w() / current_downsample;
	int downsampled_prev_h = previous_frame_arg->get_h() / current_downsample;
	int downsampled_current_w = current_frame_arg->get_w() / current_downsample;
	int downsampled_current_h = current_frame_arg->get_h() / current_downsample;

// printf("MotionScan::pixel_search %d current_downsample=%d current_frame_arg->get_w()=%d downsampled_current_w=%d\n",
// __LINE__,
// current_downsample,
// current_frame_arg->get_w(),
// downsampled_current_w);


// compute the smallest angle which can be measured at the current resolution
	x_steps = (scan_x2 - scan_x1) / current_downsample;
	y_steps = (scan_y2 - scan_y1) / current_downsample;

// in rads
	double test_angle1 = atan2((double)downsampled_current_h / 2 - 1, (double)downsampled_current_w / 2);
	double test_angle2 = atan2((double)downsampled_current_h / 2, (double)downsampled_current_w / 2 - 1);

// convert to deg
	angle_step = 360.0f * fabs(test_angle1 - test_angle2) / 2 / M_PI;


// number of steps to scan the current range at the current resolution
	if(do_rotate && angle_step < rotation_range)
	{
		angle_steps = 1 + (int)((scan_angle2 - scan_angle1) / angle_step + 0.5);
	}
	else
	{
		angle_steps = 1;
	}

// printf("MotionScan::pixel_search %d r_result=%f test_angle1=%f test_angle2=%f angle_step=%f angle_steps=%d downsample=%d\n", 
// __LINE__, 
// r_result,
// 360.0f * test_angle1 / 2 / M_PI,
// 360.0f * test_angle2 / 2 / M_PI,
// angle_step,
// angle_steps,
// current_downsample);

// printf("MotionScan::pixel_search %d angle_step=%f angle_steps=%d downsample=%d\n", 
// __LINE__, 
// angle_step,
// angle_steps,
// current_downsample);


	if(current_downsample > 1)
	{
		if(!downsample_cache)
		{
			downsample_cache = new MotionCache();
			shared_downsample = 0;
		}


		if(!shared_downsample)
		{
			downsample_cache->clear();
		}

		previous_frame = downsample_cache->get_image(current_downsample, 
			1,
			downsampled_prev_w,
			downsampled_prev_h,
			previous_frame_arg);
		current_frame = downsample_cache->get_image(current_downsample, 
			0,
			downsampled_current_w,
			downsampled_current_h,
			current_frame_arg);


	}
	else
	{
		previous_frame = previous_frame_arg;
		current_frame = current_frame_arg;
	}



// printf("MotionScan::pixel_search %d current_downsample=%d do_rotate=%d x_steps=%d y_steps=%d angle_steps=%d\n", 
// __LINE__, 
// current_downsample,
// do_rotate,
// x_steps,
// y_steps,
// angle_steps);



// test variance of constant macroblock
	int color_model = current_frame->get_color_model();
	int pixel_size = cmodel_calculate_pixelsize(color_model);
	int row_bytes = current_frame->get_bytes_per_line();
	int block_w = block_x2 - block_x1;
	int block_h = block_y2 - block_y1;

	unsigned char *current_ptr = 
		current_frame->get_rows()[block_y1 / current_downsample] + 
		(block_x1 / current_downsample) * pixel_size;
	unsigned char *previous_ptr = 
		previous_frame->get_rows()[scan_y1 / current_downsample] + 
		(scan_x1 / current_downsample) * pixel_size;

// previous_frame->draw_rect(scan_x1 / current_downsample,
// scan_y1 / current_downsample,
// (scan_x2 + block_w) / current_downsample,
// (scan_y2 + block_h) / current_downsample);
// char string[BCTEXTLEN];
// sprintf(string, "/tmp/current_frame%d.png", current_downsample);
// current_frame->write_png(string, 9);
// sprintf(string, "/tmp/previous_frame%d.png", current_downsample);
// previous_frame->write_png(string, 9);

// test detail in prev & current frame
	double range1 = calculate_range(current_ptr,
 		row_bytes,
 		block_w / current_downsample,
 		block_h / current_downsample,
 		color_model);

	if(range1 < 1)
	{
printf("MotionScan::pixel_search %d not enough detail range1=%f\n", __LINE__, range1);
		failed = 1;
		return;
	}

	double range2 = calculate_range(previous_ptr,
 		row_bytes,
 		(scan_x2 - scan_x1 + block_w) / current_downsample,
 		(scan_y2 - scan_y1 + block_h) / current_downsample,
 		color_model);

	if(range2 < 1)
	{
printf("MotionScan::pixel_search %d not enough detail range2=%f\n", __LINE__, range2);
		failed = 1;
		return;
	}


// create rotated images
	if(rotated_current &&
		(total_rotated != angle_steps ||
		rotated_current[0]->get_w() != downsampled_current_w ||
		rotated_current[0]->get_h() != downsampled_current_h))
	{
		for(int i = 0; i < total_rotated; i++)
		{
			delete rotated_current[i];
		}
		
		delete [] rotated_current;
		rotated_current = 0;
		total_rotated = 0;
	}

	if(do_rotate)
	{
		total_rotated = angle_steps;
		
		
		if(!rotated_current)
		{
			rotated_current = new VFrame*[total_rotated];
			bzero(rotated_current, sizeof(VFrame*) * total_rotated);
		}

// printf("MotionScan::pixel_search %d total_rotated=%d w=%d h=%d block_w=%d block_h=%d\n", 
// __LINE__,
// total_rotated,
// downsampled_current_w,
// downsampled_current_h,
// (block_x2 - block_x1) / current_downsample,
// (block_y2 - block_y1) / current_downsample);
		for(int i = 0; i < angle_steps; i++)
		{

// printf("MotionScan::pixel_search %d w=%d h=%d x=%d y=%d angle=%f\n", 
// __LINE__,
// downsampled_current_w,
// downsampled_current_h,
// (block_x1 + block_x2) / 2 / current_downsample,
// (block_y1 + block_y2) / 2 / current_downsample,
// step_to_angle(i, r_result));

// printf("MotionScan::pixel_search %d i=%d rotated_current[i]=%p\n", 
// __LINE__,
// i,
// rotated_current[i]);
			if(!rotated_current[i])
			{
				rotated_current[i] = new VFrame();
				rotated_current[i]->set_use_shm(0);
				rotated_current[i]->reallocate(0, 
					-1,
					0,
					0,
					0,
					downsampled_current_w + 1, 
					downsampled_current_h + 1, 
					current_frame_arg->get_color_model(), 
					-1);
//printf("MotionScan::pixel_search %d\n", __LINE__);
			}


			if(!rotater)
			{
				rotater = new AffineEngine(get_total_clients(),
					get_total_clients());
			}

// get smallest block size required to rotate the angle
// haven't had any luck with this, so adding fudge factors
			double diag = hypot((block_x2 - block_x1) / current_downsample,
				(block_y2 - block_y1) / current_downsample);
			double angle1 = atan2(block_y2 - block_y1, block_x2 - block_x1) + 
				TO_RAD(step_to_angle(i, r_result));
			double angle2 = -atan2(block_y2 - block_y1, block_x2 - block_x1) + 
				TO_RAD(step_to_angle(i, r_result));
			double max_horiz = MAX(abs(diag * cos(angle1)), abs(diag * cos(angle2)));
			double max_vert = MAX(abs(diag * sin(angle1)), abs(diag * sin(angle2)));


			int center_x = (block_x1 + block_x2) / 2 / current_downsample;
			int center_y = (block_y1 + block_y2) / 2 / current_downsample;
			int x1 = center_x - max_horiz / 2 - 1;
			int y1 = center_y - max_vert / 2 - 1;
			int x2 = x1 + max_horiz + 2;
			int y2 = y1 + max_vert + 2;
// is maximum inclusive or exclusive?
			CLAMP(x1, 0, downsampled_current_w);
			CLAMP(y1, 0, downsampled_current_h);
			CLAMP(x2, 0, downsampled_current_w);
			CLAMP(y2, 0, downsampled_current_h);

//printf("MotionScan::pixel_search %d %f %f %d %d\n", 
//__LINE__, TO_DEG(angle1), TO_DEG(angle2), (int)max_horiz, (int)max_vert);
			rotater->set_in_viewport(x1, 
				y1,
				x2 - x1,
				y2 - y1);
			rotater->set_out_viewport(x1, 
				y1,
				x2 - x1,
				y2 - y1);

			rotater->set_in_pivot(center_x, center_y);
			rotater->set_out_pivot(center_x, center_y);

			rotater->rotate(rotated_current[i],
				current_frame,
				step_to_angle(i, r_result));


//if(current_downsample == 4)
//{
// rotated_current[i]->draw_rect(block_x1 / current_downsample,
// block_y1 / current_downsample,
// block_x2 / current_downsample,
// block_y2 / current_downsample);
// char string[BCTEXTLEN];
// sprintf(string, "/tmp/rotated%d.png", i);
// rotated_current[i]->write_png(string, 9);

// printf("MotionScan::pixel_search %d step=%d step_to_angle=%f\n", 
// __LINE__, 
// i,
// step_to_angle(i, r_result));
//}

		}
// if(current_downsample == 4)
// {
// current_frame->write_png("/tmp/current_frame.png", 9);
// previous_frame->write_png("/tmp/previous_frame.png", 9);
// }

	}






// printf("MotionScan::pixel_search %d block x=%d y=%d w=%d h=%d\n", 
// __LINE__, 
// block_x1 / current_downsample,
// block_y1 / current_downsample,
// block_w / current_downsample,
// block_h / current_downsample);







//exit(1);
	total_steps = x_steps * y_steps * angle_steps;
//printf("MotionScan::pixel_search %d total_steps=%d\n", __LINE__, total_steps);

	set_package_count(total_steps);
	process_packages();
	




// Get least difference
	int64_t min_difference = -1;
    double prev_r_result = r_result;
	for(int i = 0; i < get_total_packages(); i++)
	{
		MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);


		if(pkg->difference1 < min_difference || i == 0)
		{
			min_difference = pkg->difference1;
			x_result = pkg->search_x * current_downsample * OVERSAMPLE;
			y_result = pkg->search_y * current_downsample * OVERSAMPLE;
			r_result = step_to_angle(pkg->angle_step, prev_r_result);
            best_angle_step = pkg->angle_step;

// if(current_downsample == 4)
// {
// printf("MotionScan::pixel_search %d block_x1=%d block_y1=%d block_x2=%d block_y2=%d search_x=%d search_y=%d angle_step=%d diff=%ld\n", 
// __LINE__, 
// pkg->block_x1 * current_downsample,
// pkg->block_y1 * current_downsample,
// pkg->block_x2 * current_downsample,
// pkg->block_y2 * current_downsample,
// pkg->search_x * current_downsample, 
// pkg->search_y * current_downsample, 
// pkg->angle_step,
// pkg->difference1);
// }

		}
	}


// printf("MotionScan::pixel_search %d best_angle_step=%d\n", 
// __LINE__,
// best_angle_step);



// printf("MotionScan::pixel_search %d current_downsample=%d x_result=%f y_result=%f r_result=%f\n", 
// __LINE__,
// current_downsample,
// (float)x_result / OVERSAMPLE,
// (float)y_result / OVERSAMPLE,
// r_result);

}


// subpixel motion search
void MotionScan::subpixel_search(int &x_result, int &y_result)
{
	previous_frame = previous_frame_arg;
	current_frame = current_frame_arg;

//printf("MotionScan::subpixel_search %d %d %d\n", __LINE__, x_result, y_result);
// Scan every subpixel in a SUBPIXEL_RANGE * SUBPIXEL_RANGE square
	total_steps = (SUBPIXEL_RANGE * OVERSAMPLE) * (SUBPIXEL_RANGE * OVERSAMPLE);

// These aren't used in subpixel
	x_steps = OVERSAMPLE * SUBPIXEL_RANGE;
	y_steps = OVERSAMPLE * SUBPIXEL_RANGE;
	angle_steps = 1;

	set_package_count(this->total_steps);
	process_packages();

// Get least difference
	int64_t min_difference = -1;
	for(int i = 0; i < get_total_packages(); i++)
	{
		MotionScanPackage *pkg = (MotionScanPackage*)get_package(i);
//printf("MotionScan::subpixel_search %d search_x=%d search_y=%d sub_x=%d sub_y=%d diff1=%lld diff2=%lld\n", 
//__LINE__, pkg->search_x, pkg->search_y, pkg->sub_x, pkg->sub_y, pkg->difference1, pkg->difference2);
		if(pkg->difference1 < min_difference || min_difference == -1)
		{
			min_difference = pkg->difference1;

// The sub coords are 1 pixel up & left of the block coords
			x_result = pkg->search_x * OVERSAMPLE + pkg->sub_x;
			y_result = pkg->search_y * OVERSAMPLE + pkg->sub_y;


// Fill in results
			dx_result = block_x1 * OVERSAMPLE - x_result;
			dy_result = block_y1 * OVERSAMPLE - y_result;
//printf("MotionScan::subpixel_search %d dx_result=%d dy_result=%d diff=%lld\n", 
//__LINE__, dx_result, dy_result, min_difference);
		}

		if(pkg->difference2 < min_difference)
		{
			min_difference = pkg->difference2;

			x_result = pkg->search_x * OVERSAMPLE - pkg->sub_x;
			y_result = pkg->search_y * OVERSAMPLE - pkg->sub_y;

			dx_result = block_x1 * OVERSAMPLE - x_result;
			dy_result = block_y1 * OVERSAMPLE - y_result;
//printf("MotionScan::subpixel_search %d dx_result=%d dy_result=%d diff=%lld\n", 
//__LINE__, dx_result, dy_result, min_difference);
		}
	}
}


void MotionScan::scan_frame(VFrame *previous_frame,
	VFrame *current_frame,
	int global_range_w,
	int global_range_h,
	int global_block_w,
	int global_block_h,
	int block_x,
	int block_y,
	int frame_type,
	int tracking_type,
	int action_type,
	int horizontal_only,
	int vertical_only,
	int source_position,
	int total_dx,
	int total_dy,
	int global_origin_x,
	int global_origin_y,
	int do_motion,
	int do_rotate,
	double rotation_center,
	double rotation_range)
{
	this->previous_frame_arg = previous_frame;
	this->current_frame_arg = current_frame;
	this->horizontal_only = horizontal_only;
	this->vertical_only = vertical_only;
	this->previous_frame = previous_frame_arg;
	this->current_frame = current_frame_arg;
	this->global_origin_x = global_origin_x;
	this->global_origin_y = global_origin_y;
	this->action_type = action_type;
	this->do_motion = do_motion;
	this->do_rotate = do_rotate;
	this->rotation_center = rotation_center;
	this->rotation_range = rotation_range;

//printf("MotionScan::scan_frame %d\n", __LINE__);
	dx_result = 0;
	dy_result = 0;
	dr_result = 0;
	failed = 0;

	subpixel = 0;
// starting level of detail
// TODO: base it on a table of resolutions
	current_downsample = STARTING_DOWNSAMPLE;
// DEBUG
//	current_downsample = 1;
	angle_step = 0;

// Single macroblock
	int w = current_frame->get_w();
	int h = current_frame->get_h();

// Initial search parameters
	scan_w = global_range_w;
	scan_h = global_range_h;

	int block_w = global_block_w;
	int block_h = global_block_h;

// printf("MotionScan::scan_frame %d %d %d %d %d %d %d %d %d\n", 
// __LINE__, 
// global_range_w, 
// global_range_h, 
// global_block_w, 
// global_block_h, 
// scan_w, 
// scan_h,
// block_w,
// block_h);

// Location of block in previous frame
	block_x1 = (int)(block_x - block_w / 2);
	block_y1 = (int)(block_y - block_h / 2);
	block_x2 = (int)(block_x + block_w / 2);
	block_y2 = (int)(block_y + block_h / 2);

// Offset to location of previous block.  This offset needn't be very accurate
// since it's the offset of the previous image and current image we want.
	if(frame_type == MotionScan::TRACK_PREVIOUS)
	{
		block_x1 += total_dx / OVERSAMPLE;
		block_y1 += total_dy / OVERSAMPLE;
		block_x2 += total_dx / OVERSAMPLE;
		block_y2 += total_dy / OVERSAMPLE;
	}

	skip = 0;

	switch(tracking_type)
	{
// Don't calculate
		case MotionScan::NO_CALCULATE:
			dx_result = 0;
			dy_result = 0;
			dr_result = rotation_center;
			skip = 1;
			break;

		case MotionScan::LOAD:
		{
// Load result from disk
			char string[BCTEXTLEN];

			skip = 1;
			if(do_motion)
			{
				sprintf(string, "%s%06d", 
					MOTION_FILE, 
					source_position);
//printf("MotionScan::scan_frame %d %s\n", __LINE__, string);
				FILE *input = fopen(string, "r");
				if(input)
				{
					int temp = fscanf(input, 
						"%d %d", 
						&dx_result,
						&dy_result);
// HACK
//dx_result *= 2;
//dy_result *= 2;
//printf("MotionScan::scan_frame %d %d %d\n", __LINE__, dx_result, dy_result);
					fclose(input);
				}
				else
				{
					skip = 0;
				}
			}

			if(do_rotate)
			{
				sprintf(string, 
					"%s%06d", 
					ROTATION_FILE, 
					source_position);
				FILE *input = fopen(string, "r");
				if(input)
				{
					int temp = fscanf(input, "%f", &dr_result);
// DEBUG
//dr_result += 0.25;
					fclose(input);
				}
				else
				{
					skip = 0;
				}
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
printf("MotionScan::scan_frame: data matches. skipping.\n");
			dx_result = 0;
			dy_result = 0;
			dr_result = rotation_center;
			skip = 1;
		}
	}


// Perform scan
	if(!skip)
	{
// Location of block in current frame
		int origin_offset_x = this->global_origin_x;
		int origin_offset_y = this->global_origin_y;
// top left corner of block
		int x_result = block_x1 + origin_offset_x;
		int y_result = block_y1 + origin_offset_y;
		double r_result = rotation_center;

// printf("MotionScan::scan_frame 1 %d %d %d %d %d %d %d %d\n",
// block_x1 + block_w / 2,
// block_y1 + block_h / 2,
// block_w,
// block_h,
// block_x1,
// block_y1,
// block_x2,
// block_y2);

		while(!failed)
		{
// top left of the block is here
			scan_x1 = x_result - scan_w / 2;
			scan_y1 = y_result - scan_h / 2;
			scan_x2 = x_result + scan_w / 2;
			scan_y2 = y_result + scan_h / 2;
			scan_angle1 = r_result - rotation_range;
			scan_angle2 = r_result + rotation_range;
			
// printf("MotionScan::scan_frame %d scan_angle1=%f scan_angle2=%f\n",
// __LINE__,
// scan_angle1,
// scan_angle2);

// printf("MotionScan::scan_frame %d x_result=%d y_result=%d block_x1=%d block_y1=%d block_x2=%d block_y2=%d scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d\n",
// __LINE__, 
// x_result,
// y_result,
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


// printf("MotionScan::scan_frame %d x_result=%d y_result=%d block_x1=%d block_y1=%d block_x2=%d block_y2=%d scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d\n", 
// __LINE__,
// x_result, 
// y_result,
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// scan_x1, 
// scan_y1, 
// scan_x2, 
// scan_y2);


// Give up if invalid coords.
			if(scan_y2 <= scan_y1 ||
				scan_x2 <= scan_x1 ||
				block_x2 <= block_x1 ||
				block_y2 <= block_y1)
			{
				break;
			}

// For subpixel, the top row and left column are skipped
			if(subpixel)
			{

				subpixel_search(x_result, y_result);
// printf("MotionScan::scan_frame %d x_result=%d y_result=%d\n", 
// __LINE__, 
// x_result / OVERSAMPLE, 
// y_result / OVERSAMPLE);

				break;
			}
			else
// Single pixel
			{
				pixel_search(x_result, y_result, r_result);
//printf("MotionScan::scan_frame %d x_result=%d y_result=%d\n", __LINE__, x_result / OVERSAMPLE, y_result / OVERSAMPLE);
				
				if(failed)
				{
					dr_result = 0;
					dx_result = 0;
					dy_result = 0;
				}
				else
				if(current_downsample <= 1)
				{
			// Single pixel accuracy reached.  Now do exhaustive subpixel search.
					if(action_type == MotionScan::STABILIZE ||
						action_type == MotionScan::TRACK ||
						action_type == MotionScan::NOTHING)
					{
						x_result /= OVERSAMPLE;
						y_result /= OVERSAMPLE;
//printf("MotionScan::scan_frame %d x_result=%d y_result=%d\n", __LINE__, x_result, y_result);
						scan_w = SUBPIXEL_RANGE;
						scan_h = SUBPIXEL_RANGE;
// Final R result
						dr_result = rotation_center - r_result;
						subpixel = 1;
					}
					else
					{
// Fill in results and quit
						dx_result = block_x1 * OVERSAMPLE - x_result;
						dy_result = block_y1 * OVERSAMPLE - y_result;
						dr_result = rotation_center - r_result;
						break;
					}
				}
				else
// Reduce scan area and try again
				{
//					scan_w = (scan_x2 - scan_x1) / 2;
//					scan_h = (scan_y2 - scan_y1) / 2;
// need slightly more than 2x downsampling factor

					if(current_downsample * 3 < scan_w &&
						current_downsample * 3 < scan_h)
					{
						scan_w = current_downsample * 3;
						scan_h = current_downsample * 3;
					}

					if(angle_step * 1.5 < rotation_range)
					{
						rotation_range = angle_step * 1.5;
					}
//printf("MotionScan::scan_frame %d %f %f\n", __LINE__, angle_step, rotation_range);

					current_downsample /= 2;

// convert back to pixels
					x_result /= OVERSAMPLE;
					y_result /= OVERSAMPLE;
// debug
//exit(1);
				}
			}
		}

		dx_result *= -1;
		dy_result *= -1;
		dr_result *= -1;
	}
// printf("MotionScan::scan_frame %d dx=%f dy=%f dr=%f\n", 
// __LINE__, 
// (float)dx_result / OVERSAMPLE, 
// (float)dy_result / OVERSAMPLE, 
// dr_result);




// Write results
	if(!skip && tracking_type == MotionScan::SAVE)
	{
		char string[BCTEXTLEN];
		
		
		if(do_motion)
		{
			sprintf(string, 
				"%s%06d", 
				MOTION_FILE, 
				source_position);
			FILE *output = fopen(string, "w");
			if(output)
			{
				fprintf(output, 
					"%d %d\n",
					dx_result,
					dy_result);
				fclose(output);
			}
			else
			{
				printf("MotionScan::scan_frame %d: save motion failed\n", __LINE__);
			}
		}
		
		if(do_rotate)
		{
			sprintf(string, 
				"%s%06d", 
				ROTATION_FILE, 
				source_position);
			FILE *output = fopen(string, "w");
			if(output)
			{
				fprintf(output, "%f\n", dr_result);
				fclose(output);
			}
			else
			{
				printf("MotionScan::scan_frame %d save rotation failed\n", __LINE__);
			}
		}
	}


	if(vertical_only) dx_result = 0;
	if(horizontal_only) dy_result = 0;

// printf("MotionScan::scan_frame %d dx=%d dy=%d\n", 
// __LINE__,
// this->dx_result,
// this->dy_result);
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
				difference *= difference; \
				result_temp += difference; \
			} \
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
			ABS_DIFF(unsigned char, int64_t, 1, 3)
			break;
		case BC_RGBA8888:
			ABS_DIFF(unsigned char, int64_t, 1, 4)
			break;
		case BC_RGB_FLOAT:
			ABS_DIFF(float, double, 0x10000, 3)
			break;
		case BC_RGBA_FLOAT:
			ABS_DIFF(float, double, 0x10000, 4)
			break;
		case BC_YUV888:
			ABS_DIFF(unsigned char, int64_t, 1, 3)
			break;
		case BC_YUVA8888:
			ABS_DIFF(unsigned char, int64_t, 1, 4)
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
		type *prev_row4 = (type*)(prev_ptr + current_row_bytes) + components; \
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
				difference *= difference; \
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
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 3)
			break;
		case BC_RGBA8888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 4)
			break;
		case BC_RGB_FLOAT:
			ABS_DIFF_SUB(float, double, 0x10000, 3)
			break;
		case BC_RGBA_FLOAT:
			ABS_DIFF_SUB(float, double, 0x10000, 4)
			break;
		case BC_YUV888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 3)
			break;
		case BC_YUVA8888:
			ABS_DIFF_SUB(unsigned char, int64_t, 1, 4)
			break;
	}
	return result;
}


#if 0
#define VARIANCE(type, temp_type, multiplier, components) \
{ \
	temp_type average[3] = { 0 }; \
	temp_type variance[3] = { 0 }; \
 \
	for(int i = 0; i < h; i++) \
	{ \
		type *row = (type*)current_ptr + i * current_row_bytes; \
		for(int j = 0; j < w; j++) \
		{ \
			for(int k = 0; k < 3; k++) \
			{ \
				average[k] += row[k]; \
			} \
			row += components; \
		} \
	} \
	for(int k = 0; k < 3; k++) \
	{ \
		average[k] /= w * h; \
	} \
 \
	for(int i = 0; i < h; i++) \
	{ \
		type *row = (type*)current_ptr + i * current_row_bytes; \
		for(int j = 0; j < w; j++) \
		{ \
			for(int k = 0; k < 3; k++) \
			{ \
				variance[k] += SQR(row[k] - average[k]); \
			} \
			row += components; \
 		} \
	} \
	result = (double)multiplier * \
		sqrt((variance[0] + variance[1] + variance[2]) / w / h / 3); \
}

double MotionScan::calculate_variance(unsigned char *current_ptr,
	int current_row_bytes,
	int w,
	int h,
	int color_model)
{
	double result = 0;

	switch(color_model)
	{
		case BC_RGB888:
			VARIANCE(unsigned char, int, 1, 3)
			break;
		case BC_RGBA8888:
			VARIANCE(unsigned char, int, 1, 4)
			break;
		case BC_RGB_FLOAT:
			VARIANCE(float, double, 255, 3)
			break;
		case BC_RGBA_FLOAT:
			VARIANCE(float, double, 255, 4)
			break;
		case BC_YUV888:
			VARIANCE(unsigned char, int, 1, 3)
			break;
		case BC_YUVA8888:
			VARIANCE(unsigned char, int, 1, 4)
			break;
	}


	return result;
}
#endif // 0




#define RANGE(type, temp_type, multiplier, components) \
{ \
	temp_type min[3]; \
	temp_type max[3]; \
	min[0] = 0x7fff; \
	min[1] = 0x7fff; \
	min[2] = 0x7fff; \
	max[0] = 0; \
	max[1] = 0; \
	max[2] = 0; \
 \
	for(int i = 0; i < h; i++) \
	{ \
/* printf("RANGE i=%d\n", i); */ \
		type *row = (type*)(current_ptr + i * current_row_bytes); \
		for(int j = 0; j < w; j++) \
		{ \
			for(int k = 0; k < 3; k++) \
			{ \
				if(row[k] > max[k]) max[k] = row[k]; \
				if(row[k] < min[k]) min[k] = row[k]; \
			} \
			row += components; \
		} \
	} \
 \
	for(int k = 0; k < 3; k++) \
	{ \
		/* printf("MotionScan::calculate_range %d k=%d max=%d min=%d\n", __LINE__, k, max[k], min[k]); */ \
		if(max[k] - min[k] > result) result = max[k] - min[k]; \
	} \
	result *= multiplier; \
 \
}

double MotionScan::calculate_range(unsigned char *current_ptr,
	int current_row_bytes,
	int w,
	int h,
	int color_model)
{
	double result = 0;

// printf("MotionScan::calculate_range %d row_bytes=%d current_ptr=%p w=%d h=%d color_model=%d\n", 
// __LINE__,
// row_bytes,
// current_ptr,
// w,
// h,
// color_model);
	switch(color_model)
	{
		case BC_RGB888:
			RANGE(unsigned char, int, 1, 3)
			break;
		case BC_RGBA8888:
			RANGE(unsigned char, int, 1, 4)
			break;
		case BC_RGB_FLOAT:
			RANGE(float, float, 255, 3)
			break;
		case BC_RGBA_FLOAT:
			RANGE(float, float, 255, 4)
			break;
		case BC_YUV888:
			RANGE(unsigned char, int, 1, 3)
			break;
		case BC_YUVA8888:
			RANGE(unsigned char, int, 1, 4)
			break;
	}


	return result;
}


//#define CLAMP_BLOCK

// this truncates the scan area but not the macroblock unless the macro is defined
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

	if(use_absolute)
	{
// Limit size of scan area
// Used for drawing vectors
// scan is always out of range before block.
		if(*scan_x1 < 0)
		{
			int difference = -*scan_x1;
#ifdef CLAMP_BLOCK
			*block_x1 += difference;
#endif
			*scan_x1 = 0;
		}

		if(*scan_y1 < 0)
		{
			int difference = -*scan_y1;
#ifdef CLAMP_BLOCK
			*block_y1 += difference;
#endif
			*scan_y1 = 0;
		}

		if(*scan_x2 > w)
		{
			int difference = *scan_x2 - w;
#ifdef CLAMP_BLOCK
			*block_x2 -= difference;
#endif
			*scan_x2 -= difference;
		}

		if(*scan_y2 > h)
		{
			int difference = *scan_y2 - h;
#ifdef CLAMP_BLOCK
			*block_y2 -= difference;
#endif
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
		if(*scan_x1 < 0)
		{
			int difference = -*scan_x1;
#ifdef CLAMP_BLOCK
			*block_x1 += difference;
#endif
//			*scan_x2 += difference;
			*scan_x1 = 0;
		}

		if(*scan_y1 < 0)
		{
			int difference = -*scan_y1;
#ifdef CLAMP_BLOCK
			*block_y1 += difference;
#endif
//			*scan_y2 += difference;
			*scan_y1 = 0;
		}

		if(*scan_x2 + *block_x2 - *block_x1 > w)
		{
			int difference = *scan_x2 + *block_x2 - *block_x1 - w;
			*scan_x2 -= difference;
#ifdef CLAMP_BLOCK
			*block_x2 -= difference;
#endif
		}

		if(*scan_y2 + *block_y2 - *block_y1 > h)
		{
			int difference = *scan_y2 + *block_y2 - *block_y1 - h;
			*scan_y2 -= difference;
#ifdef CLAMP_BLOCK
			*block_y2 -= difference;
#endif
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






TrackingType::TrackingType(int *output, 
    PluginClient *plugin, 
    BC_WindowBase *gui, 
    int x, 
    int y)
 : BC_PopupMenu(x, 
 	y, 
	calculate_w(gui),
	to_text(*output))
{
	this->output = output;
    this->plugin = plugin;
}

int TrackingType::handle_event()
{
	*output = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void TrackingType::create_objects()
{
	add_item(new BC_MenuItem(to_text(MotionScan::NO_CALCULATE)));
	add_item(new BC_MenuItem(to_text(MotionScan::CALCULATE)));
	add_item(new BC_MenuItem(to_text(MotionScan::SAVE)));
	add_item(new BC_MenuItem(to_text(MotionScan::LOAD)));
}

int TrackingType::from_text(const char *text)
{
	if(!strcmp(text, _("Don't Calculate"))) return MotionScan::NO_CALCULATE;
	if(!strcmp(text, _("Recalculate"))) return MotionScan::CALCULATE;
	if(!strcmp(text, _("Save coords to /tmp"))) return MotionScan::SAVE;
	if(!strcmp(text, _("Load coords from /tmp"))) return MotionScan::LOAD;
    return 0;
}

const char* TrackingType::to_text(int mode)
{
	switch(mode)
	{
		case MotionScan::NO_CALCULATE:
			return _("Don't Calculate");
			break;
		case MotionScan::CALCULATE:
			return _("Recalculate");
			break;
		case MotionScan::SAVE:
			return _("Save coords to /tmp");
			break;
		case MotionScan::LOAD:
			return _("Load coords from /tmp");
			break;
	}
    return 0;
}

int TrackingType::calculate_w(BC_WindowBase *gui)
{
	int result = 0;
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::NO_CALCULATE)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::CALCULATE)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::SAVE)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::LOAD)));
	return result + 50;
}




