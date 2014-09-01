#include "mpeg3private.h"
#include "mpeg3protos.h"

#include <pthread.h>
#include <stdlib.h>

#define CLIP(x)  ((x) >= 0 ? ((x) < 255 ? (x) : 255) : 0)


int mpeg3_new_slice_buffer(mpeg3_slice_buffer_t *slice_buffer)
{
	pthread_mutexattr_t mutex_attr;

	slice_buffer->data = malloc(1024);
	slice_buffer->buffer_size = 0;
	slice_buffer->buffer_allocation = 1024;
	slice_buffer->current_position = 0;
	slice_buffer->bits_size = 0;
	slice_buffer->bits = 0;
	slice_buffer->done = 0;
	pthread_mutexattr_init(&mutex_attr);
//	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	pthread_mutex_init(&(slice_buffer->completion_lock), &mutex_attr);
	return 0;
}

int mpeg3_delete_slice_buffer(mpeg3_slice_buffer_t *slice_buffer)
{
	free(slice_buffer->data);
	pthread_mutex_destroy(&(slice_buffer->completion_lock));
	return 0;
}

int mpeg3_expand_slice_buffer(mpeg3_slice_buffer_t *slice_buffer)
{
	int i;
	unsigned char *new_buffer = malloc(slice_buffer->buffer_allocation * 2);
	for(i = 0; i < slice_buffer->buffer_size; i++)
		new_buffer[i] = slice_buffer->data[i];
	free(slice_buffer->data);
	slice_buffer->data = new_buffer;
	slice_buffer->buffer_allocation *= 2;
	return 0;
}

/* limit coefficients to -2048..2047 */

/* move/add 8x8-Block from block[comp] to refframe */

static inline int mpeg3video_addblock(mpeg3_slice_t *slice, 
		mpeg3video_t *video, 
		int comp, 
		int bx, 
		int by, 
		int dct_type, 
		int addflag)
{
	int cc, i, iincr;
	unsigned char *rfp;
	short *bp;
	int spar = slice->sparse[comp];
/* color component index */
  	cc = (comp < 4) ? 0 : (comp & 1) + 1; 

  	if(cc == 0)
	{   
/* luminance */
    	if(video->pict_struct == FRAME_PICTURE)
		{
      		if(dct_type)
			{
/* field DCT coding */
        		rfp = video->newframe[0] + 
              		video->coded_picture_width * (by + ((comp & 2) >> 1)) + bx + ((comp & 1) << 3);
        		iincr = (video->coded_picture_width << 1);
      		}
      		else
			{
/* frame DCT coding */
        		rfp = video->newframe[0] + 
             		video->coded_picture_width * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
        		iincr = video->coded_picture_width;
      		}
		}
    	else 
		{
/* field picture */
      		rfp = video->newframe[0] + 
           		(video->coded_picture_width << 1) * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
      		iincr = (video->coded_picture_width << 1);
    	}
 	}
  	else 
	{
/* chrominance */

/* scale coordinates */
    	if(video->chroma_format != CHROMA444) bx >>= 1;
    	if(video->chroma_format == CHROMA420) by >>= 1;
    	if(video->pict_struct == FRAME_PICTURE)
		{
    		if(dct_type && (video->chroma_format != CHROMA420))
			{
/* field DCT coding */
        		rfp = video->newframe[cc]
            		  + video->chrom_width * (by + ((comp & 2) >> 1)) + bx + (comp & 8);
        		iincr = (video->chrom_width << 1);
    		}
    		else 
			{
/* frame DCT coding */
        		rfp = video->newframe[cc]
            		  + video->chrom_width * (by + ((comp & 2) << 2)) + bx + (comp & 8);
        		iincr = video->chrom_width;
    		}
    	}
    	else 
		{
/* field picture */
    		rfp = video->newframe[cc]
            	  + (video->chrom_width << 1) * (by + ((comp & 2) << 2)) + bx + (comp & 8);
    		iincr = (video->chrom_width << 1);
    	}
  	}

  	bp = slice->block[comp];

	if(addflag)
	{
		for(i = 0; i < 8; i++)
		{
    		rfp[0] = CLIP(bp[0] + rfp[0]);
    		rfp[1] = CLIP(bp[1] + rfp[1]);
    		rfp[2] = CLIP(bp[2] + rfp[2]);
    		rfp[3] = CLIP(bp[3] + rfp[3]);
    		rfp[4] = CLIP(bp[4] + rfp[4]);
    		rfp[5] = CLIP(bp[5] + rfp[5]);
    		rfp[6] = CLIP(bp[6] + rfp[6]);
    		rfp[7] = CLIP(bp[7] + rfp[7]);
    		rfp += iincr;
    		bp += 8;
		}
  	}
  	else 
  	{
    	for(i = 0; i < 8; i++)
		{
    		rfp[0] = CLIP(bp[0] + 128);
    		rfp[1] = CLIP(bp[1] + 128);
    		rfp[2] = CLIP(bp[2] + 128);
    		rfp[3] = CLIP(bp[3] + 128);
    		rfp[4] = CLIP(bp[4] + 128);
    		rfp[5] = CLIP(bp[5] + 128);
    		rfp[6] = CLIP(bp[6] + 128);
    		rfp[7] = CLIP(bp[7] + 128);
    		rfp+= iincr;
    		bp += 8;
    	}
  	}
	return 0;
}

int mpeg3_decode_slice(mpeg3_slice_t *slice)
{
	mpeg3video_t *video = slice->video;
	int comp;
	int mb_type, cbp, motion_type = 0, dct_type;
	int macroblock_address, mba_inc, mba_max;
	int slice_vert_pos_ext;
	unsigned int code;
	int bx, by;
	int dc_dct_pred[3];
	int mv_count, mv_format, mvscale;
	int pmv[2][2][2], mv_field_sel[2][2];
	int dmv, dmvector[2];
	int qs;
	int stwtype, stwclass; 
	int snr_cbp;
	int i;
	mpeg3_slice_buffer_t *slice_buffer = slice->slice_buffer;

/* number of macroblocks per picture */
  	mba_max = video->mb_width * video->mb_height;

/* field picture has half as many macroblocks as frame */
	if(video->pict_struct != FRAME_PICTURE)
	    mba_max >>= 1; 

/* macroblock address */
  	macroblock_address = 0; 
/* first macroblock in slice is not skipped */
  	mba_inc = 0;
  	slice->fault = 0;

	code = mpeg3slice_getbits(slice_buffer, 32);
/* decode slice header (may change quant_scale) */
    slice_vert_pos_ext = mpeg3video_getslicehdr(slice, video);

/* reset all DC coefficient and motion vector predictors */
    dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;
    pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;
    pmv[0][1][0] = pmv[0][1][1] = pmv[1][1][0] = pmv[1][1][1] = 0;

  	for(i = 0; 
		slice_buffer->current_position < slice_buffer->buffer_size; 
		i++)
	{
		if(mba_inc == 0)
		{
/* Done */
			if(!mpeg3slice_showbits(slice_buffer, 23)) return 0;
/* decode macroblock address increment */
    		mba_inc = mpeg3video_get_macroblock_address(slice);

        	if(slice->fault) return 1;

    		if(i == 0)
			{
/* Get the macroblock_address */
				macroblock_address = ((slice_vert_pos_ext << 7) + (code & 255) - 1) * video->mb_width + mba_inc - 1;
/* first macroblock in slice: not skipped */
				mba_inc = 1;
			}
		}

        if(slice->fault) return 1;

    	if(macroblock_address >= mba_max)
		{
/* mba_inc points beyond picture dimensions */
      		/*fprintf(stderr, "mpeg3_decode_slice: too many macroblocks in picture\n"); */
      		return 1;
    	}

/* not skipped */
    	if(mba_inc == 1)
		{
			mpeg3video_macroblock_modes(slice, 
				video, 
				&mb_type, 
				&stwtype, 
				&stwclass,
        		&motion_type, 
				&mv_count, 
				&mv_format, 
				&dmv, 
				&mvscale, 
				&dct_type);

			if(slice->fault) return 1;

      		if(mb_type & MB_QUANT)
			{
        		qs = mpeg3slice_getbits(slice_buffer, 5);

        		if(video->mpeg2)
            	 	slice->quant_scale = video->qscale_type ? mpeg3_non_linear_mquant_table[qs] : (qs << 1);
        		else 
					slice->quant_scale = qs;

        		if(video->scalable_mode == SC_DP)
/* make sure quant_scale is valid */
          			slice->quant_scale = slice->quant_scale;
      		}

/* motion vectors */


/* decode forward motion vectors */
      		if((mb_type & MB_FORWARD) || ((mb_type & MB_INTRA) && video->conceal_mv))
			{
        		if(video->mpeg2)
        			mpeg3video_motion_vectors(slice, 
						video, 
						pmv, 
						dmvector, 
						mv_field_sel, 
            			0, 
						mv_count, 
						mv_format, 
						video->h_forw_r_size, 
						video->v_forw_r_size, 
						dmv, 
						mvscale);
        		else
        		  	mpeg3video_motion_vector(slice, 
						video, 
						pmv[0][0], 
						dmvector, 
            			video->forw_r_size, 
						video->forw_r_size, 
						0, 
						0, 
						video->full_forw);
    		}
      		if(slice->fault) return 1;

/* decode backward motion vectors */
    		if(mb_type & MB_BACKWARD)
			{
        		if(video->mpeg2)
        		  	mpeg3video_motion_vectors(slice, 
						video, 
						pmv, 
						dmvector, 
						mv_field_sel, 
            			1, 
						mv_count, 
						mv_format, 
						video->h_back_r_size, 
						video->v_back_r_size, 
						0, 
						mvscale);
        		else
        		  	mpeg3video_motion_vector(slice, 
						video, 
						pmv[0][1], 
						dmvector, 
            			video->back_r_size, 
						video->back_r_size, 
						0, 
						0, 
						video->full_back);
    		}

      		if(slice->fault) return 1;

/* remove marker_bit */
      		if((mb_type & MB_INTRA) && video->conceal_mv)
        		mpeg3slice_flushbit(slice_buffer);

/* macroblock_pattern */
      		if(mb_type & MB_PATTERN)
			{
        		cbp = mpeg3video_get_cbp(slice);
        		if(video->chroma_format == CHROMA422)
				{
/* coded_block_pattern_1 */
        		  	cbp = (cbp << 2) | mpeg3slice_getbits2(slice_buffer); 
        		}
        		else
				if(video->chroma_format == CHROMA444)
				{
/* coded_block_pattern_2 */
        		  	cbp = (cbp << 6) | mpeg3slice_getbits(slice_buffer, 6); 
        		}
    		}
    		else
        	  	cbp = (mb_type & MB_INTRA) ? ((1 << video->blk_cnt) - 1) : 0;

      		if(slice->fault) return 1;
/* decode blocks */
      		mpeg3video_clearblock(slice, 0, video->blk_cnt);
      		for(comp = 0; comp < video->blk_cnt; comp++)
			{
        		if(cbp & (1 << (video->blk_cnt - comp - 1)))
				{
          			if(mb_type & MB_INTRA)
					{
            			if(video->mpeg2)
							mpeg3video_getmpg2intrablock(slice, video, comp, dc_dct_pred);
            			else
							mpeg3video_getintrablock(slice, video, comp, dc_dct_pred);
          			}
        			else 
					{
            		  	if(video->mpeg2) 
					  		mpeg3video_getmpg2interblock(slice, video, comp);
            		  	else           
					  		mpeg3video_getinterblock(slice, video, comp);
        			}
        			if(slice->fault) return 1;
        		}
      		}

/* reset intra_dc predictors */
			if(!(mb_type & MB_INTRA))
        	  	dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

/* reset motion vector predictors */
    		if((mb_type & MB_INTRA) && !video->conceal_mv)
			{
/* intra mb without concealment motion vectors */
        		pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;
        		pmv[0][1][0] = pmv[0][1][1] = pmv[1][1][0] = pmv[1][1][1] = 0;
    		}

    		if((video->pict_type == P_TYPE) && !(mb_type & (MB_FORWARD | MB_INTRA)))
			{
/* non-intra mb without forward mv in a P picture */
        		pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;

/* derive motion_type */
        		if(video->pict_struct == FRAME_PICTURE) 
					motion_type = MC_FRAME;
        		else
        		{
        			motion_type = MC_FIELD;
/* predict from field of same parity */
        			mv_field_sel[0][0] = (video->pict_struct == BOTTOM_FIELD);
        		}
      		}

    		if(stwclass == 4)
    		{
/* purely spatially predicted macroblock */
        		pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;
        		pmv[0][1][0] = pmv[0][1][1] = pmv[1][1][0] = pmv[1][1][1] = 0;
    		}
    	}
    	else 
		{
/* mba_inc!=1: skipped macroblock */
      		mpeg3video_clearblock(slice, 0, video->blk_cnt);

/* reset intra_dc predictors */
      		dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

/* reset motion vector predictors */
      		if(video->pict_type == P_TYPE)
        		pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;

/* derive motion_type */
      		if(video->pict_struct == FRAME_PICTURE)
        		motion_type = MC_FRAME;
    		else
    		{
        		motion_type = MC_FIELD;
/* predict from field of same parity */
        		mv_field_sel[0][0] = mv_field_sel[0][1] = (video->pict_struct == BOTTOM_FIELD);
    		}

/* skipped I are spatial-only predicted, */
/* skipped P and B are temporal-only predicted */
      		stwtype = (video->pict_type == I_TYPE) ? 8 : 0;

/* clear MB_INTRA */
      		mb_type &= ~MB_INTRA;

/* no block data */
      		cbp = 0; 
    	}

    	snr_cbp = 0;

/* pixel coordinates of top left corner of current macroblock */
    	bx = 16 * (macroblock_address % video->mb_width);
    	by = 16 * (macroblock_address / video->mb_width);

/* motion compensation */
    	if(!(mb_type & MB_INTRA))
    	  	mpeg3video_reconstruct(video, 
				bx, 
				by, 
				mb_type, 
				motion_type, 
				pmv, 
				mv_field_sel, 
				dmvector, 
				stwtype);

/* copy or add block data into picture */
    	for(comp = 0; comp < video->blk_cnt; comp++)
		{
      		if((cbp | snr_cbp) & (1 << (video->blk_cnt - 1 - comp)))
			{
       			mpeg3video_idct_conversion(slice->block[comp]);

        		mpeg3video_addblock(slice, 
					video, 
					comp, 
					bx, 
					by, 
					dct_type, 
					(mb_type & MB_INTRA) == 0);
      		}
    	}

/* advance to next macroblock */
    	macroblock_address++;
    	mba_inc--;
  	}

	return 0;
}

void mpeg3_slice_loop(mpeg3_slice_t *slice)
{
	mpeg3video_t *video = slice->video;
	int result = 1;

	while(!slice->done)
	{
		pthread_mutex_lock(&(slice->input_lock));

		if(!slice->done)
		{
/* Get a buffer to decode */
			result = 1;
			pthread_mutex_lock(&(video->slice_lock));
			if(slice->buffer_step > 0)
			{
				while(slice->current_buffer <= slice->last_buffer)
				{
					if(!video->slice_buffers[slice->current_buffer].done &&
						slice->current_buffer <= slice->last_buffer)
					{
						result = 0;
						break;
					}
					slice->current_buffer += slice->buffer_step;
				}
			}
			else
			{
				while(slice->current_buffer >= slice->last_buffer)
				{
					if(!video->slice_buffers[slice->current_buffer].done &&
						slice->current_buffer >= slice->last_buffer)
					{
						result = 0;
						break;
					}
					slice->current_buffer += slice->buffer_step;
				}
			}

/* Got one */
			if(!result && slice->current_buffer >= 0 && slice->current_buffer < video->total_slice_buffers)
			{
				slice->slice_buffer = &(video->slice_buffers[slice->current_buffer]);
				slice->slice_buffer->done = 1;
				pthread_mutex_unlock(&(video->slice_lock));
				pthread_mutex_unlock(&(slice->input_lock));
				mpeg3_decode_slice(slice);
				pthread_mutex_unlock(&(slice->slice_buffer->completion_lock));
			}
			else
/* Finished with all */
			{
				pthread_mutex_unlock(&(slice->completion_lock));
				pthread_mutex_unlock(&(video->slice_lock));
			}
		}

		pthread_mutex_unlock(&(slice->output_lock));
	}
}

int mpeg3_new_slice_decoder(void *video, mpeg3_slice_t *slice)
{
	pthread_attr_t  attr;
	pthread_mutexattr_t mutex_attr;

	slice->video = video;
	slice->done = 0;
	pthread_mutexattr_init(&mutex_attr);
//	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	pthread_mutex_init(&(slice->input_lock), &mutex_attr);
	pthread_mutex_lock(&(slice->input_lock));
	pthread_mutex_init(&(slice->output_lock), &mutex_attr);
	pthread_mutex_lock(&(slice->output_lock));
	pthread_mutex_init(&(slice->completion_lock), &mutex_attr);
	pthread_mutex_lock(&(slice->completion_lock));

	pthread_attr_init(&attr);
	pthread_create(&(slice->tid), &attr, (void*)mpeg3_slice_loop, slice);

	return 0;
}

int mpeg3_delete_slice_decoder(mpeg3_slice_t *slice)
{
	slice->done = 1;
	pthread_mutex_unlock(&(slice->input_lock));
	pthread_join(slice->tid, 0);
	pthread_mutex_destroy(&(slice->input_lock));
	pthread_mutex_destroy(&(slice->output_lock));
	return 0;
}
