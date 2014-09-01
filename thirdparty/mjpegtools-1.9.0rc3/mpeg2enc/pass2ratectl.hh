#ifndef _PASS2RATECTL_HH
#define _PASS2RATECTL_HH

/*  (C) 2006 Andrew Stevens */

/*  This is free software; you can redistribute it
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

#include "ratectl.hh"

/*
	The parts of of the rate-controller's state needed for save/restore if backing off
	a partial encoding
*/

class XhiPass2RCState :  public RateCtlState
{
public:
	virtual ~XhiPass2RCState() {}
	virtual RateCtlState *New() const { return new XhiPass2RCState; }
	virtual void Set( const RateCtlState &state ) { *this = static_cast<const XhiPass2RCState &>(state); }
	virtual const RateCtlState &Get() const { return *this; }


	int32_t per_pict_bits;
	int     fields_in_gop;
	double  field_rate;
	int     fields_per_pict;

    double overshoot_gain;
	int32_t buffer_variation;
	int64_t bits_transported;
	int64_t bits_used;
	int32_t gop_buffer_correction;

    int32_t target_bits;    // target bits for current frame

    double base_quant;
    double gop_Xhi;
    double gop_bitrate;


    /*
	  actsum - Total activity (sum block variances) in frame
	  actcovered - Activity macroblocks so far quantised (used to
	  fine tune quantisation to avoid starving highly
	  active blocks appearing late in frame...) UNUSED
	  avg_act - Current average activity...
	*/
	double actsum;
	double actcovered;
	double sum_avg_act;
	double avg_act;
	double avg_var;
	double sum_avg_var;
	double sum_avg_quant;
	double sum_vbuf_Q;

     /* 
        Total complexity of pictures in current GOP
     */

	int min_d, max_d;
	int min_q, max_q;

	double bits_per_mb;



    // Some statistics for measuring if things are going well.
    double sum_size[NUM_PICT_TYPES];
    int pict_count[NUM_PICT_TYPES];

};


class XhiPass2RC :  public Pass2RateCtl,  public XhiPass2RCState
{
public:
	XhiPass2RC( EncoderParams &encoder );
	virtual void InitSeq( bool reinit );
    virtual void InitGOP( std::deque<Picture *>::iterator gop_pics, int gop_len );
	virtual void InitNewPict (Picture &picture);
	virtual void UpdatePict ( Picture &picture, int &padding_needed );
	virtual int  MacroBlockQuant( const MacroBlock &mb);
	virtual int  InitialMacroBlockQuant();
    virtual int  TargetPictureEncodingSize();
	virtual void CalcVbvDelay (Picture &picture);

    double SumAvgActivity()  { return sum_avg_act; }
private:
	virtual void VbvEndOfPict (Picture &picture);

    int     cur_mquant;
    int     mquant_change_ctr;
    
	// inverse feedback gain: its in weird units 
	// The quantisation is porportionate to the
	// buffer bit overshoot (virtual buffer fullness)
	// *divided* by fb_gain  A 
	int32_t fb_gain;		
};


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
#endif
