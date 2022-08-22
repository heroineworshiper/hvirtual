/*
 * CINELERRA
 * Copyright (C) 2011-2022 Adam Williams <broadcast at earthling dot net>
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

#include "bcpbuffer.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bctexture.h"
#include "bcwindowbase.h"
#include "vframe.h"

#ifdef HAVE_GL
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int VFrame::get_opengl_state()
{
	return opengl_state;
}

void VFrame::set_opengl_state(int value)
{
	opengl_state = value;
}

int VFrame::get_window_id()
{
	return texture ? texture->window_id : -1;
}

int VFrame::get_texture_id()
{
	return texture ? texture->texture_id : -1;
}

int VFrame::get_texture_w()
{
	return texture ? texture->texture_w : 0;
}

int VFrame::get_texture_h()
{
	return texture ? texture->texture_h : 0;
}


int VFrame::get_texture_components()
{
	return texture ? texture->texture_components : 0;
}











void VFrame::to_texture()
{
#ifdef HAVE_GL

// Must be here so user can create textures without copying data by setting
// opengl_state to TEXTURE.
	BC_Texture::new_texture(&texture,
		get_w(),
		get_h(),
		get_color_model());

// Determine what to do based on state
	switch(opengl_state)
	{
		case VFrame::TEXTURE:
			return;

		case VFrame::SCREEN:
// 			if((get_w() % 4) || (get_h() % 4)) 
// 			{
// 				printf("VFrame::to_texture w=%d h=%d\n", get_w(), get_h());
// 				return;
// 			}
			if(pbuffer)
			{
				enable_opengl();
				screen_to_texture();
			}
			opengl_state = VFrame::TEXTURE;
			return;
	}

// BC_Signals::dump_stack();

	switch(color_model)
	{
		case BC_RGB888:
		case BC_YUV888:
            if((get_w() % 4))
            {
                for(int i = 0; i < get_h(); i++)
                {
                    glTexSubImage2D(GL_TEXTURE_2D,
				        0,
				        0,
				        i,
				        get_w(),
				        1,
				        GL_RGB,
				        GL_UNSIGNED_BYTE,
				        get_rows()[i]);
                }
            }
            else
            {
			    glTexSubImage2D(GL_TEXTURE_2D,
				    0,
				    0,
				    0,
				    get_w(),
				    get_h(),
				    GL_RGB,
				    GL_UNSIGNED_BYTE,
				    get_rows()[0]);
            }
			break;

		case BC_RGBA8888:
		case BC_YUVA8888:
			glTexSubImage2D(GL_TEXTURE_2D,
				0,
				0,
				0,
				get_w(),
				get_h(),
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				get_rows()[0]);
			break;

		case BC_RGB_FLOAT:
		case BC_YUV_FLOAT:
			glTexSubImage2D(GL_TEXTURE_2D,
				0,
				0,
				0,
				get_w(),
				get_h(),
				GL_RGB,
				GL_FLOAT,
				get_rows()[0]);
			break;

		case BC_RGBA_FLOAT:
			glTexSubImage2D(GL_TEXTURE_2D,
				0,
				0,
				0,
				get_w(),
				get_h(),
				GL_RGBA,
				GL_FLOAT,
				get_rows()[0]);
			break;

		case BC_A8:
//printf("VFrame::to_texture %d\n", __LINE__);
            if((get_w() % 4))
            {
                for(int i = 0; i < get_h(); i++)
                {
			        glTexSubImage2D(GL_TEXTURE_2D,
				        0,
				        0,
				        i,
				        get_w(),
				        1,
				        GL_RED,
				        GL_UNSIGNED_BYTE,
				        get_rows()[i]);
                }
            }
            else
            {
			    glTexSubImage2D(GL_TEXTURE_2D,
				    0,
				    0,
				    0,
				    get_w(),
				    get_h(),
				    GL_RED,
				    GL_UNSIGNED_BYTE,
				    get_rows()[0]);
            }
//printf("VFrame::to_texture %d\n", __LINE__);
            break;

		case BC_A_FLOAT:
			glTexSubImage2D(GL_TEXTURE_2D,
				0,
				0,
				0,
				get_w(),
				get_h(),
				GL_RED,
				GL_FLOAT,
				get_rows()[0]);
			break;

// inputs for colorspace conversion are just sent as 8 bit RGBA rounded
// up a row
        case BC_YUV420P:
        {
            int bytes = get_data_size();
// rows required to transfer planer data to contiguous texture memory
            int rows = bytes / texture->get_texture_w() / 4;
            if(rows * w * 4 < bytes) rows++;
			glTexSubImage2D(GL_TEXTURE_2D,
				0,
				0,
				0,
				texture->get_texture_w(),
				rows,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				get_data());
            break;
        }

//         case BC_YUV422P:
//             break;
// 
//         case BC_NV12:
//             break;

        case BC_YUV420P10LE:
        {
            int bytes = get_data_size();
// rows required to transfer planer data to contiguous texture memory
            int rows = bytes / texture->get_texture_w() / 4;
            if(rows * w * 4 < bytes) rows++;
			glTexSubImage2D(GL_TEXTURE_2D,
				0,
				0,
				0,
				texture->get_texture_w(),
				rows,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				get_data());
            break;
        }

		default:
			fprintf(stderr, 
				"VFrame::to_texture: unsupported color model %d.\n", 
				color_model);
			break;
	}

	opengl_state = VFrame::TEXTURE;
#endif
}

void VFrame::to_ram()
{
#ifdef HAVE_GL
printf("VFrame::to_ram %d %d %d\n", __LINE__, get_w(), get_h());
	switch(opengl_state)
	{
// Only pbuffer is supported since this is only called after the 
// overlay operation onto the pbuffer.
		case VFrame::SCREEN:
			if(pbuffer)
			{
// TODO: support for odd dimensions
				enable_opengl();
				glReadPixels(0, 
					0, 
					get_w(), 
					get_h(), 
					GL_RGB,
					GL_UNSIGNED_BYTE,
					get_rows()[0]);
				flip_vert();
			}
			opengl_state = VFrame::RAM;
			return;
	}
#endif
}

void VFrame::create_pbuffer()
{
#ifdef HAVE_GL
	if(pbuffer && 
		pbuffer->window_id != BC_WindowBase::get_synchronous()->current_window->get_id())
	{
		delete pbuffer;
		pbuffer = 0;
	}

    int fixed_w = get_w();
    int fixed_h = get_h();
    
    if((fixed_w % 4))
    {
        fixed_w = fixed_w + (4 - (fixed_w % 4));
    }
    if((fixed_h % 4))
    {
        fixed_h = fixed_h + (4 - (fixed_h % 4));
    }

// 	if((get_w() % 4) || (get_h() % 4))
// 	{
// 		printf("VFrame::create_pbuffer w=%d h=%d\n", get_w(), get_h());
// 		return;
// 	}

	if(!pbuffer)
	{
		pbuffer = new BC_PBuffer(fixed_w, fixed_h);
	}
#endif // HAVE_GL
}

void VFrame::enable_opengl()
{
	create_pbuffer();
	if(pbuffer)
	{
		pbuffer->enable_opengl();
	}
}

BC_PBuffer* VFrame::get_pbuffer()
{
	return pbuffer;
}


void VFrame::screen_to_texture(int x, int y, int w, int h)
{
#ifdef HAVE_GL
// Create texture
	BC_Texture::new_texture(&texture,
		get_w(),
		get_h(),
		get_color_model());

	if(pbuffer)
	{
		glEnable(GL_TEXTURE_2D);

// Read canvas into texture
// According to the man page, it must be GL_BACK for the onscreen buffer 
// and GL_FRONT for a single buffered PBuffer.  In reality it must be
// GL_BACK for a single buffered PBuffer if the PBuffer has alpha and using
// GL_FRONT captures the onscreen front buffer.
//		glReadBuffer(BC_WindowBase::get_synchronous()->is_pbuffer ? 
//			GL_FRONT : GL_BACK);
		glReadBuffer(GL_BACK);
		glCopyTexSubImage2D(GL_TEXTURE_2D,
			0,
			0,
			0,
			x >= 0 ? x : 0,
			y >= 0 ? y : 0,
			w >= 0 ? w : get_w(),
			h >= 0 ? h : get_h());
	}
#endif // HAVE_GL
}

void VFrame::draw_texture(float in_x1, 
		float in_y1,
		float in_x2,
		float in_y2,
		float out_x1,
		float out_y1,
		float out_x2,
		float out_y2,
		int flip_y)
{
#ifdef HAVE_GL


	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1.0);

	glTexCoord2f(in_x1 / get_texture_w(), in_y1 / get_texture_h());
	glVertex3f(out_x1, flip_y ? -out_y1 : -out_y2, 0);

	glTexCoord2f(in_x2 / get_texture_w(), in_y1 / get_texture_h());
	glVertex3f(out_x2, flip_y ? -out_y1 : -out_y2, 0);

	glTexCoord2f(in_x2 / get_texture_w(), in_y2 / get_texture_h());
	glVertex3f(out_x2, flip_y ? -out_y2 : -out_y1, 0);

	glTexCoord2f(in_x1 / get_texture_w(), in_y2 / get_texture_h());
	glVertex3f(out_x1, flip_y ? -out_y2 : -out_y1, 0);


	glEnd();

#endif
}

void VFrame::draw_texture(int flip_y)
{
	draw_texture(0, 
		0,
		get_w(),
		get_h(),
		0,
		0,
		get_w(),
		get_h(),
		flip_y);
}


void VFrame::bind_texture(int texture_unit)
{
// Bind the texture
	if(texture)
	{
		texture->bind(texture_unit);
	}
}






void VFrame::init_screen(int w, int h)
{
#ifdef HAVE_GL
    int gl_w = w;
    int gl_h = h;
//     if(gl_w % 4)
//     {
//         gl_w += 4 - (gl_w % 4);
//     }
//     if(gl_h % 4)
//     {
//         gl_h += 4 - (gl_h % 4);
//     }
//printf("VFrame::init_screen %d w=%d h=%d gl_w=%d gl_h=%d\n", 
//__LINE__, w, h, gl_w, gl_h);

	glViewport(0, 0, gl_w, gl_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float near = 1;
	float far = 100;
	float frustum_ratio = near / ((near + far) / 2);
 	float near_h = (float)gl_h * 
		frustum_ratio;
	float near_w = (float)gl_w * 
		frustum_ratio;
	glFrustum(-near_w / 2, 
		near_w / 2, 
		-near_h / 2, 
		near_h / 2, 
		near, 
		far);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
// Shift down and right so 0,0 is the top left corner
	glTranslatef(-(float)gl_w / 2, (float)gl_h / 2, 0.0);
	glTranslatef(0.0, 0.0, -(far + near) / 2);

	glDisable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
// Default for direct copy playback
	glDisable(GL_BLEND);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);
	glAlphaFunc(GL_GREATER, 0);
	glDisable(GL_LIGHTING);

	const GLfloat one[] = { 1, 1, 1, 1 };
	const GLfloat zero[] = { 0, 0, 0, 0 };
	const GLfloat light_position[] = { 0, 0, -1, 0 };
	const GLfloat light_direction[] = { 0, 0, 1, 0 };

// 	glEnable(GL_LIGHT0);
// 	glLightfv(GL_LIGHT0, GL_AMBIENT, zero);
// 	glLightfv(GL_LIGHT0, GL_DIFFUSE, one);
// 	glLightfv(GL_LIGHT0, GL_SPECULAR, one);
// 	glLighti(GL_LIGHT0, GL_SPOT_CUTOFF, 180);
// 	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
// 	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
// 	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
// 	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, zero);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0);
#endif
}

void VFrame::init_screen()
{
	init_screen(get_w(), get_h());
}

static int print_error(char *source, unsigned int object, int is_program)
{
#ifdef HAVE_GL
    char string[BCTEXTLEN];
	int len = 0;
    if(is_program)
		glGetProgramInfoLog(object, BCTEXTLEN, &len, string);
	else
		glGetShaderInfoLog(object, BCTEXTLEN, &len, string);
	if(len > 0) printf("Playback3D::print_error:\n%s\n%s\n", source, string);
	if(len > 0) return 1;
#endif
	return 0;
}





unsigned int VFrame::make_shader(int dump, ...)
{
	unsigned int result = 0;
#ifdef HAVE_GL
// Construct single source file out of arguments
	char *complete_program = 0;
	int complete_size = 0;
	int current_shader = 0;

	va_list list;
	va_start(list, dump);

	while(1)
	{
		char *text = va_arg(list, char*);
		if(!text) break;

// Replace one occurrance in each source of main() with a unique id.
		char main_replacement[BCTEXTLEN];
		sprintf(main_replacement, "main%03d()", current_shader);
//printf("VFrame::make_shader %s %s\n", text, main_replacement);
		char *source_replacement = new char[strlen(text) + strlen(main_replacement) + 1];
		char *ptr = strstr(text, "main()");

		if(ptr)
		{
			memcpy(source_replacement, text, ptr - text);
			source_replacement[ptr - text] = 0;
			strcat(source_replacement, main_replacement);
			ptr += strlen("main()");
			strcat(source_replacement, ptr);
			current_shader++;
		}
		else
		{
			memcpy(source_replacement, text, strlen(text));
			source_replacement[strlen(text)] = 0;
		}

		if(!complete_program)
		{
			complete_size = strlen(source_replacement) + 1;
			complete_program = (char*)malloc(complete_size);
			strcpy(complete_program, source_replacement);
		}
		else
		{
			complete_size += strlen(source_replacement);
			complete_program = (char*)realloc(complete_program, complete_size);
			strcat(complete_program, source_replacement);
		}

		delete [] source_replacement;
	}
	va_end(list);

// Add main() function which calls all the unique main replacements in order
	string main_function;
	main_function.assign(
		"\n"
		"void main()\n"
		"{\n");

	for(int i = 0; i < current_shader; i++)
	{
		char main_replacement[BCTEXTLEN];
		sprintf(main_replacement, "\tmain%03d();\n", i);
		main_function.append(main_replacement);
	}

	main_function.append("}\n");
	if(!complete_program)
	{
		complete_size = main_function.length() + 1;
		complete_program = (char*)malloc(complete_size);
		strcpy(complete_program, main_function.c_str());
	}
	else
	{
		complete_size += main_function.length() + 1;
		complete_program = (char*)realloc(complete_program, complete_size);
		strcat(complete_program, main_function.c_str());
	}


    if(dump)
    {
        printf("VFrame::make_shader %d\n", __LINE__);
        int line = 1;
        int line_start = 1;
        for(int i = 0; i < complete_size; i++)
        {
            if(line_start)
            {
                char string[BCTEXTLEN];
                sprintf(string, "%d", line);
                printf("%s", string);
                for(int j = 0; j < 6 - strlen(string); j++)
                    printf(" ");
                line_start = 0;
            }
            printf("%c", complete_program[i]);
            if(complete_program[i] == '\n')
            {
                line++;
                line_start = 1;
            }
        }
        printf("\n");
    }


	int got_it = 0;
	result = BC_WindowBase::get_synchronous()->get_shader(complete_program, 
		&got_it);

	if(!got_it)
	{
		result = glCreateProgram();

		unsigned int shader;
		shader = glCreateShader(GL_FRAGMENT_SHADER);
		const GLchar *text_ptr = complete_program;
		glShaderSource(shader, 1, &text_ptr, NULL);
		glCompileShader(shader);
		int error = print_error(complete_program, shader, 0);
		glAttachShader(result, shader);
		glDeleteShader(shader);

		glLinkProgram(result);
		if(!error) error = print_error(complete_program, result, 1);


// printf("BC_WindowBase::make_shader: shader=%d window_id=%d\n", 
// result,
// BC_WindowBase::get_synchronous()->current_window->get_id());
		BC_WindowBase::get_synchronous()->put_shader(result, complete_program);
	}

//printf("VFrame::make_shader\n%s\n", complete_program);
	free(complete_program);

#endif
	return result;
}

void VFrame::dump_shader(int shader_id)
{
	BC_WindowBase::get_synchronous()->dump_shader(shader_id);
}


void VFrame::clear_pbuffer()
{
#ifdef HAVE_GL
	if(cmodel_is_yuv(get_color_model()))
		glClearColor(0.0, 0.5, 0.5, 0.0);
	else
		glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

