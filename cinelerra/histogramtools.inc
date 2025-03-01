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


// Functions for drawing histograms of images

#ifndef HISTOGRAMTOOLS_INC
#define HISTOGRAMTOOLS_INC

#define HISTOGRAM_MODES 4
#define HISTOGRAM_RED 0
#define HISTOGRAM_GREEN 1
#define HISTOGRAM_BLUE 2
#define HISTOGRAM_VALUE 3

// Number of divisions in histogram.  
// 65536 + min and max range to speed up the tabulation
#define HISTOGRAM_SLOTS 0x13333
#define FLOAT_RANGE 1.2
// Minimum value in percentage
#define HISTOGRAM_MIN -10
#define HISTOGRAM_MIN_INPUT -0.1
// Maximum value in percentage
#define HISTOGRAM_MAX 110
#define HISTOGRAM_MAX_INPUT 1.1


#define HISTOGRAM_MIN_GAMMA 0.0
#define HISTOGRAM_MAX_GAMMA 10.0


class HistogramTools;
class HistogramScanner;


#endif

