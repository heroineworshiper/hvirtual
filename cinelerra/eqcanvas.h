/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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


// A canvas for drawing a spectrogram on top of a filter envelope

#ifndef EQCANVAS_H
#define EQCANVAS_H

#include "guicast.h"
#include "pluginclient.inc"

class EQCanvas
{
public:
    EQCanvas(BC_WindowBase *parent, 
        int x, 
        int y, 
        int w, 
        int h, 
        float min_db,
        float max_db);
    virtual ~EQCanvas();
    
    void initialize();
    void draw_grid();
    void update_spectrogram(PluginClient *plugin);
    void draw_envelope(double *envelope, 
        int samplerate,
        int window_size);
    
    BC_WindowBase *parent;
    BC_SubWindow *canvas;
    int x, y, w, h;
	int canvas_x;
	int canvas_y;
	int canvas_w;
	int canvas_h;
    float min_db;
    float max_db;
    int freq_divisions;
    int db_divisions;
    PluginClientFrame *last_frame;
};



#endif // EQCANVAS_H
