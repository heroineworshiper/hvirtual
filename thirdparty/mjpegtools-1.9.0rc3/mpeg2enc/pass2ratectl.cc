/* pass2ratectl.c, bitrate control class for 2nd pass of look-ahead/multi-pass
    encoder.
  */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

/* Modifications and enhancements (C) 2006 Andrew Stevens */

/* These modifications are free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#include "config.h"
#include <math.h>
#include <limits.h>
#include "mjpeg_types.h"
#include "mjpeg_logging.h"
#include "mpeg2syntaxcodes.h"
#include "tables.h"
#include "mpeg2encoder.hh"
#include "picture.hh"
#include "pass2ratectl.hh"
#include "cpu_accel.h"



/*****************************
 *
 * Pass2 rate controller.  Currently a simple virtual buffer
 * based controller that merely exploits look-ahead knowledge
 * of the complexity of upcoming frames.
 * TODO: evolve into proper 2nd pass rate-control using long-term
 * sequence statistics.
 *
 ****************************/
XhiPass2RC::XhiPass2RC(EncoderParams &encparams ) :
	Pass2RateCtl(encparams, *this)
{
	buffer_variation = 0;
	bits_transported = 0;
	bits_used = 0;
	sum_avg_act = 0.0;
	sum_avg_var = 0.0;
	
	/* TODO: These values should are really MPEG-1/2 and material type
	   dependent.  The encoder should probably run over the first 100
	   frames or so look-ahead to tune theses dynamically before doing
	   real encoding... alternative a config file should  be written!
	*/

	sum_avg_quant = 0.0;

}

/*********************
 *
 * Initialise rate control parameters
 * params:  reinit - Rate control is being re-initialised during the middle
 *                   of a run.  Don't reset adaptive parameters.
 *
 ********************/

void XhiPass2RC::InitSeq(bool reinit)
{
	double init_quant;
	/* If its stills with a size we have to hit then make the
	   guesstimates of for initial quantisation pessimistic...
	*/
	bits_transported = bits_used = 0;
	field_rate = 2*encparams.decode_frame_rate;
	fields_per_pict = encparams.fieldpic ? 1 : 2;
	if( encparams.still_size > 0 )
	{
		per_pict_bits = encparams.still_size * 8;
	}
	else
	{
		per_pict_bits = 
			static_cast<int32_t>(encparams.fieldpic
								 ? encparams.bit_rate / field_rate
								 : encparams.bit_rate / encparams.decode_frame_rate
				);
	}

	/* Everything else already set or adaptive */
	if( reinit )
		return;



	/* Calculate reasonable margins for variation in the decoder
	   buffer.  We assume that having less than 5 frame intervals
	   worth buffered is cutting it fine for avoiding under-runs.

	   The gain values represent the fraction of the under/over shoot
	   to be recovered during one second.  Gain is decreased if the
	   buffer margin is large, gain is higher for avoiding overshoot.

	   Currently, for a 1-frame sized margin gain is set to recover
	   an undershoot in half a second
	*/
    int buffer_safe = 3 * per_pict_bits ;
    overshoot_gain =  encparams.bit_rate / (encparams.video_buffer_size-buffer_safe);
	bits_per_mb = (double)encparams.bit_rate / (encparams.mb_per_pict);

	/*
	  Reaction paramer - i.e. quantisation feedback gain relative
	  to bit over/undershoot.
	  For normal frames it is fairly modest as we can compensate
	  over multiple frames and can average out variations in image
	  complexity.

	  For stills we set it a higher so corrections take place
	  more rapidly *within* a single frame.
	*/
	if( encparams.still_size > 0 )
		fb_gain = (int)floor(2.0*encparams.bit_rate/encparams.decode_frame_rate);
	else
		fb_gain = (int)floor(4.0*encparams.bit_rate/encparams.decode_frame_rate);


	/* Set the virtual buffers for per-frame rate control feedback to
	   values corresponding to the quantisation floor (if specified)
	   or a "reasonable" quantisation (6.0) if not.
	*/

	init_quant = (encparams.quant_floor > 0.0 ? encparams.quant_floor : 6.0);
}


void XhiPass2RC::InitGOP( std::deque<Picture *>::iterator gop_pics, int gop_len )
{

    
	/*
	  At the start of a GOP before any frames have gone the
	  actual buffer state represents a long term average. Any
	  undershoot due to the I_frame of the previous GOP
	  should by now have been caught up.
    */
	gop_buffer_correction = 0;
    mjpeg_debug( "PASS2 GOP INIT" );
    
    int i;
    gop_Xhi = 0.0;
    for( i = 0; i < gop_len; ++i )
    {
        gop_Xhi += gop_pics[i]->Complexity();
    }
    fields_in_gop = 2 * gop_len;
    
    /* Allocate target bits for frame based on the actual
    * complexity found in pass-1 encoding.  
    * TODO: Currently each GOP is allocated the bits transferable
    * during its duration plus/minus feedback from the
    * over/undershoot from previous GOPs.
    *
    * This is a simple place-holder.  The final algorithm will allocate
    * bits proportionate to the total complexity of the GOP relative to
    * distribution of GOP complexities in the sequence and will use an
    * buffer-state model to maximise available bits for rate spikes.
    *
    */

    double feedback_correction = buffer_variation * overshoot_gain;
    double available_bits = (encparams.bit_rate+feedback_correction)
                            * fields_in_gop/field_rate;
    
    double gop_quant = fmax( encparams.quant_floor, gop_Xhi / available_bits );
    gop_bitrate = gop_Xhi / gop_quant * field_rate/fields_in_gop; 
}


/* Step 1a: compute target bits for current picture being coded, based
 * on predicting from past frames */

void XhiPass2RC::InitNewPict(Picture &picture)
{

    double varsum;

    actsum = picture.ActivityBestMotionComp();
    varsum = picture.VarSumBestMotionComp();
    avg_act = actsum/(double)(encparams.mb_per_pict);
    avg_var = varsum/(double)(encparams.mb_per_pict);
    sum_avg_act += avg_act;
    sum_avg_var += avg_var;
    actcovered = 0.0;
    sum_vbuf_Q = 0.0;

    // Bitrate model:  bits_picture(i) =  K(i) / quantisation
    // Hence use Complexity metric = bits * quantisation
    // TODO: naive constant-quality = constant-quantisation
    // model allocate bits proportionate to complexity.
    // Soon something smarter...
    

    
    double feedback_correction = buffer_variation * overshoot_gain;
    double available_bits = (gop_bitrate+feedback_correction)
                            * fields_in_gop/field_rate;
    
    // Work out base quantisation of frame based on feedback on bits consumed
    double Xhi = picture.Complexity();
    target_bits = static_cast<int32_t>(available_bits*Xhi/gop_Xhi);
    target_bits = min( target_bits, encparams.video_buffer_size*3/4 );
    base_quant = fmax( encparams.quant_floor, Xhi/target_bits );


    /*
       To account for the wildly different sizes of frames
       we compute a correction to the current instantaneous
       buffer state that accounts for the fact that all other
       thing being equal buffer will go down a lot after the I-frame
       decode but fill up again through the B and P frames.
    */

    gop_buffer_correction += (target_bits-per_pict_bits);

    picture.avg_act = avg_act;
    picture.sum_avg_act = sum_avg_act;
    mquant_change_ctr = encparams.mb_width;
    cur_mquant = ScaleQuant( picture.q_scale_type, base_quant );

}



/*
 * Update rate-controls statistics after pictures has ended..
 *
 * RETURN: The amount of padding necessary for picture to meet syntax or
 * rate constraints...
 */

void XhiPass2RC::UpdatePict( Picture &picture, int &padding_needed)
{
	int32_t actual_bits;		/* Actual (inc. padding) picture bit counts */
	int    i;
	int    Qsum;
	int frame_overshoot;
	actual_bits = picture.EncodedSize();
	frame_overshoot = (int)actual_bits-(int)target_bits;

	/*
	  Compute the estimate of the current decoder buffer state.  We
	  use this to feedback-correct the available bit-pool with a
	  fraction of the current buffer state estimate.  If we're ahead
	  of the game we allow a small increase in the pool.  If we
	  dropping towards a dangerously low buffer we decrease the pool
	  (rather more vigorously).
	  
	  Note that since we cannot hold more than a buffer-full if we have
	  a positive buffer_variation in CBR we assume it was padded away
	  and in VBR we assume we only sent until the buffer was full.
	*/

	
	bits_used += actual_bits;
	bits_transported += per_pict_bits;
	buffer_variation  = static_cast<int32_t>(bits_transported - bits_used);

	if( buffer_variation > 0 )
	{
   		bits_transported = bits_used;
		buffer_variation = 0;	
	}

	Qsum = 0;
	for( i = 0; i < encparams.mb_per_pict; ++i )
	{
		Qsum += picture.mbinfo[i].mquant;
	}

	
    /* actual_Q is the average Quantisation of the block.
	   Its only used for stats display as the integerisation
	   of the quantisation value makes it rather coarse for use in
	   estimating bit-demand */
    picture.AQ = static_cast<double>(Qsum)/encparams.mb_per_pict;
    sum_avg_quant += picture.AQ;
	picture.SQ = sum_avg_quant;

    pict_count[picture.pict_type] += 1;

    mjpeg_debug( "Frame %c A=%6.0f %.2f: I = %6.0f P = %5.0f B = %5.0f",
                 pict_type_char[picture.pict_type],
                 actual_bits/8.0,
                 actual_bits/picture.AQ,
                 sum_size[I_TYPE]/pict_count[I_TYPE],
                 sum_size[P_TYPE]/pict_count[P_TYPE],
                 sum_size[B_TYPE]/pict_count[B_TYPE]
        );
    
                
	VbvEndOfPict(picture);
    padding_needed = 0;
}

int XhiPass2RC::InitialMacroBlockQuant()
{
    return cur_mquant;
}

int XhiPass2RC::TargetPictureEncodingSize()
{
    return target_bits;
}


/*************
 *
 * SelectQuantization - select a quantisation for the current
 * macroblock based on the fullness of the virtual decoder buffer.
 *
 * NOTE: *Must* be called for all Macroblocks as content-based quantisation tuning is
 * supported.
 ************/

int XhiPass2RC::MacroBlockQuant( const MacroBlock &mb )
{
    int lum_variance = mb.BaseLumVariance();
    if( lum_variance < encparams.boost_var_ceil )
    {
        const Picture &picture = mb.ParentPicture();
        /* A.Stevens 2000 : we measure how much *information* (total activity)
           has been covered and aim to release bits in proportion.

           We keep track of a virtual buffer that catches the difference
           between the bits allocated and the bits we actually used.  The
           fullness of this buffer controls quantisation.

        */


        /* scale against dynamic range of mquant and the bits/picture
           count.  encparams.quant_floor != 0.0 is the VBR case where we set a
           bitrate as a (high) maximum and then put a floor on
           quantisation to achieve a reasonable overall size.  Not that
           this *is* baseline quantisation.  Not adjust for local
           activity.  Otherwise we end up blurring active
           macroblocks. Silly in a VBR context.
        */

        double Qj = fmax(base_quant,encparams.quant_floor);

        /*  Heuristic: We decrease quantisation for macroblocks
            with markedly low luminace variance.  This helps make
            gentle gradients (e.g. smooth backgrounds) look better at
            (hopefully) small additonal cost  in coding bits
        */

        double act_boost;
        if( lum_variance < encparams.boost_var_ceil )
        {
            if( lum_variance < encparams.boost_var_ceil/2)
                act_boost = encparams.act_boost;
            else
            {
                double max_boost_var = encparams.boost_var_ceil/2;
                double above_max_boost = 
                    (static_cast<double>(lum_variance)-max_boost_var)
                    / max_boost_var;
                act_boost = 1.0 + (encparams.act_boost-1.0) * (1.0-above_max_boost);
            }
        }
        else
            act_boost = 1.0;
        sum_vbuf_Q += ScaleQuantf(picture.q_scale_type,Qj/act_boost);
        cur_mquant = ScaleQuant(picture.q_scale_type,Qj/act_boost) ;
    }

	/* Update activity covered */
	double act  = mb.Activity();
	actcovered += act;
	 
	return cur_mquant;
}

/* VBV calculations
 *
 * generates warnings if underflow or overflow occurs
 */

/* vbv_end_of_picture
 *
 * - has to be called directly after writing picture_data()
 * - needed for accurate VBV buffer overflow calculation
 * - assumes there is no byte stuffing prior to the next start code
 *
 * Note correction for bytes that will be stuffed away in the eventual CBR
 * bit-stream.
 */

void XhiPass2RC::VbvEndOfPict(Picture &picture)
{
}

/* calc_vbv_delay
 *
 * has to be called directly after writing the picture start code, the
 * reference point for vbv_delay
 *
 * A.Stevens 2000: 
 * Actually we call it just before the start code is written, but anyone
 * who thinks 32 bits +/- in all these other approximations matters is fooling
 * themselves.
 */

void XhiPass2RC::CalcVbvDelay(Picture &picture)
{
	

	/* VBV checks would go here...*/


	if( !encparams.mpeg1 || encparams.quant_floor != 0 || encparams.still_size > 0)
		picture.vbv_delay =  0xffff;
	else if( encparams.still_size > 0 )
		picture.vbv_delay =  static_cast<int>(90000.0/encparams.frame_rate/4);


}



/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
