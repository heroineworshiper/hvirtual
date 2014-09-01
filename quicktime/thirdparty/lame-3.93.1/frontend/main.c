/*
 *      Command line frontend program
 *
 *      Copyright (c) 1999 Mark Taylor
 *                    2000 Takehiro TOMIANGA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: main.c,v 1.1.1.1 2003/10/14 07:54:37 heroine Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char   *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if defined(_WIN32)
# include <windows.h>
#endif


/*
 main.c is example code for how to use libmp3lame.a.  To use this library,
 you only need the library and lame.h.  All other .h files are private
 to the library.
*/
#include "lame.h"

#include "brhist.h"
#include "parse.h"
#include "main.h"
#include "get_audio.h"
#include "portableio.h"
#include "timestatus.h"
#include "VbrTag.h"

/* PLL 14/04/2000 */
#if macintosh
#include <console.h>
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif




/************************************************************************
*
* main
*
* PURPOSE:  MPEG-1,2 Layer III encoder with GPSYCHO
* psychoacoustic model.
*
************************************************************************/

int
parse_args_from_string(lame_global_flags * const gfp, const char *p,
                       char *inPath, char *outPath)
{                       /* Quick & very Dirty */
    char   *q;
    char   *f;
    char   *r[128];
    int     c = 0;
    int     ret;

    if (p == NULL || *p == '\0')
        return 0;

    f = q = malloc(strlen(p) + 1);
    strcpy(q, p);

    r[c++] = "lhama";
    while (1) {
        r[c++] = q;
        while (*q != ' ' && *q != '\0')
            q++;
        if (*q == '\0')
            break;
        *q++ = '\0';
    }
    r[c] = NULL;

    ret = parse_args(gfp, c, r, inPath, outPath, NULL, NULL);
    free(f);
    return ret;
}





FILE   *
init_files(lame_global_flags * gf, char *inPath, char *outPath)
{
    FILE   *outf;
    /* Mostly it is not useful to use the same input and output name.
       This test is very easy and buggy and don't recognize different names
       assigning the same file
     */
    if (0 != strcmp("-", outPath) && 0 == strcmp(inPath, outPath)) {
        fprintf(stderr, "Input file and Output file are the same. Abort.\n");
        return NULL;
    }

    /* open the wav/aiff/raw pcm or mp3 input file.  This call will
     * open the file, try to parse the headers and
     * set gf.samplerate, gf.num_channels, gf.num_samples.
     * if you want to do your own file input, skip this call and set
     * samplerate, num_channels and num_samples yourself.
     */
    init_infile(gf, inPath);
    if ((outf = init_outfile(outPath, lame_get_decode_only(gf))) == NULL) {
        fprintf(stderr, "Can't init outfile '%s'\n", outPath);
        return NULL;
    }

    return outf;
}






/* the simple lame decoder */
/* After calling lame_init(), lame_init_params() and
 * init_infile(), call this routine to read the input MP3 file
 * and output .wav data to the specified file pointer*/
/* lame_decoder will ignore the first 528 samples, since these samples
 * represent the mpglib delay (and are all 0).  skip = number of additional
 * samples to skip, to (for example) compensate for the encoder delay */

int
lame_decoder(lame_global_flags * gfp, FILE * outf, int skip, char *inPath,
             char *outPath)
{
    short int Buffer[2][1152];
    int     iread;
    double  wavsize;
    int     i;
    void    (*WriteFunction) (FILE * fp, char *p, int n);
    int tmp_num_channels = lame_get_num_channels( gfp );



    if (silent < 10) fprintf(stderr, "\rinput:  %s%s(%g kHz, %i channel%s, ",
            strcmp(inPath, "-") ? inPath : "<stdin>",
            strlen(inPath) > 26 ? "\n\t" : "  ",
            lame_get_in_samplerate( gfp ) / 1.e3,
            tmp_num_channels, tmp_num_channels != 1 ? "s" : "");

    switch (input_format) {
    case sf_mp3:
        if (skip==0) {
            if (enc_delay>-1) skip = enc_delay + 528+1;
            else skip=lame_get_encoder_delay(gfp)+528+1;
        }else{
            // user specified a value of skip. just add for decoder
            skip += 528+1; /* mp3 decoder has a 528 sample delay, plus user supplied "skip" */
        }

        if (silent < 10) fprintf(stderr, "MPEG-%u%s Layer %s", 2 - lame_get_version(gfp),
                lame_get_out_samplerate( gfp ) < 16000 ? ".5" : "", "III");
        break;
    case sf_mp2:
        skip += 240 + 1;
        if (silent < 10) fprintf(stderr, "MPEG-%u%s Layer %s", 2 - lame_get_version(gfp),
                lame_get_out_samplerate( gfp ) < 16000 ? ".5" : "", "II");
        break;
    case sf_mp1:
        skip += 240 + 1;
        if (silent < 10) fprintf(stderr, "MPEG-%u%s Layer %s", 2 - lame_get_version(gfp),
                lame_get_out_samplerate( gfp ) < 16000 ? ".5" : "", "I");
        break;
    case sf_ogg:
        if (silent < 10) fprintf(stderr, "Ogg Vorbis");
        skip = 0;       /* other formats have no delay *//* is += 0 not better ??? */
        break;
    case sf_raw:
        if (silent < 10) fprintf(stderr, "raw PCM data");
        mp3input_data.nsamp = lame_get_num_samples( gfp );
        mp3input_data.framesize = 1152;
        skip = 0;       /* other formats have no delay *//* is += 0 not better ??? */
        break;
    case sf_wave:
        if (silent < 10) fprintf(stderr, "Microsoft WAVE");
        mp3input_data.nsamp = lame_get_num_samples( gfp );
        mp3input_data.framesize = 1152;
        skip = 0;       /* other formats have no delay *//* is += 0 not better ??? */
        break;
    case sf_aiff:
        if (silent < 10) fprintf(stderr, "SGI/Apple AIFF");
        mp3input_data.nsamp = lame_get_num_samples( gfp );
        mp3input_data.framesize = 1152;
        skip = 0;       /* other formats have no delay *//* is += 0 not better ??? */
        break;
    default:
        if (silent < 10) fprintf(stderr, "unknown");
        mp3input_data.nsamp = lame_get_num_samples( gfp );
        mp3input_data.framesize = 1152;
        skip = 0;       /* other formats have no delay *//* is += 0 not better ??? */
        assert(0);
        break;
    }

    if (silent < 10) fprintf(stderr, ")\noutput: %s%s(16 bit, Microsoft WAVE)\n",
            strcmp(outPath, "-") ? outPath : "<stdout>",
            strlen(outPath) > 45 ? "\n\t" : "  ");

    if (skip > 0)
        if (silent < 10) fprintf(stderr, "skipping initial %i samples (encoder+decoder delay)\n",
                skip);

    if ( 0 == disable_wav_header )
        WriteWaveHeader(outf, 0x7FFFFFFF, lame_get_in_samplerate( gfp ),
                        tmp_num_channels,
                        16);
    /* unknown size, so write maximum 32 bit signed value */

    wavsize = -skip;
    WriteFunction = swapbytes ? WriteBytesSwapped : WriteBytes;
    mp3input_data.totalframes = mp3input_data.nsamp / mp3input_data.framesize;

    assert(tmp_num_channels >= 1 && tmp_num_channels <= 2);

    do {
        iread = get_audio16(gfp, Buffer); /* read in 'iread' samples */
        mp3input_data.framenum += iread / mp3input_data.framesize;
        wavsize += iread;

        if (silent <= 0)
            decoder_progress(gfp, &mp3input_data);

        skip -= (i = skip < iread ? skip : iread); /* 'i' samples are to skip in this frame */

        for (; i < iread; i++) {
            if ( disable_wav_header ) {
                WriteFunction(outf, (char *) &Buffer[0][i], sizeof(short));
                if (tmp_num_channels == 2)
                    WriteFunction(outf, (char *) &Buffer[1][i], sizeof(short));
            }
            else {
                Write16BitsLowHigh(outf, Buffer[0][i]);
                if (tmp_num_channels == 2)
                    Write16BitsLowHigh(outf, Buffer[1][i]);
            }
        }
    } while (iread);

    i = (16 / 8) * tmp_num_channels;
    assert(i > 0);
    if (wavsize <= 0) {
        if (silent < 10) fprintf(stderr, "WAVE file contains 0 PCM samples\n");
        wavsize = 0;
    }
    else if (wavsize > 0xFFFFFFD0 / i) {
        if (silent < 10) fprintf(stderr,
                "Very huge WAVE file, can't set filesize accordingly\n");
        wavsize = 0xFFFFFFD0;
    }
    else {
        wavsize *= i;
    }

    if ( 0 == disable_wav_header )
        if (!fseek(outf, 0l, SEEK_SET)) /* if outf is seekable, rewind and adjust length */
            WriteWaveHeader(outf, wavsize, lame_get_in_samplerate( gfp ),
                            tmp_num_channels, 16);
    fclose(outf);

    decoder_progress_finish(gfp);
    return 0;
}
















int
lame_encoder(lame_global_flags * gf, FILE * outf, int nogap, char *inPath,
             char *outPath)
{
    unsigned char mp3buffer[LAME_MAXMP3BUFFER];
    int     Buffer[2][1152];
    int     iread, imp3;
    static const char *mode_names[2][4] = {
        {"stereo", "j-stereo", "dual-ch", "single-ch"},
        {"stereo", "force-ms", "dual-ch", "single-ch"}
    };
    int     frames;

    if (silent < 10) {
        lame_print_config(gf); /* print useful information about options being used */

        fprintf(stderr, "Encoding %s%s to %s\n",
                strcmp(inPath, "-") ? inPath : "<stdin>",
                strlen(inPath) + strlen(outPath) < 66 ? "" : "\n     ",
                strcmp(outPath, "-") ? outPath : "<stdout>");

        fprintf(stderr,
                "Encoding as %g kHz ", 1.e-3 * lame_get_out_samplerate(gf));

        if (lame_get_ogg(gf)) {
            fprintf(stderr, "VBR Ogg Vorbis\n");
        }
        else {
            const char *appendix = "";

            switch (lame_get_VBR(gf)) {
            case vbr_mt:
            case vbr_rh:
            case vbr_mtrh:
                appendix = "ca. ";
                fprintf(stderr, "VBR(q=%i)", lame_get_VBR_q(gf));
                break;
            case vbr_abr:
                fprintf(stderr, "average %d kbps",
                        lame_get_VBR_mean_bitrate_kbps(gf));
                break;
            default:
                fprintf(stderr, "%3d kbps", lame_get_brate(gf));
                break;
            }
            fprintf(stderr, " %s MPEG-%u%s Layer III (%s%gx) qval=%i\n",
                    mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)],
                    2 - lame_get_version(gf),
                    lame_get_out_samplerate(gf) < 16000 ? ".5" : "",
                    appendix,
                    0.1 * (int) (10. * lame_get_compression_ratio(gf) + 0.5),
                    lame_get_quality(gf));
        }

        if (silent <= -10)
            lame_print_internals(gf);

        fflush(stderr);
    }


    /* encode until we hit eof */
    do {
        /* read in 'iread' samples */
        iread = get_audio(gf, Buffer);
        frames = lame_get_frameNum(gf);


 /********************** status display  *****************************/
        if (silent <= 0) {
            if (update_interval > 0) {
                timestatus_klemm(gf);
            }
            else {
                if (0 == frames % 50) {
#ifdef BRHIST
                    brhist_jump_back();
#endif
                    timestatus(lame_get_out_samplerate(gf),
                               frames,
                               lame_get_totalframes(gf),
                               lame_get_framesize(gf));
#ifdef BRHIST
                    if (brhist)
                        brhist_disp(gf);
#endif
                }
            }
        }

        /* encode */
        imp3 = lame_encode_buffer_int(gf, Buffer[0], Buffer[1], iread,
                                      mp3buffer, sizeof(mp3buffer));

        /* was our output buffer big enough? */
        if (imp3 < 0) {
            if (imp3 == -1)
                fprintf(stderr, "mp3 buffer is not big enough... \n");
            else
                fprintf(stderr, "mp3 internal error:  error code=%i\n", imp3);
            return 1;
        }

        if (fwrite(mp3buffer, 1, imp3, outf) != imp3) {
            fprintf(stderr, "Error writing mp3 output \n");
            return 1;
        }

    } while (iread);

    if (nogap) {
        imp3 = lame_encode_flush_nogap(gf, mp3buffer, sizeof(mp3buffer)); /* may return one more mp3 frame */
        /* reinitialize bitstream for next encoding.  this is normally done
         * by lame_init_params(), but we cannot call that routine twice */
        lame_init_bitstream(gf);
    } else {
        imp3 = lame_encode_flush(gf, mp3buffer, sizeof(mp3buffer)); /* may return one more mp3 frame */
    }

    if (imp3 < 0) {
        if (imp3 == -1)
            fprintf(stderr, "mp3 buffer is not big enough... \n");
        else
            fprintf(stderr, "mp3 internal error:  error code=%i\n", imp3);
        return 1;

    }

    if (silent <= 0) {
#ifdef BRHIST
        brhist_jump_back();
#endif
        timestatus(lame_get_out_samplerate(gf),
                   frames, lame_get_totalframes(gf), lame_get_framesize(gf));
#ifdef BRHIST
        if (brhist) {
            brhist_disp(gf);
        }
        brhist_disp_total(gf);
#endif
        timestatus_finish();
    }

    fwrite(mp3buffer, 1, imp3, outf);

    return 0;
}






void
brhist_init_package(lame_global_flags * gf)
{
#ifdef BRHIST
    if (brhist) {
        if (brhist_init
            (gf, lame_get_VBR_min_bitrate_kbps(gf),
             lame_get_VBR_max_bitrate_kbps(gf))) {
            /* fail to initialize */
            brhist = 0;
        }
    }
    else {
        brhist_init(gf, 128, 128); // Dirty hack
    }
#endif
}




void parse_nogap_filenames(int nogapout, char *inPath, char *outPath, char *outdir) {

    char    *slasher;
    int     n;

    strcpy(outPath,outdir);
    if (!nogapout) 	{
        strncpy(outPath, inPath, PATH_MAX + 1 - 4);
        n=strlen(outPath);
        /* nuke old extension, if one  */
        if (outPath[n-3] == 'w' 
            && outPath[n-2] == 'a'
            && outPath[n-1] == 'v'
            && outPath[n-4] == '.') {
            outPath[n-3] = 'm';
            outPath[n-2] = 'p';
            outPath[n-1] = '3';
        } else {
            outPath[n+0] = '.';
            outPath[n+1] = 'm';
            outPath[n+2] = 'p';
            outPath[n+3] = '3';
            outPath[n+4] = 0;
        }
    } else 	{
        slasher = inPath;
        slasher += PATH_MAX + 1 - 4;
        
        /* backseek to last dir delemiter */
        while (*slasher != '/' && *slasher != '\\' && slasher != inPath
               && *slasher != ':')
            {
                slasher--;
            }
        
        /* skip one foward if needed */
        if (slasher != inPath 
            && (outPath[strlen(outPath)-1] == '/'
                ||
                outPath[strlen(outPath)-1] == '\\'
                ||
                outPath[strlen(outPath)-1] == ':')) 
	    slasher++;
        else if (slasher == inPath
                 && (outPath[strlen(outPath)-1] != '/'
                     &&
                     outPath[strlen(outPath)-1] != '\\'
                     && 
                     outPath[strlen(outPath)-1] != ':'))
#ifdef _WIN32
	    strcat(outPath, "\\");
#elif __OS2__
        strcat(outPath, "\\");
#else
        strcat(outPath, "/");
#endif
        
        strncat(outPath, slasher, PATH_MAX + 1 - 4);
        n=strlen(outPath);
        /* nuke old extension  */
        if (outPath[n-3] == 'w' 
            && outPath[n-2] == 'a'
            && outPath[n-1] == 'v'
            && outPath[n-4] == '.') 	  {
	    outPath[n-3] = 'm';
	    outPath[n-2] = 'p';
	    outPath[n-1] = '3';
        } else {
	    outPath[n+0] = '.';
	    outPath[n+1] = 'm';
	    outPath[n+2] = 'p';
	    outPath[n+3] = '3';
	    outPath[n+4] = 0;
        }
    }
}







int
main(int argc, char **argv)
{
    int     ret;
    lame_global_flags *gf;
    char    outPath[PATH_MAX + 1];
    char    nogapdir[PATH_MAX + 1];
    char    inPath[PATH_MAX + 1];

    /* support for "nogap" encoding of up to 200 .wav files */
#define MAX_NOGAP 200
    int    nogapout = 0;
    int     max_nogap = MAX_NOGAP;
    char   *nogap_inPath[MAX_NOGAP];

    int     i;
    FILE   *outf;

#if macintosh
    argc = ccommand(&argv);
#endif

#if defined(_WIN32)
   /* set affinity back to all CPUs.  Fix for EAC/lame on SMP systems from
     "Todd Richmond" <todd.richmond@openwave.com> */
    typedef BOOL (WINAPI *SPAMFunc)(HANDLE, DWORD);
    SPAMFunc func;
    SYSTEM_INFO si;

    if ((func = (SPAMFunc)GetProcAddress(GetModuleHandle("KERNEL32.DLL"),
        "SetProcessAffinityMask")) != NULL) {
        GetSystemInfo(&si);
        func(GetCurrentProcess(), si.dwActiveProcessorMask);
    }
#endif


#ifdef __EMX__
    /* This gives wildcard expansion on Non-POSIX shells with OS/2 */
    _wildcard(&argc, &argv);
#endif

    for (i = 0; i < max_nogap; ++i) {
        nogap_inPath[i] = malloc(PATH_MAX + 1);
    }

    memset(inPath, 0, sizeof(inPath));
    
    /* initialize libmp3lame */
    input_format = sf_unknown;
    if (NULL == (gf = lame_init())) {
        fprintf(stderr, "fatal error during initialization\n");
        return 1;
    }
    if (argc <= 1) {
        usage(gf, stderr, argv[0]); /* no command-line args, print usage, exit  */
        return 1;
    }

    /* parse the command line arguments, setting various flags in the
     * struct 'gf'.  If you want to parse your own arguments,
     * or call libmp3lame from a program which uses a GUI to set arguments,
     * skip this call and set the values of interest in the gf struct.
     * (see the file API and lame.h for documentation about these parameters)
     */
    parse_args_from_string(gf, getenv("LAMEOPT"), inPath, outPath);
    ret = parse_args(gf, argc, argv, inPath, outPath, nogap_inPath, &max_nogap);
    if (ret < 0)
        return ret == -2 ? 0 : 1;

    if (update_interval < 0.)
        update_interval = 2.;

    if (outPath[0] != '\0' && max_nogap>0) {
        strncpy(nogapdir, outPath, PATH_MAX + 1);  
        nogapout = 1;
    }
    
    /* initialize input file.  This also sets samplerate and as much
       other data on the input file as available in the headers */
    if (max_nogap > 0) {
        /* for nogap encoding of multiple input files, it is not possible to
         * specify the output file name, only an optional output directory. */
        parse_nogap_filenames(nogapout,nogap_inPath[0],outPath,nogapdir);
        outf = init_files(gf, nogap_inPath[0], outPath);
        if (lame_get_bWriteVbrTag(gf)) {
            fprintf(stderr,"Note: Disabling VBR Xing/Info tag since it interferes with --nogap\n");
            lame_set_bWriteVbrTag( gf, 0 );
        }
    }
    else {
        outf = init_files(gf, inPath, outPath);
    }
    if (outf == NULL) {
        return -1;
    }

    /* Now that all the options are set, lame needs to analyze them and
     * set some more internal options and check for problems
     */
    i = lame_init_params(gf);
    if (i < 0) {
        if (i == -1) {
            display_bitrates(stderr);
        }
        fprintf(stderr, "fatal error during initialization\n");
        return i;
    }

    if (silent > 0 || lame_get_VBR(gf) == vbr_off) {
        brhist = 0;     /* turn off VBR histogram */
    }


#ifdef HAVE_VORBIS_ENCODER
    if (lame_get_ogg(gf)) {
        lame_encode_ogg_init(gf);
        lame_set_VBR(gf, vbr_off); /* ignore lame's various VBR modes */
    }
#endif


    if (lame_get_decode_only(gf)) {
        /* decode an mp3 file to a .wav */
        if (mp3_delay_set)
            lame_decoder(gf, outf, mp3_delay, inPath, outPath);
        else
            lame_decoder(gf, outf, 0, inPath, outPath);

    }
    else {
        if (max_nogap > 0) {
            /*
             * encode multiple input files using nogap option
             */
            for (i = 0; i < max_nogap; ++i) {
                int     use_flush_nogap = (i != (max_nogap - 1));
                if (i > 0) {
                    parse_nogap_filenames(nogapout,nogap_inPath[i],outPath,nogapdir);
                    /* note: if init_files changes anything, like
                       samplerate, num_channels, etc, we are screwed */
                    outf = init_files(gf, nogap_inPath[i], outPath);
                }
                brhist_init_package(gf);
                ret =
                    lame_encoder(gf, outf, use_flush_nogap, nogap_inPath[i],
                                 outPath);	
                
                if (silent<=0) ReportLameTagProgress(gf,1);
                lame_mp3_tags_fid(gf, outf); /* add VBR tags to mp3 file */
                if (silent<=0) ReportLameTagProgress(gf,0);
                
                fclose(outf); /* close the output file */
                close_infile(); /* close the input file */
            }
            lame_close(gf);

        }
        else {
            /*
             * encode a single input file
             */
            brhist_init_package(gf);
            ret = lame_encoder(gf, outf, 0, inPath, outPath);
            
            if (silent<=0) ReportLameTagProgress(gf,1);
            lame_mp3_tags_fid(gf, outf); /* add VBR tags to mp3 file */
            if (silent<=0) ReportLameTagProgress(gf,0);
            
            fclose(outf); /* close the output file */
            close_infile(); /* close the input file */
            lame_close(gf);
        }
    }
    return ret;
}
