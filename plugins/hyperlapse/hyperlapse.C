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

#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "hyperlapse.h"
#include "hyperlapsewindow.h"
#include "language.h"

REGISTER_PLUGIN(Hyperlapse)


HyperlapseConfig::HyperlapseConfig()
{
}

int HyperlapseConfig::equivalent(HyperlapseConfig &that)
{
	return 1;
}

void HyperlapseConfig::copy_from(HyperlapseConfig &that)
{
}

void HyperlapseConfig::interpolate(
	HyperlapseConfig &prev, 
	HyperlapseConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
}

void HyperlapseConfig::limits()
{
}


Hyperlapse::Hyperlapse(PluginServer *server)
 : PluginVClient(server)
{
	prev_image = 0;
	next_image = 0;
	prev_position = -1;
	next_position = -1;
}

Hyperlapse::~Hyperlapse()
{
	if(prev_image) cvReleaseImage(&prev_image);
	if(next_image) cvReleaseImage(&next_image);
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
			thread->window->unlock_window();
		}
	}
}



int Hyperlapse::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	int need_reconfigure = load_configuration();
	int w = frame[0]->get_w();
	int h = frame[0]->get_h();


	if(!prev_image)
	{
// Only does greyscale
		prev_image = cvCreateImage( 
			cvSize(w, h), 
			8, 
			1);
	}

	if(!next_image)
	{
// Only does greyscale
		next_image = cvCreateImage( 
			cvSize(w, h), 
			8, 
			1);
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












