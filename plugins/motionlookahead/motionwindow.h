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

#ifndef MOTIONWINDOW_H
#define MOTIONWINDOW_H

#include "guicast.h"
#include "motion.inc"



class MotionSlider : public BC_ISlider
{
public:
    MotionSlider(MotionLookahead *plugin, 
		int x, 
		int y,
        int w,
        int *output,
        int min,
        int max);
	int handle_event();
	int *output;
	MotionLookahead *plugin;
};

class MotionCheck : public BC_CheckBox
{
public:
	MotionCheck(MotionLookahead *plugin, 
		int x, 
		int y,
        int *output,
        const char *text);
	int handle_event();
	int *output;
	MotionLookahead *plugin;
};

class MotionText : public BC_TextBox
{
public:
    MotionText(MotionLookahead *plugin, 
        int x,
        int y,
        float *output);
    MotionText(MotionLookahead *plugin, 
        int x,
        int y,
        int *output);
	int handle_event();
	MotionLookahead *plugin;
    float *output_f;
    int *output_i;
    MotionFPot *fpot;
    MotionIPot *ipot;
};

class MotionFPot : public BC_FPot
{
public:
	MotionFPot(MotionLookahead *plugin, 
		int x, 
		int y,
        float *output,
        float min,
        float max);
	int handle_event();
	MotionLookahead *plugin;
    float *output;
    MotionText *textbox;
};

class MotionIPot : public BC_IPot
{
public:
	MotionIPot(MotionLookahead *plugin, 
		int x, 
		int y,
        int *output,
        int min,
        int max);
	int handle_event();
	MotionLookahead *plugin;
    int *output;
    MotionText *textbox;
};

class MotionLookaheadWindow : public PluginClientWindow
{
public:
	MotionLookaheadWindow(MotionLookahead *plugin);
	~MotionLookaheadWindow();


    void create_objects();

    MotionLookahead *plugin;
    MotionCheck *do_rotate;
    MotionCheck *draw_vectors;
    MotionCheck *enable;
    MotionIPot *rotation_range;
    MotionFPot *block_x;
    MotionFPot *block_y;
    MotionIPot *block_w;
    MotionIPot *block_h;
    MotionIPot *range_w;
    MotionIPot *range_h;
    MotionSlider *frames;

};



#endif




