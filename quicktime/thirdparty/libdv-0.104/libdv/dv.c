/*
 *  dv.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - November 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser Public License
 *  along with libdv; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */

/** @file
 *  @ingroup decoder
 *  @brief Implementation for @link decoder DV Decoder @endlink
 */

/** @weakgroup decoder DV Decoder
 *
 *  This is the toplevel module of the \c liddv decoder.  
 *  
 *  @{
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <pthread.h>

#include "dv.h"
#include "encode.h"
#include "util.h"
#include "audio.h"
#include "idct_248.h"
#include "quant.h"
#include "weighting.h"
#include "vlc.h"
#include "parse.h"
#include "place.h"
#include "rgb.h"
#include "YUY2.h"
#include "YV12.h"
#if ARCH_X86 || ARCH_X86_64
#include "mmx.h"
#endif

#if YUV_420_USE_YV12
#define DV_MB420_YUV(a,b,c)     dv_mb420_YV12    (a,b,c)
#define DV_MB420_YUV_MMX(a,b,c,d,e) dv_mb420_YV12_mmx(a,b,c,d,e)
#else
#define DV_MB420_YUV(a,b,c)     dv_mb420_YUY2    (a,b,c)
#define DV_MB420_YUV_MMX(a,b,c,d,e) dv_mb420_YUY2_mmx(a,b,c,d,e)
#endif 

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)<(b)?(b):(a))

int dv_use_mmx;

#if HAVE_LIBPOPT
static void
dv_decoder_popt_callback(poptContext con, enum poptCallbackReason reason, 
		       const struct poptOption * opt, const char * arg, const void * data)
{
  dv_decoder_t *decoder = (dv_decoder_t *)data;

  if((decoder->arg_video_system < 0) || (decoder->arg_video_system > 3)) {
    dv_opt_usage(con, decoder->option_table, DV_DECODER_OPT_SYSTEM);
  } /* if */
  
} /* dv_decoder_popt_callback  */
#endif /* HAVE_LIBPOPT */

dv_decoder_t * 
dv_decoder_new(int add_ntsc_setup, int clamp_luma, int clamp_chroma) {
  dv_decoder_t *result;
  
  result = (dv_decoder_t *)calloc(1,sizeof(dv_decoder_t));
  if(!result) goto no_mem;
  
  result->add_ntsc_setup = FALSE;
  result->clamp_luma = clamp_luma;
  result->clamp_chroma = clamp_chroma;
  dv_init(clamp_luma, clamp_chroma);
	
  result->video = dv_video_new();
  if(!result->video) goto no_video;
  result->video->dv_decoder = result;

  result->audio = dv_audio_new();
  if(!result->audio) goto no_audio;
  result->audio->dv_decoder = result;

  dv_set_error_log (result, stderr);
  dv_set_audio_correction (result, DV_AUDIO_CORRECT_AVERAGE);
#if HAVE_LIBPOPT
  result->option_table[DV_DECODER_OPT_SYSTEM] = (struct poptOption) {
    longName: "video-system", 
    shortName: 'V', 
    argInfo: POPT_ARG_INT, 
    descrip: "video standard:" 
    "0=autoselect [default]," 
    " 1=525/60 4:1:1 (NTSC),"
    " 2=625/50 4:2:0 (PAL,IEC 61834 DV),"
    " 3=625/50 4:1:1 (PAL,SMPTE 314M DV)",
    argDescrip: "(0|1|2|3)",
    arg: &result->arg_video_system,
  }; /* system */

  result->option_table[DV_DECODER_OPT_VIDEO_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    descrip: "Video decode options",
    arg: &result->video->option_table[0],
  }; /* video include */

  result->option_table[DV_DECODER_OPT_AUDIO_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    descrip: "Audio decode options",
    arg: result->audio->option_table,
  }; /* audio include */

  result->option_table[DV_DECODER_OPT_CALLBACK] = (struct poptOption){
    argInfo: POPT_ARG_CALLBACK|POPT_CBFLAG_POST,
    arg:     dv_decoder_popt_callback,
    descrip: (char *)result, /* data passed to callback */
  }; /* callback */

#endif /* HAVE_LIBPOPT */

  return(result);

 no_audio:
  free(result->video);
 no_video:
  free(result);
  result=NULL;
 no_mem:
  return(result);
} /* dv_decoder_new */


void
dv_decoder_free( dv_decoder_t *decoder)
{
	if (decoder != NULL) {
		if (decoder->audio != NULL) free(decoder->audio);
		if (decoder->video != NULL) free(decoder->video);
		free(decoder);
	}
} /* dv_decoder_free */


void 
dv_init(int clamp_luma, int clamp_chroma) {
  static int done=FALSE;
  if(done) goto init_done;
#if ARCH_X86
  dv_use_mmx = mmx_ok(); 
#endif

  /* decoder */
  _dv_weight_init();
  _dv_dct_init();
  dv_dct_248_init();
  dv_construct_vlc_table();
  dv_parse_init();
  dv_place_init();
  dv_quant_init();
  dv_rgb_init(clamp_luma, clamp_chroma);
  dv_YUY2_init(clamp_luma, clamp_chroma);
  dv_YV12_init(clamp_luma, clamp_chroma);

  /* encoder */
  _dv_init_vlc_test_lookup();
  _dv_init_vlc_encode_lookup();
  _dv_init_qno_start();
  _dv_prepare_reorder_tables();
  
  done=TRUE;
 init_done:
  return;
} /* dv_init */


void
dv_reconfigure(int clamp_luma, int clamp_chroma) {
  dv_rgb_init( clamp_luma, clamp_chroma);
  dv_YUY2_init( clamp_luma, clamp_chroma);
  dv_YV12_init( clamp_luma, clamp_chroma);
} /* dv_reconfigure */


static inline void 
dv_decode_macroblock(dv_decoder_t *dv, dv_macroblock_t *mb, unsigned int quality) {
  int i;
  for (i=0;
       i<((quality & DV_QUALITY_COLOR) ? 6 : 4);
       i++) {
    if (mb->b[i].dct_mode == DV_DCT_248) {
	dv_248_coeff_t co248[64];

      _dv_quant_248_inverse (mb->b[i].coeffs, mb->qno, mb->b[i].class_no, co248);
      dv_idct_248 (co248, mb->b[i].coeffs);
    } else {
#if ARCH_X86
      _dv_quant_88_inverse_x86(mb->b[i].coeffs,mb->qno,mb->b[i].class_no);
      _dv_idct_88(mb->b[i].coeffs);
#elif ARCH_X86_64
      _dv_quant_88_inverse_x86_64(mb->b[i].coeffs,mb->qno,mb->b[i].class_no);
      _dv_idct_88(mb->b[i].coeffs);
#else /* ARCH_X86 */
      _dv_quant_88_inverse(mb->b[i].coeffs,mb->qno,mb->b[i].class_no);
      _dv_weight_88_inverse(mb->b[i].coeffs);
      _dv_idct_88(mb->b[i].coeffs);
#endif /* ARCH_X86 */
    } /* else */
  } /* for b */
} /* dv_decode_macroblock */

void 
dv_decode_video_segment(dv_decoder_t *dv, dv_videosegment_t *seg, unsigned int quality) {
  dv_macroblock_t *mb;
  dv_block_t *bl;
  int m, b;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    for (b=0,bl = mb->b;
	 b<((quality & DV_QUALITY_COLOR) ? 6 : 4);
	 b++,bl++) {
      if (bl->dct_mode == DV_DCT_248) {
	  dv_248_coeff_t co248[64];

	_dv_quant_248_inverse (mb->b[b].coeffs, mb->qno, mb->b[b].class_no, co248);
	dv_idct_248 (co248, mb->b[b].coeffs);
      } else {
#if ARCH_X86
	_dv_quant_88_inverse_x86(bl->coeffs,mb->qno,bl->class_no);
	_dv_weight_88_inverse(bl->coeffs);
	_dv_idct_88(bl->coeffs);
#elif ARCH_X86_64
	_dv_quant_88_inverse_x86_64(bl->coeffs,mb->qno,bl->class_no);
	_dv_weight_88_inverse(bl->coeffs);
	_dv_idct_88(bl->coeffs);
#else /* ARCH_X86 */
	_dv_quant_88_inverse(bl->coeffs,mb->qno,bl->class_no);
	_dv_weight_88_inverse(bl->coeffs);
	_dv_idct_88(bl->coeffs);
#endif /* ARCH_X86 */
      } /* else */
    } /* for b */
  } /* for mb */
} /* dv_decode_video_segment */

static inline void
dv_render_macroblock_rgb(dv_decoder_t *dv, dv_macroblock_t *mb, uint8_t **pixels, int *pitches ) {
  if(dv->sampling == e_dv_sample_411) {
    if(mb->x >= 704) {
      dv_mb411_right_rgb(mb, pixels, pitches, dv->add_ntsc_setup); /* Right edge are 16x16 */
    } else {
      dv_mb411_rgb(mb, pixels, pitches, dv->add_ntsc_setup);
    } /* else */
  } else {
    dv_mb420_rgb(mb, pixels, pitches);
  } /* else */
} /* dv_render_macroblock_rgb */

void
dv_render_video_segment_rgb(dv_decoder_t *dv, dv_videosegment_t *seg, uint8_t **pixels, int *pitches ) {
  dv_macroblock_t *mb;
  int m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_rgb(mb, pixels, pitches, dv->add_ntsc_setup); /* Right edge are 16x16 */
      } else {
	dv_mb411_rgb(mb, pixels, pitches, dv->add_ntsc_setup);
      } /* else */
    } else {
      dv_mb420_rgb(mb, pixels, pitches);
    } /* else */
  } /* for    */
} /* dv_render_video_segment_rgb */

static inline void
dv_render_macroblock_bgr0(dv_decoder_t *dv, dv_macroblock_t *mb, uint8_t **pixels, int *pitches ) {
  if(dv->sampling == e_dv_sample_411) {
    if(mb->x >= 704) {
      dv_mb411_right_bgr0(mb, pixels, pitches, dv->add_ntsc_setup); /* Right edge are 16x16 */
    } else {
      dv_mb411_bgr0(mb, pixels, pitches, dv->add_ntsc_setup);
    } /* else */
  } else {
    dv_mb420_bgr0(mb, pixels, pitches);
  } /* else */
} /* dv_render_macroblock_bgr0 */

void
dv_render_video_segment_bgr0(dv_decoder_t *dv, dv_videosegment_t *seg, uint8_t **pixels, int *pitches ) {
  dv_macroblock_t *mb;
  int m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_bgr0(mb, pixels, pitches, dv->add_ntsc_setup); /* Right edge are 16x16 */
      } else {
	dv_mb411_bgr0(mb, pixels, pitches, dv->add_ntsc_setup);
      } /* else */
    } else {
      dv_mb420_bgr0(mb, pixels, pitches);
    } /* else */
  } /* for    */
} /* dv_render_video_segment_bgr0 */

#if ARCH_X86 || ARCH_X86_64

static inline void
dv_render_macroblock_yuv(dv_decoder_t *dv, dv_macroblock_t *mb, uint8_t **pixels, int *pitches) {
  if(dv_use_mmx) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_YUY2_mmx(mb, pixels, pitches,
	      dv->add_ntsc_setup, dv->clamp_luma, dv->clamp_chroma); /* Right edge are 420! */
      } else {
	dv_mb411_YUY2_mmx(mb, pixels, pitches,
	      dv->add_ntsc_setup, dv->clamp_luma, dv->clamp_chroma);
      } /* else */
    } else {
      DV_MB420_YUV_MMX(mb, pixels, pitches, dv->clamp_luma, dv->clamp_chroma);
    } /* else */
  } else {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_YUY2(mb, pixels, pitches, dv->add_ntsc_setup); /* Right edge are 420! */
      } else {
	dv_mb411_YUY2(mb, pixels, pitches, dv->add_ntsc_setup);
      } /* else */
    } else {
      DV_MB420_YUV(mb, pixels, pitches);
    } /* else */
  } /* else */
} /* dv_render_macroblock_yuv */

void
dv_render_video_segment_yuv(dv_decoder_t *dv, dv_videosegment_t *seg, uint8_t **pixels, int *pitches) {
  dv_macroblock_t *mb;
  int m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv_use_mmx) {
      if(dv->sampling == e_dv_sample_411) {
	if(mb->x >= 704) {
	  dv_mb411_right_YUY2_mmx(mb, pixels, pitches,
		dv->add_ntsc_setup, dv->clamp_luma, dv->clamp_chroma); /* Right edge are 420! */
	} else {
	  dv_mb411_YUY2_mmx(mb, pixels, pitches,
		dv->add_ntsc_setup, dv->clamp_luma, dv->clamp_chroma);
	} /* else */
      } else {
	DV_MB420_YUV_MMX(mb, pixels, pitches, dv->clamp_luma, dv->clamp_chroma);
      } /* else */
    } else {
      if(dv->sampling == e_dv_sample_411) {
	if(mb->x >= 704) {
	  dv_mb411_right_YUY2(mb, pixels, pitches, dv->add_ntsc_setup); /* Right edge are 420! */
	} else {
	  dv_mb411_YUY2(mb, pixels, pitches, dv->add_ntsc_setup);
	} /* else */
      } else {
	DV_MB420_YUV(mb, pixels, pitches);
      } /* else */
    } /* else */
  } /* for    */
} /* dv_render_video_segment_yuv */

#else /* ARCH_X86 */

static inline void
dv_render_macroblock_yuv(dv_decoder_t *dv, dv_macroblock_t *mb, uint8_t **pixels, int *pitches) {
  if(dv->sampling == e_dv_sample_411) {
    if(mb->x >= 704) {
      dv_mb411_right_YUY2(mb, pixels, pitches, dv->add_ntsc_setup); /* Right edge are 420! */
    } else {
      dv_mb411_YUY2(mb, pixels, pitches, dv->add_ntsc_setup);
    } /* else */
  } else {
    DV_MB420_YUV(mb, pixels, pitches);
  } /* else */
} /* dv_render_macroblock_yuv */

void
dv_render_video_segment_yuv(dv_decoder_t *dv, dv_videosegment_t *seg, uint8_t **pixels, int *pitches) {
  dv_macroblock_t *mb;
  int m;
  for (m=0,mb = seg->mb;
       m<5;
       m++,mb++) {
    if(dv->sampling == e_dv_sample_411) {
      if(mb->x >= 704) {
	dv_mb411_right_YUY2(mb, pixels, pitches, dv->add_ntsc_setup); /* Right edge are 420! */
      } else {
	dv_mb411_YUY2(mb, pixels, pitches, dv->add_ntsc_setup);
      } /* else */
    } else {
      DV_MB420_YUV(mb, pixels, pitches);
    } /* else */
  } /* for    */
} /* dv_render_video_segment_yuv */

#endif /* ! ARCH_X86 */

static int32_t ranges[6][2];

void dv_check_coeff_ranges(dv_macroblock_t *mb) {
  dv_block_t *bl;
  int b, i;
  for (b=0,bl = mb->b;
       b<6;
       b++,bl++) {
    for(i=0;i<64;i++) {
      ranges[b][0] = MIN(ranges[b][0],bl->coeffs[i]);
      ranges[b][1] = MAX(ranges[b][1],bl->coeffs[i]);
    }
  }
}

void
dv_decode_full_frame(dv_decoder_t *dv, const uint8_t *buffer,
		     dv_color_space_t color_space, uint8_t **pixels, int *pitches) {

  bitstream_t bs = { 0 };
  dv_videosegment_t vs = { 0, 0, &bs };
  static pthread_mutex_t dv_mutex = PTHREAD_MUTEX_INITIALIZER;
  dv_videosegment_t *seg = &vs;
  dv_macroblock_t *mb;
  int ds, v, m;
  unsigned int offset = 0, dif = 0, audio=0;

  pthread_mutex_lock(&dv_mutex);
  seg->isPAL = (dv->system == e_dv_system_625_50);

  /* each DV frame consists of a sequence of DIF segments  */
  for (ds=0; ds < dv->num_dif_seqs; ds++) {
    /** Each DIF segment conists of 150 dif blocks, 135 of which are video blocks
       A video segment consists of 5 video blocks, where each video
       block contains one compressed macroblock.  DV bit allocation
       for the VLC stage can spill bits between blocks in the same
       video segment.  So parsing needs the whole segment to decode
       the VLC data
	*/
    dif += 6;
    audio=0;
    /* Loop through video segments  */
    for (v=0;v<27;v++) {
      /* skip audio block - interleaved before every 3rd video segment */
      if(!(v % 3)) {
        /*dv_dump_aaux_as(buffer+(dif*80), ds, audio); */
	dif++;
	audio++;
      } /* if */
      /* stage 1: parse and VLC decode 5 macroblocks that make up a video segment */
      offset = dif * 80;
      _dv_bitstream_new_buffer(seg->bs, (uint8_t *)buffer + offset, 80*5);
      dv_parse_video_segment(seg, dv->quality);
      /* stage 2: dequant/unweight/iDCT blocks, and place the macroblocks */
      dif+=5;
      seg->i = ds;
      seg->k = v;

      switch(color_space) {
      case e_dv_color_yuv:
	for (m=0,mb = seg->mb;
	     m<5;
	     m++,mb++) {
	  dv_decode_macroblock(dv, mb, dv->quality);
	  dv_place_macroblock(dv, seg, mb, m);
	  dv_render_macroblock_yuv(dv, mb, pixels, pitches);
	} /* for m */
	break;
      case e_dv_color_bgr0:
	for (m=0,mb = seg->mb;
	     m<5;
	     m++,mb++) {
	  dv_decode_macroblock(dv, mb, dv->quality);
	  dv_place_macroblock(dv, seg, mb, m);
	  dv_render_macroblock_bgr0(dv, mb, pixels, pitches);
	} /* for m */
        break;
      case e_dv_color_rgb:
	for (m=0,mb = seg->mb;
	     m<5;
	     m++,mb++) {
	  dv_decode_macroblock(dv, mb, dv->quality);
	  dv_place_macroblock(dv, seg, mb, m);
#if RANGE_CHECKING
	  dv_check_coeff_ranges(mb);
#endif
	  dv_render_macroblock_rgb(dv, mb, pixels, pitches);
	} /* for m */
	break;
      } /* switch */

    } /* for v */

  } /* ds */
  pthread_mutex_unlock(&dv_mutex);

#if RANGE_CHECKING
  for(i=0;i<6;i++) {
    fprintf(stderr, "range[%d] min %d max %d\n", i, ranges[i][0], ranges[i][1]);
  }
#endif
} /* dv_decode_full_frame  */

/* ---------------------------------------------------------------------------
 */
int
dv_frame_has_audio_errors (dv_decoder_t *dv)
{
  return (dv -> audio -> block_failure);
} /* dv_has_audio_errors */

/* ---------------------------------------------------------------------------
 */
int
dv_decode_full_audio(dv_decoder_t *dv, const uint8_t *buffer, int16_t **outbufs)
{
  int ds, dif, audio_dif, result;

  dif=0;
  if (!dv_parse_audio_header (dv, buffer))
  {
    goto no_audio;
  }
  dv -> audio -> block_failure = dv -> audio -> sample_failure = 0;
  for (ds = 0; ds < dv -> num_dif_seqs; ds++) {
    dif += 6;
    for (audio_dif = 0; audio_dif < 9; audio_dif++) {
      if ((result = dv_decode_audio_block (dv -> audio,
                                           buffer + (dif * 80),
                                           ds,
                                           audio_dif,
                                           outbufs)))
      {
        goto fail;
      }
      dif += 16;
    } /* for  */
  } /* for */

  if (dv -> audio -> sample_failure) {
    if (dv -> audio -> error_log) {
      fprintf (dv -> audio -> error_log,
               "# audio block/sample failure for %d blocks, %d samples of %d\n",
               dv -> audio -> block_failure,
               dv -> audio -> sample_failure,
               dv -> audio -> raw_samples_this_frame [0]); /* TODO: second channel */
    }
    dv_audio_correct_errors (dv -> audio, outbufs);
  }

  dv_audio_deemphasis (dv -> audio, outbufs);

  dv_audio_mix4ch (dv -> audio, outbufs);

  return(TRUE);

 fail:
fprintf (stderr, "# decode failure \n");
 no_audio:
fprintf (stderr, "# no audio\n");
  return(FALSE);

} /* dv_decode_full_audio */

/* ---------------------------------------------------------------------------
 */
void
dv_report_video_error (dv_decoder_t *dv, uint8_t *data)
{
  int i, error_code;

  if (!dv -> video -> error_log)
    return;
  /* -------------------------------------------------------------------------
   * got through all packets
   */
  for (i = 0; i < dv -> frame_size; i += 80) {
    /* -----------------------------------------------------------------------
     * check that's a video packet
     */
    if ((data [i] & 0xe0) == 0x80) {
      error_code = data [i + 3] >> 4;
#if 0
      if (error_code) {
        char err_msg1 [40], err_msg2 [40];
        dv_get_timestamp (dv, err_msg1);
        dv_get_recording_datetime (dv, err_msg2);
        fprintf (dv -> video -> error_log,
                 "%s %s %s %02x %02x %02x %02x\n",
/*                 (error_code < 8) ? "vel" : "veh", */
                 "ver",
                 err_msg1,
                 err_msg2,
                 data [i + 0],
                 data [i + 1],
                 data [i + 2],
                 data [i + 3]);
      }
#endif
    }
  }
}

/* ---------------------------------------------------------------------------
 */
int
dv_is_PAL (dv_decoder_t *dv)
{
  return dv -> system == e_dv_system_625_50;
}

/* ---------------------------------------------------------------------------
 */
int
dv_set_quality (dv_decoder_t *dv, int q)
{
    int old_q = dv -> quality;

  dv -> quality = q;
  return old_q;
}

/* ---------------------------------------------------------------------------
 * query functions based upon VAUX data
 */

/* ---------------------------------------------------------------------------
 */
int
dv_get_vaux_pack (dv_decoder_t *dv, uint8_t pack_id, uint8_t *data)
{
  uint8_t  id;
  if ((id = dv -> vaux_pack [pack_id]) == 0xff) 
    return -1;
  memcpy (data, dv -> vaux_data [id], 4);
  return 0;
} /* dv_get_vaux_pack */

/* ---------------------------------------------------------------------------
 */
int
dv_get_ssyb_pack (dv_decoder_t *dv, uint8_t pack_id, uint8_t *data)
{
  uint8_t  id;
  if ((id = dv -> ssyb_pack [pack_id]) == 0xff) 
    return -1;
  memcpy (data, dv -> ssyb_data [id], 4);
  return 0;
} /* dv_get_vaux_pack */

/* ---------------------------------------------------------------------------
 */
int
dv_frame_is_color (dv_decoder_t *dv)
{
  uint8_t  id;

  if ((id = dv -> vaux_pack [0x60]) != 0xff) {
    if (dv -> vaux_data [id] [1] & 0x80) {
      return 1;
    }
    return 0;
  }
  return -1;
}

/* ---------------------------------------------------------------------------
 */
int
dv_system_50_fields (dv_decoder_t *dv)
{
  uint8_t  id;

  if ((id = dv -> vaux_pack [0x60]) != 0xff) {
    if (dv -> vaux_data [id] [2] & 0x20) {
      return 1;
    }
    return 0;
  }
  return -1;
}

/* ---------------------------------------------------------------------------
 */
int
dv_format_normal (dv_decoder_t *dv)
{
  int result = dv_format_wide(dv);
  return (result == -1) ? -1 : !result;
}

/* ---------------------------------------------------------------------------
 */
int
dv_format_wide (dv_decoder_t *dv)
{
  uint8_t  id;

  if ((id = dv -> vaux_pack [0x61]) != 0xff) {
    if (dv->std == e_dv_std_smpte_314m)
      return ((dv->vaux_data[id][1] & 0x7) == 0x2);
    else
      return (((dv->vaux_data[id][1] & 0x7) == 0x2) || ((dv->vaux_data[id][1] & 0x7) == 0x7)) ;
  }
  return -1;
}

/* ---------------------------------------------------------------------------
 */
int
dv_format_letterbox (dv_decoder_t *dv)
{
  uint8_t  id;

  if ((id = dv -> vaux_pack [0x61]) != 0xff) {
    /* FIXME: what SMPTE 314M implementations support a letterbox format? */
    if ((dv->vaux_data[id][1] & 0x07) == 0x3) {
      return 1;
    }
    return 0;
  }
  return -1;
}

/* ---------------------------------------------------------------------------
 */
int
dv_frame_changed (dv_decoder_t *dv)
{
  uint8_t  id;

  if ((id = dv -> vaux_pack [0x61]) != 0xff) {
    if (dv -> vaux_data [id] [2] & 0x20) {
      return 1;
    }
    return 0;
  }
  return -1;
}

/* ---------------------------------------------------------------------------
 */
int
dv_is_progressive (dv_decoder_t *dv)
{
  uint8_t  id;

  if ((id = dv -> vaux_pack [0x61]) != 0xff) {
    if (dv -> vaux_data [id] [2] & 0x08) {
      return 0;
    }
    return 1;
  }
  return -1;
}

/* ---------------------------------------------------------------------------
 * query functions based upon SSYB data
 * decoding code is taken from kino
 */

/* ---------------------------------------------------------------------------
 */
int
dv_get_timestamp (dv_decoder_t *dv, char *tstptr)
{
    int  id;

  if ((id = dv -> ssyb_pack [0x13]) != 0xff) {
    sprintf (tstptr,
             "%02d:%02d:%02d.%02d",
             ((dv -> ssyb_data [id] [3] >> 4) & 0x03) * 10 +
              (dv -> ssyb_data [id] [3] & 0x0f),
             ((dv -> ssyb_data [id] [2] >> 4) & 0x07) * 10 +
              (dv -> ssyb_data [id] [2] & 0x0f),
             ((dv -> ssyb_data [id] [1] >> 4) & 0x07) * 10 +
              (dv -> ssyb_data [id] [1] & 0x0f),
             ((dv -> ssyb_data [id] [0] >> 4) & 0x03) * 10 +
              (dv -> ssyb_data [id] [0] & 0x0f));

    return 1;
  }
  strcpy (tstptr, "00:00:00.00");
  return 0;
}

/* ---------------------------------------------------------------------------
 * timecode is an array of 4 ints
 */
int
dv_get_timestamp_int (dv_decoder_t *dv, int *timestamp)
{
    int  id;

  if ((id = dv -> ssyb_pack [0x13]) != 0xff) {
    timestamp[0] = ((dv -> ssyb_data [id] [3] >> 4) & 0x03) * 10 +
              (dv -> ssyb_data [id] [3] & 0x0f);
	  
    timestamp[1] = ((dv -> ssyb_data [id] [2] >> 4) & 0x07) * 10 +
              (dv -> ssyb_data [id] [2] & 0x0f);
	  
    timestamp[2] = ((dv -> ssyb_data [id] [1] >> 4) & 0x07) * 10 +
              (dv -> ssyb_data [id] [1] & 0x0f);
	  
    timestamp[3] = ((dv -> ssyb_data [id] [0] >> 4) & 0x03) * 10 +
              (dv -> ssyb_data [id] [0] & 0x0f);

    return 1;
  }
  return 0;
}

/* ---------------------------------------------------------------------------
 */
int
dv_get_recording_datetime (dv_decoder_t *dv, char *dtptr)
{
    int  id1, id2, year;

  if ((id1 = dv -> ssyb_pack [0x62]) != 0xff &&
      (id2 = dv -> ssyb_pack [0x63]) != 0xff) {
    year = dv -> ssyb_data [id1] [3];
    year = (year & 0x0f) + 10 * ((year >> 4) & 0x0f);
    year += (year < 25) ? 2000 : 1900;
    sprintf (dtptr,
             "%04d-%02d-%02d %02d:%02d:%02d",
             year,
             ((dv -> ssyb_data [id1] [2] >> 4) & 0x01) * 10 +
               (dv -> ssyb_data [id1] [2] & 0x0f),
             ((dv -> ssyb_data [id1] [1] >> 4) & 0x03) * 10 +
               (dv -> ssyb_data [id1] [1] & 0x0f),
             ((dv -> ssyb_data [id2] [3] >> 4) & 0x03) * 10 +
              (dv -> ssyb_data [id2] [3] & 0x0f),
             ((dv -> ssyb_data [id2] [2] >> 4) & 0x07) * 10 +
              (dv -> ssyb_data [id2] [2] & 0x0f),
             ((dv -> ssyb_data [id2] [1] >> 4) & 0x07) * 10 +
              (dv -> ssyb_data [id2] [1] & 0x0f));
    return 1;
  }
  
  /* it may also be found in vaux for some... */
  if ((id1 = dv -> vaux_pack [0x62]) != 0xff &&
      (id2 = dv -> vaux_pack [0x63]) != 0xff) {
    year = dv -> vaux_data [id1] [3];
    year = (year & 0x0f) + 10 * ((year >> 4) & 0x0f);
    year += (year < 25) ? 2000 : 1900;
    sprintf (dtptr,
             "%04d-%02d-%02d %02d:%02d:%02d",
             year,
             ((dv -> vaux_data [id1] [2] >> 4) & 0x01) * 10 +
               (dv -> vaux_data [id1] [2] & 0x0f),
             ((dv -> vaux_data [id1] [1] >> 4) & 0x03) * 10 +
               (dv -> vaux_data [id1] [1] & 0x0f),
             ((dv -> vaux_data [id2] [3] >> 4) & 0x03) * 10 +
              (dv -> vaux_data [id2] [3] & 0x0f),
             ((dv -> vaux_data [id2] [2] >> 4) & 0x07) * 10 +
              (dv -> vaux_data [id2] [2] & 0x0f),
             ((dv -> vaux_data [id2] [1] >> 4) & 0x07) * 10 +
              (dv -> vaux_data [id2] [1] & 0x0f));
    return 1;
  }
  strcpy (dtptr, "0000-00-00 00:00:00");
  return 0;
}

/* ---------------------------------------------------------------------------
 */
int
dv_get_recording_datetime_tm (dv_decoder_t *dv, struct tm *rec_dt)
{
    int  id1, id2, year;

  if ((id1 = dv -> ssyb_pack [0x62]) != 0xff &&
      (id2 = dv -> ssyb_pack [0x63]) != 0xff) {
    year = dv -> ssyb_data [id1] [3];
    year = (year & 0x0f) + 10 * ((year >> 4) & 0x0f);
    year += (year < 25) ? 2000 : 1900;
    rec_dt -> tm_isdst = rec_dt -> tm_yday = rec_dt -> tm_wday = -1;
    rec_dt -> tm_year = year - 1900;
    rec_dt -> tm_mon = ((dv -> ssyb_data [id1] [2] >> 4) & 0x01) * 10 +
                        (dv -> ssyb_data [id1] [2] & 0x0f) - 1;
    rec_dt -> tm_mday = ((dv -> ssyb_data [id1] [1] >> 4) & 0x03) * 10 +
                         (dv -> ssyb_data [id1] [1] & 0x0f);
    rec_dt -> tm_hour = ((dv -> ssyb_data [id2] [3] >> 4) & 0x03) * 10 +
                         (dv -> ssyb_data [id2] [3] & 0x0f);
    rec_dt -> tm_min  = ((dv -> ssyb_data [id2] [2] >> 4) & 0x07) * 10 +
                         (dv -> ssyb_data [id2] [2] & 0x0f);
    rec_dt -> tm_sec  = ((dv -> ssyb_data [id2] [1] >> 4) & 0x07) * 10 +
                         (dv -> ssyb_data [id2] [1] & 0x0f);
    /* -----------------------------------------------------------------------
     * sanity check
     */
    if (mktime (rec_dt) == -1)
      return 0;

    return 1;
  }
  
  /* it may also be found in vaux for some... */
  if ((id1 = dv -> vaux_pack [0x62]) != 0xff &&
      (id2 = dv -> vaux_pack [0x63]) != 0xff) {
    year = dv -> vaux_data [id1] [3];
    year = (year & 0x0f) + 10 * ((year >> 4) & 0x0f);
    year += (year < 25) ? 2000 : 1900;
    rec_dt -> tm_isdst = rec_dt -> tm_yday = rec_dt -> tm_wday = -1;
    rec_dt -> tm_year = year - 1900;
    rec_dt -> tm_mon = ((dv -> vaux_data [id1] [2] >> 4) & 0x01) * 10 +
                        (dv -> vaux_data [id1] [2] & 0x0f) - 1;
    rec_dt -> tm_mday = ((dv -> vaux_data [id1] [1] >> 4) & 0x03) * 10 +
                         (dv -> vaux_data [id1] [1] & 0x0f);
    rec_dt -> tm_hour = ((dv -> vaux_data [id2] [3] >> 4) & 0x03) * 10 +
                         (dv -> vaux_data [id2] [3] & 0x0f);
    rec_dt -> tm_min  = ((dv -> vaux_data [id2] [2] >> 4) & 0x07) * 10 +
                         (dv -> vaux_data [id2] [2] & 0x0f);
    rec_dt -> tm_sec  = ((dv -> vaux_data [id2] [1] >> 4) & 0x07) * 10 +
                         (dv -> vaux_data [id2] [1] & 0x0f);
    /* -----------------------------------------------------------------------
     * sanity check
     */
    if (mktime (rec_dt) == -1)
      return 0;

    return 1;
  }
  return 0;
}

/* ---------------------------------------------------------------------------
 */
FILE *
dv_set_error_log (dv_decoder_t *dv, FILE *log)
{
    FILE *old_log;

  old_log = dv -> audio -> error_log;
  dv -> audio -> error_log =
    dv -> video -> error_log =
      log;
  return old_log;
}

/*@}*/
