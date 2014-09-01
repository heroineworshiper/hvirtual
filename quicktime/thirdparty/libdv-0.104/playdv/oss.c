/*
 *  oss.c
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>

#include "oss.h"

static const char *default_device = "/dev/dsp";

#ifndef        O_LARGEFILE
#define        O_LARGEFILE 0
#endif

dv_oss_t *
dv_oss_new(void)
{
  dv_oss_t *result;
  
  result = (dv_oss_t *)calloc(1,sizeof(dv_oss_t));
  if(!result) goto no_mem;

#if HAVE_LIBPOPT
  result->option_table[DV_OSS_OPT_DEVICE] = (struct poptOption) {
    longName:   "audio-device", 
    argInfo:    POPT_ARG_STRING, 
    arg:        &result->arg_audio_device,
    descrip:    "target audio device; e.g. /dev/dsp [default]",
    argDescrip: "devicename",
  }; /* device */

  result->option_table[DV_OSS_OPT_FILE] = (struct poptOption) {
    longName:   "audio-file", 
    argInfo:    POPT_ARG_STRING, 
    arg:        &result->arg_audio_file,
    descrip:    "send raw decoded audio to file, skipping audio ioctls",
    argDescrip: "filename",
  }; /* file */
#endif // HAVE_LIBPOPT

 no_mem:
  return(result);
} /* dv_oss_new */

/* Very simplistic for sound output using the OSS API */
int
dv_oss_init(dv_decoder_t *dv, dv_oss_t *oss)
{
  int   format = AFMT_S16_NE, rate_request, channels_request;
  char  *device;

  channels_request = dv_get_num_channels (dv);
  rate_request = dv_get_frequency (dv);
  oss->buffer=NULL;
  oss->fd=-1;
  if(oss->arg_audio_device && oss->arg_audio_file) goto usage;
  if(oss->arg_audio_file) {
    if ((oss->fd = open(oss->arg_audio_file,
			O_WRONLY|O_CREAT|O_TRUNC|O_LARGEFILE,
			S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) == -1) goto no_file;
  } else {
    device = (char *)(oss->arg_audio_device ? oss->arg_audio_device : default_device);
    /* open audio device */
    if ((oss->fd = open(device, O_RDWR, 0)) == -1) goto no_device;
    /* set sample format -- try for 16bit */
    if (ioctl(oss->fd, SNDCTL_DSP_SETFMT, &format) == -1) goto format_ioctl;
    if (format != AFMT_S16_NE) goto format_unsupported;
    /* set stereo mode */
    if (ioctl(oss->fd, SNDCTL_DSP_CHANNELS, &channels_request) == -1) goto channels_ioctl;
    if (channels_request != dv_get_num_channels (dv)) goto channels_unsupported;
    /* set sampling rate */
    if (ioctl(oss->fd, SNDCTL_DSP_SPEED, &rate_request) == -1) goto rate_ioctl;
    if(rate_request != dv_get_frequency (dv)) goto rate_unsupported;
  }
  if(!(oss->buffer = malloc(DV_AUDIO_MAX_SAMPLES * channels_request * sizeof(short)))) goto no_memory;
  return(TRUE);

 usage:
  fprintf(stderr,"can't send audio to both device and file\n");
  exit(-1);

 format_unsupported:
  fprintf(stderr, "soundcard doesn't support format\n");
  goto fail;

 rate_unsupported:
  fprintf(stderr,"audio rate found : %d Hz\n", rate_request);
  fprintf(stderr, "soundcard doesn't support 48kHz (does %d)\n", rate_request);
  goto fail;

 channels_unsupported:
  fprintf(stderr, "soundcard doesn't support %d channels\n", channels_request);
  goto fail;

 no_device:
  perror(device);
  goto fail;

 no_file:
  perror(oss->arg_audio_file);
  goto fail;

 format_ioctl:
  perror("SNDCTL_DSP_SETFMT");
  goto fail;

 channels_ioctl:
  perror("SNDCTL_DSP_CHANNELS");
  goto fail;

 rate_ioctl:
  perror("SNDCTL_DSP_SPEED");
  goto fail;

 no_memory:
  fprintf(stderr, "out of memory\n");
  goto fail;

 fail:
  dv_oss_close(oss);
  return(FALSE);
} /* dv_oss_init */

int
dv_oss_play(dv_decoder_t *dv, dv_oss_t *oss, short **out)
{
  int ch, i, j=0, total, written=0, result, samples, channels;

  /* Interleave the audio into a single buffer */
  for (i = 0, samples = dv_get_num_samples (dv), channels = dv_get_num_channels (dv);
       i < samples; i++) {
    for (ch = 0; ch < channels; ch++) {
      oss -> buffer [j++] = out [ch] [i];
    } /* for */
  } /* for */

  /* Send the audio to the device */
  total = samples * channels * sizeof (short);
  do {
    result = write(oss->fd, oss->buffer + written, total - written);
    if(result <= 0) goto write_error;
    written += result;
  } while(total > written);
  if(!oss->arg_audio_file) {
    if (ioctl(oss->fd, SNDCTL_DSP_POST, NULL) == -1) goto post_ioctl;
  } /* if */

  return(TRUE);

 write_error:
  perror("write");
  return(FALSE);

 post_ioctl:
  perror("SNDCTL_DSP_POST");
  return(FALSE);

} /* dv_oss_play */

void
dv_oss_close(dv_oss_t *oss) 
{
  if(oss->fd != -1) {
    close(oss->fd);
    oss->fd = -1;
  } /* if */
  if(oss->buffer) {
    oss->buffer = NULL;
    free(oss->buffer);
  } /* if */
} /* dv_oss_close */
