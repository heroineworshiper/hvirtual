
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




#ifndef ROTATESCAN_H
#define ROTATESCAN_H



#include "affine.inc"
#include "loadbalance.h"
#include "motion.inc"
#include "vframe.inc"
#include <stdint.h>

class RotateScan;


class RotateScanPackage : public LoadPackage
{
public:
	RotateScanPackage();
	float angle;
	int64_t difference;
};

class RotateScanCache
{
public:
	RotateScanCache(float angle, int64_t difference);
	float angle;
	int64_t difference;
};

class RotateScanUnit : public LoadClient
{
public:
	RotateScanUnit(RotateScan *server, MotionMain *plugin);
	~RotateScanUnit();

	void process_package(LoadPackage *package);

	RotateScan *server;
	MotionMain *plugin;
	AffineEngine *rotater;
	VFrame *temp;
};

class RotateScan : public LoadServer
{
public:
	RotateScan(MotionMain *plugin, 
		int total_clients, 
		int total_packages);
	~RotateScan();

	friend class RotateScanUnit;

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

// Invoke the motion engine for a search
// Frame before rotation
	float scan_frame(VFrame *previous_frame,
// Frame after rotation
		VFrame *current_frame,
// Pivot
		int block_x,
		int block_y);
	int64_t get_cache(float angle);
	void put_cache(float angle, int64_t difference);


// Angle result
	float result;

private:
	VFrame *previous_frame;
// Frame after motion
	VFrame *current_frame;

	MotionMain *plugin;
	int skip;

// Pivot
	int block_x;
	int block_y;
// Block to rotate
	int block_x1;
	int block_x2;
	int block_y1;
	int block_y2;
// Area to compare
	int scan_x;
	int scan_y;
	int scan_w;
	int scan_h;
// Range of angles to compare
	float scan_angle1, scan_angle2;
	int total_steps;

	ArrayList<RotateScanCache*> cache;
	Mutex *cache_lock;
};





#endif





