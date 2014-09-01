/*
 *	Command line frontend program
 *
 *	Copyright (c) 1999 Mark Taylor
 *                    2000 Takehiro TOMIANGA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "get_audio.h"        
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif


/* GLOBAL VARIABLES used by parse.c and main.c.  
   instantiated in parce.c.  ugly, ugly */
extern sound_file_format input_format;   
extern int swapbytes;              /* force byte swapping   default=0*/
extern int silent;
extern int brhist;

extern int mp3_delay;              /* for decoder only */
extern int mp3_delay_set;          /* for decoder only */
extern int enc_delay;             /* if decoder finds a Xing header */ 
extern int enc_padding;           /* if decoder finds a Xing header */ 
extern float update_interval;      /* to use Frank's time status display */
extern int disable_wav_header;     /* for decoder only */
extern mp3data_struct mp3input_data; /* used by Ogg and MP3 */
extern int in_signed;
extern int in_unsigned;
#define order_littleEndian 0
#define order_bigEndian 1
#define order_unknown 2
extern int in_endian;
extern int in_bitwidth;

#define         Min(A, B)       ((A) < (B) ? (A) : (B))
#define         Max(A, B)       ((A) > (B) ? (A) : (B))

