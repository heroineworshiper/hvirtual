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

// For the working version of x264
#define GOOD_VERSION

typedef struct
{
// Encoder side
	x264_t *encoder[FIELDS];
	x264_picture_t *pic[FIELDS];
	x264_param_t param;

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



static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
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
	int is_keyframe = 0;
	int current_field = vtrack->current_position % codec->total_fields;
	quicktime_atom_t chunk_atom;
	unsigned char header[1024];
	int header_size = 0;
	int got_pps = 0;
	int got_sps = 0;
	quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;






	pthread_mutex_lock(&h264_lock);

	if(!codec->encode_initialized[current_field])
	{
		codec->encode_initialized[current_field] = 1;
		codec->param.i_width = w_2;
		codec->param.i_height = h_2;
		codec->param.i_fps_num = quicktime_frame_rate_n(file, track);
		codec->param.i_fps_den = quicktime_frame_rate_d(file, track);

#ifndef GOOD_VERSION
		codec->param.rc.i_rc_method = X264_RC_CQP;
#endif

// Reset quantizer if fixed bitrate
		x264_param_t default_params;
		x264_param_default(&default_params);
#ifdef GOOD_VERSION
		if(codec->param.rc.b_cbr)
#else
		if(codec->param.rc.i_qp_constant)
#endif
		{
			codec->param.rc.i_qp_constant = default_params.rc.i_qp_constant;
			codec->param.rc.i_qp_min = default_params.rc.i_qp_min;
			codec->param.rc.i_qp_max = default_params.rc.i_qp_max;
		}


		if(file->cpus > 1)
		{
			codec->param.i_threads = file->cpus;
		}

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
		cmodel_transfer(0, /* Leave NULL if non existent */
			row_pointers,
			codec->pic[current_field]->img.plane[0], /* Leave NULL if non existent */
			codec->pic[current_field]->img.plane[1],
			codec->pic[current_field]->img.plane[2],
			row_pointers[0], /* Leave NULL if non existent */
			row_pointers[1],
			row_pointers[2],
			0,        /* Dimensions to capture from input frame */
			0, 
			width, 
			height,
			0,       /* Dimensions to project on output frame */
			0, 
			width, 
			height,
			file->color_model, 
			BC_YUV420P,
			0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			width,       /* For planar use the luma rowspan */
			codec->pic[current_field]->img.i_stride[0]);
		
	}












    x264_picture_t pic_out;
    x264_nal_t *nals;
	int nnal = 0;
	do
	{
		x264_encoder_encode(codec->encoder[current_field], 
			&nals, 
			&nnal, 
			codec->pic[current_field], 
			&pic_out);
//printf("encode %d nnal=%d\n", __LINE__, nnal);
	} while(codec->header_only && !nnal);
	int allocation = w_2 * h_2 * 3;
	if(!codec->work_buffer)
	{
		codec->work_buffer = calloc(1, allocation);
	}

	codec->buffer_size = 0;
//printf("encode %d nnal=%d\n", __LINE__, nnal);
	for(i = 0; i < nnal; i++)
	{
		int size_return = 0;
		int size = x264_nal_encode(codec->work_buffer + codec->buffer_size, 
			&size_return, 
			1, 
			nals + i);
		unsigned char *ptr = codec->work_buffer + codec->buffer_size;

//printf("encode %d size=%d\n", __LINE__, size);
		if(size > 0)
		{
			if(size + codec->buffer_size > allocation)
			{
				printf("qth264.c %d: overflow size=%d allocation=%d\n",
					__LINE__,
					size,
					allocation);
			}

// Size of NAL for avc
			uint64_t avc_size = size - 4;

// Synthesize header.
// Hopefully all the parameter set NAL's are present in the first frame.
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

				int nal_type = (ptr[4] & 0x1f);
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
		  				header_size);
				}
			}


// Convert to avc nal
			*ptr++ = (avc_size & 0xff000000) >> 24;
			*ptr++ = (avc_size & 0xff0000) >> 16;
			*ptr++ = (avc_size & 0xff00) >> 8;
			*ptr++ = (avc_size & 0xff);
			codec->buffer_size += size;
		}
		else
			break;
	}

	pthread_mutex_unlock(&h264_lock);



	if(!codec->header_only)
	{
		if(pic_out.i_type == X264_TYPE_IDR ||
			pic_out.i_type == X264_TYPE_I)
		{
			is_keyframe = 1;
		}

		if(codec->buffer_size)
		{
			quicktime_write_chunk_header(file, trak, &chunk_atom);
			result = !quicktime_write_data(file, 
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
				vtrack->current_position, 
				track);
		}
		vtrack->current_chunk++;
	}
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
		codec->total_fields,
		CODEC_ID_H264,
		width,
		height,
		stsd_table);
	

	if(codec->decoder) return quicktime_ffmpeg_decode(codec->decoder,
		file, 
		row_pointers, 
		track);

	return 1;
}

// Header copied straight out of another h264 file
#ifdef MANUAL_HEADER
static int write_avcc_header(unsigned char *data)
{
	int result = 0;
	unsigned char *ptr = data;


	static unsigned char test[] =
	{

		0x01, 0x42, 0xe0, 0x28, 0xff, 0xe1, 0x00, 0x1c, 0x67, 0x42, 
		0xe0, 0x28, 0x91, 0xb0, 0x1e, 0x00, 0x89, 0xf9, 0x70, 0x16,
		0xe0, 0x20, 0x20, 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x07,
		0x53, 0x04, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x68,
		0xce, 0x38, 0x80

// 		0x01, 0x4d, 0x40, 0x1f, 0xff, 0xe1, 0x00, 0x14,
// 		0x27, 0x4d, 0x40, 0x1f, 0xa9, 0x18, 0x0a, 0x00, 
// 		0xb7, 0x60, 0x0d, 0x40, 0x40, 0x40, 0x4c, 0x2b, 
// 		0x5e, 0xf7, 0xc0, 0x40, 0x01, 0x00, 0x04, 0x28, 
// 		0xce, 0x0f, 0x88
	};

	memcpy(data, test, sizeof(test));
	result = sizeof(test);

	return result;
}
#endif

static void flush(quicktime_t *file, int track)
{
	quicktime_video_map_t *track_map = &(file->vtracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_h264_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	quicktime_avcc_t *avcc = &trak->mdia.minf.stbl.stsd.table[0].avcc;

	if(!avcc->data_size)
	{

#ifndef MANUAL_HEADER
		codec->header_only = 1;
		encode(file, 0, track);
#else
		unsigned char temp[1024];
		int size = write_avcc_header(temp);

// unsigned char temp[1024] = 
// 0x64, 0x00, 0x29, 0xac, 0x1b, 0x1a, 0x50, 0x1e, 
// 0x00, 0x89, 0xf9, 0x70, 0x16, 0xa0, 0x20, 0x20, 
// 0x28, 0x00, 0x00, 0x1f, 0x48, 0x00, 0x05, 0xdc, 
// 0x07, 0x13, 0x00, 0x00, 0x76, 0x41, 0x40, 0x00, 
// 0x0e, 0x4e, 0x1d, 0x18, 0x9c, 0x07, 0xc7, 0x0c, 
// 0x29, 0x60;
// int size = 42;


		if(size)
			quicktime_set_avcc_header(avcc,
				temp, 
				size);
#endif

	}
/*
 * 	trak->mdia.minf.stbl.stsd.table[0].version = 1;
 * 	trak->mdia.minf.stbl.stsd.table[0].revision = 1;
 */
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
				codec->param.rc.i_bitrate = *(int*)value;
			else
				codec->param.rc.i_bitrate = *(int*)value / 2;
		}
		else
		if(!strcasecmp(key, "h264_quantizer"))
		{
			codec->param.rc.i_qp_constant = 
				codec->param.rc.i_qp_min = 
				codec->param.rc.i_qp_max = *(int*)value;
		}
		else
		if(!strcasecmp(key, "h264_fix_bitrate"))
#ifdef GOOD_VERSION
			codec->param.rc.b_cbr = (*(int*)value) / 1000;
#else
			codec->param.rc.i_qp_constant = (*(int*)value) / 1000;
#endif
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
	x264_param_default(&codec->param);

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




