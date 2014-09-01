/*************************************************************************/
/*                                                                       */
/*                Centre for Speech Technology Research                  */
/*                     University of Edinburgh, UK                       */
/*                      Copyright (c) 1997,1998                          */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  THE UNIVERSITY OF EDINBURGH AND THE CONTRIBUTORS TO THIS WORK        */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL THE UNIVERSITY OF EDINBURGH NOR THE CONTRIBUTORS BE LIABLE     */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*                Author :  Alan W Black                                 */
/*                Date   :  July 1997                                    */
/*-----------------------------------------------------------------------*/
/*  Optional support for /dev/dsp under FreeBSD and Linux                */
/*  These use the same underlying sound drivers (voxware).  This uses    */
/*  16bit linear if the device supports it otherwise it uses 8bit.  The  */
/*  8bit driver is still better than falling back to the "sunaudio" ulaw */
/*  8K as this driver can cope with various sample rates (and saves on   */
/*  resampling).                                                         */
/*                                                                       */
/*  Combined FreeBSD and Voxware code Feb 98                             */
/*                                                                       */
/*  This may work on NetBSD and OpenBSD but I haven't tried it           */
/*                                                                       */
/*=======================================================================*/

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <sys/stat.h>
#include "EST_cutils.h"
#include "EST_walloc.h"
#include "EST_Wave.h"
#include "EST_wave_aux.h"
#include "EST_Option.h"
#include "audioP.h"
#include "EST_io_aux.h"
#include "EST_error.h"

#ifdef SUPPORT_FREEBSD16
#include <sys/soundcard.h>
#include <fcntl.h>
int freebsd16_supported = TRUE;
int linux16_supported = FALSE;
static char *aud_sys_name = "FreeBSD";
#endif

#ifdef SUPPORT_LINUX16
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int linux16_supported = TRUE;
int freebsd16_supported = FALSE;
static char *aud_sys_name = "Linux";
#endif
static int stereo_only = 0;

#if defined(SUPPORT_LINUX16) || defined(SUPPORT_FREEBSD16)

// Code to block signals while sound is playing.
// Needed inside Java on (at least some) linux systems
// as scheduling interrupts seem to break the writes. 

#ifdef THREAD_SAFETY
#include <csignal>
#include <pthread.h>

#define THREAD_DECS() \
    sigset_t  oldmask \

#define THREAD_PROTECT() do { \
    sigset_t  newmask; \
    \
    sigfillset(&newmask); \
    \
    pthread_sigmask(SIG_BLOCK, &newmask, &oldmask); \
    } while(0)

#define THREAD_UNPROTECT() do { \
     pthread_sigmask(SIG_SETMASK, &oldmask, NULL); \
     } while (0)

#else
#define  THREAD_DECS() //empty
#define  THREAD_PROTECT() //empty
#define  THREAD_UNPROTECT() //empty
#endif

static int sb_set_sample_rate(int sbdevice, int samp_rate)
{
    int fmt;
    int sfmts;
    int stereo=0;
    int sstereo;
    int channels=1;

    ioctl(sbdevice,SNDCTL_DSP_RESET,0);
    ioctl(sbdevice,SNDCTL_DSP_SPEED,&samp_rate);
    sstereo = stereo;
    ioctl(sbdevice,SNDCTL_DSP_STEREO,&sstereo);
    /* Some devices don't do mono even when you ask them nicely */
    if (sstereo != stereo)
        stereo_only = 1;
    ioctl(sbdevice,SNDCTL_DSP_CHANNELS,&channels);
    ioctl(sbdevice,SNDCTL_DSP_GETFMTS,&sfmts);

    if (sfmts == AFMT_U8)
	fmt = AFMT_U8;         // its really an 8 bit only device
    else if (EST_LITTLE_ENDIAN)
	fmt = AFMT_S16_LE;  
    else
	fmt = AFMT_S16_BE;  
    
    ioctl(sbdevice,SNDCTL_DSP_SETFMT,&fmt);
    
    return fmt;
}

#define AUDIOBUFFSIZE 256
// #define AUDIOBUFFSIZE 20480

int play_voxware_wave(EST_Wave &inwave, EST_Option &al)
{
    int sample_rate;
    short *waveform;
    short *waveform2 = 0;
    int num_samples;
    int audio,actual_fmt;
    int i,r,n;
    char *audiodevice;

    if (al.present("-audiodevice"))
	audiodevice = al.val("-audiodevice");
    else
	audiodevice = "/dev/dsp";

    if ((audio = open(audiodevice,O_WRONLY)) == -1)
    {
	cerr << aud_sys_name << ": can't open " << audiodevice << endl;
	return -1;
    }
 
    // int tmp=open("/tmp/vox_play_wave",O_WRONLY|O_CREAT);
    
    waveform = inwave.values().memory();
    num_samples = inwave.num_samples();
    sample_rate = inwave.sample_rate();

    actual_fmt = sb_set_sample_rate(audio,sample_rate);

    if (stereo_only)
    {
	waveform2 = walloc(short,num_samples*2);
	for (i=0; i<num_samples; i++)
	{
	    waveform2[i*2] = inwave.a(i);
	    waveform2[(i*2)+1] = inwave.a(i);
	}
	waveform = waveform2;
	num_samples *= 2;
    }

    THREAD_DECS();
    THREAD_PROTECT();
    
    if (sb_set_sample_rate(audio,sample_rate) == AFMT_U8)
    {
	// Its actually 8bit unsigned so convert the buffer;
	unsigned char *uchars = walloc(unsigned char,num_samples);
	for (i=0; i < num_samples; i++)
	    uchars[i] = waveform[i]/256+128;
	for (i=0; i < num_samples; i += r)
	{
	    if (num_samples > i+AUDIOBUFFSIZE)
		n = AUDIOBUFFSIZE;
	    else
		n = num_samples-i;
	    // r = write(tmp,&uchars[i], n);
	    r = write(audio,&uchars[i], n);
	    if (r == 0)
	    {
		THREAD_UNPROTECT();
		cerr << aud_sys_name << ": failed to write to buffer" <<
		    sample_rate << endl;
		close(audio);
		return -1;
	    }
	}
	wfree(uchars);
    }
    else if ((actual_fmt == AFMT_S16_LE) || 
	     (actual_fmt == AFMT_S16_BE))
    {
      int blksize, nbuf, c;
      short *buf;

      ioctl (audio, SNDCTL_DSP_GETBLKSIZE, &blksize);
      nbuf=blksize;
      buf=new short[nbuf];

      for (i=0; i < num_samples; i += r/2)
	{
	  if (num_samples > i+nbuf)
	    n = nbuf;
	  else
	    n = num_samples-i;

	  for(c=0; c<n;c++)
	    buf[c]=waveform[c+i];

	  for(; c<nbuf;c++)
	    buf[c]=waveform[n-1];

	  // r = write(tmp,&waveform[i], n*2);
	  // r = write(audio,&waveform[i], n*2);
	  r=write(audio, buf, nbuf*2);
	  if (r <= 0)
	    {
	      THREAD_UNPROTECT();
	      EST_warning("%s: failed to write to buffer (sr=%d)",aud_sys_name, sample_rate );
		close(audio);
		return -1;
	    }
	    // ioctl(audio, SNDCTL_DSP_SYNC, 0);
	  // fprintf(stderr,"[%d]", r);
	}
      delete [] buf;
    }
    else
    {
      THREAD_UNPROTECT();
      cerr << aud_sys_name << ": unable to set sample rate " <<
	sample_rate << endl;
      close(audio);
      return -1;
    }
    
    // ioctl(audio, SNDCTL_DSP_SYNC, 0);
//    fprintf(stderr, "End Play\n");

    // close(tmp);
    close(audio);
    if (waveform2)
	wfree(waveform2);
    THREAD_UNPROTECT();
    return 1;
}

int record_voxware_wave(EST_Wave &inwave, EST_Option &al)
{
    int sample_rate=16000;  // egcs needs the initialized for some reason
    short *waveform;
    short *waveform2=0;
    int num_samples;
    int audio=-1,actual_fmt;
    int i,r,n;
    char *audiodevice;

    if (al.present("-audiodevice"))
	audiodevice = al.val("-audiodevice");
    else
	audiodevice = "/dev/dsp";

    sample_rate = al.ival("-sample_rate");
    
    if ((audio = open(audiodevice,O_RDONLY)) == -1)
    {
	cerr << aud_sys_name << ": can't open " << audiodevice
	     << "for reading" << endl;
	return -1;
    }
    
    actual_fmt = sb_set_sample_rate(audio,sample_rate);

    if ((actual_fmt == AFMT_S16_LE) || 
	(actual_fmt == AFMT_S16_BE))
    {
	// We assume that the device returns audio in native byte order
	// by default
	inwave.resize((int)(sample_rate*al.fval("-time")));
	inwave.set_sample_rate(sample_rate);
	num_samples = inwave.num_samples();
	waveform = inwave.values().memory();

	if (stereo_only)
	{
	    waveform2 = walloc(short,num_samples*2);
	    num_samples *= 2;
	}
	else
	    waveform2 = waveform;

	for (i=0; i < num_samples; i+= r)
	{
	    if (num_samples > i+AUDIOBUFFSIZE)
		n = AUDIOBUFFSIZE;
	    else
		n = num_samples-i;
	    r = read(audio,&waveform2[i], n*2);
	    r /= 2;
	    if (r <= 0)
	    {
		cerr << aud_sys_name << ": failed to read from audio device"
		    << endl;
		close(audio);
		return -1;
	    }
	}

    }
    else if (actual_fmt == AFMT_U8)
    {
	inwave.resize((int)(sample_rate*al.fval("-time")));
	inwave.set_sample_rate(sample_rate);
	num_samples = inwave.num_samples();
	waveform = inwave.values().memory();
	unsigned char *u8wave = walloc(unsigned char,num_samples);

	for (i=0; i < num_samples; i+= r)
	{
	    if (num_samples > i+AUDIOBUFFSIZE)
		n = AUDIOBUFFSIZE;
	    else
		n = num_samples-i;
	    r = read(audio,&u8wave[i],n);
	    if (r <= 0)
	    {
		cerr << aud_sys_name << ": failed to read from audio device"
		    << endl;
		close(audio);
		wfree(u8wave);
		return -1;
	    }
	    
	}
	uchar_to_short(u8wave,waveform,num_samples);
	wfree(u8wave);
    }
    else
    {
	cerr << aud_sys_name << ": unknown audio format from device: " << 
	    actual_fmt << endl;
	close(audio);
	return -1;
    }

    if (stereo_only)
    {
	for (i=0; i<num_samples; i+=2)
	    waveform[i/2] = waveform2[i];
	wfree(waveform2);
    }

    close(audio);
    return 0;
}

#else

int linux16_supported = FALSE;
int freebsd16_supported = FALSE;

int play_voxware_wave(EST_Wave &inwave, EST_Option &al)
{
    (void)inwave;
    (void)al;
    cerr << "Audio: voxware not compiled in this version" << endl;
    return -1;
}

int record_voxware_wave(EST_Wave &inwave, EST_Option &al)
{
    (void)inwave;
    (void)al;
    cerr << "Audio: voxware not compiled in this version" << endl;
    return -1;
}

#endif
