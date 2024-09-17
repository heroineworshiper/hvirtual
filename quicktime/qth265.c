/*
 * Quicktime 4 Linux
 * Copyright (C) 1997-2024 Adam Williams <broadcast at earthling dot net>
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







#include "avcodec.h"
#include "colormodels.h"
#include "funcprotos.h"
#include <pthread.h>
#include "qtffmpeg.h"
#include "quicktime.h"
#include <string.h>
#include "workarounds.h"
#include "x265.h"


// encoding H265 directly with ffmpeg:
// ffmpeg -i test.mp4 -c:v libx265 -crf 28 -tag:v hvc1 test2.mp4


typedef struct
{
// Encoder side
	int fix_bitrate;
    int bitrate;
    int quantizer;
    int encode_cmodel;
	int encode_initialized;

// Temporary storage for color conversions
	uint8_t *temp_frame;
// Storage of compressed data
	unsigned char *work_buffer;
// Amount of data in work_buffer
	int buffer_size;
// allocation of work_buffer
    int allocated;
// Set by flush to get the header
	int header_only;
    const x265_api *api;
    x265_param *param;
    x265_picture *pic_in;
    x265_picture *pic_out;
    x265_encoder *encoder;

// Decoder side
	quicktime_ffmpeg_t *decoder;

} quicktime_h265_codec_t;

static pthread_mutex_t h265_lock = PTHREAD_MUTEX_INITIALIZER;















static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_h265_codec_t *codec;
	int i;


	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->encode_initialized)
	{
		pthread_mutex_lock(&h265_lock);

        x265_encoder_close(codec->encoder);
        x265_param_free(codec->param);
        x265_picture_free(codec->pic_in);
        x265_picture_free(codec->pic_out);

		pthread_mutex_unlock(&h265_lock);
	}

	

	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->work_buffer) free(codec->work_buffer);
	if(codec->decoder) quicktime_delete_ffmpeg(codec->decoder);


	free(codec);
	return 0;
}



static void common_encode(quicktime_t *file, 
	int track,
	int frames,
	x265_nal *nals,
	int nalcount,
	x265_picture *pic_out)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_h265_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);


    if(frames > 0)
    {
        codec->buffer_size = 0;
        
        int i, j;
  		for(i = 0; i < nalcount; i++)
		{
            int size = nals[i].sizeBytes;
            uint8_t *data = nals[i].payload;

// printf("common_encode %d i=%d nalcount=%d\n", __LINE__, i, nalcount);
// for(j = 0; j < size && j < 16; j++)
// {
//     printf("%02x ", data[j]);
// }
// printf("\n");

            if(!codec->work_buffer)
            {
                codec->work_buffer = malloc(size);
                codec->allocated = size;
            }
            
            if(codec->allocated < codec->buffer_size + size)
            {
                int new_allocated = codec->buffer_size + size;
                uint8_t *new_buffer = malloc(new_allocated);
                memcpy(new_buffer, codec->work_buffer, codec->buffer_size);
                free(codec->work_buffer);
                codec->work_buffer = new_buffer;
                codec->allocated = new_allocated;
            }
            
// add size code
            int modified_size = size - 4;
            uint8_t *modified_data = data + 4;
            codec->work_buffer[codec->buffer_size++] = (modified_size >> 24) & 0xff;
            codec->work_buffer[codec->buffer_size++] = (modified_size >> 16) & 0xff;
            codec->work_buffer[codec->buffer_size++] = (modified_size >> 8) & 0xff;
            codec->work_buffer[codec->buffer_size++] = modified_size & 0xff;
            
            memcpy(codec->work_buffer + codec->buffer_size, 
                modified_data,
                modified_size);
            codec->buffer_size += modified_size;

            
            
            int is_keyframe = 0;
            if(pic_out->sliceType == X265_TYPE_IDR ||
                pic_out->sliceType == X265_TYPE_I)
            {
                is_keyframe = 1;
            }
//             printf("common_encode %d sliceType=%d\n",
//                 __LINE__,
//                 pic_out->sliceType);
            
            
		    if(codec->buffer_size)
		    {
    //printf("encode %d buffer_size=%d\n", __LINE__, codec->buffer_size);
			    quicktime_atom_t chunk_atom;
			    quicktime_write_chunk_header(file, trak, &chunk_atom);
			    int result = !quicktime_write_data(file, 
				    codec->work_buffer, 
				    codec->buffer_size);
			    quicktime_write_chunk_footer(file, 
				    trak,
				    vtrack->current_chunk,
				    &chunk_atom, 
				    1);

		        if(is_keyframe)
		        {
			        quicktime_insert_keyframe(file, 
				        vtrack->current_chunk - 1, 
				        track);
		        }
		        vtrack->current_chunk++;
		    }
        }
    }

}








static void flush(quicktime_t *file, int track)
{
	quicktime_video_map_t *track_map = &(file->vtracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_h265_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);

	pthread_mutex_lock(&h265_lock);

//    printf("flush %d\n", __LINE__);
	if(codec->encode_initialized)
	{
        x265_nal *nals;
        uint32_t nalcount;
        int frames;
		while((frames = codec->api->encoder_encode(codec->encoder, 
            &nals, 
            &nalcount, 
            0, 
            codec->pic_out)) > 0)
		{
			common_encode(file, track, frames, nals, nalcount, codec->pic_out);
		}
	}

	pthread_mutex_unlock(&h265_lock);

}




static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;


//printf("encode %d\n", __LINE__);

	int64_t offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_h265_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);
	quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;




	pthread_mutex_lock(&h265_lock);

	if(!codec->encode_initialized)
	{
		codec->encode_initialized = 1;

// only using 8 bits for now, because compression looks like shit either way
        codec->api = x265_api_get(8);
        codec->param = codec->api->param_alloc();
        codec->api->param_default_preset(codec->param, 0, 0);
        codec->param->sourceWidth = width;
        codec->param->sourceHeight = height;
// TODO: color subsampling (CSP)
        codec->param->fpsDenom = quicktime_frame_rate_d(file, track);
        codec->param->fpsNum = quicktime_frame_rate_n(file, track);
// no B frames
        codec->param->bframes = 0;
        
        if(codec->fix_bitrate)
        {
// rate control mode (X265_RC_METHODS)
            codec->param->rc.rateControlMode = X265_RC_ABR;
// bitrate for Average BitRate (X265_RC_ABR) rate control
// kilobits per second
            codec->param->rc.bitrate = codec->bitrate;
        }
        else
        {
// rate control mode (X265_RC_METHODS)
            codec->param->rc.rateControlMode = X265_RC_CQP;
// quantization for constant Q mode (X265_RC_CQP)
            codec->param->rc.qp = codec->quantizer;
        }
        



		if(file->cpus == 1)
		{
			codec->param->frameNumThreads = 1;
		}

       codec->param->bEmitHDRSEI = 0;
       codec->param->bEmitHRDSEI = 0;
       codec->param->bEmitInfoSEI = 0;


// 		printf("encode %d fix_bitrate=%d\n", __LINE__, codec->fix_bitrate);
// 		printf("encode %d bitrate=%d\n", __LINE__, codec->param->rc.bitrate);
// 		printf("encode %d q=%d\n", __LINE__, codec->param->rc.qp);
// 		printf("encode %d threads=%d\n", __LINE__, codec->param->frameNumThreads);
//      printf("encode %d internalCsp=%d\n", __LINE__, codec->param->internalCsp);



		codec->encoder = codec->api->encoder_open(codec->param);
        codec->api->encoder_parameters(codec->encoder, codec->param);
        x265_nal *nals = 0;
        uint32_t nalcount = 0;
        codec->api->encoder_headers(codec->encoder, &nals, &nalcount);


        int i, j;
        unsigned char header[4096];
	    int header_size = 0;
        
// NALs are heavily reformatted
// libavformat/hevc.c: hvcc_write
        const uint8_t hvcc_data[] = 
        {
            0x01, // configurationVersion
            0x01, // general_profile_space...
            0x60, 0x00, 0x00, 0x00, // general_profile_compatibility_flags
            0x90, 0x00, 0x00, 0x00, 0x00, 0x00, // general_constraint_indicator_flags
            0x78, // general_level_idc
            0xf0, 0x00, // min_spatial_segmentation_idc
            0xfc, // parallelismType
            0xfd, // chromaFormat
            0xf8, // bitDepthLumaMinus8
            0xf8, // bitDepthChromaMinus8
            0x00, 0x00, // avgFrameRate
            0x0f // constantFrameRate...
        };
        memcpy(header, hvcc_data, sizeof(hvcc_data));
        header_size += sizeof(hvcc_data);

// numOfArrays
        header[header_size++] = nalcount;
        
        for(i = 0; i < nalcount; i++)
        {
            if(header_size + nals[i].sizeBytes > sizeof(header))
            {
                printf("encode %d header overflowed\n", __LINE__);
                break;
            }

// array_completeness, NAL_unit_type
            header[header_size++] = 0xa0 | i;

// numNalus
            header[header_size++] = 0;
            header[header_size++] = 1;

// nalUnitLength
            int modified_size = nals[i].sizeBytes - 4;
            uint8_t *modified_payload = nals[i].payload + 4;
            header[header_size++] = (modified_size >> 8) & 0xff;
            header[header_size++] = modified_size & 0xff;

// printf("\nencode %d i=%d nalcount=%d\n", __LINE__, i, nalcount);
// for(j = 0; j < nals[i].sizeBytes; j++)
// {
//     printf(" %02x", nals[i].payload[j]);
// }
// printf("\n");

            memcpy(header + header_size, 
                modified_payload, 
                modified_size);
            header_size += modified_size;
        }
        
//         int total_size = 0;
//         for(i = 0; i < nalcount; i++)
//         {
//             total_size += nals[i].sizeBytes;
//         }
//         printf("encode %d header size=%d\n", __LINE__,  total_size);
        
        quicktime_set_avcc_header(avcc,
		  	header, 
		  	header_size,
            1);
        
        codec->pic_in = x265_picture_alloc();
        codec->pic_out = x265_picture_alloc();
        codec->api->picture_init(codec->param, codec->pic_in);
        codec->api->picture_init(codec->param, codec->pic_out);
	}
    
    
    
    
    
    
    
    codec->pic_in->colorSpace = codec->param->internalCsp;
    codec->pic_in->bitDepth = 8;
    codec->pic_in->framesize = width * height * 3 / 2;
    codec->pic_in->height = height;
    codec->pic_in->stride[0] = width;
    codec->pic_in->stride[1] = width / 2;
    codec->pic_in->stride[2] = width / 2;


	if(file->color_model == BC_YUV420P)
	{
        codec->pic_in->planes[0] = row_pointers[0];
        codec->pic_in->planes[1] = row_pointers[1];
        codec->pic_in->planes[2] = row_pointers[2];
	}
	else
	{
        if(!codec->temp_frame)
        {
            codec->temp_frame = malloc(width * height * 3 / 2);
        }

        codec->pic_in->planes[0] = codec->temp_frame;
        codec->pic_in->planes[1] = codec->temp_frame + width * height;
        codec->pic_in->planes[2] = codec->temp_frame + width * height + width * height / 4;

        int encoder_cmodel = codec->encode_cmodel;
		cmodel_transfer(0, /* Leave NULL if non existent */
			row_pointers,
			codec->pic_in->planes[0], /* Leave NULL if non existent */
			codec->pic_in->planes[1],
			codec->pic_in->planes[2],
            cmodel_components(encoder_cmodel) == 4 ? codec->pic_in->planes[3] : 0,
			row_pointers[0], /* Leave NULL if non existent */
			row_pointers[1],
			row_pointers[2],
            cmodel_components(file->color_model) == 4 ? row_pointers[3] : 0,
			0,        /* Dimensions to capture from input frame */
			0, 
			width, 
			height,
			0,       /* Dimensions to project on output frame */
			0, 
			width, 
			height,
			file->color_model, 
			encoder_cmodel,
			0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			width,       /* For planar use the luma rowspan */
			width);
		
	}


    
    x265_nal *nals;
    uint32_t nalcount;
    int frames = codec->api->encoder_encode(codec->encoder, 
        &nals, 
        &nalcount, 
        codec->pic_in, 
        codec->pic_out);

// errors
    if(frames <= 0 && nalcount > 0)
    {
        printf("encode %d: frames=%d nalcount=%d\n", 
            __LINE__, 
            frames,
            nalcount);
    }
    
    if(frames > 1)
    {
        printf("encode %d: frames=%d\n", __LINE__, frames);
    }




	common_encode(file, track, frames, nals, nalcount, codec->pic_out);

	pthread_mutex_unlock(&h265_lock);


	return result;
}




static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_h265_codec_t *codec = codec_base->priv;
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int w_16 = quicktime_quantize16(width);
	int h_16 = quicktime_quantize16(height);

// translate fourcc to ffmpeg
    int codec_id = AV_CODEC_ID_H265;
    if(quicktime_match_32((char*)codec_base->fourcc, (char*)QUICKTIME_VP09))
        codec_id = AV_CODEC_ID_VP9;

	if(!codec->decoder) codec->decoder = quicktime_new_ffmpeg(
		file->cpus,
        file->use_hw,
		1,
		codec_id,
		width,
		height,
		stsd_table);


	if(codec->decoder) return quicktime_ffmpeg_decode(codec->decoder,
		file, 
		row_pointers, 
		track);

	return 1;
}



static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_codec_t *codec = (quicktime_codec_t*)vtrack->codec;
	return (colormodel == BC_YUV420P);
}

static int writes_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_YUV420P);
}

static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	char *compressor = quicktime_compressor(vtrack->track);

	if(quicktime_match_32(compressor, QUICKTIME_H265))
	{
		quicktime_h265_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
		if(!strcasecmp(key, "h265_bitrate"))
		{
			codec->bitrate = *(int*)value / 1000;
		}
		else
		if(!strcasecmp(key, "h265_quantizer"))
		{
			codec->quantizer = *(int*)value;
		}
		else
		if(!strcasecmp(key, "h265_cmodel"))
		{
			codec->encode_cmodel = *(int*)value;
		}
		else
		if(!strcasecmp(key, "h265_fix_bitrate"))
		{
			codec->fix_bitrate = *(int*)value;
		}
	}
}

static quicktime_h265_codec_t* init_common(quicktime_video_map_t *vtrack, 
	char *compressor,
	char *title,
	char *description)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

	codec_base->priv = calloc(1, sizeof(quicktime_h265_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->flush = flush;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->set_parameter = set_parameter;
	codec_base->fourcc = compressor;
	codec_base->title = title;
	codec_base->desc = description;


	quicktime_h265_codec_t *codec = (quicktime_h265_codec_t*)codec_base->priv;
    codec->encode_cmodel = BC_YUV420P;
	return codec;
}


void quicktime_init_codec_h265(quicktime_video_map_t *vtrack)
{
    quicktime_h265_codec_t *result = init_common(vtrack,
        QUICKTIME_H265,
        "H.265",
        "H.265");
}

void quicktime_init_codec_hev1(quicktime_video_map_t *vtrack)
{
    quicktime_h265_codec_t *result = init_common(vtrack,
        QUICKTIME_HEV1,
        "H.265",
        "H.265");
}

void quicktime_init_codec_vp09(quicktime_video_map_t *vtrack)
{
    quicktime_h265_codec_t *result = init_common(vtrack,
        QUICKTIME_VP09,
        "VP9",
        "VP9");
}




