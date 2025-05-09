/*
 * CINELERRA
 * Copyright (C) 2008-2023 Adam Williams <broadcast at earthling dot net>
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

#include "dissolve.h"
#include "edl.inc"
#include "language.h"
#include "overlayframe.h"
//#include "picon_png.h"
#include "vframe.h"

#include <string.h>

PluginClient* new_plugin(PluginServer *server)
{
	return new DissolveMain(server);
}





DissolveMain::DissolveMain(PluginServer *server)
 : PluginVClient(server)
{
	overlayer = 0;
}

DissolveMain::~DissolveMain()
{
	delete overlayer;
}

const char* DissolveMain::plugin_title() { return N_("Dissolve"); }
int DissolveMain::is_video() { return 1; }
int DissolveMain::is_transition() { return 1; }
int DissolveMain::uses_gui() { return 0; }

//NEW_PICON_MACRO(DissolveMain)


int DissolveMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	fade = (float)PluginClient::get_source_position() / 
			PluginClient::get_total_len();

// Use hardware
	if(get_use_opengl())
	{
		run_opengl();
		return 0;
	}

// Use software
	if(!overlayer) overlayer = new OverlayFrame(get_project_smp() + 1);

// There is a problem when dissolving from a big picture to a small picture.
// In order to make it dissolve correctly, we have to manually decrese alpha of big picture.
// 	switch (outgoing->get_color_model())
// 	{
// 		case BC_RGBA8888:
// 		case BC_YUVA8888:
// 		{
// 			uint8_t** data_rows = (uint8_t **)outgoing->get_rows();
// 			int w = outgoing->get_w();
// 			int h = outgoing->get_h(); 
// 			for(int i = 0; i < h; i++) 
// 			{
// 				uint8_t* alpha_chan = data_rows[i] + 3; 
// 				for(int j = 0; j < w; j++) 
// 				{
// 					*alpha_chan = (uint8_t) (*alpha_chan * (1-fade));
// 					alpha_chan+=4;
// 				}
// 			}
// 			break;
// 		}
// 		case BC_RGBA_FLOAT:
// 		{
// 			float** data_rows = (float **)outgoing->get_rows();
// 			int w = outgoing->get_w();
// 			int h = outgoing->get_h(); 
// 			for(int i = 0; i < h; i++) 
// 			{ 
// 				float* alpha_chan = data_rows[i] + 3; // 3 since this is floats 
// 				for(int j = 0; j < w; j++) 
// 				{
// 					*alpha_chan = *alpha_chan * (1-fade);
// 					alpha_chan += sizeof(float);
// 				} 
// 			}
// 			break;
// 		}
// 		default:
// 			break;
// 	}

// uint8_t *i = incoming->get_rows()[0];
// uint8_t *o = outgoing->get_rows()[0];
// printf("DissolveMain::process_realtime %d fade=%f i=%d %d o=%d %d\n", 
// __LINE__, 
// fade,
// i[0],
// i[3],
// o[0],
// o[3]);

// porter duff fails us for alpha
// copy what OPENGL does
#define DISSOLVE(type, temp, max, chroma) \
{ \
    temp opacity = fade * max; \
    temp transparency = max - opacity; \
    type **out_rows = (type**)outgoing->get_rows(); \
    type **in_rows = (type**)incoming->get_rows(); \
    for(int i = 0; i < h; i++) \
    { \
        type *out_row = out_rows[i]; \
        type *in_row = in_rows[i]; \
        for(int j = 0; j < w; j++) \
        { \
            temp out_r = out_row[0]; \
            temp out_g = out_row[1]; \
            temp out_b = out_row[2]; \
            temp out_a = out_row[3]; \
            temp in_r = *in_row++; \
            temp in_g = *in_row++; \
            temp in_b = *in_row++; \
            temp in_a = *in_row++; \
            *out_row++ = (out_r * transparency + \
                in_r * opacity) / max; \
            *out_row++ = ((out_g - chroma) * transparency + \
                (in_g - chroma) * opacity) / max + chroma; \
            *out_row++ = ((out_b - chroma) * transparency + \
                (in_b - chroma) * opacity) / max + chroma; \
            *out_row++ = (out_a * transparency + \
                (in_a * opacity)) / max; \
        } \
    } \
}

    if(cmodel_has_alpha(outgoing->get_color_model()))
    {
        int w = outgoing->get_w();
        int h = outgoing->get_h();
        switch (outgoing->get_color_model())
	    {
		    case BC_RGBA8888:
                DISSOLVE(uint8_t, int16_t, 255, 0);
                break;
		    case BC_YUVA8888:
                DISSOLVE(uint8_t, int16_t, 255, 128);
                break;
		    case BC_RGBA_FLOAT:
                DISSOLVE(float, float, 1.0, 0.0);
                break;
        }
    }
    else
    {

	    overlayer->overlay(outgoing, 
		    incoming, 
		    0, 
		    0, 
		    incoming->get_w(),
		    incoming->get_h(),
		    0,
		    0,
		    incoming->get_w(),
		    incoming->get_h(),
		    fade,
		    TRANSFER_NORMAL,
 		    NEAREST_NEIGHBOR);
    }

// printf("DissolveMain::process_realtime %d fade=%f %p %p colormodels=%d %d %d %d -> %d %d\n", 
// __LINE__, 
// fade,
// incoming,
// outgoing,
// incoming->get_color_model(),
// outgoing->get_color_model(),
// i[0],
// i[3],
// o[0],
// o[3]);

	return 0;
}

int DissolveMain::handle_opengl()
{
#ifdef HAVE_GL

// Read images from RAM
	get_input()->to_texture();
	get_output()->to_texture();

// Create output pbuffer
	get_output()->enable_opengl();

	VFrame::init_screen(get_output()->get_w(), get_output()->get_h());

// Enable output texture
	get_output()->bind_texture(0);
// Draw output texture
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	get_output()->draw_texture();

// Draw input texture
	get_input()->bind_texture(0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1, 1, 1, fade);
	get_input()->draw_texture();
// restore default
	glColor4f(1, 1, 1, 1);


	glDisable(GL_BLEND);
	get_output()->set_opengl_state(VFrame::SCREEN);


#endif
    return 0;
}



