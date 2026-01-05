/*
 * CINELERRA
 * Copyright (C) 2008-2022 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "edit.h"
#include "file.h"
#include "filepng.h"
#include "language.h"
#include "mwindow.inc"
#include "quicktime.h"
#include "vframe.h"
#include "videodevice.inc"

#include <png.h>
#include <string.h>

FilePNG::FilePNG(Asset *asset, File *file)
 : FileList(asset, file, "PNGLIST", ".png", FILE_PNG, FILE_PNG_LIST)
{
    temp = 0;
}

FilePNG::~FilePNG()
{
    delete temp;
}


FilePNG::FilePNG()
 : FileList()
{
    ids.append(FILE_PNG);
    ids.append(FILE_PNG_LIST);
    has_video = 1;
    has_wr = 1;
    has_rd = 1;
}

FileBase* FilePNG::create(File *file)
{
    return new FilePNG(file->asset, file);
}

const char* FilePNG::formattostr(int format)
{
    switch(format)
    {
		case FILE_PNG:
			return PNG_NAME;
			break;
		case FILE_PNG_LIST:
			return PNG_LIST_NAME;
			break;
    }
    return 0;
}

const char* FilePNG::get_tag(int format)
{
    switch(format)
    {
		case FILE_PNG:
            return "png";
		case FILE_PNG_LIST:
            return "list";
    }
    return 0;
}

int FilePNG::check_sig(File *file, const uint8_t *test_data)
{
    Asset *asset = file->asset;
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{

//printf("FilePNG::check_sig 1\n");
		char test[16];
		int temp = fread(test, 16, 1, stream);
		fclose(stream);

//		if(png_sig_cmp((unsigned char*)test, 0, 8))
		if(png_check_sig((unsigned char*)test, 8))
		{
//printf("FilePNG::check_sig 1\n");
			return 1;
		}
		else
		if(test[0] == 'P' && test[1] == 'N' && test[2] == 'G' && 
			test[3] == 'L' && test[4] == 'I' && test[5] == 'S' && test[6] == 'T')
		{
//printf("FilePNG::check_sig 1\n");
			return 1;
		}
	}
	return 0;
}



void FilePNG::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int option_type,
	const char *locked_compressor)
{
	if(option_type == VIDEO_PARAMS)
	{
		PNGConfigVideo *window = new PNGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}




int FilePNG::can_copy_from(Asset *asset, int64_t position)
{
	if(asset->format == FILE_MOV)
	{
		if(match4(asset->vcodec, QUICKTIME_PNG)) return 1;
	}
	else
	if(asset->format == FILE_PNG || 
		asset->format == FILE_PNG_LIST)
		return 1;

	return 0;
}


// int FilePNG::colormodel_supported(int colormodel)
// {
// 	return colormodel;
// }


int FilePNG::get_best_colormodel(Asset *asset, 
    VideoInConfig *in_config, 
    VideoOutConfig *out_config)
{
	if(asset->png_use_alpha)
		return BC_RGBA8888;
	else
		return BC_RGB888;
}




int FilePNG::read_frame_header(char *path)
{
	int result = 0;
	int color_type;
	int color_depth;
	int num_trans = 0;
	
	FILE *stream;

	if(!(stream = fopen(path, "rb")))
	{
		perror("FilePNG::read_frame_header");
		return 1;
	}

	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, stream);


	png_read_info(png_ptr, info_ptr);

	asset->width = png_get_image_width(png_ptr, info_ptr);
	asset->height = png_get_image_height(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(stream);
	
	
	
	return result;
}




static void read_function(png_structp png_ptr, 
	png_bytep data, 
	png_uint_32 length)
{
	VFrame *input = (VFrame*)png_get_io_ptr(png_ptr);

	memcpy(data, input->get_data() + input->get_compressed_size(), length);
	input->set_compressed_size(input->get_compressed_size() + length);
}

static void write_function(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	VFrame *output = (VFrame*)png_get_io_ptr(png_ptr);

	if(output->get_compressed_allocated() < output->get_compressed_size() + length)
		output->allocate_compressed_data((output->get_compressed_allocated() + length) * 2);
	memcpy(output->get_data() + output->get_compressed_size(), data, length);
	output->set_compressed_size(output->get_compressed_size() + length);
}

static void flush_function(png_structp png_ptr)
{
	;
}



int FilePNG::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
	PNGUnit *png_unit = (PNGUnit*)unit;
	int result = 0;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;	
	VFrame *output_frame;
	data->set_compressed_size(0);

//printf("FilePNG::write_frame 1\n");
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_set_write_fn(png_ptr,
               data, 
			   (png_rw_ptr)write_function,
               (png_flush_ptr)flush_function);
	png_set_compression_level(png_ptr, 9);

	png_set_IHDR(png_ptr, 
		info_ptr, 
		asset->width, 
		asset->height,
    	8, 
		asset->png_use_alpha ? 
		  PNG_COLOR_TYPE_RGB_ALPHA : 
		  PNG_COLOR_TYPE_RGB, 
		PNG_INTERLACE_NONE, 
		PNG_COMPRESSION_TYPE_DEFAULT, 
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

//printf("FilePNG::write_frame 1\n");
	int png_cmodel = asset->png_use_alpha ? BC_RGBA8888 : BC_RGB888;
	if(frame->get_color_model() != png_cmodel)
	{
		if(!png_unit->temp_frame) png_unit->temp_frame = new VFrame(0, 
			-1,
			asset->width, 
			asset->height, 
			png_cmodel,
			-1);

		cmodel_transfer(png_unit->temp_frame->get_rows(), /* Leave NULL if non existent */
			frame->get_rows(),
			png_unit->temp_frame->get_y(), /* Leave NULL if non existent */
			png_unit->temp_frame->get_u(),
			png_unit->temp_frame->get_v(),
			png_unit->temp_frame->get_a(),
			frame->get_y(), /* Leave NULL if non existent */
			frame->get_u(),
			frame->get_v(),
			frame->get_a(),
			0,        /* Dimensions to capture from input frame */
			0, 
			asset->width, 
			asset->height,
			0,       /* Dimensions to project on output frame */
			0, 
			asset->width, 
			asset->height,
			frame->get_color_model(), 
			png_unit->temp_frame->get_color_model(),
			0,         /* When transferring BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			asset->width,       /* For planar use the luma rowspan */
			asset->height);
		
		output_frame = png_unit->temp_frame;
	}
	else
		output_frame = frame;


//printf("FilePNG::write_frame 2\n");
	png_write_image(png_ptr, output_frame->get_rows());
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
//printf("FilePNG::write_frame 3 %d\n", data->get_compressed_size());

	return result;
}

int FilePNG::read_frame(VFrame *output, VFrame *input)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;	
	int result = 0;
	int size = input->get_compressed_size();
	input->set_compressed_size(0);
	
	
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_set_read_fn(png_ptr, input, (png_rw_ptr)read_function);
	png_read_info(png_ptr, info_ptr);

 	int png_color_type = png_get_color_type(png_ptr, info_ptr);
	int png_color_depth = png_get_bit_depth(png_ptr, info_ptr);
    int png_channels = png_get_channels(png_ptr, info_ptr);

// convert greyscale to RGB in libpng
 	if (png_color_type == PNG_COLOR_TYPE_GRAY ||
         	png_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
 	{
 		png_set_gray_to_rgb(png_ptr);
 	}

// convert palette to RGB in libpng
	if (png_color_type == PNG_COLOR_TYPE_PALETTE)
	{
	    png_set_palette_to_rgb(png_ptr);
	}

// convert bits per channel
	if (png_color_depth <= 8)
	{
	    png_set_expand(png_ptr);
	}

// compute the input color model after libpng conversion
    int input_cmodel = BC_RGB888;
// printf("FilePNG::read_frame %d png_color_type=%d png_color_depth=%d png_channels=%d\n", 
// __LINE__, 
// png_color_type,
// png_color_depth,
// png_channels);
    switch(png_color_type)
    {
        case PNG_COLOR_TYPE_PALETTE:
            input_cmodel = BC_RGBA8888;
            break;
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_RGB:
            input_cmodel = BC_RGB888;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
        case PNG_COLOR_TYPE_RGB_ALPHA:
            input_cmodel = BC_RGBA8888;
            break;
    }

// can't use the file class since FileList uses a temporary
    VFrame *output2 = output;
    if(output->get_color_model() != input_cmodel)
    {
        if(!temp)
        {
            temp = new VFrame;
            temp->set_use_shm(0);
            temp->reallocate(0, // data
	  	        -1, // shmid
                0, // Y
                0, // U
                0, // V
	  	        asset->width, // w
		        asset->height, // h
                input_cmodel, // color_model
                -1); // bytes_per_line
        }
        output2 = temp;
    }

// read the image
	png_read_image(png_ptr, output2->get_rows());

// printf("FilePNG::read_frame %d input_cmodel=%d temp=%p %02x %02x %02x %02x %02x %02x %02x %02x\n",
// __LINE__,
// input_cmodel,
// temp,
// temp->get_rows()[0][0],
// temp->get_rows()[0][1],
// temp->get_rows()[0][2],
// temp->get_rows()[0][3],
// temp->get_rows()[0][4],
// temp->get_rows()[0][5],
// temp->get_rows()[0][6],
// temp->get_rows()[0][7]);

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

// convert to the file temporary
    if(output != output2)
    {
		cmodel_transfer(output->get_rows(), 
			output2->get_rows(),
			output->get_y(),
			output->get_u(),
			output->get_v(),
			output->get_a(),
			output2->get_y(),
			output2->get_u(),
			output2->get_v(),
			output2->get_a(),
			0, 
			0, 
			asset->width, 
			asset->height,
			0, 
			0, 
			asset->width, 
			asset->height,
			output2->get_color_model(), 
			output->get_color_model(),
			0,
			asset->width,
			asset->width);
    }

//printf("FilePNG::read_frame 4\n");
	return result;
}

FrameWriterUnit* FilePNG::new_writer_unit(FrameWriter *writer)
{
	return new PNGUnit(this, writer);
}












PNGUnit::PNGUnit(FilePNG *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	this->file = file;
	temp_frame = 0;
}
PNGUnit::~PNGUnit()
{
	if(temp_frame) delete temp_frame;
}









PNGConfigVideo::PNGConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	DP(200),
	DP(100))
{
	this->parent_window = parent_window;
	this->asset = asset;
}

PNGConfigVideo::~PNGConfigVideo()
{
}

void PNGConfigVideo::create_objects()
{
	int x = DP(10), y = DP(10);
	lock_window("PNGConfigVideo::create_objects");
	add_subwindow(new PNGUseAlpha(this, x, y));
	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

int PNGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}


PNGUseAlpha::PNGUseAlpha(PNGConfigVideo *gui, int x, int y)
 : BC_CheckBox(x, y, gui->asset->png_use_alpha, _("Use alpha"))
{
	this->gui = gui;
}

int PNGUseAlpha::handle_event()
{
	gui->asset->png_use_alpha = get_value();
	return 1;
}



