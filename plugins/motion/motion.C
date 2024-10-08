/*
 * CINELERRA
 * Copyright (C) 2016-2021 Adam Williams <broadcast at earthling dot net>
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
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "bcsignals.h"
//#include "edgeengine.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "motion.h"
#include "motionscan.h"
#include "motionwindow.h"
#include "mutex.h"
#include "overlayframe.h"
#include "rotateframe.h"
#include "rotatescan.h"
#include "spheretranslator.h"
#include "transportque.h"


#include <errno.h>
#include <unistd.h>

REGISTER_PLUGIN(MotionMain)

#undef DEBUG

// #ifndef DEBUG
// #define DEBUG
// #endif



MotionConfig::MotionConfig()
{
	global_range_w = 5;
	global_range_h = 5;
	rotation_range = 5;
	rotation_center = 0;
	block_count = 1;
	global_block_w = MIN_BLOCK;
	global_block_h = MIN_BLOCK;
//	rotation_block_w = MIN_BLOCK;
//	rotation_block_h = MIN_BLOCK;
	block_x = 50;
	block_y = 50;
//	global_positions = 256;
//	rotate_positions = 4;
	magnitude = 100;
	rotate_magnitude = 90;
	return_speed = 0;
	rotate_return_speed = 0;
	action_type = MotionScan::STABILIZE;
//	global = 1;
	rotate = 1;
	tracking_type = MotionScan::NO_CALCULATE;
	draw_vectors = 1;
	tracking_object = MotionScan::TRACK_SINGLE;
	track_frame = 0;
	bottom_is_master = 1;
	horizontal_only = 0;
	vertical_only = 0;
}

void MotionConfig::boundaries()
{
	CLAMP(global_range_w, MIN_RADIUS, MAX_RADIUS);
	CLAMP(global_range_h, MIN_RADIUS, MAX_RADIUS);
	CLAMP(rotation_range, MIN_ROTATION, MAX_ROTATION);
	CLAMP(rotation_center, -MAX_ROTATION, MAX_ROTATION);
	CLAMP(block_count, MIN_BLOCKS, MAX_BLOCKS);
	CLAMP(global_block_w, MIN_BLOCK, MAX_BLOCK);
	CLAMP(global_block_h, MIN_BLOCK, MAX_BLOCK);
//	CLAMP(rotation_block_w, MIN_BLOCK, MAX_BLOCK);
//	CLAMP(rotation_block_h, MIN_BLOCK, MAX_BLOCK);
}

int MotionConfig::equivalent(MotionConfig &that)
{
	return global_range_w == that.global_range_w &&
		global_range_h == that.global_range_h &&
		rotation_range == that.rotation_range &&
		rotation_center == that.rotation_center &&
		action_type == that.action_type &&
//		global == that.global &&
		rotate == that.rotate &&
		draw_vectors == that.draw_vectors &&
		block_count == that.block_count &&
		global_block_w == that.global_block_w &&
		global_block_h == that.global_block_h &&
//		rotation_block_w == that.rotation_block_w &&
//		rotation_block_h == that.rotation_block_h &&
		EQUIV(block_x, that.block_x) &&
		EQUIV(block_y, that.block_y) &&
//		global_positions == that.global_positions &&
//		rotate_positions == that.rotate_positions &&
		magnitude == that.magnitude &&
		return_speed == that.return_speed &&
		rotate_return_speed == that.rotate_return_speed &&
		rotate_magnitude == that.rotate_magnitude &&
		tracking_object == that.tracking_object &&
		track_frame == that.track_frame &&
		bottom_is_master == that.bottom_is_master &&
		horizontal_only == that.horizontal_only &&
		vertical_only == that.vertical_only &&
		tracking_type == that.tracking_type;
}

void MotionConfig::copy_from(MotionConfig &that)
{
	global_range_w = that.global_range_w;
	global_range_h = that.global_range_h;
	rotation_range = that.rotation_range;
	rotation_center = that.rotation_center;
	action_type = that.action_type;
//	global = that.global;
	rotate = that.rotate;
	tracking_type = that.tracking_type;
	draw_vectors = that.draw_vectors;
	block_count = that.block_count;
	block_x = that.block_x;
	block_y = that.block_y;
//	global_positions = that.global_positions;
//	rotate_positions = that.rotate_positions;
	global_block_w = that.global_block_w;
	global_block_h = that.global_block_h;
//	rotation_block_w = that.rotation_block_w;
//	rotation_block_h = that.rotation_block_h;
	magnitude = that.magnitude;
	return_speed = that.return_speed;
	rotate_magnitude = that.rotate_magnitude;
	rotate_return_speed = that.rotate_return_speed;
	tracking_object = that.tracking_object;
	track_frame = that.track_frame;
	bottom_is_master = that.bottom_is_master;
	horizontal_only = that.horizontal_only;
	vertical_only = that.vertical_only;
}

void MotionConfig::interpolate(MotionConfig &prev, 
	MotionConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->block_x = prev.block_x;
	this->block_y = prev.block_y;
	global_range_w = prev.global_range_w;
	global_range_h = prev.global_range_h;
	rotation_range = prev.rotation_range;
	rotation_center = prev.rotation_center;
	action_type = prev.action_type;
//	global = prev.global;
	rotate = prev.rotate;
	tracking_type = prev.tracking_type;
	draw_vectors = prev.draw_vectors;
	block_count = prev.block_count;
//	global_positions = prev.global_positions;
//	rotate_positions = prev.rotate_positions;
	global_block_w = prev.global_block_w;
	global_block_h = prev.global_block_h;
//	rotation_block_w = prev.rotation_block_w;
//	rotation_block_h = prev.rotation_block_h;
	magnitude = prev.magnitude;
	return_speed = prev.return_speed;
	rotate_magnitude = prev.rotate_magnitude;
	rotate_return_speed = prev.rotate_return_speed;
	tracking_object = prev.tracking_object;
	track_frame = prev.track_frame;
	bottom_is_master = prev.bottom_is_master;
	horizontal_only = prev.horizontal_only;
	vertical_only = prev.vertical_only;
}



















MotionMain::MotionMain(PluginServer *server)
 : PluginVClient(server)
{
//    edge = 0;
	engine = 0;
	rotate_engine = 0;
//	motion_rotate = 0;
	total_dx = 0;
	total_dy = 0;
	total_angle = 0;
	overlayer = 0;
	sphere = 0;
	search_area = 0;
	search_size = 0;
	temp_frame = 0;
	previous_frame_number = -1;

	prev_global_ref = 0;
	current_global_ref = 0;
	global_target_src = 0;
	global_target_dst = 0;

//	prev_rotate_ref = 0;
//	current_rotate_ref = 0;
	rotate_target_src = 0;
	rotate_target_dst = 0;
}

MotionMain::~MotionMain()
{
	delete sphere;

//    delete edge;
	delete engine;
	delete overlayer;
	delete [] search_area;
	delete temp_frame;
	delete rotate_engine;
//	delete motion_rotate;


	delete prev_global_ref;
	delete current_global_ref;
	delete global_target_src;
	delete global_target_dst;

//	delete prev_rotate_ref;
//	delete current_rotate_ref;
	delete rotate_target_src;
	delete rotate_target_dst;
}

const char* MotionMain::plugin_title() { return N_("Motion"); }
int MotionMain::is_realtime() { return 1; }
int MotionMain::is_multichannel() { return 1; }


NEW_WINDOW_MACRO(MotionMain, MotionWindow)

LOAD_CONFIGURATION_MACRO(MotionMain, MotionConfig)



void MotionMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("MotionMain::update_gui");
			
			char string[BCTEXTLEN];
// 			sprintf(string, "%d", config.global_positions);
// 			((MotionWindow*)thread->window)->global_search_positions->set_text(string);
// 			sprintf(string, "%d", config.rotate_positions);
// 			((MotionWindow*)thread->window)->rotation_search_positions->set_text(string);

			((MotionWindow*)thread->window)->global_block_w->update(config.global_block_w);
			((MotionWindow*)thread->window)->global_block_h->update(config.global_block_h);
//			((MotionWindow*)thread->window)->rotation_block_w->update(config.rotation_block_w);
//			((MotionWindow*)thread->window)->rotation_block_h->update(config.rotation_block_h);
			((MotionWindow*)thread->window)->block_x->update(config.block_x);
			((MotionWindow*)thread->window)->block_y->update(config.block_y);
			((MotionWindow*)thread->window)->block_x_text->update((float)config.block_x);
			((MotionWindow*)thread->window)->block_y_text->update((float)config.block_y);
			((MotionWindow*)thread->window)->magnitude->update(config.magnitude);
			((MotionWindow*)thread->window)->return_speed->update(config.return_speed);
			((MotionWindow*)thread->window)->rotate_magnitude->update(config.rotate_magnitude);
			((MotionWindow*)thread->window)->rotate_return_speed->update(config.rotate_return_speed);
			((MotionWindow*)thread->window)->rotation_range->update(config.rotation_range);
			((MotionWindow*)thread->window)->rotation_center->update(config.rotation_center);


			((MotionWindow*)thread->window)->track_single->update(config.tracking_object == MotionScan::TRACK_SINGLE);
			((MotionWindow*)thread->window)->track_frame_number->update(config.track_frame);
			((MotionWindow*)thread->window)->track_previous->update(config.tracking_object == MotionScan::TRACK_PREVIOUS);
			((MotionWindow*)thread->window)->previous_same->update(config.tracking_object == MotionScan::PREVIOUS_SAME_BLOCK);
			if(config.tracking_object != MotionScan::TRACK_SINGLE)
				((MotionWindow*)thread->window)->track_frame_number->disable();
			else
				((MotionWindow*)thread->window)->track_frame_number->enable();

			((MotionWindow*)thread->window)->action_type->set_text(
				ActionType::to_text(config.action_type));


//printf("MotionMain::update_gui %d %d\n", __LINE__, config.tracking_type);
			((MotionWindow*)thread->window)->tracking_type->set_text(
				TrackingType::to_text(config.tracking_type));
			((MotionWindow*)thread->window)->track_direction->set_text(
				TrackDirection::to_text(config.horizontal_only, config.vertical_only));
			((MotionWindow*)thread->window)->master_layer->set_text(
				MasterLayer::to_text(config.bottom_is_master));


			((MotionWindow*)thread->window)->update_mode();
			thread->window->unlock_window();
		}
	}
}




void MotionMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("MOTION");

	output.tag.set_property("BLOCK_COUNT", config.block_count);
//	output.tag.set_property("GLOBAL_POSITIONS", config.global_positions);
//	output.tag.set_property("ROTATE_POSITIONS", config.rotate_positions);
	output.tag.set_property("GLOBAL_BLOCK_W", config.global_block_w);
	output.tag.set_property("GLOBAL_BLOCK_H", config.global_block_h);
//	output.tag.set_property("ROTATION_BLOCK_W", config.rotation_block_w);
//	output.tag.set_property("ROTATION_BLOCK_H", config.rotation_block_h);
	output.tag.set_property("BLOCK_X", config.block_x);
	output.tag.set_property("BLOCK_Y", config.block_y);
	output.tag.set_property("GLOBAL_RANGE_W", config.global_range_w);
	output.tag.set_property("GLOBAL_RANGE_H", config.global_range_h);
	output.tag.set_property("ROTATION_RANGE", config.rotation_range);
	output.tag.set_property("ROTATION_CENTER", config.rotation_center);
	output.tag.set_property("MAGNITUDE", config.magnitude);
	output.tag.set_property("RETURN_SPEED", config.return_speed);
	output.tag.set_property("ROTATE_MAGNITUDE", config.rotate_magnitude);
	output.tag.set_property("ROTATE_RETURN_SPEED", config.rotate_return_speed);
	output.tag.set_property("ACTION_TYPE", config.action_type);
//	output.tag.set_property("GLOBAL", config.global);
	output.tag.set_property("ROTATE", config.rotate);
	output.tag.set_property("TRACKING_TYPE", config.tracking_type);
	output.tag.set_property("DRAW_VECTORS", config.draw_vectors);
	output.tag.set_property("TRACKING_OBJECT", config.tracking_object);
	output.tag.set_property("TRACK_FRAME", config.track_frame);
	output.tag.set_property("BOTTOM_IS_MASTER", config.bottom_is_master);
	output.tag.set_property("HORIZONTAL_ONLY", config.horizontal_only);
	output.tag.set_property("VERTICAL_ONLY", config.vertical_only);
	output.append_tag();
	output.terminate_string();
}

void MotionMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("MOTION"))
			{
				config.block_count = input.tag.get_property("BLOCK_COUNT", config.block_count);
//				config.global_positions = input.tag.get_property("GLOBAL_POSITIONS", config.global_positions);
//				config.rotate_positions = input.tag.get_property("ROTATE_POSITIONS", config.rotate_positions);
				config.global_block_w = input.tag.get_property("GLOBAL_BLOCK_W", config.global_block_w);
				config.global_block_h = input.tag.get_property("GLOBAL_BLOCK_H", config.global_block_h);
//				config.rotation_block_w = input.tag.get_property("ROTATION_BLOCK_W", config.rotation_block_w);
//				config.rotation_block_h = input.tag.get_property("ROTATION_BLOCK_H", config.rotation_block_h);
				config.block_x = input.tag.get_property("BLOCK_X", config.block_x);
				config.block_y = input.tag.get_property("BLOCK_Y", config.block_y);
				config.global_range_w = input.tag.get_property("GLOBAL_RANGE_W", config.global_range_w);
				config.global_range_h = input.tag.get_property("GLOBAL_RANGE_H", config.global_range_h);
				config.rotation_range = input.tag.get_property("ROTATION_RANGE", config.rotation_range);
				config.rotation_center = input.tag.get_property("ROTATION_CENTER", config.rotation_center);
				config.magnitude = input.tag.get_property("MAGNITUDE", config.magnitude);
				config.return_speed = input.tag.get_property("RETURN_SPEED", config.return_speed);
				config.rotate_magnitude = input.tag.get_property("ROTATE_MAGNITUDE", config.rotate_magnitude);
				config.rotate_return_speed = input.tag.get_property("ROTATE_RETURN_SPEED", config.rotate_return_speed);
				config.action_type = input.tag.get_property("ACTION_TYPE", config.action_type);
//				config.global = input.tag.get_property("GLOBAL", config.global);
				config.rotate = input.tag.get_property("ROTATE", config.rotate);
				config.tracking_type = input.tag.get_property("TRACKING_TYPE", config.tracking_type);
				config.draw_vectors = input.tag.get_property("DRAW_VECTORS", config.draw_vectors);
				config.tracking_object = input.tag.get_property("TRACKING_OBJECT", config.tracking_object);
				config.track_frame = input.tag.get_property("TRACK_FRAME", config.track_frame);
				config.bottom_is_master = input.tag.get_property("BOTTOM_IS_MASTER", config.bottom_is_master);
				config.horizontal_only = input.tag.get_property("HORIZONTAL_ONLY", config.horizontal_only);
				config.vertical_only = input.tag.get_property("VERTICAL_ONLY", config.vertical_only);
			}
		}
	}
	config.boundaries();
}









void MotionMain::allocate_temp(int w, int h, int color_model)
{
	if(temp_frame && 
		(temp_frame->get_w() != w ||
		temp_frame->get_h() != h))
	{
		delete temp_frame;
		temp_frame = 0;
	}
	if(!temp_frame)
		temp_frame = new VFrame(w, h, color_model);
}


void MotionMain::process_global()
{
	int w = current_global_ref->get_w();
	int h = current_global_ref->get_h();


	if(!engine) engine = new MotionScan(PluginClient::get_project_smp() + 1,
		PluginClient::get_project_smp() + 1);

// Determine if frames changed
// printf("MotionMain::process_global %d block_y=%f total_dy=%d\n",
// __LINE__,
// config.block_y * h / 100,
// total_dy);

	engine->scan_frame(current_global_ref, 
		prev_global_ref,
		config.global_range_w * w / 100,
		config.global_range_h * h / 100,
		config.global_block_w * w / 100,
		config.global_block_h * h / 100,
		config.block_x * w / 100,
		config.block_y * h / 100,
		config.tracking_object,
		config.tracking_type,
		config.action_type,
		config.horizontal_only,
		config.vertical_only,
		get_source_position(),
		total_dx,
		total_dy,
		0,
		0,
		1, // do_motion
		config.rotate, // do_rotate
		config.rotation_center,
		config.rotation_range);
		
	current_dx = engine->dx_result;
	current_dy = engine->dy_result;

// Add current motion vector to accumulation vector.
	if(config.tracking_object != MotionScan::TRACK_SINGLE)
	{
// Retract over time
		total_dx = (int64_t)total_dx * (100 - config.return_speed) / 100;
		total_dy = (int64_t)total_dy * (100 - config.return_speed) / 100;
		total_dx += engine->dx_result;
		total_dy += engine->dy_result;
// printf("MotionMain::process_global %d total_dy=%d engine->dy_result=%d\n", 
// __LINE__,
// total_dy,
// engine->dy_result);
	}
	else
// Make accumulation vector current
	{
		total_dx = engine->dx_result;
		total_dy = engine->dy_result;
	}

// Clamp accumulation vector
	if(config.magnitude < 100)
	{
		int block_w = (int64_t)config.global_block_w * 
				w / 100;
		int block_h = (int64_t)config.global_block_h * 
				h / 100;
		int block_x_orig = (int64_t)(config.block_x * 
			w / 
			100);
		int block_y_orig = (int64_t)(config.block_y *
			h / 
			100);

		int max_block_x = (int64_t)(w - block_x_orig) *
			OVERSAMPLE * 
			config.magnitude / 
			100;
		int max_block_y = (int64_t)(h - block_y_orig) *
			OVERSAMPLE *
			config.magnitude / 
			100;
		int min_block_x = (int64_t)-block_x_orig * 
			OVERSAMPLE * 
			config.magnitude / 
			100;
		int min_block_y = (int64_t)-block_y_orig * 
			OVERSAMPLE * 
			config.magnitude / 
			100;

		CLAMP(total_dx, min_block_x, max_block_x);
		CLAMP(total_dy, min_block_y, max_block_y);
	}

// printf("MotionMain::process_global %d total_dx=%d total_dy=%d\n", 
// __LINE__,
// total_dx,
// total_dy);

	if(config.tracking_object != MotionScan::TRACK_SINGLE && !config.rotate)
	{
// Transfer current reference frame to previous reference frame and update
// counter.
		prev_global_ref->copy_from(current_global_ref);
		previous_frame_number = get_source_position();
	}

// Decide what to do with target based on requested operation
	int interpolation;
	float dx;
	float dy;
	switch(config.action_type)
	{
		case MotionScan::NOTHING:
			global_target_dst->copy_from(global_target_src);
			break;
		case MotionScan::TRACK_PIXEL:
			interpolation = NEAREST_NEIGHBOR;
			dx = (int)(total_dx / OVERSAMPLE);
			dy = (int)(total_dy / OVERSAMPLE);
			break;
		case MotionScan::STABILIZE_PIXEL:
			interpolation = NEAREST_NEIGHBOR;
			dx = -(int)(total_dx / OVERSAMPLE);
			dy = -(int)(total_dy / OVERSAMPLE);
			break;
			break;
		case MotionScan::TRACK:
			interpolation = CUBIC_LINEAR;
			dx = (float)total_dx / OVERSAMPLE;
			dy = (float)total_dy / OVERSAMPLE;
			break;
		case MotionScan::STABILIZE:
		case MotionScan::STABILIZE_SPHERE:
			interpolation = CUBIC_LINEAR;
			dx = -(float)total_dx / OVERSAMPLE;
			dy = -(float)total_dy / OVERSAMPLE;
			break;
	}


	if(config.action_type != MotionScan::NOTHING &&
		config.action_type != MotionScan::STABILIZE_SPHERE)
	{
		if(!overlayer) 
			overlayer = new OverlayFrame(PluginClient::get_project_smp() + 1);
		global_target_dst->clear_frame();
		overlayer->overlay(global_target_dst,
			global_target_src,
			0,
			0,
			global_target_src->get_w(),
			global_target_src->get_h(),
			dx,
			dy,
			(float)global_target_src->get_w() + dx,
			(float)global_target_src->get_h() + dy,
			1,
			TRANSFER_REPLACE,
			interpolation);
	}

	if(config.action_type == MotionScan::STABILIZE_SPHERE)
	{
		sphere->set_pivot(config.block_x, config.block_y);
		sphere->set_x(-dx * 360 / w);
		sphere->set_y(-dy * 180 / h);
		sphere->set_z(0);
	}
}


void MotionMain::process_rotation()
{
	int block_x;
	int block_y;

// Always require global
// Convert the previous global reference into the previous rotation reference.
// Convert global target destination into rotation target source.
	if(!overlayer) 
	{
		overlayer = new OverlayFrame(PluginClient::get_project_smp() + 1);
	}
	
	float dx;
	float dy;
	if(config.tracking_object == MotionScan::TRACK_SINGLE)
	{
		dx = (float)total_dx / OVERSAMPLE;
		dy = (float)total_dy / OVERSAMPLE;
	}
	else
	{
		dx = (float)current_dx / OVERSAMPLE;
		dy = (float)current_dy / OVERSAMPLE;
	}

// Pivot is destination global position
	block_x = (int)(w * 
		config.block_x / 
		100 +
		(float)total_dx / 
		OVERSAMPLE);
	block_y = (int)(h * 
		config.block_y / 
		100 +
		(float)total_dy / 
		OVERSAMPLE);


// Use the global target output as the rotation target input
	if(config.action_type != MotionScan::STABILIZE_SPHERE)
	{
		rotate_target_src->copy_from(global_target_dst);
	}



// Transfer current reference frame to previous reference frame and update
// counter.
	if(config.tracking_object != MotionScan::TRACK_SINGLE)
	{
		prev_global_ref->copy_from(current_global_ref);
		previous_frame_number = get_source_position();
	}




	current_angle = engine->dr_result;

// Add current rotation to accumulation
	if(config.tracking_object != MotionScan::TRACK_SINGLE)
	{
// Retract over time
		total_angle = total_angle * (100 - config.rotate_return_speed) / 100;
// Accumulate current rotation
		total_angle += current_angle;

// Clamp rotation accumulation
		if(config.rotate_magnitude < 90)
		{
			CLAMP(total_angle, -config.rotate_magnitude, config.rotate_magnitude);
		}

	}
	else
	{
		total_angle = current_angle;
	}

#ifdef DEBUG
printf("MotionMain::process_rotation total_angle=%f\n", total_angle);
#endif


// Calculate rotation parameters based on requested operation
	float angle = 0;
	switch(config.action_type)
	{
		case MotionScan::NOTHING:
			rotate_target_dst->copy_from(rotate_target_src);
			break;
		case MotionScan::TRACK:
		case MotionScan::TRACK_PIXEL:
			angle = total_angle;
			break;
		case MotionScan::STABILIZE:
		case MotionScan::STABILIZE_PIXEL:
		case MotionScan::STABILIZE_SPHERE:
			angle = -total_angle;
			break;
	}

printf("MotionMain::process_rotation angle=%f\n", angle);


// apply the rotation
	if(config.action_type != MotionScan::NOTHING &&
		config.action_type != MotionScan::STABILIZE_SPHERE)
	{
		if(!rotate_engine)
			rotate_engine = new AffineEngine(PluginClient::get_project_smp() + 1,
				PluginClient::get_project_smp() + 1);

		rotate_target_dst->clear_frame();

// Determine pivot based on a number of factors.
		switch(config.action_type)
		{
			case MotionScan::TRACK:
			case MotionScan::TRACK_PIXEL:
// Use destination of global tracking.
				rotate_engine->set_in_pivot(block_x, block_y);
				rotate_engine->set_out_pivot(block_x, block_y);
				break;

			case MotionScan::STABILIZE:
			case MotionScan::STABILIZE_PIXEL:
// Use origin of global stabilize operation
				rotate_engine->set_in_pivot((int)(rotate_target_dst->get_w() * 
						config.block_x / 
						100),
					(int)(rotate_target_dst->get_h() * 
						config.block_y / 
						100));
				rotate_engine->set_out_pivot((int)(rotate_target_dst->get_w() * 
						config.block_x / 
						100),
					(int)(rotate_target_dst->get_h() * 
						config.block_y / 
						100));

				break;
		}


		rotate_engine->rotate(rotate_target_dst, rotate_target_src, angle);


	}
	
	if(config.action_type == MotionScan::STABILIZE_SPHERE)
	{
		sphere->set_z(angle);
	}


}

void MotionMain::process_sphere()
{
// process with arguments set in the global/rotate functions
	sphere->process(global_target_dst,
		global_target_src);

}








int MotionMain::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	int need_reconfigure = load_configuration();
	int color_model = frame[0]->get_color_model();
	w = frame[0]->get_w();
	h = frame[0]->get_h();
	

#ifdef DEBUG
printf("MotionMain::process_buffer %d start_position=%lld\n", __LINE__, start_position);
#endif


// Calculate the source and destination pointers for each of the operations.
// Get the layer to track motion in.
	reference_layer = config.bottom_is_master ?
		PluginClient::total_in_buffers - 1 :
		0;
// Get the layer to apply motion in.
	target_layer = config.bottom_is_master ?
		0 :
		PluginClient::total_in_buffers - 1;


	output_frame = frame[target_layer];


// Get the position of previous reference frame.
	int64_t actual_previous_number;
// Skip if match frame not available
	int skip_current = 0;


	if(config.tracking_object == MotionScan::TRACK_SINGLE)
	{
		actual_previous_number = config.track_frame;
		if(get_direction() == PLAY_REVERSE)
			actual_previous_number++;
		if(actual_previous_number == start_position)
			skip_current = 1;
	}
	else
	{
		actual_previous_number = start_position;
		if(get_direction() == PLAY_FORWARD)
		{
			actual_previous_number--;
			if(actual_previous_number < get_source_start())
				skip_current = 1;
			else
			{
				KeyFrame *keyframe = get_prev_keyframe(start_position, 1);
				if(keyframe->position > 0 &&
					actual_previous_number < keyframe->position)
					skip_current = 1;
			}
		}
		else
		{
			actual_previous_number++;
			if(actual_previous_number >= get_source_start() + get_total_len())
				skip_current = 1;
			else
			{
				KeyFrame *keyframe = get_next_keyframe(start_position, 1);
				if(keyframe->position > 0 &&
					actual_previous_number >= keyframe->position)
					skip_current = 1;
			}
		}

// Only count motion since last keyframe
		

	}


//	if(!config.global && !config.rotate) skip_current = 1;




// printf("process_realtime %d %lld %lld\n", 
// skip_current, 
// previous_frame_number, 
// actual_previous_number);
// Load match frame and reset vectors
	int need_reload = !skip_current && 
		(previous_frame_number != actual_previous_number ||
		need_reconfigure);
	if(need_reload)
	{
		total_dx = 0;
		total_dy = 0;
		total_angle = 0;
		previous_frame_number = actual_previous_number;
	}


	if(skip_current)
	{
		total_dx = 0;
		total_dy = 0;
		current_dx = 0;
		current_dy = 0;
		total_angle = 0;
		current_angle = 0;
	}




// Get the global pointers.  Here we walk through the sequence of events.
//	if(config.global)
	if(1)
	{
// Assume global only.  Global reads previous frame and compares
// with current frame to get the current translation.
// The center of the search area is fixed in compensate mode or
// the user value + the accumulation vector in track mode.
		if(!prev_global_ref)
			prev_global_ref = new VFrame(w, h, color_model);
		if(!current_global_ref)
			current_global_ref = new VFrame(w, h, color_model);

// Global loads the current target frame into the src and 
// writes it to the dst frame with desired translation.
		if(!global_target_src)
			global_target_src = new VFrame(w, h, color_model);
		if(!global_target_dst)
			global_target_dst = new VFrame(w, h, color_model);


//         if(!edge)
//         {
//             edge = new EdgeEngine(
// 			    PluginClient::get_project_smp() + 1,
// 			    PluginClient::get_project_smp() + 1);
//         }

// Load the global frames
		if(need_reload)
		{
			read_frame(prev_global_ref, 
				reference_layer, 
				previous_frame_number, 
				frame_rate,
				0);
// edge detect the reference frames
//            edge->process(prev_global_ref, prev_global_ref, 1);
		}

		read_frame(current_global_ref, 
			reference_layer, 
			start_position, 
			frame_rate,
			0);
		read_frame(global_target_src,
			target_layer,
			start_position,
			frame_rate,
			0);
// edge detect the reference frames
//        edge->process(current_global_ref, current_global_ref, 1);

// char temp[BCTEXTLEN];
// sprintf(temp, "/tmp/ref%06ld.png", start_position);
// current_global_ref->write_png(temp, 9);
// sprintf(temp, "/tmp/target%06ld.png", start_position);
// global_target_src->write_png(temp, 9);



// Global followed by rotate
		if(config.rotate)
		{
// Must translate the previous global reference by the current global
// accumulation vector to match the current global reference.
// The center of the search area is always the user value + the accumulation
// vector.
//			if(!prev_rotate_ref)
//				prev_rotate_ref = new VFrame(w, h, color_model);
// The current global reference is the current rotation reference.
//			if(!current_rotate_ref)
//				current_rotate_ref = new VFrame(w, h, color_model);
//			current_rotate_ref->copy_from(current_global_ref);

// The global target destination is copied to the rotation target source
// then written to the rotation output with rotation.
// The pivot for the rotation is the center of the search area 
// if we're tracking.
// The pivot is fixed to the user position if we're compensating.
			if(!rotate_target_src)
				rotate_target_src = new VFrame(w, h, color_model);
			if(!rotate_target_dst)
				rotate_target_dst = new VFrame(w, h, color_model);
		}
	}
//	else
// Rotation only
// 	if(config.rotate)
// 	{
// // Rotation reads the previous reference frame and compares it with current 
// // reference frame.
// 		if(!prev_rotate_ref)
// 			prev_rotate_ref = new VFrame(w, h, color_model);
// 		if(!current_rotate_ref)
// 			current_rotate_ref = new VFrame(w, h, color_model);
// 
// // Rotation loads target frame to temporary, rotates it, and writes it to the
// // target frame.  The pivot is always fixed.
// 		if(!rotate_target_src)
// 			rotate_target_src = new VFrame(w, h, color_model);
// 		if(!rotate_target_dst)
// 			rotate_target_dst = new VFrame(w, h, color_model);
// 
// 
// // Load the rotate frames
// 		if(need_reload)
// 		{
// 			read_frame(prev_rotate_ref, 
// 				reference_layer, 
// 				previous_frame_number, 
// 				frame_rate,
// 				0);
// 		}
// 		read_frame(current_rotate_ref, 
// 			reference_layer, 
// 			start_position, 
// 			frame_rate,
// 			0);
// 		read_frame(rotate_target_src,
// 			target_layer,
// 			start_position,
// 			frame_rate,
// 			0);
// 	}







//PRINT_TRACE
//printf("skip_current=%d config.global=%d\n", skip_current, config.global);


	if(!skip_current)
	{
		if(config.action_type == MotionScan::STABILIZE_SPHERE)
		{
			if(!sphere)
			{
				sphere = new SphereTranslateEngine(PluginClient::get_project_smp() + 1, 
						PluginClient::get_project_smp() + 1);
			}
		}
	
// Get position change from previous frame to current frame
		process_global();
// Get rotation change from previous frame to current frame
		if(config.rotate) process_rotation();
//frame[target_layer]->copy_from(prev_rotate_ref);
//frame[target_layer]->copy_from(current_rotate_ref);

		if(config.action_type == MotionScan::STABILIZE_SPHERE)
		{
			process_sphere();
		}


// Transfer the relevant target frame to the output
		if(config.rotate && config.action_type != MotionScan::STABILIZE_SPHERE)
		{
			frame[target_layer]->copy_from(rotate_target_dst);
		}
		else
		{
			frame[target_layer]->copy_from(global_target_dst);
		}
	}
	else
// skip the current frame
// Read the target destination directly
	{
		read_frame(frame[target_layer],
			target_layer,
			start_position,
			frame_rate,
			0);
	}

// draw the reference frame if it's a different track
    if(target_layer != reference_layer)
    {
        frame[reference_layer]->copy_from(current_global_ref);
    }

	if(config.draw_vectors)
	{
		draw_vectors(frame[target_layer]);
	}

#ifdef DEBUG
printf("MotionMain::process_buffer %d\n", __LINE__);
#endif
	return 0;
}



void MotionMain::draw_vectors(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();
	int global_x1, global_y1;
	int global_x2, global_y2;
	int block_x, block_y;
	int block_w, block_h;
	int block_x1, block_y1;
	int block_x2, block_y2;
	int block_x3, block_y3;
	int block_x4, block_y4;
	int scan_w, scan_h;
	int scan_x1, scan_y1;
	int scan_x2, scan_y2;
	int scan_x3, scan_y3;
	int scan_x4, scan_y4;


// global motion search
// Get vector
// Start of vector is center of previous block.
// End of vector is total accumulation.
	if(config.tracking_object == MotionScan::TRACK_SINGLE)
	{
		global_x1 = (int64_t)(config.block_x * 
			w / 
			100);
		global_y1 = (int64_t)(config.block_y *
			h / 
			100);
		global_x2 = global_x1 + total_dx / OVERSAMPLE;
		global_y2 = global_y1 + total_dy / OVERSAMPLE;
//printf("MotionMain::draw_vectors %d %d %d %d %d %d\n", total_dx, total_dy, global_x1, global_y1, global_x2, global_y2);
	}
	else
// Start of vector is center of previous block.
// End of vector is current change.
	if(config.tracking_object == MotionScan::PREVIOUS_SAME_BLOCK)
	{
		global_x1 = (int64_t)(config.block_x * 
			w / 
			100);
		global_y1 = (int64_t)(config.block_y *
			h / 
			100);
		global_x2 = global_x1 + current_dx / OVERSAMPLE;
		global_y2 = global_y1 + current_dy / OVERSAMPLE;
	}
	else
	{
		global_x1 = (int64_t)(config.block_x * 
			w / 
			100 + 
			(total_dx - current_dx) / 
			OVERSAMPLE);
		global_y1 = (int64_t)(config.block_y *
			h / 
			100 +
			(total_dy - current_dy) /
			OVERSAMPLE);
		global_x2 = (int64_t)(config.block_x * 
			w / 
			100 + 
			total_dx / 
			OVERSAMPLE);
		global_y2 = (int64_t)(config.block_y *
			h / 
			100 +
			total_dy /
			OVERSAMPLE);
	}

	block_x = global_x1;
	block_y = global_y1;
	block_w = config.global_block_w * w / 100;
	block_h = config.global_block_h * h / 100;
	block_x1 = block_x - block_w / 2;
	block_y1 = block_y - block_h / 2;
	block_x2 = block_x + block_w / 2;
	block_y2 = block_y + block_h / 2;
	scan_w = config.global_range_w * w / 100;
	scan_h = config.global_range_h * h / 100;
	scan_x1 = block_x1 - scan_w / 2;
	scan_y1 = block_y1 - scan_h / 2;
	scan_x2 = block_x2 + scan_w / 2;
	scan_y2 = block_y2 + scan_h / 2;

// printf("MotionMain::draw_vectors block_x1=%d block_y1=%d block_x2=%d block_y2=%d scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d\n",
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// scan_x1,
// scan_y1,
// scan_x2,
// scan_y2);

	MotionScan::clamp_scan(w, 
		h, 
		&block_x1,
		&block_y1,
		&block_x2,
		&block_y2,
		&scan_x1,
		&scan_y1,
		&scan_x2,
		&scan_y2,
		1);

// Vector
	draw_arrow(frame, global_x1, global_y1, global_x2, global_y2);

// Macroblock
	draw_line(frame, block_x1, block_y1, block_x2, block_y1);
	draw_line(frame, block_x2, block_y1, block_x2, block_y2);
	draw_line(frame, block_x2, block_y2, block_x1, block_y2);
	draw_line(frame, block_x1, block_y2, block_x1, block_y1);


// Search area
	draw_line(frame, scan_x1, scan_y1, scan_x2, scan_y1);
	draw_line(frame, scan_x2, scan_y1, scan_x2, scan_y2);
	draw_line(frame, scan_x2, scan_y2, scan_x1, scan_y2);
	draw_line(frame, scan_x1, scan_y2, scan_x1, scan_y1);

// printf("MotionMain::draw_vectors %d block_x1=%d block_y1=%d block_x2=%d block_y2=%d scan_x1=%d scan_y1=%d scan_x2=%d scan_y2=%d\n", 
// __LINE__,
// block_x1,
// block_y1,
// block_x2,
// block_y2,
// scan_x1,
// scan_y1,
// scan_x2,
// scan_y2);

// Block should be endpoint of motion
	if(config.rotate)
	{
		block_x = global_x2;
		block_y = global_y2;
	}

	block_w = config.global_block_w * w / 100;
	block_h = config.global_block_h * h / 100;
	if(config.rotate)
	{
		float angle = total_angle * 2 * M_PI / 360;
		double base_angle1 = atan((float)block_h / block_w);
		double base_angle2 = atan((float)block_w / block_h);
		double target_angle1 = base_angle1 + angle;
		double target_angle2 = base_angle2 + angle;
		double radius = sqrt(block_w * block_w + block_h * block_h) / 2;
		block_x1 = (int)(block_x - cos(target_angle1) * radius);
		block_y1 = (int)(block_y - sin(target_angle1) * radius);
		block_x2 = (int)(block_x + sin(target_angle2) * radius);
		block_y2 = (int)(block_y - cos(target_angle2) * radius);
		block_x3 = (int)(block_x - sin(target_angle2) * radius);
		block_y3 = (int)(block_y + cos(target_angle2) * radius);
		block_x4 = (int)(block_x + cos(target_angle1) * radius);
		block_y4 = (int)(block_y + sin(target_angle1) * radius);

		draw_line(frame, block_x1, block_y1, block_x2, block_y2);
		draw_line(frame, block_x2, block_y2, block_x4, block_y4);
		draw_line(frame, block_x4, block_y4, block_x3, block_y3);
		draw_line(frame, block_x3, block_y3, block_x1, block_y1);


// Center
		if(!config.global)
		{
			draw_line(frame, block_x, block_y - 5, block_x, block_y + 6);
			draw_line(frame, block_x - 5, block_y, block_x + 6, block_y);
		}
	}
}



void MotionMain::draw_pixel(VFrame *frame, int x, int y)
{
	if(!(x >= 0 && y >= 0 && x < frame->get_w() && y < frame->get_h())) return;

#define DRAW_PIXEL(x, y, components, do_yuv, max, type) \
{ \
	type **rows = (type**)frame->get_rows(); \
	rows[y][x * components] = max - rows[y][x * components]; \
	if(!do_yuv) \
	{ \
		rows[y][x * components + 1] = max - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = max - rows[y][x * components + 2]; \
	} \
	else \
	{ \
		rows[y][x * components + 1] = (max / 2 + 1) - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = (max / 2 + 1) - rows[y][x * components + 2]; \
	} \
	if(components == 4) \
		rows[y][x * components + 3] = max; \
}


	switch(frame->get_color_model())
	{
		case BC_RGB888:
			DRAW_PIXEL(x, y, 3, 0, 0xff, unsigned char);
			break;
		case BC_RGBA8888:
			DRAW_PIXEL(x, y, 4, 0, 0xff, unsigned char);
			break;
		case BC_RGB_FLOAT:
			DRAW_PIXEL(x, y, 3, 0, 1.0, float);
			break;
		case BC_RGBA_FLOAT:
			DRAW_PIXEL(x, y, 4, 0, 1.0, float);
			break;
		case BC_YUV888:
			DRAW_PIXEL(x, y, 3, 1, 0xff, unsigned char);
			break;
		case BC_YUVA8888:
			DRAW_PIXEL(x, y, 4, 1, 0xff, unsigned char);
			break;
		case BC_RGB161616:
			DRAW_PIXEL(x, y, 3, 0, 0xffff, uint16_t);
			break;
		case BC_YUV161616:
			DRAW_PIXEL(x, y, 3, 1, 0xffff, uint16_t);
			break;
		case BC_RGBA16161616:
			DRAW_PIXEL(x, y, 4, 0, 0xffff, uint16_t);
			break;
		case BC_YUVA16161616:
			DRAW_PIXEL(x, y, 4, 1, 0xffff, uint16_t);
			break;
	}
}


void MotionMain::draw_line(VFrame *frame, int x1, int y1, int x2, int y2)
{
	int w = labs(x2 - x1);
	int h = labs(y2 - y1);
//printf("MotionMain::draw_line 1 %d %d %d %d\n", x1, y1, x2, y2);

	if(!w && !h)
	{
		draw_pixel(frame, x1, y1);
	}
	else
	if(w > h)
	{
// Flip coordinates so x1 < x2
		if(x2 < x1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = y2 - y1;
		int denominator = x2 - x1;
		for(int i = x1; i < x2; i++)
		{
			int y = y1 + (int64_t)(i - x1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(frame, i, y);
		}
	}
	else
	{
// Flip coordinates so y1 < y2
		if(y2 < y1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = x2 - x1;
		int denominator = y2 - y1;
		for(int i = y1; i < y2; i++)
		{
			int x = x1 + (int64_t)(i - y1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(frame, x, i);
		}
	}
//printf("MotionMain::draw_line 2\n");
}

#define ARROW_SIZE 10
void MotionMain::draw_arrow(VFrame *frame, int x1, int y1, int x2, int y2)
{
	double angle = atan((float)(y2 - y1) / (float)(x2 - x1));
	double angle1 = angle + (float)145 / 360 * 2 * 3.14159265;
	double angle2 = angle - (float)145 / 360 * 2 * 3.14159265;
	int x3;
	int y3;
	int x4;
	int y4;
	if(x2 < x1)
	{
		x3 = x2 - (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 - (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 - (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 - (int)(ARROW_SIZE * sin(angle2));
	}
	else
	{
		x3 = x2 + (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 + (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 + (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 + (int)(ARROW_SIZE * sin(angle2));
	}

// Main vector
	draw_line(frame, x1, y1, x2, y2);
//	draw_line(frame, x1, y1 + 1, x2, y2 + 1);

// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(frame, x2, y2, x3, y3);
//	draw_line(frame, x2, y2 + 1, x3, y3 + 1);
// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(frame, x2, y2, x4, y4);
//	draw_line(frame, x2, y2 + 1, x4, y4 + 1);
}







