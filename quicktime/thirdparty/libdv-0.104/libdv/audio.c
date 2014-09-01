/*
 *  audio.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - January 2001
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "dv.h"
#include "util.h"
#include "audio.h"

int dv_is_normal_speed (dv_decoder_t*);

/** @file
 *  @ingroup decoder
 *  @brief Audio routines for decoding
 *
 * DV audio data is shuffled within the frame data.  The unshuffle 
 * tables are indexed by DIF sequence number, then audio DIF block number.
 * The first audio channel (pair) is in the first half of DIF sequences, and the
 * second audio channel (pair) is in the second half.
 *
 * DV audio can be sampled at 48, 44.1, and 32 kHz.  Normally, samples
 * are quantized to 16 bits, with 2 channels.  In 32 kHz mode, it is
 * possible to support 4 channels by non-linearly quantizing to 12
 * bits.
 *
 * The code here always returns 16 bit samples.  In the case of 12
 * bit, we upsample to 16 according to the DV standard defined
 * mapping.
 *
 * DV audio can be "locked" or "unlocked".  In unlocked mode, the
 * number of audio samples per video frame can vary somewhat.  Header
 * info in the audio sections (AAUX AS) is used to tell the decoder on
 * a frame be frame basis how many samples are present. 
 */

static int dv_audio_unshuffle_60[5][9] = {
  { 0, 15, 30, 10, 25, 40,  5, 20, 35 },
  { 3, 18, 33, 13, 28, 43,  8, 23, 38 },
  { 6, 21, 36,  1, 16, 31, 11, 26, 41 },
  { 9, 24, 39,  4, 19, 34, 14, 29, 44 },
  {12, 27, 42,  7, 22, 37,  2, 17, 32 },
};

static int dv_audio_unshuffle_50[6][9] = {
  {  0, 18, 36, 13, 31, 49,  8, 26, 44 },
  {  3, 21, 39, 16, 34, 52, 11, 29, 47 },
  {  6, 24, 42,  1, 19, 37, 14, 32, 50 }, 
  {  9, 27, 45,  4, 22, 40, 17, 35, 53 }, 
  { 12, 30, 48,  7, 25, 43,  2, 20, 38 },
  { 15, 33, 51, 10, 28, 46,  5, 23, 41 },
};

/* Minumum number of samples, indexed by system (PAL/NTSC) and
   sampling rate (32 kHz, 44.1 kHz, 48 kHz) */

static int min_samples[2][3] = {
  { 1580, 1452, 1053 }, /* 60 fields (NTSC) */
  { 1896, 1742, 1264 }, /* 50 fields (PAL) */
};

static int max_samples[2][3] = {
  { 1620, 1489, 1080 }, /* 60 fields (NTSC) */
  { 1944, 1786, 1296 }, /* 50 fields (PAL) */
};

static int frequency[8] = {
  48000, 44100, 32000, -3, -4, -5, -6, -7
};


#ifdef __GNUC__
static int quantization[8] = {
  [0] 16,
  [1] 12,
  [2] 20,
  [3 ... 7] = -1,
};
#else /* ! __GNUC__ */
static int quantization[8] = {
  16,
  12,
  20,
  -1,
  -1,
  -1,
  -1,
  -1,
};
#endif /* ! __GNUC__ */



#if HAVE_LIBPOPT
static void
dv_audio_popt_callback(poptContext con, enum poptCallbackReason reason, 
		       const struct poptOption * opt, const char * arg, const void * data)
{
  dv_audio_t *audio = (dv_audio_t *)data;

  if((audio->arg_audio_frequency < 0) || (audio->arg_audio_frequency > 3)) {
    dv_opt_usage(con, audio->option_table, DV_AUDIO_OPT_FREQUENCY);
  } /* if */
  
  if((audio->arg_audio_quantization < 0) || (audio->arg_audio_quantization > 2)) {
    dv_opt_usage(con, audio->option_table, DV_AUDIO_OPT_QUANTIZATION);
  } /* if */

  if((audio->arg_audio_emphasis < 0) || (audio->arg_audio_emphasis > 2)) {
    dv_opt_usage(con, audio->option_table, DV_AUDIO_OPT_EMPHASIS);
  } /* if */

} /* dv_audio_popt_callback  */
#endif /* HAVE_LIBPOPT */

static int
dv_audio_samples_per_frame(dv_aaux_as_t *dv_aaux_as, int freq) {
  int result = -1;
  int col;
  
  switch(freq) {
  case 48000:
    col = 0;
    break;
  case 44100:
    col = 1;
    break;
  case 32000:
    col = 2;
    break;
  default:
    goto unknown_freq;
    break;
  }
  if(!(dv_aaux_as->pc3.system < 2)) goto unknown_format;

  result = dv_aaux_as->pc1.af_size + min_samples[dv_aaux_as->pc3.system][col];
 done:
  return(result);

 unknown_format:
  fprintf(stderr, "libdv(%s):  badly formed AAUX AS data [pc3.system:%d, pc4.smp:%d]\n",
	  __FUNCTION__, dv_aaux_as->pc3.system, dv_aaux_as->pc4.smp);
  goto done;

 unknown_freq:
  fprintf(stderr, "libdv(%s):  frequency %d not supported\n",
	  __FUNCTION__, freq);
  goto done;
} /* dv_audio_samples_per_frame */

/* Take a DV 12bit audio sample upto 16 bits. 
 * See IEC 61834-2, Figure 16, on p. 129 */

static __inline__ int32_t 
dv_upsample(int32_t sample) {
  int32_t shift, result;
  
  shift = (sample & 0xf00) >> 8;
  switch(shift) {

#ifdef __GNUC__
  case 0x2 ... 0x7:
    shift--;
    result = (sample - (256 * shift)) << shift;
    break;
  case 0x8 ... 0xd:
    shift = 0xe - shift;
    result = ((sample + ((256 * shift) + 1)) << shift) - 1;
    break;
#else /* ! __GNUC__ */
  case 0x2:
  case 0x3:
  case 0x4:
  case 0x5:
  case 0x6:
  case 0x7:
    shift--;
    result = (sample - (256 * shift)) << shift;
    break;
  case 0x8:
  case 0x9:
  case 0xa:
  case 0xb:
  case 0xc:
  case 0xd:
    shift = 0xe - shift;
    result = ((sample + ((256 * shift) + 1)) << shift) - 1;
    break;
#endif /* ! __GNUC__ */

  default:
    result = sample;
    break;
  } /* switch */
  return(result);
} /* dv_upsample */

/* ---------------------------------------------------------------------------
 */
void dv_test12bit_conv (void)
{
    int i;

  for (i = 0; i < 0x7ff; ++i)
  {
    fprintf (stderr, " (%5d,%5d,0x%08x,0x%08x) -> (%5d,%5d,0x%08x,0x%08x) (%d)\n\r",
             i, -i, i, -i,
             dv_upsample (i), dv_upsample(-i), dv_upsample (i), dv_upsample (-i),
             dv_upsample (-i) + dv_upsample (i));
  }
}

/* ---------------------------------------------------------------------------
 */
dv_audio_t *
dv_audio_new(void)
{
  dv_audio_t *result;

  if(!(result = (dv_audio_t *)calloc(1,sizeof(dv_audio_t)))) goto no_mem;

#if HAVE_LIBPOPT
  result->option_table[DV_AUDIO_OPT_FREQUENCY] = (struct poptOption){
    longName:   "frequency",
    shortName:  'f',
    argInfo:    POPT_ARG_INT,
    arg:        &result->arg_audio_frequency,
    descrip:    "audio frequency: 0=autodetect [default], 1=32 kHz, 2=44.1 kHz, 3=48 kHz",
    argDescrip: "(0|1|2|3)"
  }; /* freq */

  result->option_table[DV_AUDIO_OPT_QUANTIZATION] = (struct poptOption){
    longName:   "quantization",
    shortName:  'Q',
    argInfo:    POPT_ARG_INT,
    arg:        &result->arg_audio_quantization,
    descrip:    "audio quantization: 0=autodetect [default], 1=12 bit, 2=16bit",
    argDescrip: "(0|1|2)"
  }; /* quant */

  result->option_table[DV_AUDIO_OPT_EMPHASIS] = (struct poptOption){
    longName:   "emphasis",
    shortName:  'e',
    argInfo:    POPT_ARG_INT,
    arg:        &result->arg_audio_emphasis,
    descrip:    "first-order preemphasis of 50/15 us: 0=autodetect [default], 1=on, 2=off",
    argDescrip: "(0|1|2)"
  }; /* quant */

  result->option_table[DV_AUDIO_OPT_CHAN_MIX] = (struct poptOption) {
    longName:   "audio-mix",
    argInfo:    POPT_ARG_INT,
    arg:        &result->arg_mixing_level,
    descrip:    "mixing level between 1st and 2nd channel for 32kHz 12bit. 0 [default]",
    argDescrip: "(-16 .. 16)",
  };

  result->option_table[DV_AUDIO_OPT_CALLBACK] = (struct poptOption){
    argInfo: POPT_ARG_CALLBACK|POPT_CBFLAG_POST,
    arg:     dv_audio_popt_callback,
    descrip: (char *)result, /* data passed to callback */
  }; /* callback */

#endif /* HAVE_LIBPOPT */
  /* dv_test12bit_conv (); */
  return(result);

 no_mem:
  return(result);
} /* dv_audio_new */

void
dv_dump_aaux_as(void *buffer, int ds, int audio_dif)
{
  dv_aaux_as_t *dv_aaux_as;

  dv_aaux_as = (dv_aaux_as_t *)buffer + 3; /* Is this correct after cast? */

  if(dv_aaux_as->pc0 == 0x50) {
    /* AAUX AS  */

    printf("DS %d, Audio DIF %d, AAUX AS pack: ", ds, audio_dif);

    if(dv_aaux_as->pc1.lf) {
      printf("Unlocked audio");
    } else {
      printf("Locked audio");
    }

    printf(", Sampling ");
    printf("%.1f kHz", (float)frequency[dv_aaux_as->pc4.smp] / 1000.0);

    printf(" (%d samples, %d fields)",
	   dv_audio_samples_per_frame(dv_aaux_as,frequency[dv_aaux_as->pc4.smp]),
	   (dv_aaux_as->pc3.system ? 50 : 60));

    printf(", Quantization %d bits", quantization[dv_aaux_as->pc4.qu]);

    printf(", Emphasis %s\n", (dv_aaux_as->pc4.ef ? "off" : "on"));

  } else {

    fprintf(stderr, "libdv(%s):  Missing AAUX AS PACK!\n", __FUNCTION__);

  } /* else */

} /* dv_dump_aaux_as */

/* ---------------------------------------------------------------------------
 */
void
dv_dump_audio_header (dv_decoder_t *decoder, int ds, uint8_t *inbuf)
{
    int     i;
    uint8_t *p;

  fprintf (stderr, " ");
  for (i = 0, p = inbuf + 80 * 16 * ((ds&1) ? 0: 3); i < 8; i++, p++)
  {
    fprintf (stderr, " %02x ", *p);
  }

  for (i = 0, p = inbuf + 80 * 16 * ((ds & 1) ? 1: 4); i < 8; i++, p++)
  {
    fprintf (stderr, " %02x ", *p);
  }
  fprintf (stderr, "\n");

}

/* ---------------------------------------------------------------------------
 */
int
dv_parse_audio_header(dv_decoder_t *decoder, const uint8_t *inbuf)
{
  dv_audio_t    *audio = decoder->audio;
  dv_aaux_as_t  *dv_aaux_as  = (dv_aaux_as_t *) (inbuf + 80*6+80*16*3 + 3),
                *dv_aaux_as1 = NULL;
  dv_aaux_asc_t *dv_aaux_asc = (dv_aaux_asc_t *)(inbuf + 80*6+80*16*4 + 3),
                *dv_aaux_asc1 = NULL;

  if((dv_aaux_as->pc0 != 0x50) || (dv_aaux_asc->pc0 != 0x51)) goto bad_id;

  audio->max_samples =  max_samples[dv_aaux_as->pc3.system][dv_aaux_as->pc4.smp];
  /* For now we assume that 12bit = 4 channels */
  if(dv_aaux_as->pc4.qu > 1) goto unsupported_quantization;
  /*audio->num_channels = (dv_aaux_as->pc4.qu+1) * 2;
  // TODO verify this is right with known 4-channel input */

  audio->num_channels =
    audio->raw_num_channels = 2; /* TODO verify this is right with known 4-channel input */
                                 /* DONE: see below */
  switch(audio->arg_audio_frequency) {
  case 0:
    audio->frequency = frequency[dv_aaux_as->pc4.smp];
    break;
  case 1:
    audio->frequency = 32000;
    break;
  case 2:
    audio->frequency = 44100;
    break;
  case 3:
    audio->frequency = 44100;
    break;
  }  /* switch  */

  switch(audio->arg_audio_quantization) {
  case 0:
    audio->quantization = quantization[dv_aaux_as->pc4.qu];
    break;
  case 1:
    audio->quantization = 12;
    break;
  case 2:
    audio->quantization = 16;
    break;
  }  /* switch  */

  switch(audio->arg_audio_emphasis) {
  case 0:
    if(decoder->std == e_dv_std_iec_61834) {
      audio->emphasis = (dv_aaux_as->pc4.ef == 0);
    } else if(decoder->std == e_dv_std_smpte_314m) {
      audio->emphasis = (dv_aaux_asc->pc1.ss == 1);
    } else {
      /* TODO: should never happen... */
    }
    break;
  case 1:
    audio->emphasis = TRUE;
    break;
  case 2:
    audio->emphasis = FALSE;
    break;
  } /* switch  */

  /* -------------------------------------------------------------------------
   * so try to detect 4 channel audio mode
   */
  if (audio -> frequency == 32000 && audio -> quantization == 12)
  {
    /* -----------------------------------------------------------------------
     * check different location PAL/NTSC + EVEN/ODD index
     * in even dif sequences as and acs info is at index 3/4
     * in odd def sequences as and asc info is at index 0/1
     * ref: SMPTE 314m, p. 16, table 15
     */
    if (dv_aaux_as -> pc3. system)
    {
      dv_aaux_as1 = (dv_aaux_as_t *) (inbuf +
                      80 * (9 * 16 + 6) * 6 + /* go 6 dif sequences ahead  */
                      80 *  6 +               /* skip 6 header blocks      */
                      80 * 16 * 3 +           /* select block 3 (1a + 15v) */
                      3);                     /* skip id bytes             */
      dv_aaux_asc1 = (dv_aaux_asc_t *) (inbuf +
                      80 * (9 * 16 + 6) * 6 + /* go 6 dif sequences ahead  */
                      80 *  6 +               /* skip 6 header blocks      */
                      80 * 16 * 4 +           /* select block 4 (1a + 15v) */
                      3);                     /* skip id bytes             */
    }
    else
    {
      dv_aaux_as1 = (dv_aaux_as_t *) (inbuf +
                      80 * (9 * 16 + 6) * 6 + /* go 6 dif sequences ahead  */
                      80 *  6 +               /* skip 6 header blocks      */
                      80 * 16 * 0 +           /* select block 0 (1a + 15v) */
                      3);                     /* skip id bytes             */
      dv_aaux_asc1 = (dv_aaux_asc_t *) (inbuf +
                      80 * (9 * 16 + 6) * 6 + /* go 6 dif sequences ahead  */
                      80 *  6 +               /* skip 6 header blocks      */
                      80 * 16 * 1 +           /* select block 1 (1a + 15v) */
                      3);                     /* skip id bytes             */
    }
    if (dv_aaux_as1 -> pc2. audio_mode != 0xf)
    {
      audio -> raw_num_channels = 4;
      audio -> aaux_as1  = *dv_aaux_as1;
      audio -> aaux_asc1 = *dv_aaux_asc1;
    }
  }
  audio -> samples_this_frame =
    audio -> raw_samples_this_frame [0] = dv_audio_samples_per_frame (dv_aaux_as,
                                                                      audio -> frequency);
  if (audio -> raw_num_channels == 4)
  {
    audio -> raw_samples_this_frame [1] = dv_audio_samples_per_frame (dv_aaux_as1,
                                                                      audio -> frequency);
  } else {
    audio -> raw_samples_this_frame [1] = 0;
  }
  audio -> aaux_as  = *dv_aaux_as;
  audio -> aaux_asc = *dv_aaux_asc;

  return dv_is_normal_speed(decoder); /* don't do audio if speed is not 1 */

 bad_id:
  return(FALSE);

 unsupported_quantization:
  fprintf(stderr, "libdv(%s):  Malformrmed AAUX AS? pc4.qu == %d\n",
	  __FUNCTION__, audio->aaux_as.pc4.qu);
  return(FALSE);

} /* dv_parse_audio_header */

int
dv_update_num_samples(dv_audio_t *dv_audio, const uint8_t *inbuf) {

  dv_aaux_as_t *dv_aaux_as = (dv_aaux_as_t *)(inbuf + 80*6+80*16*3 + 3);

  if(dv_aaux_as->pc0 != 0x50) goto bad_id;
  dv_audio->samples_this_frame =
  dv_audio->raw_samples_this_frame[0]=
              dv_audio_samples_per_frame (dv_aaux_as, dv_audio -> frequency);
  /* TODO: update second channels bytes too */
  return(TRUE);

 bad_id:
  return(FALSE);

} /* dv_update_num_samples */

/* This code originates from cdda2wav, by way of Giovanni Iachello <g.iachello@iol.it>
   to Arne Schirmacher <arne@schirmacher.de>. */
void
dv_audio_deemphasis(dv_audio_t *audio, int16_t **outbuf)
{
    int i, ch;
    /* this implements an attenuation treble shelving filter
       to undo the effect of pre-emphasis. The filter is of
       a recursive first order */
    short lastin;
    double lastout;
    short *pmm;
    /* See deemphasis.gnuplot */
    double V0     = 0.3365;
    double OMEGAG = (1./19e-6);
    double T  = (1./audio->frequency);
    double H0 = (V0-1.);
    double B  = (V0*tan((OMEGAG * T)/2.0));
    double a1 = ((B - 1.)/(B + 1.));
    double b0 = (1.0 + (1.0 - a1) * H0/2.0);
    double b1 = (a1 + (a1 - 1.0) * H0/2.0);

  if (audio->emphasis) {
    for (ch=0; ch< audio->raw_num_channels; ch++) {
      /* ---------------------------------------------------------------------
       * For 48Khz:   a1=-0.659065
       *              b0= 0.449605
       *              b1=-0.108670
       * For 44.1Khz: a1=-0.62786881719628784282
       *              b0= 0.45995451989513153057
       *              b1=-0.08782333709141937339
       * For 32kHZ ?
       */
      lastin = audio->lastin [ch];
      lastout = audio->lastout [ch];
      for (pmm = (short *)outbuf [ch], i=0;
           i < audio->raw_samples_this_frame [0]; /* TODO: check for second channel */
           i++) {
        lastout = *pmm * b0 + lastin * b1 - lastout * a1;
        lastin = *pmm;
        *pmm++ = (lastout > 0.0) ? lastout + 0.5 : lastout - 0.5;
      } /* for (pmn = .. */
      audio->lastout [ch] = lastout;
      audio->lastin [ch] = lastin;
    } /* for (ch = .. */
  } /* if (audio -> .. */
} /* dv_audio_deemphasis */

/* ---------------------------------------------------------------------------
 */
int
dv_decode_audio_block(dv_audio_t *dv_audio,
                      const uint8_t *inbuf,
                      int ds,
                      int audio_dif,
                      int16_t **outbufs)
{
  int channel, bp, i_base, i, stride;
  int16_t *samples, *ysamples, *zsamples;
  int16_t y,z;
  int32_t msb_y, msb_z, lsb;
  int half_ds, full_failure;
  char      err_msg1 [40],
            err_msg2 [40];

#if 0
  if ((inbuf[0] & 0xe0) != 0x60) goto bad_id;
#endif

  half_ds = (dv_audio->aaux_as.pc3.system ? 6 : 5);

  if(ds < half_ds) {
    channel = 0;
  } else {
    channel = 1;
    ds -= half_ds;
  } /* else */

  if(dv_audio->aaux_as.pc3.system) {
    /* PAL */
    i_base = dv_audio_unshuffle_50[ds][audio_dif];
    stride = 54;
  } else {
    /* NTSC */
    i_base = dv_audio_unshuffle_60[ds][audio_dif];
    stride = 45;
  } /* else */

  full_failure = 0;

  if(dv_audio->quantization == 16) {
    samples = outbufs[channel];
    for (bp = 8; bp < 80; bp+=2) {

      i = i_base + (bp - 8)/2 * stride;
      if ((samples[i] = ((int16_t)inbuf[bp] << 8) | inbuf[bp+1]) == (int16_t) 0x8000) {
        full_failure++;
      }

    } /* for */
    /* -----------------------------------------------------------------------
     * check if some or all samples in block failed
     */
    if (full_failure) {
      if (dv_audio -> error_log) {
        if (dv_get_timestamp (dv_audio -> dv_decoder, err_msg1) &&
            dv_get_recording_datetime (dv_audio -> dv_decoder, err_msg2)) {
        fprintf (dv_audio -> error_log,
                   "%s %s %s %02x %02x %02x 16 %d/36\n",
                   (full_failure == 36) ? "abf": "asf",
                   err_msg1, err_msg2,
                   inbuf [0], inbuf [1], inbuf [2],
                   full_failure);
        } else {
          fprintf (dv_audio -> error_log,
                   "# audio %s failure (16bit): "
                   "header = %02x %02x %02x\n",
                   (full_failure == 36) ? "block": "sample",
                   inbuf [0], inbuf [1], inbuf [2]);
        }
      }
      if (full_failure == 36) {
        dv_audio -> block_failure++;
      }
    }
    dv_audio -> sample_failure += full_failure;

  } else if(dv_audio->quantization == 12) {

    /* See 61834-2 figure 18, and text of section 6.5.3 and 6.7.2 */

    ysamples = outbufs[channel * 2];
    zsamples = outbufs[1 + channel * 2];

    for (bp = 8; bp < 80; bp+=3) {

      i = i_base + (bp - 8)/3 * stride;

      msb_y = inbuf[bp];
      msb_z = inbuf[bp+1];
      lsb   = inbuf[bp+2];

      y = ((msb_y << 4) & 0xff0) | ((lsb >> 4) & 0xf);
      z = ((msb_z << 4) & 0xff0) | (lsb & 0xf);

      if(y > 2048) y -= 4096;
      if(z > 2048) z -= 4096;
      /* ---------------------------------------------------------------------
       * so check if a sample has an error value 0x800 and keep this code
       * for later correction.
       */
      if (y == 2048) {
        full_failure++;
        ysamples[i] = 0x8000;
      } else {
        ysamples[i] = dv_upsample(y);
      }
      if (z == 2048) {
        full_failure++;
        zsamples[i] = 0x8000;
      } else {
        zsamples[i] = dv_upsample(z);
      }
    } /* for  */
    /* -----------------------------------------------------------------------
     * check if some or all samples in block failed
     */
    if (full_failure) {
      if (dv_audio -> error_log) {
        if (dv_get_timestamp (dv_audio -> dv_decoder, err_msg1) &&
            dv_get_recording_datetime (dv_audio -> dv_decoder, err_msg2)) {
        fprintf (dv_audio -> error_log,
                   "%s %s %s %02x %02x %02x 12 %d/48\n",
                   (full_failure == 48) ? "abf": "asf",
                   err_msg1, err_msg2,
                   inbuf [0], inbuf [1], inbuf [2], full_failure);
        } else {
          fprintf (dv_audio -> error_log,
                   "# audio %s failure (12bit): "
                   "header = %02x %02x %02x\n",
                   (full_failure == 48) ? "block": "sample",
                   inbuf [0], inbuf [1], inbuf [2]);
        }
      }
      if (full_failure == 48) {
        dv_audio -> block_failure++;
      }
    }
    dv_audio -> sample_failure += full_failure;

  } else {
    goto unsupported_sampling;
  } /* else */

  return(0);

 unsupported_sampling:
  fprintf(stderr, "libdv(%s):  unsupported audio sampling.\n", __FUNCTION__);
  return(-1);

#if 0
 bad_id:
  fprintf(stderr, "libdv(%s):  not an audio block\n", __FUNCTION__);
  return(-1);
#endif

} /* dv_decode_audio_block */

/* ---------------------------------------------------------------------------
 */
void
dv_audio_correct_errors (dv_audio_t *dv_audio, int16_t **outbufs)
{
    int      num_ch, i, k, cnt;
    int16_t  *dptr, *sptr, last_valid, next_valid, diff;

  switch (dv_audio -> correction_method) {
  case DV_AUDIO_CORRECT_SILENCE:
    for (num_ch = 0; num_ch < dv_audio -> raw_num_channels; ++num_ch) {
      dptr = sptr = outbufs [num_ch];
      for (i = k = 0; i < dv_audio -> raw_samples_this_frame [num_ch / 2]; ++i) {
        if (*sptr == (int16_t) 0x8000) {
          ++k;
          ++sptr;
        } else {
          *dptr++ = *sptr++;
        }
      }
      if (k) {
        memset (dptr, 0, k);
      }
    }
    break;
  case DV_AUDIO_CORRECT_AVERAGE:
    for (num_ch = 0; num_ch < dv_audio -> raw_num_channels; ++num_ch) {
      dptr = sptr = outbufs [num_ch];
      last_valid = 0;
      for (i = 0; i < dv_audio -> raw_samples_this_frame [num_ch / 2]; i++) {
        if (*sptr != (int16_t) 0x8000) {
          last_valid = *dptr++ = *sptr++;
          continue;
        }
        for (k = i, cnt = 0;
             (k < dv_audio -> raw_samples_this_frame [num_ch / 2]) &&
             (*sptr == (int16_t) 0x8000);
             k++, cnt++, sptr++) {
          ;
        }
        i += (cnt - 1);
        next_valid = (k == dv_audio -> raw_samples_this_frame [num_ch / 2]) ? 0 : *sptr;
        diff = (next_valid - last_valid) / (cnt + 1);
#if 0
        fprintf (stderr,
                 " last_valid = 0x%04x diff =0x%04x next_valid = 0x%04x cnt = %d\n",
                 last_valid, diff, next_valid, cnt);
#endif
        while (cnt-- > 0) {
          last_valid += diff;
          *dptr++ = last_valid;
        }
      }
    }
    break;
  case DV_AUDIO_CORRECT_NONE:
    break;
  default:
    break;
  }
}

/* ---------------------------------------------------------------------------
 */
void
dv_audio_mix4ch (dv_audio_t *dv_audio, int16_t **outbufs)
{
    int     current_samples,
            num_ch,
            ch0_div, ch1_div,
            i, k;
    int16_t *dptr, *sptr;

  if (!(dv_audio -> raw_num_channels == 4))
    return;

  /* -------------------------------------------------------------------------
   * take entire channel 0
   */
  if (dv_audio -> arg_mixing_level >= 16)
    return;

  /* -------------------------------------------------------------------------
   * take entire channel 1
   */
  if (dv_audio -> arg_mixing_level <= -16)
  {
    for (num_ch = 0; num_ch < 2; ++num_ch)
    {
      dptr = outbufs [num_ch];
      sptr = outbufs [num_ch + 2];
      for (i = k = 0; i < dv_audio -> raw_samples_this_frame [1]; ++i)
      {
        *dptr++ = *sptr++;
      }
    }
    dv_audio -> raw_samples_this_frame [0] =
      dv_audio -> samples_this_frame =
      dv_audio -> raw_samples_this_frame [1];
    return;
  }

  /* -------------------------------------------------------------------------
   * mix both channles according to mixing level
   */
  current_samples = (dv_audio -> raw_samples_this_frame [0] >
                      dv_audio -> raw_samples_this_frame [1]) ?
                        dv_audio -> raw_samples_this_frame [1]:
                          dv_audio -> raw_samples_this_frame [0];

  ch0_div = ch1_div = 2;

  if (dv_audio -> arg_mixing_level < 0)
  {
    ch0_div = 1 << (1 - dv_audio -> arg_mixing_level);
  }
  else if (dv_audio -> arg_mixing_level > 0)
  {
    ch1_div = 1 << (1 + dv_audio -> arg_mixing_level);
  }
  for (num_ch = 0; num_ch < 2; ++num_ch)
  {
    dptr = outbufs [num_ch];
    sptr = outbufs [num_ch + 2];
    for (i = k = 0; i < current_samples; ++i)
    {
      *dptr = (*dptr / ch0_div) + (*sptr++ / ch1_div);
       dptr++;
    }
  }
  dv_audio -> raw_samples_this_frame [0] = dv_audio -> samples_this_frame = current_samples;
}

/* ---------------------------------------------------------------------------
 */
int
dv_set_audio_correction (dv_decoder_t *dv, int method)
{
    int  old_method;

  old_method = dv -> audio -> correction_method;
  dv -> audio -> correction_method = method;
  return old_method;
} /* dv_set_audio_correction */

/* ---------------------------------------------------------------------------
 */
int
dv_set_mixing_level (dv_decoder_t *dv, int new_value)
{
    int old_value;

  old_value = dv -> audio -> arg_mixing_level;
  dv -> audio -> arg_mixing_level = new_value;
  return (old_value);
}

/* ---------------------------------------------------------------------------
 */
int
dv_get_num_samples (dv_decoder_t *dv)
{
  return dv -> audio -> samples_this_frame;
}

/* ---------------------------------------------------------------------------
 */
int
dv_get_num_channels (dv_decoder_t *dv)
{
  return dv -> audio -> num_channels;
}

/* ---------------------------------------------------------------------------
 */
int
dv_is_4ch (dv_decoder_t *dv)
{
  return dv -> audio -> raw_num_channels == 4;
}

/* ---------------------------------------------------------------------------
 */
int
dv_get_raw_samples (dv_decoder_t *dv, int chan)
{
  return dv -> audio -> raw_samples_this_frame [chan];
}

/* ---------------------------------------------------------------------------
 */
int
dv_get_frequency (dv_decoder_t *dv)
{
  return dv -> audio -> frequency;
}

/* ---------------------------------------------------------------------------
 */
int
dv_is_new_recording (dv_decoder_t *dv, const uint8_t *buffer)
{
    int temp_time_stamp [4],
        zero_time_stamp [4] = {0, 0, 0, 0},
        new_recording = 0;

  /* -------------------------------------------------------------------------
   * we need valid and parsed audio headers for this
   */
  if (dv_parse_audio_header (dv, buffer))
  {
    /* -----------------------------------------------------------------------
     * only for 32kHz 12bit we need some extra checks
     */
    if (dv -> audio -> frequency == 32000 && dv -> audio -> quantization == 12)
    {
      /* ---------------------------------------------------------------------
       * 1. new recording by rec start in first channel
       */
      if (!dv -> audio -> aaux_asc. pc2. rec_st)
        new_recording++;

      /* ---------------------------------------------------------------------
       * reset frame change logic for rec end (frame changed twice)
       */
      dv_get_timestamp_int (dv, temp_time_stamp);
      if (!dv -> audio -> new_recording_on_next_frame &&
          memcmp (dv -> audio -> new_recording_current_time_stamp,
                  temp_time_stamp,
                  4 * sizeof (int)))
      {
        memcpy (dv -> audio -> new_recording_current_time_stamp,
                zero_time_stamp,
                4 * sizeof (int));
      }

      /* ---------------------------------------------------------------------
       * frame change flag is set and frame changed. so trigger new_recording.
       */
      if (dv -> audio -> new_recording_on_next_frame &&
          memcmp (dv -> audio -> new_recording_current_time_stamp,
                  temp_time_stamp,
                  4 * sizeof (int)))
      {
        dv -> audio -> new_recording_on_next_frame = 0;
      }

      /* ---------------------------------------------------------------------
       * 2. new recording by rec end some time ago and frame changed
       */
      if (memcmp (dv -> audio -> new_recording_current_time_stamp,
                   zero_time_stamp,
                   4 * sizeof (int)) &&
          !dv -> audio -> new_recording_on_next_frame)
      {
        new_recording++;
      }

      /* ---------------------------------------------------------------------
       * now set markers for next frame
       */
      if (dv -> audio -> raw_num_channels == 4 &&
          !dv -> audio -> aaux_asc1. pc2. rec_end)
      {
        memcpy (dv -> audio -> new_recording_current_time_stamp,
                temp_time_stamp,
                4 * sizeof (int));
        dv -> audio -> new_recording_on_next_frame = 1;
      }
    }
    else if (!dv -> audio -> aaux_asc. pc2. rec_st)
    {
        return new_recording = 1;
    }

  }
  return new_recording;

}

#if 0
/* ---------------------------------------------------------------------------
 * dv_audio_do_fade returns:
 *      0 for not fading action is required
 *   <> 0 if fading action is required
 *
 *   *ch0, *ch1 are set to:
 *      0 if no fading action is required for this channel
 *      bit0 == 1 -> fade in should be done
 *      bit1 == 1 -> fade out should be done
 * function is TODO
 * And what about a single frame with rec start and end and fade start/end set too ??
 */
int
dv_audio_do_fade (dv_decoder_t *dv, int *ch0, int *ch1)
{
  *ch0 = *ch1 = 0;
  return *ch0 + *ch1;
}
#endif

int dv_is_normal_speed (dv_decoder_t *dv)
{
  int normal_speed = TRUE;
	
  if (dv->std == e_dv_std_iec_61834) {
    normal_speed = (dv->audio->aaux_asc.pc3.speed == 0x20);
  } else if (dv->std == e_dv_std_smpte_314m) {
    if(dv->audio->aaux_as.pc3.system) {
      /* PAL */
      normal_speed = (dv->audio->aaux_asc.pc3.speed == 0x64);
    } else {
      /* NTSC */
      normal_speed = (dv->audio->aaux_asc.pc3.speed == 0x78);
    } /* else */
  }
  return normal_speed;
}
