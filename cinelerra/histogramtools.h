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


// Functions for drawing histograms & processing color curves

#ifndef HISTOGRAMTOOLS_H
#define HISTOGRAMTOOLS_H

#include "cicolors.h"
#include "guicast.h"
#include "histogramtools.inc"
#include "loadbalance.h"
#include "vframe.inc"

class HistogramScannerPackage : public LoadPackage
{
public:
	HistogramScannerPackage();
	int start, end;
};

class HistogramScannerUnit : public LoadClient
{
public:
	HistogramScannerUnit(HistogramTools *tools,
        HistogramScanner *server);
	~HistogramScannerUnit();
	void process_package(LoadPackage *package);
	HistogramScanner *server;
	HistogramTools *tools;
	int *accum[5];
};

class HistogramScanner : public LoadServer
{
public:
	HistogramScanner(HistogramTools *tools,
        int total_clients, 
		int total_packages);
	void process_packages(VFrame *data, int do_value);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	int total_size;
    HistogramTools *tools;
	VFrame *data;
	int do_value;
    YUV yuv;
};



class HistogramTools
{
public:
    HistogramTools();
    virtual ~HistogramTools();

    int initialized();
// generate lookup tables
    void initialize(int use_value);
// your job: override this for the processor
    virtual float calculate_level(float input, int mode, int do_value);
    void calculate_histogram(VFrame *data, int do_value);
    void draw(BC_WindowBase *canvas, 
        int y, 
        int h, 
        int mode,
        float min_input,
        float max_input);

// 8 bit RGB lookup table for applying curves
	int *lookup[HISTOGRAM_MODES];
// software YUV uses a 16 bit intermediate
	int *lookup16[HISTOGRAM_MODES];
// lookup table for computing input for the value histogram
	int *preview_lookup[HISTOGRAM_MODES];
// the histogram bins
	int *accum[HISTOGRAM_MODES];
    HistogramScanner *engine;
};


#endif

