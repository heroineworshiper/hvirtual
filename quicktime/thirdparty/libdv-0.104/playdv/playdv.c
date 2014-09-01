/*
 *  playdv.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
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

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#include "libdv/dv.h"
#include "display.h"
#include "oss.h"

#ifndef        timersub
# define timersub(a, b, result)                          \
  do {                                                   \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;        \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;     \
    if ((result)->tv_usec < 0) {                         \
      --(result)->tv_sec;                                \
      (result)->tv_usec += 1000000;                      \
    }                                                    \
  } while (0)
#endif

#define DV_PLAYER_OPT_VERSION         0
#define DV_PLAYER_OPT_DISABLE_AUDIO   1
#define DV_PLAYER_OPT_DISABLE_VIDEO   2
#define DV_PLAYER_OPT_NUM_FRAMES      3
#define DV_PLAYER_OPT_OSS_INCLUDE     4
#define DV_PLAYER_OPT_DISPLAY_INCLUDE 5
#define DV_PLAYER_OPT_DECODER_INCLUDE 6
#define DV_PLAYER_OPT_AUTOHELP        7
#define DV_PLAYER_OPT_DUMP_FRAMES     8
#define DV_PLAYER_OPT_NO_MMAP         9
#define DV_PLAYER_OPT_LOOP_COUNT      10
#define DV_PLAYER_NUM_OPTS            11

/* Book-keeping for mmap */
typedef struct dv_mmap_region_s {
  void   *map_start;  /* Start of mapped region (page aligned) */
  size_t  map_length; /* Size of mapped region */
  guint8 *data_start; /* Data we asked for */
} dv_mmap_region_t;

typedef struct {
  dv_decoder_t    *decoder;
  dv_display_t    *display;
  dv_oss_t        *oss;
  dv_mmap_region_t mmap_region;
  struct stat      statbuf;
  struct timeval   tv[3];
  gint             arg_disable_audio;
  gint             arg_disable_video;
  gint             no_mmap;
  gint             arg_num_frames;
  char*            arg_dump_frames;
  gint             arg_loop_count;
#if HAVE_LIBPOPT
  struct poptOption option_table[DV_PLAYER_NUM_OPTS+1]; 
#endif /* HAVE_LIBPOPT */
} dv_player_t;

dv_player_t *
dv_player_new(void) 
{
  dv_player_t *result;
  
  if(!(result = (dv_player_t *)calloc(1,sizeof(dv_player_t)))) goto no_mem;
  if(!(result->display = dv_display_new())) goto no_display;
  if(!(result->oss = dv_oss_new())) goto no_oss;
  if(!(result->decoder = dv_decoder_new(TRUE, FALSE, FALSE))) goto no_decoder;
  result->arg_loop_count = 1;

#if HAVE_LIBPOPT
  result->option_table[DV_PLAYER_OPT_VERSION] = (struct poptOption) {
    longName: "version", 
    shortName: 'v', 
    val: 'v', 
    descrip: "show playdv version number",
  }; /* version */

  result->option_table[DV_PLAYER_OPT_DISABLE_AUDIO] = (struct poptOption) {
    longName: "disable-audio", 
    arg:      &result->arg_disable_audio,
    descrip:  "skip audio decoding",
  }; /* disable audio */

  result->option_table[DV_PLAYER_OPT_DISABLE_VIDEO] = (struct poptOption) {
    longName: "disable-video", 
    descrip:  "skip video decoding",
    arg:      &result->arg_disable_video, 
  }; /* disable video */

  result->option_table[DV_PLAYER_OPT_NUM_FRAMES] = (struct poptOption) {
    longName:   "num-frames", 
    shortName:  'n', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_num_frames,
    argDescrip: "count",
    descrip:    "stop after <count> frames",
  }; /* number of frames */

  result->option_table[DV_PLAYER_OPT_DUMP_FRAMES] = (struct poptOption) {
    longName:   "dump-frames", 
    argInfo:    POPT_ARG_STRING,
    arg:        &result->arg_dump_frames,
    argDescrip: "pattern",
    descrip:    "dump all frames to file pattern like capture%05d.ppm "
    "(or - for stdout)"
  }; /* dump all frames */

  result->option_table[DV_PLAYER_OPT_NO_MMAP] = (struct poptOption) {
    longName:   "no-mmap", 
    arg:        &result->no_mmap,
    descrip:    "don't use mmap for reading. (usefull for pipes)"
  }; /* no mmap */

  result->option_table[DV_PLAYER_OPT_LOOP_COUNT] = (struct poptOption) {
    longName:   "loop-count", 
    shortName:  'l', 
    argInfo:    POPT_ARG_INT, 
    arg:        &result->arg_loop_count,
    argDescrip: "count",
    descrip:    "loop playback <count> times, 0 for infinite",
  }; /* loop count */

  result->option_table[DV_PLAYER_OPT_OSS_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    arg:     result->oss->option_table,
    descrip: "Audio output options",
  }; /* oss */

  result->option_table[DV_PLAYER_OPT_DISPLAY_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    arg:     result->display->option_table,
    descrip: "Video output options",
  }; /* display */

  result->option_table[DV_PLAYER_OPT_DECODER_INCLUDE] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    arg:     result->decoder->option_table,
    descrip: "Decoder options",
  }; /* decoder */

  result->option_table[DV_PLAYER_OPT_AUTOHELP] = (struct poptOption) {
    argInfo: POPT_ARG_INCLUDE_TABLE,
    arg:     poptHelpOptions,
    descrip: "Help options",
  }; /* autohelp */

#endif /* HAVE_LIBPOPT */

  return(result);

 no_decoder:
  free(result->oss);
 no_oss:
  free(result->display);
 no_display:
  free(result);
  result = NULL;
 no_mem:
  return(result);
} /* dv_player_new */


/* I decided to try to use mmap for reading the input.  I got a slight
 * (about %5-10) performance improvement */
void mmap_unaligned(int fd, int no_mmap, off_t offset, size_t length, 
		    dv_mmap_region_t *mmap_region) 
{
	size_t real_length;
	off_t  real_offset;
	size_t page_size;
	size_t start_padding;
	void *real_start;

	static off_t last_offset = 0;
	static size_t last_length = 0;
	static size_t overrun_size = 0;
	static size_t overrun_offset = 0;
	
	if (no_mmap) {
		size_t to_read = length;
		
		if (!mmap_region->map_start) {
			mmap_region->map_start = malloc(144000);
		}

		if (last_offset == offset) {
			if (last_length < length) {
				to_read -= last_length;
			} else if (last_length > length) {
				overrun_size = last_length - length;
				overrun_offset = length;
				last_length = length;
				return;
			} else {
				return;
			}
		}
		last_offset = offset;
		last_length = length;
		if (overrun_size) {
			memmove(mmap_region->map_start, 
				mmap_region->map_start + overrun_offset,
				overrun_size);
			if (to_read > overrun_size) {
				to_read -= overrun_size;
				overrun_size = 0;
			} else {
				overrun_size -= to_read;
				overrun_offset += to_read;
				to_read = 0;
			}
		}
		while (to_read > 0) {
			int rval = read(fd, mmap_region->map_start 
					+ length - to_read, to_read);
			if (rval == 0) {
				if (to_read != length) {
					fprintf(stderr, "Short read!\n");
					exit(-1);
				}
				exit(0);
			}
			if (rval < 0) {
				perror("read");
				exit(-1); 
			}
			to_read -= rval;
		}
		mmap_region->map_length = length;
		mmap_region->data_start = mmap_region->map_start;
	} else {
		page_size = getpagesize();
		start_padding = offset % page_size;
		real_offset = (offset / page_size) * page_size;
		real_length = real_offset + start_padding + length;
		real_start = mmap(0, real_length, PROT_READ, MAP_SHARED, 
				  fd, real_offset);
		
		mmap_region->map_start = real_start;
		mmap_region->map_length = real_length;
		mmap_region->data_start = real_start + start_padding;
	}
} /* mmap_unaligned */

int munmap_unaligned(dv_mmap_region_t *mmap_region, int no_mmap) 
{
	if (no_mmap) {
		return 0;
	} else {
		return(munmap(mmap_region->map_start,mmap_region->map_length));
	}
} /* munmap_unaligned */

int 
main(int argc,char *argv[]) 
{
  dv_player_t *dv_player = NULL;
  const char *filename;     /* name of input file */
  int fd;
  off_t offset = 0, eof = 0;
  guint frame_count = 0;
  gint i;
  gdouble seconds = 0.0;
  gint16 *audio_buffers[4];
#if HAVE_LIBPOPT
  int rc;             /* return code from popt */
  poptContext optCon; /* context for parsing command-line options */
#endif /* HAVE_LIBPOPT */

  if(!(dv_player = dv_player_new())) goto no_mem;

#if HAVE_LIBPOPT
  /* Parse options using popt */
  optCon = poptGetContext(NULL, argc, (const char **)argv, dv_player->option_table, 0);
  poptSetOtherOptionHelp(optCon, "[raw dv file or -- for stdin]");

  while ((rc = poptGetNextOpt(optCon)) > 0) {
    switch (rc) {
    case 'v':
      goto display_version;
      break;
    default:
      break;
    } /* switch */
  } /* while */

  filename = poptGetArg(optCon);
  if (rc < -1) goto bad_arg;
  if((filename == NULL) || !(poptPeekArg(optCon) == NULL))
	  filename = "-";
  poptFreeContext(optCon);
#else
  /* No popt, no usage and no options!  HINT: get popt if you don't
   * have it yet, it's at: ftp://ftp.redhat.com/pub/redhat/code/popt 
   */
  filename = argv[1];
#endif /* HAVE_LIBOPT */

  if (strcmp(filename, "-") == 0) {
	  dv_player->no_mmap = 1;
	  fd = STDIN_FILENO;
  } else {
	  /* Open the input file, do fstat to get it's total size */
	  if(-1 == (fd = open(filename,O_RDONLY))) goto openfail;
  }
  if (!dv_player->no_mmap) {
	  if(fstat(fd, &dv_player->statbuf)) goto fstatfail;
	  eof = dv_player->statbuf.st_size;
  }

  dv_player->decoder->quality = dv_player->decoder->video->quality;

  /* Read in header of first frame to see how big frames are */
  mmap_unaligned(fd,dv_player->no_mmap,0,120000,&dv_player->mmap_region);
  if(MAP_FAILED == dv_player->mmap_region.map_start) goto map_failed;

  if(dv_parse_header(dv_player->decoder, dv_player->mmap_region.data_start)< 0)
    goto header_parse_error;

  if (dv_format_wide (dv_player->decoder))
    fprintf (stderr, "format 16:9\n");
  if (dv_format_normal (dv_player->decoder))
    fprintf (stderr, "format 4:3\n");

  fprintf(stderr, "Audio is %.1f kHz, %d bits quantization, "
	  "%d channels, emphasis %s\n",
	 (float)dv_player->decoder->audio->frequency / 1000.0,
	 dv_player->decoder->audio->quantization,
	 dv_player->decoder->audio->num_channels,
	 (dv_player->decoder->audio->emphasis ? "on" : "off"));

  munmap_unaligned(&dv_player->mmap_region,dv_player->no_mmap);

  eof -= dv_player->decoder->frame_size; /* makes loop condition simpler */

  if(!dv_player->arg_disable_video) {
    if(!dv_display_init (dv_player->display, &argc, &argv, 
			 dv_player->decoder->width, dv_player->decoder->height, 
			 dv_player->decoder->sampling, "playdv", "playdv")) goto no_display;
  } /* if */

  dv_player->arg_disable_audio = 
    dv_player->arg_disable_audio || (!dv_oss_init(dv_player->decoder, dv_player->oss));

  for(i=0; i < 4; i++) {
    if(!(audio_buffers[i] = malloc(DV_AUDIO_MAX_SAMPLES*sizeof(gint16)))) goto no_mem;
  } /* if */

  gettimeofday(dv_player->tv+0,NULL);

restart:

  dv_player->decoder->prev_frame_decoded = 0;
  for(offset=0;
      offset <= eof || dv_player->no_mmap; 
      offset += dv_player->decoder->frame_size) {

     /*
     * Map the frame's data into memory
     */
    mmap_unaligned (fd,dv_player->no_mmap, 
		    offset, dv_player->decoder->frame_size,
		    &dv_player->mmap_region);
    if (MAP_FAILED == dv_player->mmap_region.map_start) goto map_failed;
    if (dv_parse_header (dv_player->decoder,
        		 dv_player->mmap_region.data_start) > 0) {
      /*
       * video norm has changed so remap region for current frame
       */
      munmap_unaligned (&dv_player->mmap_region,dv_player->no_mmap);
      mmap_unaligned (fd, dv_player->no_mmap, offset,
		      dv_player->decoder->frame_size, &dv_player->mmap_region);
      if (MAP_FAILED == dv_player->mmap_region.map_start) goto map_failed;
      dv_display_set_norm (dv_player->display, dv_player->decoder->system);
    } /* if */

    /* -----------------------------------------------------------------------
     * now frame is complete, so we may parse all the rest of info packs
     */
    dv_parse_packs (dv_player -> decoder, dv_player -> mmap_region. data_start);

    /* -----------------------------------------------------------------------
     * keep track of any possible format changes of dv source material
     */
    dv_display_check_format (dv_player->display,
			     dv_format_wide (dv_player->decoder));

    /* Parse and unshuffle audio */
    if(!dv_player->arg_disable_audio) {
      if(dv_decode_full_audio(dv_player->decoder, dv_player->mmap_region.data_start, audio_buffers)) {
	dv_oss_play(dv_player->decoder, dv_player->oss, audio_buffers);
      } /* if */
    } /* if */

    if(!dv_player->arg_disable_video) {
#if 0
      if (dv_format_wide (dv_player->decoder))
        fprintf (stderr, "format 16:9\n");
      if (dv_format_normal (dv_player->decoder))
        fprintf (stderr, "format 4:3\n");
#endif

      if (!dv_player->decoder->prev_frame_decoded ||
          dv_frame_changed (dv_player->decoder)) {

        dv_report_video_error (dv_player -> decoder,
                               dv_player -> mmap_region. data_start);
                               
	/* Parse and decode video */
	dv_decode_full_frame(dv_player->decoder, dv_player->mmap_region.data_start, 
			     dv_player->display->color_space, 
			     dv_player->display->pixels, 
			     dv_player->display->pitches);
	dv_player->decoder->prev_frame_decoded = 1;
      } else {
	fprintf (stderr, "same_frame\n");
      }

      /* ----------------------------------------------------------------------
       * save all frames. even it was not nessessary to decode
       */
      if(dv_player->arg_dump_frames) {
          FILE* fp;
          char fname[4096];

        if (strcmp(dv_player->arg_dump_frames, "-") == 0) {
          fp = stdout;
        } else {
          snprintf(fname, 4096, dv_player->arg_dump_frames,
                   frame_count);
          fp = fopen(fname, "w");
        }
        fprintf(fp, "P6\n# CREATOR: playdv\n%d %d\n255\n",
                dv_player->display->width, dv_player->display->height);
        fwrite(dv_player->display->pixels[0],
               3, dv_player->display->width
                * dv_player->display->height, fp);
        if (fp != stdout) {
          fclose(fp);
        }
      }
      /* Display */
      dv_display_show(dv_player->display);
    } /* if  */

    frame_count++;
    if((dv_player->arg_num_frames > 0) && (frame_count >= dv_player->arg_num_frames)) {
      goto end_of_file;
    } /* if  */
#if 0
{int dummy;read(0,&dummy,1);}
#endif

    /* Release the frame's data */
    munmap_unaligned(&dv_player->mmap_region, dv_player->no_mmap); 

  } /* while */

 end_of_file:
  
  /* Handle looping */
  if (--dv_player->arg_loop_count != 0) {
    lseek( fd, 0, SEEK_SET);
    goto restart;
  }
  
  gettimeofday(dv_player->tv+1,NULL);
  timersub(dv_player->tv+1,dv_player->tv+0,dv_player->tv+2);
  seconds = (double)dv_player->tv[2].tv_usec / 1000000.0; 
  seconds += dv_player->tv[2].tv_sec;
  fprintf(stderr,"Processed %d frames in %05.2f seconds (%05.2f fps)\n", 
	  frame_count, seconds, (double)frame_count/seconds);
  if(!dv_player->arg_disable_video) {
    dv_display_exit(dv_player->display);
  } /* if */
  dv_decoder_free(dv_player->decoder);
  free(dv_player);
  exit(0);

  /* Error handling section */
#if HAVE_LIBPOPT
 display_version:
  fprintf(stderr,"playdv: version %s, http://libdv.sourceforge.net/\n",
	  VERSION);
  exit(0);

 bad_arg:
  /* an error occurred during option processing */
  fprintf(stderr, "%s: %s\n",
	  poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
	  poptStrerror(rc));
  exit(-1);
#endif
 no_display:
  exit(-1);
 openfail:
  perror("open:");
  exit(-1);
 fstatfail:
  perror("fstat:");
  exit(-1);
 map_failed:
  perror("mmap:");
  exit(-1);
 header_parse_error:
  fprintf(stderr,"Parser error reading first header\n");
  exit(-1);
 no_mem:
  fprintf(stderr,"Out of memory\n");
  exit(-1);
} /* main */
