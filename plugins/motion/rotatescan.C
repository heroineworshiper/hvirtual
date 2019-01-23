
/*
 * CINELERRA
 * Copyright (C) 2016 Adam Williams <broadcast at earthling dot net>
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
#include "motionscan.h"
#include "rotatescan.h"
#include "motion.h"
#include "mutex.h"
#include "vframe.h"









RotateScanPackage::RotateScanPackage()
{
}


RotateScanUnit::RotateScanUnit(RotateScan *server, MotionMain *plugin)
 : LoadClient(server)
{
	this->server = server;
	this->plugin = plugin;
	rotater = 0;
	temp = 0;
}

RotateScanUnit::~RotateScanUnit()
{
	delete rotater;
	delete temp;
}

void RotateScanUnit::process_package(LoadPackage *package)
{
	if(server->skip) return;
	RotateScanPackage *pkg = (RotateScanPackage*)package;

	if((pkg->difference = server->get_cache(pkg->angle)) < 0)
	{
//printf("RotateScanUnit::process_package %d\n", __LINE__);
		int color_model = server->previous_frame->get_color_model();
		int pixel_size = BC_CModels::calculate_pixelsize(color_model);
		int row_bytes = server->previous_frame->get_bytes_per_line();

		if(!rotater)
			rotater = new AffineEngine(1, 1);
		if(!temp) temp = new VFrame(0,
			-1,
			server->previous_frame->get_w(),
			server->previous_frame->get_h(),
			color_model,
			-1);
//printf("RotateScanUnit::process_package %d\n", __LINE__);


// Rotate original block size
// 		rotater->set_viewport(server->block_x1, 
// 			server->block_y1,
// 			server->block_x2 - server->block_x1,
// 			server->block_y2 - server->block_y1);
		rotater->set_in_viewport(server->block_x1, 
			server->block_y1,
			server->block_x2 - server->block_x1,
			server->block_y2 - server->block_y1);
		rotater->set_out_viewport(server->block_x1, 
			server->block_y1,
			server->block_x2 - server->block_x1,
			server->block_y2 - server->block_y1);
//		rotater->set_pivot(server->block_x, server->block_y);
		rotater->set_in_pivot(server->block_x, server->block_y);
		rotater->set_out_pivot(server->block_x, server->block_y);
//printf("RotateScanUnit::process_package %d\n", __LINE__);
		rotater->rotate(temp,
			server->previous_frame,
			pkg->angle);

// Scan reduced block size
//plugin->output_frame->copy_from(server->current_frame);
//plugin->output_frame->copy_from(temp);
// printf("RotateScanUnit::process_package %d %d %d %d %d\n", 
// __LINE__,
// server->scan_x,
// server->scan_y,
// server->scan_w,
// server->scan_h);
// Clamp coordinates
		int x1 = server->scan_x;
		int y1 = server->scan_y;
		int x2 = x1 + server->scan_w;
		int y2 = y1 + server->scan_h;
		x2 = MIN(temp->get_w(), x2);
		y2 = MIN(temp->get_h(), y2);
		x2 = MIN(server->current_frame->get_w(), x2);
		y2 = MIN(server->current_frame->get_h(), y2);
		x1 = MAX(0, x1);
		y1 = MAX(0, y1);

		if(x2 > x1 && y2 > y1)
		{
			pkg->difference = MotionScan::abs_diff(
				temp->get_rows()[y1] + x1 * pixel_size,
				server->current_frame->get_rows()[y1] + x1 * pixel_size,
				row_bytes,
				x2 - x1,
				y2 - y1,
				color_model);
//printf("RotateScanUnit::process_package %d\n", __LINE__);
			server->put_cache(pkg->angle, pkg->difference);
		}

// printf("RotateScanUnit::process_package 10 x=%d y=%d w=%d h=%d block_x=%d block_y=%d angle=%f scan_w=%d scan_h=%d diff=%lld\n", 
// server->block_x1, 
// server->block_y1,
// server->block_x2 - server->block_x1,
// server->block_y2 - server->block_y1,
// server->block_x,
// server->block_y,
// pkg->angle, 
// server->scan_w,
// server->scan_h,
// pkg->difference);
	}
}






















RotateScan::RotateScan(MotionMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(
//1, 1 
total_clients, total_packages 
)
{
	this->plugin = plugin;
	cache_lock = new Mutex("RotateScan::cache_lock");
}


RotateScan::~RotateScan()
{
	delete cache_lock;
}

void RotateScan::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		RotateScanPackage *pkg = (RotateScanPackage*)get_package(i);
		pkg->angle = i * 
			(scan_angle2 - scan_angle1) / 
			(total_steps - 1) + 
			scan_angle1;
	}
}

LoadClient* RotateScan::new_client()
{
	return new RotateScanUnit(this, plugin);
}

LoadPackage* RotateScan::new_package()
{
	return new RotateScanPackage;
}


float RotateScan::scan_frame(VFrame *previous_frame,
	VFrame *current_frame,
	int block_x,
	int block_y)
{
	skip = 0;
	this->block_x = block_x;
	this->block_y = block_y;

//printf("RotateScan::scan_frame %d\n", __LINE__);
	switch(plugin->config.tracking_type)
	{
		case MotionScan::NO_CALCULATE:
			result = plugin->config.rotation_center;
			skip = 1;
			break;

		case MotionScan::LOAD:
		{
			char string[BCTEXTLEN];
			sprintf(string, 
				"%s%06ld", 
				ROTATION_FILE, 
				plugin->get_source_position());
			FILE *input = fopen(string, "r");
			if(input)
			{
				int temp = fscanf(input, "%f", &result);
				fclose(input);
				skip = 1;
			}
			else
			{
				perror("RotateScan::scan_frame LOAD");
			}
			break;
		}
	}








	this->previous_frame = previous_frame;
	this->current_frame = current_frame;
	int w = current_frame->get_w();
	int h = current_frame->get_h();
	int block_w = w * plugin->config.global_block_w / 100;
	int block_h = h * plugin->config.global_block_h / 100;

	if(this->block_x - block_w / 2 < 0) block_w = this->block_x * 2;
	if(this->block_y - block_h / 2 < 0) block_h = this->block_y * 2;
	if(this->block_x + block_w / 2 > w) block_w = (w - this->block_x) * 2;
	if(this->block_y + block_h / 2 > h) block_h = (h - this->block_y) * 2;

	block_x1 = this->block_x - block_w / 2;
	block_x2 = this->block_x + block_w / 2;
	block_y1 = this->block_y - block_h / 2;
	block_y2 = this->block_y + block_h / 2;


// Calculate the maximum area available to scan after rotation.
// Must be calculated from the starting range because of cache.
// Get coords of rectangle after rotation.
	double center_x = this->block_x;
	double center_y = this->block_y;
	double max_angle = plugin->config.rotation_range;
	double base_angle1 = atan((float)block_h / block_w);
	double base_angle2 = atan((float)block_w / block_h);
	double target_angle1 = base_angle1 + max_angle * 2 * M_PI / 360;
	double target_angle2 = base_angle2 + max_angle * 2 * M_PI / 360;
	double radius = sqrt(block_w * block_w + block_h * block_h) / 2;
	double x1 = center_x - cos(target_angle1) * radius;
	double y1 = center_y - sin(target_angle1) * radius;
	double x2 = center_x + sin(target_angle2) * radius;
	double y2 = center_y - cos(target_angle2) * radius;
	double x3 = center_x - sin(target_angle2) * radius;
	double y3 = center_y + cos(target_angle2) * radius;

// Track top edge to find greatest area.
	double max_area1 = 0;
	double max_x1 = 0;
	double max_y1 = 0;
	for(double x = x1; x < x2; x++)
	{
		double y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
		if(x >= center_x && x < block_x2 && y >= block_y1 && y < center_y)
		{
			double area = fabs(x - center_x) * fabs(y - center_y);
			if(area > max_area1)
			{
				max_area1 = area;
				max_x1 = x;
				max_y1 = y;
			}
		}
	}

// Track left edge to find greatest area.
	double max_area2 = 0;
	double max_x2 = 0;
	double max_y2 = 0;
	for(double y = y1; y < y3; y++)
	{
		double x = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
		if(x >= block_x1 && x < center_x && y >= block_y1 && y < center_y)
		{
			double area = fabs(x - center_x) * fabs(y - center_y);
			if(area > max_area2)
			{
				max_area2 = area;
				max_x2 = x;
				max_y2 = y;
			}
		}
	}

	double max_x, max_y;
	max_x = max_x2;
	max_y = max_y1;

// Get reduced scan coords
	scan_w = (int)(fabs(max_x - center_x) * 2);
	scan_h = (int)(fabs(max_y - center_y) * 2);
	scan_x = (int)(center_x - scan_w / 2);
	scan_y = (int)(center_y - scan_h / 2);
// printf("RotateScan::scan_frame center=%d,%d scan=%d,%d %dx%d\n", 
// this->block_x, this->block_y, scan_x, scan_y, scan_w, scan_h);
// printf("    angle_range=%f block= %d,%d,%d,%d\n", max_angle, block_x1, block_y1, block_x2, block_y2);

// Determine min angle from size of block
	double angle1 = atan((double)block_h / block_w);
	double angle2 = atan((double)(block_h - 1) / (block_w + 1));
	double min_angle = fabs(angle2 - angle1) / OVERSAMPLE;
	min_angle = MAX(min_angle, MIN_ANGLE);

//printf("RotateScan::scan_frame %d min_angle=%f\n", __LINE__, min_angle * 360 / 2 / M_PI);

	cache.remove_all_objects();
	

	if(!skip)
	{
		if(previous_frame->data_matches(current_frame))
		{
//printf("RotateScan::scan_frame: frames match.  Skipping.\n");
			result = plugin->config.rotation_center;
			skip = 1;
		}
	}

	if(!skip)
	{
// Initial search range
		float angle_range = max_angle;
		result = plugin->config.rotation_center;
		total_steps = plugin->config.rotate_positions;


		while(angle_range >= min_angle * total_steps)
		{
			scan_angle1 = result - angle_range;
			scan_angle2 = result + angle_range;


			set_package_count(total_steps);
//set_package_count(1);
			process_packages();

			int64_t min_difference = -1;
			for(int i = 0; i < get_total_packages(); i++)
			{
				RotateScanPackage *pkg = (RotateScanPackage*)get_package(i);
				if(pkg->difference < min_difference || min_difference == -1)
				{
					min_difference = pkg->difference;
					result = pkg->angle;
				}
//break;
			}

			angle_range /= 2;

//break;
		}
	}

//printf("RotateScan::scan_frame %d\n", __LINE__);

	if(!skip && plugin->config.tracking_type == MotionScan::SAVE)
	{
		char string[BCTEXTLEN];
		sprintf(string, 
			"%s%06ld", 
			ROTATION_FILE, 
			plugin->get_source_position());
		FILE *output = fopen(string, "w");
		if(output)
		{
			fprintf(output, "%f\n", result);
			fclose(output);
		}
		else
		{
			perror("RotateScan::scan_frame SAVE");
		}
	}

//printf("RotateScan::scan_frame %d angle=%f\n", __LINE__, result);
	


	return result;
}

int64_t RotateScan::get_cache(float angle)
{
	int64_t result = -1;
	cache_lock->lock("RotateScan::get_cache");
	for(int i = 0; i < cache.total; i++)
	{
		RotateScanCache *ptr = cache.values[i];
		if(fabs(ptr->angle - angle) <= MIN_ANGLE)
		{
			result = ptr->difference;
			break;
		}
	}
	cache_lock->unlock();
	return result;
}

void RotateScan::put_cache(float angle, int64_t difference)
{
	RotateScanCache *ptr = new RotateScanCache(angle, difference);
	cache_lock->lock("RotateScan::put_cache");
	cache.append(ptr);
	cache_lock->unlock();
}









RotateScanCache::RotateScanCache(float angle, int64_t difference)
{
	this->angle = angle;
	this->difference = difference;
}



