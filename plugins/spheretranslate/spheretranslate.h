
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

#ifndef SPHERETRANSLATE_H
#define SPHERETRANSLATE_H

// the simplest plugin possible

class SphereTranslateMain;

#include "bchash.h"
#include "mutex.h"
#include "translatewin.h"
#include "overlayframe.h"
#include "pluginvclient.h"

class SphereTranslateConfig
{
public:
	SphereTranslateConfig();
	int equivalent(SphereTranslateConfig &that);
	void copy_from(SphereTranslateConfig &that);
	void interpolate(SphereTranslateConfig &prev, 
		SphereTranslateConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);

	float translate_x, translate_y, translate_z;
	float rotate_x, rotate_y, rotate_z;
};


class SphereTranslateMain : public PluginVClient
{
public:
	SphereTranslateMain(PluginServer *server);
	~SphereTranslateMain();

// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS(SphereTranslateConfig)
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);


	OverlayFrame *overlayer;   // To translate images
};


#endif
