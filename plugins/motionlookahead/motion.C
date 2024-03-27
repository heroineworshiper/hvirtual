/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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

// A stabilizer which looks good but is not forensically accurate.
// Reads ahead to determine the drift.

#include "affine.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "motion.h"
#include "motionscan.h"
#include "motionwindow.h"
#include "overlayframe.h"
#include "transportque.h"

#include <string.h>

REGISTER_PLUGIN(MotionLookahead)

LeastSquares::LeastSquares(float y)
{
    sum_x = 0;
    sum_y = y;
    sum_xy = 0;
    sum_x2 = 0;
    x = 0;
    n = 1;
}

void LeastSquares::append(float y)
{
    x++;
    n++;
    sum_x += x;
    sum_y += y;
    sum_xy += (float)x * y;
    sum_x2 += x * x;
}

float LeastSquares::get_b()
{
    if(n == 1) return sum_y;
    return (sum_y - get_m() * sum_x) / n;
}

float LeastSquares::get_m()
{
    if(n == 1) return 0;
    return (float)(n * sum_xy - sum_x * sum_y) /
        (float)(n * sum_x2 - sum_x * sum_x);
}




MotionLookaheadConfig::MotionLookaheadConfig()
{
// Block position in percentage 0 - 100
	block_x = 50;
	block_y = 50;
// Block size in percent of image size
	block_w = 40;
	block_h = 40;
// Translation search range in percent of image size
	range_w = 20;
	range_h = 20;

    enable = 0;
	do_rotate = 1;
// rotation search in degrees
	rotation_range = 10;

	draw_vectors = 1;
// frames to look ahead
    frames = 60;
}


int MotionLookaheadConfig::equivalent(MotionLookaheadConfig &that)
{
    return EQUIV(block_x, that.block_x) &&
		EQUIV(block_y, that.block_y) &&
        block_w == that.block_w &&
        block_h == that.block_h &&
        range_w == that.range_w &&
        range_h == that.range_h &&
        enable == that.enable &&
        do_rotate == that.do_rotate &&
        rotation_range == that.rotation_range &&
        draw_vectors == that.draw_vectors &&
        frames == that.frames;
}

void MotionLookaheadConfig::copy_from(MotionLookaheadConfig &that)
{
	block_x = that.block_x;
	block_y = that.block_y;
    block_w = that.block_w;
    block_h = that.block_h;
    range_w = that.range_w;
    range_h = that.range_h;
    enable = that.enable;
    do_rotate = that.do_rotate;
    rotation_range = that.rotation_range;
    draw_vectors = that.draw_vectors;
    frames = that.frames;
}

void MotionLookaheadConfig::interpolate(MotionLookaheadConfig &prev, 
	MotionLookaheadConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
    copy_from(prev);
}

void MotionLookaheadConfig::boundaries()
{
	CLAMP(block_w, MIN_BLOCK, MAX_BLOCK);
	CLAMP(block_h, MIN_BLOCK, MAX_BLOCK);
	CLAMP(range_w, MIN_RADIUS, MAX_RADIUS);
	CLAMP(range_h, MIN_RADIUS, MAX_RADIUS);
    CLAMP(rotation_range, MIN_ROTATION, MAX_ROTATION);
    CLAMP(frames, MIN_LOOKAHEAD, MAX_LOOKAHEAD);
}


MotionLookahead::MotionLookahead(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	rotate_engine = 0;
    translate_engine = 0;
    frames_read = 0;
    frames_start = 0;

    prev_block_x = -1;
    prev_block_y = -1;
    prev_block_w = -1;
    prev_block_h = -1;
    prev_range_w = -1;
    prev_range_h = -1;
    prev_do_rotate = -1;
    prev_rotation_range = -1;
    prev_direction = -1;

	total_dx = 0;
	total_dy = 0;
	total_angle = 0;
    center_dx = 0;
    center_dy = 0;
    center_angle = 0;
    frames_scanned = 0;
    
    current_dx = 0;
    current_dy = 0;
    current_angle = 0;
}

MotionLookahead::~MotionLookahead()
{
	delete engine;
	delete rotate_engine;
    delete translate_engine;
    frames.remove_all_objects();
    vectors.remove_all_objects();
}

const char* MotionLookahead::plugin_title() { return N_("Super Stabilizer"); }
int MotionLookahead::is_realtime() { return 1; }

NEW_WINDOW_MACRO(MotionLookahead, MotionLookaheadWindow)

LOAD_CONFIGURATION_MACRO(MotionLookahead, MotionLookaheadConfig)


void MotionLookahead::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("STABILIZER");

	output.tag.set_property("BLOCK_X", config.block_x);
	output.tag.set_property("BLOCK_Y", config.block_y);
	output.tag.set_property("BLOCK_W", config.block_w);
	output.tag.set_property("BLOCK_H", config.block_h);
	output.tag.set_property("RANGE_W", config.range_w);
	output.tag.set_property("RANGE_H", config.range_h);
	output.tag.set_property("ENABLE", config.enable);
	output.tag.set_property("ROTATE", config.do_rotate);
	output.tag.set_property("ROTATION_RANGE", config.rotation_range);
	output.tag.set_property("DRAW_VECTORS", config.draw_vectors);
	output.tag.set_property("FRAMES", config.frames);
	output.append_tag();
	output.terminate_string();
}

void MotionLookahead::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("STABILIZER"))
			{
				config.block_x = input.tag.get_property("BLOCK_X", config.block_x);
				config.block_y = input.tag.get_property("BLOCK_Y", config.block_y);
				config.block_w = input.tag.get_property("BLOCK_W", config.block_w);
				config.block_h = input.tag.get_property("BLOCK_H", config.block_h);
				config.range_w = input.tag.get_property("RANGE_W", config.range_w);
				config.range_h = input.tag.get_property("RANGE_H", config.range_h);
				config.rotation_range = input.tag.get_property("ROTATION_RANGE", config.rotation_range);
				config.enable = input.tag.get_property("ENABLE", config.enable);
				config.do_rotate = input.tag.get_property("ROTATE", config.do_rotate);
				config.draw_vectors = input.tag.get_property("DRAW_VECTORS", config.draw_vectors);
				config.frames = input.tag.get_property("FRAMES", config.frames);
			}
		}
	}
	config.boundaries();
}

void MotionLookahead::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
            MotionLookaheadWindow *gui = (MotionLookaheadWindow*)thread->window;
			gui->lock_window("MotionLookahead::update_gui");
			gui->block_x->update(config.block_x);
			gui->block_y->update(config.block_y);
            gui->block_w->update(config.block_w);
			gui->block_h->update(config.block_h);
            gui->range_w->update(config.range_w);
			gui->range_h->update(config.range_h);
			gui->frames->update(config.frames);
			gui->enable->update(config.enable);
			gui->do_rotate->update(config.do_rotate);
			gui->draw_vectors->update(config.draw_vectors);
			gui->rotation_range->update(config.rotation_range);
			thread->window->unlock_window();
		}
	}
}


int MotionLookahead::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	int h = frame->get_h();
	int w = frame->get_w();
	int color_model = frame->get_color_model();
	int need_reconfigure = load_configuration();

    if(config.enable)
    {
    // reset the vectors in certain circumstances
        int reset_vectors = 0;
        int reset_accums = 0;
        if(!EQUIV(prev_block_x, config.block_x) ||
            !EQUIV(prev_block_y, config.block_y) ||
            prev_block_w != config.block_w ||
            prev_block_h != config.block_h ||
            prev_range_w != config.range_w ||
            prev_range_h != config.range_h ||
            prev_do_rotate != config.do_rotate ||
            prev_rotation_range != config.rotation_range ||
            prev_direction != get_direction())
        {
            reset_vectors = 1;
            reset_accums = 1;
        }

// reset the lookahead buffer if we changed direction        
        if(get_direction() != prev_direction)
        {
            frames_read = 0;
        }

        prev_block_x = config.block_x;
        prev_block_y = config.block_y;
        prev_block_w = config.block_w;
        prev_block_h = config.block_h;
        prev_range_w = config.range_w;
        prev_range_h = config.range_h;
        prev_do_rotate = config.do_rotate;
        prev_rotation_range = config.rotation_range;
        prev_direction = get_direction();


// reset the accumulators in certain circumstances
        int64_t prev_position = start_position;
// last frame to use for predictions
        int64_t end_position = 0;
// total frames to use for predictions
        int prediction_frames = 0;
	    if(get_direction() == PLAY_FORWARD)
	    {
		    prev_position--;
            end_position = start_position + config.frames;

            int64_t source_start = get_source_start();
            int64_t source_end = source_start + get_total_len();
            KeyFrame *keyframe = get_next_keyframe(start_position, 1);
//printf("MotionLookahead::process_buffer %d start_position=%d source_start=%d source_end=%d\n", 
//__LINE__, (int)start_position, (int)source_start, (int)source_end);

// end of plugin is before end of buffer
            if(end_position > source_end)
                end_position = source_end;

// next keyframe is before end of buffer
            if(keyframe->position > start_position && 
                keyframe->position < end_position)
                end_position = keyframe->position;

// 1st frame of plugin
		    if(prev_position < source_start)
		    {
        	    reset_accums = 1;
		    }
            else
		    {
// keyframe on the current frame
			    keyframe = get_prev_keyframe(start_position, 1);
			    if(keyframe->position > 0 &&
				    prev_position < keyframe->position)
				    reset_accums = 1;
		    }
            
            prediction_frames = end_position - start_position;
	    }
	    else
	    {
		    prev_position++;
            end_position = start_position - config.frames;
            int64_t source_start = get_source_start();
            int64_t source_end = source_start + get_total_len();
            KeyFrame *keyframe = get_prev_keyframe(start_position, 1);
// printf("MotionLookahead::process_buffer %d start_position=%d source_start=%d source_end=%d keyframe=%d\n", 
// __LINE__, 
// (int)start_position, 
// (int)source_start, 
// (int)source_end, 
// (int)keyframe->position);

// start of plugin is before end of lookahead buffer
            if(source_start > end_position)
                end_position = source_start;

// next keyframe is before end of lookahead buffer
            if(keyframe->position < start_position && 
                keyframe->position > end_position)
                end_position = keyframe->position;



// 1st frame of plugin
		    if(prev_position >= source_end)
			{
                reset_accums = 1;
		    }
            else
		    {
// keyframe on the current frame
			    keyframe = get_next_keyframe(start_position, 1);
			    if(keyframe->position > 0 &&
				    prev_position >= keyframe->position)
				    reset_accums = 1;
		    }
            
            prediction_frames = start_position - end_position;
	    }


        int replay = 0;
        if(get_direction() == PLAY_FORWARD &&
            start_position > frames_start && 
            start_position < frames_start + config.frames)
        {
// shift the lookahead buffers for a seek before we invalidate any data
            int diff = start_position - frames_start;
            VFrame *temp_frame = frames.get(0);
            for(int i = 0; i < frames.size() - diff; i++)
                frames.set(i, frames.get(i + diff));
            frames.set(frames.size() - diff, temp_frame);

// invalidate motion vectors
            MotionVector *temp_vector = vectors.get(0);
            for(int i = 0; i < vectors.size() - diff; i++)
                vectors.set(i, vectors.get(i + diff));
            vectors.set(vectors.size() - diff, temp_vector);

            frames_start += diff;
            frames_read -= diff;
            frames_scanned -= diff;
        }
        else
        if(get_direction() == PLAY_REVERSE &&
            start_position < frames_start && 
            start_position > frames_start - config.frames)
        {
// shift the lookahead buffers for a seek before we invalidate any data
            int diff = frames_start - start_position;
            VFrame *temp_frame = frames.get(0);
            for(int i = 0; i < frames.size() - diff; i++)
                frames.set(i, frames.get(i + diff));
            frames.set(frames.size() - diff, temp_frame);

// invalidate motion vectors
            MotionVector *temp_vector = vectors.get(0);
            for(int i = 0; i < vectors.size() - diff; i++)
                vectors.set(i, vectors.get(i + diff));
            vectors.set(vectors.size() - diff, temp_vector);

            frames_start -= diff;
            frames_read -= diff;
            frames_scanned -= diff;
        }
        else
        if(start_position == frames_start)
        {
// if buffers aren't changing, replay the last frame
            if(frames_read >= config.frames &&
                frames_scanned >= config.frames) replay = 1;
        }
        else
        {
// invalidate all the data
            frames_start = start_position;
            frames_read = 0;
            frames_scanned = 0;
        }

	    if(reset_accums)
        {
	        total_dx = 0;
	        total_dy = 0;
	        total_angle = 0;
            center_dx = 0;
            center_dy = 0;
            center_angle = 0;
            replay = 0;
        }

        if(reset_vectors)
        {
            frames_scanned = 0;
            replay = 0;
        }

    // initialize the lookahead buffer
        if(frames_read == 0)
        {
            frames_start = start_position;
        }

//printf("MotionLookahead::process_buffer %d frames_read=%d config.frames=%d\n", 
//__LINE__, frames_read, config.frames);

// fill lookahead buffer
        while(frames_read < config.frames)
        {
// get the destination
            VFrame *dst = 0;
            if(frames.size() < frames_read + 1)
            {
// allocate shared memory
                dst = new VFrame(w, h, color_model);
                frames.append(dst);
            }
            else
            {
                dst = frames.get(frames_read);
            }

            int64_t position;
            if(get_direction() == PLAY_FORWARD)
                position = frames_start + frames_read;
            else
                position = frames_start - frames_read;
            read_frame(dst, 
			    0, 
			    position, 
			    frame_rate,
			    0);
            frames_read++;
printf("MotionLookahead::process_buffer %d position=%d frames_read=%d\n", 
__LINE__, 
(int)position, 
frames_read);
        }

// scan lookahead buffer
        while(frames_scanned < config.frames)
        {
            int current_dx = 0;
            int current_dy = 0;
            float current_angle = 0;

// skip 1st frame
            if(frames_scanned > 0)
            {
	            if(!engine) engine = new MotionScan(
                    PluginClient::get_project_smp() + 1,
		            PluginClient::get_project_smp() + 1);
                engine->scan_frame(frames.get(frames_scanned - 1), 
		            frames.get(frames_scanned),
		            config.range_w * w / 100,
		            config.range_h * h / 100,
		            config.block_w * w / 100,
		            config.block_h * h / 100,
		            config.block_x * w / 100,
		            config.block_y * h / 100,
		            MotionScan::PREVIOUS_SAME_BLOCK,
		            MotionScan::CALCULATE,
		            MotionScan::TRACK_PIXEL,
		            0,
		            0,
		            get_source_position(),
		            0,
		            0,
		            0,
		            0,
		            1, // do_motion
		            config.do_rotate, // do_rotate
		            0,
		            config.rotation_range);
	            current_dx = engine->dx_result;
	            current_dy = engine->dy_result;
                if(config.do_rotate) current_angle = engine->dr_result;
    // printf("MotionLookahead::process_buffer %d current_angle=%f\n", 
    // __LINE__,
    // current_angle);
            }

// get the vector destination
            MotionVector *dst = 0;
            if(vectors.size() < frames_scanned + 1)
            {
// allocate shared memory
                dst = new MotionVector;
                vectors.append(dst);
            }
            else
            {
                dst = vectors.get(frames_scanned);
            }

            if(dst)
            {
                dst->dx_result = current_dx;
                dst->dy_result = current_dy;
                dst->angle_result = current_angle;
            }

            frames_scanned++;
printf("MotionLookahead::process_buffer %d frames_scanned=%d dx=%d dy=%d angle=%f\n", 
__LINE__, frames_scanned, current_dx, current_dy, current_angle);
        }

// step the total accumulated motion forward
        if(!replay)
        {
            MotionVector *current = vectors.get(0);
            if(reset_accums)
            {
// set 1st vector to 0
                current->dx_result = 0;
                current->dy_result = 0;
                current->angle_result = 0;
            }

            total_dx += current->dx_result;
            total_dy += current->dy_result;
            total_angle += current->angle_result;

// compute the future center
            float future_dx = total_dx;
            float future_dy = total_dy;
            float future_angle = total_angle;
            LeastSquares least_squares_x(future_dx);
            LeastSquares least_squares_y(future_dy);
            LeastSquares least_squares_angle(future_angle);
            for(int i = 1; i < prediction_frames; i++)
            {
                MotionVector *src = vectors.get(i);
                future_dx += src->dx_result;
                future_dy += src->dy_result;
                future_angle += src->angle_result;
                least_squares_x.append(future_dx);
                least_squares_y.append(future_dy);
                least_squares_angle.append(future_angle);
            }


// future position from least squares
            future_dx = least_squares_x.get_b() + least_squares_x.get_m() * prediction_frames;
            future_dy = least_squares_y.get_b() + least_squares_y.get_m() * prediction_frames;
            future_angle = least_squares_angle.get_b() + least_squares_angle.get_m() * prediction_frames;
printf("MotionLookahead::process_buffer %d prediction_frames=%d center_dx=%f center_dy=%f center_angle=%f\n", 
__LINE__, 
prediction_frames,
future_dx, 
future_dy, 
future_angle);


// copy the current regressed center
            if(reset_accums)
            {
                center_dx = 0;
                center_dy = 0;
                center_angle = 0;
            }
            else
// throw away the last frame's center to make it more stable
            if(prediction_frames > 1)
            {
// blend the future position
//                float blend = prediction_frames - 1;
//                float inv_blend = 1;
//                center_dx = (center_dx * blend + future_dx * inv_blend) / prediction_frames;
//                center_dy = (center_dy * blend + future_dy * inv_blend) / prediction_frames;
//                center_angle = (center_angle * blend + future_angle * inv_blend) / prediction_frames;

// linear interpolate the future position
                center_dx += (future_dx - center_dx) / prediction_frames;
                center_dy += (future_dy - center_dy) / prediction_frames;
                center_angle += (future_angle - center_angle) / prediction_frames;
            }





// amount to shift current frame
            current_dx = (float)(total_dx - center_dx) / OVERSAMPLE;
            current_dy = (float)(total_dy - center_dy) / OVERSAMPLE;
            current_angle = total_angle - center_angle;
    // printf("MotionLookahead::process_buffer %d reset_accums=%d center=%d %d %f current=%f %f %f\n", 
    // __LINE__, 
    // reset_accums, 
    // center_dx, 
    // center_dy, 
    // center_angle,
    // current_dx, 
    // current_dy, 
    // current_angle);
        }

	    frame->clear_frame();
        if(config.do_rotate)
        {
		    if(!rotate_engine)
			    rotate_engine = new AffineEngine(
                    PluginClient::get_project_smp() + 1,
				    PluginClient::get_project_smp() + 1);
	        int block_x = (int)(config.block_x * w / 100);
	        int block_y = (int)(config.block_y * h / 100);
            rotate_engine->set_in_pivot(block_x, block_y);
            rotate_engine->set_out_pivot(block_x + current_dx, block_y + current_dy);
            rotate_engine->rotate(frame, frames.get(0), current_angle);
        }
        else
        {
	        if(!translate_engine) 
		        translate_engine = new OverlayFrame(PluginClient::get_project_smp() + 1);
            translate_engine->overlay(frame,
		        frames.get(0),
		        0,
		        0,
		        w,
		        h,
		        current_dx,
		        current_dy,
		        (float)w + current_dx,
		        (float)h + current_dy,
		        1,
		        TRANSFER_REPLACE,
		        NEAREST_NEIGHBOR);
        }
    }
    else
    {
// !enable
        read_frame(frame, 
			0, 
			start_position, 
			frame_rate,
			0);
    }


	if(config.draw_vectors)
		draw_vectors(frame);

	return 0;
}

void MotionLookahead::draw_vectors(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();
	int block_x, block_y;
	int block_w, block_h;
	int scan_w, scan_h;
	int block_x1, block_y1;
	int block_x2, block_y2;
	int scan_x1, scan_y1;
	int scan_x2, scan_y2;
    int current_dx = 0;
    int current_dy = 0;
    int current_angle = 0;
	int current_x1, current_y1;
	int current_x2, current_y2;

	block_x = (int)(config.block_x * w / 100);
	block_y = (int)(config.block_y * h / 100);
	block_w = config.block_w * w / 100;
	block_h = config.block_h * h / 100;
	block_x1 = block_x - block_w / 2;
	block_y1 = block_y - block_h / 2;
	block_x2 = block_x + block_w / 2;
	block_y2 = block_y + block_h / 2;
	scan_w = config.range_w * w / 100;
	scan_h = config.range_h * h / 100;
	scan_x1 = block_x1 - scan_w / 2;
	scan_y1 = block_y1 - scan_h / 2;
	scan_x2 = block_x2 + scan_w / 2;
	scan_y2 = block_y2 + scan_h / 2;

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

    if(frames_scanned > 0)
    {
        current_dx = -vectors.get(0)->dx_result;
        current_dy = -vectors.get(0)->dy_result;
        current_angle = -vectors.get(0)->angle_result;
    }

    int dst_x = block_x + current_dx / OVERSAMPLE;
    int dst_y = block_y + current_dy / OVERSAMPLE;
   	frame->draw_arrow(block_x, 
        block_y, 
        dst_x, 
        dst_y);

	frame->draw_rect(block_x1, block_y1, block_x2, block_y2);
	frame->draw_rect(scan_x1, scan_y1, scan_x2, scan_y2);

	if(config.do_rotate)
	{
		float angle = current_angle * 2 * M_PI / 360;
		double base_angle1 = atan((float)block_h / block_w);
		double base_angle2 = atan((float)block_w / block_h);
		double target_angle1 = base_angle1 + angle;
		double target_angle2 = base_angle2 + angle;
		double radius = sqrt(block_w * block_w + block_h * block_h) / 2;
		int rotated_x1 = (int)(dst_x - cos(target_angle1) * radius);
		int rotated_y1 = (int)(dst_y - sin(target_angle1) * radius);
		int rotated_x2 = (int)(dst_x + sin(target_angle2) * radius);
		int rotated_y2 = (int)(dst_y - cos(target_angle2) * radius);
		int rotated_x3 = (int)(dst_x - sin(target_angle2) * radius);
		int rotated_y3 = (int)(dst_y + cos(target_angle2) * radius);
		int rotated_x4 = (int)(dst_x + cos(target_angle1) * radius);
		int rotated_y4 = (int)(dst_y + sin(target_angle1) * radius);

		frame->draw_line(rotated_x1, rotated_y1, rotated_x2, rotated_y2);
		frame->draw_line(rotated_x2, rotated_y2, rotated_x4, rotated_y4);
		frame->draw_line(rotated_x4, rotated_y4, rotated_x3, rotated_y3);
		frame->draw_line(rotated_x3, rotated_y3, rotated_x1, rotated_y1);
    }
}





















