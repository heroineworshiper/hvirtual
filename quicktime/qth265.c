#include "avcodec.h"
#include "colormodels.h"
#include "funcprotos.h"
#include <pthread.h>
#include "qtffmpeg.h"
#include "quicktime.h"
#include <string.h>
#include "workarounds.h"
#include "x265.h"


typedef struct
{
// Encoder side
	int fix_bitrate;
	int encode_initialized;

// Temporary storage for color conversions
	char *temp_frame;
// Storage of compressed data
	unsigned char *work_buffer;
// Amount of data in work_buffer
	int buffer_size;
// Set by flush to get the header
	int header_only;

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


		pthread_mutex_unlock(&h265_lock);
	}

	

	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->work_buffer) free(codec->work_buffer);
	if(codec->decoder) quicktime_delete_ffmpeg(codec->decoder);


	free(codec);
	return 0;
}





static void flush(quicktime_t *file, int track)
{
#if 0

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
 
#endif
}




static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;


#if 0
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

#endif

	return result;
}




static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_h265_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int w_16 = quicktime_quantize16(width);
	int h_16 = quicktime_quantize16(height);


	if(!codec->decoder) codec->decoder = quicktime_new_ffmpeg(
		file->cpus,
		1,
		AV_CODEC_ID_H265,
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
/*
 * 		if(!strcasecmp(key, "h265_bitrate"))
 * 		{
 * 			codec->param.rc.i_bitrate = *(int*)value / 1000;
 * 		}
 * 		else
 * 		if(!strcasecmp(key, "h265_quantizer"))
 * 		{
 * 			codec->param.rc.i_qp_constant = 
 * 				codec->param.rc.i_qp_min = 
 * 				codec->param.rc.i_qp_max = *(int*)value;
 * 		}
 * 		else
 */
		if(!strcasecmp(key, "h265_fix_bitrate"))
		{
			codec->fix_bitrate = *(int*)value;
//printf("set_parameter %d fix_bitrate=%d\n", __LINE__, *(int*)value);
		}
	}
}

static quicktime_h265_codec_t* init_common(quicktime_video_map_t *vtrack, 
	char *compressor,
	char *title,
	char *description)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_h265_codec_t *codec;

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


	codec = (quicktime_h265_codec_t*)codec_base->priv;

	return codec;
}


void quicktime_init_codec_h265(quicktime_video_map_t *vtrack)
{
    quicktime_h265_codec_t *result = init_common(vtrack,
        QUICKTIME_H265,
        "H.265",
        "H.265");
}




