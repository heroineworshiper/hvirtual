#include "config.h"
#include "mmxsse_motion.h"
#include "fastintfns.h"

#include <stdlib.h>

int build_sub22_mests_mmxe( me_result_set *sub44set,
				me_result_set *sub22set,
				int i0,  int j0, int ihigh, int jhigh, 
				int null_ctl_sad,
				uint8_t *s22org,  uint8_t *s22blk, 
				int frowstride, int fh,
				int reduction)
{
	int i,k,s;
	int threshold = 6*null_ctl_sad / (2 * 2*reduction);

	int min_weight;
	int ilim = ihigh-i0;
	int jlim = jhigh-j0;
	int x,y;
	uint8_t *s22orgblk;
	int32_t resvec[4];
        me_result_s *mc = sub22set->mests;

	for( k = 0; k < sub44set->len; ++k )
	{

		x = sub44set->mests[k].x;
		y = sub44set->mests[k].y;

		s22orgblk =  s22org +((y+j0)>>1)*frowstride +((x+i0)>>1);
		/*
		  Get SAD for 2*2 subsampled macroblocks: orgblk,orgblk(+2,0),
		  orgblk(0,+2), and orgblk(+2,+2) Done all in one go to reduce
		  memory bandwidth demand
		*/
		mblock_sub22_nearest4_sads_mmxe(s22orgblk, s22blk, frowstride, fh, resvec);
		for( i = 0; i < 4; ++i )
		{
			if( x <= ilim && y <= jlim )
			{	
				s =resvec[i]+(intmax(abs(x), abs(y))<<3);
				if( s < threshold )
				{
					mc->x = (int8_t)x;
					mc->y = (int8_t)y;
					mc->weight = s;
                                        mc++;
				}
			}

			if( i == 1 )
			{
				x -= 2;
				y += 2;
			}
			else
			{
				x += 2;
			}
		}

	}

	sub22set->len = mc - sub22set->mests;
	
	sub_mean_reduction( sub22set, reduction, &min_weight );
	return sub22set->len;
}
