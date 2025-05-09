/*
 * CINELERRA
 * Copyright (C) 1997-2024 Adam Williams <broadcast at earthling dot net>
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


#ifndef CURVES_INC
#define CURVES_INC




// change curves.sl if you change this


#define CURVE_MAX 1.0
#define CURVE_MIN 0.0
#define POINT_W DP(10)

// maximum points for opengl
#define MAX_POINTS 16

#define WINDOW_W DP(400)
#define WINDOW_H DP(400)

class CurvesConfig;
class CurvesWindow;
class CurvesMain;
class CurvesEngine;


typedef struct
{
    float x, y;
} curve_point_t;


#endif



