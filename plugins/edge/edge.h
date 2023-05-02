/*
 * CINELERRA
 * Copyright (C) 1997-2021 Adam Williams <broadcast at earthling dot net>
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



#ifndef EDGE_H
#define EDGE_H

#include "pluginvclient.h"

class EdgeEngine;
class Edge;

class EdgeConfig
{
public:
	EdgeConfig();

	int equivalent(EdgeConfig &that);
	void copy_from(EdgeConfig &that);
	void interpolate(EdgeConfig &prev, 
		EdgeConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	void limits();


	int amount;
};

class Edge : public PluginVClient
{
public:
	Edge(PluginServer *server);
	~Edge();
// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS2(EdgeConfig)
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);

	EdgeEngine *engine;
	VFrame *temp;
};



#endif



