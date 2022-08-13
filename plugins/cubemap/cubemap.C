/*
 * CINELERRA
 * Copyright (C) 2021 Adam Williams <broadcast at earthling dot net>
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

// this converts a 6 panel gootube cubemap into something watchable

#include "affine.h"
#include "cursors.h"
#include "language.h"
#include "cubemap.h"
#include "theme.h"






REGISTER_PLUGIN(CubeMap)



CubeMapConfig::CubeMapConfig()
{
    x_shift = -1;
    y_scale = 1.0f;
}

int CubeMapConfig::equivalent(CubeMapConfig &that)
{
	return (x_shift == that.x_shift) &&
        EQUIV(y_scale, that.y_scale);
}

void CubeMapConfig::copy_from(CubeMapConfig &that)
{
    x_shift = that.x_shift;
    y_scale = that.y_scale;
}

void CubeMapConfig::interpolate(CubeMapConfig &prev, 
	CubeMapConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
    x_shift = prev.x_shift;
    y_scale = prev.y_scale * prev_scale + next.y_scale * next_scale;
}

void CubeMapConfig::boundaries()
{
    CLAMP(x_shift, -4, 4);
    CLAMP(y_scale, 0.1, 1.0);
}












CubeMapWindow::CubeMapWindow(CubeMap *plugin)
 : PluginClientWindow(plugin,
	DP(300), 
	DP(200), 
	DP(300), 
	DP(200), 
	0)
{
	this->plugin = plugin; 
}

CubeMapWindow::~CubeMapWindow()
{
}

void CubeMapWindow::create_objects()
{
    int margin = client->get_theme()->widget_border;
	int x = margin, y = margin;
    
    BC_Title *title;
    int y2 = y + 
        (BC_Pot::calculate_h() - 
        BC_Title::calculate_h(this, _("XYZ"), MEDIUMFONT)) / 2;
    add_tool(title = new BC_Title(x, y2, _("X Shift:")));
    int x1 = x + title->get_w() + margin;
    add_tool(x_shift = new CubeMapInt(plugin, 
        &plugin->config.x_shift, 
        x1, 
        y, 
        -4, 
        4));
    y += x_shift->get_h() + margin;
    y2 = y + 
        (BC_Pot::calculate_h() - 
        BC_Title::calculate_h(this, _("XYZ"), MEDIUMFONT)) / 2;
    add_tool(title = new BC_Title(x, y2, _("Y Scale:")));
    x1 = x + title->get_w() + margin;
    add_tool(y_scale = new CubeMapFloat(plugin, 
        &plugin->config.y_scale, 
        x1, 
        y, 
        0.1, 
        1.0));
    
	show_window();
}



int CubeMapWindow::resize_event(int w, int h)
{
	return 1;
}

void CubeMapWindow::update()
{
    x_shift->update((int64_t)plugin->config.x_shift);
    y_scale->update(plugin->config.y_scale);
}




CubeMapFloat::CubeMapFloat(CubeMap *plugin, float *output, int x, int y, float min, float max)
 : BC_FPot(x, y, *output, min, max)
{
    this->plugin = plugin;
    this->output = output;
    set_precision(0.01);
}

int CubeMapFloat::handle_event()
{
    *output = get_value();
    plugin->send_configure_change();
    return 1;
}



CubeMapInt::CubeMapInt(CubeMap *plugin, 
    int *output, 
    int x, 
    int y, 
    int min, 
    int max)
 : BC_IPot(x, y, *output, min, max)
{
    this->plugin = plugin;
    this->output = output;
}

int CubeMapInt::handle_event()
{
    *output = get_value();
    plugin->send_configure_change();
    return 1;
}





CubeMap::CubeMap(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	temp = 0;
}

CubeMap::~CubeMap()
{
	
	if(engine) delete engine;
	if(temp) delete temp;
}

const char* CubeMap::plugin_title() { return N_("Cube Unmap"); }
int CubeMap::is_realtime() { return 1; }



NEW_WINDOW_MACRO(CubeMap, CubeMapWindow)
LOAD_CONFIGURATION_MACRO(CubeMap, CubeMapConfig)



void CubeMap::update_gui()
{
	if(thread)
	{
		int reconfigured = load_configuration();
        if(reconfigured)
        {
    		thread->window->lock_window();
            ((CubeMapWindow*)thread->window)->update();
	    	thread->window->unlock_window();
        }
	}
}





void CubeMap::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("CUBEMAP");
    output.tag.set_property("X_SHIFT", config.x_shift);
    output.tag.set_property("Y_SCALE", config.y_scale);
	output.append_tag();
	output.terminate_string();
}

void CubeMap::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("CUBEMAP"))
			{
                config.x_shift = input.tag.get_property("X_SHIFT", config.x_shift);
                config.y_scale = input.tag.get_property("Y_SCALE", config.y_scale);
			}
		}
	}
    
    config.boundaries();
}

typedef struct
{
    int x1, y1, x2, y2;
} cuberect_t;

int CubeMap::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	int need_reconfigure = load_configuration();


// Opengl does some funny business with stretching.
	int use_opengl = get_use_opengl();

	int w = frame->get_w();
	int h = frame->get_h();
	int color_model = frame->get_color_model();

// create translation table for 4 horizontal tiles
//    int dst_y1 = h / 4;
//    int dst_y2 = h * 3 / 4;
    float dst_h = (float)h * config.y_scale;
    int dst_y1 = (int)(h / 2 - dst_h / 2);
    int dst_y2 = (int)(dst_y1 + dst_h);

// destination rects
    cuberect_t dest_rects[TOTAL_FACES];
    dest_rects[0].x1 = 0;
    dest_rects[0].y1 = dst_y1;
    dest_rects[0].x2 = w / 4;
    dest_rects[0].y2 = dst_y2;

    dest_rects[1].x1 = w / 4;
    dest_rects[1].y1 = dst_y1;
    dest_rects[1].x2 = w / 2;
    dest_rects[1].y2 = dst_y2;

    dest_rects[2].x1 = w / 2;
    dest_rects[2].y1 = dst_y1;
    dest_rects[2].x2 = w * 3 / 4;
    dest_rects[2].y2 = dst_y2;

    dest_rects[3].x1 = w * 3 / 4;
    dest_rects[3].y1 = dst_y1;
    dest_rects[3].x2 = w;
    dest_rects[3].y2 = dst_y2;

// source rects
// top left to center left 
    translation[0].src_x1 = 0;
    translation[0].src_y1 = 0;
    translation[0].src_x2 = w / 3;
    translation[0].src_y2 = h / 2;
    translation[0].rotation = 0;

// top center to top left 
    translation[1].src_x1 = w / 3;
    translation[1].src_y1 = 0;
    translation[1].src_x2 = w * 2 / 3;
    translation[1].src_y2 = h / 2;
    translation[1].rotation = 0;

// top right to top center
    translation[2].src_x1 = w * 2 / 3;
    translation[2].src_y1 = 0;
    translation[2].src_x2 = w;
    translation[2].src_y2 = h / 2;
    translation[2].rotation = 0;

// bottom center to top right
    translation[3].src_x1 = w / 3;
    translation[3].src_y1 = h / 2;
    translation[3].src_x2 = w * 2 / 3;
    translation[3].src_y2 = h;
    translation[3].rotation = -90;

// match sources to dests
    int x_shift = config.x_shift;
    if(x_shift < 0)
    {
        x_shift += TOTAL_FACES;
    }
    for(int i = 0; i < TOTAL_FACES; i++)
    {
        translation[i].dst_x1 = dest_rects[(i + x_shift) % TOTAL_FACES].x1;
        translation[i].dst_x2 = dest_rects[(i + x_shift) % TOTAL_FACES].x2;
        translation[i].dst_y1 = dest_rects[(i + x_shift) % TOTAL_FACES].y1;
        translation[i].dst_y2 = dest_rects[(i + x_shift) % TOTAL_FACES].y2;
    }


	if(!engine) engine = new AffineEngine(get_project_smp() + 1,
		get_project_smp() + 1);

	if(!use_opengl && !temp)
	{
		temp = new VFrame(0,
			-1,
			w,
			h,
			color_model,
			-1);
	}
    
    
    
	if(use_opengl)
	{
        read_frame(frame, 
		    0, 
		    start_position, 
		    frame_rate,
		    use_opengl);
		return run_opengl();
    }
    else
    {
	    read_frame(temp, 
		    0, 
		    start_position, 
		    frame_rate,
		    use_opengl);
        frame->clear_frame();
        for(int i = 0; i < TOTAL_FACES; i++)
        {
            cubetable_t *table = &translation[i];
            engine->set_out_viewport(table->dst_x1, 
                table->dst_y1, 
                table->dst_x2 - table->dst_x1, 
                table->dst_y2 - table->dst_y1);
            float dst_x[4];
            float dst_y[4];
            int rotation_index = table->rotation / 90;
            if(rotation_index < 0)
            {
                rotation_index += 4;
            }
//rotation_index = 0;



// need to scale entire frame such that the 
// desired region fills the output viewport
            float src_center_x = (float)(table->src_x1 + table->src_x2) / 2;
            float src_center_y = (float)(table->src_y1 + table->src_y2) / 2;
            float dst_center_x = (float)(table->dst_x1 + table->dst_x2) / 2;
            float dst_center_y = (float)(table->dst_y1 + table->dst_y2) / 2;

            if(table->rotation == 0)
            {
                float scale_x = (float)(table->dst_x2 - table->dst_x1) / 
                    (float)(table->src_x2 - table->src_x1);
                float scale_y = (float)(table->dst_y2 - table->dst_y1) / 
                    (float)(table->src_y2 - table->src_y1);
// top left
                dst_x[0] = (dst_center_x - src_center_x * scale_x) * 100 / w;
                dst_y[0] = (dst_center_y - src_center_y * scale_y) * 100 / h;
// top right
                dst_x[1] = (dst_center_x + ((float)w - src_center_x) * scale_x) * 100 / w;
                dst_y[1] = (dst_center_y - src_center_y * scale_y) * 100 / h;
// bottom right
                dst_x[2] = (dst_center_x + ((float)w - src_center_x) * scale_x) * 100 / w;
                dst_y[2] = (dst_center_y + ((float)h - src_center_y) * scale_y) * 100 / h;
// bottom left
                dst_x[3] = (dst_center_x - src_center_x * scale_x) * 100 / w;
                dst_y[3] = (dst_center_y + ((float)h - src_center_y) * scale_y) * 100 / h;
            }
            else
            if(table->rotation == -90)
            {
                float scale_x = (float)(table->dst_x2 - table->dst_x1) / 
                    (float)(table->src_y2 - table->src_y1);
                float scale_y = (float)(table->dst_y2 - table->dst_y1) / 
                    (float)(table->src_x2 - table->src_x1);
// top left
                dst_x[0] = (dst_center_x - src_center_y * scale_x) * 100 / w;
                dst_y[0] = (dst_center_y - src_center_x * scale_y) * 100 / h;
// top right
                dst_x[1] = (dst_center_x + ((float)h - src_center_y) * scale_x) * 100 / w;
                dst_y[1] = (dst_center_y - src_center_x * scale_y) * 100 / h;
// bottom right
                dst_x[2] = (dst_center_x + ((float)h - src_center_y) * scale_x) * 100 / w;
                dst_y[2] = (dst_center_y + ((float)w - src_center_x) * scale_y) * 100 / h;
// bottom left
                dst_x[3] = (dst_center_x - src_center_y * scale_x) * 100 / w;
                dst_y[3] = (dst_center_y + ((float)w - src_center_x) * scale_y) * 100 / h;
// printf("CubeMap::process_buffer %d scale_x=%f scale_y=%f\n",
// __LINE__,
// scale_x, 
// scale_y);
            }



            engine->process(frame, // output
                temp, // input
                temp, // temp
                AffineEngine::PERSPECTIVE, // mode
                dst_x[rotation_index],
                dst_y[rotation_index],
                dst_x[(rotation_index + 1) % 4],
                dst_y[(rotation_index + 1) % 4],
                dst_x[(rotation_index + 2) % 4],
                dst_y[(rotation_index + 2) % 4],
                dst_x[(rotation_index + 3) % 4],
                dst_y[(rotation_index + 3) % 4],
                1); // forward
        }
    }




	return 1;
}


int CubeMap::handle_opengl()
{
#ifdef HAVE_GL
    VFrame *output = get_output();
	float border_color[] = { 0, 0, 0, 0 };
	if(cmodel_is_yuv(output->get_color_model()))
	{
		border_color[1] = 0.5;
		border_color[2] = 0.5;
	}
	if(!cmodel_has_alpha(output->get_color_model()))
	{
		border_color[3] = 1.0;
	}

// transfer input frame to a texture
	int w = output->get_w();
	int h = output->get_h();
	int color_model = output->get_color_model();

    output->enable_opengl();
    output->init_screen();
    output->to_texture();
    output->bind_texture(0);
    output->clear_pbuffer();
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
// draw segments of the texture with transformations
	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1.0);
    for(int i = 0; i < TOTAL_FACES; i++)
    {
        cubetable_t *table = &translation[i];
        int flip_y = 0;
        int rotation_index = -table->rotation / 90;
        if(rotation_index < 0)
        {
            rotation_index += 4;
        }
        float src_x[4];
        float src_y[4];
        src_x[0] = (float)table->src_x1 / output->get_texture_w();
        src_y[0] = (float)table->src_y1 / output->get_texture_h();
        src_x[1] = (float)table->src_x2 / output->get_texture_w();
        src_y[1] = (float)table->src_y1 / output->get_texture_h();
        src_x[2] = (float)table->src_x2 / output->get_texture_w();
        src_y[2] = (float)table->src_y2 / output->get_texture_h();
        src_x[3] = (float)table->src_x1 / output->get_texture_w();
        src_y[3] = (float)table->src_y2 / output->get_texture_h();


	    glTexCoord2f(src_x[rotation_index], src_y[rotation_index]);
	    glVertex3f(table->dst_x1, flip_y ? -table->dst_y1 : -table->dst_y2, 0);

	    glTexCoord2f(src_x[(rotation_index + 1) % 4], src_y[(rotation_index + 1) % 4]);
	    glVertex3f(table->dst_x2, flip_y ? -table->dst_y1 : -table->dst_y2, 0);

	    glTexCoord2f(src_x[(rotation_index + 2) % 4], src_y[(rotation_index + 2) % 4]);
	    glVertex3f(table->dst_x2, flip_y ? -table->dst_y2 : -table->dst_y1, 0);

	    glTexCoord2f(src_x[(rotation_index + 3) % 4], src_y[(rotation_index + 3) % 4]);
	    glVertex3f(table->dst_x1, flip_y ? -table->dst_y2 : -table->dst_y1, 0);

    }
	glEnd();
    output->set_opengl_state(VFrame::SCREEN);
#endif
}






