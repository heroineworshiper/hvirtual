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
#include "x264.h"

// This generates our own header using fixed parameters
//#define MANUAL_HEADER


typedef struct
{
// Encoder side
	x264_t *encoder[FIELDS];
	x264_picture_t *pic[FIELDS];
	x264_param_t param;
	int fix_bitrate;
    int encode_cmodel;

	int encode_initialized[FIELDS];

// Temporary storage for color conversions
	char *temp_frame;
// Storage of compressed data
	unsigned char *work_buffer;
// Amount of data in work_buffer
	int buffer_size;
	int total_fields;
// Set by flush to get the header
	int header_only;

// Decoder side
	quicktime_ffmpeg_t *decoder;

} quicktime_h264_codec_t;

static pthread_mutex_t h264_lock = PTHREAD_MUTEX_INITIALIZER;












// Direct copy routines
int quicktime_h264_is_key(unsigned char *data, long size, char *codec_id)
{
	
}




static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_h264_codec_t *codec;
	int i;


	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	for(i = 0; i < codec->total_fields; i++)
	{
		if(codec->encode_initialized[i])
		{
			pthread_mutex_lock(&h264_lock);


			if(codec->pic[i])
			{
				x264_picture_clean(codec->pic[i]);
				free(codec->pic[i]);
			}

			if(codec->encoder[i])
			{
				x264_encoder_close(codec->encoder[i]);
			}

			pthread_mutex_unlock(&h264_lock);
		}
	}

	

	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->work_buffer) free(codec->work_buffer);
	if(codec->decoder) quicktime_delete_ffmpeg(codec->decoder);


	free(codec);
	return 0;
}




static void common_encode(quicktime_t *file, 
	int track,
	int size,
	x264_nal_t *nals,
	int nnal,
	x264_picture_t *pic_out)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_h264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;
	int i;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);
	int w_2 = quicktime_quantize2(width);
// ffmpeg interprets the codec height as the presentation height
	int h_2 = quicktime_quantize2(height);
	unsigned char header[1024];
	int header_size = 0;
	int got_pps = 0;
	int got_sps = 0;

	codec->buffer_size = 0;
	if(size > 0)
	{
		for(i = 0; i < nnal; i++)
		{
	// Synthesize header.
			if(!avcc->data_size)
			{
				if(header_size < 6)
				{
					header[header_size++] = 0x01;
					header[header_size++] = 0x4d;
					header[header_size++] = 0x40;
					header[header_size++] = 0x1f;
					header[header_size++] = 0xff;
					header[header_size++] = 0xe1;
				}

				unsigned char *ptr = nals[i].p_payload;
				int nal_type = (ptr[4] & 0x1f);
				int avc_size = nals[i].i_payload - 4;

	// Picture parameter or sequence parameter set
				if(nal_type == 0x7 && !got_sps)
				{
					got_sps = 1;
					header[header_size++] = (avc_size & 0xff00) >> 8;
					header[header_size++] = (avc_size & 0xff);
					memcpy(&header[header_size], 
						ptr + 4,
						avc_size);
					header_size += avc_size;
				}
				else
				if(nal_type == 0x8 && !got_pps)
				{
					got_pps = 1;
		// Number of sps nal's.
					header[header_size++] = 0x1;
					header[header_size++] = (avc_size & 0xff00) >> 8;
					header[header_size++] = (avc_size & 0xff);
					memcpy(&header[header_size], 
						ptr + 4,
						avc_size);
					header_size += avc_size;
				}

		// Write header
				if(got_sps && got_pps)
				{
		/*
		* printf("encode %d\n", __LINE__);
		* int j;
		* for(j = 0; j < header_size; j++)
		* {
		* printf("%02x ", header[j]);
		* }
		* printf("\n");
		*/
					quicktime_set_avcc_header(avcc,
		  				header, 
		  				header_size,
                        0);
				}
			}
		}

		for(i = 0; i < nnal; i++)
		{
			size = nals[i].i_payload;
// 			printf("common_encode %d: i=%d size=%d %02x %02x %02x %02x %02x %02x %02x %02x\n",
// 				__LINE__,
// 				i,
// 				nals[i].i_payload,
// 				nals[i].p_payload[0],
// 				nals[i].p_payload[1],
// 				nals[i].p_payload[2],
// 				nals[i].p_payload[3],
// 				nals[i].p_payload[4],
// 				nals[i].p_payload[5],
// 				nals[i].p_payload[6],
// 				nals[i].p_payload[7]);



// Get start of encoded data
			unsigned char *ptr = nals[i].p_payload;
			while(ptr - nals[i].p_payload < nals[i].i_payload && *ptr != 0x1)
			{
				ptr++;
			}
			if(ptr - nals[i].p_payload < nals[i].i_payload) ptr++;
			

// Size of encoded data in NAL
			int avc_start = ptr - nals[i].p_payload;
			int avc_size = size - avc_start;
			
			
			int allocation = w_2 * h_2 * 3;
			if(!codec->work_buffer)
			{
				codec->work_buffer = calloc(1, allocation);
			}

// encode size of NAL
			ptr = codec->work_buffer + codec->buffer_size;
			*ptr++ = (avc_size & 0xff000000) >> 24;
			*ptr++ = (avc_size & 0xff0000) >> 16;
			*ptr++ = (avc_size & 0xff00) >> 8;
			*ptr++ = (avc_size & 0xff);
// store compressed data
			memcpy(ptr,
				nals[i].p_payload + avc_start,
				avc_size);
			codec->buffer_size += avc_size + 4;



			if(size + codec->buffer_size > allocation)
			{
				printf("common_encode %d: overflow size=%d allocation=%d\n",
					__LINE__,
					size,
					allocation);
			}




		}


		int is_keyframe = 0;
		if(pic_out->i_type == X264_TYPE_IDR ||
			pic_out->i_type == X264_TYPE_I)
		{
			is_keyframe = 1;
		}

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
		}

		if(is_keyframe)
		{
			quicktime_insert_keyframe(file, 
				vtrack->current_chunk - 1, 
				track);
		}
		vtrack->current_chunk++;
	}
}


static void flush(quicktime_t *file, int track)
{
	quicktime_video_map_t *track_map = &(file->vtracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_h264_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);
	int w_2 = quicktime_quantize2(width);
// ffmpeg interprets the codec height as the presentation height
	int h_2 = quicktime_quantize2(height);
	int current_field = track_map->current_position % codec->total_fields;
	int i;

	pthread_mutex_lock(&h264_lock);

// this only encodes 1 field
	if(codec->encode_initialized[current_field])
	{
		while(x264_encoder_delayed_frames(codec->encoder[current_field]))
		{
    		x264_picture_t pic_out;
    		x264_nal_t *nals;
			int nnal = 0;
			int size = x264_encoder_encode(codec->encoder[current_field], 
				&nals, 
				&nnal, 
				0, 
				&pic_out);
			codec->buffer_size = 0;

			common_encode(file, track, size, nals, nnal, &pic_out);
		}
	}

	pthread_mutex_unlock(&h264_lock);

/*
 * 	trak->mdia.minf.stbl.stsd.table[0].version = 1;
 * 	trak->mdia.minf.stbl.stsd.table[0].revision = 1;
 */
}




static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{

//printf("encode %d\n", __LINE__);

	int64_t offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_h264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);
	int w_2 = quicktime_quantize2(width);
// ffmpeg interprets the codec height as the presentation height
	int h_2 = quicktime_quantize2(height);
	int i;
	int result = 0;
	int bytes = 0;
	int current_field = vtrack->current_position % codec->total_fields;
	quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;






	pthread_mutex_lock(&h264_lock);

	if(!codec->encode_initialized[current_field])
	{
		codec->encode_initialized[current_field] = 1;

		x264_param_t default_params;
		x264_param_default_preset(&default_params, "medium", NULL);

// make it encode I frames sequentially
		codec->param.i_bframe = 0;
		codec->param.i_width = w_2;
		codec->param.i_height = h_2;
		codec->param.i_fps_num = quicktime_frame_rate_n(file, track);
		codec->param.i_fps_den = quicktime_frame_rate_d(file, track);

	    codec->param.i_csp = X264_CSP_I420;
		codec->param.rc.i_rc_method = X264_RC_CQP;
	    codec->param.b_vfr_input = 0;
	    codec->param.b_repeat_headers = 1;
    	codec->param.b_annexb = 1;
		codec->param.b_sliced_threads = 0;

// Use default quantizer parameters if fixed bitrate
		if(codec->fix_bitrate)
		{
			codec->param.rc.i_rc_method = X264_RC_ABR;
			codec->param.rc.i_qp_constant = default_params.rc.i_qp_constant;
			codec->param.rc.i_qp_min = default_params.rc.i_qp_min;
			codec->param.rc.i_qp_max = default_params.rc.i_qp_max;
		}

// profile restrictions
		if((result = x264_param_apply_profile( &codec->param, "high" )) < 0)
		{
			printf("encode %d result=%d\n", __LINE__, result);
		}


		if(file->cpus > 1)
		{
			codec->param.i_threads = file->cpus;
		}
//codec->param.i_threads = 0;

		printf("encode %d fix_bitrate=%d\n", __LINE__, codec->fix_bitrate);
		printf("encode %d i_bitrate=%d\n", __LINE__, codec->param.rc.i_bitrate);
		printf("encode %d i_qp_constant=%d\n", __LINE__, codec->param.rc.i_qp_constant);
		printf("encode %d i_qp_min=%d\n", __LINE__, codec->param.rc.i_qp_min);
		printf("encode %d i_qp_max=%d\n", __LINE__, codec->param.rc.i_qp_max);
		printf("encode %d i_threads=%d\n", __LINE__, codec->param.i_threads);
		printf("encode %d i_lookahead_threads=%d\n", __LINE__, codec->param.i_lookahead_threads);
		printf("encode %d b_sliced_threads=%d\n", __LINE__, codec->param.b_sliced_threads);
		printf("encode %d i_sync_lookahead=%d\n", __LINE__, codec->param.i_sync_lookahead);
		printf("encode %d i_bframe=%d\n", __LINE__, codec->param.i_bframe);
		printf("encode %d i_frame_packing=%d\n", __LINE__, codec->param.i_frame_packing);


		codec->encoder[current_field] = x264_encoder_open(&codec->param);
		codec->pic[current_field] = calloc(1, sizeof(x264_picture_t));
//printf("encode 1 %d %d\n", codec->param.i_width, codec->param.i_height);
  		x264_picture_alloc(codec->pic[current_field], 
			X264_CSP_I420, 
			codec->param.i_width, 
			codec->param.i_height);
	}






	codec->pic[current_field]->i_type = X264_TYPE_AUTO;
	codec->pic[current_field]->i_qpplus1 = 0;


	if(codec->header_only)
	{
		bzero(codec->pic[current_field]->img.plane[0], w_2 * h_2);
		bzero(codec->pic[current_field]->img.plane[1], w_2 * h_2 / 4);
		bzero(codec->pic[current_field]->img.plane[2], w_2 * h_2 / 4);
	}
	else
	if(file->color_model == BC_YUV420P)
	{
		memcpy(codec->pic[current_field]->img.plane[0], row_pointers[0], w_2 * h_2);
		memcpy(codec->pic[current_field]->img.plane[1], row_pointers[1], w_2 * h_2 / 4);
		memcpy(codec->pic[current_field]->img.plane[2], row_pointers[2], w_2 * h_2 / 4);
	}
	else
	{
//printf("encode 2 %p %p %p\n", codec->pic[current_field]->img.plane[0], codec->pic[current_field]->img.plane[1], codec->pic[current_field]->img.plane[2]);
		int encoder_cmodel = codec->encode_cmodel;
        cmodel_transfer(0, /* Leave NULL if non existent */
			row_pointers,
			codec->pic[current_field]->img.plane[0], /* Leave NULL if non existent */
			codec->pic[current_field]->img.plane[1],
			codec->pic[current_field]->img.plane[2],
            cmodel_components(encoder_cmodel) == 4 ? codec->pic[current_field]->img.plane[3] : 0,
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
			codec->pic[current_field]->img.i_stride[0]);
		
	}












    x264_picture_t pic_out;
    x264_nal_t *nals;
	int nnal = 0;
	int size = 0;

// printf("encode current_position=%lld current_chunk=%lld\n", 
// vtrack->current_position,
// vtrack->current_chunk);

	size = x264_encoder_encode(codec->encoder[current_field], 
		&nals, 
		&nnal, 
		codec->pic[current_field], 
		&pic_out);

	common_encode(file, track, size, nals, nnal, &pic_out);

//printf("encode %d nnal=%d\n", __LINE__, nnal);
//printf("encode %d size=%d nnal=%d\n", __LINE__, size, nnal);
//printf("encode %d\n", __LINE__);

	pthread_mutex_unlock(&h264_lock);

//	flush(file, track);


	return result;
}




static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_h264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int w_16 = quicktime_quantize16(width);
	int h_16 = quicktime_quantize16(height);


	if(!codec->decoder) codec->decoder = quicktime_new_ffmpeg(
		file->cpus,
        file->use_hw,
		codec->total_fields,
		AV_CODEC_ID_H264,
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

	if(quicktime_match_32(compressor, QUICKTIME_H264) ||
		quicktime_match_32(compressor, QUICKTIME_HV64))
	{
		quicktime_h264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
		if(!strcasecmp(key, "h264_bitrate"))
		{

			if(quicktime_match_32(compressor, QUICKTIME_H264))
			{
				codec->param.rc.i_bitrate = *(int*)value / 1000;
			}
			else
			{
				codec->param.rc.i_bitrate = *(int*)value / 2 / 1000;
			}

//printf("set_parameter %d codec=%p h264_bitrate=%d\n", __LINE__, codec, *(int*)value);
		}
		else
		if(!strcasecmp(key, "h264_quantizer"))
		{
			codec->param.rc.i_qp_constant = 
				codec->param.rc.i_qp_min = 
				codec->param.rc.i_qp_max = *(int*)value;
//printf("set_parameter %d h264_quantizer=%d\n", __LINE__, *(int*)value);
		}
		else
		if(!strcasecmp(key, "h264_cmodel"))
		{
			codec->encode_cmodel = *(int*)value;
		}
		else
		if(!strcasecmp(key, "h264_fix_bitrate"))
		{
			codec->fix_bitrate = *(int*)value;
//printf("set_parameter %d fix_bitrate=%d\n", __LINE__, *(int*)value);
		}
	}
}

static quicktime_h264_codec_t* init_common(quicktime_video_map_t *vtrack, 
	char *compressor,
	char *title,
	char *description)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_h264_codec_t *codec;

	codec_base->priv = calloc(1, sizeof(quicktime_h264_codec_t));
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


	codec = (quicktime_h264_codec_t*)codec_base->priv;
//	x264_param_default(&codec->param);
	x264_param_default_preset(&codec->param, "medium", NULL);
    codec->encode_cmodel = BC_YUV420P;
	return codec;
}


void quicktime_init_codec_h264(quicktime_video_map_t *vtrack)
{
    quicktime_h264_codec_t *result = init_common(vtrack,
        QUICKTIME_H264,
        "H.264",
        "H.264");
	result->total_fields = 1;
}


// field based H.264
void quicktime_init_codec_hv64(quicktime_video_map_t *vtrack)
{
	quicktime_h264_codec_t *result = init_common(vtrack, 
		QUICKTIME_HV64,
		"Dual H.264",
		"H.264 with two streams alternating every other frame.  (Not standardized)");
	result->total_fields = 2;
}




