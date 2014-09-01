/***********************************************************
 * YUVDENOISER for the mjpegtools                          *
 * ------------------------------------------------------- *
 * (C) 2001-2004 Stefan Fendt                              *
 *                                                         *
 * Licensed and protected by the GNU-General-Public-       *
 * License version 2 or if you prefer any later version of *
 * that license). See the file LICENSE for detailed infor- *
 * mation.                                                 *
 *                                                         *
 * FILE: main.c                                            *
 *                                                         *
 ***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "mjpeg_types.h"
#include "yuv4mpeg.h"
#include "mjpeg_logging.h"
#include "cpu_accel.h"
#include "motionsearch.h"

int verbose = 1;
int width = 0;
int height = 0;
int lwidth = 0;
int lheight = 0;
int cwidth = 0;
int cheight = 0;
int input_chroma_subsampling = 0;
int input_interlaced = 0;
int hq_mode = 0;

int renoise_Y=0;
int renoise_U=0;
int renoise_V=0;

int temp_Y_thres = 0;
int temp_U_thres = 0;
int temp_V_thres = 0;

int med_pre_Y_thres = 0;
int med_pre_U_thres = 0;
int med_pre_V_thres = 0;

int med_post_Y_thres = 0;
int med_post_U_thres = 0;
int med_post_V_thres = 0;

int gauss_Y = 0;
int gauss_U = 0;
int gauss_V = 0;

uint8_t *frame1[3];
uint8_t *frame2[3];
uint8_t *frame3[3];
uint8_t *frame4[3];
uint8_t *frame5[3];
uint8_t *frame6[3];
uint8_t *frame7[3];

uint8_t *scratchplane1;
uint8_t *scratchplane2;
uint8_t *outframe[3];

int buff_offset;
int buff_size;

uint16_t transform_L16[256];
uint8_t transform_G8[65536];

/***********************************************************
 * helper-functions                                        *
 ***********************************************************/


void
gauss_filter_plane (uint8_t * frame, int w, int h, int t)
{
int i;
int v;
uint8_t * src = frame;
uint8_t * dst = scratchplane1;

if(t==0) return;

memcpy ( src-w*2, src, w );
memcpy ( src-w  , src, w );

memcpy ( src+(w*h)  , src+(w*h)-w, w );
memcpy ( src+(w*h)+w, src+(w*h)-w, w );

for(i=0;i<(w*h);i++)
	{

	v  = *(src    -2)*1;

	v += *(src-w  -1)*1;
	v += *(src    -1)*2;
	v += *(src+w  -1)*1;

	v += *(src-w*2  )*1;
	v += *(src-w    )*2;
	v += *(src      )*4;
	v += *(src+w    )*2;
	v += *(src+w*2  )*1;

	v += *(src-w  +1)*1;
	v += *(src    +1)*2;
	v += *(src+w  +1)*1;

	v += *(src    +2)*1;

	v /= 20;

	v = *(src)*(256-t) + (v)*(t);
	v /= 256;

	*(dst)=v;

	dst++;
	src++;
	}

memcpy ( frame,scratchplane1,w*h );
}

void
temporal_filter_planes (int idx, int w, int h, int t)
{
  uint32_t r, c, m;
  int32_t d;
  int x;

  uint8_t *f1 = frame1[idx];
  uint8_t *f2 = frame2[idx];
  uint8_t *f3 = frame3[idx];
  uint8_t *f4 = frame4[idx];
  uint8_t *f5 = frame5[idx];
  uint8_t *f6 = frame6[idx];
  uint8_t *f7 = frame7[idx];
  uint8_t *of = outframe[idx];

  if (t == 0)			// shortcircuit filter if t = 0...
    {
      memcpy (of, f4, w * h);
      return;
    }

      for (x = 0; x < (w * h); x++)
	{

	  r  = *(f4-1-w);
	  r += *(f4  -w)*2;
	  r += *(f4+1-w);
          r += *(f4-1  )*2;
	  r += *(f4    )*4;
          r += *(f4+1  )*2;
	  r += *(f4-1+w);
          r += *(f4  +w)*2;
          r += *(f4+1+w);
	  r /= 16;

	  m = *(f4)*(t+1)*2;
	  c = t+1;

	  d  = *(f3-1-w);
	  d += *(f3  -w)*2;
	  d += *(f3+1-w);
          d += *(f3-1  )*2;
	  d += *(f3    )*4;
          d += *(f3+1  )*2;
	  d += *(f3-1+w);
          d += *(f3  +w)*2;
          d += *(f3+1+w);
	  d /= 16;

	  d = t - abs (r-d);
	  d = d<0? 0:d;
	  c += d;
          m += *(f3)*d*2;

	  d  = *(f2-1-w);
	  d += *(f2  -w)*2;
	  d += *(f2+1-w);
          d += *(f2-1  )*2;
	  d += *(f2    )*4;
          d += *(f2+1  )*2;
	  d += *(f2-1+w);
          d += *(f2  +w)*2;
          d += *(f2+1+w);
	  d /= 16;

	  d = t - abs (r-d);
	  d = d<0? 0:d;
	  c += d;
          m += *(f2)*d*2;

	  d  = *(f1-1-w);
	  d += *(f1  -w)*2;
	  d += *(f1+1-w);
          d += *(f1-1  )*2;
	  d += *(f1    )*4;
          d += *(f1+1  )*2;
	  d += *(f1-1+w);
          d += *(f1  +w)*2;
          d += *(f1+1+w);
	  d /= 16;

	  d = t - abs (r-d);
	  d = d<0? 0:d;
	  c += d;
          m += *(f1)*d*2;

	  d  = *(f5-1-w);
	  d += *(f5  -w)*2;
	  d += *(f5+1-w);
          d += *(f5-1  )*2;
	  d += *(f5    )*4;
          d += *(f5+1  )*2;
	  d += *(f5-1+w);
          d += *(f5  +w)*2;
          d += *(f5+1+w);
	  d /= 16;

	  d = t - abs (r-d);
	  d = d<0? 0:d;
	  c += d;
          m += *(f5)*d*2;

	  d  = *(f6-1-w);
	  d += *(f6  -w)*2;
	  d += *(f6+1-w);
          d += *(f6-1  )*2;
	  d += *(f6    )*4;
          d += *(f6+1  )*2;
	  d += *(f6-1+w);
          d += *(f6  +w)*2;
          d += *(f6+1+w);
	  d /= 16;

	  d = t - abs (r-d);
	  d = d<0? 0:d;
	  c += d;
          m += *(f6)*d*2;

	  d  = *(f7-1-w);
	  d += *(f7  -w)*2;
	  d += *(f7+1-w);
          d += *(f7-1  )*2;
	  d += *(f7    )*4;
          d += *(f7+1  )*2;
	  d += *(f7-1+w);
          d += *(f7  +w)*2;
          d += *(f7+1+w);
	  d /= 16;

	  d = t - abs (r-d);
	  d = d<0? 0:d;
	  c += d;
          m += *(f7)*d*2;

	  *(of) = ((m/c)+1)/2;

	  f1++;
	  f2++;
	  f3++;
	  f4++;
	  f5++;
	  f6++;
	  f7++;
	  of++;
	}
}

void
temporal_filter_planes_MC (int idx, int w, int h, int t)
{
  uint32_t sad,min;
  uint32_t r, c, m;
  int32_t d;
  int x,y,sx,sy;

  uint32_t v;
  int x1,y1;
  int x2,y2;
  int x3,y3;
  int x5,y5;
  int x6,y6;
  int x7,y7;

  uint8_t *f1 = frame1[idx];
  uint8_t *f2 = frame2[idx];
  uint8_t *f3 = frame3[idx];
  uint8_t *f4 = frame4[idx];
  uint8_t *f5 = frame5[idx];
  uint8_t *f6 = frame6[idx];
  uint8_t *f7 = frame7[idx];
  uint8_t *of = outframe[idx];

#if 0

  if (t == 0)			// shortcircuit filter if t = 0...
    {
      memcpy (of, f4, w * h);
      return;
    }
#endif

      for (y = 0; y < h; y+=16)
      for (x = 0; x < w; x+=16)
	{

	// find best matching 16x16 block for f3
	min=psad_00 ( f4+(x)+(y)*w,f3+(x)+(y)*w,w,16,0x00ffffff );
	x3=y3=0;
	for (sy=-4; sy < 4; sy++)
	for (sx=-4; sx < 4; sx++)
	{
		sad  = psad_00 ( f4+(x)+(y)*w,f3+(x+sx)+(y+sy)*w,w,16,0x00ffffff );
		sad += psad_00 ( f4+(x+8)+(y)*w,f3+(x+sx+8)+(y+sy)*w,w,16,0x00ffffff );
		if(sad<min)
		{
		x3 = sx;
		y3 = sy;
		min = sad;
		}
	}

	// find best matching 16x16 block for f5
	min=psad_00 ( f4+(x)+(y)*w,f5+(x)+(y)*w,w,16,0x00ffffff );
	x5=y5=0;
	for (sy=-4; sy < 4; sy++)
	for (sx=-4; sx < 4; sx++)
	{
		sad  = psad_00 ( f4+(x)+(y)*w,f5+(x+sx)+(y+sy)*w,w,16,0x00ffffff );
		sad += psad_00 ( f4+(x+8)+(y)*w,f5+(x+sx+8)+(y+sy)*w,w,16,0x00ffffff );
		if(sad<min)
		{
		x5 = sx;
		y5 = sy;
		min = sad;
		}
	}

	// find best matching 16x16 block for f2
	min=psad_00 ( f4+(x)+(y)*w,f2+(x)+(y)*w,w,16,0x00ffffff );
	x2=y2=0;
	for (sy=(y3-4); sy < (y3+4); sy++)
	for (sx=(x3-4); sx < (x3+4); sx++)
	{
		sad  = psad_00 ( f4+(x)+(y)*w,f2+(x+sx)+(y+sy)*w,w,16,0x00ffffff );
		sad += psad_00 ( f4+(x+8)+(y)*w,f2+(x+sx+8)+(y+sy)*w,w,16,0x00ffffff );
		if(sad<min)
		{
		x2 = sx;
		y2 = sy;
		min = sad;
		}
	}

	// find best matching 16x16 block for f6
	min=psad_00 ( f4+(x)+(y)*w,f6+(x)+(y)*w,w,16,0x00ffffff );
	x6=y6=0;
	for (sy=(y5-4); sy < (y5+4); sy++)
	for (sx=(x5-4); sx < (x5+4); sx++)
	{
		sad  = psad_00 ( f4+(x)+(y)*w,f6+(x+sx)+(y+sy)*w,w,16,0x00ffffff );
		sad += psad_00 ( f4+(x+8)+(y)*w,f6+(x+sx+8)+(y+sy)*w,w,16,0x00ffffff );
		if(sad<min)
		{
		x6 = sx;
		y6 = sy;
		min = sad;
		}
	}

	// find best matching 16x16 block for f2
	min=psad_00 ( f4+(x)+(y)*w,f1+(x)+(y)*w,w,16,0x00ffffff );
	x1=y1=0;
	for (sy=(y2-4); sy < (y2+4); sy++)
	for (sx=(x2-4); sx < (x2+4); sx++)
	{
		sad  = psad_00 ( f4+(x)+(y)*w,f1+(x+sx)+(y+sy)*w,w,16,0x00ffffff );
		sad += psad_00 ( f4+(x+8)+(y)*w,f1+(x+sx+8)+(y+sy)*w,w,16,0x00ffffff );
		if(sad<min)
		{
		x1 = sx;
		y1 = sy;
		min = sad;
		}
	}

	// find best matching 16x16 block for f7
	min=psad_00 ( f4+(x)+(y)*w,f7+(x)+(y)*w,w,16,0x00ffffff );
	x7=y7=0;
	for (sy=(y6-4); sy < (y6+4); sy++)
	for (sx=(x6-4); sx < (x6+4); sx++)
	{
		sad  = psad_00 ( f4+(x)+(y)*w,f7+(x+sx)+(y+sy)*w,w,16,0x00ffffff );
		sad += psad_00 ( f4+(x+8)+(y)*w,f7+(x+sx+8)+(y+sy)*w,w,16,0x00ffffff );
		if(sad<min)
		{
		x7 = sx;
		y7 = sy;
		min = sad;
		}
	}

	for (sy=0; sy < 16; sy++)
	for (sx=0; sx < 16; sx++)
	{
		// gauss-filtered reference pixel
		r  = *(f4+(x+sx-1)+(y+sy-1)*w);
		r += *(f4+(x+sx  )+(y+sy-1)*w)*2;
		r += *(f4+(x+sx+1)+(y+sy-1)*w);
		r += *(f4+(x+sx-1)+(y+sy  )*w)*2;
		r += *(f4+(x+sx  )+(y+sy  )*w)*4;
		r += *(f4+(x+sx+1)+(y+sy  )*w)*2;
		r += *(f4+(x+sx-1)+(y+sy+1)*w);
		r += *(f4+(x+sx  )+(y+sy+1)*w)*2;
		r += *(f4+(x+sx+1)+(y+sy+1)*w);
		r /= 16;

		// add non-filtered reference to the accummulator
		m = *(f4+(x+sx  )+(y+sy  )*w)*t;
		c = t;

		// gauss-filtered and translated test pixel
		sx += x1;
		sy += y1;
		v  = *(f1+(x+sx-1)+(y+sy-1)*w);
		v += *(f1+(x+sx  )+(y+sy-1)*w)*2;
		v += *(f1+(x+sx+1)+(y+sy-1)*w);
		v += *(f1+(x+sx-1)+(y+sy  )*w)*2;
		v += *(f1+(x+sx  )+(y+sy  )*w)*4;
		v += *(f1+(x+sx+1)+(y+sy  )*w)*2;
		v += *(f1+(x+sx-1)+(y+sy+1)*w);
		v += *(f1+(x+sx  )+(y+sy+1)*w)*2;
		v += *(f1+(x+sx+1)+(y+sy+1)*w);
		sx -= x1;
		sy -= y1;
		v /= 16;

		// add weighted and translated but non-filtered test-pixel
		d = t - abs (r-v);
		d = d<0? 0:d;
	  	c += d;
          	m += *(f1+(x+sx+x1)+(y+sy+y1)*w)*d;

		// gauss-filtered and translated test pixel
		sx += x2;
		sy += y2;
		v  = *(f2+(x+sx-1)+(y+sy-1)*w);
		v += *(f2+(x+sx  )+(y+sy-1)*w)*2;
		v += *(f2+(x+sx+1)+(y+sy-1)*w);
		v += *(f2+(x+sx-1)+(y+sy  )*w)*2;
		v += *(f2+(x+sx  )+(y+sy  )*w)*4;
		v += *(f2+(x+sx+1)+(y+sy  )*w)*2;
		v += *(f2+(x+sx-1)+(y+sy+1)*w);
		v += *(f2+(x+sx  )+(y+sy+1)*w)*2;
		v += *(f2+(x+sx+1)+(y+sy+1)*w);
		sx -= x2;
		sy -= y2;
		v /= 16;

		// add weighted and translated but non-filtered test-pixel
		d = t - abs (r-v);
		d = d<0? 0:d;
	  	c += d;
          	m += *(f2+(x+sx+x2)+(y+sy+y2)*w)*d;

		// gauss-filtered and translated test pixel
		sx += x3;
		sy += y3;
		v  = *(f3+(x+sx-1)+(y+sy-1)*w);
		v += *(f3+(x+sx  )+(y+sy-1)*w)*2;
		v += *(f3+(x+sx+1)+(y+sy-1)*w);
		v += *(f3+(x+sx-1)+(y+sy  )*w)*2;
		v += *(f3+(x+sx  )+(y+sy  )*w)*4;
		v += *(f3+(x+sx+1)+(y+sy  )*w)*2;
		v += *(f3+(x+sx-1)+(y+sy+1)*w);
		v += *(f3+(x+sx  )+(y+sy+1)*w)*2;
		v += *(f3+(x+sx+1)+(y+sy+1)*w);
		sx -= x3;
		sy -= y3;
		v /= 16;

		// add weighted and translated but non-filtered test-pixel
		d = t - abs (r-v);
		d = d<0? 0:d;
	  	c += d;
          	m += *(f3+(x+sx+x3)+(y+sy+y3)*w)*d;

		// gauss-filtered and translated test pixel
		sx += x5;
		sy += y5;
		v  = *(f5+(x+sx-1)+(y+sy-1)*w);
		v += *(f5+(x+sx  )+(y+sy-1)*w)*2;
		v += *(f5+(x+sx+1)+(y+sy-1)*w);
		v += *(f5+(x+sx-1)+(y+sy  )*w)*2;
		v += *(f5+(x+sx  )+(y+sy  )*w)*4;
		v += *(f5+(x+sx+1)+(y+sy  )*w)*2;
		v += *(f5+(x+sx-1)+(y+sy+1)*w);
		v += *(f5+(x+sx  )+(y+sy+1)*w)*2;
		v += *(f5+(x+sx+1)+(y+sy+1)*w);
		sx -= x5;
		sy -= y5;
		v /= 16;

		// add weighted and translated but non-filtered test-pixel
		d = t - abs (r-v);
		d = d<0? 0:d;
	  	c += d;
          	m += *(f5+(x+sx+x5)+(y+sy+y5)*w)*d;

		// gauss-filtered and translated test pixel
		sx += x6;
		sy += y6;
		v  = *(f6+(x+sx-1)+(y+sy-1)*w);
		v += *(f6+(x+sx  )+(y+sy-1)*w)*2;
		v += *(f6+(x+sx+1)+(y+sy-1)*w);
		v += *(f6+(x+sx-1)+(y+sy  )*w)*2;
		v += *(f6+(x+sx  )+(y+sy  )*w)*4;
		v += *(f6+(x+sx+1)+(y+sy  )*w)*2;
		v += *(f6+(x+sx-1)+(y+sy+1)*w);
		v += *(f6+(x+sx  )+(y+sy+1)*w)*2;
		v += *(f6+(x+sx+1)+(y+sy+1)*w);
		sx -= x6;
		sy -= y6;
		v /= 16;

		// add weighted and translated but non-filtered test-pixel
		d = t - abs (r-v);
		d = d<0? 0:d;
	  	c += d;
          	m += *(f6+(x+sx+x6)+(y+sy+y6)*w)*d;

		// gauss-filtered and translated test pixel
		sx += x7;
		sy += y7;
		v  = *(f7+(x+sx-1)+(y+sy-1)*w);
		v += *(f7+(x+sx  )+(y+sy-1)*w)*2;
		v += *(f7+(x+sx+1)+(y+sy-1)*w);
		v += *(f7+(x+sx-1)+(y+sy  )*w)*2;
		v += *(f7+(x+sx  )+(y+sy  )*w)*4;
		v += *(f7+(x+sx+1)+(y+sy  )*w)*2;
		v += *(f7+(x+sx-1)+(y+sy+1)*w);
		v += *(f7+(x+sx  )+(y+sy+1)*w)*2;
		v += *(f7+(x+sx+1)+(y+sy+1)*w);
		sx -= x7;
		sy -= y7;
		v /= 16;

		// add weighted and translated but non-filtered test-pixel
		d = t - abs (r-v);
		d = d<0? 0:d;
	  	c += d;
          	m += *(f7+(x+sx+x7)+(y+sy+y7)*w)*d;

		*(of+(x+sx)+(y+sy)*w) = m/c;

	}
	}
}

void renoise (uint8_t * frame, int w, int h, int level )
{
uint8_t random[8192];
int i;
static int cnt=0;

for(i=0;i<8192;i++)
	random[(i+cnt/2)&8191]=(i+i+i+i*i*i+1-random[i-1]+random[i-20]*random[i-5]*random[i-25])&255;
cnt++;

for(i=0;i<(w*h);i++)
	*(frame+i)=(*(frame+i)*(255-level)+random[i&8191]*level)/255;
}


void filter_plane_median ( uint8_t * plane, int w, int h, int level)
{
	int i;
	int min;
	int max;
	int avg;
	int cnt;
	int c;
	int e;
	uint8_t * p;
	uint8_t * d;

	if(level==0) return;

	p = plane;
	d = scratchplane1;

	// remove strong outliers from the image. An outlier is a pixel which lies outside
	// of max-thres and min+thres of the surrounding pixels. This should not cause blurring
	// and it should leave an evenly spread noise-floor to the image.
	for(i=0;i<=(w*h);i++)
	{
	// reset min/max-filter
	min=255;
	max=0;
	// check every remaining position arround the reference-pixel for being min/max...

	min=(min>*(p-w-1))? *(p-w-1):min;
	max=(max<*(p-w-1))? *(p-w-1):max;
	min=(min>*(p-w+0))? *(p-w+0):min;
	max=(max<*(p-w+0))? *(p-w+0):max;
	min=(min>*(p-w+1))? *(p-w+1):min;
	max=(max<*(p-w+1))? *(p-w+1):max;

	min=(min>*(p  -1))? *(p  -1):min;
	max=(max<*(p  -1))? *(p  -1):max;
	min=(min>*(p  +1))? *(p  +1):min;
	max=(max<*(p  +1))? *(p  +1):max;

	min=(min>*(p+w-1))? *(p+w-1):min;
	max=(max<*(p+w-1))? *(p+w-1):max;
	min=(min>*(p+w+0))? *(p+w+0):min;
	max=(max<*(p+w+0))? *(p+w+0):max;
	min=(min>*(p+w+1))? *(p+w+1):min;
	max=(max<*(p+w+1))? *(p+w+1):max;

	if( *(p)<(min) )
	{
	*(d)=min;
	}
	else
	if( *(p)>(max) )
	{
	*(d)=max;
	}
	else
	{
	*(d)=*(p);
	}

	d++;
	p++;
	}

	// in the second stage we try to average similar spatial pixels, only. This, like
	// a median, should also not reduce sharpness but flatten the noisefloor. This
	// part is quite similar to what 2dclean/yuvmedianfilter do. But because of the
	// different weights given to the pixels it is less aggressive...

	p = scratchplane1;
	d = scratchplane2;

	// this filter needs values outside of the imageplane, so we just copy the first line 
	// and the last line into the out-of-range area...

	memcpy ( p-w  , p, w );
	memcpy ( p-w*2, p, w );

	memcpy ( p+(w*h)  , p+(w*h)-w, w );
	memcpy ( p+(w*h)+w, p+(w*h)-w, w );

	for(i=0;i<=(w*h);i++)
	{
		avg=*(p)*level*2;
		cnt=level;

		c = *(p-w*2-2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-w*2-1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-w*2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-w*2+1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-w*2+2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-w*1-2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-w*1-1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-w*1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-w*1+1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-w*1+2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p-1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*1-2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*1-1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*1+1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*1+2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*2-2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*2-1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*2+1);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		c = *(p+w*2+2);
		e = abs(c-*(p));
		e = ((level-e)<0)? 0:level-e;
		avg += e*c*2;
		cnt += e;

		*(d)=(((avg/cnt)+1)/2);

		d++;
		p++;
	}

	memcpy(plane,scratchplane2,w*h);
}

/***********************************************************
 * Main Loop                                               *
 ***********************************************************/

int
main (int argc, char *argv[])
{
  int c;
  int fd_in = fileno(stdin);
  int fd_out = fileno(stdout);
  int err = 0;
  char *msg = NULL;
  y4m_ratio_t rx, ry;
  y4m_frame_info_t iframeinfo;
  y4m_stream_info_t istreaminfo;
  y4m_frame_info_t oframeinfo;
  y4m_stream_info_t ostreaminfo;

  mjpeg_info("yuvdenoise version %s", VERSION);

  while ((c = getopt (argc, argv, "qhvt:g:m:M:r:G:")) != -1)
    {
      switch (c)
	{
	case 'h':
	  {
  	    mjpeg_info("... | yuvdenoise [OPTIONS] | ...");
  	    mjpeg_info("Brief description of the accepted options:\n");
  	    mjpeg_info("-g [Y=0...255],[U=0...255],[V=0...255]");
  	    mjpeg_info("   This sets the parameters [Y,U,V] for the gauss-filter. 0 means no filtering,");
  	    mjpeg_info("    which is the default, 255 is maximum gaussfiltering. Some camera manufacturers");
  	    mjpeg_info("    think it's a good idea to sharpen the frames extremly. This however will raise");
  	    mjpeg_info("    typical video-compression-artifacts (ringing, blocking, alias, etc...). If you");
  	    mjpeg_info("    desire to have a video free of these, you can raise the gauss-filter-values");
  	    mjpeg_info("    until your image is undesirable soft... (Short: setting decent values helps");
  	    mjpeg_info("    both: the viewer and the encoder -- setting too high values is ugly.) This may");
  	    mjpeg_info("    also help with extremly noisy images from a weak air-captured signal...");
  	    mjpeg_info("    Unlike y4mspatialfilter this will not boost ringing artifacts.\n");
  	    mjpeg_info("-m [0...255],[0...255],[0...255]");
  	    mjpeg_info("    Spatial-Pre-Filter. This one helps with really noisy signals. You should not");
  	    mjpeg_info("    use this with moderate to low noise material. Used with care it helps the tem-");
	     mjpeg_info("   poral-filter a lot. Misuse will lead to rather dull images (like an overly median-filtered image...");
  	    mjpeg_info("-t [0...255],[0...255],[0...255]");
  	    mjpeg_info("    Temporal-Noise-Filter. This one dramaticaly reduces noise without loosing sharpness. If set too high, however, it may introduce visable ghost-images or smear. ");
  	    mjpeg_info("-M [0...255],[0...255],[0...255]");
  	    mjpeg_info("    Spatial-Post-Filter. This one removes spatial noise left by the temporal-filter.");
  	    mjpeg_info("    Used with care it can dramaticaly lower the bitrate. Using it with to high settings will lead to the same artifacts as the spatial-pre-filter produces.");
  	    mjpeg_info("-G [0...255],[0...255],[0...255]");
  	    mjpeg_info("    Set -m,-t,-M values at once. -t is boosted by a factor of 2. Useful if you do");
  	    mjpeg_info("    not want to tweak all the values by hand... which is better.");
  	    mjpeg_info("-r [0...255],[0...255],[0...255]");
  	    mjpeg_info("    Add some static masking noise. Might be used as an effect, too... *g*");
  	    mjpeg_info("-q  HighQuality-Mode. Warning: On almost any machine this is dead slow...");
	    exit (0);
	    break;
	  }
	case 'v':
	  {
	    verbose = 1;
	    break;
	  }
	case 'q':
	  {
	    hq_mode = 1;
	    break;
	  }
	case 't':
	  {
	    sscanf (optarg, "%i,%i,%i", &temp_Y_thres, &temp_U_thres,
		    &temp_V_thres);
	    break;
	  }
	case 'g':
	  {
	    sscanf (optarg, "%i,%i,%i", &gauss_Y, &gauss_U, &gauss_V);
	    break;
	  }
	case 'm':
	  {
	    sscanf (optarg, "%i,%i,%i", &med_pre_Y_thres, &med_pre_U_thres, &med_pre_V_thres);
	    break;
	  }
	case 'M':
	  {
	    sscanf (optarg, "%i,%i,%i", &med_post_Y_thres, &med_post_U_thres, &med_post_V_thres);
	    break;
	  }
	case 'G':
	  {
	    sscanf (optarg, "%i,%i,%i", &med_pre_Y_thres, &med_pre_U_thres, &med_pre_V_thres);
	    med_post_Y_thres = temp_Y_thres = med_pre_Y_thres;
	    med_post_U_thres = temp_U_thres = med_pre_U_thres;
	    med_post_V_thres = temp_V_thres = med_pre_V_thres;
	    temp_Y_thres *= 2;
	    temp_U_thres *= 2;
	    temp_V_thres *= 2;
	    break;
	  }
	case 'r':
	  {
	    sscanf (optarg, "%i,%i,%i", &renoise_Y, &renoise_U, &renoise_V);
	    break;
	  }
	case '?':
	default:
	  exit (1);
	}
    }

  mjpeg_info("Using the following thresholds/settings:");
  mjpeg_info("Gauss-Pre-Filter      [Y,U,V] : [%i,%i,%i]",
	     gauss_Y, gauss_U, gauss_V);
  mjpeg_info("Median-Pre-Filter     [Y,U,V] : [%i,%i,%i]",
	     med_pre_Y_thres, med_pre_U_thres, med_pre_V_thres);
  mjpeg_info("Temporal-Noise-Filter [Y,U,V] : [%i,%i,%i]",
	     temp_Y_thres, temp_U_thres, temp_V_thres);
  mjpeg_info("Median-Post-Filter    [Y,U,V] : [%i,%i,%i]",
	     med_post_Y_thres, med_post_U_thres, med_post_V_thres);
  mjpeg_info("Renoise               [Y,U,V] : [%i,%i,%i]",
	     renoise_Y, renoise_U, renoise_V);
  mjpeg_info("HQ-Mode                       : %s",
	     (hq_mode==0? "off":"on"));

  /* initialize stream-information */
  y4m_accept_extensions (1);
  y4m_init_stream_info (&istreaminfo);
  y4m_init_frame_info (&iframeinfo);
  y4m_init_stream_info (&ostreaminfo);
  y4m_init_frame_info (&oframeinfo);

  /* open input stream */
  if ((err = y4m_read_stream_header (fd_in, &istreaminfo)) != Y4M_OK)
      mjpeg_error_exit1("Couldn't read YUV4MPEG header: %s!", y4m_strerr (err));

  /* get format information */
  width = y4m_si_get_width (&istreaminfo);
  height = y4m_si_get_height (&istreaminfo);
  input_chroma_subsampling = y4m_si_get_chroma (&istreaminfo);
  input_interlaced = y4m_si_get_interlace (&istreaminfo);
  mjpeg_info("Y4M-Stream %ix%i(%s)",
	     width,
	     height, y4m_chroma_description (input_chroma_subsampling));

  lwidth = width;
  lheight = height;

  // Setup the denoiser to use the appropriate chroma processing
  if (input_chroma_subsampling == Y4M_CHROMA_420JPEG ||
      input_chroma_subsampling == Y4M_CHROMA_420MPEG2 ||
      input_chroma_subsampling == Y4M_CHROMA_420PALDV)
    msg = "Processing Mode : 4:2:0";
  else if (input_chroma_subsampling == Y4M_CHROMA_411)
    msg = "Processing Mode : 4:1:1";
  else if (input_chroma_subsampling == Y4M_CHROMA_422)
    msg = "Processing Mode : 4:2:2";
  else if (input_chroma_subsampling == Y4M_CHROMA_444)
    msg = "Processing Mode : 4:4:4";
  else
    mjpeg_error_exit1 (" ### Unsupported Y4M Chroma sampling ### ");

  rx = y4m_chroma_ss_x_ratio (input_chroma_subsampling);
  ry = y4m_chroma_ss_y_ratio (input_chroma_subsampling);
  cwidth = width / rx.d;
  cheight = height / ry.d;

  mjpeg_info("%s %s", msg,
	     (input_interlaced ==
	      Y4M_ILACE_NONE) ? "progressive" : "interlaced");
  mjpeg_info("Luma-Plane      : %ix%i pixels", lwidth, lheight);
  mjpeg_info("Chroma-Plane    : %ix%i pixels", cwidth, cheight);

  if (input_interlaced != Y4M_ILACE_NONE)
    {
      // process the fields as images side by side
      lwidth *= 2;
      cwidth *= 2;
      lheight /= 2;
      cheight /= 2;
    }

  y4m_si_set_interlace (&ostreaminfo, y4m_si_get_interlace (&istreaminfo));
  y4m_si_set_chroma (&ostreaminfo, y4m_si_get_chroma (&istreaminfo));
  y4m_si_set_width (&ostreaminfo, width);
  y4m_si_set_height (&ostreaminfo, height);
  y4m_si_set_framerate (&ostreaminfo, y4m_si_get_framerate (&istreaminfo));
  y4m_si_set_sampleaspect (&ostreaminfo,
			   y4m_si_get_sampleaspect (&istreaminfo));

  /* write the outstream header */
  y4m_write_stream_header (fd_out, &ostreaminfo);

  /* now allocate the needed buffers */
  {
    /* calculate the memory offset needed to allow the processing
     * functions to overshot. The biggest overshot is needed for the
     * MC-functions, so we'll use 8*width...
     */
    buff_offset = lwidth * 8;
    buff_size = buff_offset * 2 + lwidth * lheight;

    frame1[0] = buff_offset + (uint8_t *) malloc (buff_size);
    frame1[1] = buff_offset + (uint8_t *) malloc (buff_size);
    frame1[2] = buff_offset + (uint8_t *) malloc (buff_size);

    frame2[0] = buff_offset + (uint8_t *) malloc (buff_size);
    frame2[1] = buff_offset + (uint8_t *) malloc (buff_size);
    frame2[2] = buff_offset + (uint8_t *) malloc (buff_size);

    frame3[0] = buff_offset + (uint8_t *) malloc (buff_size);
    frame3[1] = buff_offset + (uint8_t *) malloc (buff_size);
    frame3[2] = buff_offset + (uint8_t *) malloc (buff_size);

    frame4[0] = buff_offset + (uint8_t *) malloc (buff_size);
    frame4[1] = buff_offset + (uint8_t *) malloc (buff_size);
    frame4[2] = buff_offset + (uint8_t *) malloc (buff_size);

    frame5[0] = buff_offset + (uint8_t *) malloc (buff_size);
    frame5[1] = buff_offset + (uint8_t *) malloc (buff_size);
    frame5[2] = buff_offset + (uint8_t *) malloc (buff_size);

    frame6[0] = buff_offset + (uint8_t *) malloc (buff_size);
    frame6[1] = buff_offset + (uint8_t *) malloc (buff_size);
    frame6[2] = buff_offset + (uint8_t *) malloc (buff_size);

    frame7[0] = buff_offset + (uint8_t *) malloc (buff_size);
    frame7[1] = buff_offset + (uint8_t *) malloc (buff_size);
    frame7[2] = buff_offset + (uint8_t *) malloc (buff_size);

    outframe[0] = buff_offset + (uint8_t *) malloc (buff_size);
    outframe[1] = buff_offset + (uint8_t *) malloc (buff_size);
    outframe[2] = buff_offset + (uint8_t *) malloc (buff_size);

    scratchplane1 = buff_offset + (uint8_t *) malloc (buff_size);
    scratchplane2 = buff_offset + (uint8_t *) malloc (buff_size);

    mjpeg_info("Buffers allocated.");
  }

  /* initialize motion_library */
  init_motion_search ();

  /* read every frame until the end of the input stream and process it */
  while (Y4M_OK == (err = y4m_read_frame (fd_in,
					    &istreaminfo,
					    &iframeinfo, frame1)))
    {

      static uint32_t frame_nr = 0;
      uint8_t *temp[3];

      frame_nr++;

	gauss_filter_plane (frame1[0], lwidth, lheight, gauss_Y);
	gauss_filter_plane (frame1[1], cwidth, cheight, gauss_U);
	gauss_filter_plane (frame1[2], cwidth, cheight, gauss_V);

	filter_plane_median (frame1[0], lwidth, lheight, med_pre_Y_thres);
	filter_plane_median (frame1[1], cwidth, cheight, med_pre_U_thres);
	filter_plane_median (frame1[2], cwidth, cheight, med_pre_V_thres);

	if(hq_mode==1)
	{
		temporal_filter_planes_MC (0, lwidth, lheight, temp_Y_thres);
      		temporal_filter_planes_MC (1, cwidth, cheight, temp_U_thres);
      		temporal_filter_planes_MC (2, cwidth, cheight, temp_V_thres);
	}
	else
	{
		temporal_filter_planes (0, lwidth, lheight, temp_Y_thres);
      		temporal_filter_planes (1, cwidth, cheight, temp_U_thres);
      		temporal_filter_planes (2, cwidth, cheight, temp_V_thres);
	}

	filter_plane_median (outframe[0], lwidth, lheight, med_post_Y_thres);
	filter_plane_median (outframe[1], cwidth, cheight, med_post_U_thres);
	filter_plane_median (outframe[2], cwidth, cheight, med_post_V_thres);

      	renoise (outframe[0], lwidth, lheight, renoise_Y );
      	renoise (outframe[1], cwidth, cheight, renoise_U );
      	renoise (outframe[2], cwidth, cheight, renoise_V );

      if (frame_nr >= 4)
	y4m_write_frame (fd_out, &ostreaminfo, &oframeinfo, outframe);

      // rotate buffer pointers to rotate input-buffers
      temp[0] = frame7[0];
      temp[1] = frame7[1];
      temp[2] = frame7[2];

      frame7[0] = frame6[0];
      frame7[1] = frame6[1];
      frame7[2] = frame6[2];

      frame6[0] = frame5[0];
      frame6[1] = frame5[1];
      frame6[2] = frame5[2];

      frame5[0] = frame4[0];
      frame5[1] = frame4[1];
      frame5[2] = frame4[2];

      frame4[0] = frame3[0];
      frame4[1] = frame3[1];
      frame4[2] = frame3[2];

      frame3[0] = frame2[0];
      frame3[1] = frame2[1];
      frame3[2] = frame2[2];

      frame2[0] = frame1[0];
      frame2[1] = frame1[1];
      frame2[2] = frame1[2];

      frame1[0] = temp[0];
      frame1[1] = temp[1];
      frame1[2] = temp[2];

    }
	// write out the left frames...
	y4m_write_frame (fd_out, &ostreaminfo, &oframeinfo, frame4);
	y4m_write_frame (fd_out, &ostreaminfo, &oframeinfo, frame3);
	y4m_write_frame (fd_out, &ostreaminfo, &oframeinfo, frame2);
	y4m_write_frame (fd_out, &ostreaminfo, &oframeinfo, frame1);

  /* free allocated buffers */
  {
    free (frame1[0] - buff_offset);
    free (frame1[1] - buff_offset);
    free (frame1[2] - buff_offset);

    free (frame2[0] - buff_offset);
    free (frame2[1] - buff_offset);
    free (frame2[2] - buff_offset);

    free (frame3[0] - buff_offset);
    free (frame3[1] - buff_offset);
    free (frame3[2] - buff_offset);

    free (frame4[0] - buff_offset);
    free (frame4[1] - buff_offset);
    free (frame4[2] - buff_offset);

    free (frame5[0] - buff_offset);
    free (frame5[1] - buff_offset);
    free (frame5[2] - buff_offset);

    free (frame6[0] - buff_offset);
    free (frame6[1] - buff_offset);
    free (frame6[2] - buff_offset);

    free (frame7[0] - buff_offset);
    free (frame7[1] - buff_offset);
    free (frame7[2] - buff_offset);

    free (outframe[0] - buff_offset);
    free (outframe[1] - buff_offset);
    free (outframe[2] - buff_offset);

	free (scratchplane1 - buff_offset);
	free (scratchplane2 - buff_offset);

    mjpeg_info("Buffers freed.");
  }

  /* did stream end unexpectedly ? */
  if (err != Y4M_ERR_EOF)
    mjpeg_error_exit1 ("%s", y4m_strerr (err));

  /* Exit gently */
  return (0);
}
