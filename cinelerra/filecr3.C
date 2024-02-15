/*
 * CINELERRA
 * Copyright (C) 2020-2022 Adam Williams <broadcast at earthling dot net>
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
#include "bchash.h"
#include "clip.h"
#include "colormodels.h"
#include "file.h"
#include "filecr3.h"
#include "mutex.h"
#include <string.h>
#include <unistd.h>

#include "libraw/libraw.h"


FileCR3::FileCR3(Asset *asset, File *file)
 : FileList(asset, file, "CR3LIST", ".cr3", FILE_CR3, FILE_CR3_LIST)
{
	reset();
	if(asset->format == FILE_UNKNOWN)
	{
    	asset->format = FILE_CR3;
    }
}

FileCR3::~FileCR3()
{
//printf("FileCR3::~FileCR3\n");
	close_file();
}


FileCR3::FileCR3()
 : FileList()
{
    ids.append(FILE_CR3);
    ids.append(FILE_CR3_LIST);
    has_video = 1;
    has_rd = 1;
}


FileBase* FileCR3::create(File *file)
{
    return new FileCR3(file->asset, file);
}


const char* FileCR3::formattostr(int format)
{
    switch(format)
    {
		case FILE_CR3:
			return CR3_NAME;
			break;
		case FILE_CR3_LIST:
			return CR3_LIST_NAME;
			break;
    }
    return 0;
}

void FileCR3::reset()
{
}

int FileCR3::check_sig(File *file, const uint8_t *test_data)
{
    Asset *asset = file->asset;
//printf("FileCR3::check_sig %d %s\n", __LINE__, asset->path);
// check suffix
    int len = strlen(asset->path);
    if(len > 4)
    {
        if(!strcasecmp(asset->path + len - 4, ".cr3") ||
            !strcasecmp(asset->path + len - 4, ".cr2"))
        {
            return 1;
        }
    }
//printf("FileCR3::check_sig %d %s\n", __LINE__, asset->path);

// check for file list
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{
		char test[10];
		int temp = fread(test, 10, 1, stream);
		fclose(stream);

		if((test[0] == 'C' && test[1] == 'R' && test[2] == '3' && 
			test[3] == 'L' && test[4] == 'I' && test[5] == 'S' && test[6] == 'T') ||
            (test[0] == 'C' && test[1] == 'R' && test[2] == '2' && 
			test[3] == 'L' && test[4] == 'I' && test[5] == 'S' && test[6] == 'T'))
		{
			return 1;
		}
	}
//printf("FileCR3::check_sig %d %s\n", __LINE__, asset->path);

	return 0;
}

int FileCR3::read_frame_header(char *path)
{
    int err = 0;
//printf("FileCR3::read_frame_header %d\n", __LINE__);
    LibRaw* libraw = new LibRaw;
    err = libraw->open_file(path);

    if(err)
    {
        printf("FileCR3::read_frame_header %d path=%s err=%d\n",
            __LINE__,
            path,
            err);
        return 1;
    }

//     printf("FileCR3::read_frame_header %d w=%d h=%d\n",
//         __LINE__,
//         libraw->imgdata.sizes.width,
//         libraw->imgdata.sizes.height);

	asset->video_data = 1;
	asset->layers = 1;
	asset->width = libraw->imgdata.sizes.width;
    asset->height = libraw->imgdata.sizes.height;


    delete libraw;
	return err;
}






int FileCR3::read_frame(VFrame *frame, char *path)
{
    int err = 0;
    LibRaw* libraw = new LibRaw;
    cmodel_init();

    err = libraw->open_file(path);
    if(err)
    {
        printf("FileCR3::read_frame %d path=%s err=%d\n",
            __LINE__,
            path,
            err);
        return 1;
    }



    libraw->unpack();

// printf("FileCR3::read_frame %d %s\n",
// __LINE__,
// path);
// printf("FileCR3::read_frame %d: colormodel %d %p\n",
// __LINE__,
// frame->get_color_model(),
// cmodel_yuv_table);
    libraw->imgdata.params.use_camera_wb = 1;
    libraw->dcraw_process();

    int width;
    int height;
    int colors;
    int bps;
    libraw->get_mem_image_format(&width, &height, &colors, &bps);
//     printf("FileCR3::read_frame %d w=%d h=%d colors=%d bps=%d\n",
//         __LINE__,
//         width, 
//         height, 
//         colors, 
//         bps);
//     printf("FileCR3::read_frame %d %p %p %p %p %d %d %d %d\n",
//         __LINE__,
//         libraw->imgdata.image[0],
//         libraw->imgdata.image[1],
//         libraw->imgdata.image[2],
//         libraw->imgdata.image[3],
//         width,
//         height,
//         colors,
//         bps);
//     for(int i = 0; i < 16; i++)
//     {
//         printf("%04x ", (uint16_t)libraw->imgdata.image[0][i]);
//     }
//     printf("\n");

// convert from 16 bit RGB 4 channels to the output

#define CONVERT_HEAD(type) \
for(int i = 0; i < height; i++) \
{ \
    type *output = (type*)frame->get_rows()[i]; \
    uint16_t *input = libraw->imgdata.image[0] + i * width * 4; \
    for(int j = 0; j < width; j++) \
    { \

#define CONVERT_TAIL \
            input++; \
    } \
}

    switch(frame->get_color_model())
    {
// needed for previews
        case BC_RGB888:
            CONVERT_HEAD(uint8_t)
                *output++ = (uint8_t)((*input++) >> 8);
                *output++ = (uint8_t)((*input++) >> 8);
                *output++ = (uint8_t)((*input++) >> 8);
            CONVERT_TAIL
            break;
        case BC_RGBA8888:
            CONVERT_HEAD(uint8_t)
                *output++ = (uint8_t)((*input++) >> 8);
                *output++ = (uint8_t)((*input++) >> 8);
                *output++ = (uint8_t)((*input++) >> 8);
                *output++ = 0xff;
            CONVERT_TAIL
            break;
        case BC_RGB_FLOAT:
            CONVERT_HEAD(float)
                *output++ = (float)(*input++) / 0xffff;
                *output++ = (float)(*input++) / 0xffff;
                *output++ = (float)(*input++) / 0xffff;
            CONVERT_TAIL
            break;
        case BC_RGBA_FLOAT:
            CONVERT_HEAD(float)
                *output++ = (float)(*input++) / 0xffff;
                *output++ = (float)(*input++) / 0xffff;
                *output++ = (float)(*input++) / 0xffff;
                *output++ = 1.0;
            CONVERT_TAIL
            break;
        case BC_YUV888:
            CONVERT_HEAD(uint8_t)
                int y, u, v;
                int r = (uint8_t)((*input++) >> 8);
                int g = (uint8_t)((*input++) >> 8);
                int b = (uint8_t)((*input++) >> 8);
                RGB_TO_YUV(y, u, v, r, g, b);
                *output++ = y;
                *output++ = u;
                *output++ = v;
            CONVERT_TAIL
            break;
        case BC_YUVA8888:
            CONVERT_HEAD(uint8_t)
                int y, u, v;
                int r = (uint8_t)((*input++) >> 8);
                int g = (uint8_t)((*input++) >> 8);
                int b = (uint8_t)((*input++) >> 8);
                RGB_TO_YUV(y, u, v, r, g, b);
                *output++ = y;
                *output++ = u;
                *output++ = v;
                *output++ = 0xff;
            CONVERT_TAIL
            break;
        default:
            printf("FileCR3::read_frame %d: unsupported output colormodel %d\n",
                __LINE__,
                frame->get_color_model());
    }

    delete libraw;
    
	return 0;
}

// int FileCR3::colormodel_supported(int colormodel)
// {
// 	if(colormodel == BC_RGB_FLOAT ||
// 		colormodel == BC_RGBA_FLOAT)
// 		return colormodel;
// 	return BC_RGB_FLOAT;
// }
// 

// Be sure to add a line to File::get_best_colormodel
int FileCR3::get_best_colormodel(Asset *asset, int driver)
{
//printf("FileCR3::get_best_colormodel %d\n", __LINE__);
	return BC_RGB_FLOAT;
}


int FileCR3::use_path()
{
	return 1;
}





