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

#include "colormodels2.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "qtffmpeg.h"
#include "qtprivate.h"

#include <pthread.h>
#include <string.h>


// FFMPEG decoding functions for quicktime.






int ffmpeg_initialized = 0;
pthread_mutex_t ffmpeg_lock = PTHREAD_MUTEX_INITIALIZER;




static void dump_context(void *ptr)
{
	AVCodecContext *context = (AVCodecContext*)ptr;

// 	printf("dump_context %d\n", __LINE__);
// 	printf("    bit_rate=%d\n", context->bit_rate);
// 	printf("    bit_rate_tolerance=%d\n", context->bit_rate_tolerance);
// 	printf("    flags=%d\n", context->flags);
// 	printf("    sub_id=%d\n", context->sub_id);
// 	printf("    me_method=%d\n", context->me_method);
// 	printf("    extradata_size=%d\n", context->extradata_size);
// 	printf("    time_base.num=%d\n", context->time_base.num);
// 	printf("    time_base.den=%d\n", context->time_base.den);
// 	printf("    width=%d\n", context->width);
// 	printf("    height=%d\n", context->height);
// 	printf("    gop_size=%d\n", context->gop_size);
// 	printf("    pix_fmt=%d\n", context->pix_fmt);
// 	printf("    rate_emu=%d\n", context->rate_emu);
// 	printf("    sample_rate=%d\n", context->sample_rate);
// 	printf("    channels=%d\n", context->channels);
// 	printf("    sample_fmt=%d\n", context->sample_fmt);
// 	printf("    frame_size=%d\n", context->frame_size);
// 	printf("    frame_number=%d\n", context->frame_number);
// 	printf("    real_pict_num=%d\n", context->real_pict_num);
// 	printf("    delay=%d\n", context->delay);
// 	printf("    qcompress=%d\n", (int)context->qcompress);
// 	printf("    qblur=%d\n", (int)context->qblur);
// 	printf("    qmin=%d\n", context->qmin);
// 	printf("    qmax=%d\n", context->qmax);
// 	printf("    max_qdiff=%d\n", context->max_qdiff);
// 	printf("    max_b_frames=%d\n", context->max_b_frames);
// 	printf("    b_quant_factor=%d\n", (int)context->b_quant_factor);
// 	printf("    b_frame_strategy=%d\n", context->b_frame_strategy);
// 	printf("    hurry_up=%d\n", context->hurry_up);
// 	printf("    rtp_payload_size=%d\n", context->rtp_payload_size);
// 	printf("    codec_id=%d\n", context->codec_id);
// 	printf("    codec_tag=%d\n", context->codec_tag);
// 	printf("    workaround_bugs=%d\n", context->workaround_bugs);
// //	printf("    error_resilience=%d\n", context->error_resilience);
// 	printf("    has_b_frames=%d\n", context->has_b_frames);
// 	printf("    block_align=%d\n", context->block_align);
// 	printf("    parse_only=%d\n", context->parse_only);
// 	printf("    idct_algo=%d\n", context->idct_algo);
// 	printf("    slice_count=%d\n", context->slice_count);
// 	printf("    slice_offset=%p\n", context->slice_offset);
// 	printf("    error_concealment=%d\n", context->error_concealment);
// 	printf("    dsp_mask=%08x\n", (int)context->dsp_mask);
// 	printf("    slice_flags=%d\n", context->slice_flags);
// 	printf("    xvmc_acceleration=%d\n", context->xvmc_acceleration);
// 	printf("    antialias_algo=%d\n", context->antialias_algo);
// 	printf("    thread_count=%d\n", context->thread_count);
// 	printf("    skip_top=%d\n", context->skip_top);
// 	printf("    profile=%d\n", context->profile);
// 	printf("    level=%d\n", context->level);
// 	printf("    lowres=%d\n", context->lowres);
// 	printf("    coded_width=%d\n", context->coded_width);
// 	printf("    coded_height=%d\n", context->coded_height);
}

quicktime_ffmpeg_t* quicktime_new_ffmpeg(int cpus,
    int use_hw,
	int fields,
	int ffmpeg_id,
	int w,
	int h,
	quicktime_stsd_table_t *stsd_table)
{
	quicktime_ffmpeg_t *ptr = calloc(1, sizeof(quicktime_ffmpeg_t));
	quicktime_esds_t *esds = &stsd_table->esds;
	quicktime_avcc_t *avcc = &stsd_table->avcc;
	int i;

	ptr->fields = fields;
	ptr->width = w;
	ptr->height = h;
	ptr->ffmpeg_id = ffmpeg_id;


//printf("quicktime_new_ffmpeg 1 ffmpeg_id=%d\n", ptr->ffmpeg_id);
	if(ffmpeg_id == AV_CODEC_ID_SVQ1)
	{
		ptr->width_i = quicktime_quantize32(ptr->width);
		ptr->height_i = quicktime_quantize32(ptr->height);
	}
	else
	{
		ptr->width_i = quicktime_quantize16(ptr->width);
		ptr->height_i = quicktime_quantize16(ptr->height);
	}

	pthread_mutex_lock(&ffmpeg_lock);
	if(!ffmpeg_initialized)
	{
		ffmpeg_initialized = 1;
//  		avcodec_init();
//		avcodec_register_all();
	}

	for(i = 0; i < fields; i++)
	{
		ptr->decoder[i] = avcodec_find_decoder(ptr->ffmpeg_id);
		if(!ptr->decoder[i])
		{
			printf("quicktime_new_ffmpeg: avcodec_find_decoder returned NULL.\n");
			quicktime_delete_ffmpeg(ptr);
			return 0;
		}
        
// try hardware decoding
// Append _cuvid to the name to use hardware decoding
        if(use_hw)
        {
            char string[1024];
            sprintf(string, "%s_cuvid", ptr->decoder[i]->name);
            const AVCodec *hw_codec = avcodec_find_decoder_by_name(string);
            if(hw_codec)
            {
                ptr->decoder[i] = hw_codec;
            }
        }

		AVCodecContext *context = 
			ptr->decoder_context[i] = 
			avcodec_alloc_context3(ptr->decoder[i]);
		static char fake_data[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
// 		static unsigned char extra_data[] = 
// 		{
// 			0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x20, 0x00, 0xc8, 0x88, 0xba, 0x98, 0x60, 0xfa, 0x67, 0x80, 0x91, 0x02, 0x83, 0x1f
// 		};
		context->width = ptr->width_i;
		context->height = ptr->height_i;
//		context->width = w;
//		context->height = h;
		context->extradata = (unsigned char *)fake_data;
		context->extradata_size = 0;

		if(esds->mpeg4_header && esds->mpeg4_header_size) 
		{
//printf("quicktime_new_ffmpeg %d\n", __LINE__);
			context->extradata = (unsigned char *)esds->mpeg4_header;
			context->extradata_size = esds->mpeg4_header_size;
		}

		if(avcc->data && avcc->data_size)
		{
//printf("quicktime_new_ffmpeg %d\n", __LINE__);
			context->extradata = (unsigned char *)avcc->data;
			context->extradata_size = avcc->data_size;
		}

		if(cpus > 1)
		{
//			avcodec_thread_init(context, cpus);
			context->thread_count = cpus;
            context->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
		}

        printf("quicktime_new_ffmpeg %d cpus=%d codec=%s id=%d\n", 
            __LINE__,
            cpus,
            ptr->decoder[i]->name,
            context->codec_id);

		if(avcodec_open2(context, ptr->decoder[i], 0) < 0)
		{
			int error = 1;
// Try again with 1 thread
			if(cpus > 1)
			{
//				avcodec_thread_init(context, 1);
				context->thread_count = 1;
                context->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
				if(avcodec_open2(context, ptr->decoder[i], 0) >= 0)
				{
					error = 0;
				}
			}

			if(error)
			{
				printf("quicktime_new_ffmpeg: avcodec_open failed.\n");
				quicktime_delete_ffmpeg(ptr);
			}
		}
		
		
		
		ptr->last_frame[i] = -1;
	}
	pthread_mutex_unlock(&ffmpeg_lock);

	return ptr;
}



void quicktime_delete_ffmpeg(quicktime_ffmpeg_t *ptr)
{
	int i;
	if(ptr)
	{
		pthread_mutex_lock(&ffmpeg_lock);
		for(i = 0; i < ptr->fields; i++)
		{
			if(ptr->decoder_context[i])
			{
	    		avcodec_close(ptr->decoder_context[i]);
				free(ptr->decoder_context[i]);
			}
			
			if(ptr->picture[i])
			{
				av_frame_free(&ptr->picture[i]);
			}
		}
		pthread_mutex_unlock(&ffmpeg_lock);



		if(ptr->temp_frame) free(ptr->temp_frame);
		if(ptr->work_buffer) free(ptr->work_buffer);


		free(ptr);
	}
}

// static void frame_defaults(AVFrame *frame)
// {
// 	memset(frame, 0, sizeof(AVFrame));
// 	av_frame_unref(frame);
// }
 


// send the read_position to the decoder
// advance the read_position by the number of fields
// return -1 if read failed
// return 0 if it succeeded
// return 1 if it read but didn't generate output
static int decode_wrapper(quicktime_t *file,
	quicktime_video_map_t *vtrack,
	quicktime_ffmpeg_t *ffmpeg,
	int current_field, 
	int track,
	int drop_it)
{
	int got_picture = 0; 
	int result = 0; 
    int read_failed = 0;
    int decode_failed = 0;
	int bytes = 0;
	int header_bytes = 0;
 	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	quicktime_trak_t *trak = vtrack->track;
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
// swap positions to get it to read the right frame
	int64_t position_temp = vtrack->current_position;

//printf("decode_wrapper %d\n", __LINE__);

// try popping a frame without reading a packet
	if(!ffmpeg->picture[current_field])
	{
		ffmpeg->picture[current_field] = av_frame_alloc();
	}

    result = avcodec_receive_frame(ffmpeg->decoder_context[current_field], 
        ffmpeg->picture[current_field]);

// read packets
    if(result < 0 || !ffmpeg->picture[current_field]->data[0])
	{
        result = 0;

    	quicktime_set_video_position(file, ffmpeg->read_position[current_field], track);

	    bytes = quicktime_frame_size(file, ffmpeg->read_position[current_field], track); 
	    if(ffmpeg->read_position[current_field] == 0)
	    {
		    header_bytes = stsd_table->esds.mpeg4_header_size;
	    }

	    if(!ffmpeg->work_buffer || ffmpeg->buffer_size < bytes + header_bytes)
	    {
		    if(ffmpeg->work_buffer) free(ffmpeg->work_buffer); 
		    ffmpeg->buffer_size = bytes + header_bytes; 
		    ffmpeg->work_buffer = calloc(1, ffmpeg->buffer_size + 100); 
	    }

	    if(header_bytes)
	    {
		    memcpy(ffmpeg->work_buffer, stsd_table->esds.mpeg4_header, header_bytes);
	    }

// printf("decode_wrapper %d  field=%d offset=0x%lx bytes=%d header_bytes=%d\n", 
// __LINE__, current_field, quicktime_ftell(file), bytes, header_bytes);
// usleep(100000);

	    if(!quicktime_read_data(file, 
		    ffmpeg->work_buffer + header_bytes, 
		    bytes))
	    {
            read_failed = 1;
		    result = -1;
	    }
        

// printf("decode_wrapper %d offset=%ld result=%d\n", 
// __LINE__, 
// ffmpeg->read_position[current_field],
// result);


	    if(!result)
	    {


// No way to determine if there was an error based on nonzero status.
// Need to test row pointers to determine if an error occurred.
		    if(drop_it)
			    ffmpeg->decoder_context[current_field]->skip_frame = AVDISCARD_NONREF /* AVDISCARD_BIDIR */;
		    else
			    ffmpeg->decoder_context[current_field]->skip_frame = AVDISCARD_DEFAULT;
//		avcodec_get_frame_defaults(&ffmpeg->picture[current_field]);

/*
 * printf("decode_wrapper %d frame_number=%d decoder_context=%p picture=%p buffer=%p bytes=%d\n", 
 * __LINE__,
 * frame_number,
 * ffmpeg->decoder_context[current_field],
 * &ffmpeg->picture[current_field],
 * ffmpeg->work_buffer,
 * bytes + header_bytes);
 */


/*
 * if(frame_number >= 200 && frame_number < 280)
 * {
 * char string[1024];
 * sprintf(string, "/tmp/debug%03d", frame_number);
 * FILE *out = fopen(string, "w");
 * fwrite(ffmpeg->work_buffer, bytes, 1, out);
 * fclose(out);
 * }
 */


/*
 * dump_context(ffmpeg->decoder_context[current_field]);
 * ffmpeg->decoder_context[current_field]->bit_rate = 64000;
 * ffmpeg->decoder_context[current_field]->bit_rate_tolerance = 4000000;
 * ffmpeg->decoder_context[current_field]->codec_tag = 1482049860;
 */

    		ffmpeg->decoder_context[current_field]->workaround_bugs =  FF_BUG_NO_PADDING;

// For ffmpeg.080508 you must add
// s->workaround_bugs =  FF_BUG_NO_PADDING;
// in h263dec.c to get MPEG-4 to work.
// Also must compile using -O2 instead of -O3 on gcc 4.1


		    AVPacket *packet = av_packet_alloc();
		    packet->data = ffmpeg->work_buffer;
		    packet->size = bytes + header_bytes;

//printf("decode_wrapper %d size=%d\n", __LINE__, packet->size);
/*
 * for(int i = 0; i < 256; i++)
 * {
 * 	printf("%02x", packet->data[i]);
 * 	if((i + 1) % 16 == 0)
 * 	{
 * 		printf("\n");
 * 	}
 * 	else
 * 	{
 * 		printf(" ");
 * 	}
 * }
 * printf("\n");
 */


//		result = avcodec_decode_video2(ffmpeg->decoder_context[current_field], 
//			ffmpeg->picture[current_field], 
//			&got_picture, 
//			packet);

            result = avcodec_send_packet(ffmpeg->decoder_context[current_field], 
                packet);
		    av_packet_free(&packet);


            result = avcodec_receive_frame(ffmpeg->decoder_context[current_field], 
                ffmpeg->picture[current_field]);

            if(result < 0)
            {
                decode_failed = 1;
            }

// printf("decode_wrapper %d result=%d\n", 
// __LINE__, 
// result);
// 
// if(ffmpeg->picture[current_field]->data[0])
// {
// int i;
// for(i = 0; i < 16; i++)
// {
// printf("%02x ", ffmpeg->picture[current_field]->data[0][i]);
// }
// printf("\n");
// }
    // advance the position
		    ffmpeg->read_position[current_field] += ffmpeg->fields;
    //printf("decode_wrapper %d read_position=%d\n", __LINE__, ffmpeg->read_position[current_field]);
	    }
    }


//printf("decode_wrapper %d %p\n", __LINE__, ffmpeg->picture[current_field]->data[0]);
	if(result >= 0 && ffmpeg->picture[current_field]->data[0])
	{
		result = 0;
// got a frame
// set the pointers for the user to read it
        AVFrame *picture = ffmpeg->picture[current_field];
	    switch(ffmpeg->decoder_context[current_field]->pix_fmt)
	    {
		    case AV_PIX_FMT_YUV420P:
// It's not much of an abstraction
		    case AV_PIX_FMT_YUVJ420P:
			    file->src_colormodel = BC_YUV420P;
			    break;

    // 		case AV_PIX_FMT_YUV422:
    // 			file->src_colormodel = BC_YUV422;
    // 			break;

            case AV_PIX_FMT_YUV444P:
		    case AV_PIX_FMT_YUVJ444P:
			    file->src_colormodel = BC_YUV444P;
                break;

		    case AV_PIX_FMT_YUV422P:
		    case AV_PIX_FMT_YUVJ422P:
			    file->src_colormodel = BC_YUV422P;
			    break;
		    case AV_PIX_FMT_YUV410P:
			    file->src_colormodel = BC_YUV9P;
			    break;
            case AV_PIX_FMT_YUV420P10LE:
                file->src_colormodel = BC_YUV420P10LE;
                break;
            case AV_PIX_FMT_NV12:
			    file->src_colormodel = BC_NV12;
                break;
		    default:
			    fprintf(stderr, 
				    "decode_wrapper %d: unrecognized color model %d\n", 
                    __LINE__,
				    ffmpeg->decoder_context[current_field]->pix_fmt);
//printf("AV_PIX_FMT_GBRP=%d\n", AV_PIX_FMT_GBRP);
			    file->src_colormodel = 0;
			    break;
	    }

        file->src_data = picture->data[0];
        file->src_y = picture->data[0];
        file->src_u = picture->data[1];
        file->src_v = picture->data[2];
        file->src_rowspan = picture->linesize[0];
        file->src_w = ffmpeg->decoder_context[current_field]->width;
        file->src_h = ffmpeg->decoder_context[current_field]->height;
        file->frame_number = ffmpeg->last_frame[current_field];
        file->frame_layer = track;
// printf("decode_wrapper %d src_colormodel=%d src_w=%d src_h=%d y=%p u=%p v=%p rowspan=%d %d %d\n", 
// __LINE__,
// file->src_colormodel,
// file->src_w,
// file->src_h,
// file->src_y,
// file->src_u - file->src_y,
// file->src_v - file->src_u,
// picture->linesize[0],
// picture->linesize[1],
// picture->linesize[2]);
// int i;
// for(i = 0; i < 16; i++)
// {
// printf("%02x ", file->src_y[i]);
// }
// printf("\n");
// advance the position
		ffmpeg->last_frame[current_field] += ffmpeg->fields;
	}
    else
    {
        if(read_failed)
            result = -1;
        if(decode_failed)
            result = 1;
    }

#ifdef ARCH_X86
	asm("emms");
#endif

// reset official position to what it was before the read_position
	vtrack->current_position = position_temp;

	return result;
}

// Get amount chroma planes are downsampled from luma plane.
// Used for copying planes into cache.
// static int get_chroma_factor(quicktime_ffmpeg_t *ffmpeg, int current_field)
// {
// 	switch(ffmpeg->decoder_context[current_field]->pix_fmt)
// 	{
// 		case AV_PIX_FMT_YUV420P:
// // assume the UV planes are contiguous so goofy NV formats have the same amount of space
// // in the U plane as 2 distinct UV planes
// 		case AV_PIX_FMT_NV12:
// 		case AV_PIX_FMT_NV21:
// 			return 4;
// 			break;
// 		case AV_PIX_FMT_YUVJ420P:
// 			return 4;
// 			break;
// 
// // 		case AV_PIX_FMT_YUV422:
// // 			return 2;
// // 			break;
// 
// 		case AV_PIX_FMT_YUV422P:
// 			return 2;
// 			break;
// 		case AV_PIX_FMT_YUV410P:
// 			return 9;
// 			break;
//         case AV_PIX_FMT_YUV420P10LE:
//             return 4;
//             break;
// 		default:
// 			fprintf(stderr, 
// 				"get_chroma_factor: unrecognized color model %d %d\n", 
// 				ffmpeg->decoder_context[current_field]->pix_fmt,
//                 AV_PIX_FMT_YUV420P10LE);
// 			return 9;
// 			break;
// 	}
// }


// reduce bits per pixel
// static void downsample(quicktime_ffmpeg_t *ffmpeg, 
//     quicktime_t *file, 
//     unsigned char **picture_y,
//     unsigned char **picture_u,
//     unsigned char **picture_v,
//     int *rowspan)
// {
// //printf("downsample %d\n", __LINE__);
//     if(ffmpeg->decoder_context[0]->pix_fmt ==
//         AV_PIX_FMT_YUV420P10LE)
//     {
//         int i, j;
//         if(!ffmpeg->temp_frame)
//         {
//             ffmpeg->temp_frame = malloc(ffmpeg->width_i * ffmpeg->height_i * 3 / 2);
//         }
// 
// 
// // printf("downsample %d %d %d\n", 
// // __LINE__, 
// // rowspan,
// // file->in_w);
// 
// // for(i = 0; i < 16; i++)
// // {
// //     printf("%02x ", (*picture_y)[i]);
// // }
// // printf("\n");
// 
// 
// 
// 
// 
//         for(i = file->in_y; i < file->in_y + file->in_h; i++)
//         {
//             unsigned char *in_y = (*picture_y) + i * *rowspan;
//             unsigned char *out_y = ffmpeg->temp_frame + i * ffmpeg->width_i;
// 
// 
//             for(j = 0; j < ffmpeg->width_i; j++)
//             {
//                 *out_y++ = (in_y[1] << 6) | (in_y[0] >> 2);
//                 in_y += 2;
//             }
// 
//             if(!(i % 2))
//             {
//                 unsigned char *in_u = (*picture_u) + (i / 2) * *rowspan / 2 + file->in_x;
//                 unsigned char *in_v = (*picture_v) + (i / 2) * *rowspan / 2 + file->in_x;
//                 unsigned char *out_u = ffmpeg->temp_frame + 
//                    ffmpeg->width_i * ffmpeg->height_i +
//                    (i / 2) * ffmpeg->width_i / 2;
//                 unsigned char *out_v = ffmpeg->temp_frame + 
//                    ffmpeg->width_i * ffmpeg->height_i +
//                    ffmpeg->width_i / 2 * ffmpeg->height_i / 2 +
//                    (i / 2) * ffmpeg->width_i / 2;
// 
// 
//                 for(j = 0; j < ffmpeg->width_i / 2; j++)
//                 {
//                       *out_u++ = (in_u[1] << 6) | (in_u[0] >> 2);
//                       in_u += 2;
//                       *out_v++ = (in_v[1] << 6) | (in_v[0] >> 2);
//                       in_v += 2;
//                 }
//             }
//         }
// 
//         *picture_y = ffmpeg->temp_frame;
//         *picture_u = ffmpeg->temp_frame + ffmpeg->width_i * ffmpeg->height_i;
//         *picture_v = *picture_u + ffmpeg->width_i / 2 * ffmpeg->height_i / 2;
//         *rowspan = ffmpeg->width_i;
//     }
// //printf("downsample %d\n", __LINE__);
// 
// }


int quicktime_ffmpeg_decode(quicktime_ffmpeg_t *ffmpeg,
	quicktime_t *file, 
	unsigned char **row_pointers, 
	int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	int current_field = vtrack->current_position % ffmpeg->fields;
	int result = 0;
	int seeking_done = 0;
	int i;
	int debug = 0;
// 	unsigned char *picture_y = 0;
// 	unsigned char *picture_u = 0;
// 	unsigned char *picture_v = 0;
// 	int rowspan = 0;
	int64_t track_length = quicktime_track_samples(file, trak);

// printf("quicktime_ffmpeg_decode %d current_position=%ld last_frame=%ld\n", 
// __LINE__, 
// vtrack->current_position,
// ffmpeg->last_frame[current_field]);

// Try frame cache if it was decoded while seeking
// 	int got_cached = quicktime_get_cache(file->frame_cache,
// 		vtrack->current_position,
//         track,
// 		&picture_y,
// 		&picture_u,
// 		&picture_v);



// printf("quicktime_ffmpeg_decode %d current_position=%ld result=%d\n", 
// __LINE__, 
// vtrack->current_position,
// result);
// if(picture_y)
// printf("%02x%02x%02x%02x%02x%02x%02x%02x\n", 
// picture_y[0],
// picture_y[1],
// picture_y[2],
// picture_y[3],
// picture_y[4],
// picture_y[5],
// picture_y[6],
// picture_y[7]);


// Didn't get frame from cache
// 	if(!got_cached)
// 	{
// Codecs which work without locking:
// H264
// MPEG-4
//		pthread_mutex_lock(&ffmpeg_lock);



		if(ffmpeg->last_frame[current_field] == -1 &&
			ffmpeg->ffmpeg_id != AV_CODEC_ID_H264)
		{
			int current_frame = vtrack->current_position;
			ffmpeg->read_position[current_field] = current_field;
// For certain codecs,
// must decode frame with stream header first but only the first frame in the
// field sequence has a stream header.
            while(1)
            {
			    result = decode_wrapper(file, 
				    vtrack, 
				    ffmpeg, 
				    current_field, 
				    track,
				    0);
                if(result == 0 || result == -1) break;
            }
// printf("quicktime_ffmpeg_decode %d current_position=%ld result=%d %p\n", 
// __LINE__, 
// vtrack->current_position,
// result,
// ffmpeg->picture[current_field]);
// Reset position because decode wrapper set it
			quicktime_set_video_position(file, current_frame, track);

// nonsense because it never outputs the 1st frame
			ffmpeg->last_frame[current_field] = current_field;
		}


// number of frames to feed the decoder before it generates an output
#define SEEK_THRESHOLD 5

// Handle seeking
// Seeking requires keyframes
		if(quicktime_has_keyframes(file, track) && 
// Not next frame
			vtrack->current_position != ffmpeg->last_frame[current_field] + ffmpeg->fields &&
// Not same frame requested twice
			vtrack->current_position != ffmpeg->last_frame[current_field])
		{


if(debug) printf("quicktime_ffmpeg_decode %d current_position=%ld last_frame=%ld\n", 
__LINE__, 
vtrack->current_position,
ffmpeg->last_frame[current_field]);


			int first_frame = 0;
// new read position
			int frame1 = vtrack->current_position;

// If an interleaved codec, the opposite field would have been decoded in the previous
// seek.
// 			if(!quicktime_has_cache(file->frame_cache, 
//                 vtrack->current_position + 1, 
//                 track))
// 				quicktime_reset_cache(file->frame_cache);



// Get first keyframe of same field
			do
			{
				frame1 = quicktime_get_keyframe_before(file, 
					frame1 - 1, 
					track);
			}while(frame1 > 0 && 
                (frame1 % ffmpeg->fields) != current_field);
// printf("quicktime_ffmpeg_decode %d vtrack->current_position=%d frame1=%d\n", 
// __LINE__, 
// (int)vtrack->current_position,
// (int)frame1);

// if it's less than SEEK_THRESHOLD frames earlier, rewind another keyframe
			if(vtrack->current_position - frame1 < SEEK_THRESHOLD)
			{
				do
				{
					frame1 = quicktime_get_keyframe_before(file,
						frame1 - 1,
						track);
				}while(frame1 > 0 && 
                    (frame1 % ffmpeg->fields) != current_field);
			}
// printf("quicktime_ffmpeg_decode %d vtrack->current_position=%d frame1=%d\n", 
// __LINE__, 
// (int)vtrack->current_position,
// (int)frame1);

// Drop frames instead of starting from the keyframe
			if(ffmpeg->last_frame[current_field] > frame1 &&
				vtrack->current_position > ffmpeg->last_frame[current_field])
			{
if(debug) printf("quicktime_ffmpeg_decode %d frame1=%d dropping frames\n", 
__LINE__,
frame1);

				frame1 = ffmpeg->read_position[current_field];

			}
			else
			{
// restart decoding
if(debug) printf("quicktime_ffmpeg_decode %d frame1=%d restarting\n", 
__LINE__,
frame1);
// very important to reset the codec when changing keyframes
				avcodec_flush_buffers(ffmpeg->decoder_context[current_field]);

// reset the read position
				ffmpeg->read_position[current_field] = frame1;
				ffmpeg->last_frame[current_field] = frame1 - ffmpeg->fields;
			}

			first_frame = frame1;


// printf("quicktime_ffmpeg_decode %d vtrack->current_position=%d last_frame=%d frame1=%d\n", 
// __LINE__,
// (int)vtrack->current_position,
// (int)ffmpeg->last_frame[current_field],
// frame1);
// read frames until the current position is decoded
			while(ffmpeg->last_frame[current_field] < vtrack->current_position &&
				ffmpeg->read_position[current_field] < track_length)
			{
				result = decode_wrapper(file, 
					vtrack, 
					ffmpeg, 
					current_field, 
					track,
// Don't drop if we want to cache it
					0);
if(debug) printf("quicktime_ffmpeg_decode %d last_frame=%ld read_position=%ld result=%d\n", 
__LINE__, 
ffmpeg->last_frame[current_field], 
ffmpeg->read_position[current_field],
result);

// read error
				if(result < 0)
				{
					break;
				}




// cache the frame if it is not being returned
 				if(result == 0 &&
                    ffmpeg->last_frame[current_field] < vtrack->current_position)
 				{
					quicktime_put_cache(file->frame_cache);
// printf("quicktime_ffmpeg_decode %d caching last_frame=%ld current_position=%ld\n", 
// __LINE__, 
// ffmpeg->last_frame[current_field],
// vtrack->current_position);
				}
			}


			seeking_done = 1;
		}

// Not decoded in the seeking process.  Decode until a valid frame appears.
		if(!seeking_done &&
// Same frame not requested
			vtrack->current_position != ffmpeg->last_frame[current_field])
		{
			int64_t track_length = quicktime_track_samples(file, trak);
// printf("quicktime_ffmpeg_decode %d current_position=%d read_position=%d track_length=%d\n", 
// __LINE__, 
// (int)vtrack->current_position, 
// (int)ffmpeg->read_position[current_field],
// (int)track_length);
			do
			{


				result = decode_wrapper(file, 
					vtrack, 
					ffmpeg, 
					current_field, 
					track,
					0);

if(debug) printf("quicktime_ffmpeg_decode %d result=%d current_position=%ld read_position=%ld last_frame=%ld\n", 
__LINE__, 
result, 
vtrack->current_position,
ffmpeg->read_position[current_field],
ffmpeg->last_frame[current_field]);

			} while(result > 0 && 
				ffmpeg->last_frame[current_field] < track_length - 1 &&
				ffmpeg->read_position[current_field] < track_length);
		}
		else
// same frame requested & not in cache.  
// Assume a single layer is being decoded & the last 
// frame is still in file->src_
		if(!seeking_done &&
			vtrack->current_position == ffmpeg->last_frame[current_field])
		{
		}
//printf("quicktime_ffmpeg_decode %d current_position=%ld\n", __LINE__, vtrack->current_position);

//		pthread_mutex_unlock(&ffmpeg_lock);


//		ffmpeg->last_frame[current_field] = vtrack->current_position;
//	}



// printf("quicktime_ffmpeg_decode %d result=%d vtrack->current_position=%ld rowspan=%d picture_y=%p input_cmodel=%d output_cmodel=%d\n", 
// __LINE__, 
// result, 
// vtrack->current_position, 
// rowspan,
// picture_y,
// input_cmodel,
// file->color_model);
//printf("quicktime_ffmpeg_decode %d\n", __LINE__);

	return result;
}














// Attempts to read more samples than this will crash
#define TEMP_ALLOCATION 0x100000

void quicktime_ffaudio_init(quicktime_ffaudio_t *ffaudio, int codec_id)
{
    ffaudio->codec_id = codec_id;
}

void quicktime_ffaudio_delete(quicktime_ffaudio_t *ffaudio)
{
    if(ffaudio->decoder_context)
    {
    	avcodec_close(ffaudio->decoder_context);
    	free(ffaudio->decoder_context);
    }

    bzero(ffaudio, sizeof(quicktime_ffaudio_t));
}

int quicktime_ffaudio_decode(quicktime_t *file,
    quicktime_audio_map_t *track_map,
    quicktime_ffaudio_t *ffaudio,
	float *output,
	int64_t samples, 
	int channel)
{
	quicktime_trak_t *trak = track_map->track;
	int64_t current_position = track_map->current_position;
	int64_t end_position = current_position + samples;
	quicktime_vbr_t *vbr = &track_map->vbr;
    quicktime_stsd_table_t *stsd = &trak->mdia.minf.stbl.stsd.table[0];
    int i;

    if(!ffaudio->decoder_initialized)
    {
        ffaudio->decoder_initialized = 1;
		pthread_mutex_lock(&ffmpeg_lock);
        ffaudio->decoder = avcodec_find_decoder(ffaudio->codec_id);
		if(!ffaudio->decoder)
		{
			printf("quicktime_ffaudio_decode %d: no ffmpeg decoder found.\n", __LINE__);
		    pthread_mutex_unlock(&ffmpeg_lock);
			return 1;
		}

		ffaudio->decoder_context = 
			avcodec_alloc_context3(ffaudio->decoder);
        
        quicktime_esds_t *esds = &stsd->esds;
        quicktime_dac3_t *dac3 = &stsd->dac3;
        ffaudio->decoder_context->sample_rate = stsd->sample_rate;
    	ffaudio->decoder_context->channels = stsd->channels;

//        ffaudio->decoder_context->profile = FF_PROFILE_AAC_HE;


//printf("quicktime_ffaudio_decode %d %d %d %d %d\n", 
//__LINE__, esds->got_esds_rate, esds->channels, stsd->channels, esds->mpeg4_header_size);

		if(esds->mpeg4_header && esds->mpeg4_header_size) 
		{
			ffaudio->decoder_context->extradata = (unsigned char *)esds->mpeg4_header;
			ffaudio->decoder_context->extradata_size = esds->mpeg4_header_size;
		}


		if(avcodec_open2(ffaudio->decoder_context, ffaudio->decoder, 0) < 0)
		{
			printf("quicktime_ffaudio_decode %d: avcodec_open failed.\n", __LINE__);
		    pthread_mutex_unlock(&ffmpeg_lock);
			return 1;
		}

		pthread_mutex_unlock(&ffmpeg_lock);

        quicktime_init_vbr(vbr, track_map->channels);
		if(!ffaudio->temp_buffer)
			ffaudio->temp_buffer = calloc(sizeof(float), TEMP_ALLOCATION);
    }

	if(quicktime_align_vbr(track_map, samples))
	{
		return 1;
	}

// Decode until buffer is full
    int errors = 0;
	while(quicktime_vbr_end(vbr) < end_position && errors < 100)
	{
		if(quicktime_read_vbr(file, track_map)) break;

		AVPacket *packet = av_packet_alloc();
        packet->data = quicktime_vbr_input(vbr);
        packet->size = quicktime_vbr_input_size(vbr);
// printf("quicktime_ffaudio_decode %d size=%d ", __LINE__, packet->size);
// #define MIN(x, y) ((x) < (y) ? (x) : (y))
// quicktime_print_buffer("", packet->data, MIN(8, packet->size));
// printf("\n");

        int result = avcodec_send_packet(ffaudio->decoder_context, packet);
		av_packet_free(&packet);


        if(result < 0)
        {
            printf("quicktime_ffaudio_decode %d avcodec_send_packet failed %c%c%c%c\n", 
                __LINE__,
                (-result) & 0xff,
                ((-result) >> 8) & 0xff,
                ((-result) >> 16) & 0xff,
                ((-result) >> 24) & 0xff);
            errors++;
        }
        quicktime_shift_vbr(track_map, quicktime_vbr_input_size(vbr));

		AVFrame *frame = av_frame_alloc();
        result = 0;
        while(result >= 0)
        {
            result = avcodec_receive_frame(ffaudio->decoder_context, frame);
// convert to floating point
            if(result >= 0)
            {
// printf("quicktime_ffaudio_decode %d samplerate=%d channels=%d\n",
// __LINE__, 
// ffaudio->decoder_context->sample_rate,
// ffaudio->decoder_context->channels);
// overwrite headers
//                 stsd->sample_rate = ffaudio->decoder_context->sample_rate;
//     	        stsd->channels = ffaudio->decoder_context->channels;
//                 track_map->channels = ffaudio->decoder_context->channels;


// transfer from frame to temp buffer
                int samples = quicktime_ffmpeg_get_audio(frame, 
	    			ffaudio->temp_buffer);
                quicktime_store_vbr_float(track_map,
				    ffaudio->temp_buffer,
				    samples);
            }
            else
            {
//                 printf("quicktime_ffaudio_decode %d avcodec_receive_frame failed %d\n", 
//                     __LINE__,
//                     result);
            }
        }
		av_frame_free(&frame);

    }

// Transfer from VBR buffer to output
	quicktime_copy_vbr_float(vbr, 
		current_position, 
		samples,
		output, 
		channel);
    return 0;
}


// convert ffmpeg audio to interleaved float output buffer
// return the number of samples converted
int quicktime_ffmpeg_get_audio(AVFrame *frame, float *dst)
{
	int channels = frame->channels;
	int samples = frame->nb_samples;
	int i, j;
	for(j = 0; j < channels; j++)
	{
		float *out = dst + j;
		
		switch(frame->format)
		{
			case AV_SAMPLE_FMT_FLTP:
			{
				float *in = (float*)frame->data[j];
				for(i = 0; i < samples; i++)
				{
					*out = *in;
					in++;
					out += channels;
				}
				break;
			}
			
            case AV_SAMPLE_FMT_S16:
            {
				int16_t *in = (int16_t*)frame->data[0] + j;
				for(i = 0; i < samples; i++)
				{
					*out = (float)*in / 32767;
					in += channels;
					out += channels;
				}
                break;
            }

			case AV_SAMPLE_FMT_S16P:
			{
				int16_t *in = (int16_t*)frame->data[j];
				for(i = 0; i < samples; i++)
				{
					*out = (float)*in / 32767;
					in++;
					out += channels;
				}
				break;
			}
			
			default:
				printf("quicktime_ffmpeg_get_audio %d unsupported audio format %d\n", 
					__LINE__,
					frame->format);
				break;
		}
	}
    return samples;
}




