/*
 * CINELERRA
 * Copyright (C) 2008-2024 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "swapchannels.h"
#include "theme.h"
#include "vframe.h"



#include <stdint.h>
#include <string.h>


#define RED_SRC 0
#define GREEN_SRC 1
#define BLUE_SRC 2
#define ALPHA_SRC 3
#define NO_SRC 4
#define MAX_SRC 5
#define TOTAL_SRC 6


// no reason to support more than 4 shared tracks
#define MAX_INPUTS 16
#define RENDERED_CHANNELS -1

// 1st buffer is output
// output buffers beyond 0 are undefined & should be muted
#define OUTPUT_BUFFER 0

REGISTER_PLUGIN(SwapMain)








SwapConfig::SwapConfig()
{
	r_channel = RED_SRC;
	g_channel = GREEN_SRC;
	b_channel = BLUE_SRC;
    a_channel = ALPHA_SRC;
    r_layer = 0;
    g_layer = 0;
    b_layer = 0;
    a_layer = 0;
}


int SwapConfig::equivalent(SwapConfig &that)
{
	return (r_channel == that.r_channel &&
		g_channel == that.g_channel &&
		b_channel == that.b_channel &&
		a_channel == that.a_channel &&
        r_layer == that.r_layer &&
		g_layer == that.g_layer &&
		b_layer == that.b_layer &&
		a_layer == that.a_layer);
}

void SwapConfig::copy_from(SwapConfig &that)
{
	r_channel = that.r_channel;
	g_channel = that.g_channel;
	b_channel = that.b_channel;
	a_channel = that.a_channel;
	r_layer = that.r_layer;
	g_layer = that.g_layer;
	b_layer = that.b_layer;
	a_layer = that.a_layer;
}





#define WINDOW_W DP(400)
#define WINDOW_H DP(170)


SwapWindow::SwapWindow(SwapMain *plugin)
 : PluginClientWindow(plugin,
	WINDOW_W, 
	WINDOW_H, 
	WINDOW_W, 
	WINDOW_H, 
	0)
{
	this->plugin = plugin;
}

SwapWindow::~SwapWindow()
{
}

	
void SwapWindow::create_objects()
{
	int margin = client->get_theme()->widget_border;
	int x = margin, y = margin;

    const char* titles[] = {
        _("-> Red"),
        _("-> Green"),
        _("-> Blue"),
        _("-> Alpha")
    };
    int total_titles = sizeof(titles) / sizeof(char*);
    int title_w = 0;
    for(int i = 0; i < total_titles; i++)
    {
        int w = BC_Title::calculate_w(this, titles[i]);
        if(w > title_w) title_w = w;
    }

    int menu_w = 0;
    for(int i = 0; i < TOTAL_SRC; i++)
    {
        int w = BC_PopupMenu::calculate_w(this, SwapMain::output_to_text(i));
        if(w > menu_w) menu_w = w;
    }

    int x2 = margin + menu_w + margin;
    int x3 = x2 + title_w + margin;
    int y1 = y;
    int source_track_w = x2 - margin * 2;
//printf("SwapWindow::create_objects %d menu_w=%d\n", __LINE__, menu_w);

    BC_Title *title;
    add_subwindow(title = new BC_Title(x, y, _("Source buffer")));
    
    
    
    
    
    if(x + title->get_w() + margin > x2)
        x2 = x + title->get_w() + margin;
    add_subwindow(title = new BC_Title(x2, y, _("Source channel")));
    if(x2 + title->get_w() + margin > x3)
        x3 = x2 + title->get_w() + margin;
    
    add_subwindow(title = new BC_Title(x3, y, _("Dest channel")));
    y += title->get_h() + margin;

// printf("SwapWindow::create_objects %d total_tracks=%d\n", 
// __LINE__, plugin->get_total_buffers());

	add_subwindow(new BC_Title(x3, y, titles[0]));
	add_subwindow(r_channel = new SwapChannelMenu(plugin, &(plugin->config.r_channel), x2, y, menu_w));
	r_channel->create_objects();
    add_subwindow(r_track = new SwapLayerMenu(plugin, &(plugin->config.r_layer), x, y, source_track_w));
    r_track->create_objects();

	y += r_channel->get_h() + margin;
	add_subwindow(new BC_Title(x3, y, titles[1]));
	add_subwindow(g_channel = new SwapChannelMenu(plugin, &(plugin->config.g_channel), x2, y, menu_w));
	g_channel->create_objects();
    add_subwindow(g_track = new SwapLayerMenu(plugin, &(plugin->config.g_layer), x, y, source_track_w));
    g_track->create_objects();

	y += r_channel->get_h() + margin;
	add_subwindow(new BC_Title(x3, y, titles[2]));
	add_subwindow(b_channel = new SwapChannelMenu(plugin, &(plugin->config.b_channel), x2, y, menu_w));
	b_channel->create_objects();
    add_subwindow(b_track = new SwapLayerMenu(plugin, &(plugin->config.b_layer), x, y, source_track_w));
    b_track->create_objects();

	y += r_channel->get_h() + margin;
	add_subwindow(new BC_Title(x3, y, titles[3]));
	add_subwindow(a_channel = new SwapChannelMenu(plugin, &(plugin->config.a_channel), x2, y, menu_w));
	a_channel->create_objects();
    add_subwindow(a_track = new SwapLayerMenu(plugin, &(plugin->config.a_layer), x, y, source_track_w));
    a_track->create_objects();

	show_window();
	flush();
}









SwapChannelMenu::SwapChannelMenu(SwapMain *client, 
    int *output, 
    int x, 
    int y, 
    int w)
 : BC_PopupMenu(x, y, w, client->output_to_text(*output))
{
	this->client = client;
	this->output = output;
}

int SwapChannelMenu::handle_event()
{
	client->send_configure_change();
	return 1;
}

void SwapChannelMenu::create_objects()
{
	add_item(new SwapItem(this, client->output_to_text(RED_SRC)));
	add_item(new SwapItem(this, client->output_to_text(GREEN_SRC)));
	add_item(new SwapItem(this, client->output_to_text(BLUE_SRC)));
	add_item(new SwapItem(this, client->output_to_text(ALPHA_SRC)));
	add_item(new SwapItem(this, client->output_to_text(NO_SRC)));
	add_item(new SwapItem(this, client->output_to_text(MAX_SRC)));
}




SwapItem::SwapItem(SwapChannelMenu *menu, const char *title)
 : BC_MenuItem(title)
{
	this->menu = menu;
}

int SwapItem::handle_event()
{
	menu->set_text(get_text());
	*(menu->output) = menu->client->text_to_output(get_text());
	menu->handle_event();
	return 1;
}










SwapLayerMenu::SwapLayerMenu(SwapMain *plugin, int *output, int x, int y, int w)
 : BC_PopupMenu(x, y, w, plugin->output_to_track(*output))
{
	this->plugin = plugin;
	this->output = output;
}

int SwapLayerMenu::handle_event()
{
	plugin->send_configure_change();
	return 1;
}

void SwapLayerMenu::create_objects()
{
    char string[BCTEXTLEN];
    for(int i = 0; i < MAX_INPUTS; i++)
    {
        sprintf(string, "%d", i);
        add_item(new SwapLayerItem(this, string));
    }
}




SwapLayerItem::SwapLayerItem(SwapLayerMenu *menu, const char *title)
 : BC_MenuItem(title)
{
	this->menu = menu;
}

int SwapLayerItem::handle_event()
{
	menu->set_text(get_text());
	*(menu->output) = atoi(get_text());
	menu->handle_event();
	return 1;
}


















SwapMain::SwapMain(PluginServer *server)
 : PluginVClient(server)
{
	reset();
	
}

SwapMain::~SwapMain()
{
	
	
}

void SwapMain::reset()
{
	temp = 0;
}


const char* SwapMain::plugin_title()  { return N_("Swap channels"); }
int SwapMain::is_synthesis() { return 1; }
int SwapMain::is_realtime()  { return 1; }
int SwapMain::is_multichannel() { return 1; }

NEW_PICON_MACRO(SwapMain)
NEW_WINDOW_MACRO(SwapMain, SwapWindow)


void SwapMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("SWAPCHANNELS");
	output.tag.set_property("RED", config.r_channel);
	output.tag.set_property("GREEN", config.g_channel);
	output.tag.set_property("BLUE", config.b_channel);
	output.tag.set_property("ALPHA", config.a_channel);
	output.tag.set_property("R_LAYER", config.r_layer);
	output.tag.set_property("G_LAYER", config.g_layer);
	output.tag.set_property("B_LAYER", config.b_layer);
	output.tag.set_property("A_LAYER", config.a_layer);
	output.append_tag();
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void SwapMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SWAPCHANNELS"))
			{
				config.r_channel = input.tag.get_property("RED", config.r_channel);
				config.g_channel = input.tag.get_property("GREEN", config.g_channel);
				config.b_channel = input.tag.get_property("BLUE", config.b_channel);
				config.a_channel = input.tag.get_property("ALPHA", config.a_channel);
				config.r_layer = input.tag.get_property("R_LAYER", config.r_layer);
				config.g_layer = input.tag.get_property("G_LAYER", config.g_layer);
				config.b_layer = input.tag.get_property("B_LAYER", config.b_layer);
				config.a_layer = input.tag.get_property("A_LAYER", config.a_layer);
			}
		}
	}
}

void SwapMain::update_gui()
{
	if(thread) 
	{
		load_configuration();
		thread->window->lock_window();
		((SwapWindow*)thread->window)->r_channel->set_text(output_to_text(config.r_channel));
		((SwapWindow*)thread->window)->g_channel->set_text(output_to_text(config.g_channel));
		((SwapWindow*)thread->window)->b_channel->set_text(output_to_text(config.b_channel));
		((SwapWindow*)thread->window)->a_channel->set_text(output_to_text(config.a_channel));
		((SwapWindow*)thread->window)->r_track->set_text(output_to_track(config.r_layer));
		((SwapWindow*)thread->window)->g_track->set_text(output_to_track(config.g_layer));
		((SwapWindow*)thread->window)->b_track->set_text(output_to_track(config.b_layer));
		((SwapWindow*)thread->window)->a_track->set_text(output_to_track(config.a_layer));
		thread->window->unlock_window();
	}
}


int SwapMain::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	
 	read_data(prev_keyframe);
	return 1;
}
























#define MAXMINSRC(src, min, max) \
	(src == MAX_SRC ? max : min)

#define SWAP_CHANNELS(type, min, max, components) \
{ \
	int h = temp->get_h(); \
	int w = temp->get_w(); \
	int r_channel = config.r_channel; \
	int g_channel = config.g_channel; \
	int b_channel = config.b_channel; \
	int a_channel = config.a_channel; \
	int r_layer = config.r_layer; \
	int g_layer = config.g_layer; \
	int b_layer = config.b_layer; \
	int a_layer = config.a_layer; \
 \
/* set alpha inputs to 100% for 3 channels */ \
	if(components == 3) \
	{ \
		if(r_channel == ALPHA_SRC) r_channel = MAX_SRC; \
		if(g_channel == ALPHA_SRC) g_channel = MAX_SRC; \
		if(b_channel == ALPHA_SRC) b_channel = MAX_SRC; \
	} \
 \
 \
	for(int i = 0; i < h; i++) \
	{ \
		type *inrow = (type*)temp->get_rows()[i]; \
		type *outrow = (type*)frame[OUTPUT_BUFFER]->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			if(r_channel < NO_SRC) \
			{ \
                if(r_layer == input_track) \
            	    *outrow++ = *(inrow + r_channel); \
                else \
                    outrow++; \
			} \
            else \
				*outrow++ = MAXMINSRC(r_channel, 0, max); \
 \
			if(g_channel < NO_SRC) \
            { \
                if(g_layer == input_track) \
				    *outrow++ = *(inrow + g_channel); \
                else \
                    outrow++; \
			} \
            else \
				*outrow++ = MAXMINSRC(g_channel, min, max); \
 \
			if(b_channel < NO_SRC) \
            { \
                if(b_layer == input_track) \
				    *outrow++ = *(inrow + b_channel); \
                else \
                    outrow++; \
            } \
			else \
				*outrow++ = MAXMINSRC(b_channel, min, max); \
 \
			if(components == 4) \
			{ \
				if(a_channel < NO_SRC) \
				{ \
                    if(a_layer == input_track) \
                	    *outrow++ = *(inrow + a_channel); \
                    else \
                        outrow++; \
				} \
                else \
					*outrow++ = MAXMINSRC(a_channel, 0, max); \
			} \
 \
			inrow += components; \
		} \
	} \
}

static int have_layer(int *layers, int layer, int total)
{
    for(int i = 0; i < total; i++) 
        if(layers[i] == layer) return 1;
    return 0;
}

int SwapMain::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

// determine which input layers to read.
// Up to MAX_INPUTS can be read
    int input_layers[MAX_INPUTS + 1];
    int total_inputs = 0;
    if(!have_layer(input_layers, config.r_layer, total_inputs))
        input_layers[total_inputs++] = config.r_layer;
    if(!have_layer(input_layers, config.g_layer, total_inputs))
        input_layers[total_inputs++] = config.g_layer;
    if(!have_layer(input_layers, config.b_layer, total_inputs))
        input_layers[total_inputs++] = config.b_layer;
    if(!have_layer(input_layers, config.a_layer, total_inputs))
        input_layers[total_inputs++] = config.a_layer;
// 1 more in case all the channels are rendered
    if(total_inputs == 0) 
        input_layers[total_inputs++] = RENDERED_CHANNELS;

// for(int i = 0; i < get_total_buffers(); i++)
// {
// printf("SwapMain::process_buffer %d buffer %d\n", 
// __LINE__,
// i);
// frame[i]->dump(4);
// }

    if(total_inputs > 0)
    {
	    temp = new_temp(frame[0]->get_w(), 
		    frame[0]->get_h(), 
		    frame[0]->get_color_model());
    }

    first = 1;
    for(int i = 0; i < total_inputs; i++)
    {
        input_track = input_layers[i];
        if(input_track != RENDERED_CHANNELS &&
            input_track < get_total_buffers())
	        read_frame(temp, 
		        input_track, 
		        start_position, 
		        frame_rate,
		        get_use_opengl());

// opengl it once for each source track
	    if(get_use_opengl())
	    {
		    run_opengl();
	    }
        else
        {
	        switch(temp->get_color_model())
	        {
		        case BC_RGB_FLOAT:
			        SWAP_CHANNELS(float, 0, 1, 3);
			        break;
		        case BC_RGBA_FLOAT:
			        SWAP_CHANNELS(float, 0, 1, 4);
			        break;
		        case BC_RGB888:
			        SWAP_CHANNELS(unsigned char, 0, 0xff, 3);
			        break;
		        case BC_YUV888:
			        SWAP_CHANNELS(unsigned char, 0x80, 0xff, 3);
			        break;
		        case BC_RGBA8888:
			        SWAP_CHANNELS(unsigned char, 0, 0xff, 4);
			        break;
		        case BC_YUVA8888:
			        SWAP_CHANNELS(unsigned char, 0x80, 0xff, 4);
			        break;
	        }
            
            
// clear all the unused output buffers
            if(first)
            {
                for(int i = 0; i < get_total_buffers(); i++)
                {
                    if(i != OUTPUT_BUFFER)
                    {
                        frame[i]->clear_frame();
                    }
                }
            }
        }
        first = 0;
    }

// for(int i = 0; i < get_total_buffers(); i++)
// {
// printf("SwapMain::process_buffer %d channel %d\n", 
// __LINE__,
// i);
// frame[i]->dump(4);
// }

	
	return 0;
}


const char* SwapMain::output_to_text(int value)
{
	switch(value)
	{
		case RED_SRC:
			return _("Red");
			break;
		case GREEN_SRC:
			return _("Green");
			break;
		case BLUE_SRC:
			return _("Blue");
			break;
		case ALPHA_SRC:
			return _("Alpha");
			break;
		case NO_SRC:
			return _("0%");
			break;
		case MAX_SRC:
			return _("100%");
			break;
		default:
			return "";
			break;
	}
}

int SwapMain::text_to_output(const char *text)
{
	if(!strcmp(text, _("Red"))) return RED_SRC;
	if(!strcmp(text, _("Green"))) return GREEN_SRC;
	if(!strcmp(text, _("Blue"))) return BLUE_SRC;
	if(!strcmp(text, _("Alpha"))) return ALPHA_SRC;
	if(!strcmp(text, _("0%"))) return NO_SRC;
	if(!strcmp(text, _("100%"))) return MAX_SRC;
	return 0;
}

const char* SwapMain::output_to_track(int number)
{
    switch(number)
    {
        case 0: return "0"; break;
        case 1: return "1"; break;
        case 2: return "2"; break;
        case 3: return "3"; break;
    }
    return "";
}

void SwapMain::color_switch(char *output_frag, 
    int src_channel, 
    int src_layer, 
    const char *dst_channel)
{
// write the dest channel
    if(src_channel >= NO_SRC || src_layer == input_track)
    {
// start the line
	    sprintf(output_frag + strlen(output_frag), 
            "	out_color.%s = ", 
            dst_channel);
// the source channel
	    switch(src_channel)
	    {
		    case RED_SRC: strcat(output_frag, "in_color.r;\n"); break;
		    case GREEN_SRC: strcat(output_frag, "in_color.g;\n"); break;
		    case BLUE_SRC: strcat(output_frag, "in_color.b;\n"); break;
		    case ALPHA_SRC: strcat(output_frag, "in_color.a;\n"); break;
		    case NO_SRC: strcat(output_frag, "chroma_offset;\n"); break;
		    case MAX_SRC: strcat(output_frag, "1.0;\n"); break;
	    }
    }
}


int SwapMain::handle_opengl()
{
#ifdef HAVE_GL
	char output_frag[BCTEXTLEN];
	sprintf(output_frag, 
		"uniform sampler2D src_tex;\n"
		"uniform sampler2D dst_tex;\n"
		"uniform vec2 dst_tex_dimensions;\n"
		"uniform float chroma_offset;\n"
		"void main()\n"
		"{\n"
// the input pixel of the current input layer
		"	vec4 in_color = texture2D(src_tex, gl_TexCoord[0].st);\n"
// read back the output pixel
		"	vec4 out_color = texture2D(dst_tex, gl_FragCoord.xy / dst_tex_dimensions);\n");

// dynamically create the lines that transfer the src channels to the dest
	color_switch(output_frag, config.r_channel, config.r_layer, "r");
	color_switch(output_frag, config.g_channel, config.g_layer, "g");
	color_switch(output_frag, config.b_channel, config.b_layer, "b");
	color_switch(output_frag, config.a_channel, config.a_layer, "a");

	strcat(output_frag, 
		"	gl_FragColor = out_color;\n"
		"}\n");

// DEBUG
// if(input_track == 1)
// sprintf(output_frag,
// "uniform sampler2D src_tex;\n"
// "uniform sampler2D dst_tex;\n"
// "uniform vec2 dst_tex_dimensions;\n"
// "uniform float chroma_offset;\n"
// "void main()\n"
// "{\n"
// "        vec4 in_color = texture2D(src_tex, gl_TexCoord[0].st);\n"
// "        vec4 out_color = texture2D(dst_tex, gl_FragCoord.xy / dst_tex_dimensions);\n"
// "        out_color.r = 1.0;\n"
// "        out_color.g = 1.0;\n"
// "        out_color.b = 1.0;\n"
// "        out_color.a = 1.0;\n"
// "        gl_FragColor = out_color;\n"
// "}\n");



    VFrame *src = temp;
    VFrame *dst = get_output(OUTPUT_BUFFER);

	dst->enable_opengl();
	dst->init_screen();

// Read destination back to texture
	dst->to_texture();

// move source to texture
	src->enable_opengl();
	src->init_screen();
	src->to_texture();

	dst->enable_opengl();
	dst->init_screen();
	src->bind_texture(0);
	dst->bind_texture(1);

//printf("SwapMain::handle_opengl %d shader=\n%s\n", __LINE__, output_frag);
	unsigned int shader_id = VFrame::make_shader(0,
		output_frag,
		0);
	glUseProgram(shader_id);
	glUniform1i(glGetUniformLocation(shader_id, "src_tex"), 0);
	glUniform1i(glGetUniformLocation(shader_id, "dst_tex"), 1);
	glUniform1f(glGetUniformLocation(shader_id, "chroma_offset"), 
		cmodel_is_yuv(dst->get_color_model()) ? 0.5 : 0.0);
	glUniform2f(glGetUniformLocation(shader_id, "dst_tex_dimensions"), 
		(float)dst->get_texture_w(), 
		(float)dst->get_texture_h());

	src->draw_texture();

	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
	dst->set_opengl_state(VFrame::SCREEN);


// clear all the unused output buffers
    if(first)
    {
        for(int i = 0; i < get_total_buffers(); i++)
        {
            if(i != OUTPUT_BUFFER)
            {
                dst = get_output(i);
                dst->enable_opengl();
// make the results more predictable by clearing the unused outputs
                dst->clear_pbuffer();
                dst->set_opengl_state(VFrame::SCREEN);
            }
        }
    }

#endif // HAVE_GL
    return 0;
}



