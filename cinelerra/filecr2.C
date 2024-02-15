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
#include "bchash.h"
#include "clip.h"
#include "colormodels.h"
#include "file.h"
#include "filecr2.h"
#include "mutex.h"
#include <string.h>
#include <unistd.h>


extern "C"
{
extern char dcraw_info[1024];
extern float **dcraw_data;
extern int dcraw_alpha;
extern float dcraw_matrix[9];
int dcraw_main (int argc, const char **argv);
}


FileCR2::FileCR2(Asset *asset, File *file)
 : FileList(asset, file, "CR2LIST", ".cr2", FILE_CR2, FILE_CR2_LIST)
{
	reset();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_CR2;
}

FileCR2::~FileCR2()
{
//printf("FileCR2::~FileCR2\n");
	close_file();
}


FileCR2::FileCR2()
 : FileList()
{
    ids.append(FILE_CR2);
    ids.append(FILE_CR2_LIST);
    has_video = 1;
    has_rd = 1;
}

FileBase* FileCR2::create(File *file)
{
    return new FileCR2(file->asset, file);
}


const char* FileCR2::formattostr(int format)
{
    switch(format)
    {
		case FILE_CR2:
			return CR2_NAME;
			break;
		case FILE_CR2_LIST:
			return CR2_LIST_NAME;
			break;
    }
    return 0;
}


void FileCR2::reset()
{
}

int FileCR2::check_sig(File *file, const uint8_t *test_data)
{
    Asset *asset = file->asset;
    int len = strlen(asset->path);
    if(len > 4)
    {
        if(!strcasecmp(asset->path + len - 4, ".cr2"))
        {
            return 1;
        }
    }

// check for file list
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{
		char test[10];
		int temp = fread(test, 10, 1, stream);
		fclose(stream);

		if(test[0] == 'C' && test[1] == 'R' && test[2] == '2' && 
			test[3] == 'L' && test[4] == 'I' && test[5] == 'S' && test[6] == 'T')
		{
//printf("FileCR2::check_sig %d\n", __LINE__);
			return 1;
		}
	}

//printf("FileCR2::check_sig %d\n", __LINE__);

//  use library
	char string[BCTEXTLEN];
	int argc = 3;

	strcpy(string, asset->path);

	const char *argv[4] =
	{
		"dcraw",
		"-i",
		string,
		0
	};

	int result = dcraw_main(argc, argv);

//printf("FileCR2::check_sig %d %d\n", __LINE__, result);

	return !result;
}

// int FileCR2::open_file(int rd, int wr)
// {
// 
// 	int argc = 3;
// 	const char *argv[4] = 
// 	{
// 		"dcraw",
// 		"-i",
// 		asset->path,
// 		0
// 	};
// 
// 	int result = dcraw_main(argc, argv);
// 	if(!result) format_to_asset();
// 
// 	return result;
// }

int FileCR2::read_frame_header(char *path)
{
	int argc = 3;
//printf("FileCR2::read_frame_header %d\n", __LINE__);
	const char *argv[4] = 
	{
		"dcraw",
		"-i",
		path,
		0
	};

	int result = dcraw_main(argc, argv);
	if(!result) format_to_asset();

//printf("FileCR2::read_frame_header %d %d\n", __LINE__, result);
	return result;
}




// int FileCR2::close_file()
// {
// 	return 0;
// }
// 
void FileCR2::format_to_asset()
{
	asset->video_data = 1;
	asset->layers = 1;
	sscanf(dcraw_info, "%d %d", &asset->width, &asset->height);
}


int FileCR2::read_frame(VFrame *frame, char *path)
{
printf("FileCR2::read_frame %d cmodel=%d\n", __LINE__, frame->get_color_model());
    VFrame *frame_ptr = frame;

// decode directly to shared memory with alpha
	if(frame->get_color_model() == BC_RGBA_FLOAT)
		dcraw_alpha = 1;
    else
        dcraw_alpha = 0;

// get a shared memory temporary to decode into
    if(frame->get_color_model() != BC_RGB_FLOAT &&
        frame->get_color_model() != BC_RGBA_FLOAT)
    {
        frame_ptr = file->get_read_temp(BC_RGB_FLOAT, 
            frame->get_w() * 3 * sizeof(float), 
            frame->get_w(), 
            frame->get_h());
    }
//printf("FileCR2::read_frame %d frame_ptr=%p\n", __LINE__, frame_ptr);

// Want to disable interpolation if an interpolation plugin is on, but
// this is impractical because of the amount of caching.  The interpolation
// could not respond to a change in the plugin settings and it could not
// reload the frame after the plugin was added.  Also, since an 8 bit
// PBuffer would be required, it could never have enough resolution.
//	int interpolate = 0;
// 	if(!strcmp(frame->get_next_effect(), "Interpolate Pixels"))
// 		interpolate = 0;


// printf("FileCR2::read_frame %d\n", interpolate);
// frame->dump_stacks();
// output to stdout
	int argc = 0;
	char *argv[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	argv[argc++] = (char*)"dcraw";
// write to stdout
	argv[argc++] = (char*)"-c";
// no rotation
	argv[argc++] = (char*)"-j";

// printf("FileCR2::read_frame %d interpolate=%d white_balance=%d\n", 
// __LINE__,
// file->interpolate_raw,
// file->white_balance_raw);

// Use camera white balance.  
// Before 2006, DCraw had no Canon white balance.
// In 2006 DCraw seems to support Canon white balance.
// Still no gamma support.
// Need to toggle this in preferences because it defeats dark frame subtraction.
//	if(file->white_balance_raw)
//		argv[argc++] = (char*)"-w";

// always white balance if interpolating.  Interpolating alone doesn't work
	if(file->interpolate_raw)
	{
		argv[argc++] = (char*)"-w";
	}

	if(!file->interpolate_raw)
	{
// Trying to do everything but interpolate doesn't work because convert_to_rgb
// doesn't work with bayer patterns.
// Use document mode and hack dcraw to apply white balance in the write_ function.
		argv[argc++] = (char*)"-d";
	}

//printf("FileCR2::read_frame %d %s\n", __LINE__, path);
	argv[argc++] = path;

	dcraw_data = (float**)frame_ptr->get_rows();

//Timer timer;
	int result = dcraw_main(argc, (const char**) argv);

// This was only used by the bayer interpolate plugin, which itself created
// too much complexity to use effectively.
// It required bypassing the cache any time a plugin parameter changed 
// to store the color matrix from dcraw in the frame stack along with the new
// plugin parameters.  The cache couldn't know if a parameter in the stack came
// from dcraw or a plugin & replace it.
	char string[BCTEXTLEN];
	sprintf(string, 
		"%f %f %f %f %f %f %f %f %f",
		dcraw_matrix[0],
		dcraw_matrix[1],
		dcraw_matrix[2],
		dcraw_matrix[3],
		dcraw_matrix[4],
		dcraw_matrix[5],
		dcraw_matrix[6],
		dcraw_matrix[7],
		dcraw_matrix[8]);


	frame_ptr->get_params()->update("DCRAW_MATRIX", string);

// float *ptr = (float*)frame->get_rows()[1346];
// printf("FileCR2::read_frame %d %f %f %f\n", 
// __LINE__, 
// ptr[3046 * 3 + 0],
// ptr[3046 * 3 + 1],
// ptr[3046 * 3 + 2]);
//frame->dump_params();

	return 0;
}

// int FileCR2::colormodel_supported(int colormodel)
// {
// 	if(colormodel == BC_RGB_FLOAT ||
// 		colormodel == BC_RGBA_FLOAT)
// 		return colormodel;
// 	return BC_RGB_FLOAT;
// }
// 

// Be sure to add a line to File::get_best_colormodel
int FileCR2::get_best_colormodel(Asset *asset, int driver)
{
//printf("FileCR2::get_best_colormodel %d\n", __LINE__);
	return BC_RGB_FLOAT;
}

// int64_t FileCR2::get_memory_usage()
// {
// 	int64_t result = asset->width * asset->height * sizeof(float) * 3;
// //printf("FileCR2::get_memory_usage %d %lld\n", __LINE__, result);
// 	return result;
// }


int FileCR2::use_path()
{
	return 1;
}





