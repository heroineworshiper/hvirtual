/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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



#ifndef HYPERLAPSE_H
#define HYPERLAPSE_H

#include "affine.inc"
#include "pluginvclient.h"

#include "opencv2/core/core_c.h"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;

class HyperlapseConfig
{
public:
	HyperlapseConfig();
	
	int equivalent(HyperlapseConfig &that);
	void copy_from(HyperlapseConfig &that);
	void interpolate(HyperlapseConfig &prev, 
		HyperlapseConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	void limits();
	int draw_vectors;
};

class Hyperlapse : public PluginVClient
{
public:
	Hyperlapse(PluginServer *server);
	~Hyperlapse();
// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS2(HyperlapseConfig)
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	void grey_crop(unsigned char *dst, 
		VFrame *src, 
		int x1, 
		int y1,
		int x2,
		int y2,
		int dst_w,
		int dst_h);

	CvMat accum_matrix;
	double accum_matrix_mem[9];
	long prev_position;
	long next_position;
	AffineEngine *affine;
	VFrame *temp;
	IplImage *prev_image;
	IplImage *next_image;
	IplImage *next_pyr;
	IplImage *prev_pyr;
	CvPoint2D32f *next_corners;
	CvPoint2D32f *prev_corners;
	double x_accum;
	double y_accum;
	double angle_accum;
};



#endif



