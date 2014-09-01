#include "huffman.h"
#include "mpeg3private.h"
#include "mpeg3protos.h"
#include "tables.h"

#include <stdio.h>
#include <string.h>

struct gr_info_s 
{
      int scfsi;
      unsigned part2_3_length;
      unsigned big_values;
      unsigned scalefac_compress;
      unsigned block_type;
      unsigned mixed_block_flag;
      unsigned table_select[3];
      unsigned subblock_gain[3];
      unsigned maxband[3];
      unsigned maxbandl;
      unsigned maxb;
      unsigned region1start;
      unsigned region2start;
      unsigned preflag;
      unsigned scalefac_scale;
      unsigned count1table_select;
      float *full_gain[3];
      float *pow2gain;
};

struct sideinfo_s
{
	unsigned main_data_begin;
	unsigned private_bits;
	struct 
	{
    	struct gr_info_s gr[2];
	} ch[2];
};









static int get_scale_factors_1(mpeg3_layer_t *audio,
		int *scf, 
		struct gr_info_s *gr_info, 
		int ch, 
		int gr)
{
	static unsigned char slen[2][16] = 
		{{0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
		 {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}};
	int numbits;
	int num0 = slen[0][gr_info->scalefac_compress];
	int num1 = slen[1][gr_info->scalefac_compress];

    if (gr_info->block_type == 2) 
	{
		int i = 18;
		numbits = (num0 + num1) * 18;

		if (gr_info->mixed_block_flag) 
		{
			for(i = 8; i; i--)
				*scf++ = mpeg3bits_getbits(audio->stream, num0);
			i = 9;
/* num0 * 17 + num1 * 18 */
			numbits -= num0; 
		}

		for( ; i; i--)
			*scf++ = mpeg3bits_getbits(audio->stream, num0);
		for(i = 18; i; i--)
			*scf++ = mpeg3bits_getbits(audio->stream, num1);
/* short[13][0..2] = 0 */
		*scf++ = 0; 
		*scf++ = 0; 
		*scf++ = 0; 
    }
    else 
	{
    	int i;
    	int scfsi = gr_info->scfsi;

    	if(scfsi < 0)
		{ 
/* scfsi < 0 => granule == 0 */
			for(i = 11; i; i--)
			{
				*scf++ = mpeg3bits_getbits(audio->stream, num0);
			}
			for(i = 10; i; i--)
				*scf++ = mpeg3bits_getbits(audio->stream, num1);
			numbits = (num0 + num1) * 10 + num0;
			*scf++ = 0;
    	}
    	else 
		{
    		numbits = 0;
    		if(!(scfsi & 0x8)) 
			{
        		for(i = 0; i < 6; i++)
				{
        			*scf++ = mpeg3bits_getbits(audio->stream, num0);
				}
        		numbits += num0 * 6;
    		}
    		else 
			{
        		scf += 6; 
    		}

    		if(!(scfsi & 0x4)) 
			{
        		for(i = 0; i < 5; i++)
        		  *scf++ = mpeg3bits_getbits(audio->stream, num0);
        		numbits += num0 * 5;
    		}
    		else 
			{
				scf += 5;
    		}

    		if(!(scfsi & 0x2)) 
			{
        	    for(i = 0; i < 5; i++)
        			*scf++ = mpeg3bits_getbits(audio->stream, num1);
        	    numbits += num1 * 5;
    		}
    		else 
			{
        	    scf += 5; 
    		}

    		if(!(scfsi & 0x1)) 
			{
        	    for(i = 0; i < 5; i++)
        			*scf++ = mpeg3bits_getbits(audio->stream, num1);
        	    numbits += num1 * 5;
    		}
    		else 
			{
        	    scf += 5;
    		}
    		*scf++ = 0;  /* no l[21] in original sources */
    	}
    }
    return numbits;
}

static int get_scale_factors_2(mpeg3_layer_t *audio,
		int *scf,
		struct gr_info_s *gr_info,
		int i_stereo)
{
	unsigned char *pnt;
	int i, j, n = 0, numbits = 0;
	unsigned int slen;
	static unsigned char stab[3][6][4] = 
	{{{ 6, 5, 5,5 }, { 6, 5, 7,3 }, { 11,10,0,0},
      { 7, 7, 7,0 }, { 6, 6, 6,3 }, {  8, 8,5,0}},
	 {{ 9, 9, 9,9 }, { 9, 9,12,6 }, { 18,18,0,0},
      {12,12,12,0 }, {12, 9, 9,6 }, { 15,12,9,0}},
	 {{ 6, 9, 9,9 }, { 6, 9,12,6 }, { 15,18,0,0},
      { 6,15,12,0 }, { 6,12, 9,6 }, {  6,18,9,0}}}; 

/* i_stereo AND second channel -> do_layer3() checks this */
	if(i_stereo) 
      	slen = mpeg3_i_slen2[gr_info->scalefac_compress >> 1];
	else
      	slen = mpeg3_n_slen2[gr_info->scalefac_compress];

  	gr_info->preflag = (slen >> 15) & 0x1;

	n = 0;  
	if(gr_info->block_type == 2 ) 
	{
    	n++;
    	if(gr_info->mixed_block_flag)
    	  	n++;
	}

	pnt = stab[n][(slen >> 12) & 0x7];

	for(i = 0; i < 4; i++)
	{
    	int num = slen & 0x7;
    	slen >>= 3;
    	if(num) 
		{
    		for(j = 0; j < (int)(pnt[i]); j++)
        	    *scf++ = mpeg3bits_getbits(audio->stream, num);
    		numbits += pnt[i] * num;
    	}
    	else 
		{
    	    for(j = 0; j < (int)(pnt[i]); j++)
        		*scf++ = 0;
    	}
	}
  
	n = (n << 1) + 1;
	for(i = 0; i < n; i++)
		*scf++ = 0;

  	return numbits;
}

static int pretab1[22] = {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0};
static int pretab2[22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*
 * Dequantize samples (includes huffman decoding)
 *
 * 24 is enough because tab13 has max. a 19 bit huffvector
 */

#define BITSHIFT ((sizeof(int32_t) - 1) * 8)
#define REFRESH_MASK \
	while(num < BITSHIFT) \
	{ \
		mask |= mpeg3bits_getbits(audio->stream, 8) << (BITSHIFT - num); \
		num += 8; \
		part2remain -= 8; \
	}


static int dequantize_sample(mpeg3_layer_t *audio,
		float xr[SBLIMIT][SSLIMIT],
		int *scf,
   		struct gr_info_s *gr_info,
		int sfreq,
		int part2bits)
{
	int shift = 1 + gr_info->scalefac_scale;
	float *xrpnt = (float*)xr;
	int l[3],l3;
	int part2remain = gr_info->part2_3_length - part2bits;
	int *me;
	int num = mpeg3bits_getbitoffset(audio->stream);
	int32_t mask = mpeg3bits_getbits(audio->stream, num);
//printf("III_dequantize_sample 1 %08x %d\n", mask, num);
	mask = mask << (BITSHIFT + 8 - num);
	part2remain -= num;

  	{
    	int bv       = gr_info->big_values;
    	int region1  = gr_info->region1start;
    	int region2  = gr_info->region2start;

    	l3 = ((576 >> 1) - bv) >> 1;   

/*
 * we may lose the 'odd' bit here !! 
 * check this later again 
 */

    	if(bv <= region1) 
		{
    	    l[0] = bv; 
			l[1] = 0; 
			l[2] = 0;
    	}
    	else 
		{
    		l[0] = region1;
    		if(bv <= region2) 
			{
        	    l[1] = bv - l[0];  l[2] = 0;
    		}
    		else 
			{
        	    l[1] = region2 - l[0]; 
				l[2] = bv - region2;
    		}
    	}
	}
 
  	if(gr_info->block_type == 2) 
	{
/*
 * decoding with short or mixed mode BandIndex table 
 */
    	int i, max[4];
    	int step = 0, lwin = 3, cb = 0;
    	register float v = 0.0;
    	register int *m, mc;

    	if(gr_info->mixed_block_flag) 
		{
    		max[3] = -1;
    		max[0] = max[1] = max[2] = 2;
    		m = mpeg3_map[sfreq][0];
    		me = mpeg3_mapend[sfreq][0];
    	}
    	else 
		{
    		max[0] = max[1] = max[2] = max[3] = -1;
/* max[3] not floatly needed in this case */
    		m = mpeg3_map[sfreq][1];
    		me = mpeg3_mapend[sfreq][1];
    	}

		mc = 0;
		for(i = 0; i < 2; i++) 
		{
			int lp = l[i];
			struct newhuff *h = mpeg3_ht + gr_info->table_select[i];
			for( ; lp; lp--, mc--) 
			{
    			register int x,y;
    			if(!mc) 
				{
    				mc    = *m++;
    				xrpnt = ((float*)xr) + (*m++);
    				lwin  = *m++;
    				cb    = *m++;
    				if(lwin == 3) 
					{
        				v = gr_info->pow2gain[(*scf++) << shift];
        				step = 1;
    				}
    				else 
					{
        				v = gr_info->full_gain[lwin][(*scf++) << shift];
        				step = 3;
    				}
    			}

        		{
        			register short *val = h->table;
        			REFRESH_MASK;
        			while((y = *val++) < 0) 
					{
            			if (mask < 0)
            				val -= y;
            			num--;
            			mask <<= 1;
        			}
        			x = y >> 4;
        			y &= 0xf;
        		}

        		if(x == 15 && h->linbits) 
				{
        			max[lwin] = cb;
        			REFRESH_MASK;
        			x += ((uint32_t)mask) >> (BITSHIFT + 8 - h->linbits);
        			num -= h->linbits + 1;
        			mask <<= h->linbits;
        			if(mask < 0)
            			*xrpnt = -mpeg3_ispow[x] * v;
        			else
            			*xrpnt =  mpeg3_ispow[x] * v;
        			mask <<= 1;
        		}
        		else 
				if(x) 
				{
        			max[lwin] = cb;
        			if(mask < 0)
            			*xrpnt = -mpeg3_ispow[x] * v;
        			else
            			*xrpnt =  mpeg3_ispow[x] * v;
        			num--;
        			mask <<= 1;
        		}
        		else
        			*xrpnt = 0.0;

        		xrpnt += step;
        		if(y == 15 && h->linbits) 
				{
        			max[lwin] = cb;
        			REFRESH_MASK;
        			y += ((uint32_t) mask) >> (BITSHIFT + 8 - h->linbits);
        			num -= h->linbits + 1;
        			mask <<= h->linbits;
        			if(mask < 0)
            			*xrpnt = -mpeg3_ispow[y] * v;
        			else
            			*xrpnt =  mpeg3_ispow[y] * v;
        			mask <<= 1;
        		}
        		else 
				if(y) 
				{
        			max[lwin] = cb;
        			if(mask < 0)
            			*xrpnt = -mpeg3_ispow[y] * v;
        			else
            			*xrpnt =  mpeg3_ispow[y] * v;
        			num--;
        			mask <<= 1;
        		}
        		else
        			*xrpnt = 0.0;
        		xrpnt += step;
    		}
    	}

    	for( ;l3 && (part2remain + num > 0); l3--) 
		{
    		struct newhuff *h = mpeg3_htc + gr_info->count1table_select;
    		register short *val = h->table, a;

    		REFRESH_MASK;
    		while((a = *val++) < 0) 
			{
        		if (mask < 0)
        			val -= a;
        		num--;
        		mask <<= 1;
    		}
	        if(part2remain + num <= 0) 
			{
				num -= part2remain + num;
				break;
      		}

    		for(i = 0; i < 4; i++) 
			{
        		if(!(i & 1)) 
				{
        			if(!mc) 
					{
            			mc = *m++;
            			xrpnt = ((float*)xr) + (*m++);
            			lwin = *m++;
            			cb = *m++;
            			if(lwin == 3) 
						{
            				v = gr_info->pow2gain[(*scf++) << shift];
            				step = 1;
            			}
            			else 
						{
            				v = gr_info->full_gain[lwin][(*scf++) << shift];
            				step = 3;
            			}
        			}
        			mc--;
        		}
        		if((a & (0x8 >> i))) 
				{
        			max[lwin] = cb;
        			if(part2remain + num <= 0) 
					{
            			break;
        			}
        			if(mask < 0) 
            			*xrpnt = -v;
        			else
            			*xrpnt = v;
        			num--;
        			mask <<= 1;
        		}
        		else
        		  *xrpnt = 0.0;
        		xrpnt += step;
    		}
    	}

    	if(lwin < 3) 
		{ 
/* short band? */
    		while(1) 
			{
        		for( ;mc > 0; mc--) 
				{
/* short band -> step=3 */
        			*xrpnt = 0.0; 
					xrpnt += 3; 
        			*xrpnt = 0.0; 
					xrpnt += 3;
        		}
        		if(m >= me)
        			break;
        		mc    = *m++;
        		xrpnt = ((float*)xr) + *m++;
/* optimize: field will be set to zero at the end of the function */
        		if(*m++ == 0)
        			break; 
/* cb */
        		m++; 
    		}
    	}

    	gr_info->maxband[0] = max[0] + 1;
    	gr_info->maxband[1] = max[1] + 1;
    	gr_info->maxband[2] = max[2] + 1;
    	gr_info->maxbandl = max[3] + 1;

    	{
    		int rmax = max[0] > max[1] ? max[0] : max[1];
    		rmax = (rmax > max[2] ? rmax : max[2]) + 1;
    		gr_info->maxb = rmax ? mpeg3_shortLimit[sfreq][rmax] : mpeg3_longLimit[sfreq][max[3] + 1];
    	}

	}
	else 
	{
/*
 * decoding with 'long' BandIndex table (block_type != 2)
 */
    	int *pretab = gr_info->preflag ? pretab1 : pretab2;
    	int i, max = -1;
    	int cb = 0;
    	int *m = mpeg3_map[sfreq][2];
    	register float v = 0.0;
    	int mc = 0;

/*
 * long hash table values
 */
    	for(i = 0; i < 3; i++) 
		{
    		int lp = l[i];
    		struct newhuff *h = mpeg3_ht + gr_info->table_select[i];

    		for(; lp; lp--, mc--) 
			{
        		int x, y;

				if(!mc) 
				{
					mc = *m++;
					cb = *m++;
					if(cb == 21)
    				  	v = 0.0;
					else
    				  	v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
				}
    			{
    				register short *val = h->table;
    				REFRESH_MASK;
    				while((y = *val++) < 0) 
					{
        				if(mask < 0)
        					val -= y;
        				num--;
        				mask <<= 1;
    				}
    				x = y >> 4;
    				y &= 0xf;
    			}

        		if(x == 15 && h->linbits) 
				{
        			max = cb;
					REFRESH_MASK;
        			x += ((uint32_t) mask) >> (BITSHIFT + 8 - h->linbits);
        			num -= h->linbits + 1;
        			mask <<= h->linbits;
        			if(mask < 0)
            			*xrpnt++ = -mpeg3_ispow[x] * v;
        			else
            			*xrpnt++ =  mpeg3_ispow[x] * v;
        			mask <<= 1;
        		}
        		else 
				if(x) 
				{
        			max = cb;
        			if(mask < 0)
            			*xrpnt++ = -mpeg3_ispow[x] * v;
        			else
            			*xrpnt++ =  mpeg3_ispow[x] * v;
        			num--;
        			mask <<= 1;
        		}
        		else
        			*xrpnt++ = 0.0;

       			if(y == 15 && h->linbits) 
				{
        			max = cb;
					REFRESH_MASK;
        			y += ((uint32_t) mask) >> (BITSHIFT + 8 - h->linbits);
        			num -= h->linbits + 1;
        			mask <<= h->linbits;
        			if(mask < 0)
            			*xrpnt++ = -mpeg3_ispow[y] * v;
        			else
            			*xrpnt++ =  mpeg3_ispow[y] * v;
        			mask <<= 1;
        		}
        		else 
				if(y) 
				{
        		  max = cb;
        		  if(mask < 0)
            		  *xrpnt++ = -mpeg3_ispow[y] * v;
        		  else
            		  *xrpnt++ =  mpeg3_ispow[y] * v;
        		  num--;
        		  mask <<= 1;
        		}
        		else
        			*xrpnt++ = 0.0;
    		}
    	}

/*
 * short (count1table) values
 */
    	for( ; l3 && (part2remain + num > 0); l3--) 
		{
    		struct newhuff *h = mpeg3_htc + gr_info->count1table_select;
    		register short *val = h->table, a;

    		REFRESH_MASK;
    		while((a = *val++) < 0) 
			{
        		if(mask < 0)
        			val -= a;
        		num--;
        		mask <<= 1;
    		}
    		if(part2remain + num <= 0) 
			{
				num -= part2remain + num;
        		break;
    		}

    		for(i = 0; i < 4; i++) 
			{
        		if(!(i & 1)) 
				{
        			if(!mc) 
					{
            			mc = *m++;
            			cb = *m++;
            			if(cb == 21)
            		    	v = 0.0;
            			else
            				v = gr_info->pow2gain[((*scf++) + (*pretab++)) << shift];
        			}
        			mc--;
        		}
        		if((a & (0x8 >> i)))
				{
        			max = cb;
        			if(part2remain + num <= 0) 
					{
            			break;
        			}
        			if(mask < 0)
            			*xrpnt++ = -v;
        			else
            			*xrpnt++ = v;
        			num--;
        			mask <<= 1;
        		}
        		else
        			*xrpnt++ = 0.0;
    		}
    	}

    	gr_info->maxbandl = max + 1;
    	gr_info->maxb = mpeg3_longLimit[sfreq][gr_info->maxbandl];
	}

	part2remain += num;

//
	mpeg3bits_start_reverse(audio->stream);
	mpeg3bits_getbits_reverse(audio->stream, num);
	mpeg3bits_start_forward(audio->stream);
//printf("III_dequantize_sample 3 %d %04x\n", audio->stream->bit_number, mpeg3bits_showbits(audio->stream, 16));
	num = 0;

	while(xrpnt < &xr[SBLIMIT][0]) 
      	*xrpnt++ = 0.0;

	while(part2remain > 16)
	{
    	mpeg3bits_getbits(audio->stream, 16); /* Dismiss stuffing Bits */
    	part2remain -= 16;
	}

	if(part2remain > 0)
	{
      	mpeg3bits_getbits(audio->stream, part2remain);
	}
	else 
	if(part2remain < 0) 
	{
      	printf("dequantize_sample: can't rewind stream %d bits! data=%02x%02x%02x%02x\n", 
		-part2remain,
		(unsigned char)audio->stream->input_ptr[-3], 
		(unsigned char)audio->stream->input_ptr[-2], 
		(unsigned char)audio->stream->input_ptr[-1], 
		(unsigned char)audio->stream->input_ptr[0]);
      	return 1; /* -> error */
	}
	return 0;
}

static int get_side_info(mpeg3_layer_t *audio,
		struct sideinfo_s *si,
		int channels,
 		int ms_stereo,
		long sfreq,
		int single,
		int lsf)
{
	int ch, gr;
	int powdiff = (single == 3) ? 4 : 0;
	static const int tabs[2][5] = { { 2,9,5,3,4 } , { 1,8,1,2,9 } };
	const int *tab = tabs[lsf];

	si->main_data_begin = mpeg3bits_getbits(audio->stream, tab[1]);
	if(channels == 1)
		si->private_bits = mpeg3bits_getbits(audio->stream, tab[2]);
	else 
    	si->private_bits = mpeg3bits_getbits(audio->stream, tab[3]);
	if(!lsf)
	{
		for(ch = 0; ch < channels; ch++)
		{
    		si->ch[ch].gr[0].scfsi = -1;
    		si->ch[ch].gr[1].scfsi = mpeg3bits_getbits(audio->stream, 4);
		}
	}

	for(gr = 0; gr < tab[0]; gr++) 
	{
		for(ch = 0; ch < channels; ch++)
		{
			register struct gr_info_s *gr_info = &(si->ch[ch].gr[gr]);

			gr_info->part2_3_length = mpeg3bits_getbits(audio->stream, 12);
			gr_info->big_values = mpeg3bits_getbits(audio->stream, 9);
			if(gr_info->big_values > 288) 
			{
				fprintf(stderr,"get_side_info: big_values too large!\n");
				gr_info->big_values = 288;
			}
			gr_info->pow2gain = mpeg3_gainpow2 + 256 - mpeg3bits_getbits(audio->stream, 8) + powdiff;
			if(ms_stereo)
				gr_info->pow2gain += 2;
			gr_info->scalefac_compress = mpeg3bits_getbits(audio->stream, tab[4]);

			if(mpeg3bits_getbits(audio->stream, 1)) 
			{
/* window switch flag  */
				int i;
				gr_info->block_type       = mpeg3bits_getbits(audio->stream, 2);
				gr_info->mixed_block_flag = mpeg3bits_getbits(audio->stream, 1);
				gr_info->table_select[0]  = mpeg3bits_getbits(audio->stream, 5);
				gr_info->table_select[1]  = mpeg3bits_getbits(audio->stream, 5);
/*
 * table_select[2] not needed, because there is no region2,
 * but to satisfy some verifications tools we set it either.
 */
        		gr_info->table_select[2] = 0;
        		for(i = 0; i < 3; i++)
        	    	gr_info->full_gain[i] = gr_info->pow2gain + (mpeg3bits_getbits(audio->stream, 3) << 3);

        		if(gr_info->block_type == 0) 
				{
        			fprintf(stderr,"Blocktype == 0 and window-switching == 1 not allowed.\n");
        			return 1;
        		}

/* region_count/start parameters are implicit in this case. */       
				if(!lsf || gr_info->block_type == 2)
        	   		gr_info->region1start = 36 >> 1;
				else 
				{
/* check this again for 2.5 and sfreq=8 */
        			if(sfreq == 8)
						gr_info->region1start = 108 >> 1;
        			else
						gr_info->region1start = 54 >> 1;
        		}
        		gr_info->region2start = 576 >> 1;
			}
			else 
			{
				int i, r0c, r1c;
				for(i = 0; i < 3; i++)
					gr_info->table_select[i] = mpeg3bits_getbits(audio->stream, 5);

				r0c = mpeg3bits_getbits(audio->stream, 4);
				r1c = mpeg3bits_getbits(audio->stream, 3);
				gr_info->region1start = mpeg3_bandInfo[sfreq].longIdx[r0c + 1] >> 1 ;
				gr_info->region2start = mpeg3_bandInfo[sfreq].longIdx[r0c + 1 + r1c + 1] >> 1;
				gr_info->block_type = 0;
				gr_info->mixed_block_flag = 0;
			}
			if(!lsf) gr_info->preflag = mpeg3bits_getbits(audio->stream, 1);
			gr_info->scalefac_scale = mpeg3bits_getbits(audio->stream, 1);
			gr_info->count1table_select = mpeg3bits_getbits(audio->stream, 1);
		}
	}
	return 0;
}

static int hybrid(mpeg3_layer_t *audio,
		float fsIn[SBLIMIT][SSLIMIT],
		float tsOut[SSLIMIT][SBLIMIT],
	   int ch,
	   struct gr_info_s *gr_info)
{
	float *tspnt = (float *) tsOut;
	float *rawout1,*rawout2;
	int bt, sb = 0;

	
	{
    	int b = audio->mp3_blc[ch];
    	rawout1 = audio->mp3_block[b][ch];
    	b = -b + 1;
    	rawout2 = audio->mp3_block[b][ch];
    	audio->mp3_blc[ch] = b;
	}
  
	if(gr_info->mixed_block_flag) 
	{
    	sb = 2;
    	mpeg3audio_dct36(fsIn[0], rawout1, rawout2, mpeg3_win[0], tspnt);
    	mpeg3audio_dct36(fsIn[1], rawout1 + 18, rawout2 + 18, mpeg3_win1[0], tspnt + 1);
    	rawout1 += 36; 
		rawout2 += 36; 
		tspnt += 2;
	}

	bt = gr_info->block_type;
	if(bt == 2) 
	{
    	for( ; sb < gr_info->maxb; sb += 2, tspnt += 2, rawout1 += 36, rawout2 += 36) 
		{
    		mpeg3audio_dct12(fsIn[sb]  ,rawout1   ,rawout2   ,mpeg3_win[2] ,tspnt);
    		mpeg3audio_dct12(fsIn[sb + 1], rawout1 + 18, rawout2 + 18, mpeg3_win1[2], tspnt + 1);
    	}
	}
	else 
	{
    	for( ; sb < gr_info->maxb; sb += 2, tspnt += 2, rawout1 += 36, rawout2 += 36) 
		{
    		mpeg3audio_dct36(fsIn[sb], rawout1, rawout2, mpeg3_win[bt], tspnt);
    		mpeg3audio_dct36(fsIn[sb + 1], rawout1 + 18, rawout2 + 18, mpeg3_win1[bt], tspnt + 1);
    	}
	}

	for( ; sb < SBLIMIT; sb++, tspnt++) 
	{
    	int i;
    	for(i = 0; i < SSLIMIT; i++) 
		{
    		tspnt[i * SBLIMIT] = *rawout1++;
    		*rawout2++ = 0.0;
    	}
	}
	return 0;
}

static int antialias(mpeg3_layer_t *audio,
		float xr[SBLIMIT][SSLIMIT],
		struct gr_info_s *gr_info)
{
	int sblim;

	if(gr_info->block_type == 2) 
	{
    	if(!gr_info->mixed_block_flag) 
        	return 0;
    	sblim = 1; 
	}
	else 
	{
        sblim = gr_info->maxb-1;
	}

/* 31 alias-reduction operations between each pair of sub-bands */
/* with 8 butterflies between each pair                         */

	{
    	int sb;
    	float *xr1 = (float*)xr[1];

    	for(sb = sblim; sb; sb--, xr1 += 10) 
		{
    		int ss;
    		float *cs, *ca;
    		float *xr2;
    		cs = mpeg3_aa_cs;
			ca = mpeg3_aa_ca;
    		xr2 = xr1;

    		for(ss = 7; ss >= 0; ss--)
    		{
/* upper and lower butterfly inputs */
        		register float bu, bd;
        		bu = *--xr2;
				bd = *xr1;
        		*xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
        		*xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );
    		}
    	}
    }
	return 0;
}

/* 
 * III_stereo: calculate float channel values for Joint-I-Stereo-mode
 */
static int calc_i_stereo(mpeg3_layer_t *audio, 
		float xr_buf[2][SBLIMIT][SSLIMIT],
		int *scalefac,
   		struct gr_info_s *gr_info,
		int sfreq,
		int ms_stereo,
		int lsf)
{
	float (*xr)[SBLIMIT*SSLIMIT] = (float (*)[SBLIMIT*SSLIMIT] ) xr_buf;
	struct mpeg3_bandInfoStruct *bi = &mpeg3_bandInfo[sfreq];
	const float *tab1, *tab2;

    int tab;
/* TODO: optimize as static */
    static const float *tabs[3][2][2] = 
	{ 
       { { mpeg3_tan1_1, mpeg3_tan2_1 }     , { mpeg3_tan1_2, mpeg3_tan2_2 } },
       { { mpeg3_pow1_1[0], mpeg3_pow2_1[0] } , { mpeg3_pow1_2[0], mpeg3_pow2_2[0] } } ,
       { { mpeg3_pow1_1[1], mpeg3_pow2_1[1] } , { mpeg3_pow1_2[1], mpeg3_pow2_2[1] } } 
    };

    tab = lsf + (gr_info->scalefac_compress & lsf);
    tab1 = tabs[tab][ms_stereo][0];
    tab2 = tabs[tab][ms_stereo][1];

    if(gr_info->block_type == 2) 
	{
    	int lwin,do_l = 0;
    	if(gr_info->mixed_block_flag)
        	do_l = 1;

    	for(lwin = 0; lwin < 3; lwin++) 
		{ 
/* process each window */
/* get first band with zero values */
/* sfb is minimal 3 for mixed mode */
        	int is_p, sb, idx, sfb = gr_info->maxband[lwin];  
        	if(sfb > 3) do_l = 0;

        	for( ; sfb < 12 ; sfb++) 
			{
/* scale: 0-15 */ 
        		is_p = scalefac[sfb * 3 + lwin - gr_info->mixed_block_flag]; 
        		if(is_p != 7) 
				{
            		float t1, t2;
            		sb  = bi->shortDiff[sfb];
            		idx = bi->shortIdx[sfb] + lwin;
            		t1  = tab1[is_p]; 
					t2 = tab2[is_p];
            		for( ; sb > 0; sb--, idx += 3) 
					{
            			float v = xr[0][idx];
            			xr[0][idx] = v * t1;
            			xr[1][idx] = v * t2;
            		}
        		}
			}

/* in the original: copy 10 to 11 , here: copy 11 to 12 maybe still wrong??? (copy 12 to 13?) */
/* scale: 0-15 */
        	is_p = scalefac[11 * 3 + lwin - gr_info->mixed_block_flag]; 
        	sb   = bi->shortDiff[12];
        	idx  = bi->shortIdx[12] + lwin;
        	if(is_p != 7) 
			{
        		float t1, t2;
        		t1 = tab1[is_p]; 
				t2 = tab2[is_p];
        		for( ; sb > 0; sb--, idx += 3) 
				{  
            		float v = xr[0][idx];
            		xr[0][idx] = v * t1;
            		xr[1][idx] = v * t2;
        		}
        	}
    	} /* end for(lwin; .. ; . ) */

/* also check l-part, if ALL bands in the three windows are 'empty'
* and mode = mixed_mode 
*/
		if(do_l) 
		{
			int sfb = gr_info->maxbandl;
			int idx = bi->longIdx[sfb];

			for ( ; sfb < 8; sfb++) 
			{
				int sb = bi->longDiff[sfb];
/* scale: 0-15 */
				int is_p = scalefac[sfb]; 
				if(is_p != 7) 
				{
					float t1, t2;
					t1 = tab1[is_p]; 
					t2 = tab2[is_p];
					for( ; sb > 0; sb--, idx++) 
					{
						float v = xr[0][idx];
						xr[0][idx] = v * t1;
						xr[1][idx] = v * t2;
					}
				}
				else 
				   idx += sb;
			}
    	}     
	} 
    else 
	{ 
/* ((gr_info->block_type != 2)) */
		int sfb = gr_info->maxbandl;
		int is_p, idx = bi->longIdx[sfb];
		for( ; sfb < 21; sfb++) 
		{
			int sb = bi->longDiff[sfb];
/* scale: 0-15 */
			is_p = scalefac[sfb]; 
			if(is_p != 7) 
			{
        		float t1, t2;
        		t1 = tab1[is_p]; 
				t2 = tab2[is_p];
        		for( ; sb > 0; sb--, idx++) 
				{
					 float v = xr[0][idx];
            		 xr[0][idx] = v * t1;
            		 xr[1][idx] = v * t2;
        		}
			}
			else
				idx += sb;
      	}

    	is_p = scalefac[20];
    	if(is_p != 7) 
		{  
/* copy l-band 20 to l-band 21 */
        	int sb;
        	float t1 = tab1[is_p], t2 = tab2[is_p]; 

        	for(sb = bi->longDiff[21]; sb > 0; sb--, idx++)
			{
        		float v = xr[0][idx];
        		xr[0][idx] = v * t1;
        		xr[1][idx] = v * t2;
        	}
    	}
    } /* ... */
	return 0;
}

int mpeg3audio_dolayer3(mpeg3_layer_t *audio, 
	char *frame, 
	int frame_size, 
	float **output,
	int render)
{
	int gr, ch, ss;
/* max 39 for short[13][3] mode, mixed: 38, long: 22 */
	int scalefacs[2][39]; 
	struct sideinfo_s sideinfo;
	int single = audio->single;
	int ms_stereo, i_stereo;
	int sfreq = audio->sampling_frequency_code;
	int stereo1, granules;
	int i;
	int output_offset = 0;

//printf("mpeg3audio_dolayer3 1\n");
// Skip header
	frame += 4;
	frame_size -= 4;

/* flip/init buffer */
	audio->bsbufold = audio->bsbuf;
	audio->bsbuf = audio->bsspace[audio->bsnum] + 512;
	audio->bsnum ^= 1;

/* Copy frame into history buffer */
	memcpy(audio->bsbuf, frame, frame_size);

/*
 * printf(__FUNCTION__ " %d %02x%02x%02x%02x\n", 
 * audio->first_frame, 
 * (unsigned char)audio->bsbuf[0],
 * (unsigned char)audio->bsbuf[1],
 * (unsigned char)audio->bsbuf[2],
 * (unsigned char)audio->bsbuf[3]);
 */


	if(!audio->first_frame)
	{
/* Set up bitstream to use buffer */
		mpeg3bits_use_ptr(audio->stream, audio->bsbuf);

//printf(__FUNCTION__ " 4\n");
//printf(__FUNCTION__ " 7 %x\n", mpeg3bits_showbits(audio->stream, 16));
/* CRC must be skipped here for proper alignment with the backstep */
 		if(audio->error_protection)
			mpeg3bits_getbits(audio->stream, 16);
//printf(__FUNCTION__ " 8 %x\n", mpeg3bits_showbits(audio->stream, 16));
//printf(__FUNCTION__ " 5\n");

		if(audio->channels == 1)
		{
/* stream is mono */
    		stereo1 = 1;
    		single = 0;
		}
 		else
		{
/* Stereo */
    		stereo1 = 2;
		}

		if(audio->mode == MPG_MD_JOINT_STEREO)
		{
    		ms_stereo = (audio->mode_ext & 0x2) >> 1;
    		i_stereo  = audio->mode_ext & 0x1;
		}
		else
    		ms_stereo = i_stereo = 0;

  		if(audio->lsf)
		{
    		granules = 1;
  		}
  		else 
		{
    		granules = 2;
  		}
//printf(__FUNCTION__ " 6\n");

  		if(get_side_info(audio, 
			&sideinfo, 
			audio->channels, 
			ms_stereo, 
			sfreq, 
			single, 
			audio->lsf))
		{
			mpeg3_layer_reset(audio);
			return output_offset;
		}

//printf(__FUNCTION__ " 7\n");
/* Step back */
		if(sideinfo.main_data_begin >= 512)
		{
			return output_offset;
		}

		if(sideinfo.main_data_begin)
		{
/*
 * printf(__FUNCTION__ " 7 %d %d %d\n", 
 * audio->ssize, 
 * sideinfo.main_data_begin,
 * audio->prev_framesize);
 */
			memcpy(audio->bsbuf + audio->ssize - sideinfo.main_data_begin, 
				audio->bsbufold + audio->prev_framesize - sideinfo.main_data_begin, 
				sideinfo.main_data_begin);
			mpeg3bits_use_ptr(audio->stream, 
				audio->bsbuf + audio->ssize - sideinfo.main_data_begin);
		}


  		for(gr = 0; gr < granules; gr++)
		{
    		float hybridIn [2][SBLIMIT][SSLIMIT];
    		float hybridOut[2][SSLIMIT][SBLIMIT];

    		{
				struct gr_info_s *gr_info = &(sideinfo.ch[0].gr[gr]);
				int32_t part2bits;
				if(audio->lsf)
					part2bits = get_scale_factors_2(audio, scalefacs[0], gr_info, 0);
				else
					part2bits = get_scale_factors_1(audio, scalefacs[0], gr_info, 0, gr);
//printf("dolayer3 4 %04x\n", mpeg3bits_showbits(audio->stream, 16));

				if(dequantize_sample(audio, 
					hybridIn[0], 
					scalefacs[0], 
					gr_info, 
					sfreq, 
					part2bits))
				{
					mpeg3_layer_reset(audio);
					return output_offset;
				}
//printf("dolayer3 5 %04x\n", mpeg3bits_showbits(audio->stream, 16));
    		}

      		if(audio->channels == 2) 
			{
    			struct gr_info_s *gr_info = &(sideinfo.ch[1].gr[gr]);
    			int32_t part2bits;
    			if(audio->lsf) 
        			part2bits = get_scale_factors_2(audio, scalefacs[1], gr_info, i_stereo);
    			else
        			part2bits = get_scale_factors_1(audio, scalefacs[1], gr_info, 1, gr);

    			if(dequantize_sample(audio, 
					hybridIn[1], 
					scalefacs[1], 
					gr_info, 
					sfreq, 
					part2bits))
				{
					mpeg3_layer_reset(audio);
        			return output_offset;
				}

    			if(ms_stereo)
				{
        			int i;
        			int maxb = sideinfo.ch[0].gr[gr].maxb;
        			if(sideinfo.ch[1].gr[gr].maxb > maxb)
            			maxb = sideinfo.ch[1].gr[gr].maxb;
        			for(i = 0; i < SSLIMIT * maxb; i++)
					{
        				float tmp0 = ((float*)hybridIn[0])[i];
        				float tmp1 = ((float*)hybridIn[1])[i];
        				((float*)hybridIn[0])[i] = tmp0 + tmp1;
        				((float*)hybridIn[1])[i] = tmp0 - tmp1;
        			}
    	  		}

    			if(i_stereo)
        			calc_i_stereo(audio, hybridIn, scalefacs[1], gr_info, sfreq, ms_stereo, audio->lsf);

    			if(ms_stereo || i_stereo || (single == 3)) 
				{
        			if(gr_info->maxb > sideinfo.ch[0].gr[gr].maxb) 
        				sideinfo.ch[0].gr[gr].maxb = gr_info->maxb;
        			else
        				gr_info->maxb = sideinfo.ch[0].gr[gr].maxb;
    			}

    			switch(single) 
				{
        			case 3:
        				{
            				register int i;
            				register float *in0 = (float*)hybridIn[0], *in1 = (float*)hybridIn[1];
/* *0.5 done by pow-scale */
            				for(i = 0; i < SSLIMIT * gr_info->maxb; i++, in0++)
            					*in0 = (*in0 + *in1++); 
        				}
        				break;
        			case 1:
        				{
            				register int i;
            				register float *in0 = (float*)hybridIn[0], *in1 = (float*)hybridIn[1];
            				for(i = 0; i < SSLIMIT * gr_info->maxb; i++)
            					*in0++ = *in1++;
        				}
        				break;
    			}
			}
//printf(__FUNCTION__ " 9\n");

    		for(ch = 0; ch < stereo1; ch++)
			{
    			struct gr_info_s *gr_info = &(sideinfo.ch[ch].gr[gr]);
//printf(__FUNCTION__ " 9.1\n");
    			antialias(audio, hybridIn[ch], gr_info);
//printf(__FUNCTION__ " 9.2\n");
    			hybrid(audio, hybridIn[ch], hybridOut[ch], ch, gr_info);
//printf(__FUNCTION__ " 9.3\n");
    		}

//printf(__FUNCTION__ " 10\n");


    		for(ss = 0; ss < SSLIMIT; ss++)
			{
    			if(single >= 0)
				{
					if(render) 
						mpeg3audio_synth_stereo(audio, 
							hybridOut[0][ss], 
							0,
							output[0], 
							&(output_offset));
					else
						output_offset += 32;
    			}
    			else 
				{
        			int p1 = output_offset;
        			if(render)
					{
						mpeg3audio_synth_stereo(audio, 
							hybridOut[0][ss], 
							0, 
							output[0], 
							&p1);
        				mpeg3audio_synth_stereo(audio, 
							hybridOut[1][ss], 
							1, 
							output[1], 
							&(output_offset));
					}
					else
						output_offset += 32;
    			}
    		}
		}
	}
	else
	{
		audio->first_frame = 0;
	}

//printf(__FUNCTION__ " 12\n");


	return output_offset;
}







void mpeg3_layer_reset(mpeg3_layer_t *audio)
{
//printf("mpeg3_layer_reset 1\n");
	audio->first_frame = 1;
//	audio->prev_framesize = 0;
//	bzero(audio->bsspace, sizeof(audio->bsspace));
	bzero(audio->mp3_block, sizeof(audio->mp3_block));
	bzero(audio->mp3_blc, sizeof(audio->mp3_blc));
	mpeg3audio_reset_synths(audio);
}






/* Return 1 if the head check doesn't find a header. */
int mpeg3_layer_check(unsigned char *data)
{
	uint32_t head = ((uint32_t)(data[0] << 24)) | 
		((uint32_t)(data[1] << 16)) | 
		((uint32_t)(data[2] << 8)) | 
		((uint32_t)data[3]);


    if((head & 0xffe00000) != 0xffe00000) return 1;

    if(!((head >> 17) & 3)) return 1;

    if(((head >> 12) & 0xf) == 0xf) return 1;

	if(!((head >> 12) & 0xf)) return 1;

    if(((head >> 10) & 0x3) == 0x3 ) return 1;

	if(((head >> 19) & 1) == 1 && ((head >> 17) & 3) == 3 && ((head >> 16) & 1) == 1)
		return 1;

    if((head & 0xffff0000) == 0xfffe0000) return 1;
// JPEG header
	if((head & 0xffff0000) == 0xffed0000) return 1;

    return 0;
}


/* Decode layer header */
int mpeg3_layer_header(mpeg3_layer_t *layer_data, unsigned char *data)
{
	uint32_t header;
	int sampling_frequency_code;
	int layer;
	int lsf;
	int mpeg35;
	int channels;
	int mode;


// ID3 tag
	switch(layer_data->id3_state)
	{
		case MPEG3_ID3_IDLE:
			if(data[0] == 0x49 &&
				data[1] == 0x44 &&
				data[2] == 0x33)
			{
// Read header
				layer_data->id3_state = MPEG3_ID3_HEADER;
				layer_data->id3_current_byte = 0;
				return 0;
			}
			break;

		case MPEG3_ID3_HEADER:
			layer_data->id3_current_byte++;
			if(layer_data->id3_current_byte >= 6)
			{
				layer_data->id3_size = (data[0] << 21) |
					(data[1] << 14) |
					(data[2] << 7) |
					(data[3]);
				layer_data->id3_current_byte = 0;
				layer_data->id3_state = MPEG3_ID3_SKIP;

/*
 * printf("mpeg3_layer_header %d %02x%02x%02x%02x size=0x%x layer_data->layer=%d\n", 
 * __LINE__,
 * data[0],
 * data[1],
 * data[2],
 * data[3],
 * layer_data->id3_size,
 * layer_data->layer);
 */

			}
			return 0;
			break;

		case MPEG3_ID3_SKIP:


/*
 * printf("mpeg3_layer_header %d layer_data->id3_current_byte=0x%x %02x%02x%02x%02x\n", 
 * __LINE__,
 * layer_data->id3_current_byte,
 * data[0],
 * data[1],
 * data[2],
 * data[3]);
 */


			layer_data->id3_current_byte++;
			if(layer_data->id3_current_byte >= layer_data->id3_size)
				layer_data->id3_state = MPEG3_ID3_IDLE;
			return 0;
			break;
	}

	if(mpeg3_layer_check(data))
	{
		return 0;
	}



// printf("mpeg3_layer_header %d layer_data->id3_state=%d %02x%02x%02x%02x\n",
// __LINE__,
// layer_data->id3_state,
// data[0],
// data[1],
// data[2],
// data[3]);


	header = (data[0] << 24) | 
		(data[1] << 16) | 
		(data[2] << 8) | 
		data[3];
    if(header & (1 << 20)) 
	{
        lsf = (header & (1 << 19)) ? 0x0 : 0x1;
        mpeg35 = 0;
    }
    else 
	{
    	lsf = 1;
    	mpeg35 = 1;
    }

    layer = 4 - ((header >> 17) & 3);

//printf("mpeg3_layer_header 1 %d header=%08x layer=%d layer_data->layer=%d\n", __LINE__, header, layer, layer_data->layer);
	if(layer_data->layer != 0 &&
		layer != layer_data->layer)
	{
		return 0;
	}

    if(mpeg35) 
        sampling_frequency_code = 6 + ((header >> 10) & 0x3);
    else
        sampling_frequency_code = ((header >> 10) & 0x3) + (lsf * 3);


	if(layer_data->samplerate != 0 &&
		sampling_frequency_code != layer_data->sampling_frequency_code)
	{
		return 0;
	}

    mode = ((header >> 6) & 0x3);
	channels = (mode == MPG_MD_MONO) ? 1 : 2;
/*
 *     if(layer_data->channels < 0) 
 * 	else
 * 	if(layer_data->channels != channels)
 * 		return 0;
 */


//	if(channels > layer_data->channels)
//		layer_data->channels = channels;
	layer_data->channels = channels;
	layer_data->layer = layer;
	layer_data->lsf = lsf;
	layer_data->mpeg35 = mpeg35;
	layer_data->mode = mode;
	layer_data->sampling_frequency_code = sampling_frequency_code;
	layer_data->samplerate = mpeg3_freqs[layer_data->sampling_frequency_code];
    layer_data->error_protection = ((header >> 16) & 0x1) ^ 0x1;

    layer_data->bitrate_index = ((header >> 12) & 0xf);
    layer_data->padding   = ((header >> 9) & 0x1);
    layer_data->extension = ((header >> 8) & 0x1);
    layer_data->mode_ext  = ((header >> 4) & 0x3);
    layer_data->copyright = ((header >> 3) & 0x1);
    layer_data->original  = ((header >> 2) & 0x1);
    layer_data->emphasis  = header & 0x3;
	if(layer_data->channels > 1) 
		layer_data->single = -1;
	else
		layer_data->single = 3;

    if(!layer_data->bitrate_index) return 0;
	layer_data->bitrate = 1000 * mpeg3_tabsel_123[layer_data->lsf][layer_data->layer - 1][layer_data->bitrate_index];

	layer_data->prev_framesize = layer_data->framesize - 4;
    switch(layer_data->layer) 
	{
      	case 1:
        	layer_data->framesize  = (long)mpeg3_tabsel_123[layer_data->lsf][0][layer_data->bitrate_index] * 12000;
        	layer_data->framesize /= mpeg3_freqs[layer_data->sampling_frequency_code];
        	layer_data->framesize  = ((layer_data->framesize + layer_data->padding) << 2);
        	break;
      	case 2:
        	layer_data->framesize = (long)mpeg3_tabsel_123[layer_data->lsf][1][layer_data->bitrate_index] * 144000;
        	layer_data->framesize /= mpeg3_freqs[layer_data->sampling_frequency_code];
        	layer_data->framesize += layer_data->padding;
        	break;
      	case 3:
        	if(layer_data->lsf)
        	  	layer_data->ssize = (layer_data->channels == 1) ? 9 : 17;
        	else
        	  	layer_data->ssize = (layer_data->channels == 1) ? 17 : 32;
        	if(layer_data->error_protection)
        	  	layer_data->ssize += 2;
        	layer_data->framesize  = (long)mpeg3_tabsel_123[layer_data->lsf][2][layer_data->bitrate_index] * 144000;
        	layer_data->framesize /= mpeg3_freqs[layer_data->sampling_frequency_code] << (layer_data->lsf);
        	layer_data->framesize = layer_data->framesize + layer_data->padding;
        	break; 
      	default:
        	return 0;
    }




/*
 * printf("mpeg3_layer_header %d bitrate=%d framesize=%d samplerate=%d channels=%d layer=%d\n", 
 * __LINE__,
 * layer_data->bitrate, 
 * layer_data->framesize, 
 * layer_data->samplerate, 
 * layer_data->channels,
 * layer_data->layer);
 */



	if(layer_data->bitrate < 64000 && layer_data->layer != 3) return 0;
	if(layer_data->framesize > MAXFRAMESIZE) return 0;
//printf("mpeg3_layer_header 10 %d\n", layer);

	return layer_data->framesize;
}








mpeg3_layer_t* mpeg3_new_layer()
{
	mpeg3_layer_t *result = calloc(1, sizeof(mpeg3_layer_t));
	result->bsbuf = result->bsspace[1];
	result->bo = 1;
	result->channels = -1;
	result->stream = mpeg3bits_new_stream(0, 0);
	mpeg3_new_decode_tables(result);
	return result;
}



void mpeg3_delete_layer(mpeg3_layer_t *audio)
{
	mpeg3bits_delete_stream(audio->stream);
	free(audio);
}



