/*
 * CINELERRA
 * Copyright (C) 2008-2025 Adam Williams <broadcast at earthling dot net>
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
#include "condition.h"
#include "clip.h"
#include "maskauto.h"
#include "maskautos.h"
#include "maskengine.h"
#include "mutex.h"
#include "transportque.inc"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

MaskPackage::MaskPackage()
{
}

MaskPackage::~MaskPackage()
{
}







MaskUnit::MaskUnit(MaskEngine *engine)
 : LoadClient(engine)
{
	this->engine = engine;
	this->temp = 0;
    this->temp2 = 0;
}


MaskUnit::~MaskUnit()
{
	if(temp) delete temp;
    if(temp2) delete temp2;
}


#define OVERSAMPLE 8
















void MaskUnit::draw_line_clamped(VFrame *frame, 
	int &x1, 
	int &y1, 
	int x2, 
	int y2,
	unsigned char pixel_value)
{
	int draw_x1;
	int draw_y1;
	int draw_x2;
	int draw_y2;
    int radius = (int)(engine->radius * OVERSAMPLE);
    int need_radius = (engine->mode == MASK_MULTIPLY_PATH ||
        engine->mode == MASK_SUBTRACT_PATH);

	if(y2 < y1)
	{
		draw_x1 = x2;
		draw_y1 = y2;
		draw_x2 = x1;
		draw_y2 = y1;
	}
	else
	{
		draw_x1 = x1;
		draw_y1 = y1;
		draw_x2 = x2;
		draw_y2 = y2;
	}

	unsigned char **rows = (unsigned char**)frame->get_rows();
	int w = frame->get_w() - 1;
	int h = frame->get_h();

	if(draw_y2 != draw_y1)
	{
		float slope = ((float)draw_x2 - draw_x1) / ((float)draw_y2 - draw_y1);
//printf("MaskUnit::draw_line_clamped %d slope=%f\n", __LINE__, slope);

		for(float y = draw_y1; y < draw_y2; y++)
		{
			int x_i = (int)((y - draw_y1) * slope + draw_x1);
			int y_i = (int)y;

            if(need_radius)
            {
			    int x2_i = (int)(((y + 1) - draw_y1) * slope + draw_x1);
                if(x2_i < x_i)
                {
                    int temp = x2_i;
                    x2_i = x_i;
                    x_i = temp;
                }
                
                for(int y3 = y_i - radius; y3 < y_i + radius; y3++)
                {
                    if(y3 >= 0 && y3 < h)
                    {
                        for(int x3 = x_i - radius; x3 < x2_i + radius; x3++)
                        {
                            if(x3 >= 0 && x3 < w)
                            {
                                rows[y3][x3] = pixel_value;
                            }
                        }
                    }
                }
            }
            else
            if(y_i >= 0 && y_i < h)
            {
                CLAMP(x_i, 0, w - 1);
 				if(rows[y_i][x_i] == pixel_value)
 					rows[y_i][x_i] = 0;
 				else
					rows[y_i][x_i] = pixel_value;
            }
		}
	}
    else
    if(need_radius)
    {
// horizontal line
        for(int y3 = y1 - radius; y3 < y1 + radius; y3++)
        {
//printf("MaskUnit::draw_line_clamped %d y3=%d\n", __LINE__, y3);
            if(y3 >= 0 && y3 < h)
            {
                if(draw_x2 < draw_x1)
                {
                    draw_x2 = x1;
                    draw_x1 = x2;
                }

                for(int x3 = draw_x1 - radius; x3 < draw_x2 + radius; x3++)
                {
                    if(x3 >= 0 && x3 < w)
                    {
                        rows[y3][x3] = pixel_value;
                    }
                }
            }
        }
    }
}

void MaskUnit::blur_strip(double *val_p, 
	double *val_m, 
	double *dst, 
	double *src, 
	int size,
	int max)
{
	double *sp_p = src;
	double *sp_m = src + size - 1;
	double *vp = val_p;
	double *vm = val_m + size - 1;
	double initial_p = sp_p[0];
	double initial_m = sp_m[0];

//printf("MaskUnit::blur_strip %d\n", size);
	for(int k = 0; k < size; k++)
	{
		int terms = (k < 4) ? k : 4;
		int l;
		for(l = 0; l <= terms; l++)
		{
			*vp += n_p[l] * sp_p[-l] - d_p[l] * vp[-l];
			*vm += n_m[l] * sp_m[l] - d_m[l] * vm[l];
		}

		for( ; l <= 4; l++)
		{
			*vp += (n_p[l] - bd_p[l]) * initial_p;
			*vm += (n_m[l] - bd_m[l]) * initial_m;
		}
		sp_p++;
		sp_m--;
		vp++;
		vm--;
	}

	for(int i = 0; i < size; i++)
	{
		double sum = val_p[i] + val_m[i];
		CLAMP(sum, 0, max);
		dst[i] = sum;
	}
}

void MaskUnit::do_feather(VFrame *output,
	VFrame *input, 
	double feather, 
	int start_y, 
	int end_y, 
	int start_x, 
	int end_x)
{
//printf("MaskUnit::do_feather %f\n", feather);
// Get constants
	double constants[8];
	double div;
	double std_dev = sqrt(-(double)(feather * feather) / (2 * log(1.0 / 255.0)));
	div = sqrt(2 * M_PI) * std_dev;
	constants[0] = -1.783 / std_dev;
	constants[1] = -1.723 / std_dev;
	constants[2] = 0.6318 / std_dev;
	constants[3] = 1.997  / std_dev;
	constants[4] = 1.6803 / div;
	constants[5] = 3.735 / div;
	constants[6] = -0.6803 / div;
	constants[7] = -0.2598 / div;

	n_p[0] = constants[4] + constants[6];
	n_p[1] = exp(constants[1]) *
				(constants[7] * sin(constants[3]) -
				(constants[6] + 2 * constants[4]) * cos(constants[3])) +
				exp(constants[0]) *
				(constants[5] * sin(constants[2]) -
				(2 * constants[6] + constants[4]) * cos(constants[2]));

	n_p[2] = 2 * exp(constants[0] + constants[1]) *
				((constants[4] + constants[6]) * cos(constants[3]) * 
				cos(constants[2]) - constants[5] * 
				cos(constants[3]) * sin(constants[2]) -
				constants[7] * cos(constants[2]) * sin(constants[3])) +
				constants[6] * exp(2 * constants[0]) +
				constants[4] * exp(2 * constants[1]);

	n_p[3] = exp(constants[1] + 2 * constants[0]) *
				(constants[7] * sin(constants[3]) - 
				constants[6] * cos(constants[3])) +
				exp(constants[0] + 2 * constants[1]) *
				(constants[5] * sin(constants[2]) - constants[4] * 
				cos(constants[2]));
	n_p[4] = 0.0;

	d_p[0] = 0.0;
	d_p[1] = -2 * exp(constants[1]) * cos(constants[3]) -
				2 * exp(constants[0]) * cos(constants[2]);

	d_p[2] = 4 * cos(constants[3]) * cos(constants[2]) * 
				exp(constants[0] + constants[1]) +
				exp(2 * constants[1]) + exp (2 * constants[0]);

	d_p[3] = -2 * cos(constants[2]) * exp(constants[0] + 2 * constants[1]) -
				2 * cos(constants[3]) * exp(constants[1] + 2 * constants[0]);

	d_p[4] = exp(2 * constants[0] + 2 * constants[1]);

	for(int i = 0; i < 5; i++) d_m[i] = d_p[i];

	n_m[0] = 0.0;
	for(int i = 1; i <= 4; i++)
		n_m[i] = n_p[i] - d_p[i] * n_p[0];

	double sum_n_p, sum_n_m, sum_d;
	double a, b;

	sum_n_p = 0.0;
	sum_n_m = 0.0;
	sum_d = 0.0;
	for(int i = 0; i < 5; i++)
	{
		sum_n_p += n_p[i];
		sum_n_m += n_m[i];
		sum_d += d_p[i];
	}

	a = sum_n_p / (1 + sum_d);
	b = sum_n_m / (1 + sum_d);

	for(int i = 0; i < 5; i++)
	{
		bd_p[i] = d_p[i] * a;
		bd_m[i] = d_m[i] * b;
	}






















#define DO_FEATHER(type, max) \
{ \
	int frame_w = input->get_w(); \
	int frame_h = input->get_h(); \
	int size = MAX(frame_w, frame_h); \
	double *src = new double[size]; \
	double *dst = new double[size]; \
	double *val_p = new double[size]; \
	double *val_m = new double[size]; \
	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
	int j; \
 \
/* printf("DO_FEATHER 1\n"); */ \
	if(end_x > start_x) \
	{ \
		for(j = start_x; j < end_x; j++) \
		{ \
	/* printf("DO_FEATHER 1.1 %d\n", j); */ \
			bzero(val_p, sizeof(double) * frame_h); \
			bzero(val_m, sizeof(double) * frame_h); \
			for(int k = 0; k < frame_h; k++) \
			{ \
				src[k] = (double)in_rows[k][j]; \
			} \
	 \
			blur_strip(val_p, val_m, dst, src, frame_h, max); \
	 \
			for(int k = 0; k < frame_h; k++) \
			{ \
				out_rows[k][j] = (type)dst[k]; \
			} \
		} \
	} \
 \
 	if(end_y > start_y) \
	{ \
		for(j = start_y; j < end_y; j++) \
		{ \
	/* printf("DO_FEATHER 2 %d\n", j); */ \
			bzero(val_p, sizeof(double) * frame_w); \
			bzero(val_m, sizeof(double) * frame_w); \
			for(int k = 0; k < frame_w; k++) \
			{ \
				src[k] = (double)out_rows[j][k]; \
			} \
	 \
			blur_strip(val_p, val_m, dst, src, frame_w, max); \
	 \
			for(int k = 0; k < frame_w; k++) \
			{ \
				out_rows[j][k] = (type)dst[k]; \
			} \
		} \
	} \
 \
/* printf("DO_FEATHER 3\n"); */ \
 \
	delete [] src; \
	delete [] dst; \
	delete [] val_p; \
	delete [] val_m; \
/* printf("DO_FEATHER 4\n"); */ \
}








//printf("do_feather %d\n", frame->get_color_model());
	switch(input->get_color_model())
	{
		case BC_A8:
			DO_FEATHER(unsigned char, 0xff);
			break;

		case BC_A_FLOAT:
			DO_FEATHER(float, 1);
			break;
	}




}

void MaskUnit::process_package(LoadPackage *package)
{
	MaskPackage *ptr = (MaskPackage*)package;

	if(engine->recalculate && 
		engine->step == DO_MASK)
	{
		VFrame *mask;
		if(engine->feather > 0) 
		{
        	mask = engine->temp_mask;
		}
        else
		{
        	mask = engine->mask;
        }

// Generated oversampling frame
		int mask_w = mask->get_w();
		int mask_h = mask->get_h();
		int oversampled_package_w = mask_w * OVERSAMPLE;
		int oversampled_package_h = (ptr->end_y - ptr->start_y) * OVERSAMPLE;
//printf("MaskUnit::process_package 1\n");

		if(temp && 
			(temp->get_w() != oversampled_package_w ||
			temp->get_h() != oversampled_package_h))
		{
			delete temp;
			temp = 0;
		}
//printf("MaskUnit::process_package 1\n");

		if(!temp)
		{
			temp = new VFrame(0, 
				-1,
				oversampled_package_w, 
				oversampled_package_h,
				BC_A8,
				-1);
		}

		temp->clear_frame();



// printf("MaskUnit::process_package %d point_sets=%d\n", 
// __LINE__, 
// engine->point_sets.size());


// Draw oversampled region of polygons on temp
		for(int k = 0; k < engine->point_sets.size(); k++)
		{
			int old_x, old_y;
// value to assign to pixels in this point set.
// Each point set gets a different value.
			unsigned char pixel_value = k + 1;
			ArrayList<MaskPoint*> *points = engine->point_sets.values[k];
            int total_points = points->size();
//printf("MaskUnit::process_package %d k=%d total_points=%d\n", __LINE__, k, total_points);

// need enough points to make a closed polygon
			if(total_points < 2 && 
                (engine->mode == MASK_MULTIPLY_ALPHA ||
                    engine->mode == MASK_SUBTRACT_ALPHA))
            {
// next point set
                continue;
            }
            else
// make an open polygon
            if((engine->mode == MASK_MULTIPLY_PATH ||
                engine->mode == MASK_SUBTRACT_PATH ||
                engine->mode == MASK_NONE))
            {
// need enough points to make a line
                if(total_points < 2)
                {
// next point set
                    continue;
                }

                total_points--;
            }
            

//printf("MaskUnit::process_package %d k=%d total_points=%d\n", __LINE__, k, total_points);
			for(int i = 0; i < total_points; i++)
			{
				MaskPoint *point1 = points->get(i);
				MaskPoint *point2 = 0;
                
                if((engine->mode == MASK_MULTIPLY_PATH ||
                    engine->mode == MASK_SUBTRACT_PATH))
                {
                    point2 = points->get(i + 1);
                }
                else
                {
                    point2 = (i >= total_points - 1) ? 
					    points->get(0) : 
					    points->get(i + 1);
                }

				float x, y;
				int segments = (int)(sqrt(SQR(point1->x - point2->x) + 
                    SQR(point1->y - point2->y)));
				if(point1->control_x2 == 0 &&
					point1->control_y2 == 0 &&
					point2->control_x1 == 0 &&
					point2->control_y1 == 0)
				{
                	segments = 1;
                }
                
//printf("MaskUnit::process_package %d segments=%d\n", __LINE__, segments);
				float x0 = point1->x;
				float y0 = point1->y;
				float x1 = point1->x + point1->control_x2;
				float y1 = point1->y + point1->control_y2;
				float x2 = point2->x + point2->control_x1;
				float y2 = point2->y + point2->control_y1;
				float x3 = point2->x;
				float y3 = point2->y;

				for(int j = 0; j <= segments; j++)
				{
					float t = (float)j / segments;
					float tpow2 = t * t;
					float tpow3 = t * t * t;
					float invt = 1 - t;
					float invtpow2 = invt * invt;
					float invtpow3 = invt * invt * invt;

					x = (        invtpow3 * x0
						+ 3 * t     * invtpow2 * x1
						+ 3 * tpow2 * invt     * x2 
						+     tpow3            * x3);
					y = (        invtpow3 * y0 
						+ 3 * t     * invtpow2 * y1
						+ 3 * tpow2 * invt     * y2 
						+     tpow3            * y3);

					y -= ptr->start_y;
					x *= OVERSAMPLE;
					y *= OVERSAMPLE;

					if(j > 0)
					{
						draw_line_clamped(temp, 
                            old_x, 
                            old_y, 
                            (int)x, 
                            (int)y, 
                            pixel_value);
					}

					old_x = (int)x;
					old_y = (int)y;
				}
			}

SET_TRACE
//printf("MaskUnit::process_package 1\n");




            if(engine->mode == MASK_MULTIPLY_ALPHA ||
                engine->mode == MASK_SUBTRACT_ALPHA)
            {
// Fill in the polygon in the horizontal direction
			    for(int i = 0; i < oversampled_package_h; i++)
			    {
				    unsigned char *row = (unsigned char*)temp->get_rows()[i];
				    int value = 0x0;
				    int total = 0;

 				    for(int j = 0; j < oversampled_package_w; j++)
					    if(row[j] == pixel_value) total++;

 				    if(total > 1)
				    {
					    if(total & 0x1) total--;
					    for(int j = 0; j < oversampled_package_w; j++)
					    {
						    if(row[j] == pixel_value && total > 0)
						    {
							    if(value)
								{
                                    value = 0x0;
							    }
                                else
								{
                                    value = pixel_value;
							    }
                                total--;
						    }
						    else
						    {
							    if(value) row[j] = pixel_value;
						    }
					    }
				    }
			    }
            }
		}





// downsample temp back to mask

#define DOWNSAMPLE(type, temp_type, inactive, active) \
for(int i = 0; i < ptr->end_y - ptr->start_y; i++) \
{ \
	type *output_row = (type*)mask->get_rows()[i + ptr->start_y]; \
	unsigned char **input_rows = (unsigned char**)temp->get_rows() + i * OVERSAMPLE; \
 \
 \
	for(int j = 0; j < mask_w; j++) \
	{ \
		temp_type total = 0; \
 \
/* Accumulate pixel */ \
		for(int k = 0; k < OVERSAMPLE; k++) \
		{ \
			unsigned char *input_vector = input_rows[k] + j * OVERSAMPLE; \
			for(int l = 0; l < OVERSAMPLE; l++) \
			{ \
				total += (input_vector[l] ? active : inactive); \
			} \
		} \
 \
/* Divide pixel */ \
		total /= OVERSAMPLE * OVERSAMPLE; \
 \
		output_row[j] = total; \
	} \
}

SET_TRACE

// Downsample polygon with inversion
		switch(mask->get_color_model())
		{
			case BC_A8:
			{
				unsigned char active;
                unsigned char inactive = 0;
				active = (int)((float)engine->value / 100 * 0xff);
                if(engine->mode == MASK_SUBTRACT_ALPHA ||
                    engine->mode == MASK_SUBTRACT_PATH)
                {
                    active = 0xff - active;
                    inactive = 0xff;
                }
                
				DOWNSAMPLE(unsigned char, int64_t, inactive, active);
				break;
			}

			case BC_A_FLOAT:
			{
				float active;
                float inactive = 0.0;
				active = (float)engine->value / 100;
                if(engine->mode == MASK_SUBTRACT_ALPHA ||
                    engine->mode == MASK_SUBTRACT_PATH)
                {
                    active = 1.0 - active;
                    inactive = 1.0;
                }

				DOWNSAMPLE(float, double, inactive, active);
				break;
			}
		}
	}


	if(engine->step == DO_X_FEATHER)
	{

		if(engine->recalculate)
		{
// Feather polygon
			if(engine->feather > 0) do_feather(engine->mask, 
				engine->temp_mask, 
				engine->feather, 
				ptr->start_y, 
				ptr->end_y,
				0,
				0);
		}
//printf("MaskUnit::process_package 3 %f\n", engine->feather);
	}

	if(engine->step == DO_Y_FEATHER)
	{
		if(engine->recalculate)
		{
// Feather polygon
			if(engine->feather > 0) do_feather(engine->mask, 
				engine->temp_mask, 
				engine->feather, 
				0, 
				0,
				ptr->start_x,
				ptr->end_x);
		}
	}

	if(engine->step == DO_APPLY)
	{
// Apply mask
		int mask_w = engine->mask->get_w();


#define APPLY_MASK(type, max, components, do_yuv) \
{ \
	type *output_row = (type*)engine->output->get_rows()[i]; \
	type *mask_row = (type*)engine->mask->get_rows()[i]; \
	int chroma_offset = (int)(max + 1) / 2; \
 \
	for(int j  = 0; j < mask_w; j++) \
	{ \
		if(components == 4) \
		{ \
			output_row[j * 4 + 3] = output_row[j * 4 + 3] * mask_row[j] / max; \
		} \
		else \
		{ \
			output_row[j * 3] = output_row[j * 3] * mask_row[j] / max; \
 \
			output_row[j * 3 + 1] = output_row[j * 3 + 1] * mask_row[j] / max; \
			output_row[j * 3 + 2] = output_row[j * 3 + 2] * mask_row[j] / max; \
 \
			if(do_yuv) \
			{ \
				output_row[j * 3 + 1] += chroma_offset * (max - mask_row[j]) / max; \
				output_row[j * 3 + 2] += chroma_offset * (max - mask_row[j]) / max; \
			} \
		} \
	} \
}




//printf("MaskUnit::process_package 1 %d\n", engine->mode);
		for(int i = ptr->start_y; i < ptr->end_y; i++)
		{
			switch(engine->output->get_color_model())
			{
				case BC_RGB888:
					APPLY_MASK(unsigned char, 0xff, 3, 0);
					break;
				case BC_RGB_FLOAT:
					APPLY_MASK(float, 1.0, 3, 0);
					break;
				case BC_YUV888:
					APPLY_MASK(unsigned char, 0xff, 3, 1);
					break;
				case BC_RGBA_FLOAT:
					APPLY_MASK(float, 1.0, 4, 0);
					break;
				case BC_YUVA8888:
					APPLY_MASK(unsigned char, 0xff, 4, 1);
					break;
				case BC_RGBA8888:
					APPLY_MASK(unsigned char, 0xff, 4, 0);
					break;
			}
		}
	}
}





MaskEngine::MaskEngine(int cpus)
 : LoadServer(cpus, cpus)
// : LoadServer(1, OVERSAMPLE * 2)
{
	mask = 0;
}

MaskEngine::~MaskEngine()
{
	if(mask) 
	{
		delete mask;
		delete temp_mask;
	}

	for(int i = 0; i < point_sets.total; i++)
	{
		ArrayList<MaskPoint*> *points = point_sets.values[i];
		points->remove_all_objects();
	}
	point_sets.remove_all_objects();
}

int MaskEngine::points_equivalent(ArrayList<MaskPoint*> *new_points, 
	ArrayList<MaskPoint*> *points)
{
//printf("MaskEngine::points_equivalent %d %d\n", new_points->total, points->total);
	if(new_points->total != points->total) return 0;
	
	for(int i = 0; i < new_points->total; i++)
	{
		if(!(*new_points->values[i] == *points->values[i])) return 0;
	}
	
	return 1;
}

void MaskEngine::do_mask(VFrame *output, 
	int64_t start_position_project,
	MaskAutos *keyframe_set, 
	MaskAuto *keyframe,
	MaskAuto *default_auto)
{
	int new_color_model = 0;
	recalculate = 0;

	switch(output->get_color_model())
	{
		case BC_RGB_FLOAT:
		case BC_RGBA_FLOAT:
			new_color_model = BC_A_FLOAT;
			break;

		case BC_RGB888:
		case BC_RGBA8888:
		case BC_YUV888:
		case BC_YUVA8888:
			new_color_model = BC_A8;
			break;
	}

// Determine if recalculation is needed
SET_TRACE

	if(mask && 
		(mask->get_w() != output->get_w() ||
		mask->get_h() != output->get_h() ||
		mask->get_color_model() != new_color_model))
	{
		delete mask;
		delete temp_mask;
		mask = 0;
		recalculate = 1;
	}

	if(!recalculate)
	{
		if(point_sets.total != keyframe_set->total_submasks(start_position_project, 
			PLAY_FORWARD))
			recalculate = 1;
	}

	if(!recalculate)
	{
		for(int i = 0; 
			i < keyframe_set->total_submasks(start_position_project, 
				PLAY_FORWARD) && !recalculate; 
			i++)
		{
			ArrayList<MaskPoint*> *new_points = new ArrayList<MaskPoint*>;
			keyframe_set->get_points(new_points, 
				i, 
				start_position_project, 
				PLAY_FORWARD);
			if(!points_equivalent(new_points, point_sets.values[i])) recalculate = 1;
			new_points->remove_all_objects();
			delete new_points;
		}
	}

    int new_mode = default_auto->mode;
	int new_value = keyframe_set->get_value(start_position_project, 
		PLAY_FORWARD);
	float new_feather = keyframe_set->get_feather(start_position_project, 
		PLAY_FORWARD);
	float new_radius = keyframe_set->get_radius(start_position_project, 
		PLAY_FORWARD);

	if(recalculate ||
        mode != new_mode ||
		!EQUIV(new_feather, feather) ||
		!EQUIV(new_radius, radius) ||
		!EQUIV(new_value, value))
	{
		recalculate = 1;
		if(!mask) 
		{
			mask = new VFrame(0, 
					-1,
					output->get_w(), 
					output->get_h(),
					new_color_model,
					-1);
			temp_mask = new VFrame(0, 
					-1,
					output->get_w(), 
					output->get_h(),
					new_color_model,
					-1);
		}
        
		if(new_feather > 0)
		{
        	temp_mask->clear_frame();
		}
        else
		{
        	mask->clear_frame();
        }

		for(int i = 0; i < point_sets.size(); i++)
		{
			ArrayList<MaskPoint*> *points = point_sets.get(i);
			points->remove_all_objects();
		}
		point_sets.remove_all_objects();

		for(int i = 0; 
			i < keyframe_set->total_submasks(start_position_project, 
				PLAY_FORWARD); 
			i++)
		{
			ArrayList<MaskPoint*> *new_points = new ArrayList<MaskPoint*>;
			keyframe_set->get_points(new_points, 
				i, 
				start_position_project, 
				PLAY_FORWARD);
			point_sets.append(new_points);
		}

// force driver to transfer it to a texture
#ifdef HAVE_GL
        mask->set_opengl_state(VFrame::RAM);
#endif
	}



	this->output = output;
	this->mode = new_mode;
	this->feather = new_feather;
    this->radius = new_radius;
	this->value = new_value;


// Run units
SET_TRACE
//    int temp_packages = get_total_packages();
//    set_package_count(1);
	step = DO_MASK;
	process_packages();
//    set_package_count(temp_packages);
    
	step = DO_Y_FEATHER;
	process_packages();
	step = DO_X_FEATHER;
	process_packages();

#ifdef HAVE_GL
    if(output->get_opengl_state() == VFrame::RAM)
#endif // HAVE_GL
	{
// only apply it if it's in RAM
        step = DO_APPLY;
    	process_packages();
    }
SET_TRACE


}

void MaskEngine::init_packages()
{
//printf("MaskEngine::init_packages %d total_packages=%d\n", __LINE__, get_total_packages());
	int division = (int)((float)output->get_h() / (get_total_packages() / 2) + 0.5);
	if(division < 1) division = 1;

	for(int i = 0; i < get_total_packages(); i++)
	{
		MaskPackage *ptr = (MaskPackage*)get_package(i);

		ptr->start_y = output->get_h() * i / get_total_packages();
		ptr->end_y = output->get_h() * (i + 1) / get_total_packages();

		ptr->start_x = output->get_w() * i / get_total_packages();
		ptr->end_x = output->get_w() * (i + 1) / get_total_packages();
	}
//printf("MaskEngine::init_packages 2\n");
}

LoadClient* MaskEngine::new_client()
{
	return new MaskUnit(this);
}

LoadPackage* MaskEngine::new_package()
{
	return new MaskPackage;
}





