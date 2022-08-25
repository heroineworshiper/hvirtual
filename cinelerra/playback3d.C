/*
 * CINELERRA
 * Copyright (C) 2009-2022 Adam Williams <broadcast at earthling dot net>
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

#define GL_GLEXT_PROTOTYPES

#include "bcsignals.h"
#include "bcwindowbase.h"
#include "canvas.h"
#include "clip.h"
#include "condition.h"
#include "cwindowgui.inc"
#include "maskautos.h"
#include "maskauto.h"
#include "mutex.h"
#include "overlayframe.inc"
#include "playback3d.h"
#include "pluginclient.h"
#include "pluginvclient.h"
#include "transportque.inc"
#include "vframe.h"

#ifdef HAVE_GL
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <string.h>
#include <unistd.h>


// Shaders
// These should be passed to VFrame::make_shader to construct shaders.
// Can't hard code sampler2D

static const char *cmodel_head = 
	"uniform sampler2D src_tex;\n"
	"uniform float rowspan; // bytes per Y line\n"
	"uniform float image_w; // number of pixels\n"
	"uniform float image_h; // number of pixels\n"
	"uniform float texture_w; // number of pixels\n"
	"uniform float texture_h; // number of pixels\n"
	"uniform float pixel_w; // pixel size as fraction of texture w\n"
	"uniform float pixel_h; // pixel size as fraction of texture h\n"
	"uniform float half_w; // half pixel size as fraction of texture w\n"
	"uniform float half_h; // half pixel size as fraction of texture h\n"
	"uniform float u_offset; // bytes to U\n"
	"uniform float v_offset; // bytes to V\n"
 	"const mat3 yuv_to_rgb_matrix = mat3(\n"
 	"	 1,       1,        1, \n"
 	"	 0,       -0.34414, 1.77200, \n"
 	"	 1.40200, -0.71414, 0);\n"
 	"const mat3 rgb_to_yuv_matrix = mat3(\n"
 	"	 0.29900, -0.16874, 0.50000, \n"
 	"	 0.58700, -0.33126, -0.41869, \n"
 	"	 0.11400, 0.50000,  -0.08131);\n"
    "\n"
    "// offset in bytes\n"
    "float get_value8(float offset)\n"
    "{\n"
    "    float in_row = floor(offset / texture_w / 4.0);\n"
    "    float in_col2 = offset - in_row * texture_w * 4.0;\n"
    "    float in_col = floor(in_col2 / 4.0);\n"
    "    int in_channel = int(in_col2 - in_col * 4.0);\n"
    "    vec2 coord = vec2(in_col * pixel_w + half_w, in_row * pixel_h + half_h);\n"
    "    vec4 color = texture2D(src_tex, coord);\n"
    "    return color[in_channel];\n"
    "}\n"
    "\n"
    "// offset in bytes\n"
    "float get_value10(float offset)\n"
    "{\n"
    "    float in_row = floor(offset / 2.0 / texture_w / 2.0);\n"
    "    float in_col2 = offset / 2.0 - in_row * texture_w * 2.0;\n"
    "    float in_col = floor(in_col2 / 2.0);\n"
    "    int in_channel = int(in_col2 - in_col * 2.0) * 2;\n"
    "    vec2 coord = vec2(in_col * pixel_w + half_w, in_row * pixel_h + half_h);\n"
    "    vec4 color = texture2D(src_tex, coord);\n"
    "    float low = color[in_channel] * 255.0;\n"
    "    float high = color[in_channel + 1] * 255.0;\n"
    "    return (low + high * 256.0) / 1023.0;\n"
    "}\n"
    "\n"
	"void main()\n"
	"{\n"
	"    vec2 coord = gl_TexCoord[0].st;\n";


static const char *YUV420P_to_yuv_frag = 
    "    float offset = rowspan * floor(coord.y / pixel_h) + floor(coord.x / pixel_w);\n"
    "    vec3 yuv;\n"
    "    yuv.r = get_value8(offset);\n"
    "    offset = u_offset + (rowspan / 2.0) * floor((coord.y / 2.0) / pixel_h) + floor((coord.x / 2.0) / pixel_w);\n"
    "    yuv.g = get_value8(offset);\n"
    "    offset = v_offset + (rowspan / 2.0) * floor((coord.y / 2.0) / pixel_h) + floor((coord.x / 2.0) / pixel_w);\n"
    "    yuv.b = get_value8(offset);\n"
    "    gl_FragColor = vec4(yuv.rgb, 1.0);\n"
	"}\n";

static const char *YUV420P_to_rgb_frag = 
    "    float offset = rowspan * floor(coord.y / pixel_h) + floor(coord.x / pixel_w);\n"
    "    vec3 yuv = vec3(0.0, -0.5, -0.5);\n"
    "    yuv.r = get_value8(offset);\n"
    "    offset = u_offset + (rowspan / 2.0) * floor((coord.y / 2.0) / pixel_h) + floor((coord.x / 2.0) / pixel_w);\n"
    "    yuv.g += get_value8(offset);\n"
    "    offset = v_offset + (rowspan / 2.0) * floor((coord.y / 2.0) / pixel_h) + floor((coord.x / 2.0) / pixel_w);\n"
    "    yuv.b += get_value8(offset);\n"
    "    gl_FragColor = vec4(yuv_to_rgb_matrix * yuv, 1.0);\n"
	"}\n";

static const char *YUV9P_to_yuv_frag = 
    "    float offset = rowspan * floor(coord.y / pixel_h) + floor(coord.x / pixel_w);\n"
    "    vec3 yuv;\n"
    "    yuv.r = get_value8(offset);\n"
    "    offset = u_offset + (rowspan / 4.0) * floor((coord.y / 4.0) / pixel_h) + floor((coord.x / 4.0) / pixel_w);\n"
    "    yuv.g = get_value8(offset);\n"
    "    offset = v_offset + (rowspan / 4.0) * floor((coord.y / 4.0) / pixel_h) + floor((coord.x / 4.0) / pixel_w);\n"
    "    yuv.b = get_value8(offset);\n"
    "    gl_FragColor = vec4(yuv.rgb, 1.0);\n"
	"}\n";

static const char *YUV9P_to_rgb_frag = 
    "    float offset = rowspan * floor(coord.y / pixel_h) + floor(coord.x / pixel_w);\n"
    "    vec3 yuv = vec3(0.0, -0.5, -0.5);\n"
    "    yuv.r = get_value8(offset);\n"
    "    offset = u_offset + (rowspan / 4.0) * floor((coord.y / 4.0) / pixel_h) + floor((coord.x / 4.0) / pixel_w);\n"
    "    yuv.g += get_value8(offset);\n"
    "    offset = v_offset + (rowspan / 4.0) * floor((coord.y / 4.0) / pixel_h) + floor((coord.x / 4.0) / pixel_w);\n"
    "    yuv.b += get_value8(offset);\n"
    "    gl_FragColor = vec4(yuv_to_rgb_matrix * yuv, 1.0);\n"
	"}\n";

static const char *YUV422P_to_yuv_frag = 
    "    float offset = rowspan * floor(coord.y / pixel_h) + floor(coord.x / pixel_w);\n"
    "    vec3 yuv;\n"
    "    yuv.r = get_value8(offset);\n"
    "    offset = u_offset + (rowspan / 2.0) * floor(coord.y / pixel_h) + floor((coord.x / 2.0) / pixel_w);\n"
    "    yuv.g = get_value8(offset);\n"
    "    offset = v_offset + (rowspan / 2.0) * floor(coord.y / pixel_h) + floor((coord.x / 2.0) / pixel_w);\n"
    "    yuv.b = get_value8(offset);\n"
    "    gl_FragColor = vec4(yuv.rgb, 1.0);\n"
	"}\n";

static const char *YUV422P_to_rgb_frag = 
    "    float offset = rowspan * floor(coord.y / pixel_h) + floor(coord.x / pixel_w);\n"
    "    vec3 yuv = vec3(0.0, -0.5, -0.5);\n"
    "    yuv.r = get_value8(offset);\n"
    "    offset = u_offset + (rowspan / 2.0) * floor(coord.y / pixel_h) + floor((coord.x / 2.0) / pixel_w);\n"
    "    yuv.g += get_value8(offset);\n"
    "    offset = v_offset + (rowspan / 2.0) * floor(coord.y / pixel_h) + floor((coord.x / 2.0) / pixel_w);\n"
    "    yuv.b += get_value8(offset);\n"
    "    gl_FragColor = vec4(yuv_to_rgb_matrix * yuv, 1.0);\n"
	"}\n";

static const char *YUV420P10LE_to_yuv_frag = 
    "    float offset = rowspan * floor(coord.y / pixel_h) + floor(coord.x * 2.0 / pixel_w);\n"
    "    vec3 yuv;\n"
    "    yuv.r = get_value10(offset);\n"
    "    offset = u_offset + (rowspan / 2.0) * floor((coord.y / 2.0) / pixel_h) + floor(coord.x / pixel_w);\n"
    "    yuv.g = get_value10(offset);\n"
    "    offset = v_offset + (rowspan / 2.0) * floor((coord.y / 2.0) / pixel_h) + floor(coord.x / pixel_w);\n"
    "    yuv.b = get_value10(offset);\n"
    "    gl_FragColor = vec4(yuv.rgb, 1.0);\n"
	"}\n";

static const char *YUV420P10LE_to_rgb_frag = 
    "    float offset = rowspan * floor(coord.y / pixel_h) + floor(coord.x * 2.0 / pixel_w);\n"
    "    vec3 yuv = vec3(0.0, -0.5, -0.5);\n"
    "    yuv.r = get_value10(offset);\n"
    "    offset = u_offset + (rowspan / 2.0) * floor((coord.y / 2.0) / pixel_h) + floor(coord.x / pixel_w);\n"
    "    yuv.g += get_value10(offset);\n"
    "    offset = v_offset + (rowspan / 2.0) * floor((coord.y / 2.0) / pixel_h) + floor(coord.x / pixel_w);\n"
    "    yuv.b += get_value10(offset);\n"
    "    gl_FragColor = vec4(yuv_to_rgb_matrix * yuv, 1.0);\n"
	"}\n";

static const char *yuv_to_rgb_frag = 
	"	vec4 yuva = texture2D(src_tex, coord);\n"
	"	yuva.rgb -= vec3(0, 0.5, 0.5);\n"
	"	gl_FragColor = vec4(yuv_to_rgb_matrix * yuva.rgb, yuva.a);\n"
	"}\n";

static const char *yuva_to_yuv_frag = 
	"	vec4 yuva = texture2D(src_tex, coord);\n"
	"   float a = yuva.a;\n"
	"   float anti_a = 1.0 - a;\n"
	"	yuva.r *= a;\n"
	"   yuva.g = yuva.g * a + 0.5 * anti_a;\n"
	"   yuva.b = yuva.b * a + 0.5 * anti_a;\n"
	"   yuva.a = 1.0;\n"
	"	gl_FragColor = yuva;\n"
	"}\n";

static const char *yuva_to_rgb_frag = 
	"	vec4 yuva = texture2D(src_tex, coord);\n"
	"	yuva.rgb -= vec3(0, 0.5, 0.5);\n"
	"   yuva.rgb = yuv_to_rgb_matrix * yuva.rgb;\n"
	"   yuva.rgb *= yuva.a;\n"
	"   yuva.a = 1.0;\n"
	"	gl_FragColor = yuva;\n"
	"}\n";

static const char *rgb_to_yuv_frag = 
	"	vec4 rgba = texture2D(src_tex, coord);\n"
	"   rgba.rgb = rgb_to_yuv_matrix * rgba.rgb;\n"
	"   rgba.rgb += vec3(0, 0.5, 0.5);\n"
	"	gl_FragColor = rgba;\n"
	"}\n";


static const char *rgba_to_rgb_frag = 
	"	vec4 rgba = texture2D(src_tex, coord);\n"
	"	rgba.rgb *= rgba.a;\n"
	"   rgba.a = 1.0;\n"
	"	gl_FragColor = rgba;\n"
	"}\n";

static const char *rgba_to_yuv_frag = 
	"	vec4 rgba = texture2D(src_tex, coord);\n"
	"   rgba.rgb *= rgba.a;\n"
	"   rgba.a = 1.0;\n"
	"   rgba.rgb = rgb_to_yuv_matrix * rgba.rgb;\n"
	"   rgba.rgb += vec3(0, 0.5, 0.5);\n"
	"	gl_FragColor = rgba;\n"
	"}\n";

static const char *get_pixels_frag = 
	"uniform sampler2D dst_tex;\n"  // tex2
	"uniform vec2 dst_tex_dimensions;\n" // tex2_dimensions
	"uniform vec3 chroma_offset;\n"
	"void main()\n"
	"{\n"
	"	vec4 result_color;\n"
	"	vec4 dst_color = texture2D(dst_tex, gl_FragCoord.xy / dst_tex_dimensions);\n" // canvas
	"	vec4 src_color = gl_FragColor;\n" // read back from YUV -> RGB conversion
	"	src_color.rgb -= chroma_offset;\n"
	"	dst_color.rgb -= chroma_offset;\n";

static const char *put_pixels_frag = 
	"	result_color.rgb = mix(dst_color.rgb, result_color.rgb, src_color.a);\n"
	"	result_color.rgb += chroma_offset;\n"
	"	result_color.a = src_color.a + (1.0 - src_color.a) * dst_color.a;\n"
//	"	result_color.a = max(src_color.a, dst_color.a);\n"
	"	gl_FragColor = result_color;\n"
	"}\n";

static const char *blend_normal_frag = 
	"	result_color.rgb = src_color.rgb;\n";

static const char *blend_add_frag = 
	"	result_color.rgb = dst_color.rgb + src_color.rgb;\n";

static const char *blend_max_frag = 
	"	result_color.r = max(abs(dst_color.r, src_color.r);\n"
	"	result_color.g = max(abs(dst_color.g, src_color.g);\n"
	"	result_color.b = max(abs(dst_color.b, src_color.b);\n";

static const char *blend_min_frag = 
	"	result_color.r = min(abs(dst_color.r, src_color.r);\n"
	"	result_color.g = min(abs(dst_color.g, src_color.g);\n"
	"	result_color.b = min(abs(dst_color.b, src_color.b);\n";

static const char *blend_subtract_frag = 
	"	result_color.rgb = dst_color.rgb - src_color.rgb;\n";

static const char *blend_multiply_frag = 
	"	result_color.rgb = dst_color.rgb * src_color.rgb;\n";

static const char *blend_divide_frag = 
	"	result_color.rgb = dst_color.rgb / src_color.rgb;\n"
	"	if(src_color.r == 0.0) result_color.r = 1.0;\n"
	"	if(src_color.g == 0.0) result_color.g = 1.0;\n"
	"	if(src_color.b == 0.0) result_color.b = 1.0;\n";

static const char *replace_frag = 
	"uniform vec3 chroma_offset;\n"
	"void main()\n"
	"{\n"
    "    vec3 src_color = gl_FragColor;\n"
	"    gl_FragColor = vec3(src_color.r, src_color.g, src_color.b, src_color.a);\n"
	"}\n";

// static const char *multiply_alpha_frag = 
// 	"uniform vec3 chroma_offset;\n"
// 	"void main()\n"
// 	"{\n"
// 	"	vec4 src_color = gl_FragColor;\n" // read back from YUV -> RGB conversion
// 	"	src_color.rgb -= chroma_offset;\n"
// 	"	src_color.rgb *= vec3(src_color.a, src_color.a, src_color.a);\n"
// 	"	gl_FragColor.rgb = src_color.rgb + chroma_offset;\n"
// 	"}\n";

static const char *multiply_alpha_frag = 
	"void main()\n"
	"{\n"
	"	vec4 src_color = gl_FragColor;\n" // read back from YUV -> RGB conversion
	"	src_color.rgb *= vec3(src_color.a, src_color.a, src_color.a);\n"
	"	gl_FragColor.rgb = src_color.rgb;\n"
	"}\n";

// static const char *checker_alpha_frag = 
// 	"void main()\n"
//     "{\n"
//     "    gl_FragColor.rgb = vec3(gl_TexCoord[0].st.x, gl_TexCoord[0].st.x, gl_TexCoord[0].st.x);\n"
//     "    gl_FragColor.a = 1.0;\n"
//     "}\n";

static const char *read_texture_frag = 
	"uniform sampler2D src_tex;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(src_tex, gl_TexCoord[0].st);\n"
	"}\n";

static const char *multiply_mask4_frag = 
	"uniform sampler2D src_tex;\n"
	"uniform sampler2D tex1;\n"
	"uniform float scale;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(src_tex, gl_TexCoord[0].st);\n"
	"	gl_FragColor.a *= texture2D(tex1, gl_TexCoord[0].st / vec2(scale, scale)).r;\n"
	"}\n";

static const char *multiply_mask3_frag = 
	"uniform sampler2D src_tex;\n"
	"uniform sampler2D tex1;\n"
	"uniform float scale;\n"
	"uniform bool is_yuv;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(src_tex, gl_TexCoord[0].st);\n"
	"   float a = texture2D(tex1, gl_TexCoord[0].st / vec2(scale, scale)).r;\n"
	"	gl_FragColor.rgb *= vec3(a, a, a);\n"
	"}\n";

static const char *multiply_yuvmask3_frag = 
	"uniform sampler2D src_tex;\n"
	"uniform sampler2D tex1;\n"
	"uniform float scale;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(src_tex, gl_TexCoord[0].st);\n"
	"   float a = texture2D(tex1, gl_TexCoord[0].st / vec2(scale, scale)).r;\n"
	"	gl_FragColor.gb -= vec2(0.5, 0.5);\n"
	"	gl_FragColor.rgb *= vec3(a, a, a);\n"
	"	gl_FragColor.gb += vec2(0.5, 0.5);\n"
	"}\n";

static const char *fade_rgba_frag =
	"uniform sampler2D src_tex;\n"
	"uniform float alpha;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(src_tex, gl_TexCoord[0].st);\n"
	"	gl_FragColor.a *= alpha;\n"
	"}\n";

static const char *fade_yuv_frag =
	"uniform sampler2D src_tex;\n"
	"uniform float alpha;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(src_tex, gl_TexCoord[0].st);\n"
	"	gl_FragColor.r *= alpha;\n"
	"	gl_FragColor.gb -= vec2(0.5, 0.5);\n"
	"	gl_FragColor.g *= alpha;\n"
	"	gl_FragColor.b *= alpha;\n"
	"	gl_FragColor.gb += vec2(0.5, 0.5);\n"
	"}\n";








Playback3DCommand::Playback3DCommand()
 : BC_SynchronousCommand()
{
	canvas = 0;
	is_nested = 0;
    video_on = 0;
}

void Playback3DCommand::copy_from(BC_SynchronousCommand *command)
{
	Playback3DCommand *ptr = (Playback3DCommand*)command;
	this->canvas = ptr->canvas;
	this->is_cleared = ptr->is_cleared;

	this->in_x1 = ptr->in_x1;
	this->in_y1 = ptr->in_y1;
	this->in_x2 = ptr->in_x2;
	this->in_y2 = ptr->in_y2;
	this->out_x1 = ptr->out_x1;
	this->out_y1 = ptr->out_y1;
	this->out_x2 = ptr->out_x2;
	this->out_y2 = ptr->out_y2;
	this->alpha = ptr->alpha;
	this->mode = ptr->mode;
	this->interpolation_type = ptr->interpolation_type;

	this->input = ptr->input;
	this->start_position_project = ptr->start_position_project;
    this->mask = ptr->mask;
	this->keyframe_set = ptr->keyframe_set;
	this->keyframe = ptr->keyframe;
	this->default_auto = ptr->default_auto;
	this->plugin_client = ptr->plugin_client;
	this->want_texture = ptr->want_texture;
	this->is_nested = ptr->is_nested;
	this->dst_cmodel = ptr->dst_cmodel;
    this->video_on = ptr->video_on;

	BC_SynchronousCommand::copy_from(command);
}




Playback3D::Playback3D()
 : BC_Synchronous()
{
	temp_texture = 0;
}

Playback3D::~Playback3D()
{
}




BC_SynchronousCommand* Playback3D::new_command()
{
	return new Playback3DCommand;
}



void Playback3D::handle_command(BC_SynchronousCommand *command)
{
//printf("Playback3D::handle_command 1 %d\n", command->command);
	switch(command->command)
	{
		case Playback3DCommand::WRITE_BUFFER:
			write_buffer_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::CLEAR_OUTPUT:
			clear_output_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::CLEAR_INPUT:
			clear_input_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::DO_CAMERA:
			do_camera_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::OVERLAY:
			overlay_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::DO_FADE:
			do_fade_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::DO_MASK:
			do_mask_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::PLUGIN:
			run_plugin_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::COPY_FROM:
			copy_from_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::CONVERT_CMODEL:
			convert_cmodel_sync((Playback3DCommand*)command);
			break;

// 		case Playback3DCommand::DRAW_REFRESH:
// 			draw_refresh_sync((Playback3DCommand*)command);
// 			break;
	}
//printf("Playback3D::handle_command 10\n");
}




void Playback3D::copy_from(Canvas *canvas, 
	VFrame *dst,
	VFrame *src,
	int want_texture)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::COPY_FROM;
	command.canvas = canvas;
	command.frame = dst;
	command.input = src;
	command.want_texture = want_texture;
	send_command(&command);
}

void Playback3D::copy_from_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	command->canvas->lock_canvas("Playback3D::copy_from_sync");
	BC_WindowBase *window = command->canvas->get_canvas();
	if(window)
	{
		window->lock_window("Playback3D:copy_from_sync");
		window->enable_opengl();

		if(command->input->get_opengl_state() == VFrame::SCREEN &&
			command->input->get_w() == command->frame->get_w() &&
			command->input->get_h() == command->frame->get_h())
		{
// printf("Playback3D::copy_from_sync 1 %d %d %d %d %d\n", 
// command->input->get_w(),
// command->input->get_h(),
// command->frame->get_w(),
// command->frame->get_h(),
// command->frame->get_color_model());
			int w = command->input->get_w();
			int h = command->input->get_h();
// With NVidia at least,
// 			if(command->input->get_w() % 4)
// 			{
// 				printf("Playback3D::copy_from_sync: w=%d not supported because it is not divisible by 4.\n", w);
// 			}
// 			else
// Copy to texture
			if(command->want_texture)
			{
//printf("Playback3D::copy_from_sync 1 dst=%p src=%p\n", command->frame, command->input);
// Screen_to_texture requires the source pbuffer enabled.
				command->input->enable_opengl();
				command->frame->screen_to_texture();
				command->frame->set_opengl_state(VFrame::TEXTURE);
			}
			else
// Copy from pbuffer to RAM
			{
// printf("Playback3D::copy_from_sync %d src=%dx%d dst=%dx%d color_model=%d rows=%p\n", 
// __LINE__, 
// command->input->get_w(),
// command->input->get_h(),
// command->frame->get_w(),
// command->frame->get_h(),
// command->frame->get_color_model(),
// command->frame->get_rows()[0]);
				command->input->enable_opengl();
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                int gl_cmodel = GL_RGB;
                if(cmodel_has_alpha(command->frame->get_color_model()))
                {
                    gl_cmodel = GL_RGBA;
                }
                int gl_type = GL_UNSIGNED_BYTE;
                if(cmodel_is_float(command->frame->get_color_model()))
                    gl_type = GL_FLOAT;

                if((w % 4))
                {
                    for(int i = 0; i < h; i++)
                    {
                        glReadPixels(0,
					        i,
					        w,
					        1,
					        gl_cmodel,
					        gl_type,
					        command->frame->get_rows()[i]);
                    }
                }
                else
                {
				    glReadPixels(0,
					    0,
					    w,
					    h,
					    gl_cmodel,
					    gl_type,
					    command->frame->get_rows()[0]);
				}

// do the software flip in the caller
//                command->frame->flip_vert();
				command->frame->set_opengl_state(VFrame::RAM);
// printf("Playback3D::copy_from_sync %d input=%p\n", 
// __LINE__,
// command->input);
//for(int i = 0; i < 2048; i++) command->frame->get_rows()[0][i] = 0xff;
			}
		}
		else
		{
			printf("Playback3D::copy_from_sync: invalid formats opengl_state=%d %dx%d -> %dx%d\n",
				command->input->get_opengl_state(),
				command->input->get_w(),
				command->input->get_h(),
				command->frame->get_w(),
				command->frame->get_h());
		}

		window->unlock_window();
	}
	command->canvas->unlock_canvas();
#endif
}




// void Playback3D::draw_refresh(Canvas *canvas, 
// 	VFrame *frame,
// 	float in_x1, 
// 	float in_y1, 
// 	float in_x2, 
// 	float in_y2, 
// 	float out_x1, 
// 	float out_y1, 
// 	float out_x2, 
// 	float out_y2)
// {
// 	Playback3DCommand command;
// 	command.command = Playback3DCommand::DRAW_REFRESH;
// 	command.canvas = canvas;
// 	command.frame = frame;
// 	command.in_x1 = in_x1;
// 	command.in_y1 = in_y1;
// 	command.in_x2 = in_x2;
// 	command.in_y2 = in_y2;
// 	command.out_x1 = out_x1;
// 	command.out_y1 = out_y1;
// 	command.out_x2 = out_x2;
// 	command.out_y2 = out_y2;
// 	send_command(&command);
// }
// 
// void Playback3D::draw_refresh_sync(Playback3DCommand *command)
// {
// 	command->canvas->lock_canvas("Playback3D::draw_refresh_sync");
// 	BC_WindowBase *window = command->canvas->get_canvas();
// 	if(window)
// 	{
// 		window->lock_window("Playback3D:draw_refresh_sync");
// 		window->enable_opengl();
// 
// // Read output pbuffer back to RAM in project colormodel
// // RGB 8bit is fastest for OpenGL to read back.
// 		command->frame->reallocate(0, 
// 			0,
// 			0,
// 			0,
// 			command->frame->get_w(), 
// 			command->frame->get_h(), 
// 			BC_RGB888, 
// 			-1);
// 		command->frame->to_ram();
// 
// 		window->clear_box(0, 
// 						0, 
// 						window->get_w(), 
// 						window->get_h());
// 		window->draw_vframe(command->frame,
// 							(int)command->out_x1, 
// 							(int)command->out_y1, 
// 							(int)(command->out_x2 - command->out_x1), 
// 							(int)(command->out_y2 - command->out_y1),
// 							(int)command->in_x1, 
// 							(int)command->in_y1, 
// 							(int)(command->in_x2 - command->in_x1), 
// 							(int)(command->in_y2 - command->in_y1),
// 							0);
// 
// 		window->unlock_window();
// 	}
// 	command->canvas->unlock_canvas();
// }





void Playback3D::write_buffer(Canvas *canvas, 
	VFrame *frame,
	float in_x1, 
	float in_y1, 
	float in_x2, 
	float in_y2, 
	float out_x1, 
	float out_y1, 
	float out_x2, 
	float out_y2, 
	int is_cleared)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::WRITE_BUFFER;
	command.canvas = canvas;
	command.frame = frame;
	command.in_x1 = in_x1;
	command.in_y1 = in_y1;
	command.in_x2 = in_x2;
	command.in_y2 = in_y2;
	command.out_x1 = out_x1;
	command.out_y1 = out_y1;
	command.out_x2 = out_x2;
	command.out_y2 = out_y2;
	command.is_cleared = is_cleared;
	send_command(&command);
}


void Playback3D::write_buffer_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::write_buffer_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *canvas = command->canvas->get_canvas();
		canvas->lock_window("Playback3D::write_buffer_sync");
// Update hidden cursor
		canvas->update_video_cursor();
// Make sure the OpenGL context is enabled first.
		canvas->enable_opengl();


//printf("Playback3D::write_buffer_sync %d opengl_state=%d\n", __LINE__, command->frame->get_opengl_state());
		switch(command->frame->get_opengl_state())
		{
// Upload texture and composite to screen
			case VFrame::RAM:
				command->frame->to_texture();
				draw_output(command);
				break;
// Composite texture to screen and swap buffer
			case VFrame::TEXTURE:
				draw_output(command);
				break;
			case VFrame::SCREEN:
// copy to texture & draw to screen with alpha multiply
// can't draw directly from screen to screen
                command->frame->enable_opengl();
                command->frame->screen_to_texture();
                draw_output(command);
// swap buffers only
//printf("Playback3D::write_buffer_sync %d swap buffers only\n", __LINE__);
//				canvas->flip_opengl();
				break;
			default:
				printf("Playback3D::write_buffer_sync unknown state\n");
				break;
		}
		canvas->unlock_window();
	}

	command->canvas->unlock_canvas();
}



void Playback3D::draw_output(Playback3DCommand *command)
{
#ifdef HAVE_GL
	int texture_id = command->frame->get_texture_id();
	BC_WindowBase *canvas = command->canvas->get_canvas();

// printf("Playback3D::draw_output %d texture_id=%d colormodel=%d canvas=%p\n", 
// __LINE__,
// texture_id,
// command->frame->get_color_model(),
// command->canvas->get_canvas());




	if(texture_id >= 0)
	{
// draw on the screen
   		canvas->enable_opengl();
		canvas_w = canvas->get_w();
		canvas_h = canvas->get_h();
// set up frustum
		VFrame::init_screen(canvas_w, canvas_h);

		if(!command->is_cleared)
		{
// If we get here, the virtual console was not used.
//			init_frame(command);
// clear the canvas
        	glClearColor(0.0, 0.0, 0.0, 0.0);
	        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

// Texture
// Undo any previous shader settings
		command->frame->bind_texture(0);




// Convert colormodel
		unsigned int frag_shader = 0;
        const char *shaders[] = { 0, 0, 0, 0 };
        int current_shader = 0;
		switch(command->frame->get_color_model())
		{
			case BC_YUV888:
			case BC_YUVA8888:
			case BC_YUV_FLOAT:
                shaders[current_shader++] = cmodel_head;
                shaders[current_shader++] = yuv_to_rgb_frag;
				break;
            default:
                shaders[current_shader++] = read_texture_frag;
                break;
		}


        if(cmodel_has_alpha(command->frame->get_color_model()))
        {
//            shaders[current_shader++] = checker_alpha_frag;
            shaders[current_shader++] = multiply_alpha_frag;
//printf("Playback3D::draw_output %d current_shader=%d\n", __LINE__, current_shader);
        }

        if(current_shader > 0)
        {
            frag_shader = VFrame::make_shader(0,
				shaders[0],
				shaders[1],
				shaders[2],
				0);
        }

		if(frag_shader > 0) 
		{
			glUseProgram(frag_shader);
			int variable = glGetUniformLocation(frag_shader, "src_tex");
// Set texture unit of the texture
			glUniform1i(variable, 0);
// Disable YUV for alpha multiply
//            glUniform3f(glGetUniformLocation(frag_shader, "chroma_offset"), 0.0, 0.0, 0.0);
		}

// multiply alpha
// 		if(cmodel_has_alpha(command->frame->get_color_model()))
// 		{
// 			glEnable(GL_BLEND);
// 			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 		}

		command->frame->draw_texture(command->in_x1, 
			command->in_y1,
			command->in_x2,
			command->in_y2,
			command->out_x1,
			command->out_y1,
			command->out_x2,
			command->out_y2,
			1);


// printf("Playback3D::draw_output 2 %f,%f %f,%f -> %f,%f %f,%f\n",
// command->in_x1,
// command->in_y1,
// command->in_x2,
// command->in_y2,
// command->out_x1,
// command->out_y1,
// command->out_x2,
// command->out_y2);

// disable frag shader
		glUseProgram(0);

		command->canvas->get_canvas()->flip_opengl();
		
	}
#endif
}


// void Playback3D::init_frame(Playback3DCommand *command)
// {
// #ifdef HAVE_GL
//     BC_WindowBase *canvas = command->canvas->get_canvas();
// 	canvas_w = canvas->get_w();
// 	canvas_h = canvas->get_h();
// 
// // printf("Playback3D::init_frame %d canvas_w=%d canvas_h=%d video_on=%d,%d cmodel=%d\n", 
// // __LINE__,
// // canvas_w,
// // canvas_h,
// // canvas->get_video_on(),
// // command->video_on,
// // command->frame->get_color_model());
// 
//     if(!command->frame)
//     {
//         printf("Playback3D::init_frame %d output_frame not set\n", __LINE__);
//         return;
//     }
// 
// // video mode with alpha needs the checker pattern
// //     if(command->video_on &&
// //         cmodel_has_alpha(command->frame->get_color_model()))
// //     {
// // printf("Playback3D::init_frame %d checker frame=%p state=%d\n", 
// // __LINE__,
// // command->frame,
// // command->frame->get_opengl_state());
// // 
// // 
// // // draw checker background
// //     	glClearColor(0.6, 0.6, 0.6, 1.0);
// // 	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
// // 
// //         glColor4f(0.4, 0.4, 0.4, 1.0);
// // 	    glDisable(GL_BLEND);
// // 		glBegin(GL_QUADS);
// // 		glNormal3f(0, 0, 1.0);
// // 
// //         for(int i = 0; i < canvas_h; i += CHECKER_H)
// //         {
// //             for(int j = ((i / CHECKER_H) % 2) * CHECKER_W; 
// //                 j < canvas_w; 
// //                 j += CHECKER_W * 2)
// //             {
// //                 glVertex3f((float)j, -(float)i, 0.0);
// //                 glVertex3f((float)(j + CHECKER_W), -(float)i, 0.0);
// //                 glVertex3f((float)(j + CHECKER_W), -(float)(i + CHECKER_H), 0.0);
// //                 glVertex3f((float)j, -(float)(i + CHECKER_H), 0.0);
// //             }
// //         }
// // 
// // 		glEnd();
// // 	    glColor4f(1.0, 1.0, 1.0, 1.0);
// //     }
// //     else
// // all modes draw to a pbuffer with alpha
//     {
// 
//         if(cmodel_is_yuv(command->frame->get_color_model()))
//         	glClearColor(0.0, 0.5, 0.5, 0.0);
//         else
//         	glClearColor(0.0, 0.0, 0.0, 0.0);
// 	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     }
// // DEBUG
// //glClearColor(1.0, 1.0, 1.0, 1.0);
// #endif
// }


void Playback3D::clear_output(Canvas *canvas, VFrame *output, int video_on)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::CLEAR_OUTPUT;
	command.canvas = canvas;
	command.frame = output;
    command.video_on = video_on;
	send_command(&command);
}

void Playback3D::clear_output_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::clear_output_sync");
	BC_WindowBase *canvas = command->canvas->get_canvas();
	if(canvas)
	{
		canvas->lock_window("Playback3D::clear_output_sync");
// must always enable OpenGL in the canvas to access the OpenGL context
		canvas->enable_opengl();

//printf("Playback3D::clear_output_sync %d frame=%p video_on=%d\n", 
//__LINE__, command->frame, command->video_on);
// Always blend on a pbuffer
//		if(!command->video_on)
//		{
			command->frame->enable_opengl();
//		}


//		init_frame(command);
        if(cmodel_is_yuv(command->frame->get_color_model()))
        	glClearColor(0.0, 0.5, 0.5, 0.0);
        else
        	glClearColor(0.0, 0.0, 0.0, 0.0);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		command->canvas->get_canvas()->unlock_window();
	}
	command->canvas->unlock_canvas();
}


void Playback3D::clear_input(Canvas *canvas, VFrame *frame)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::CLEAR_INPUT;
	command.canvas = canvas;
	command.frame = frame;
	send_command(&command);
}

void Playback3D::clear_input_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::clear_output_sync");
	if(command->canvas->get_canvas())
	{
		command->canvas->get_canvas()->lock_window("Playback3D::clear_output_sync");
		command->canvas->get_canvas()->enable_opengl();
		command->frame->enable_opengl();
		command->frame->clear_pbuffer();
		command->frame->set_opengl_state(VFrame::SCREEN);
		command->canvas->get_canvas()->unlock_window();
	}
	command->canvas->unlock_canvas();
}

void Playback3D::do_camera(Canvas *canvas,
	VFrame *output,
	VFrame *input,
	float in_x1, 
	float in_y1, 
	float in_x2, 
	float in_y2, 
	float out_x1, 
	float out_y1, 
	float out_x2, 
	float out_y2)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::DO_CAMERA;
	command.canvas = canvas;
	command.input = input;
	command.frame = output;
	command.in_x1 = in_x1;
	command.in_y1 = in_y1;
	command.in_x2 = in_x2;
	command.in_y2 = in_y2;
	command.out_x1 = out_x1;
	command.out_y1 = out_y1;
	command.out_x2 = out_x2;
	command.out_y2 = out_y2;
	send_command(&command);
}

void Playback3D::do_camera_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::do_camera_sync");
	if(command->canvas->get_canvas())
	{
		command->canvas->get_canvas()->lock_window("Playback3D::clear_output_sync");
		command->canvas->get_canvas()->enable_opengl();

		command->input->to_texture();
		command->frame->enable_opengl();
		command->frame->init_screen();
		command->frame->clear_pbuffer();

		command->input->bind_texture(0);
// Must call draw_texture in input frame to get the texture coordinates right.

// printf("Playback3D::do_camera_sync %d input=%p frame=%p\n", 
// __LINE__, 
// command->input,
// command->frame);
// printf("Playback3D::do_camera_sync 1 %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", 
// command->in_x1, 
// command->in_y2, 
// command->in_x2, 
// command->in_y1, 
// command->out_x1,
// (float)command->input->get_h() - command->out_y1,
// command->out_x2,
// (float)command->input->get_h() - command->out_y2);
		command->input->draw_texture(
			command->in_x1, 
			command->in_y2, 
			command->in_x2, 
			command->in_y1, 
			command->out_x1,
			(float)command->frame->get_h() - command->out_y1,
			command->out_x2,
			(float)command->frame->get_h() - command->out_y2);


		command->frame->set_opengl_state(VFrame::SCREEN);
		command->canvas->get_canvas()->unlock_window();
	}
	command->canvas->unlock_canvas();
}

void Playback3D::overlay(Canvas *canvas,
	VFrame *input, 
	float in_x1, 
	float in_y1, 
	float in_x2, 
	float in_y2, 
	float out_x1, 
	float out_y1, 
	float out_x2, 
	float out_y2, 
	float alpha,        // 0 - 1
	int mode,
	int interpolation_type,
	VFrame *output,
	int is_nested)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::OVERLAY;
	command.canvas = canvas;
	command.frame = output;
	command.input = input;
	command.in_x1 = in_x1;
	command.in_y1 = in_y1;
	command.in_x2 = in_x2;
	command.in_y2 = in_y2;
	command.out_x1 = out_x1;
	command.out_y1 = out_y1;
	command.out_x2 = out_x2;
	command.out_y2 = out_y2;
	command.alpha = alpha;
	command.mode = mode;
	command.interpolation_type = interpolation_type;
	command.is_nested = is_nested;
    if(command.frame)
    {
        command.video_on = 0;
    }
    else
    {
        command.video_on = 1;
    }
	send_command(&command);
}

void Playback3D::overlay_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	command->canvas->lock_canvas("Playback3D::overlay_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *canvas = command->canvas->get_canvas();
	    canvas->lock_window("Playback3D::overlay_sync");
// Make sure OpenGL context is current.
		canvas->enable_opengl();

		canvas->update_video_cursor();

//printf("Playback3D::overlay_sync %d canvas=%p frame=%p\n", __LINE__, canvas, command->frame);
// Always render to PBuffer with alpha channel
//		if(!command->video_on)
//		{
			command->frame->enable_opengl();
			command->frame->set_opengl_state(VFrame::SCREEN);
			canvas_w = command->frame->get_w();
			canvas_h = command->frame->get_h();
//		}
//		else
// Render to canvas with no alpha channel
//		{
//			canvas_w = canvas->get_w();
//			canvas_h = canvas->get_h();
//		}

		glColor4f(1, 1, 1, 1);
// printf("Playback3D::overlay_sync %d video_on=%d input=%p frame=%p input_state=%d canvas_w=%d canvas_h=%d\n", 
// __LINE__, 
// command->video_on,
// command->input,
// command->frame,
// command->input->get_opengl_state(), 
// canvas_w, 
// canvas_h);

		switch(command->input->get_opengl_state())
		{
// Upload texture and composite to screen
			case VFrame::RAM:
				command->input->to_texture();
				break;
// Just composite texture to screen
			case VFrame::TEXTURE:
				break;
// read from PBuffer to texture, then composite texture to screen
			case VFrame::SCREEN:
				command->input->enable_opengl();
				command->input->screen_to_texture();
				if(!command->video_on)
				{
                	command->frame->enable_opengl();
				}
                else
				{
                	canvas->enable_opengl();
				}
                break;
			default:
				printf("Playback3D::overlay_sync unknown state\n");
				break;
		}


		const char *shader_stack[8] = { 0 };
		int total_shaders = 0;
        int is_yuv = cmodel_is_yuv(command->input->get_color_model());

		VFrame::init_screen(canvas_w, canvas_h);

// Enable texture
		command->input->bind_texture(0);

// Cheat by converting to RGB here if it's for playback.
// This avoids a software YUV -> RGB conversion in VDeviceX11:close_all.
//printf("Playback3D::overlay_sync %d %d %d\n", __LINE__, command->is_nested, command->input->get_color_model());
// 		if(!command->is_nested && is_yuv)
// 		{
// //printf("Playback3D::overlay_sync %d yuv_to_rgb_frag\n", __LINE__);
// 			shader_stack[total_shaders++] = yuv_to_rgb_frag;
//             is_yuv = 0;
// 		}

// Change blend operation
		switch(command->mode)
		{
			case TRANSFER_NORMAL:
// GL_ONE_MINUS_SRC_ALPHA doesn't work 
//				glEnable(GL_BLEND);
//				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//printf("Playback3D::overlay_sync %d TRANSFER_NORMAL\n", __LINE__);
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
                shader_stack[total_shaders++] = get_pixels_frag;
				shader_stack[total_shaders++] = blend_normal_frag;
                shader_stack[total_shaders++] = put_pixels_frag;
				break;

// bottom track is always a replace
			case TRANSFER_REPLACE:
				glDisable(GL_BLEND);
//printf("Playback3D::overlay_sync %d TRANSFER_REPLACE %d\n", __LINE__, command->input->get_texture_components());
        		if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
// 				if(command->input->get_texture_components() == 4)
// 				{
//                     if(command->video_on)
//                     {
// // Screen has no alpha.
//     					shader_stack[total_shaders++] = multiply_alpha_frag;
//                     }
// 				}
				break;

// To do these operations, we need to copy the input buffer to a texture
// and blend 2 textures in another shader
			case TRANSFER_ADDITION:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
                shader_stack[total_shaders++] = get_pixels_frag;
				shader_stack[total_shaders++] = blend_add_frag;
                shader_stack[total_shaders++] = put_pixels_frag;
				break;
			case TRANSFER_SUBTRACT:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
                shader_stack[total_shaders++] = get_pixels_frag;
				shader_stack[total_shaders++] = blend_subtract_frag;
                shader_stack[total_shaders++] = put_pixels_frag;
				break;
			case TRANSFER_MULTIPLY:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
                shader_stack[total_shaders++] = get_pixels_frag;
				shader_stack[total_shaders++] = blend_multiply_frag;
                shader_stack[total_shaders++] = put_pixels_frag;
				break;
			case TRANSFER_MAX:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
                shader_stack[total_shaders++] = get_pixels_frag;
				shader_stack[total_shaders++] = blend_max_frag;
                shader_stack[total_shaders++] = put_pixels_frag;
				break;
			case TRANSFER_MIN:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
                shader_stack[total_shaders++] = get_pixels_frag;
				shader_stack[total_shaders++] = blend_min_frag;
                shader_stack[total_shaders++] = put_pixels_frag;
				break;
			case TRANSFER_DIVIDE:
				enable_overlay_texture(command);
				if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag;
                shader_stack[total_shaders++] = get_pixels_frag;
				shader_stack[total_shaders++] = blend_divide_frag;
                shader_stack[total_shaders++] = put_pixels_frag;
				break;
		}

// create checker pattern
//         if(cmodel_components(command->frame->get_color_model()) == 4)
//         {
//             shader_stack[total_shaders++] = checker_alpha_frag;
//         }

		unsigned int frag_shader = 0;
		if(total_shaders > 0)
		{
			frag_shader = VFrame::make_shader(0,
				shader_stack[0],
				shader_stack[1],
				shader_stack[2],
				shader_stack[3],
				shader_stack[4],
				0);

			glUseProgram(frag_shader);
// printf("Playback3D::overlay_sync %d is_yuv=%d total_shaders=%d\n", 
// __LINE__, 
// is_yuv,
// total_shaders);

		    if(is_yuv)
			    glUniform3f(glGetUniformLocation(frag_shader, "chroma_offset"), 0.0, 0.5, 0.5);
		    else
			    glUniform3f(glGetUniformLocation(frag_shader, "chroma_offset"), 0.0, 0.0, 0.0);

// Set texture unit of the texture
			glUniform1i(glGetUniformLocation(frag_shader, "src_tex"), 0); // tex
// Set texture unit of temp_texture
			glUniform1i(glGetUniformLocation(frag_shader, "dst_tex"), 1); // tex2
// Set dimensions of the temp texture
			if(temp_texture)
				glUniform2f(glGetUniformLocation(frag_shader, "dst_tex_dimensions"), // tex2_dimensions 
					(float)temp_texture->get_texture_w(), 
					(float)temp_texture->get_texture_h());
		}
		else
        {
			glUseProgram(0);
        }









// printf("Playback3D::overlay_sync %d %.0f %.0f %.0f %.0f -> %.0f %.0f %.0f %.0f\n", 
// __LINE__,
// command->in_x1, 
// command->in_y1,
// command->in_x2,
// command->in_y2,
// command->out_x1,
// command->out_y1,
// command->out_x2,
// command->out_y2);

// dest Y has to be flipped
        float out_y1 = command->frame->get_h() - command->out_y1;
        float out_y2 = command->frame->get_h() - command->out_y2;

		command->input->draw_texture(command->in_x1, 
			command->in_y1,
			command->in_x2,
			command->in_y2,
			command->out_x1,
			out_y2,
			command->out_x2,
			out_y1,
// Don't flip vertical if nested or rendering
// In the nested case, the output is possibly flipped 
// upon injestion into the parent EDL.
// In the rendering case, the output is now right side up & 
// we must skip the final flip for the file writer.
			0 /* command->is_nested */);

		glUseProgram(0);


// Delete temp texture
		if(temp_texture)
		{
			delete temp_texture;
			temp_texture = 0;
			glActiveTexture(GL_TEXTURE1);
			glDisable(GL_TEXTURE_2D);
		}
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);



		canvas->unlock_window();
	}
	command->canvas->unlock_canvas();
#endif // HAVE_GL
}

// copy the canvas or pbuffer to a texture before blending
void Playback3D::enable_overlay_texture(Playback3DCommand *command)
{
#ifdef HAVE_GL
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE1);
	BC_Texture::new_texture(&temp_texture,
		canvas_w, 
		canvas_h, 
		command->input->get_color_model());
	temp_texture->bind(1);

// Read canvas into texture
	glReadBuffer(GL_BACK);
	glCopyTexSubImage2D(GL_TEXTURE_2D,
		0,
		0,
		0,
		0,
		0,
		canvas_w,
		canvas_h);
#endif // HAVE_GL
}


void Playback3D::do_mask(Canvas *canvas,
	VFrame *output, 
    VFrame *mask,
	int64_t start_position_project,
	MaskAutos *keyframe_set, 
	MaskAuto *keyframe,
	MaskAuto *default_auto)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::DO_MASK;
	command.canvas = canvas;
	command.frame = output;
    command.mask = mask;
	command.start_position_project = start_position_project;
	command.keyframe_set = keyframe_set;
	command.keyframe = keyframe;
	command.default_auto = default_auto;

	send_command(&command);
}



#ifdef HAVE_GL
static void combine_callback(GLdouble coords[3], 
	GLdouble *vertex_data[4],
	GLfloat weight[4], 
	GLdouble **dataOut)
{
	GLdouble *vertex;

	vertex = (GLdouble *) malloc(6 * sizeof(GLdouble));
	vertex[0] = coords[0];
	vertex[1] = coords[1];
	vertex[2] = coords[2];

	for (int i = 3; i < 6; i++)
	{
		vertex[i] = weight[0] * vertex_data[0][i] +
			weight[1] * vertex_data[1][i] +
			weight[2] * vertex_data[2][i] +
			weight[3] * vertex_data[3][i];
	}

	*dataOut = vertex;
}
#endif


void Playback3D::do_mask_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	command->canvas->lock_canvas("Playback3D::do_mask_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *window = command->canvas->get_canvas();
		window->lock_window("Playback3D::do_mask_sync");
		window->enable_opengl();

		switch(command->frame->get_opengl_state())
		{
			case VFrame::RAM:
// Time to upload to the texture
				command->frame->to_texture();
				break;

			case VFrame::SCREEN:
// Read back from PBuffer
// Bind context to pbuffer
				command->frame->enable_opengl();
				command->frame->screen_to_texture();
				break;
		}



// Create PBuffer and draw the mask on it
		command->frame->enable_opengl();

// Initialize coordinate system
		int w = command->frame->get_w();
		int h = command->frame->get_h();
		command->frame->init_screen();
		int value = command->keyframe_set->get_value(command->start_position_project,
			PLAY_FORWARD);
		float feather = command->keyframe_set->get_feather(command->start_position_project,
			PLAY_FORWARD);

// Clear screen
		glDisable(GL_TEXTURE_2D);
		if(command->default_auto->mode == MASK_MULTIPLY_ALPHA)
		{
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glColor4f((float)value / 100, 
				(float)value / 100, 
				(float)value / 100, 
				1.0);
		}
		else
		{
			glClearColor(1.0, 1.0, 1.0, 1.0);
			glColor4f((float)1.0 - (float)value / 100, 
				(float)1.0 - (float)value / 100, 
				(float)1.0 - (float)value / 100, 
				1.0);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



// tesselate with GLU.  No path support.
#if 0
// Draw mask with scaling to simulate feathering
		GLUtesselator *tesselator = gluNewTess();
		gluTessProperty(tesselator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
		gluTessCallback(tesselator, GLU_TESS_VERTEX, (GLvoid (*) ( )) &glVertex3dv);
		gluTessCallback(tesselator, GLU_TESS_BEGIN, (GLvoid (*) ( )) &glBegin);
		gluTessCallback(tesselator, GLU_TESS_END, (GLvoid (*) ( )) &glEnd);
		gluTessCallback(tesselator, GLU_TESS_COMBINE, (GLvoid (*) ( ))&combine_callback);


// Draw every submask as a new polygon
		int total_submasks = command->keyframe_set->total_submasks(
			command->start_position_project, 
			PLAY_FORWARD);
		float scale = feather + 1;
 		int display_list = glGenLists(1);
 		glNewList(display_list, GL_COMPILE);
		for(int k = 0; k < total_submasks; k++)
		{
			gluTessBeginPolygon(tesselator, NULL);
			gluTessBeginContour(tesselator);
			ArrayList<MaskPoint*> *points = new ArrayList<MaskPoint*>;
			command->keyframe_set->get_points(points, 
				k, 
				command->start_position_project, 
				PLAY_FORWARD);

			int first_point = 0;
// Need to tabulate every vertex in persistent memory because
// gluTessVertex doesn't copy them.
			ArrayList<GLdouble*> coords;
			for(int i = 0; i < points->total; i++)
			{
				MaskPoint *point1 = points->values[i];
				MaskPoint *point2 = (i >= points->total - 1) ? 
					points->values[0] : 
					points->values[i + 1];


// This is very slow.
				float x, y;
				int segments = (int)(sqrt(SQR(point1->x - point2->x) + SQR(point1->y - point2->y)));
				if(point1->control_x2 == 0 &&
					point1->control_y2 == 0 &&
					point2->control_x1 == 0 &&
					point2->control_y1 == 0)
					segments = 1;

				float x0 = point1->x;
				float y0 = point1->y;
				float x1 = point1->x + point1->control_x2;
				float y1 = point1->y + point1->control_y2;
				float x2 = point2->x + point2->control_x1;
				float y2 = point2->y + point2->control_y1;
				float x3 = point2->x;
				float y3 = point2->y;

				for(int j = 0; j <= segments; j++)
				{
					float t = (float)j / segments;
					float tpow2 = t * t;
					float tpow3 = t * t * t;
					float invt = 1 - t;
					float invtpow2 = invt * invt;
					float invtpow3 = invt * invt * invt;

					x = (        invtpow3 * x0
						+ 3 * t     * invtpow2 * x1
						+ 3 * tpow2 * invt     * x2 
						+     tpow3            * x3);
					y = (        invtpow3 * y0 
						+ 3 * t     * invtpow2 * y1
						+ 3 * tpow2 * invt     * y2 
						+     tpow3            * y3);


					if(j > 0 || first_point)
					{
						GLdouble *coord = new GLdouble[3];
						coord[0] = x / scale;
						coord[1] = -h + y / scale;
						coord[2] = 0;
						coords.append(coord);
						first_point = 0;
					}
				}
			}

// Now that we know the total vertices, send them to GLU
			for(int i = 0; i < coords.total; i++)
				gluTessVertex(tesselator, coords.values[i], coords.values[i]);

			gluTessEndContour(tesselator);
			gluTessEndPolygon(tesselator);
			points->remove_all_objects();
			delete points;
			coords.remove_all_objects();
		}
        gluDeleteTess(tesselator);


		glEndList();
 		glCallList(display_list);
 		glDeleteLists(display_list, 1);
#endif // 0

		glColor4f(1, 1, 1, 1);


#if 0
// Read mask into temporary texture.
// For feathering, just read the part of the screen after the downscaling.


		float w_scaled = w / scale;
		float h_scaled = h / scale;
// Don't vary the texture size according to scaling because that 
// would waste memory.
// This enables and binds the temporary texture.
		glActiveTexture(GL_TEXTURE1);
		BC_Texture::new_texture(&temp_texture,
			w, 
			h, 
			command->frame->get_color_model());
		temp_texture->bind(1);
		glReadBuffer(GL_BACK);

// Need to add extra size to fill in the bottom right
		glCopyTexSubImage2D(GL_TEXTURE_2D,
			0,
			0,
			0,
			0,
			0,
			(int)MIN(w_scaled + 2, w),
			(int)MIN(h_scaled + 2, h));
#endif // 0
//printf("Playback3D::do_mask_sync %d opengl_state=%d\n", __LINE__, command->mask->get_opengl_state());

// mask goes into texture unit 1
        if(command->mask->get_opengl_state() == VFrame::RAM)
        {
//printf("Playback3D::do_mask_sync %d to_texture\n", __LINE__);
            command->mask->to_texture();
        }
		command->mask->bind_texture(1);
// dest goes into texture unit 0
		command->frame->bind_texture(0);

//printf("Playback3D::do_mask_sync %d\n", __LINE__);

// For feathered masks, use a shader to multiply.
// For unfeathered masks, we could use a stencil buffer 
// for further optimization but we also need a YUV algorithm.
		unsigned int frag_shader = 0;
//		switch(temp_texture->get_texture_components())
		switch(cmodel_components(command->frame->get_color_model()))
		{
			case 3: 
				if(command->frame->get_color_model() == BC_YUV888)
					frag_shader = VFrame::make_shader(0,
						multiply_yuvmask3_frag,
						0);
				else
					frag_shader = VFrame::make_shader(0,
						multiply_mask3_frag,
						0);
				break;
			case 4: 
				frag_shader = VFrame::make_shader(0,
					multiply_mask4_frag,
					0);
				break;
		}

		if(frag_shader)
		{
			int variable;
			glUseProgram(frag_shader);
			if((variable = glGetUniformLocation(frag_shader, "src_tex")) >= 0)
				glUniform1i(variable, 0);
			if((variable = glGetUniformLocation(frag_shader, "tex1")) >= 0)
				glUniform1i(variable, 1);
//			if((variable = glGetUniformLocation(frag_shader, "scale")) >= 0)
//				glUniform1f(variable, scale);
			if((variable = glGetUniformLocation(frag_shader, "scale")) >= 0)
				glUniform1f(variable, 1.0);
		}
//printf("Playback3D::do_mask_sync %d\n", __LINE__);



// Write texture to PBuffer with multiply and scaling for feather.

		
		command->frame->draw_texture(0, 0, w, h, 0, 0, w, h);
		command->frame->set_opengl_state(VFrame::SCREEN);

//printf("Playback3D::do_mask_sync %d\n", __LINE__);

// Disable temp texture
		glUseProgram(0);

//printf("Playback3D::do_mask_sync %d\n", __LINE__);
 		glActiveTexture(GL_TEXTURE1);
 		glDisable(GL_TEXTURE_2D);
// 		delete temp_texture;
// 		temp_texture = 0;

		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);

// Default drawable
		window->enable_opengl();
		window->unlock_window();
	}
	command->canvas->unlock_canvas();
#endif // HAVE_GL
}







int Playback3D::skip_convert_cmodel(int state,
    int src_cmodel,
    int dst_cmodel)
{
	return (state == VFrame::TEXTURE ||
		    state == VFrame::SCREEN) &&
		((src_cmodel == BC_YUV888 && dst_cmodel == BC_YUV_FLOAT) ||
        (src_cmodel == BC_RGB888 && dst_cmodel == BC_RGB_FLOAT) ||
		(src_cmodel == BC_RGBA8888 && dst_cmodel == BC_RGBA_FLOAT) ||
		(src_cmodel == BC_YUV_FLOAT && dst_cmodel == BC_YUV888) ||
		(src_cmodel == BC_RGB_FLOAT && dst_cmodel == BC_RGB888) ||
		(src_cmodel == BC_RGBA_FLOAT && dst_cmodel == BC_RGBA8888) ||
// OpenGL sets alpha to 1 on import
		(src_cmodel == BC_RGB888 && dst_cmodel == BC_RGBA8888) ||
		(src_cmodel == BC_YUV888 && dst_cmodel == BC_YUVA8888) ||
		(src_cmodel == BC_RGB_FLOAT && dst_cmodel == BC_RGBA_FLOAT));
}


void Playback3D::convert_cmodel(Canvas *canvas, 
	VFrame *output, 
	int dst_cmodel)
{

// Do nothing if colormodels are equivalent & the image is in hardware.
	int src_cmodel = output->get_color_model();
	if(skip_convert_cmodel(output->get_opengl_state(),
        src_cmodel,
        dst_cmodel))
    {
        return;
    }




	Playback3DCommand command;
	command.command = Playback3DCommand::CONVERT_CMODEL;
	command.canvas = canvas;
	command.input = output;
	command.frame = output;
	command.dst_cmodel = dst_cmodel;
	send_command(&command);
}

void Playback3D::convert_cmodel(Canvas *canvas, 
	VFrame *input, 
	VFrame *output)
{

// Do nothing if colormodels are equivalent & the image is in hardware.
	int src_cmodel = input->get_color_model();
	int dst_cmodel = output->get_color_model();
	if(skip_convert_cmodel(output->get_opengl_state(),
        src_cmodel,
        dst_cmodel))
    {
        return;
    }




	Playback3DCommand command;
	command.command = Playback3DCommand::CONVERT_CMODEL;
	command.canvas = canvas;
	command.input = input;
	command.frame = output;
	command.dst_cmodel = output->get_color_model();
	send_command(&command);
}

void Playback3D::convert_cmodel_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	command->canvas->lock_canvas("Playback3D::convert_cmodel_sync");

	if(command->canvas->get_canvas())
	{
		BC_WindowBase *window = command->canvas->get_canvas();
		window->lock_window("Playback3D::convert_cmodel_sync");
		window->enable_opengl();


// printf("Playback3D::convert_cmodel_sync %d debug=%d state=%d\n", 
//     __LINE__, 
//     debug,
//     command->input->get_opengl_state());

// Transfer from input's texture into frame's pbuffer
		command->frame->enable_opengl();
		command->frame->init_screen();
		command->input->to_texture();
		command->input->bind_texture(0);

// Colormodel permutation
		int src_cmodel = command->input->get_color_model();
		int dst_cmodel = command->dst_cmodel;
		typedef struct
		{
			int src;
			int dst;
			const char *shader;
		} cmodel_shader_table_t;
		static cmodel_shader_table_t cmodel_shader_table[]  = 
		{
			{ BC_RGB888, BC_YUV888, rgb_to_yuv_frag },
			{ BC_RGB888, BC_YUVA8888, rgb_to_yuv_frag },
			{ BC_RGBA8888, BC_RGB888, rgba_to_rgb_frag },
			{ BC_RGBA8888, BC_RGB_FLOAT, rgba_to_rgb_frag },
			{ BC_RGBA8888, BC_YUV888, rgba_to_yuv_frag },
			{ BC_RGBA8888, BC_YUVA8888, rgb_to_yuv_frag },
			{ BC_RGB_FLOAT, BC_YUV888, rgb_to_yuv_frag },
			{ BC_RGB_FLOAT, BC_YUVA8888, rgb_to_yuv_frag },
			{ BC_RGBA_FLOAT, BC_RGB888, rgba_to_rgb_frag },
			{ BC_RGBA_FLOAT, BC_RGB_FLOAT, rgba_to_rgb_frag },
			{ BC_RGBA_FLOAT, BC_YUV888, rgba_to_yuv_frag },
			{ BC_RGBA_FLOAT, BC_YUVA8888, rgb_to_yuv_frag },
			{ BC_YUV888, BC_RGB888, yuv_to_rgb_frag },
			{ BC_YUV888, BC_RGBA8888, yuv_to_rgb_frag },
			{ BC_YUV888, BC_RGB_FLOAT, yuv_to_rgb_frag },
			{ BC_YUV888, BC_RGBA_FLOAT, yuv_to_rgb_frag },
			{ BC_YUVA8888, BC_RGB888, yuva_to_rgb_frag },
			{ BC_YUVA8888, BC_RGBA8888, yuv_to_rgb_frag },
			{ BC_YUVA8888, BC_RGB_FLOAT, yuva_to_rgb_frag },
			{ BC_YUVA8888, BC_RGBA_FLOAT, yuv_to_rgb_frag },
			{ BC_YUVA8888, BC_YUV888, yuva_to_yuv_frag },
            { BC_YUV420P10LE, BC_YUV888,     YUV420P10LE_to_yuv_frag },
            { BC_YUV420P10LE, BC_YUVA8888,   YUV420P10LE_to_yuv_frag },
            { BC_YUV420P10LE, BC_RGB888,     YUV420P10LE_to_rgb_frag },
            { BC_YUV420P10LE, BC_RGBA8888,   YUV420P10LE_to_rgb_frag },
            { BC_YUV420P10LE, BC_RGB_FLOAT,  YUV420P10LE_to_rgb_frag },
            { BC_YUV420P10LE, BC_RGBA_FLOAT, YUV420P10LE_to_rgb_frag },
            { BC_YUV420P, BC_YUV888,     YUV420P_to_yuv_frag },
            { BC_YUV420P, BC_YUVA8888,   YUV420P_to_yuv_frag },
            { BC_YUV420P, BC_RGB888,     YUV420P_to_rgb_frag },
            { BC_YUV420P, BC_RGBA8888,   YUV420P_to_rgb_frag },
            { BC_YUV420P, BC_RGB_FLOAT,  YUV420P_to_rgb_frag },
            { BC_YUV420P, BC_RGBA_FLOAT, YUV420P_to_rgb_frag },
            { BC_YUV422P, BC_YUV888,     YUV422P_to_yuv_frag },
            { BC_YUV422P, BC_YUVA8888,   YUV422P_to_yuv_frag },
            { BC_YUV422P, BC_RGB888,     YUV422P_to_rgb_frag },
            { BC_YUV422P, BC_RGBA8888,   YUV422P_to_rgb_frag },
            { BC_YUV422P, BC_RGB_FLOAT,  YUV422P_to_rgb_frag },
            { BC_YUV422P, BC_RGBA_FLOAT, YUV422P_to_rgb_frag },
            { BC_YUV9P, BC_YUV888,     YUV9P_to_yuv_frag },
            { BC_YUV9P, BC_YUVA8888,   YUV9P_to_yuv_frag },
            { BC_YUV9P, BC_RGB888,     YUV9P_to_rgb_frag },
            { BC_YUV9P, BC_RGBA8888,   YUV9P_to_rgb_frag },
            { BC_YUV9P, BC_RGB_FLOAT,  YUV9P_to_rgb_frag },
            { BC_YUV9P, BC_RGBA_FLOAT, YUV9P_to_rgb_frag },
		};

	    const char *shader_stack[] = { 0, 0, 0 };
        shader_stack[0] = cmodel_head;
		for(int i = 0; 
            i < sizeof(cmodel_shader_table) / sizeof(cmodel_shader_table_t); 
            i++)
		{
			if(cmodel_shader_table[i].src == src_cmodel &&
				cmodel_shader_table[i].dst == dst_cmodel)
			{
				shader_stack[1] = cmodel_shader_table[i].shader;
				break;
			}
		}

// printf("Playback3D::convert_cmodel_sync %d rowspan=%ld\n", 
// __LINE__, 
// command->input->get_bytes_per_line());

		if(shader_stack[1])
		{
	        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            
            
			unsigned int shader_id = -1;
			shader_id = VFrame::make_shader(0,
				shader_stack[0],
				shader_stack[1],
				0);
			glUseProgram(shader_id);
			glUniform1i(glGetUniformLocation(shader_id, "src_tex"), 0);
	        float image_h = command->frame->get_h();
	        float image_w = command->frame->get_w();
	        float texture_h = command->input->get_texture_h();
	        float texture_w = command->input->get_texture_w();
	        float pixel_h = 1.0 / command->input->get_texture_h();
	        float pixel_w = 1.0 / command->input->get_texture_w();
            float u_offset = (float)(command->input->get_u() - command->input->get_y());
            float v_offset = (float)(command->input->get_v() - command->input->get_y());
// printf("Playback3D::convert_cmodel_sync %d rowspan=%d\n",
// __LINE__,
// command->input->get_bytes_per_line());

			glUniform1f(glGetUniformLocation(shader_id, "rowspan"), command->input->get_bytes_per_line());
			glUniform1f(glGetUniformLocation(shader_id, "pixel_w"), pixel_w);
			glUniform1f(glGetUniformLocation(shader_id, "pixel_h"), pixel_h);
			glUniform1f(glGetUniformLocation(shader_id, "half_w"), pixel_w / 2);
			glUniform1f(glGetUniformLocation(shader_id, "half_h"), pixel_h / 2);
			glUniform1f(glGetUniformLocation(shader_id, "texture_w"), texture_w);
			glUniform1f(glGetUniformLocation(shader_id, "texture_h"), texture_h);
			glUniform1f(glGetUniformLocation(shader_id, "image_w"), image_w);
			glUniform1f(glGetUniformLocation(shader_id, "image_h"), image_h);
			glUniform1f(glGetUniformLocation(shader_id, "u_offset"), u_offset);
			glUniform1f(glGetUniformLocation(shader_id, "v_offset"), v_offset);


			command->input->draw_texture();

			glUseProgram(0);

	        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			command->frame->set_opengl_state(VFrame::SCREEN);
		}
        else
        {
            printf("Playback3D::convert_cmodel_sync %d: unsupported conversion %d->%d\n",
                __LINE__,
                src_cmodel,
                dst_cmodel);
        }

		window->unlock_window();
	}

	command->canvas->unlock_canvas();
#endif // HAVE_GL
}

void Playback3D::do_fade(Canvas *canvas, VFrame *frame, float fade)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::DO_FADE;
	command.canvas = canvas;
	command.frame = frame;
	command.alpha = fade;
	send_command(&command);
}

void Playback3D::do_fade_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	command->canvas->lock_canvas("Playback3D::do_fade_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *window = command->canvas->get_canvas();
		window->lock_window("Playback3D::do_fade_sync");
		window->enable_opengl();

		switch(command->frame->get_opengl_state())
		{
			case VFrame::RAM:
				command->frame->to_texture();
				break;

			case VFrame::SCREEN:
// Read back from PBuffer
// Bind context to pbuffer
				command->frame->enable_opengl();
				command->frame->screen_to_texture();
				break;
		}


		command->frame->enable_opengl();
		command->frame->init_screen();
		command->frame->bind_texture(0);

//		glClearColor(0.0, 0.0, 0.0, 0.0);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		unsigned int frag_shader = 0;
		switch(command->frame->get_color_model())
		{
// For the alpha colormodels, the native function seems to multiply the 
// components by the alpha instead of just the alpha.
			case BC_RGBA8888:
			case BC_RGBA_FLOAT:
			case BC_YUVA8888:
//printf("Playback3D::do_fade_sync %d frame=%p alpha=%f\n", 
//__LINE__, command->frame, command->alpha);
				frag_shader = VFrame::make_shader(0,
					fade_rgba_frag,
					0);
				break;

			case BC_RGB888:
			case BC_RGB_FLOAT:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
				glColor4f(command->alpha, command->alpha, command->alpha, 1);
				break;


			case BC_YUV888:
				frag_shader = VFrame::make_shader(0,
					fade_yuv_frag,
					0);
				break;
            
            default:
                printf("Playback3D::do_fade_sync %d: unsupported colormodel %d\n",
                    __LINE__,
                    command->frame->get_color_model());
                break;
		}


		if(frag_shader)
		{
			glUseProgram(frag_shader);
			int variable;
			if((variable = glGetUniformLocation(frag_shader, "src_tex")) >= 0)
				glUniform1i(variable, 0);
			if((variable = glGetUniformLocation(frag_shader, "alpha")) >= 0)
				glUniform1f(variable, command->alpha);
		}

		command->frame->draw_texture();
		command->frame->set_opengl_state(VFrame::SCREEN);

		if(frag_shader)
		{
			glUseProgram(0);
		}

		glColor4f(1, 1, 1, 1);
		glDisable(GL_BLEND);

		window->unlock_window();
	}
	command->canvas->unlock_canvas();
#endif
}











int Playback3D::run_plugin(Canvas *canvas, PluginClient *client)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::PLUGIN;
	command.canvas = canvas;
	command.plugin_client = client;
	return send_command(&command);
}

void Playback3D::run_plugin_sync(Playback3DCommand *command)
{
	command->canvas->lock_canvas("Playback3D::run_plugin_sync");
	if(command->canvas->get_canvas())
	{
		BC_WindowBase *window = command->canvas->get_canvas();
		window->lock_window("Playback3D::run_plugin_sync");
		window->enable_opengl();

		command->result = ((PluginVClient*)command->plugin_client)->handle_opengl();

		window->unlock_window();
	}
	command->canvas->unlock_canvas();
}


