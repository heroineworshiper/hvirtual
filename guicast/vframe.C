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

#include <errno.h>
#include <png.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/shm.h>

#include "bchash.h"
#include "bcpbuffer.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bctexture.h"
#include "bcwindowbase.h"
#include "clip.h"
//#include "bccmodels.h"
#include "vframe.h"

class PngReadFunction
{
public:
	static void png_read_function(png_structp png_ptr,
            	   png_bytep data, 
				   png_size_t length)
	{
		VFrame *frame = (VFrame*)png_get_io_ptr(png_ptr);
		if(frame->image_size - frame->image_offset < length)
		{
			printf("PngReadFunction::png_read_function %d: overrun\n", __LINE__);
			length = frame->image_size - frame->image_offset;
		}

		memcpy(data, &frame->image[frame->image_offset], length);
		frame->image_offset += length;
	};
};







VFrameScene::VFrameScene()
{
}

VFrameScene::~VFrameScene()
{
}







//static BCCounter counter;


VFrame::VFrame(const unsigned char *png_data)
{
	reset_parameters(1);
	params = new BC_Hash;
	read_png(png_data);
}

VFrame::VFrame(VFrame &frame)
{
	reset_parameters(1);
	params = new BC_Hash;
	allocate_data(0, 
		-1,
		0, 
		0, 
		0, 
		frame.w, 
		frame.h, 
		frame.color_model, 
		frame.bytes_per_line);
	memcpy(data, frame.data, bytes_per_line * h);
	copy_stacks(&frame);
}

VFrame::VFrame(int w, 
	int h, 
	int color_model)
{
	reset_parameters(1);
	params = new BC_Hash;
	allocate_data(data, 
		-1,
		0, 
		0, 
		0, 
		w, 
		h, 
		color_model, 
		-1);
}

VFrame::VFrame(unsigned char *data, 
	int shmid,
	int w, 
	int h, 
	int color_model, 
	long bytes_per_line)
{
	reset_parameters(1);
	params = new BC_Hash;
	allocate_data(data, 
		shmid,
		0, 
		0, 
		0, 
		w, 
		h, 
		color_model, 
		bytes_per_line);
}

VFrame::VFrame(unsigned char *data, 
		int shmid,
		long y_offset,
		long u_offset,
		long v_offset, 
		int w, 
		int h, 
		int color_model, 
		long bytes_per_line)
{
	reset_parameters(1);
	params = new BC_Hash;
	allocate_data(data, 
		shmid,
		y_offset, 
		u_offset, 
		v_offset, 
		w, 
		h, 
		color_model, 
		bytes_per_line);
}

VFrame::VFrame()
{
	reset_parameters(1);
	params = new BC_Hash;
	this->color_model = BC_COMPRESSED;
}











VFrame::~VFrame()
{
	clear_objects(1);
// Delete effect stack
	prev_effects.remove_all_objects();
	next_effects.remove_all_objects();
	delete params;
	delete scene;
}

int VFrame::equivalent(VFrame *src, int test_stacks)
{
	return (src->get_color_model() == get_color_model() &&
		src->get_w() == get_w() &&
		src->get_h() == get_h() &&
		src->bytes_per_line == bytes_per_line &&
		(!test_stacks || equal_stacks(src)));
}

int VFrame::data_matches(VFrame *frame)
{
	if(data && frame->get_data() &&
		frame->params_match(get_w(), get_h(), get_color_model()) &&
		get_data_size() == frame->get_data_size())
	{
		int data_size = get_data_size();
		unsigned char *ptr1 = get_data();
		unsigned char *ptr2 = frame->get_data();
		for(int i = 0; i < data_size; i++)
		{
			if(*ptr1++ != *ptr2++) return 0;
		}
		return 1;
	}
	return 0;
}

// long VFrame::set_shm_offset(long offset)
// {
// 	shm_offset = offset;
// 	return 0;
// }
// 
// long VFrame::get_shm_offset()
// {
// 	return shm_offset;
// }
// 
int VFrame::get_memory_type()
{
	return memory_type;
}

int VFrame::params_match(int w, int h, int color_model)
{
	return (this->w == w &&
		this->h == h &&
		this->color_model == color_model);
}


int VFrame::reset_parameters(int do_opengl)
{
	scene = 0;
	field2_offset = -1;
	memory_type = VFrame::PRIVATE;
//	shm_offset = 0;
	shmid = -1;
	use_shm = 1;
	bytes_per_line = 0;
	data = 0;
	rows = 0;
	color_model = 0;
	compressed_allocated = 0;
	compressed_size = 0;   // Size of current image
	w = 0;
	h = 0;
	y = u = v = 0;
	y_offset = 0;
	u_offset = 0;
	v_offset = 0;
	sequence_number = -1;
	is_keyframe = 0;

	if(do_opengl)
	{
// By default, anything is going to be done in RAM
		opengl_state = VFrame::RAM;
		pbuffer = 0;
		texture = 0;
	}

	prev_effects.set_array_delete();
	next_effects.set_array_delete();
	return 0;
}

int VFrame::clear_objects(int do_opengl)
{
// Remove texture
	if(do_opengl)
	{
		delete texture;
		texture = 0;

		delete pbuffer;
		pbuffer = 0;
	}

// Delete data
	switch(memory_type)
	{
		case VFrame::PRIVATE:
// Memory check
// if(this->w * this->h > 1500 * 1100)
// printf("VFrame::clear_objects 2 this=%p data=%p\n", this, data);   
			if(data)
			{
//printf("VFrame::clear_objects %d this=%p shmid=%p data=%p\n", __LINE__, this, shmid, data);   
				if(shmid >= 0) 
					shmdt(data);
				else
					free(data);
//PRINT_TRACE
			}

			data = 0;
			shmid = -1;
			break;
		
		case VFrame::SHMGET:
			if(data) shmdt(data);
			data = 0;
			shmid = -1;
			break;
	}

// Delete row pointers
	switch(color_model)
	{
		case BC_COMPRESSED:
		case BC_YUV420P:
			break;

		default:
			delete [] rows;
			rows = 0;
			break;
	}


	return 0;
}

int VFrame::get_field2_offset()
{
	return field2_offset;
}

int VFrame::set_field2_offset(int value)
{
	this->field2_offset = value;
	return 0;
}

void VFrame::set_keyframe(int value)
{
	this->is_keyframe = value;
}

int VFrame::get_keyframe()
{
	return is_keyframe;
}


VFrameScene* VFrame::get_scene()
{
	return scene;
}

int VFrame::calculate_bytes_per_pixel(int color_model)
{
	return cmodel_calculate_pixelsize(color_model);
}

long VFrame::get_bytes_per_line()
{
	return bytes_per_line;
}

long VFrame::get_data_size()
{
	return calculate_data_size(w, h, bytes_per_line, color_model) - 4;
//	return h * bytes_per_line;
}

long VFrame::calculate_data_size(int w, int h, int bytes_per_line, int color_model)
{
	return cmodel_calculate_datasize(w, h, bytes_per_line, color_model);
	return 0;
}

void VFrame::create_row_pointers()
{
	switch(color_model)
	{
		case BC_YUV420P:
		case BC_YUV411P:
			if(!this->v_offset)
			{
                int pad = 0;
// ffmpeg expects 1 more row for odd numbered heights
                if((h % 2) > 0)
                    pad = w / 2;
				this->y_offset = 0;
				this->u_offset = w * h;
				this->v_offset = w * h + (w / 2) * (h / 2) + pad;
			}
            
            if(this->data)
            {
			    y = this->data + this->y_offset;
			    u = this->data + this->u_offset;
			    v = this->data + this->v_offset;
            }
			break;

		case BC_YUV422P:
			if(!this->v_offset)
			{
				this->y_offset = 0;
				this->u_offset = w * h;
				this->v_offset = w * h + (w / 2) * h;
			}
            if(this->data)
            {
			    y = this->data + this->y_offset;
			    u = this->data + this->u_offset;
			    v = this->data + this->v_offset;
            }
			break;

		default:
            if(this->data)
            {
			    rows = new unsigned char*[h];
			    for(int i = 0; i < h; i++)
			    {
				    rows[i] = &this->data[i * this->bytes_per_line];
			    }
            }
			break;
	}
}

int VFrame::allocate_data(unsigned char *data, 
	int shmid,
	long y_offset,
	long u_offset,
	long v_offset,
	int w, 
	int h, 
	int color_model, 
	long bytes_per_line)
{
	this->w = w;
	this->h = h;
	this->color_model = color_model;
	this->bytes_per_pixel = calculate_bytes_per_pixel(color_model);
	this->y_offset = this->u_offset = this->v_offset = 0;
	if(shmid == 0)
	{
//		printf("VFrame::allocate_data %d shmid == 0\n", __LINE__, shmid);
	}

	if(bytes_per_line >= 0)
	{
		this->bytes_per_line = bytes_per_line;
	}
	else
	{
    	this->bytes_per_line = this->bytes_per_pixel * w;
    }

// Allocate data + padding for MMX
	if(data)
	{
//printf("VFrame::allocate_data %d %p\n", __LINE__, this->data);
		memory_type = VFrame::SHARED;
		this->data = data;
		this->shmid = -1;
		this->y_offset = y_offset;
		this->u_offset = u_offset;
		this->v_offset = v_offset;
	}
	else
	if(shmid >= 0)
	{
		memory_type = VFrame::SHMGET;
		this->data = (unsigned char*)shmat(shmid, NULL, 0);
//printf("VFrame::allocate_data %d shmid=%d data=%p\n", __LINE__, shmid, this->data);
		this->shmid = shmid;
		this->y_offset = y_offset;
		this->u_offset = u_offset;
		this->v_offset = v_offset;
	}
	else
	{
		memory_type = VFrame::PRIVATE;
		int size = calculate_data_size(this->w, 
			this->h, 
			this->bytes_per_line, 
			this->color_model);
		if(BC_WindowBase::get_resources()->vframe_shm && use_shm)
		{
			this->shmid = shmget(IPC_PRIVATE, 
				size, 
				IPC_CREAT | 0777);
			if(this->shmid < 0)
			{
				printf("VFrame::allocate_data %d could not allocate shared memory\n", __LINE__);
			}

			this->data = (unsigned char*)shmat(this->shmid, NULL, 0);
//if(size > 0x100000) printf("VFrame::allocate_data %d size=%d shmid=%d data=%p\n", __LINE__, size, this->shmid, this->data);

//printf("VFrame::allocate_data %d %p\n", __LINE__, this->data);
// This causes it to automatically delete when the program exits.
			shmctl(this->shmid, IPC_RMID, 0);
		}
		else
		{
// Have to use malloc for libpng
			this->data = (unsigned char*)malloc(size);
		}

// Memory check
// if(this->w * this->h > 1500 * 1100)
// printf("VFrame::allocate_data 2 this=%p w=%d h=%d this->data=%p\n", 
// this, this->w, this->h, this->data);   

		if(!this->data)
			printf("VFrame::allocate_data %dx%d: memory exhausted.\n", this->w, this->h);

//printf("VFrame::allocate_data %d %p data=%p %d %d\n", __LINE__, this, this->data, this->w, this->h);
//if(size > 1000000) printf("VFrame::allocate_data %d\n", size);
	}

// Create row pointers
	create_row_pointers();
	return 0;
}



void VFrame::set_memory(unsigned char *data, 
	int shmid,
	long y_offset,
	long u_offset,
	long v_offset)
{
	clear_objects(0);

	if(data)
	{
		memory_type = VFrame::SHARED;
		this->data = data;
		this->shmid = -1;
		this->y_offset = y_offset;
		this->u_offset = u_offset;
		this->v_offset = v_offset;
	}
	else
	if(shmid >= 0)
	{
		memory_type = VFrame::SHMGET;
		this->data = (unsigned char*)shmat(shmid, NULL, 0);
		this->shmid = shmid;
	}
	
    if(this->data)
    {
	    y = this->data + this->y_offset;
	    u = this->data + this->u_offset;
	    v = this->data + this->v_offset;

	    create_row_pointers();
    }
}

void VFrame::set_compressed_memory(unsigned char *data,
	int shmid,
	int data_size,
	int data_allocated)
{
	clear_objects(0);

	if(data)
	{
		memory_type = VFrame::SHARED;
		this->data = data;
		this->shmid = -1;
	}
	else
	if(shmid >= 0)
	{
		memory_type = VFrame::SHMGET;
		this->data = (unsigned char*)shmat(shmid, NULL, 0);
		this->shmid = shmid;
	}

	this->compressed_allocated = data_allocated;
	this->compressed_size = data_size;
}


// Reallocate uncompressed buffer with or without alpha
int VFrame::reallocate(
	unsigned char *data, 
	int shmid,
	long y_offset,
	long u_offset,
	long v_offset,
	int w, 
	int h, 
	int color_model, 
	long bytes_per_line)
{
//	if(shmid == 0) printf("VFrame::reallocate %d shmid=%d\n", __LINE__, shmid);
	clear_objects(0);
//	reset_parameters(0);
	allocate_data(data, 
		shmid,
		y_offset, 
		u_offset, 
		v_offset, 
		w, 
		h, 
		color_model, 
		bytes_per_line);
	return 0;
}

int VFrame::allocate_compressed_data(long bytes)
{
	if(bytes < 1) return 1;

// Want to preserve original contents
	if(data && compressed_allocated < bytes)
	{
		int new_shmid = -1;
		unsigned char *new_data = 0;
		if(BC_WindowBase::get_resources()->vframe_shm && use_shm)
		{
			new_shmid = shmget(IPC_PRIVATE, 
				bytes, 
				IPC_CREAT | 0777);
			new_data = (unsigned char*)shmat(new_shmid, NULL, 0);
			shmctl(new_shmid, IPC_RMID, 0);
		}
		else
		{
// Have to use malloc for libpng
			new_data = (unsigned char*)malloc(bytes);
		}

		bcopy(data, new_data, compressed_allocated);
UNBUFFER(data);

		if(memory_type == VFrame::PRIVATE)
		{
			if(shmid >= 0) 
				if(data) shmdt(data);
			else
				free(data);
		}
		else
		if(memory_type == VFrame::SHMGET)
		{
			if(data) shmdt(data);
		}

		data = new_data;
		shmid = new_shmid;
		compressed_allocated = bytes;
	}
	else
	if(!data)
	{
		if(BC_WindowBase::get_resources()->vframe_shm && use_shm)
		{
			shmid = shmget(IPC_PRIVATE, 
				bytes, 
				IPC_CREAT | 0777);
			data = (unsigned char*)shmat(shmid, NULL, 0);
			shmctl(shmid, IPC_RMID, 0);
		}
		else
		{
// Have to use malloc for libpng
			data = (unsigned char*)malloc(bytes);
		}

		compressed_allocated = bytes;
		compressed_size = 0;
	}

	return 0;
}

// scale based on the dpi for the GUI
void VFrame::read_png(const unsigned char *data, int dpi)
{
// test if scaling is needed based on BC_Resources::dp_to_px rules
	if(dpi < MIN_DPI)
	{
		read_png(data);
		return;
	}

// Load it in a temporary VFrame
	VFrame *src = new VFrame(data);
	int src_w = src->get_w();
	int src_h = src->get_h();
	int dst_w = src->get_w() * dpi / BASE_DPI;
	int dst_h = src->get_h() * dpi / BASE_DPI;


	reallocate(NULL, 
		-1,
		0, 
		0, 
		0, 
		dst_w, 
		dst_h, 
		src->get_color_model(),
		-1);

	int components = cmodel_components(get_color_model());

	int src_x1[dst_w];
	int src_x2[dst_w];
	int src_x1_a[dst_w];
	int src_x2_a[dst_w];
	for(int dst_x = 0; dst_x < dst_w; dst_x++)
	{
		float src_x = (float)dst_x * BASE_DPI / dpi;
		src_x1[dst_x] = (int)src_x;
		src_x2_a[dst_x] = (int)(255 * (src_x - src_x1[dst_x]));
		src_x2[dst_x] = src_x1[dst_x] + 1;
		if(src_x2[dst_x] >= src_w)
		{
			src_x2[dst_x] = src_w - 1;
		}
		src_x1_a[dst_x] = 255 - src_x2_a[dst_x];
	}

	for(int dst_y = 0; dst_y < dst_h; dst_y++)
	{
		float src_y = (float)dst_y * BASE_DPI / dpi;
		int src_y1 = (int)src_y;
		int src_y2_a = (int)(255 * (src_y - src_y1));
		int src_y2 = src_y1 + 1;
		if(src_y2 >= src_h)
		{
			src_y2 = src_h - 1;
		}
		int src_y1_a = 255 - src_y2_a;
		unsigned char *src_row1 = src->get_rows()[src_y1];
		unsigned char *src_row2 = src->get_rows()[src_y2];
		unsigned char *dst_row = get_rows()[dst_y];
//printf("VFrame::read_png %d %d %d %d %d\n", __LINE__, src_w, src_h, src_y1, src_y2);

		for(int dst_x = 0; dst_x < dst_w; dst_x++)
		{
			int x1 = src_x1[dst_x];
			int x2 = src_x2[dst_x];
			int x1_a = src_x1_a[dst_x];
			int x2_a = src_x2_a[dst_x];
			for(int i = 0; i < components; i++)
			{
				int accum = 
					(int)src_row1[x1 * components + i] * x1_a * src_y1_a +
					(int)src_row1[x2 * components + i] * x2_a * src_y1_a +
					(int)src_row2[x1 * components + i] * x1_a * src_y2_a +
					(int)src_row2[x2 * components + i] * x2_a * src_y2_a;
				accum /= 255 * 255;
				*dst_row++ = accum;
			}
		}
	}


	delete src;

}


int VFrame::read_png(const unsigned char *data)
{

// Test for RAW format
	if(data[4] == 'R' && 
		data[5] == 'A' &&
		data[6] == 'W' &&
		data[7] == ' ')
	{
printf("VFrame::read_png %d", __LINE__);

		int new_color_model;
		w = (data[8]) |
			(data[9] << 8) |
			(data[10] << 16) |
			(data[11] << 24);
		h = (data[12]) |
			(data[13] << 8) |
			(data[14] << 16) |
			(data[15] << 24);
		int components = data[16];
		if(components == 3)
		{
			new_color_model = BC_RGB888;
		}
		else
		if(components == 4)
		{
			new_color_model = BC_RGBA8888;
		}

// This shares the data directly
// 		reallocate(data + 20, 
// 			0, 
// 			0, 
// 			0, 
// 			w, 
// 			h, 
// 			new_color_model,
// 			-1);

// Can't use shared data for theme since button constructions overlay the
// images directly.
		reallocate(NULL, 
			-1,
			0, 
			0, 
			0, 
			w, 
			h, 
			new_color_model,
			-1);
		memcpy(get_data(), data + 20, w * h * components);

	}
	else
	if(data[4] == 0x89 &&
		data[5] == 'P' &&
		data[6] == 'N' &&
		data[7] == 'G')
	{

		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		int new_color_model;

		image_offset = 0;
		image = data + 4;
		image_size = (((unsigned long)data[0]) << 24) | 
			(((unsigned long)data[1]) << 16) | 
			(((unsigned long)data[2]) << 8) | 
			(unsigned char)data[3];
		png_set_read_fn(png_ptr, this, PngReadFunction::png_read_function);
		png_read_info(png_ptr, info_ptr);


		w = png_get_image_width(png_ptr, info_ptr);
		h = png_get_image_height(png_ptr, info_ptr);

		int src_color_model = png_get_color_type(png_ptr, info_ptr);
		switch(src_color_model)
		{
			case PNG_COLOR_TYPE_RGB:
				new_color_model = BC_RGB888;
				break;


			case PNG_COLOR_TYPE_GRAY_ALPHA:
			case PNG_COLOR_TYPE_RGB_ALPHA:
			default:
				new_color_model = BC_RGBA8888;
				break;
		}


		reallocate(NULL, 
			-1,
			0, 
			0, 
			0, 
			w, 
			h, 
			new_color_model,
			-1);

//printf("VFrame::read_png %d %d %d %p\n", __LINE__, w, h, get_rows());
		png_read_image(png_ptr, get_rows());



		if(src_color_model == PNG_COLOR_TYPE_GRAY_ALPHA)
		{
			for(int i = 0; i < get_h(); i++)
			{
				unsigned char *row = get_rows()[i];
				unsigned char *out_ptr = row + get_w() * 4 - 4;
				unsigned char *in_ptr = row + get_w() * 2 - 2;

				for(int j = get_w() - 1; j >= 0; j--)
				{
					out_ptr[0] = in_ptr[0];
					out_ptr[1] = in_ptr[0];
					out_ptr[2] = in_ptr[0];
					out_ptr[3] = in_ptr[1];
					out_ptr -= 4;
					in_ptr -= 2;
				}
			}
		}


		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	}
	else
	{
		printf("VFrame::read_png %d: unknown file format 0x%02x 0x%02x 0x%02x 0x%02x\n", 
			__LINE__,
			data[4],
			data[5],
			data[6],
			data[7]);
	}
	return 0;
}

int VFrame::write_png(const char *path, int compression)
{
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	FILE *out_fd = fopen(path, "w");
	if(!out_fd)
	{
		printf("VFrame::write_png %d %s %s\n", __LINE__, path, strerror(errno));
		return 1;
	}

	int png_cmodel = PNG_COLOR_TYPE_RGB;
	switch(get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			png_cmodel = PNG_COLOR_TYPE_RGB;
			break;
		
		case BC_A8:
			png_cmodel = PNG_COLOR_TYPE_GRAY;
			break;
	}

	png_init_io(png_ptr, out_fd);
	png_set_compression_level(png_ptr, compression);
	png_set_IHDR(png_ptr, 
		info_ptr, 
		get_w(), 
		get_h(),
    	8, 
		png_cmodel, 
		PNG_INTERLACE_NONE, 
		PNG_COMPRESSION_TYPE_DEFAULT, 
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, get_rows());
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(out_fd);
	return 0;
}



int VFrame::get_shmid()
{
	return shmid;
}

void VFrame::set_use_shm(int value)
{
	this->use_shm = value;
}

int VFrame::get_use_shm()
{
	return use_shm;
}

unsigned char* VFrame::get_data()
{
	return data;
}

long VFrame::get_compressed_allocated()
{
	return compressed_allocated;
}

long VFrame::get_compressed_size()
{
	return compressed_size;
}

long VFrame::set_compressed_size(long size)
{
	compressed_size = size;
	return 0;
}

int VFrame::get_color_model()
{
	return color_model;
}


int VFrame::equals(VFrame *frame)
{
	if(frame->data == data) 
		return 1;
	else
		return 0;
}

#define ZERO_YUV(components, type, max) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *row = (type*)get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			row[j * components] = 0; \
			row[j * components + 1] = (max + 1) / 2; \
			row[j * components + 2] = (max + 1) / 2; \
			if(components == 4) row[j * components + 3] = 0; \
		} \
	} \
}

int VFrame::clear_frame()
{
//printf("VFrame::clear_frame %d\n", __LINE__);
	switch(color_model)
	{
		case BC_COMPRESSED:
			break;

		case BC_YUV888:
			ZERO_YUV(3, unsigned char, 0xff);
			break;
		
		case BC_YUVA8888:
			ZERO_YUV(4, unsigned char, 0xff);
			break;

		case BC_YUV161616:
			ZERO_YUV(3, uint16_t, 0xffff);
			break;
		
		case BC_YUVA16161616:
			ZERO_YUV(4, uint16_t, 0xffff);
			break;
		
		default:
			bzero(data, calculate_data_size(w, h, bytes_per_line, color_model));
			break;
	}
	return 0;
}

void VFrame::rotate90()
{
// Allocate new frame
	int new_w = h, new_h = w, new_bytes_per_line = bytes_per_pixel * new_w;
	unsigned char *new_data = new unsigned char[calculate_data_size(new_w, new_h, new_bytes_per_line, color_model)];
	unsigned char **new_rows = new unsigned char*[new_h];
	for(int i = 0; i < new_h; i++)
		new_rows[i] = &new_data[new_bytes_per_line * i];

// Copy data
	for(int in_y = 0, out_x = new_w - 1; in_y < h; in_y++, out_x--)
	{
		for(int in_x = 0, out_y = 0; in_x < w; in_x++, out_y++)
		{
			for(int k = 0; k < bytes_per_pixel; k++)
			{
				new_rows[out_y][out_x * bytes_per_pixel + k] = 
					rows[in_y][in_x * bytes_per_pixel + k];
			}
		}
	}

// Swap frames
	clear_objects(0);
	data = new_data;
	rows = new_rows;
	bytes_per_line = new_bytes_per_line;
	w = new_w;
	h = new_h;
}

void VFrame::rotate270()
{
// Allocate new frame
	int new_w = h, new_h = w, new_bytes_per_line = bytes_per_pixel * new_w;
	unsigned char *new_data = new unsigned char[calculate_data_size(new_w, new_h, new_bytes_per_line, color_model)];
	unsigned char **new_rows = new unsigned char*[new_h];
	for(int i = 0; i < new_h; i++)
		new_rows[i] = &new_data[new_bytes_per_line * i];

// Copy data
	for(int in_y = 0, out_x = 0; in_y < h; in_y++, out_x++)
	{
		for(int in_x = 0, out_y = new_h - 1; in_x < w; in_x++, out_y--)
		{
			for(int k = 0; k < bytes_per_pixel; k++)
			{
				new_rows[out_y][out_x * bytes_per_pixel + k] = 
					rows[in_y][in_x * bytes_per_pixel + k];
			}
		}
	}

// Swap frames
	clear_objects(0);
	data = new_data;
	rows = new_rows;
	bytes_per_line = new_bytes_per_line;
	w = new_w;
	h = new_h;
}

void VFrame::flip_vert()
{
	unsigned char *temp = new unsigned char[bytes_per_line];
	for(int i = 0, j = h - 1; i < j; i++, j--)
	{
		memcpy(temp, rows[j], bytes_per_line);
		memcpy(rows[j], rows[i], bytes_per_line);
		memcpy(rows[i], temp, bytes_per_line);
	}
	delete [] temp;
}

void VFrame::flip_horiz()
{
	unsigned char temp[32];
	for(int i = 0; i < h; i++)
	{
		unsigned char *row = rows[i];
		for(int j = 0; j < bytes_per_line / 2; j += bytes_per_pixel)
		{
			memcpy(temp, row + j, bytes_per_pixel);
			memcpy(row + j, row + bytes_per_line - j - bytes_per_pixel, bytes_per_pixel);
			memcpy(row + bytes_per_line - j - bytes_per_pixel, temp, bytes_per_pixel);
		}
	}
}



int VFrame::copy_from(VFrame *frame)
{
	if(this->w != frame->get_w() ||
		this->h != frame->get_h())
	{
		printf("VFrame::copy_from %d sizes differ src %dx%d != dst %dx%d\n",
			__LINE__,
			frame->get_w(),
			frame->get_h(),
			get_w(),
			get_h());
		return 1;
	}

	int w = MIN(this->w, frame->get_w());
	int h = MIN(this->h, frame->get_h());
	

	switch(frame->color_model)
	{
		case BC_COMPRESSED:
			allocate_compressed_data(frame->compressed_size);
			memcpy(data, frame->data, frame->compressed_size);
			this->compressed_size = frame->compressed_size;
			break;

		case BC_YUV420P:
//printf("%d %d %p %p %p %p %p %p\n", w, h, get_y(), get_u(), get_v(), frame->get_y(), frame->get_u(), frame->get_v());
			memcpy(get_y(), frame->get_y(), w * h);
			memcpy(get_u(), frame->get_u(), w * h / 4);
			memcpy(get_v(), frame->get_v(), w * h / 4);
			break;

		case BC_YUV422P:
//printf("%d %d %p %p %p %p %p %p\n", w, h, get_y(), get_u(), get_v(), frame->get_y(), frame->get_u(), frame->get_v());
			memcpy(get_y(), frame->get_y(), w * h);
			memcpy(get_u(), frame->get_u(), w * h / 2);
			memcpy(get_v(), frame->get_v(), w * h / 2);
			break;

		default:
// printf("VFrame::copy_from %d\n", calculate_data_size(w, 
// 				h, 
// 				-1, 
// 				frame->color_model));
// Copy without extra 4 bytes in case the source is a hardware device
			memcpy(data, frame->data, get_data_size());
			break;
	}

	return 0;
}


#define OVERLAY(type, max, components) \
{ \
	type **in_rows = (type**)src->get_rows(); \
	type **out_rows = (type**)get_rows(); \
	int in_w = src->get_w(); \
	int in_h = src->get_h(); \
 \
	for(int i = 0; i < in_h; i++) \
	{ \
		if(i + out_y1 >= 0 && i + out_y1 < h) \
		{ \
			type *src_row = in_rows[i]; \
			type *dst_row = out_rows[i + out_y1] + out_x1 * components; \
 \
			for(int j = 0; j < in_w; j++) \
			{ \
				if(j + out_x1 >= 0 && j + out_x1 < w) \
				{ \
					int opacity = src_row[3]; \
					int transparency = dst_row[3] * (max - src_row[3]) / max; \
					dst_row[0] = (transparency * dst_row[0] + opacity * src_row[0]) / max; \
					dst_row[1] = (transparency * dst_row[1] + opacity * src_row[1]) / max; \
					dst_row[2] = (transparency * dst_row[2] + opacity * src_row[2]) / max; \
					dst_row[3] = MAX(dst_row[3], src_row[3]); \
				} \
 \
				dst_row += components; \
				src_row += components; \
			} \
		} \
	} \
}


void VFrame::overlay(VFrame *src, 
		int out_x1, 
		int out_y1)
{
	switch(get_color_model())
	{
		case BC_RGBA8888:
			OVERLAY(unsigned char, 0xff, 4);
			break;
	}
}



int VFrame::get_scale_tables(int *column_table, int *row_table, 
			int in_x1, int in_y1, int in_x2, int in_y2,
			int out_x1, int out_y1, int out_x2, int out_y2)
{
	int y_out, i;
	float w_in = in_x2 - in_x1;
	float h_in = in_y2 - in_y1;
	int w_out = out_x2 - out_x1;
	int h_out = out_y2 - out_y1;

	float hscale = w_in / w_out;
	float vscale = h_in / h_out;

	for(i = 0; i < w_out; i++)
	{
		column_table[i] = (int)(hscale * i);
	}

	for(i = 0; i < h_out; i++)
	{
		row_table[i] = (int)(vscale * i) + in_y1;
	}
	return 0;
}

int VFrame::get_bytes_per_pixel()
{
	return bytes_per_pixel;
}

unsigned char** VFrame::get_rows()
{
	if(rows)
	{
		return rows;
	}
	return 0;
}

int VFrame::get_w()
{
	return w;
}

int VFrame::get_h()
{
	return h;
}

int VFrame::get_w_fixed()
{
	return w - 1;
}

int VFrame::get_h_fixed()
{
	return h - 1;
}

unsigned char* VFrame::get_y()
{
	return y;
}

unsigned char* VFrame::get_u()
{
	return u;
}

unsigned char* VFrame::get_v()
{
	return v;
}

void VFrame::set_number(long number)
{
	sequence_number = number;
}

long VFrame::get_number()
{
	return sequence_number;
}

void VFrame::push_prev_effect(const char *name)
{
	char *ptr;
	prev_effects.append(ptr = new char[strlen(name) + 1]);
	strcpy(ptr, name);
	if(prev_effects.total > MAX_STACK_ELEMENTS) prev_effects.remove_object(0);
}

void VFrame::pop_prev_effect()
{
	if(prev_effects.total)
		prev_effects.remove_object(prev_effects.last());
}

void VFrame::push_next_effect(const char *name)
{
	char *ptr;
	next_effects.append(ptr = new char[strlen(name) + 1]);
	strcpy(ptr, name);
	if(next_effects.total > MAX_STACK_ELEMENTS) next_effects.remove_object(0);
}

void VFrame::pop_next_effect()
{
	if(next_effects.total)
		next_effects.remove_object(next_effects.last());
}

const char* VFrame::get_next_effect(int number)
{
	if(!next_effects.total) return "";
	else
	if(number > next_effects.total - 1) number = next_effects.total - 1;

	return next_effects.values[next_effects.total - number - 1];
}

const char* VFrame::get_prev_effect(int number)
{
	if(!prev_effects.total) return "";
	else
	if(number > prev_effects.total - 1) number = prev_effects.total - 1;

	return prev_effects.values[prev_effects.total - number - 1];
}

BC_Hash* VFrame::get_params()
{
	return params;
}

void VFrame::clear_stacks()
{
	next_effects.remove_all_objects();
	prev_effects.remove_all_objects();
	delete params;
	params = new BC_Hash;
}

void VFrame::copy_params(VFrame *src)
{
	params->copy_from(src->params);
}

void VFrame::copy_stacks(VFrame *src)
{
	clear_stacks();

	for(int i = 0; i < src->next_effects.total; i++)
	{
		char *ptr;
		next_effects.append(ptr = new char[strlen(src->next_effects.values[i]) + 1]);
		strcpy(ptr, src->next_effects.values[i]);
	}
	for(int i = 0; i < src->prev_effects.total; i++)
	{
		char *ptr;
		prev_effects.append(ptr = new char[strlen(src->prev_effects.values[i]) + 1]);
		strcpy(ptr, src->prev_effects.values[i]);
	}

	params->copy_from(src->params);
}

int VFrame::equal_stacks(VFrame *src)
{
	for(int i = 0; i < src->next_effects.total && i < next_effects.total; i++)
	{
		if(strcmp(src->next_effects.values[i], next_effects.values[i])) return 0;
	}

	for(int i = 0; i < src->prev_effects.total && i < prev_effects.total; i++)
	{
		if(strcmp(src->prev_effects.values[i], prev_effects.values[i])) return 0;
	}

	if(!params->equivalent(src->params)) return 0;
	return 1;
}

void VFrame::dump_stacks()
{
	printf("VFrame::dump_stacks\n");
	printf("	next_effects:\n");
	for(int i = next_effects.total - 1; i >= 0; i--)
		printf("		%s\n", next_effects.values[i]);
	printf("	prev_effects:\n");
	for(int i = prev_effects.total - 1; i >= 0; i--)
		printf("		%s\n", prev_effects.values[i]);
}

void VFrame::dump_params()
{
	params->dump();
}

void VFrame::dump()
{
	printf("VFrame::dump %d this=%p\n", __LINE__, this);
	printf("    w=%d h=%d colormodel=%d rows=%p opengl_state=%d use_shm=%d shmid=%d\n", 
		w, 
		h,
		color_model,
		rows,
        opengl_state,
		use_shm,
		shmid);
}

int VFrame::filefork_size()
{
	return sizeof(int) * 12 + sizeof(long);
}


void VFrame::to_filefork(unsigned char *buffer)
{
//printf("VFrame::to_filefork %d shmid=%d w=%d h=%d\n", __LINE__, shmid, w, h);
	*(int*)(buffer + 0) = shmid;
	*(int*)(buffer + 4) = y_offset;
	*(int*)(buffer + 8) = u_offset;
	*(int*)(buffer + 12) = v_offset;
	*(int*)(buffer + 16) = w;
	*(int*)(buffer + 20) = h;
	*(int*)(buffer + 24) = color_model;
	*(int*)(buffer + 28) = bytes_per_line;
	*(int*)(buffer + 32) = compressed_allocated;
	*(int*)(buffer + 36) = compressed_size;
	*(int*)(buffer + 40) = is_keyframe;
	*(long*)(buffer + 44) = sequence_number;
	
	
//printf("VFrame::to_filefork %d %lld\n", __LINE__, sequence_number);
// printf("VFrame::to_filefork %d", __LINE__);
// for(int i = 0; i < 40; i++)
// {
// printf(" %02x", buffer[i]);
// }
// printf("\n");
// dump();
}


void VFrame::from_filefork(unsigned char *buffer)
{
// This frame will always be preallocated shared memory
//printf("VFrame::from_filefork %d %d\n", __LINE__, *(int*)(buffer + 24));
	if(*(int*)(buffer + 24) == BC_COMPRESSED)
	{
		set_compressed_memory(0,
			*(int*)(buffer + 0), // shmid
			*(int*)(buffer + 36), // compressed_size
			*(int*)(buffer + 32)); // compressed_allocated
		color_model = BC_COMPRESSED;
//printf("VFrame::from_filefork %d %d\n", __LINE__, get_compressed_size());

	}
	else
	{
// printf("VFrame::from_filefork %d", __LINE__);
// for(int i = 0; i < 40; i++)
// {
// printf(" %02x", buffer[i]);
// }
// printf("\n");
		reallocate(0,
			*(int*)(buffer + 0), // shmid
			*(int*)(buffer + 4), // y_offset
			*(int*)(buffer + 8), // u_offset
			*(int*)(buffer + 12), // v_offset
			*(int*)(buffer + 16), // w
			*(int*)(buffer + 20), // h
			*(int*)(buffer + 24), // colormodel
			*(int*)(buffer + 28)); // bytes per line
//dump();
	}

	is_keyframe = *(int*)(buffer + 40);
	sequence_number = *(long*)(buffer + 44);
//printf("VFrame::from_filefork %d %lld\n", __LINE__, sequence_number);
}

int VFrame::get_memory_usage()
{
	if(get_compressed_allocated()) return get_compressed_allocated();
	return get_h() * get_bytes_per_line();
}

void VFrame::draw_pixel(int x, int y)
{
	if(!(x >= 0 && y >= 0 && x < get_w() && y < get_h())) return;

#define DRAW_PIXEL(x, y, components, do_yuv, max, type) \
{ \
	type **rows = (type**)get_rows(); \
	rows[y][x * components] = max - rows[y][x * components]; \
	if(!do_yuv) \
	{ \
		rows[y][x * components + 1] = max - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = max - rows[y][x * components + 2]; \
	} \
	else \
	{ \
		rows[y][x * components + 1] = (max / 2 + 1) - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = (max / 2 + 1) - rows[y][x * components + 2]; \
	} \
	if(components == 4) \
		rows[y][x * components + 3] = max; \
}


	switch(get_color_model())
	{
		case BC_RGB888:
			DRAW_PIXEL(x, y, 3, 0, 0xff, unsigned char);
			break;
		case BC_RGBA8888:
			DRAW_PIXEL(x, y, 4, 0, 0xff, unsigned char);
			break;
		case BC_RGB_FLOAT:
			DRAW_PIXEL(x, y, 3, 0, 1.0, float);
			break;
		case BC_RGBA_FLOAT:
			DRAW_PIXEL(x, y, 4, 0, 1.0, float);
			break;
		case BC_YUV888:
			DRAW_PIXEL(x, y, 3, 1, 0xff, unsigned char);
			break;
		case BC_YUVA8888:
			DRAW_PIXEL(x, y, 4, 1, 0xff, unsigned char);
			break;
		case BC_RGB161616:
			DRAW_PIXEL(x, y, 3, 0, 0xffff, uint16_t);
			break;
		case BC_YUV161616:
			DRAW_PIXEL(x, y, 3, 1, 0xffff, uint16_t);
			break;
		case BC_RGBA16161616:
			DRAW_PIXEL(x, y, 4, 0, 0xffff, uint16_t);
			break;
		case BC_YUVA16161616:
			DRAW_PIXEL(x, y, 4, 1, 0xffff, uint16_t);
			break;
	}
}




void VFrame::draw_line(int x1, int y1, int x2, int y2)
{
	int w = labs(x2 - x1);
	int h = labs(y2 - y1);
//printf("FindObjectMain::draw_line 1 %d %d %d %d\n", x1, y1, x2, y2);

	if(!w && !h)
	{
		draw_pixel(x1, y1);
	}
	else
	if(w > h)
	{
// Flip coordinates so x1 < x2
		if(x2 < x1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = y2 - y1;
		int denominator = x2 - x1;
		for(int i = x1; i <= x2; i++)
		{
			int y = y1 + (int64_t)(i - x1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(i, y);
		}
	}
	else
	{
// Flip coordinates so y1 < y2
		if(y2 < y1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = x2 - x1;
		int denominator = y2 - y1;
		for(int i = y1; i <= y2; i++)
		{
			int x = x1 + (int64_t)(i - y1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(x, i);
		}
	}
//printf("FindObjectMain::draw_line 2\n");
}



void VFrame::draw_rect(int x1, int y1, int x2, int y2)
{
	draw_line(x1, y1, x2, y1);
	draw_line(x2, y1 + 1, x2, y2);
	draw_line(x2 - 1, y2, x1, y2);
	draw_line(x1, y2 - 1, x1, y1 + 1);
}



void VFrame::draw_oval(int x1, int y1, int x2, int y2)
{
	int w = x2 - x1;
	int h = y2 - y1;
	int center_x = (x2 + x1) / 2;
	int center_y = (y2 + y1) / 2;
	int x_table[h / 2];

//printf("VFrame::draw_oval %d %d %d %d %d\n", __LINE__, x1, y1, x2, y2);

	for(int i = 0; i < h / 2; i++)
	{
// A^2 = -(B^2) + C^2
		x_table[i] = (int)(sqrt(-SQR(h / 2 - i) + SQR(h / 2)) * w / h);
//printf("VFrame::draw_oval %d i=%d x=%d\n", __LINE__, i, x_table[i]);
	}

	for(int i = 0; i < h / 2 - 1; i++)
	{
		int x3 = x_table[i];
		int x4 = x_table[i + 1];

		if(x4 > x3 + 1)
		{
			for(int j = x3; j < x4; j++)
			{
				draw_pixel(center_x + j, y1 + i);
				draw_pixel(center_x - j, y1 + i);
				draw_pixel(center_x + j, y2 - i - 1);
				draw_pixel(center_x - j, y2 - i - 1);
			}
		}
		else
		{
			draw_pixel(center_x + x3, y1 + i);
			draw_pixel(center_x - x3, y1 + i);
			draw_pixel(center_x + x3, y2 - i - 1);
			draw_pixel(center_x - x3, y2 - i - 1);
		}
	}
	
	draw_pixel(center_x, y1);
	draw_pixel(center_x, y2 - 1);
 	draw_pixel(x1, center_y);
 	draw_pixel(x2 - 1, center_y);
 	draw_pixel(x1, center_y - 1);
 	draw_pixel(x2 - 1, center_y - 1);

	
}



#define ARROW_SIZE 10
void VFrame::draw_arrow(int x1, int y1, int x2, int y2)
{
	double angle = atan((float)(y2 - y1) / (float)(x2 - x1));
	double angle1 = angle + (float)145 / 360 * 2 * 3.14159265;
	double angle2 = angle - (float)145 / 360 * 2 * 3.14159265;
	int x3;
	int y3;
	int x4;
	int y4;
	if(x2 < x1)
	{
		x3 = x2 - (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 - (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 - (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 - (int)(ARROW_SIZE * sin(angle2));
	}
	else
	{
		x3 = x2 + (int)(ARROW_SIZE * cos(angle1));
		y3 = y2 + (int)(ARROW_SIZE * sin(angle1));
		x4 = x2 + (int)(ARROW_SIZE * cos(angle2));
		y4 = y2 + (int)(ARROW_SIZE * sin(angle2));
	}

// Main vector
	draw_line(x1, y1, x2, y2);
//	draw_line(x1, y1 + 1, x2, y2 + 1);

// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(x2, y2, x3, y3);
// Arrow line
	if(abs(y2 - y1) || abs(x2 - x1)) draw_line(x2, y2, x4, y4);
}











