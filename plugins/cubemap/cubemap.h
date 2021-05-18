/*
 * CINELERRA
 * Copyright (C) 2021 Adam Williams <broadcast at earthling dot net>
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


// this converts a 6 panel gootube cubemap into something watchable

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "affine.inc"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "vframe.h"

class CubeMap;
class CubeMapWindow;




class CubeMapConfig
{
public:
	CubeMapConfig();

	int equivalent(CubeMapConfig &that);
	void copy_from(CubeMapConfig &that);
	void interpolate(CubeMapConfig &prev, 
		CubeMapConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
    void boundaries();
// shift dest rects
    int x_shift;
    float y_scale;
};

class CubeMapInt : public BC_IPot
{
public:
	CubeMapInt(CubeMap *plugin, int *output, int x, int y, int min, int max);
	int handle_event();
	CubeMap *plugin;
    int *output;
};

class CubeMapFloat : public BC_FPot
{
public:
	CubeMapFloat(CubeMap *plugin, float *output, int x, int y, float min, float max);
	int handle_event();
	CubeMap *plugin;
    float *output;
};

class CubeMapWindow : public PluginClientWindow
{
public:
	CubeMapWindow(CubeMap *plugin);
	~CubeMapWindow();

	void create_objects();
	int resize_event(int x, int y);
    void update();
	CubeMap *plugin;
    CubeMapInt *x_shift;
    CubeMapFloat *y_scale;
};





// translate source regions to dest regions
typedef struct {
    int src_x1, src_y1, src_x2, src_y2;
    int dst_x1, dst_y1, dst_x2, dst_y2;
// degrees
    int rotation;
} cubetable_t;
#define TOTAL_FACES 4

class CubeMap : public PluginVClient
{
public:
	CubeMap(PluginServer *server);
	~CubeMap();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS2(CubeMapConfig)

	VFrame *temp;
	AffineEngine *engine;
    cubetable_t translation[TOTAL_FACES];
};












