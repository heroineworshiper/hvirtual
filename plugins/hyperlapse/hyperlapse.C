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

#include "affine.h"
#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "hyperlapse.h"
#include "hyperlapsewindow.h"
#include "language.h"
#include "transportque.inc"

#include "opencv2/calib3d/calib3d_c.h"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/video/tracking_c.h"
#include "opencv2/videostab/motion_core.hpp"
#include "opencv2/videostab/global_motion.hpp"

#include <vector>

REGISTER_PLUGIN(Hyperlapse)

#define MAX_COUNT 250
#define WIN_SIZE 20

using namespace videostab;
using namespace std;

HyperlapseConfig::HyperlapseConfig()
{
	draw_vectors = 0;
	do_stabilization = 1;
	block_size = 5;
	search_radius = 15;
	max_movement = 15;
	settling_speed = 10;
}

int HyperlapseConfig::equivalent(HyperlapseConfig &that)
{
	if(this->draw_vectors != that.draw_vectors ||
		this->do_stabilization != do_stabilization ||
		this->block_size != block_size ||
		this->search_radius != search_radius ||
		this->settling_speed != settling_speed ||
		this->max_movement != max_movement) return 0;
	return 1;
}

void HyperlapseConfig::copy_from(HyperlapseConfig &that)
{
	this->draw_vectors = that.draw_vectors;
	this->do_stabilization = that.do_stabilization;
	this->block_size = that.block_size;
	this->search_radius = that.search_radius;
	this->settling_speed = that.settling_speed;
	this->max_movement = that.max_movement;
}

void HyperlapseConfig::interpolate(
	HyperlapseConfig &prev, 
	HyperlapseConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	copy_from(next);
}

void HyperlapseConfig::limits()
{
	CLAMP(block_size, 5, 100);
	CLAMP(search_radius, 1, 100);
	CLAMP(settling_speed, 0, 100);
	CLAMP(max_movement, 1, 100);
}


Hyperlapse::Hyperlapse(PluginServer *server)
 : PluginVClient(server)
{
	prev_image = 0;
	next_image = 0;
	next_pyr = 0;
	prev_pyr = 0;
	next_corners = new CvPoint2D32f[ MAX_COUNT ];   
	prev_corners = new CvPoint2D32f[ MAX_COUNT ];       
	prev_position = -1;
	next_position = -1;
	affine = 0;
	temp = 0;
	x_accum = 0;
	y_accum = 0;
	angle_accum = 0;
}

Hyperlapse::~Hyperlapse()
{
	if(prev_image) cvReleaseImage(&prev_image);
	if(next_image) cvReleaseImage(&next_image);
	if(next_pyr) cvReleaseImage(&next_pyr);
	if(prev_pyr) cvReleaseImage(&prev_pyr);
	delete [] next_corners;
	delete [] prev_corners;
	if(affine) delete affine;
	if(temp) delete temp;
}

const char* Hyperlapse::plugin_title() { return N_("Hyperlapse"); }
int Hyperlapse::is_realtime() { return 1; }

NEW_WINDOW_MACRO(Hyperlapse, HyperlapseWindow);
LOAD_CONFIGURATION_MACRO(Hyperlapse, HyperlapseConfig)

void Hyperlapse::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("HYPERLAPSE");
	output.tag.set_property("DRAW_VECTORS", config.draw_vectors);
	output.tag.set_property("DO_STABILIZATION", config.do_stabilization);
	output.tag.set_property("BLOCK_SIZE", config.block_size);
	output.tag.set_property("SEARCH_RADIUS", config.search_radius);
	output.tag.set_property("SETTLING_SPEED", config.settling_speed);
	output.tag.set_property("MAX_MOVEMENT", config.max_movement);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/HYPERLAPSE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Hyperlapse::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("HYPERLAPSE"))
			{
				config.draw_vectors = input.tag.get_property("DRAW_VECTORS", config.draw_vectors);
				config.do_stabilization = input.tag.get_property("DO_STABILIZATION", config.do_stabilization);
				config.block_size = input.tag.get_property("BLOCK_SIZE", config.block_size);
				config.search_radius = input.tag.get_property("SEARCH_RADIUS", config.search_radius);
				config.max_movement = input.tag.get_property("MAX_MOVEMENT", config.max_movement);
				config.settling_speed = input.tag.get_property("SETTLING_SPEED", config.settling_speed);
				config.limits();
			
			}
			else
			if(input.tag.title_is("/HYPERLAPSE"))
			{
				result = 1;
			}
		}
	}

}

void Hyperlapse::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("Hyperlapse::update_gui");
			HyperlapseWindow *window = (HyperlapseWindow*)thread->window;
			window->vectors->update(config.draw_vectors);
			window->do_stabilization->update(config.do_stabilization);
			window->block_size->update(config.block_size);
			window->search_radius->update(config.search_radius);
			window->settling_speed->update(config.settling_speed);
			
			thread->window->unlock_window();
		}
	}
}



int Hyperlapse::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{

	int need_reconfigure = load_configuration();
	int w = get_input(0)->get_w();
	int h = get_input(0)->get_h();
	int color_model = get_input(0)->get_color_model();
    double identity_matrix[9]={1,0,0, 0,1,0, 0,0,1};

// initialize everything
	if(!prev_image)
	{
// Only does greyscale
		prev_image = cvCreateImage( 
			cvSize(w, h), 
			8, 
			1);
		next_image = cvCreateImage( 
			cvSize(w, h), 
			8, 
			1);
		CvSize pyr_sz = cvSize( w + 8, h / 3 );     
        next_pyr = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1);     
        prev_pyr = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1);     

		memcpy(accum_matrix_mem, identity_matrix, 9 * sizeof(double));
     	accum_matrix = cvMat(3, 3, CV_64F, accum_matrix_mem);  


		affine = new AffineEngine(PluginClient::get_project_smp() + 1,
			PluginClient::get_project_smp() + 1);
		
		temp = new VFrame(0,
			-1,
			w,
			h,
			color_model,
			-1);
		
	}


// Get the position of previous reference frame.
	int64_t actual_previous_number;
	int skip_current = 0;
	actual_previous_number = start_position;
	int step = 1;
	if(get_direction() == PLAY_REVERSE)
	{
		step = -1;
		actual_previous_number++;
		if(actual_previous_number >= get_source_start() + get_total_len())
		{
			skip_current = 1;
		}
		else
		{
			KeyFrame *keyframe = get_next_keyframe(start_position, 1);
			if(keyframe->position > 0 &&
				actual_previous_number >= keyframe->position)
				skip_current = 1;
		}
	}
	else
	{
		actual_previous_number--;
		if(actual_previous_number < get_source_start())
		{
			skip_current = 1;
		}
		else
		{
			KeyFrame *keyframe = get_prev_keyframe(start_position, 1);
			if(keyframe->position > 0 &&
				actual_previous_number < keyframe->position)
				skip_current = 1;
		}
	}
	

// move current image to previous position
	if(next_position >= 0 && next_position == actual_previous_number)
	{
		IplImage *temp = prev_image;
		prev_image = next_image;
		next_image = temp;
		prev_position = next_position;
	}
	else
// load previous image
	if(actual_previous_number >= 0)
	{
//printf("Hyperlapse::process_buffer %d\n", __LINE__);
		read_frame(get_input(0), 
			0, 
			actual_previous_number, 
			frame_rate);
		grey_crop((unsigned char*)prev_image->imageData, 
			get_input(0), 
			0, 
			0, 
			w, 
			h,
			w,
			h);

	}

// reset the accumulator
// printf("Hyperlapse::process_buffer %d %d %lld %lld %lld\n", 
// __LINE__,
// skip_current,
// prev_position,
// actual_previous_number,
// start_position);
	if(skip_current || prev_position != actual_previous_number)
	{
		skip_current = 1;
// reset the accumulator
		memcpy(accum_matrix_mem, identity_matrix, 9 * sizeof(double));
	}


// load next image
	next_position = start_position;
	VFrame *input_frame = get_input(0);
	if(config.do_stabilization) input_frame = temp;
//printf("Hyperlapse::process_buffer %d %d\n", __LINE__, start_position);
	read_frame(input_frame, 
		0, 
		start_position, 
		frame_rate);
	grey_crop((unsigned char*)next_image->imageData, 
		input_frame, 
		0, 
		0, 
		w, 
		h,
		w,
		h);

	int corner_count = MAX_COUNT;
    char features_found[MAX_COUNT];     
    float feature_errors[MAX_COUNT];
//	int block_size = MIN(w, h) * config.block_size / 100;
	int block_size = config.block_size;
printf("Hyperlapse::process_buffer %d block_size=%d\n", __LINE__, block_size);

    cvGoodFeaturesToTrack(next_image, 
		0, 
		0, 
		next_corners,  // corners
		&corner_count,  // corner_count
		0.01,  // quality_level
		block_size,  // min_distance
		0,    // mask
		block_size,    // block_size
		0,    // use_harris
		0.04);     // k

	cvFindCornerSubPix(next_image, 
		next_corners, 
		corner_count, 
		cvSize(WIN_SIZE, WIN_SIZE), 
		cvSize(-1, -1), 
		cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 
			20, 
			0.03));        


// optical flow

	cvCalcOpticalFlowPyrLK(next_image, 
		prev_image, 
		next_pyr, 
		prev_pyr, 
		next_corners, 
		prev_corners, 
		corner_count, 
		cvSize(WIN_SIZE, WIN_SIZE), 
		5, 
		features_found, 
		feature_errors, 
		cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 
			20, 
			0.3), 
		0);

    int fCount=0;  
    for(int i = 0; i < corner_count; ++i)  
    {
        if(features_found[i] == 0 || feature_errors[i] > MAX_COUNT)  
        	continue;  

        fCount++;  
    }

// printf("Hyperlapse::process_buffer %d fCount=%d draw_vectors=%d\n", 
// __LINE__, 
// fCount,
// config.draw_vectors);
	if(fCount > 0 && !skip_current)
	{
    	int inI = 0;  
    	CvPoint2D32f *pt1 = new CvPoint2D32f[fCount];  
    	CvPoint2D32f *pt2 = new CvPoint2D32f[fCount];  
    	for(int i = 0; i < corner_count; ++i)  
    	{
    		if(features_found[i] == 0 || feature_errors[i] > MAX_COUNT)
    			continue;  

    		pt1[inI] = next_corners[i];  
    		pt2[inI] = prev_corners[i];  

			if(config.draw_vectors) 
			{
				input_frame->draw_arrow(
					pt2[inI].x, 
					pt2[inI].y, 
					pt1[inI].x, 
					pt1[inI].y);
			}
    		inI++;  
    	}
// printf("Hyperlapse::process_buffer %d fCount=%d corner_count=%d inI=%d\n", 
// __LINE__, 
// fCount, 
// corner_count, 
// inI);

// 2D motion
#if 0
		MotionModel model = MM_TRANSLATION;
		RansacParams params = RansacParams::default2dMotion(model);
		int ninliers = 0;
		vector<Point2f> points1;
		vector<Point2f> points2;

		for(int i = 0; i < corner_count; i++)
		{
			points1.push_back(Point2f(pt1[i].x, pt1[i].y));
			points2.push_back(Point2f(pt2[i].x, pt2[i].y));
		}

		Mat_<float> translationM = estimateGlobalMotionRansac(
        	points1, 
			points2, 
			model, 
			params,
        	0, 
			&ninliers);

		model = MM_ROTATION;
		params = RansacParams::default2dMotion(model);
		Mat_<float> rotationM = estimateGlobalMotionRansac(
			points2, 
        	points1, 
			model, 
			params,
        	0, 
			&ninliers);


		double temp[9];
		CvMat temp_matrix = cvMat(3, 3, CV_64F, temp);
		for(int i = 0; i < 9; i++)
		{
			if(i == 2 || i == 5)
				temp[i] = translationM(i / 3, i % 3);
			else
				temp[i] = rotationM(i / 3, i % 3);
		}
		cvMatMul(&accum_matrix, &temp_matrix, &accum_matrix);
		


#endif // 0


// homography
#if 1
    	CvMat M1, M2;  
    	double H[9] = { 1,0,0, 0,1,0, 0,0,1 };
    	CvMat mxH = cvMat(3, 3, CV_64F, H);  
    	M1 = cvMat(1, fCount, CV_32FC2, pt1);  
    	M2 = cvMat(1, fCount, CV_32FC2, pt2);  

//M2 = H*M1 , old = H*current  
    	if(!cvFindHomography(&M1, 
			&M2, 
			&mxH, 
			CV_RANSAC, 
			2))
    	{
printf("Hyperlapse::process_buffer %d: Find Homography Fail!\n", __LINE__);  
    	}
		else
		{
// printf("Hyperlapse::process_buffer %d mxH=\n%f %f %f\n%f %f %f\n%f %f %f\n", 
// __LINE__,
// mxH.data.db[0],
// mxH.data.db[1],
// mxH.data.db[2],
// mxH.data.db[3],
// mxH.data.db[4],
// mxH.data.db[5],
// mxH.data.db[6],
// mxH.data.db[7],
// mxH.data.db[8]);
  			cvMatMul(&accum_matrix, &mxH, &accum_matrix);
    	}
#endif // 0

		

    	delete [] pt1;  
    	delete [] pt2; 	 

	}

// deglitch
// 	if(EQUIV(accum_matrix.data.db[0], 0))
// 	{
// printf("Hyperlapse::process_buffer %d\n", __LINE__);
//     	double gH[9]={1,0,0, 0,1,0, 0,0,1};  
//      	for(int i = 0; i < 9; i++)
// 			accum_matrix.data.db[i] = gH[i];
// 	}


	if(config.do_stabilization)
	{
		AffineMatrix matrix;
		double *src = accum_matrix.data.db;
	// interpolate with identity matrix
 		for(int i = 0; i < 9; i++)
 		{
 			src[i] = src[i] * (100 - config.settling_speed) / 100 + 
				identity_matrix[i] * config.settling_speed / 100;
 		}

		for(int i = 0; i < 3; i++)
		{
			for(int j = 0; j < 3; j++)
			{
				matrix.values[i][j] = src[i * 3 + j];
			}
		}

	//printf("Hyperlapse::process_buffer %d %d matrix=\n", __LINE__, start_position);
	//matrix.dump();

		affine->set_matrix(&matrix);
// input_frame is always temp, if we get here
		get_input(0)->clear_frame();
		affine->process(get_input(0),
			temp, 
			0,
			AffineEngine::TRANSFORM,
			0, 
			0, 
			w, 
			0, 
			w, 
			h, 
			0, 
			h,
			1);
	}


	return 0;
}


// Convert to greyscale & crop for OpenCV
void Hyperlapse::grey_crop(unsigned char *dst, 
	VFrame *src, 
	int x1, 
	int y1,
	int x2,
	int y2,
	int dst_w,
	int dst_h)
{
// Dimensions of dst frame
	int w = x2 - x1;
	int h = y2 - y1;

	bzero(dst, dst_w * dst_h);

//printf("FindObjectMain::grey_crop %d %d %d\n", __LINE__, w, h);
	for(int i = 0; i < h; i++)
	{

#define RGB_TO_VALUE(r, g, b) \
((r) * CMODEL_R_TO_Y + (g) * CMODEL_G_TO_Y + (b) * CMODEL_B_TO_Y)


#define CONVERT(in_type, max, components, is_yuv) \
{ \
	in_type *input = ((in_type*)src->get_rows()[i + y1]) + x1 * components; \
	unsigned char *output = dst + i * dst_w; \
 \
	for(int j = 0; j < w; j++) \
	{ \
/* Y channel only */ \
		if(is_yuv) \
		{ \
			*output = *input; \
		} \
/* RGB */ \
		else \
		{ \
			float r = (float)input[0] / max; \
			float g = (float)input[1] / max; \
			float b = (float)input[2] / max; \
			*output = RGB_TO_VALUE(r, g, b); \
		} \
 \
		input += components; \
		output++; \
	} \
}
		switch(src->get_color_model())
		{
			case BC_RGB888:
			{
				CONVERT(unsigned char, 0xff, 3, 0)
				break;
			}

			case BC_RGBA8888:
			{
				CONVERT(unsigned char, 0xff, 4, 0)
				break;
			}

			case BC_RGB_FLOAT:
			{
				CONVERT(float, 1.0, 3, 0)
				break;
			}

			case BC_RGBA_FLOAT:
			{
				CONVERT(float, 1.0, 4, 0)
				break;
			}

			case BC_YUV888:
 			{
				CONVERT(unsigned char, 0xff, 3, 1)
				break;
			}

			case BC_YUVA8888:
			{
				CONVERT(unsigned char, 0xff, 4, 1)
				break;
			}
		}
	}
}












