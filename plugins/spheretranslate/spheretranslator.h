
/*
 * CINELERRA
 * Copyright (C) 2017 Adam Williams <broadcast at earthling dot net>
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




#ifndef SPHERETRANSLATOR_H
#define SPHERETRANSLATOR_H

#include "loadbalance.h"
#include "spheretranslator.inc"
#include "vframe.inc"





class SphereTranslatePackage : public LoadPackage
{
public:
	SphereTranslatePackage();
	int row1, row2;
};


class SphereTranslateUnit : public LoadClient
{
public:
	SphereTranslateUnit(SphereTranslateEngine *engine);
	~SphereTranslateUnit();
	
	void rotate_to_matrix(float matrix[3][3], 
		float rotate_x, 
		float rotate_y, 
		float rotate_z);
	void multiply_pixel_matrix(float *pvf, float *pvi, float matrix[3][3]);
	void multiply_matrix_matrix(float dst[3][3], 
		float arg1[3][3], 
		float arg2[3][3]);

	void process_package(LoadPackage *package);
	void process_equirect(SphereTranslatePackage *pkg);
	void process_align(SphereTranslatePackage *pkg);
	double calculate_max_z(double a, double r);

	
	SphereTranslateEngine *engine;
};

class SphereTranslateEngine : public LoadServer
{
public:
	SphereTranslateEngine(int total_clients,
		int total_packages);
	~SphereTranslateEngine();
	
	
	void set_pivot(float x, float y);
	void set_x(float value);
	void set_y(float value);
	void set_z(float value);
	void process(VFrame *output, VFrame *input);
	void process(VFrame *output, 
		VFrame *input, 
		float rotate_x,
		float rotate_y,
		float rotate_z,
		float pivot_x,
		float pivot_y);
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	VFrame *input, *output;
	float rotate_x;
	float rotate_y;
	float rotate_z;
	float pivot_x;
	float pivot_y;
};








#endif




