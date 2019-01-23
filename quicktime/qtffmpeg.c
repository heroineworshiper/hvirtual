#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "qtffmpeg.h"
#include "qtprivate.h"
#include <pthread.h>
#include <string.h>
// FFMPEG front end for quicktime.




// Different ffmpeg versions
#define FFMPEG_2010



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
		avcodec_register_all();
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
// Not exactly user friendly.
			context->thread_count = cpus;
		}

		if(avcodec_open2(context, ptr->decoder[i], 0) < 0)
		{
			int error = 1;
// Try again with 1 thread
			if(cpus > 1)
			{
//				avcodec_thread_init(context, 1);
				context->thread_count = 1;
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
	int bytes = 0;
	int header_bytes = 0;
 	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	quicktime_trak_t *trak = vtrack->track;
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
// swap positions to get it to read the right frame
	int64_t position_temp = vtrack->current_position;

// printf("decode_wrapper %d read_position=%ld current_position=%ld current_field=%d drop_it=%d\n", 
// __LINE__,
// frame_number,
// current_position,
// current_field,
// drop_it);

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
		result = -1;
	}

// int i;
// for(i = 0; i < 16; i++)
// {
// printf("%02x ", ffmpeg->work_buffer[i]);
// }
// printf("\n");

// static FILE *out = 0;
// if(!out) out = fopen("/tmp/debug.mp4", "w");
// fwrite(ffmpeg->work_buffer, bytes + header_bytes, 1, out);
//printf("decode_wrapper %d frame_number=%d result=%d\n", __LINE__, frame_number, result);


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


		if(!ffmpeg->picture[current_field])
		{
			ffmpeg->picture[current_field] = av_frame_alloc();
		}

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


		result = avcodec_decode_video2(ffmpeg->decoder_context[current_field], 
			ffmpeg->picture[current_field], 
			&got_picture, 
			packet);
		av_packet_free(&packet);


// printf("decode_wrapper %d got_picture=%d ptr=%p\n", 
// __LINE__, 
// got_picture,
// ffmpeg->picture[current_field]->data[0]);

// if(ffmpeg->picture[current_field]->data[0])
// {
// int i;
// for(i = 0; i < 16; i++)
// {
// printf("%02x ", ffmpeg->picture[current_field]->data[0][i]);
// }
// printf("\n");
// }

		if(ffmpeg->picture[current_field]->data[0])
		{
			result = 0;
// advance the position
			ffmpeg->last_frame[current_field] += ffmpeg->fields;
		}
		else
		{
// ffmpeg can't recover if the first frame errored out, like in a direct copy
// sequence.
			result = 1;
		}

#ifdef ARCH_X86
		asm("emms");
#endif

// advance the position
		ffmpeg->read_position[current_field] += ffmpeg->fields;
//printf("decode_wrapper %d read_position=%d\n", __LINE__, ffmpeg->read_position[current_field]);
	}

// reset official position to what it was before the read_position
	vtrack->current_position = position_temp;

	return result;
}

// Get amount chroma planes are downsampled from luma plane.
// Used for copying planes into cache.
static int get_chroma_factor(quicktime_ffmpeg_t *ffmpeg, int current_field)
{
	switch(ffmpeg->decoder_context[current_field]->pix_fmt)
	{
		case AV_PIX_FMT_YUV420P:
			return 4;
			break;
		case AV_PIX_FMT_YUVJ420P:
			return 4;
			break;

#ifndef FFMPEG_2010
		case AV_PIX_FMT_YUV422:
			return 2;
			break;
#endif

		case AV_PIX_FMT_YUV422P:
			return 2;
			break;
		case AV_PIX_FMT_YUV410P:
			return 9;
			break;
        case AV_PIX_FMT_YUV420P10LE:
            return 4;
            break;
		default:
			fprintf(stderr, 
				"get_chroma_factor: unrecognized color model %d %d\n", 
				ffmpeg->decoder_context[current_field]->pix_fmt,
                AV_PIX_FMT_YUV420P10LE);
			return 9;
			break;
	}
}


// reduce bits per pixel
static void downsample(quicktime_ffmpeg_t *ffmpeg, 
    quicktime_t *file, 
    unsigned char **picture_y,
    unsigned char **picture_u,
    unsigned char **picture_v,
    int *rowspan)
{
//printf("downsample %d\n", __LINE__);
    if(ffmpeg->decoder_context[0]->pix_fmt ==
        AV_PIX_FMT_YUV420P10LE)
    {
        int i, j;
        if(!ffmpeg->temp_frame)
        {
            ffmpeg->temp_frame = malloc(ffmpeg->width_i * ffmpeg->height_i * 3 / 2);
        }


// printf("downsample %d %d %d\n", 
// __LINE__, 
// rowspan,
// file->in_w);

// for(i = 0; i < 16; i++)
// {
//     printf("%02x ", (*picture_y)[i]);
// }
// printf("\n");





        for(i = file->in_y; i < file->in_y + file->in_h; i++)
        {
            unsigned char *in_y = (*picture_y) + i * *rowspan;
            unsigned char *out_y = ffmpeg->temp_frame + i * ffmpeg->width_i;


            for(j = 0; j < ffmpeg->width_i; j++)
            {
                *out_y++ = (in_y[1] << 6) | (in_y[0] >> 2);
                in_y += 2;
            }

            if(!(i % 2))
            {
                unsigned char *in_u = (*picture_u) + (i / 2) * *rowspan / 2 + file->in_x;
                unsigned char *in_v = (*picture_v) + (i / 2) * *rowspan / 2 + file->in_x;
                unsigned char *out_u = ffmpeg->temp_frame + 
                   ffmpeg->width_i * ffmpeg->height_i +
                   (i / 2) * ffmpeg->width_i / 2;
                unsigned char *out_v = ffmpeg->temp_frame + 
                   ffmpeg->width_i * ffmpeg->height_i +
                   ffmpeg->width_i / 2 * ffmpeg->height_i / 2 +
                   (i / 2) * ffmpeg->width_i / 2;


                for(j = 0; j < ffmpeg->width_i / 2; j++)
                {
                      *out_u++ = (in_u[1] << 6) | (in_u[0] >> 2);
                      in_u += 2;
                      *out_v++ = (in_v[1] << 6) | (in_v[0] >> 2);
                      in_v += 2;
                }
            }
        }

        *picture_y = ffmpeg->temp_frame;
        *picture_u = ffmpeg->temp_frame + ffmpeg->width_i * ffmpeg->height_i;
        *picture_v = *picture_u + ffmpeg->width_i / 2 * ffmpeg->height_i / 2;
        *rowspan = ffmpeg->width_i;
    }
//printf("downsample %d\n", __LINE__);

}


int quicktime_ffmpeg_decode(quicktime_ffmpeg_t *ffmpeg,
	quicktime_t *file, 
	unsigned char **row_pointers, 
	int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	int current_field = vtrack->current_position % ffmpeg->fields;
	int input_cmodel;
	int result = 0;
	int seeking_done = 0;
	int i;
	int debug = 0;
	unsigned char *picture_y = 0;
	unsigned char *picture_u = 0;
	unsigned char *picture_v = 0;
	int rowspan = 0;
	int64_t track_length = quicktime_track_samples(file, trak);

// printf("quicktime_ffmpeg_decode %d current_position=%ld last_frame=%ld\n", 
// __LINE__, 
// vtrack->current_position,
// ffmpeg->last_frame[current_field]);

// Try frame cache
	result = quicktime_get_frame(vtrack->frame_cache,
		vtrack->current_position,
		&picture_y,
		&picture_u,
		&picture_v);



if(debug) printf("quicktime_ffmpeg_decode %d current_position=%ld result=%d\n", 
__LINE__, 
vtrack->current_position,
result);


// Didn't get frame from cache
	if(!result)
	{
// Codecs which work without locking:
// H264
// MPEG-4
//		pthread_mutex_lock(&ffmpeg_lock);



// this doesn't work
		if(ffmpeg->last_frame[current_field] == -1 &&
			ffmpeg->ffmpeg_id != AV_CODEC_ID_H264)
		{
			int current_frame = vtrack->current_position;
			ffmpeg->read_position[current_field] = current_field;
// For certain codecs,
// must decode frame with stream header first but only the first frame in the
// field sequence has a stream header.
			result = decode_wrapper(file, 
				vtrack, 
				ffmpeg, 
				current_field, 
				track,
				0);
			rowspan = ffmpeg->picture[current_field]->linesize[0];
			picture_y = ffmpeg->picture[current_field]->data[0];
			picture_u = ffmpeg->picture[current_field]->data[1];
			picture_v = ffmpeg->picture[current_field]->data[2];
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
			if(!quicktime_has_frame(vtrack->frame_cache, vtrack->current_position + 1))
				quicktime_reset_cache(vtrack->frame_cache);



// Get first keyframe of same field
			do
			{
				frame1 = quicktime_get_keyframe_before(file, 
					frame1 - 1, 
					track);
			}while(frame1 > 0 && (frame1 % ffmpeg->fields) != current_field);
// printf("quicktime_ffmpeg_decode %d frame1=%d frame2=%d\n", 
// __LINE__, 
// frame1,
// frame2);

// if it's less than SEEK_THRESHOLD frames earlier, rewind another keyframe
			if(vtrack->current_position - frame1 < SEEK_THRESHOLD)
			{
				do
				{
					frame1 = quicktime_get_keyframe_before(file,
						frame1 - 1,
						track);
				}while(frame1 > 0 && (frame1 & ffmpeg->fields) != current_field);
//printf("quicktime_ffmpeg_decode 2 %d\n", frame1);
			}

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


// printf("quicktime_ffmpeg_decode %d last_frame=%ld frame1=%d frame2=%d\n", 
// __LINE__,
// ffmpeg->last_frame[current_field],
// frame1,
// frame2);
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
if(debug) printf("quicktime_ffmpeg_decode %d last_frame=%ld read_position=%ld result=%d picture_y=%p\n", 
__LINE__, 
ffmpeg->last_frame[current_field], 
ffmpeg->read_position[current_field],
result,
picture_y);

// read error
				if(result < 0)
				{
					break;
				}

				
				rowspan = ffmpeg->picture[current_field]->linesize[0];
				picture_y = ffmpeg->picture[current_field]->data[0];
				picture_u = ffmpeg->picture[current_field]->data[1];
				picture_v = ffmpeg->picture[current_field]->data[2];



// cache the frame
				if(result == 0)
				{


// downsample the pixels
                    downsample(ffmpeg, 
                        file, 
                        &picture_y, 
                        &picture_u, 
                        &picture_v, 
                        &rowspan);

					int y_size = rowspan * ffmpeg->height_i;
					int u_size = y_size / get_chroma_factor(ffmpeg, current_field);
					int v_size = y_size / get_chroma_factor(ffmpeg, current_field);
					quicktime_put_frame(vtrack->frame_cache,
						ffmpeg->last_frame[current_field],
						picture_y,
						picture_u,
						picture_v,
						y_size,
						u_size,
						v_size);
				}
			}

//printf("quicktime_ffmpeg_decode %d\n", __LINE__);

			seeking_done = 1;
		}

// Not decoded in seeking process.  Decode until a valid frame appears.
		if(!seeking_done &&
// Same frame not requested
			vtrack->current_position != ffmpeg->last_frame[current_field])
		{
			int64_t track_length = quicktime_track_samples(file, trak);
			do
			{

// printf("quicktime_ffmpeg_decode %d current_position=%ld read_position=%ld track_length=%ld\n", 
// __LINE__, 
// vtrack->current_position, 
// ffmpeg->read_position[current_field],
// track_length);

				result = decode_wrapper(file, 
					vtrack, 
					ffmpeg, 
					current_field, 
					track,
					0);

				rowspan = ffmpeg->picture[current_field]->linesize[0];
				picture_y = ffmpeg->picture[current_field]->data[0];
				picture_u = ffmpeg->picture[current_field]->data[1];
				picture_v = ffmpeg->picture[current_field]->data[2];

if(debug) printf("quicktime_ffmpeg_decode %d result=%d picture_y=%p current_position=%ld read_position=%ld last_frame=%ld\n", 
__LINE__, 
result, 
picture_y,
vtrack->current_position,
ffmpeg->read_position[current_field],
ffmpeg->last_frame[current_field]);

			} while(result > 0 && 
				ffmpeg->last_frame[current_field] < track_length - 1 &&
				ffmpeg->read_position[current_field] < track_length);


            downsample(ffmpeg, 
                file, 
                &picture_y, 
                &picture_u, 
                &picture_v, 
                &rowspan);
		}
		else
// same frame requested
		if(!seeking_done &&
			vtrack->current_position == ffmpeg->last_frame[current_field])
		{
			rowspan = ffmpeg->picture[current_field]->linesize[0];
			picture_y = ffmpeg->picture[current_field]->data[0];
			picture_u = ffmpeg->picture[current_field]->data[1];
			picture_v = ffmpeg->picture[current_field]->data[2];

            downsample(ffmpeg, 
                file, 
                &picture_y, 
                &picture_u, 
                &picture_v, 
                &rowspan);
		}
//printf("quicktime_ffmpeg_decode %d current_position=%ld\n", __LINE__, vtrack->current_position);

//		pthread_mutex_unlock(&ffmpeg_lock);


//		ffmpeg->last_frame[current_field] = vtrack->current_position;
	}
	else
	{
// handle the case of colorspaces that were downsampled before caching    
        if(ffmpeg->decoder_context[current_field]->pix_fmt == AV_PIX_FMT_YUV420P10LE)
        {
            rowspan = ffmpeg->width_i;
        }
        else
        {
    		rowspan = ffmpeg->picture[current_field]->linesize[0];
        }
    
	}

// Hopefully this setting will be left over if the cache was used.
	switch(ffmpeg->decoder_context[current_field]->pix_fmt)
	{
		case AV_PIX_FMT_YUV420P:
			input_cmodel = BC_YUV420P;
			break;
// It's not much of a decoder library
		case AV_PIX_FMT_YUVJ420P:
			input_cmodel = BC_YUV420P;
			break;

#ifndef FFMPEG_2010
		case AV_PIX_FMT_YUV422:
			input_cmodel = BC_YUV422;
			break;
#endif

		case AV_PIX_FMT_YUV422P:
			input_cmodel = BC_YUV422P;
			break;
		case AV_PIX_FMT_YUV410P:
			input_cmodel = BC_YUV9P;
			break;
        case AV_PIX_FMT_YUV420P10LE:
            input_cmodel = BC_YUV420P;
            break;
		default:
			fprintf(stderr, 
				"quicktime_ffmpeg_decode: unrecognized color model %d\n", 
				ffmpeg->decoder_context[current_field]->pix_fmt);
			input_cmodel = 0;
			break;
	}

// printf("quicktime_ffmpeg_decode %d result=%d vtrack->current_position=%ld rowspan=%d picture_y=%p input_cmodel=%d output_cmodel=%d\n", 
// __LINE__, 
// result, 
// vtrack->current_position, 
// rowspan,
// picture_y,
// input_cmodel,
// file->color_model);

	if(picture_y)
	{
		unsigned char **input_rows;

		input_rows = 
			malloc(sizeof(unsigned char*) * 
			ffmpeg->decoder_context[current_field]->height);


		for(i = 0; i < ffmpeg->decoder_context[current_field]->height; i++)
			input_rows[i] = picture_y + 
				i * 
				ffmpeg->decoder_context[current_field]->width * 
				cmodel_calculate_pixelsize(input_cmodel);


		cmodel_transfer(row_pointers, /* output */
			input_rows,
			row_pointers[0], /* output */
			row_pointers[1],
			row_pointers[2],
			picture_y, /* input */
			picture_u,
			picture_v,
			file->in_x,        /* Dimensions to capture from input frame */
			file->in_y, 
			file->in_w, 
			file->in_h,
			0,       /* Dimensions to project on output frame */
			0, 
			file->out_w, 
			file->out_h,
			input_cmodel, 
			file->color_model,
			0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			rowspan,       /* For planar use the luma rowspan */
			ffmpeg->width);

		free(input_rows);
	}


	return result;
}

// convert ffmpeg audio to interleaved float output buffer
void quicktime_ffmpeg_get_audio(AVFrame *frame, float *dst)
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
}




