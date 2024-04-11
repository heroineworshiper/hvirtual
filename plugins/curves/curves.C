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

// curve equations from gimpcurve.c
// gimp_curve_calculate -> gimp_curve_plot
// Some kind of bezier curve which calculates control points automatically.

#include "clip.h"
#include "curves.h"
#include "curveswindow.h"
#include "filexml.h"
#include "histogramtools.h"
#include "language.h"
#include "playback3d.h"
#include "vframe.h"
#include <string.h>

REGISTER_PLUGIN(CurvesMain)



CurvesConfig::CurvesConfig()
{
	plot = 1;
    split = 0;
    reset(-1, 1);
}

int CurvesConfig::equivalent(CurvesConfig &that)
{
    if(plot != that.plot || 
        split != that.split) return 0;

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
        if(points[i].size() != that.points[i].size()) return 0;

		for(int j = 0; j < that.points[i].size(); j++)
        {
            curve_point_t *point1 = that.points[i].get_pointer(j);
            curve_point_t *point2 = points[i].get_pointer(j);
            if(point1->x != point2->x ||
                point1->y != point2->y) return 0;
        }
	}

    return 1;
}

void CurvesConfig::copy_from(CurvesConfig &that)
{
    reset(-1, 0);
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		for(int j = 0; j < that.points[i].size(); j++)
        {
            curve_point_t *point = that.points[i].get_pointer(j);
            points[i].append(*point);
        }
	}
    plot = that.plot;
    split = that.split;
}

void CurvesConfig::interpolate(CurvesConfig &prev, 
	CurvesConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = 1.0 - next_scale;

    reset(-1, 0);

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
// Copy previous points in
        for(int j = 0; j < prev.points[i].size(); j++)
        {
            curve_point_t *new_point = prev.points[i].get_pointer(j);
            points[i].append(*new_point);
        }

// blend points which exist
        for(int j = 0; j < MIN(prev.points[i].size(), next.points[i].size()); j++)
        {
            curve_point_t *prev_point = prev.points[i].get_pointer(j);
            curve_point_t *next_point = next.points[i].get_pointer(j);
            curve_point_t *new_point = points[i].get_pointer(j);
            new_point->x = prev_point->x * prev_scale + next_point->x * next_scale;
            new_point->y = prev_point->y * prev_scale + next_point->y * next_scale;
        }
//printf("CurvesConfig::interpolate %d mode=%d points=%d\n", __LINE__, i, points[i].size());
	}


    plot = prev.plot;
    split = prev.split;
}

// Used by constructor and reset button
void CurvesConfig::reset(int mode, int defaults)
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
        if(mode < 0 || mode == i)
        {
    		points[i].remove_all();
// default points
            if(defaults)
            {
                curve_point_t point1, point2;
                point1.x = CURVE_MIN;
                point1.y = CURVE_MIN;
                point2.x = CURVE_MAX;
                point2.y = CURVE_MAX;
                points[i].append(point1);
                points[i].append(point2);
            }
        }
	}
}

void CurvesConfig::boundaries()
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		for(int j = 0; j < points[i].size(); j++)
        {
            curve_point_t *point = points[i].get_pointer(j);
            CLAMP(point->x, CURVE_MIN, CURVE_MAX);
            CLAMP(point->y, CURVE_MIN, CURVE_MAX);
        }
	}
}

int CurvesConfig::set_point(int mode, curve_point_t &point)
{
    for(int i = points[mode].size() - 1; i >= 0; i--)
    {
        curve_point_t *dst_point = points[mode].get_pointer(i);
        if(dst_point->x < point.x)
        {
            points[mode].insert_after(point, i);
            return i + 1;
        }
    }
    
    points[mode].insert(point, 0);
    return 0;
}














CurvesMain::CurvesMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
    tools = new CurveTools(this);

	mode = HISTOGRAM_VALUE;
	w = WINDOW_W;
	h = WINDOW_H;
    current_point = 0;
}

CurvesMain::~CurvesMain()
{
    delete tools;
	delete engine;
}

const char* CurvesMain::plugin_title() { return N_("Curves"); }
int CurvesMain::is_realtime() { return 1; }
NEW_WINDOW_MACRO(CurvesMain, CurvesWindow)
LOAD_CONFIGURATION_MACRO(CurvesMain, CurvesConfig)

int CurvesMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	int need_reconfigure = load_configuration();
	int use_opengl = get_use_opengl() &&
		(!config.plot || !gui_open());

// printf("CurvesMain::process_buffer %d %d %d %d %d\n",
// __LINE__,
// config.points[0].size(),
// config.points[1].size(),
// config.points[2].size(),
// config.points[3].size());
//use_opengl = 0;

	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		use_opengl);

    if(need_reconfigure || !tools->initialized())
    {
        tools->initialize(1);
    }

// Apply curves in GPU
	if(use_opengl) return run_opengl();

	if(!engine) engine = new CurvesEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
// Plot it
	if(config.plot) send_render_gui(frame, 1);
// Apply curves
	engine->process_packages(frame);

    return 0;
}

void CurvesMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("CURVES");
	output.tag.set_property("PLOT", config.plot);
	output.tag.set_property("SPLIT", config.split);
	output.tag.set_property("W", w);
	output.tag.set_property("H", h);
	output.tag.set_property("MODE", mode);
	output.append_tag();
	output.append_newline();

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
        output.tag.set_title("CURVE");
        output.tag.set_property("MODE", i);
	    output.append_tag();
	    output.append_newline();
        for(int j = 0; j < config.points[i].size(); j++)
        {
            output.tag.set_title("POINT");
	        output.tag.set_property("X", config.points[i].get(j).x);
	        output.tag.set_property("Y", config.points[i].get(j).y);
	        output.append_tag();
	        output.append_newline();
        }
	}

	output.terminate_string();
}

void CurvesMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));
	int result = 0;
	int current_input_mode = 0;
    config.reset(-1, 0);
    
	while(1)
	{
		if(input.read_tag()) break;

        if(input.tag.title_is("CURVES"))
		{
			config.plot = input.tag.get_property("PLOT", config.plot);
			config.split = input.tag.get_property("SPLIT", config.split);
			if(is_defaults())
			{
				w = input.tag.get_property("W", w);
				h = input.tag.get_property("H", h);
				mode = input.tag.get_property("MODE", mode);
			}
            
        }
        else
        if(input.tag.title_is("CURVE"))
        {
            current_input_mode = input.tag.get_property("MODE", 0);
        }
        else
        if(input.tag.title_is("POINT"))
        {
            curve_point_t point;
            point.x = input.tag.get_property("X", (double)0);
            point.y = input.tag.get_property("Y", (double)0);
            config.points[current_input_mode].append(point);
        }
    }
	config.boundaries();
// printf("CurvesMain::read_data %d size=%d\n", 
// __LINE__, 
// config.points[0].size());
}

void CurvesMain::update_gui()
{
	if(thread)
	{
		((CurvesWindow*)thread->window)->lock_window("CurvesMain::update_gui");
		int reconfigure = load_configuration();
//printf("CurvesMain::update_gui %d %d\n", __LINE__, reconfigure);
		if(reconfigure) 
		{
			((CurvesWindow*)thread->window)->update(1,
				1,
                1);
		}
		((CurvesWindow*)thread->window)->unlock_window();
	}
}

void CurvesMain::render_gui(void *data, int size)
{
	if(thread)
	{
// generate the lookup tables
    	((CurvesWindow*)thread->window)->lock_window("CurvesMain::render_gui 1");
        tools->initialize(0);
		((CurvesWindow*)thread->window)->unlock_window();

        tools->calculate_histogram((VFrame*)data, 1);

		((CurvesWindow*)thread->window)->lock_window("CurvesMain::render_gui 2");
// Draw the histogram
        ((CurvesWindow*)thread->window)->update(1, 0, 0);
		((CurvesWindow*)thread->window)->unlock_window();
    }
}

int CurvesMain::calculate_use_opengl()
{
	int result = get_use_opengl() &&
		(!config.plot || !gui_open());
	return result;
}

void CurvesMain::calculate_histogram(VFrame *data, int do_value)
{
}

// from gimp_curve_plot
float CurvesMain::calculate_bezier(curve_point_t *p1, 
    curve_point_t *p2, 
    curve_point_t *p3, 
    curve_point_t *p4, 
    float x)
{
    float x0, x3, y0, y1, y2, y3;
    float dx, dy;
    float slope;

    
/* the outer control points for the bezier curve. */
    x0 = p2->x;
    y0 = p2->y;
    x3 = p3->x;
    y3 = p3->y;

/*
 * the x values of the inner control points are fixed at
 * x1 = 2/3*x0 + 1/3*x3   and  x2 = 1/3*x0 + 2/3*x3
 * this ensures that the x values increase linearily with the
 * parameter t and enables us to skip the calculation of the x
 * values altogehter - just calculate y(t) evenly spaced.
 */

    dx = x3 - x0;
    dy = y3 - y0;

    if(dx <= 0) return x;

    if (p1 == p2 && p3 == p4)
    {
/* No information about the neighbors,
 * calculate y1 and y2 to get a straight line
 */
        y1 = y0 + dy / 3.0;
        y2 = y0 + dy * 2.0 / 3.0;
    }
    else 
    if (p1 == p2 && p3 != p4)
    {
      /* only the right neighbor is available. Make the tangent at the
       * right endpoint parallel to the line between the left endpoint
       * and the right neighbor. Then point the tangent at the left towards
       * the control handle of the right tangent, to ensure that the curve
       * does not have an inflection point.
       */
        slope = (p4->y - y0) / (p4->x - x0);

        y2 = y3 - slope * dx / 3.0;
        y1 = y0 + (y2 - y0) / 2.0;
    }
    else 
    if (p1 != p2 && p3 == p4)
    {
        /* see previous case */
        slope = (y3 - p1->y) / (x3 - p1->x);

        y1 = y0 + slope * dx / 3.0;
        y2 = y3 + (y1 - y3) / 2.0;
    }
    else /* (p1 != p2 && p3 != p4) */
    {
        /* Both neighbors are available. Make the tangents at the endpoints
         * parallel to the line between the opposite endpoint and the adjacent
         * neighbor.
         */
        slope = (y3 - p1->y) / (x3 - p1->x);

        y1 = y0 + slope * dx / 3.0;

        slope = (p4->y - y0) / (p4->x - x0);

        y2 = y3 - slope * dx / 3.0;
    }

    /*
     * finally calculate the y(t) values for the given bezier values. We can
     * use homogenously distributed values for t, since x(t) increases linearily.
     */
    float y, t;

    t = (x - x0) / dx;
    y =     y0 * (1-t) * (1-t) * (1-t) +
        3 * y1 * (1-t) * (1-t) * t     +
        3 * y2 * (1-t) * t     * t     +
            y3 * t     * t     * t;

    return y;
}

float CurvesMain::calculate_level(float input, int mode, int do_value)
{
    int total = config.points[mode].size();
    if(total == 0)
    {
	    if(do_value && mode != HISTOGRAM_VALUE)
	    {
		    return calculate_level(input, HISTOGRAM_VALUE, 0);
	    }
        return input;
    }

// find points bordering input
    int i;
    for(i = total - 1; i >= 0; i--)
    {
        curve_point_t *point = config.points[mode].get_pointer(i);
        if(point->x <= input)
        {
            break;
        }
    }

// Boundaries
    if(i >= total - 1)
        return config.points[mode].get_pointer(total - 1)->y;
    if(i < 0)
        return config.points[mode].get_pointer(0)->y;

// from gimp_curve_calculate
    curve_point_t *p1 = config.points[mode].get_pointer(MAX(i - 1, 0));
    curve_point_t *p2 = config.points[mode].get_pointer(i);
    curve_point_t *p3 = config.points[mode].get_pointer(i + 1);
    curve_point_t *p4 = config.points[mode].get_pointer(MIN(i + 2, total - 1));

    float output = calculate_bezier(p1, p2, p3, p4, input);
    CLAMP(output, CURVE_MIN, CURVE_MAX);


// Apply value curve to rendered output for the channel
	if(do_value && mode != HISTOGRAM_VALUE)
	{
		output = calculate_level(output, HISTOGRAM_VALUE, 0);
	}


    return output;
}


int CurvesMain::handle_opengl()
{
#ifdef HAVE_GL

	const char *yuv_shader = 
		"const vec3 black = vec3(0.0, 0.5, 0.5);\n"
		"\n"
		"vec4 yuv_to_rgb(vec4 color)\n"
		"{\n"
			YUV_TO_RGB_FRAG("color")
		"	return color;\n"
		"}\n"
		"\n"
		"vec4 rgb_to_yuv(vec4 color)\n"
		"{\n"
			RGB_TO_YUV_FRAG("color")
		"	return color;\n"
		"}\n";

	const char *rgb_shader = 
		"const vec3 black = vec3(0.0, 0.0, 0.0);\n"
		"\n"
		"vec4 yuv_to_rgb(vec4 color)\n"
		"{\n"
		"	return color;\n"
		"}\n"
		"vec4 rgb_to_yuv(vec4 color)\n"
		"{\n"
		"	return color;\n"
		"}\n";


	extern unsigned char _binary_curves_sl_start[];
	static char *shader = (char*)_binary_curves_sl_start;

	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();

	const char *shader_stack[] = { 0, 0 };
	int current_shader = 0;

	switch(get_output()->get_color_model())
	{
        case BC_RGB888:
        case BC_RGBA8888:
		case BC_RGB_FLOAT:
        case BC_RGBA_FLOAT:
            shader_stack[current_shader++] = rgb_shader;
			shader_stack[current_shader++] = shader;
            break;

		case BC_YUV888:
		case BC_YUVA8888:
            shader_stack[current_shader++] = yuv_shader;
			shader_stack[current_shader++] = shader;
            break;
    }

	unsigned int frag = VFrame::make_shader(0, 
		shader_stack[0], 
		shader_stack[1], 
		0);
	if(frag)
	{
		glUseProgram(frag);
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);

// create temp arrays
        float temp_points[MAX_POINTS * 2];
        const char* points_titles[] = {
            "points_r",
            "points_g",
            "points_b",
            "points_v",
        };
        const char* totals_titles[] = {
            "total_r",
            "total_g",
            "total_b",
            "total_v",
        };
        for(int i = 0; i < HISTOGRAM_MODES; i++)
        {
            int total = MIN(config.points[i].size(), MAX_POINTS);
            for(int j = 0; j < total; j++)
            {
                curve_point_t *point = config.points[i].get_pointer(j);
                temp_points[j * 2] = point->x;
                temp_points[j * 2 + 1] = point->y;
            }
            
            glUniform2fv(glGetUniformLocation(frag, points_titles[i]), 
                total,
                temp_points);
            glUniform1i(glGetUniformLocation(frag, totals_titles[i]), 
                total);
        }
	}

	get_output()->bind_texture(0);

	glDisable(GL_BLEND);

// Draw the affected half
	if(config.split)
	{
		glBegin(GL_TRIANGLES);
		glNormal3f(0, 0, 1.0);

		glTexCoord2f(0.0 / get_output()->get_texture_w(), 
			0.0 / get_output()->get_texture_h());
		glVertex3f(0.0, -(float)get_output()->get_h(), 0);


		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(), 
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), -0.0, 0);

		glTexCoord2f(0.0 / get_output()->get_texture_w(), 
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f(0.0, -0.0, 0);


		glEnd();
	}
	else
	{
		get_output()->draw_texture();
	}


	glUseProgram(0);

// Draw the unaffected half
	if(config.split)
	{
		glBegin(GL_TRIANGLES);
		glNormal3f(0, 0, 1.0);


		glTexCoord2f(0.0 / get_output()->get_texture_w(), 
			0.0 / get_output()->get_texture_h());
		glVertex3f(0.0, -(float)get_output()->get_h(), 0);

		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(), 
			0.0 / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), 
			-(float)get_output()->get_h(), 0);

		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(), 
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), -0.0, 0);


 		glEnd();
	}

	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
    return 0;
}












CurveTools::CurveTools(CurvesMain *plugin)
{
    this->plugin = plugin;
}

float CurveTools::calculate_level(float input, int mode, int do_value)
{
    return plugin->calculate_level(input, mode, do_value);
}








CurvesPackage::CurvesPackage()
 : LoadPackage()
{
}




CurvesUnit::CurvesUnit(CurvesEngine *server, 
	CurvesMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

CurvesUnit::~CurvesUnit()
{
}

void CurvesUnit::process_package(LoadPackage *package)
{
	CurvesPackage *pkg = (CurvesPackage*)package;




#define PROCESS(type, components) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)data->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
		    	continue; \
			row[0] = lookup_r[row[0]]; \
			row[1] = lookup_g[row[1]]; \
			row[2] = lookup_b[row[2]]; \
			row += components; \
		} \
	} \
}

#define PROCESS_YUV(type, components, max) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)data->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
		    { \
            	continue; \
            } \
 \
/* Convert to 16 bit RGB */ \
			if(max == 0xff) \
			{ \
				y = (row[0] << 8) | row[0]; \
				u = (row[1] << 8) | row[1]; \
				v = (row[2] << 8) | row[2]; \
			} \
			else \
			{ \
				y = row[0]; \
				u = row[1]; \
				v = row[2]; \
			} \
 \
			plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v); \
 \
/* Look up in RGB domain */ \
			r = lookup_r[r]; \
			g = lookup_g[g]; \
			b = lookup_b[b]; \
 \
/* Convert to 16 bit YUV */ \
			plugin->yuv.rgb_to_yuv_16(r, g, b, y, u, v); \
 \
			if(max == 0xff) \
			{ \
				row[0] = y >> 8; \
				row[1] = u >> 8; \
				row[2] = v >> 8; \
			} \
			else \
			{ \
				row[0] = y; \
				row[1] = u; \
				row[2] = v; \
			} \
			row += components; \
		} \
	} \
}

#define PROCESS_FLOAT(components) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		float *row = (float*)data->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
		    	continue; \
			float r = row[0]; \
			float g = row[1]; \
			float b = row[2]; \
 \
			r = plugin->calculate_level(r, HISTOGRAM_RED, 1); \
			g = plugin->calculate_level(g, HISTOGRAM_GREEN, 1); \
			b = plugin->calculate_level(b, HISTOGRAM_BLUE, 1); \
 \
 			row[0] = r; \
			row[1] = g; \
			row[2] = b; \
 \
			row += components; \
		} \
	} \
}


	VFrame *data = server->data;
	int w = data->get_w();
	int h = data->get_h();
	int *lookup_r = plugin->tools->lookup[0];
	int *lookup_g = plugin->tools->lookup[1];
	int *lookup_b = plugin->tools->lookup[2];
    if(data->get_color_model() == BC_YUV888 ||
        data->get_color_model() == BC_YUVA8888)
    {
		lookup_r = plugin->tools->lookup16[0];
		lookup_g = plugin->tools->lookup16[1];
		lookup_b = plugin->tools->lookup16[2];
    }
	int r, g, b, y, u, v, a;
	switch(data->get_color_model())
	{
		case BC_RGB888:
			PROCESS(unsigned char, 3)
			break;
		case BC_RGB_FLOAT:
			PROCESS_FLOAT(3);
			break;
		case BC_RGBA8888:
			PROCESS(unsigned char, 4)
			break;
		case BC_RGBA_FLOAT:
			PROCESS_FLOAT(4);
			break;
		case BC_YUV888:
			PROCESS_YUV(unsigned char, 3, 0xff)
			break;
		case BC_YUVA8888:
			PROCESS_YUV(unsigned char, 4, 0xff)
			break;
	}
}






CurvesEngine::CurvesEngine(CurvesMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
// : LoadServer(1, 1)
{
	this->plugin = plugin;
}

void CurvesEngine::init_packages()
{
	total_size = data->get_h();
	int package_size = (int)((float)total_size / 
		get_total_packages() + 1);
	int start = 0;

// printf("CurvesEngine::CurvesEngine %d %d %d\n", 
// __LINE__,
// get_total_packages(),
// get_total_clients());

	for(int i = 0; i < get_total_packages(); i++)
	{
		CurvesPackage *package = (CurvesPackage*)get_package(i);
		package->start = total_size * i / get_total_packages();
		package->end = total_size * (i + 1) / get_total_packages();
	}
}

LoadClient* CurvesEngine::new_client()
{
	return new CurvesUnit(this, plugin);
}

LoadPackage* CurvesEngine::new_package()
{
	return new CurvesPackage;
}

void CurvesEngine::process_packages(VFrame *data)
{
	this->data = data;
	LoadServer::process_packages();
}



