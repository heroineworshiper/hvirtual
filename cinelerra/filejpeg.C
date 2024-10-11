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
#include "bcsignals.h"
#include "edit.h"
#include "file.h"
#include "filejpeg.h"
#include "jpegwrapper.h"
#include "language.h"
#include "libmjpeg.h"
#include "mwindow.inc"
#include "quicktime.h"
#include "playbackconfig.h"
#include "recordconfig.h"
#include "vframe.h"
#include "videodevice.inc"


#include <errno.h>
#include <string.h>


FileJPEG::FileJPEG(Asset *asset, File *file)
 : FileList(asset, file, "JPEGLIST", ".jpg", FILE_JPEG, FILE_JPEG_LIST)
{
	decompressor = 0;
}

FileJPEG::~FileJPEG()
{
	if(decompressor) mjpeg_delete((mjpeg_t*)decompressor);
}



FileJPEG::FileJPEG()
 : FileList()
{
    ids.append(FILE_JPEG);
    ids.append(FILE_JPEG_LIST);
    has_video = 1;
    has_wr = 1;
    has_rd = 1;
}

FileBase* FileJPEG::create(File *file)
{
    return new FileJPEG(file->asset, file);
}

const char* FileJPEG::formattostr(int format)
{
    switch(format)
    {
		case FILE_JPEG:
			return JPEG_NAME;
			break;
		case FILE_JPEG_LIST:
			return JPEG_LIST_NAME;
			break;
    }
    return 0;
}

const char* FileJPEG::get_tag(int format)
{
    switch(format)
    {
		case FILE_JPEG:
		case FILE_JPEG_LIST:
            return "jpg";
    }
    return 0;
}

int FileJPEG::check_sig(File *file, const uint8_t *test_data)
{
    Asset *asset = file->asset;
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{
		char test[10];
		int temp = fread(test, 10, 1, stream);
		fclose(stream);

		if(test[6] == 'J' && test[7] == 'F' && test[8] == 'I' && test[9] == 'F')
		{
			return 1;
		}
		else
		if(test[0] == 'J' && test[1] == 'P' && test[2] == 'E' && test[3] == 'G' && 
			test[4] == 'L' && test[5] == 'I' && test[6] == 'S' && test[7] == 'T')
		{
			return 1;
		}
	}

	if(strlen(asset->path) > 4)
	{
		int len = strlen(asset->path);
		if(!strncasecmp(asset->path + len - 4, ".jpg", 4)) return 1;
	}
	return 0;
}



void FileJPEG::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int option_type,
	const char *locked_compressor)
{
	if(option_type == VIDEO_PARAMS)
	{
		JPEGConfigVideo *window = new JPEGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}


int FileJPEG::can_copy_from(Asset *asset, int64_t position)
{
//printf("FileJPEG::can_copy_from %d %s\n", asset->format, asset->vcodec);
	if(asset->format == FILE_MOV)
	{
		if(match4(asset->vcodec, QUICKTIME_JPEG)) return 1;
	}
	else
	if(asset->format == FILE_JPEG || 
		asset->format == FILE_JPEG_LIST)
		return 1;

	return 0;
}

// int FileJPEG::colormodel_supported(int colormodel)
// {
// 	return colormodel;
// }


int FileJPEG::get_best_colormodel(Asset *asset, 
        VideoInConfig *in_config, 
        VideoOutConfig *out_config)
{
    if(in_config)
    {
        switch(in_config->driver)
        {
		case VIDEO4LINUX:
			return BC_YUV420P;
			break;
		case VIDEO4LINUX2:
            switch(in_config->v4l2_format)
            {
                case CAPTURE_RGB:
                    return BC_YUV420P;
                    break;
                case CAPTURE_YUYV:
                    return BC_YUV422;
                    break;
                case CAPTURE_JPEG:
                case CAPTURE_JPEG_NOHEAD:
                case CAPTURE_MJPG_1FIELD:
                    return BC_COMPRESSED;
                    break;
                case CAPTURE_MJPG:
                    return BC_YUV422;
                    break;
            }
			return BC_YUV420P;
			break;
		case CAPTURE_BUZ:
		case CAPTURE_LML:
//		case VIDEO4LINUX2MJPG:
			return BC_YUV422;
			break;
//		case VIDEO4LINUX2JPEG:
//			return BC_YUV420P;
//			break;
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			return BC_YUV420P;
			break;
        }
    }
    
    if(out_config)
    {
        switch(out_config->driver)
        {
		case PLAYBACK_X11:
			return BC_RGB888;
			break;
		case PLAYBACK_X11_XV:
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
		case PLAYBACK_ASYNCHRONOUS:
			return BC_YUV420P;
			break;
		case PLAYBACK_X11_GL:
			return BC_YUV888;
			break;
		case PLAYBACK_LML:
		case PLAYBACK_BUZ:
			return BC_YUV422P;
			break;
        }
    }

	return BC_YUV420P;
}


int FileJPEG::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
	int result = 0;
	JPEGUnit *jpeg_unit = (JPEGUnit*)unit;

//PRINT_TRACE
	if(!jpeg_unit->compressor)
		jpeg_unit->compressor = mjpeg_new(asset->width, 
			asset->height, 
			1);

	mjpeg_set_quality((mjpeg_t*)jpeg_unit->compressor, asset->jpeg_quality);

// printf("FileJPEG::write_frame %d color_model=%d %dx%d rows=%p\n", 
// __LINE__, frame->get_color_model(),
// asset->width, asset->height, frame->get_rows()[0]);
//for(int i = 0; i < asset->width * asset->height * 3; i++)
//((float*)frame->get_rows()[0])[i] = 1;
	mjpeg_compress((mjpeg_t*)jpeg_unit->compressor, 
		frame->get_rows(), 
		frame->get_y(), 
		frame->get_u(), 
		frame->get_v(),
		frame->get_color_model(),
		1);
//PRINT_TRACE


// insert spherical tag
	if(asset->jpeg_sphere)
	{
		const char *sphere_tag = 
			"http://ns.adobe.com/xap/1.0/\x00<?xpacket begin='\xef\xbb\xbf' id='W5M0MpCehiHzreSzNTczkc9d'?>\n"
			"<x:xmpmeta xmlns:x='adobe:ns:meta/' x:xmptk='Image::Cinelerra'>\n"
			"<rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
			"\n"
			" <rdf:Description rdf:about=''\n"
			"  xmlns:GPano='http://ns.google.com/photos/1.0/panorama/'>\n"
			"  <GPano:ProjectionType>equirectangular</GPano:ProjectionType>\n"
			" </rdf:Description>\n"
			"</rdf:RDF>\n"
			"</x:xmpmeta>\n"
			"<?xpacket end='w'?>";
//PRINT_TRACE

// calculate length by skipping the \x00 byte
		int skip = 32;
		int tag_len = strlen(sphere_tag + skip) + skip;
		int tag_len2 = tag_len + 2;
		int tag_len3 = tag_len + 4;
		
		data->allocate_compressed_data(
			mjpeg_output_size((mjpeg_t*)jpeg_unit->compressor) + tag_len3);
		data->set_compressed_size(
			mjpeg_output_size((mjpeg_t*)jpeg_unit->compressor) + tag_len3);
			
		int jfif_size = 0x14;
		uint8_t *ptr = data->get_data();
		memcpy(ptr, 
			mjpeg_output_buffer((mjpeg_t*)jpeg_unit->compressor), 
			jfif_size);
		ptr += jfif_size;
		*ptr++ = 0xff;
		*ptr++ = 0xe1;
		*ptr++ = (tag_len2 >> 8) & 0xff;
		*ptr++ = tag_len2 & 0xff;
		memcpy(ptr,
			sphere_tag,
			tag_len);
		ptr += tag_len;
		memcpy(ptr,
			mjpeg_output_buffer((mjpeg_t*)jpeg_unit->compressor) + jfif_size,
			mjpeg_output_size((mjpeg_t*)jpeg_unit->compressor) - jfif_size);
	}
	else
	{
//PRINT_TRACE
		data->allocate_compressed_data(mjpeg_output_size((mjpeg_t*)jpeg_unit->compressor));
//PRINT_TRACE
		data->set_compressed_size(mjpeg_output_size((mjpeg_t*)jpeg_unit->compressor));
//PRINT_TRACE
		memcpy(data->get_data(), 
			mjpeg_output_buffer((mjpeg_t*)jpeg_unit->compressor), 
			mjpeg_output_size((mjpeg_t*)jpeg_unit->compressor));
//PRINT_TRACE
	}

//PRINT_TRACE
	return result;
}










int FileJPEG::read_frame_header(char *path)
{
	int result = 0;


	FILE *stream;

	if(!(stream = fopen(path, "rb")))
	{
		printf("FileJPEG::read_frame_header %d %s: %s\n", __LINE__, path, strerror(errno));
		return 1;
	}
//printf("FileJPEG::read_frame_header %d\n", __LINE__);

	unsigned char test[2];
	int temp = fread(test, 2, 1, stream);
	fseek(stream, 0, SEEK_SET);
	if(test[0] != 0xff ||
		test[1] != 0xd8)
		return 1;
	

	struct jpeg_decompress_struct jpeg_decompress;
	struct jpeg_error_mgr jpeg_error;

	jpeg_decompress.err = jpeg_std_error(&jpeg_error);
	jpeg_create_decompress(&jpeg_decompress);

	jpeg_stdio_src(&jpeg_decompress, stream);
	jpeg_read_header(&jpeg_decompress, TRUE);

	asset->width = jpeg_decompress.image_width;
	asset->height = jpeg_decompress.image_height;

	jpeg_destroy((j_common_ptr)&jpeg_decompress);
	fclose(stream);
//printf("FileJPEG::read_frame_header %d\n", __LINE__);

	return result;
}



int FileJPEG::read_frame(VFrame *output, VFrame *input)
{
	if(input->get_compressed_size() < 2 ||
		input->get_data()[0] != 0xff ||
		input->get_data()[1] != 0xd8)
		return 1;

	if(!decompressor) decompressor = mjpeg_new(asset->width, 
		asset->height, 
		1);
// printf("FileJPEG::read_frame %d %p %d %d %d %p %p %p %p %d\n", 
// __LINE__,
// input->get_data(), 
// input->get_compressed_size(),
// output->get_w(),
// output->get_h(),
// output->get_rows(), 
// output->get_y(), 
// output->get_u(), 
// output->get_v(),
// output->get_color_model());
	mjpeg_decompress((mjpeg_t*)decompressor, 
		input->get_data(), 
		input->get_compressed_size(),
		0,  
		output->get_rows(), 
		output->get_y(), 
		output->get_u(), 
		output->get_v(),
		output->get_color_model(),
		1);
//	PRINT_TRACE

//printf("FileJPEG::read_frame %d\n", __LINE__);
	return 0;
}

FrameWriterUnit* FileJPEG::new_writer_unit(FrameWriter *writer)
{
	return new JPEGUnit(this, writer);
}






JPEGUnit::JPEGUnit(FileJPEG *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	this->file = file;
	compressor = 0;
}
JPEGUnit::~JPEGUnit()
{
	if(compressor) mjpeg_delete((mjpeg_t*)compressor);
}







JPEGConfigVideo::JPEGConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	DP(400),
	DP(200))
{
	this->parent_window = parent_window;
	this->asset = asset;
}

JPEGConfigVideo::~JPEGConfigVideo()
{
}

void JPEGConfigVideo::create_objects()
{
	int x = DP(10), y = DP(10);
	lock_window("JPEGConfigVideo::create_objects");
	add_subwindow(new BC_Title(x, y, _("Quality:")));
	BC_ISlider *slider;
	add_subwindow(slider = new BC_ISlider(x + DP(80), 
		y,
		0,
		DP(200),
		DP(200),
		0,
		100,
		asset->jpeg_quality,
		0,
		0,
		&asset->jpeg_quality));
	y += slider->get_h() + DP(10);
	add_subwindow(new BC_CheckBox(x, 
		y, 
		&asset->jpeg_sphere, 
		_("Tag for spherical playback")));

	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

int JPEGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}



