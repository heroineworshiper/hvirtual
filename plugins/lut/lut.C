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

#include "browsebutton.h"
#include "cicolors.h"
#include "filexml.h"
#include "language.h"
#include "lut.h"
#include "mwindow.h"
#include "playback3d.h"
#include "theme.h"
#include <string.h>

// shades of R, G, & B in the LUT image
#define STEPS 64
#define SQRT_STEPS 8
// size of the LUT image 
#define W (STEPS * SQRT_STEPS)
#define H (STEPS * SQRT_STEPS)

LUTConfig::LUTConfig()
{
    path.assign("");
}
void LUTConfig::copy_from(LUTConfig &src)
{
    path.assign(src.path);
}
int LUTConfig::equivalent(LUTConfig &src)
{
    return !path.compare(src.path);
}
void LUTConfig::interpolate(LUTConfig &prev, 
	LUTConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
    path.assign(prev.path);
}




LutPath::LutPath(LUTWindow *window, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, window->client->config.path.c_str())
{
	this->window = window;
}

int LutPath::handle_event()
{
// Suggestions
	calculate_suggestions(window->file_entries);
	window->client->config.path.assign(get_text());
	window->client->send_configure_change();
	return 1;
}



LUTWindow::LUTWindow(LUTEffect *client)
 : PluginClientWindow(client,
	DP(480), 
	DP(100), 
	DP(100), 
	DP(100), 
	0)
{
	this->client = client;
    file_entries = 0;
}
LUTWindow::~LUTWindow()
{
    file_entries->remove_all_objects();
    delete file_entries;
}
void LUTWindow::create_objects()
{
	int margin = client->get_theme()->widget_border;
	int left_margin = client->get_theme()->window_border;
	int top_margin = client->get_theme()->window_border;
	int x = left_margin, y = top_margin;
	file_entries = new ArrayList<BC_ListBoxItem*>;
    BC_Title *title;
    add_tool(title = new BC_Title(x, y, _("Path of LUT file:")));
    y += title->get_h() + margin;
    add_tool(path = new LutPath(this, 
        x, 
        y, 
        get_w() - x - left_margin - margin - MWindow::theme->get_image_set("magnify_button")[0]->get_w()));
	add_tool(browse = new BrowseButton(
		MWindow::theme,
		this,
		path, 
		x + path->get_w() + margin, 
		y, 
		client->config.path.c_str(),
		_("LUT path"),
		_("Select a file to read:"),
		0));
	show_window(1);
}
void LUTWindow::update()
{
    path->update(client->config.path.c_str());
}

LUTServer::LUTServer(LUTEffect *plugin)
 : LoadServer(plugin->PluginClient::smp + 1, plugin->PluginClient::smp + 1)
{
	this->plugin = plugin;
}


void LUTServer::init_packages()
{
    VFrame *data = plugin->get_input(0);
	for(int i = 0; i < get_total_packages(); i++)
	{
		LUTPackage *package = (LUTPackage*)get_package(i);
		package->y1 = data->get_h() * i / get_total_packages();
		package->y2 = data->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* LUTServer::new_client()
{
	return new LUTUnit(plugin, this);
}

LoadPackage* LUTServer::new_package()
{
	return new LUTPackage;
}

LUTPackage::LUTPackage()
 : LoadPackage()
{
}

LUTUnit::LUTUnit(LUTEffect *plugin, LUTServer *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}

#define DO_IT(type, components, max, is_yuv) \
{ \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		type *row = (type*)data->get_rows()[i]; \
        for(int j = 0; j < w; j++) \
        { \
/* extract the source pixel */ \
            float r, g, b; \
            if(is_yuv) \
            { \
			    YUV::yuv_to_rgb_f(r, \
				    g, \
				    b, \
				    (float)row[0] / max, \
				    (float)row[1] / max - 0.5, \
				    (float)row[2] / max - 0.5); \
            } \
            else \
            { \
		 	    r = (float)row[0] / max; \
		 	    g = (float)row[1] / max; \
		 	    b = (float)row[2] / max; \
            } \
 \
/* if(i == 345 && j == 1170) */ \
/* printf("DO_IT %d %f %f %f\n", __LINE__, r, g, b); */ \
 \
 \
/* quantize it */ \
            int r_q1 = r * STEPS; \
            int g_q1 = g * STEPS; \
            int b_q1 = b * STEPS; \
            int r_q2 = r_q1 + 1; \
            int g_q2 = g_q1 + 1; \
            int b_q2 = b_q1 + 1; \
            CLAMP(r_q1, 0, STEPS - 1); \
            CLAMP(g_q1, 0, STEPS - 1); \
            CLAMP(b_q1, 0, STEPS - 1); \
            CLAMP(r_q2, 0, STEPS - 1); \
            CLAMP(g_q2, 0, STEPS - 1); \
            CLAMP(b_q2, 0, STEPS - 1); \
            int offset1 = 3 * ((b_q1 / SQRT_STEPS) * STEPS * W + \
                (b_q1 % SQRT_STEPS) * STEPS + \
                r_q1 + \
                g_q1 * W); \
            int offset2 = 3 * ((b_q2 / SQRT_STEPS) * STEPS * W + \
                (b_q2 % SQRT_STEPS) * STEPS + \
                r_q2 + \
                g_q2 * W); \
            float *pixel1 = table + offset1; \
            float *pixel2 = table + offset2; \
            float r_weight = r * STEPS - r_q1; \
            float g_weight = g * STEPS - g_q1; \
            float b_weight = b * STEPS - b_q1; \
            r = pixel1[0] * (1.0 - r_weight) + pixel2[0] * r_weight; \
            g = pixel1[1] * (1.0 - g_weight) + pixel2[1] * g_weight; \
            b = pixel1[2] * (1.0 - b_weight) + pixel2[2] * b_weight; \
 \
/* store the source pixel */ \
		    if(is_yuv) \
		    { \
			    float y, u, v; \
			    YUV::rgb_to_yuv_f(r, g, b, y, u, v); \
			    r = y; \
			    g = u + 0.5; \
			    b = v + 0.5; \
		    } \
		    if(max == 0xff) \
		    { \
			    CLAMP(r, 0, 1); \
			    CLAMP(g, 0, 1); \
			    CLAMP(b, 0, 1); \
		    } \
/* if(i == 345 && j == 1170) */ \
/* printf("DO_IT %d %f %f %f\n", __LINE__, r, g, b); */ \
		    row[0] = (type)(r * max); \
		    row[1] = (type)(g * max); \
		    row[2] = (type)(b * max); \
		    row += components; \
        } \
    } \
}


void LUTUnit::process_package(LoadPackage *package)
{
    LUTPackage *pkg = (LUTPackage*)package;
    VFrame *data = plugin->get_input(0);
    float *table = (float*)plugin->table->get_rows()[0];
	int h = data->get_h();
	int w = data->get_w();
    switch(data->get_color_model())
    {
        case BC_RGB888: DO_IT(uint8_t, 3, 0xff, 0); break;
        case BC_RGBA8888: DO_IT(uint8_t, 4, 0xff, 0); break;
        case BC_RGB_FLOAT: DO_IT(float, 3, 1.0, 0); break;
        case BC_RGBA_FLOAT: DO_IT(float, 4, 1.0, 0); break;
        case BC_YUV888: DO_IT(uint8_t, 3, 0xff, 1); break;
        case BC_YUVA8888: DO_IT(uint8_t, 4, 0xff, 1); break;
    }
}


REGISTER_PLUGIN(LUTEffect)
LUTEffect::LUTEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
    table = 0;
    reconfigure = 1;
}
LUTEffect::~LUTEffect()
{
    delete engine;
    delete table;
}

PLUGIN_CLASS_MEMBERS(LUTConfig);
NEW_WINDOW_MACRO(LUTEffect, LUTWindow)
LOAD_CONFIGURATION_MACRO(LUTEffect, LUTConfig)
const char* LUTEffect::plugin_title() 
{ 
    return N_("LUT"); 
}
VFrame* LUTEffect::new_picon()
{
    return 0;
}
int LUTEffect::is_realtime()
{
    return 1;
}
void LUTEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->get_data());
	output.tag.set_title("LUT");
	output.tag.set_property("PATH", config.path.c_str());
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}
void LUTEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->get_data());
    while(!input.read_tag())
    {
        if(input.tag.title_is("LUT"))
        {
            config.path.assign(input.tag.get_value("PATH"));
        }
    }
}
void LUTEffect::update_gui()
{
	if(thread)
	{
		int reconfigure = load_configuration();
		if(reconfigure) 
		{
    		((LUTWindow*)thread->window)->lock_window("LUTEffect::update_gui");
			((LUTWindow*)thread->window)->update();
    		((LUTWindow*)thread->window)->unlock_window();
		}
	}
}
int LUTEffect::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
    reconfigure |= load_configuration();
	int use_opengl = get_use_opengl();

// load the LUT
    if(reconfigure)
    {
        FILE *fd = fopen(config.path.c_str(), "r");
	    if(!fd)
	    {
		    printf("LUTEffect::process_buffer %d: couldn't read %s\n", 
                __LINE__, config.path.c_str());
            delete table;
            table = 0;
	    }
        else
        {
		    fseek(fd, 0, SEEK_END);
		    int size = ftell(fd);
		    fseek(fd, 0, SEEK_SET);
		    unsigned char *buffer = new unsigned char[size + 4];
		    int _ = fread(buffer + 4, size, 1, fd);
		    buffer[0] = size >> 24;
		    buffer[1] = (size >> 16) & 0xff;
		    buffer[2] = (size >> 8) & 0xff;
		    buffer[3] = size & 0xff;

		    VFrame *src = new VFrame(buffer);
// convert it to RGB float
            if(src->get_w() != W || 
                src->get_h() != H ||
                (src->get_color_model() != BC_RGBA8888 &&
                    src->get_color_model() != BC_RGB888))
            {
                printf("LUTEffect::process_buffer %d LUT image must be %dx%d RGB888\n", 
                    __LINE__,
                    W,
                    H);
                src->dump(4);
            }
            else
            {
                delete table;
// this forces opengl to reupload the texture
                table = new VFrame;
                table->set_use_shm(0);
                table->reallocate(
		            0,   // Data if shared.  0 if not
		            -1,             // shmid if IPC.  -1 if not
		            0,  // planes if shared YUV.  0 if not
		            0,
		            0,
		            W, 
		            H, 
		            BC_RGB_FLOAT, 
		            -1);
                int components = cmodel_components(src->get_color_model());
                for(int i = 0; i < H; i++)
                {
                    uint8_t *src_row = src->get_rows()[i];
                    float *dst_row = (float*)table->get_rows()[i];
                    for(int j = 0; j < W; j++)
                    {
                        dst_row[j * 3] = (float)src_row[0] / 255;
                        dst_row[j * 3 + 1] = (float)src_row[1] / 255;
                        dst_row[j * 3 + 2] = (float)src_row[2] / 255;
                        src_row += components;
                    }
                }
            }

		    delete [] buffer;
            delete src;
        }
        
    }

    read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		use_opengl);
    if(table)
    {
	    if(use_opengl) 
        {
            run_opengl();
        }
        else
        {
    	    if(!engine) engine = new LUTServer(this);
            engine->process_packages();
        }
    }
    reconfigure = 0;

	return 0;
}


int LUTEffect::handle_opengl()
{
#ifdef HAVE_GL
// load the output & LUT into textures
	get_output()->to_texture();

// don't know if the texture is retained between calls & since it's 512x512
// it might as well be reuploaded
    table->enable_opengl();
    table->init_screen();
// this only uploads when the texture or pbuffer changes
    table->to_texture();
//printf("LUTEffect::handle_opengl %d\n", __LINE__);
//table->dump(4);

    get_output()->enable_opengl();
    get_output()->init_screen();
	get_output()->bind_texture(0);
	table->bind_texture(1);
    

    int is_yuv = cmodel_is_yuv(get_output()->get_color_model());
    static const char *head = 
	    "uniform sampler2D output_tex;\n"
	    "uniform sampler2D table_tex;\n"
	    "uniform int steps;\n"
	    "uniform int sqrt_steps;\n"
        "\n"
        "void main()\n"
	    "{\n"
	    "    gl_FragColor = texture2D(output_tex, gl_TexCoord[0].st);\n";
    static const char *mane = 
        "    int size = steps * sqrt_steps;\n"
        "// quantize it\n"
        "    int r_q1 = int(gl_FragColor.r * float(steps));\n"
        "    int g_q1 = int(gl_FragColor.g * float(steps));\n"
        "    int b_q1 = int(gl_FragColor.b * float(steps));\n"
        "    int r_q2 = r_q1 + 1;\n"
        "    int g_q2 = g_q1 + 1;\n"
        "    int b_q2 = b_q1 + 1;\n"
        "    r_q1 = int(clamp(float(r_q1), 0.0, float(steps - 1)));\n"
        "    g_q1 = int(clamp(float(g_q1), 0.0, float(steps - 1)));\n"
        "    b_q1 = int(clamp(float(b_q1), 0.0, float(steps - 1)));\n"
        "    r_q2 = int(clamp(float(r_q2), 0.0, float(steps - 1)));\n"
        "    g_q2 = int(clamp(float(g_q2), 0.0, float(steps - 1)));\n"
        "    b_q2 = int(clamp(float(b_q2), 0.0, float(steps - 1)));\n"
        "// look up in table\n"
        "    int offset_y1 = int(b_q1 / sqrt_steps) * steps + g_q1;\n"
        "    int offset_x1 = int(mod(float(b_q1), float(sqrt_steps))) * steps + r_q1;\n"
        "    int offset_y2 = int(b_q2 / sqrt_steps) * steps + g_q2;\n"
        "    int offset_x2 = int(mod(float(b_q2), float(sqrt_steps))) * steps + r_q2;\n"
        "    float r_weight = gl_FragColor.r * float(steps) - float(r_q1);\n"
        "    float g_weight = gl_FragColor.g * float(steps) - float(g_q1);\n"
        "    float b_weight = gl_FragColor.b * float(steps) - float(b_q1);\n"
        "    vec4 lut_color1 = texture2D(table_tex, vec2(float(offset_x1) / float(size), float(offset_y1) / float(size)));\n"
        "    vec4 lut_color2 = texture2D(table_tex, vec2(float(offset_x2) / float(size), float(offset_y2) / float(size)));\n"
        "// interpolate 2 quantized values\n"
		"	 gl_FragColor.r = mix(lut_color1.r, lut_color2.r, r_weight);\n"
		"	 gl_FragColor.g = mix(lut_color1.g, lut_color2.g, g_weight);\n"
		"	 gl_FragColor.b = mix(lut_color1.b, lut_color2.b, b_weight);\n";


	int shader = VFrame::make_shader(0, 
		head, 
		is_yuv ? YUV_TO_RGB_FRAG("gl_FragColor") : "", 
		mane, 
		is_yuv ? RGB_TO_YUV_FRAG("gl_FragColor") : "", 
        "}\n",
		0);
	if(shader > 0)
	{
		glUseProgram(shader);
	    glUniform1i(glGetUniformLocation(shader, "output_tex"), 0);
	    glUniform1i(glGetUniformLocation(shader, "table_tex"), 1);
	    glUniform1i(glGetUniformLocation(shader, "steps"), STEPS);
	    glUniform1i(glGetUniformLocation(shader, "sqrt_steps"), SQRT_STEPS);
	}

// don't use interpolation to look up table values
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	get_output()->draw_texture();

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
    return 0;
}









