/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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


// draw a color swatch for a given brightness

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "cicolors.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "playback3d.h"
#include "swatch.h"
#include "theme.h"
#include "vframe.h"




REGISTER_PLUGIN(SwatchMain)


#define MAX_VALUE 100



SwatchConfig::SwatchConfig()
{
	brightness = MAX_VALUE;
    saturation = MAX_VALUE;
    fix_brightness = 0;
    angle = 0;
}

int SwatchConfig::equivalent(SwatchConfig &that)
{
	return brightness == that.brightness &&
        saturation == that.saturation &&
        fix_brightness == that.fix_brightness &&
        angle == that.angle;
}

void SwatchConfig::copy_from(SwatchConfig &that)
{
	brightness = that.brightness;
	saturation = that.saturation;
    fix_brightness = that.fix_brightness;
    angle = that.angle;
}

void SwatchConfig::interpolate(SwatchConfig &prev, 
	SwatchConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


	this->brightness = (int)(prev.brightness * prev_scale + next.brightness * next_scale);
	this->saturation = (int)(prev.saturation * prev_scale + next.saturation * next_scale);
	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale);
    fix_brightness = prev.fix_brightness;
}




SwatchSlider::SwatchSlider(SwatchMain *plugin, 
    SwatchWindow *gui,
    int x, 
    int y,
    int min,
    int max,
    int *output)
 : BC_ISlider(x,
	y,
	0, 
	gui->get_w() - plugin->get_theme()->widget_border - x, 
    gui->get_w() - plugin->get_theme()->widget_border - x, 
    min, 
    max, 
    *output)
{
	this->plugin = plugin;
    this->output = output;
}

int SwatchSlider::handle_event ()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

SwatchOption::SwatchOption(SwatchMain *plugin, 
    SwatchWindow *gui, 
    int x, 
    int y, 
    const char *text,
    int fix_brightness)
 : BC_Radial(x, 
	y, 
	(plugin->config.fix_brightness && fix_brightness), 
	text)
{
	this->plugin = plugin;
    this->gui = gui;
    this->fix_brightness = fix_brightness;
}

int SwatchOption::handle_event()
{
    plugin->config.fix_brightness = fix_brightness;
    gui->update_fixed();
    plugin->send_configure_change();
    return 1;
}




SwatchWindow::SwatchWindow(SwatchMain *plugin)
 : PluginClientWindow(plugin,
	DP(350), 
	DP(250), 
	DP(350), 
	DP(250), 
	0)
{
	this->plugin = plugin;
}

SwatchWindow::~SwatchWindow()
{
}

void SwatchWindow::create_objects()
{
	int margin = client->get_theme()->widget_border;
	int x = margin, y = margin;
	BC_Title *title;

	add_subwindow(brightness_title = new BC_Title(x, y, _("Brightness:")));
    y += brightness_title->get_h() + margin;
	add_subwindow (brightness = new SwatchSlider(plugin, this, x, y, 0, MAX_VALUE, &plugin->config.brightness));
    y += brightness->get_h() + margin;

	add_subwindow(saturation_title = new BC_Title(x, y, _("Saturation:")));
    y += saturation_title->get_h() + margin;
	add_subwindow (saturation = new SwatchSlider(plugin, this, x, y, 0, MAX_VALUE, &plugin->config.saturation));
    y += saturation->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Angle:")));
    y += title->get_h() + margin;
	add_subwindow (angle = new SwatchSlider(plugin, this, x, y, -180, 180, &plugin->config.angle));
    y += saturation->get_h() + margin;

    add_subwindow(fix_brightness = new SwatchOption(plugin, 
        this, 
        x, 
        y, 
        _("Constant Brightness"),
        1));
    y += fix_brightness->get_h() + margin;
    add_subwindow(fix_saturation = new SwatchOption(plugin, 
        this, 
        x, 
        y, 
        _("Constant Saturation"),
        0));
    update_fixed();
	show_window();
}

void SwatchWindow::update_fixed()
{
    fix_brightness->update(plugin->config.fix_brightness);
    fix_saturation->update(!plugin->config.fix_brightness);
    if(plugin->config.fix_brightness)
    {
        saturation_title->set_color(BC_Resources::disabled_text_color);
        brightness_title->set_color(BC_Resources::default_text_color);
        saturation->disable();
        brightness->enable();
    }
    else
    {
        saturation_title->set_color(BC_Resources::default_text_color);
        brightness_title->set_color(BC_Resources::disabled_text_color);
        saturation->enable();
        brightness->disable();
    }
}


SwatchMain::SwatchMain(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
    temp = 0;
}

SwatchMain::~SwatchMain()
{
	if(engine) delete engine;
    if(temp) delete temp;
}

const char* SwatchMain::plugin_title() { return N_("Color Swatch"); }
int SwatchMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(SwatchMain, SwatchWindow)

LOAD_CONFIGURATION_MACRO(SwatchMain, SwatchConfig)

int SwatchMain::is_synthesis()
{
	return 1;
}

VFrame* SwatchMain::new_picon()
{
    return 0;
}

int SwatchMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	need_reconfigure |= load_configuration();
	if(get_use_opengl()) return run_opengl();

	if(!temp) temp = new VFrame(0, 
		-1,
		frame->get_w(),
		frame->get_h(),
	    frame->get_color_model(),
		-1);

    if(need_reconfigure)
    {
	    if(!engine) engine = new SwatchServer(this,
		    get_project_smp() + 1,
		    get_project_smp() + 1);
	    engine->process_packages();
    }
    
    frame->copy_from(temp);


	return 0;
}


void SwatchMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			((SwatchWindow*)thread->window)->lock_window("SwatchMain::update_gui");
			((SwatchWindow*)thread->window)->brightness->update(config.brightness);
			((SwatchWindow*)thread->window)->saturation->update(config.saturation);
			((SwatchWindow*)thread->window)->angle->update(config.angle);
            ((SwatchWindow*)thread->window)->update_fixed();
			((SwatchWindow*)thread->window)->unlock_window();
		}
	}
}





void SwatchMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("SWATCH");

	output.tag.set_property("BRIGHTNESS", config.brightness);
	output.tag.set_property("SATURATION", config.saturation);
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("FIX_BRIGHTNESS", config.fix_brightness);
	output.append_tag();
	output.terminate_string();
//printf("SwatchMain::save_data %d %s\n", __LINE__, output.string);
}

void SwatchMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SWATCH"))
			{
				config.brightness = input.tag.get_property("BRIGHTNESS", config.brightness);
				config.saturation = input.tag.get_property("SATURATION", config.saturation);
				config.angle = input.tag.get_property("ANGLE", config.angle);
				config.fix_brightness = input.tag.get_property("FIX_BRIGHTNESS", config.fix_brightness);
			}
		}
	}
}

int SwatchMain::handle_opengl()
{
#ifdef HAVE_GL
	const char *head_frag =
		"uniform sampler2D tex;\n"
		"uniform vec2 texture_extents;\n"
		"uniform vec2 center_coord;\n"
		"uniform float value;\n"
		"uniform float saturation;\n"
		"uniform float angle;\n"
		"uniform bool fix_value;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vec2 out_coord = gl_TexCoord[0].st * texture_extents;\n"
		"	vec2 in_coord = vec2(out_coord.x - center_coord.x, out_coord.y - center_coord.y);\n"
        "   float max_s = center_coord.x;\n"
        "   if(center_coord.y < max_s) max_s = center_coord.y;\n"
        "   vec4 pixel;\n"
        "   pixel.a = 1.0;\n"
        "   pixel.r = atan(in_coord.x, in_coord.y) / 2.0 / 3.14159 * 360.0 + angle; // hue\n"
        "   if(pixel.r < 0.0) pixel.r += 360.0;\n"
        "   if(fix_value)\n"
        "   {\n"
        "       pixel.g = length(vec2(in_coord.x, in_coord.y)) / max_s; // saturation\n"
        "       if(pixel.g > 1.0) pixel.g = 1.0; \n"
        "       pixel.b = value;\n"
        "   }\n"
        "   else\n"
        "   {\n"
        "       pixel.g = saturation;\n"
        "       pixel.b = length(vec2(in_coord.x, in_coord.y)) / max_s; // value\n"
        "       if(pixel.b > 1.0) pixel.b = 1.0; \n"
        "   }\n"
        HSV_TO_RGB_FRAG("pixel");

	static const char *put_yuv_frag =
			RGB_TO_YUV_FRAG("pixel")
		"	gl_FragColor = pixel;\n"
		"}\n";

	static const char *put_rgb_frag =
		"	gl_FragColor = pixel;\n"
		"}\n";




	const char *shader_stack[5] = { 0, 0, 0, 0, 0 };
	shader_stack[0] = head_frag;
    if(cmodel_is_yuv(get_output()->get_color_model()))
        shader_stack[1] = put_yuv_frag;
    else
        shader_stack[1] = put_rgb_frag;

	get_output()->set_opengl_state(VFrame::TEXTURE);
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);

	unsigned int frag = VFrame::make_shader(0, 
		shader_stack[0], 
		shader_stack[1], 
		0);

	if(frag)
	{
		glUseProgram(frag);
		float w = get_output()->get_w();
		float h = get_output()->get_h();
		float texture_w = get_output()->get_texture_w();
		float texture_h = get_output()->get_texture_h();
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);
		glUniform2f(glGetUniformLocation(frag, "center_coord"), 
				(GLfloat)w / 2,
				(GLfloat)h / 2);
		glUniform2f(glGetUniformLocation(frag, "texture_extents"), 
				(GLfloat)texture_w,
				(GLfloat)texture_h);

		glUniform1f(glGetUniformLocation(frag, "value"), (float)config.brightness / MAX_VALUE);
		glUniform1f(glGetUniformLocation(frag, "saturation"), (float)config.saturation / MAX_VALUE);
		glUniform1f(glGetUniformLocation(frag, "angle"), (float)config.angle);
        glUniform1i(glGetUniformLocation(frag, "fix_value"), config.fix_brightness);
	}

	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	
#endif
    return 0;
}











SwatchPackage::SwatchPackage()
 : LoadPackage()
{
}




SwatchUnit::SwatchUnit(SwatchServer *server, SwatchMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}




#define CREATE_SWATCH(type, components, max, is_yuv) \
{ \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		type *out_row = (type*)plugin->temp->get_rows()[i]; \
        for(int j = 0; j < w; j++) \
        { \
            float hue = atan2(j - center_x, i - center_y) * 360 / 2 / M_PI + angle; \
            if(fix_brightness) \
            { \
                saturation = hypot(j - center_x, i - center_y) / max_s; \
                if(saturation > 1) saturation = 1; \
            } \
            else \
            { \
                value = hypot(j - center_x, i - center_y) / max_s; \
                if(value > 1) value = 1; \
            } \
            if(hue < 0) hue += 360; \
            if(is_yuv) \
            { \
                int y, u, v; \
                HSV::hsv_to_yuv(y, u, v, hue, saturation, value, max); \
                out_row[0] = y; \
                out_row[1] = u; \
                out_row[2] = v; \
            } \
            else \
            { \
                float r, g, b; \
                HSV::hsv_to_rgb(r, g, b, hue, saturation, value); \
                out_row[0] = (type)(r * max); \
                out_row[1] = (type)(g * max); \
                out_row[2] = (type)(b * max); \
            } \
 \
/* the alpha */ \
            if(components == 4) out_row[3] = max; \
            out_row += components; \
        } \
    } \
}

void SwatchUnit::process_package(LoadPackage *package)
{
	SwatchPackage *pkg = (SwatchPackage*)package;
	int h = plugin->temp->get_h();
	int w = plugin->temp->get_w();
	int center_x = w / 2;
	int center_y = h / 2;
	int cmodel = plugin->temp->get_color_model();
// maximum saturation
    float max_s = center_x;
    if(center_y < max_s) max_s = center_y;
    int fix_brightness = plugin->config.fix_brightness;
    float saturation = (float)plugin->config.saturation / MAX_VALUE;
    float value = (float)plugin->config.brightness / MAX_VALUE;
    float angle = plugin->config.angle;

	switch(cmodel)
	{
		case BC_RGB888:
			CREATE_SWATCH(unsigned char, 3, 0xff, 0)
			break;

		case BC_RGBA8888:
			CREATE_SWATCH(unsigned char, 4, 0xff, 0)
			break;

		case BC_RGB_FLOAT:
			CREATE_SWATCH(float, 3, 1.0, 0)
			break;

		case BC_RGBA_FLOAT:
			CREATE_SWATCH(float, 4, 1.0, 0)
			break;

		case BC_YUV888:
			CREATE_SWATCH(unsigned char, 3, 0xff, 1)
			break;

		case BC_YUVA8888:
			CREATE_SWATCH(unsigned char, 4, 0xff, 1)
			break;
	}
}






SwatchServer::SwatchServer(SwatchMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void SwatchServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		SwatchPackage *package = (SwatchPackage*)get_package(i);
		package->y1 = plugin->temp->get_h() * 
			i / 
			get_total_packages();
		package->y2 = plugin->temp->get_h() * 
			(i + 1) /
			get_total_packages();
	}
}

LoadClient* SwatchServer::new_client()
{
	return new SwatchUnit(this, plugin);
}

LoadPackage* SwatchServer::new_package()
{
	return new SwatchPackage;
}





