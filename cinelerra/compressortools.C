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


// Objects for compressors
#include "clip.h"
#include "compressortools.h"
#include "cursors.h"
#include "language.h"
#include "pluginclient.h"
#include "theme.h"


BandConfig::BandConfig()
{
    freq = 0;
    solo = 0;
    bypass = 0;
}

BandConfig::~BandConfig()
{
    
}

void BandConfig::copy_from(BandConfig *src)
{
	levels.remove_all();
	for(int i = 0; i < src->levels.total; i++)
	{
    	levels.append(src->levels.values[i]);
    }

    freq = src->freq;
    solo = src->solo;
    bypass = src->bypass;
}

int BandConfig::equiv(BandConfig *src)
{
    if(levels.total != src->levels.total ||
        solo != src->solo ||
        bypass != src->bypass ||
        freq != src->freq)
    {
        return 0;
    }


	for(int i = 0; 
		i < levels.total && i < src->levels.total; 
		i++)
	{
		compressor_point_t *this_level = &levels.values[i];
		compressor_point_t *that_level = &src->levels.values[i];
		if(!EQUIV(this_level->x, that_level->x) ||
			!EQUIV(this_level->y, that_level->y))
		{
        	return 0;
        }
	}
    
    return 1;
}




CompressorConfigBase::CompressorConfigBase(int total_bands)
{
    this->total_bands = total_bands;
    bands = new BandConfig[total_bands];
	min_db = -78.0;
    max_db = 6.0;
    for(int band = 0; band < total_bands; band++)
    {
        bands[band].freq = Freq::tofreq((band + 1) * TOTALFREQS / total_bands);
    }
    current_band = 0;
}


CompressorConfigBase::~CompressorConfigBase()
{
    delete [] bands;
}


double CompressorConfigBase::get_y(int band, int number)
{
    BandConfig *ptr = &bands[band];
	if(!ptr->levels.total) 
    {
		return 1.0;
	}
    else
	if(number >= ptr->levels.total)
	{
    	return ptr->levels.values[ptr->levels.total - 1].y;
	}
    else
	{
    	return ptr->levels.values[number].y;
    }
}

double CompressorConfigBase::get_x(int band, int number)
{
    BandConfig *ptr = &bands[band];
	if(!ptr->levels.total)
    {
		return 0.0;
	}
    else
	if(number >= ptr->levels.total)
	{
    	return ptr->levels.values[ptr->levels.total - 1].x;
	}
    else
	{
    	return ptr->levels.values[number].x;
    }
}

double CompressorConfigBase::calculate_db(int band, double x)
{
    ArrayList<compressor_point_t> *levels = &bands[band].levels;

    if(!levels->total)
    {
		return x;
    }
    else
// the only point.  Use slope from min_db
    if(levels->total == 1)
    {
        compressor_point_t *point = &levels->values[0];
        return point->y +
            (x - point->x) *
            (point->y - min_db) / 
            (point->x - min_db);
    }

	for(int i = levels->total - 1; i >= 0; i--)
	{
        compressor_point_t *point = &levels->values[i];
		if(point->x <= x)
		{
// between 2 points.  Use slope between 2 points
			if(i < levels->total - 1)
			{
                compressor_point_t *next_point = &levels->values[i + 1];
				return point->y + 
					(x - point->x) *
					(next_point->y - point->y) / 
					(next_point->x - point->x);
			}
			else
			{
// the last point.  Use slope of last 2 points
                compressor_point_t *prev_point = &levels->values[i - 1];
				return point->y +
					(x - point->x) * 
					(point->y - prev_point->y) / 
					(point->x - prev_point->x);
			}
		}
	}

// before 1st point.  Use slope between 1st point & min_db
    compressor_point_t *point = &levels->values[0];
    return point->y +
        (x - point->x) *
        (point->y - min_db) / 
        (point->x - min_db);
}


int CompressorConfigBase::set_point(int band, double x, double y)
{
    BandConfig *ptr = &bands[band];
	for(int i = ptr->levels.total - 1; i >= 0; i--)
	{
		if(ptr->levels.values[i].x < x)
		{
			ptr->levels.append();
			i++;
			for(int j = ptr->levels.total - 2; j >= i; j--)
			{
				ptr->levels.values[j + 1] = ptr->levels.values[j];
			}
			ptr->levels.values[i].x = x;
			ptr->levels.values[i].y = y;

			return i;
		}
	}

	ptr->levels.append();
	for(int j = ptr->levels.total - 2; j >= 0; j--)
	{
		ptr->levels.values[j + 1] = ptr->levels.values[j];
	}
	ptr->levels.values[0].x = x;
	ptr->levels.values[0].y = y;
	return 0;
}

void CompressorConfigBase::remove_point(int band, int number)
{
    BandConfig *ptr = &bands[band];
	for(int j = number; j < ptr->levels.total - 1; j++)
	{
		ptr->levels.values[j] = ptr->levels.values[j + 1];
	}
	ptr->levels.remove();
}



double CompressorConfigBase::calculate_output(int band, double x)
{
    double x_db = DB::todb(x);
    return DB::fromdb(calculate_db(band, x_db));
}


double CompressorConfigBase::calculate_gain(int band, double input)
{
	double y_linear = calculate_output(band, input);
	double gain;
// limit the gain to a sane number
	if(fabs(input - 0.0) > 0.000001)
	{
    	gain = y_linear / input;
	}
    else
    {
		gain = 100000;
    }

	return gain;
}














CompressorCanvasBase::CompressorCanvasBase(CompressorConfigBase *config, 
    PluginClient *plugin,
    PluginClientWindow *window, 
    int x, 
    int y, 
    int w, 
    int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->config = config;
    this->plugin = plugin;
    this->window = window;
	current_operation = NONE;

    graph_x = 0;
    graph_y = 0;
    graph_w = w - graph_x;
    graph_h = h - graph_y;
    subdivisions = 6;
    divisions = (int)(config->max_db - config->min_db) / subdivisions;
}

void CompressorCanvasBase::create_objects()
{
	set_cursor(CROSS_CURSOR, 0, 0);
    draw_scales();
    update();
}

void CompressorCanvasBase::draw_scales()
{
	window->set_font(SMALLFONT);
	window->set_color(get_resources()->default_text_color);

    int big_line = DP(10);
    int small_line = DP(5);
// output divisions
	for(int i = 0; i <= divisions; i++)
	{
		int y = get_y() + DP(10) + graph_y + graph_h / divisions * i;
		int x = get_x() - big_line;
		char string[BCTEXTLEN];
		
		sprintf(string, "%.0f", config->max_db - 
            (float)i / divisions * 
            (config->max_db - config->min_db));
		int text_w = get_text_width(SMALLFONT, string);
		window->draw_text(x - text_w, y, string);
		
		int y1 = get_y() + graph_y + graph_h / divisions * i;
		int y2 = get_y() + graph_y + graph_h / divisions * (i + 1);
		for(int j = 0; j < subdivisions; j++)
		{
			y = y1 + (y2 - y1) * j / subdivisions;
			if(j == 0)
			{
				window->draw_line(get_x() - big_line, y, get_x(), y);
			}
			else
			if(i < divisions)
			{
				window->draw_line(get_x() - small_line, y, get_x(), y);
			}
		}
	}

// input divisions
	for(int i = 0; i <= divisions; i++)
	{
		int y = get_y() + get_h();
		int x = get_x() + graph_x + graph_w * i / divisions;
        int y1 = y + window->get_text_ascent(SMALLFONT);
		char string[BCTEXTLEN];

		sprintf(string, 
            "%.0f", 
            (float)i / divisions * 
                (config->max_db - config->min_db) + config->min_db);
		int text_w = get_text_width(SMALLFONT, string);
        window->draw_text(x - text_w, y1 + big_line, string);

		int x1 = get_x() + graph_x + graph_w * i / divisions;
		int x2 = get_x() + graph_x + graph_w * (i + 1) / divisions;
		for(int j = 0; j < subdivisions; j++)
		{
			x = x1 + (x2 - x1) * j / subdivisions;
			if(j == 0)
			{
				window->draw_line(x, 
                    y, 
                    x, 
                    y + big_line);
			}
			else
			if(i < divisions)
			{
				window->draw_line(x, 
                    y, 
                    x, 
                    y + small_line);
			}
		}
	}


}

#define POINT_W DP(10)

// get Y from X
int CompressorCanvasBase::x_to_y(int band, int x)
{
	double x_db = config->min_db + (double)x / graph_w * 
        (config->max_db - config->min_db);
	double y_db = config->calculate_db(band, x_db);
	int y = graph_y + graph_h - 
        (int)((y_db - config->min_db) * 
        graph_h / 
        (config->max_db - config->min_db)); 

//printf("CompressorCanvasBase::x_to_y %d x=%d x_db=%f y_db=%f y=%d\n", 
//__LINE__, x, x_db, y_db, y);
    return y;
}

// get X from DB
int CompressorCanvasBase::db_to_x(double db)
{
    int x = graph_x + 
        (double)(db - config->min_db) *
        graph_w /
        (config->max_db - config->min_db);
    return x;
}

// get Y from DB
int CompressorCanvasBase::db_to_y(double db)
{
    int y = graph_y + graph_h - 
        (int)((db - config->min_db) * 
        graph_h / 
        (config->max_db - config->min_db)); 

//printf("CompressorCanvasBase::x_to_y %d x=%d x_db=%f y_db=%f y=%d\n", 
//__LINE__, x, x_db, y_db, y);
    return y;
}


double CompressorCanvasBase::x_to_db(int x)
{
	CLAMP(x, 0, get_w());
	double x_db = (double)(x - graph_x) *
        (config->max_db - config->min_db) / 
        graph_w +
        config->min_db;
    CLAMP(x_db, config->min_db, config->max_db);
    return x_db;
}

double CompressorCanvasBase::y_to_db(int y)
{
	CLAMP(y, 0, get_h());
	double y_db = (double)(y - graph_y) * 
        (config->min_db - config->max_db) / 
        graph_h + 
        config->max_db;
//printf("CompressorCanvasBase::cursor_motion_event %d x=%d y=%d x_db=%f y_db=%f\n", 
//__LINE__, x, y, x_db, y_db);
    CLAMP(y_db, config->min_db, config->max_db);
    return y_db;
}



void CompressorCanvasBase::update()
{
	int y1, y2;

// headroom boxes
    set_color(window->get_bg_color());
    draw_box(graph_x, 0, get_w(), graph_y);
    draw_box(graph_w, graph_y, get_w() - graph_w, get_h() - graph_y);
//     const int checker_w = DP(10);
//     const int checker_h = DP(10);
//     set_color(MDGREY);
//     for(int i = 0; i < get_h(); i += checker_h)
//     {
//         for(int j = (i % 2) * checker_w; j < get_w(); j += checker_w * 2)
//         {
//             if(!(i >= graph_y && 
//                 i + checker_h < graph_y + graph_h &&
//                 j >= graph_x &&
//                 j + checker_w < graph_x + graph_w))
//             {
//                 draw_box(j, i, checker_w, checker_h);
//             }
//         }
//     }

// canvas boxes
	clear_box(graph_x, graph_y, graph_w, graph_h);

	draw_3d_border(0, 
		0, 
		get_w(), 
		get_h(), 
		window->get_bg_color(),
		BLACK,
		MDGREY, 
		window->get_bg_color());



	set_line_dashes(1);
	set_color(GREEN);
	
	for(int i = 1; i < divisions; i++)
	{
		int y = graph_y + graph_h * i / divisions;
		draw_line(graph_x, y, graph_x + graph_w, y);
// 0db 
        if(i == 1)
        {
            draw_line(graph_x, y + 1, graph_x + graph_w, y + 1);
        }
		
		int x = graph_x + graph_w * i / divisions;
		draw_line(x, graph_y, x, graph_y + graph_h);
// 0db 
        if(i == divisions - 1)
        {
            draw_line(x + 1, graph_y, x + 1, graph_y + graph_h);
        }
	}
	set_line_dashes(0);


	set_font(MEDIUMFONT);
	draw_text(plugin->get_theme()->widget_border, 
		get_h() / 2, 
		_("Output"));
	draw_text(get_w() / 2 - get_text_width(MEDIUMFONT, _("Input")) / 2, 
		get_h() - plugin->get_theme()->widget_border, 
		_("Input"));


    for(int pass = 0; pass < 2; pass++)
    {
        for(int band = 0; band < config->total_bands; band++)
        {
// draw the active band on top of the others
            if(band == config->current_band && pass == 0 ||
                band != config->current_band && pass == 1)
            {
                continue;
            }

            if(band == config->current_band)
            {
	            set_color(WHITE);
	            set_line_width(2);
            }
            else
            {
	            set_color(MEGREY);
	            set_line_width(1);
            }

// draw the line
	        for(int i = graph_x; i <= graph_x + graph_w; i++)
	        {
		        y2 = x_to_y(band, i);

		        if(i > graph_x)
		        {
			        draw_line(i - 1, y1, i, y2);
		        }

		        y1 = y2;
	        }

	        set_line_width(1);

// draw the points
            if(band == config->current_band)
            {
                BandConfig *band_config = &config->bands[band];
	            int total = band_config->levels.total ? band_config->levels.total : 1;
	            for(int i = 0; i < band_config->levels.total; i++)
	            {
		            double x_db = config->get_x(band, i);
		            double y_db = config->get_y(band, i);

		            int x = db_to_x(x_db);
		            int y = db_to_y(y_db);

                    if(i == current_point)
                    {
    		            draw_box(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
                    }
                    else
                    {
    		            draw_rectangle(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
                    }
	            }
            }
        }
    }
	
	flash();
}

int CompressorCanvasBase::button_press_event()
{
    BandConfig *band_config = &config->bands[config->current_band];
// Check existing points
	if(is_event_win() && 
        cursor_inside())
	{
		for(int i = 0; i < band_config->levels.total; i++)
		{
			double x_db = config->get_x(config->current_band, i);
			double y_db = config->get_y(config->current_band, i);

			int x = db_to_x(x_db);
			int y = db_to_y(y_db);

			if(get_cursor_x() <= x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() <= y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			{
				current_operation = DRAG;
				current_point = i;
				return 1;
			}
		}



        if(get_cursor_x() >= graph_x &&
            get_cursor_x() < graph_x + graph_w &&
            get_cursor_y() >= graph_y &&
            get_cursor_y() < graph_y + graph_h)
        {
// Create new point
		    double x_db = x_to_db(get_cursor_x());
		    double y_db = y_to_db(get_cursor_y());

		    current_point = config->set_point(config->current_band, x_db, y_db);
		    current_operation = DRAG;
		    update_window();
		    plugin->send_configure_change();
		    return 1;
        }
	}
	return 0;
//plugin->config.dump();
}

int CompressorCanvasBase::button_release_event()
{
    BandConfig *band_config = &config->bands[config->current_band];

	if(current_operation == DRAG)
	{
		if(current_point > 0)
		{
			if(band_config->levels.values[current_point].x <
				band_config->levels.values[current_point - 1].x)
            {
				config->remove_point(config->current_band, current_point);
            }
		}

		if(current_point < band_config->levels.total - 1)
		{
			if(band_config->levels.values[current_point].x >=
				band_config->levels.values[current_point + 1].x)
            {
				config->remove_point(config->current_band, current_point);
            }
		}

		update_window();
		plugin->send_configure_change();
		current_operation = NONE;
		return 1;
	}

	return 0;
}

int CompressorCanvasBase::cursor_motion_event()
{
    BandConfig *band_config = &config->bands[config->current_band];

	if(current_operation == DRAG)
	{
		int x = get_cursor_x();
		int y = get_cursor_y();
		double x_db = x_to_db(x);
		double y_db = y_to_db(y);
        
        if(shift_down())
        {
            const int grid_precision = 6;
            x_db = config->max_db + (double)(grid_precision * (int)((x_db - config->max_db) / grid_precision - 0.5));
            y_db = config->max_db + (double)(grid_precision * (int)((y_db - config->max_db) / grid_precision - 0.5));
        }
        
        
//printf("CompressorCanvasBase::cursor_motion_event %d x=%d y=%d x_db=%f y_db=%f\n", 
//__LINE__, x, y, x_db, y_db);
		band_config->levels.values[current_point].x = x_db;
		band_config->levels.values[current_point].y = y_db;
		update_window();
		plugin->send_configure_change();
		return 1;
//plugin->config.dump();
	}
	else
// Change cursor over points
	if(is_event_win() && cursor_inside())
	{
		int new_cursor = CROSS_CURSOR;

		for(int i = 0; i < band_config->levels.total; i++)
		{
			double x_db = config->get_x(config->current_band, i);
			double y_db = config->get_y(config->current_band, i);

			int x = db_to_x(x_db);
			int y = db_to_y(y_db);

			if(get_cursor_x() <= x + POINT_W / 2 && get_cursor_x() >= x - POINT_W / 2 &&
				get_cursor_y() <= y + POINT_W / 2 && get_cursor_y() >= y - POINT_W / 2)
			{
				new_cursor = UPRIGHT_ARROW_CURSOR;
				break;
			}
		}

// out of active area
        if(get_cursor_x() >= graph_x + graph_w ||
            get_cursor_y() < graph_y)
        {
            new_cursor = UPRIGHT_ARROW_CURSOR;
        }

		if(new_cursor != get_cursor())
		{
			set_cursor(new_cursor, 0, 1);
		}
	}
	return 0;
}


void CompressorCanvasBase::update_window()
{
    printf("CompressorCanvasBase::update_window %d empty\n", __LINE__);
}



