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




#include "clip.h"
#include "histogramtools.h"
#include "mwindow.h"
#include "preferences.h"
#include <string.h>

HistogramTools::HistogramTools()
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		lookup[i] = 0;
		lookup16[i] = 0;
		accum[i] = 0;
		preview_lookup[i] = 0;
	}
    engine = 0;
}

HistogramTools::~HistogramTools()
{
	for(int i = 0; i < HISTOGRAM_MODES;i++)
	{
		delete [] lookup[i];
		delete [] lookup16[i];
		delete [] accum[i];
		delete [] preview_lookup[i];
	}
    delete engine;
}

int HistogramTools::initialized()
{
    return lookup[0] && lookup16[0] && accum[0];
}

void HistogramTools::initialize(int use_value)
{
	if(!accum[0])
	{
		for(int i = 0; i < HISTOGRAM_MODES; i++)
			accum[i] = new int[HISTOGRAM_SLOTS];
	}


    for(int mode = 0; mode < 3; mode++)
    {
	    if(!lookup[mode])
		    lookup[mode] = new int[HISTOGRAM_SLOTS];
	    if(!lookup16[mode])
		    lookup16[mode] = new int[HISTOGRAM_SLOTS];
	    if(!preview_lookup[mode])
		    preview_lookup[mode] = new int[HISTOGRAM_SLOTS];
        

// Generate lookup tables for integer colormodels
	    for(int i = 0; i < 256; i++)
	    {
		    lookup[mode][i] = 
			    (int)(calculate_level((float)i / 0xff, mode, use_value) * 
			    0xff);
		    CLAMP(lookup[mode][i], 0, 0xff);
	    }


        for(int i = 0; i < 65536; i++)
        {
	        lookup16[mode][i] = 
		        (int)(calculate_level((float)i / 0xffff, mode, use_value) * 
		        0xffff);
	        CLAMP(lookup16[mode][i], 0, 0xffff);
        }
        

// Lookup table for preview only used for GUI
	    if(!use_value)
	    {
		    for(int i = 0; i < 0x10000; i++)
		    {
			    preview_lookup[mode][i] = 
				    (int)(calculate_level((float)i / 0xffff, mode, use_value) * 
				    0xffff);
			    CLAMP(preview_lookup[mode][i], 0, 0xffff);
		    }
	    }
        
    }
}


float HistogramTools::calculate_level(float input, int mode, int do_value)
{
    return 0;
}

void HistogramTools::calculate_histogram(VFrame *data, int do_value)
{
	if(!engine) engine = new HistogramScanner(this,
		MWindow::preferences->processors,
		MWindow::preferences->processors);

	engine->process_packages(data, 1);

	for(int i = 0; i < engine->get_total_clients(); i++)
	{
		HistogramScannerUnit *unit = (HistogramScannerUnit*)engine->get_client(i);

		if(i == 0)
		{
			for(int j = 0; j < HISTOGRAM_MODES; j++)
			{
				memcpy(accum[j], unit->accum[j], sizeof(int) * HISTOGRAM_SLOTS);
			}
		}
		else
		{
			for(int j = 0; j < HISTOGRAM_MODES; j++)
			{
				int *out = accum[j];
				int *in = unit->accum[j];
				for(int k = 0; k < HISTOGRAM_SLOTS; k++)
					out[k] += in[k];
			}
		}
	}

// Remove top and bottom from calculations.  Doesn't work in high
// precision colormodels.
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		accum[i][0] = 0;
		accum[i][HISTOGRAM_SLOTS - 1] = 0;
	}
}

void HistogramTools::draw(BC_WindowBase *canvas, 
    int y, 
    int h, 
    int mode,
    float min_input,
    float max_input)
{
    int colors[] = { RED, GREEN, BLUE, MEGREY };
    int color = colors[mode];
    int canvas_w = canvas->get_w();

	int normalize = 0;
	int max = 0;
    int start_bin = (min_input - HISTOGRAM_MIN_INPUT) * 
        HISTOGRAM_SLOTS / 
        (HISTOGRAM_MAX_INPUT - HISTOGRAM_MIN_INPUT);
    int end_bin = (max_input - HISTOGRAM_MIN_INPUT) * 
        HISTOGRAM_SLOTS / 
        (HISTOGRAM_MAX_INPUT - HISTOGRAM_MIN_INPUT);

	int accum_per_canvas_i = (end_bin - start_bin) / canvas_w + 1;
	float accum_per_canvas_f = (float)(end_bin - start_bin) / canvas_w;

    if(!initialized()) return;

	for(int i = start_bin; i < end_bin; i++)
	{
		if(accum[mode][i] > normalize) normalize = accum[mode][i];
	}

// printf("HistogramTools::draw %d start_bin=%d end_bin=%d normalize=%d\n", 
// __LINE__, 
// start_bin, 
// end_bin,
// normalize);


	if(normalize)
	{
		canvas->set_color(color);
		for(int i = 0; i < canvas_w; i++)
		{
			int accum_start = (int)(start_bin + accum_per_canvas_f * i);
			int accum_end = accum_start + accum_per_canvas_i;
			max = 0;
			for(int j = accum_start; j < accum_end; j++)
			{
				max = MAX(accum[mode][j], max);
			}

//			max = max * h / normalize;
			max = (int)(log(max) / log(normalize) * h);

			canvas->draw_line(i, y + h - max, i, y + h);
		}
	}
}















HistogramScannerPackage::HistogramScannerPackage()
 : LoadPackage()
{
}




HistogramScannerUnit::HistogramScannerUnit(HistogramTools *tools,
    HistogramScanner *server)
 : LoadClient(server)
{
    this->tools = tools;
	this->server = server;
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
    	accum[i] = new int[HISTOGRAM_SLOTS];
    }
}

HistogramScannerUnit::~HistogramScannerUnit()
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
		delete [] accum[i];
}

void HistogramScannerUnit::process_package(LoadPackage *package)
{
	HistogramScannerPackage *pkg = (HistogramScannerPackage*)package;

	int do_value = server->do_value;


#define HISTOGRAM_HEAD(type) \
{ \
    for(int i = pkg->start; i < pkg->end; i++) \
    { \
	    type *row = (type*)data->get_rows()[i]; \
	    for(int j = 0; j < w; j++) \
	    {

#define HISTOGRAM_TAIL(components) \
    /* Value takes the maximum of the output RGB values */ \
		    if(do_value) \
		    { \
			    CLAMP(r, 0, HISTOGRAM_SLOTS - 1); \
			    CLAMP(g, 0, HISTOGRAM_SLOTS - 1); \
			    CLAMP(b, 0, HISTOGRAM_SLOTS - 1); \
			    r_out = lookup_r[r]; \
			    g_out = lookup_g[g]; \
			    b_out = lookup_b[b]; \
    /*				v = (r * 76 + g * 150 + b * 29) >> 8; */ \
			    v = MAX(r_out, g_out); \
			    v = MAX(v, b_out); \
			    v += -HISTOGRAM_MIN * 0xffff / 100; \
			    CLAMP(v, 0, HISTOGRAM_SLOTS - 1); \
			    accum_v[v]++; \
		    } \
    \
		    r += -HISTOGRAM_MIN * 0xffff / 100; \
		    g += -HISTOGRAM_MIN * 0xffff / 100; \
		    b += -HISTOGRAM_MIN * 0xffff / 100; \
		    CLAMP(r, 0, HISTOGRAM_SLOTS - 1); \
		    CLAMP(g, 0, HISTOGRAM_SLOTS - 1); \
		    CLAMP(b, 0, HISTOGRAM_SLOTS - 1); \
		    accum_r[r]++; \
		    accum_g[g]++; \
		    accum_b[b]++; \
		    row += components; \
	    } \
    } \
}




	VFrame *data = server->data;
	int w = data->get_w();
	int h = data->get_h();
	int *accum_r = accum[HISTOGRAM_RED];
	int *accum_g = accum[HISTOGRAM_GREEN];
	int *accum_b = accum[HISTOGRAM_BLUE];
	int *accum_v = accum[HISTOGRAM_VALUE];
	int32_t r, g, b, a, y, u, v;
	int r_out, g_out, b_out;
	int *lookup_r = tools->preview_lookup[HISTOGRAM_RED];
	int *lookup_g = tools->preview_lookup[HISTOGRAM_GREEN];
	int *lookup_b = tools->preview_lookup[HISTOGRAM_BLUE];

	switch(data->get_color_model())
	{
		case BC_RGB888:
			HISTOGRAM_HEAD(unsigned char)
			r = (row[0] << 8) | row[0];
			g = (row[1] << 8) | row[1];
			b = (row[2] << 8) | row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGB_FLOAT:
			HISTOGRAM_HEAD(float)
			r = (int)(row[0] * 0xffff);
			g = (int)(row[1] * 0xffff);
			b = (int)(row[2] * 0xffff);
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUV888:
			HISTOGRAM_HEAD(unsigned char)
			y = (row[0] << 8) | row[0];
			u = (row[1] << 8) | row[1];
			v = (row[2] << 8) | row[2];
			server->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGBA8888:
			HISTOGRAM_HEAD(unsigned char)
			r = (row[0] << 8) | row[0];
			g = (row[1] << 8) | row[1];
			b = (row[2] << 8) | row[2];
			HISTOGRAM_TAIL(4)
			break;
		case BC_RGBA_FLOAT:
			HISTOGRAM_HEAD(float)
			r = (int)(row[0] * 0xffff);
			g = (int)(row[1] * 0xffff);
			b = (int)(row[2] * 0xffff);
			HISTOGRAM_TAIL(4)
			break;
		case BC_YUVA8888:
			HISTOGRAM_HEAD(unsigned char)
			y = (row[0] << 8) | row[0];
			u = (row[1] << 8) | row[1];
			v = (row[2] << 8) | row[2];
			server->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(4)
			break;
	}
}







HistogramScanner::HistogramScanner(HistogramTools *tools,
    int total_clients, 
	int total_packages)
// : LoadServer(total_clients, total_packages)
 : LoadServer(1, 1)
{
    this->tools = tools;
}

void HistogramScanner::init_packages()
{
	total_size = data->get_h();
	int package_size = (int)((float)total_size / 
			get_total_packages() + 1);
	int start = 0;

	for(int i = 0; i < get_total_packages(); i++)
	{
		HistogramScannerPackage *package = (HistogramScannerPackage*)get_package(i);
		package->start = total_size * i / get_total_packages();
		package->end = total_size * (i + 1) / get_total_packages();
	}

// Initialize clients here in case some don't get run.
	for(int i = 0; i < get_total_clients(); i++)
	{
		HistogramScannerUnit *unit = (HistogramScannerUnit*)get_client(i);
		for(int i = 0; i < HISTOGRAM_MODES; i++)
			bzero(unit->accum[i], sizeof(int) * HISTOGRAM_SLOTS);
	}
}

LoadClient* HistogramScanner::new_client()
{
	return new HistogramScannerUnit(tools, this);
}

LoadPackage* HistogramScanner::new_package()
{
	return new HistogramScannerPackage;
}

void HistogramScanner::process_packages(VFrame *data, int do_value)
{
	this->data = data;
	this->do_value = do_value;
	LoadServer::process_packages();
}
