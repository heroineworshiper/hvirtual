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

REGISTER_PLUGIN(Hyperlapse)

#define MAX_COUNT 250
#define WIN_SIZE 20


HyperlapseConfig::HyperlapseConfig()
{
	draw_vectors = 0;
}

int HyperlapseConfig::equivalent(HyperlapseConfig &that)
{
	if(this->draw_vectors != that.draw_vectors) return 0;
	return 1;
}

void HyperlapseConfig::copy_from(HyperlapseConfig &that)
{
	this->draw_vectors = that.draw_vectors;
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

     	double gH[9]={1,0,0, 0,1,0, 0,0,1};  
     	gmxH = cvMat(3, 3, CV_64F, gH);  
		
		affine = new AffineEngine(PluginClient::get_project_smp() + 1,
			PluginClient::get_project_smp() + 1);
		
		temp = new VFrame(0,
			-1,
			w,
			h,
			color_model,
			-1);
		
	}


	int step = 1;
	if(get_direction() == PLAY_REVERSE)
		step = -1;

// move currrent image to previous position
	if(next_position >= 0 && next_position == start_position - step)
	{
		IplImage *temp = prev_image;
		prev_image = next_image;
		next_image = temp;
	}
	else
// load previous image
	if(start_position - step >= 0)
	{
//printf("Hyperlapse::process_buffer %d\n", __LINE__);
		read_frame(get_input(0), 
			0, 
			start_position - step, 
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
	
// load next image
	next_position = start_position;
//printf("Hyperlapse::process_buffer %d %d\n", __LINE__, start_position);
	read_frame(get_input(0), 
		0, 
		start_position, 
		frame_rate);
	grey_crop((unsigned char*)next_image->imageData, 
		get_input(0), 
		0, 
		0, 
		w, 
		h,
		w,
		h);

//printf("Hyperlapse::process_buffer %d\n", __LINE__);
//for(int  i = 0; i < 256; i++) printf("%02x ", (unsigned char)next_image->imageData[i]);
//printf("\n");
	int corner_count = MAX_COUNT;
    char features_found[MAX_COUNT];     
    float feature_errors[MAX_COUNT];     
    cvGoodFeaturesToTrack(next_image, 
		0, 
		0, 
		next_corners,  // corners
		&corner_count,  // corner_count
		0.01,  // quality_level
		5.0,  // min_distance
		0,    // mask
		3,    // block_size
		0,    // use_harris
		0.04);     // k

//printf("Hyperlapse::process_buffer %d corner_count=%d\n", __LINE__, corner_count);
	cvFindCornerSubPix(next_image, 
		next_corners, 
		corner_count, 
		cvSize(WIN_SIZE, WIN_SIZE), 
		cvSize(-1, -1), 
		cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 
			20, 
			0.03));        
//printf("Hyperlapse::process_buffer %d\n", __LINE__);


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
//printf("Hyperlapse::process_buffer %d\n", __LINE__);

    int fCount=0;  
    for(int i = 0; i < corner_count; ++i)  
    {
        if(features_found[i] == 0 || feature_errors[i] > MAX_COUNT)  
        	continue;  

        fCount++;  
    }

//printf("Hyperlapse::process_buffer %d fCount=%d\n", __LINE__, fCount);
	if(fCount > 0)
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

//			get_input(0)->draw_arrow(pt2[inI].x, pt2[inI].y, pt1[inI].x, pt1[inI].y);
    		inI++;  
    	}  
printf("Hyperlapse::process_buffer %d fCount=%d corner_count=%d inI=%d\n", 
__LINE__, 
fCount, 
corner_count, 
inI);

// find homography
    	CvMat M1, M2;  
    	double H[9];  
    	CvMat mxH = cvMat(3, 3, CV_64F, H);  
    	M1 = cvMat(1, fCount, CV_32FC2, pt1);  
    	M2 = cvMat(1, fCount, CV_32FC2, pt2);  
//printf("Hyperlapse::process_buffer %d\n", __LINE__);

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
			//printf(" %lf %lf %lf \n %lf %lf %lf \n %lf %lf %lf\n", H[0], H[1], H[2], H[3], H[4], H[5], H[6], H[7], H[8] );  
    	}  
//printf("Hyperlapse::process_buffer %d\n", __LINE__);

    	delete [] pt1;  
    	delete [] pt2; 	 

  		cvMatMul(&gmxH, &mxH, &gmxH);   
//printf("Hyperlapse::process_buffer %d\n", __LINE__);
	}

//printf("Hyperlapse::process_buffer %d\n", __LINE__);
	AffineMatrix matrix;
	double *src = gmxH.data.db;
	for(int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			matrix.values[i][j] = src[i * 3 + j];
		}
	}
	affine->set_matrix(&matrix);
	temp->copy_from(get_input(0));
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
	
	

// // output of warping by H goes here
//     IplImage* WarpImg = cvCreateImage(
// 		cvSize(w, h), 
// 		8, 
// 		3);
// 
// // Ma*Mb   -> Mc  
//     cvWarpPerspective(current_Img, WarpImg, &gmxH);   
// 
// 	cvReleaseImage(&WarpImg);


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












