
/*
 *  zalphastrm_out.cpp:  Members of input stream classes related to muxing out into
 *               the output stream.
 *
 *  Copyright (C) 2001 Gernot Ziegler <gz@lysator.liu.se>
 *  Copyright (C) 2001 Andrew Stevens <andrew.stevens@philips.com>
 *
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include <config.h>
#include <assert.h>
#include "zalphastrm.hpp"
#include "multiplexor.hpp"


ZAlphaStream::ZAlphaStream(IBitStream &ibs, VideoParams *parms, 
                         Multiplexor &into ) :
	VideoStream( ibs, parms, into)
//ZAlphaStream::ZAlphaStream(IBitStream &ibs, OutputStream &into )	:
//	VideoStream( ibs, into)
{
	prev_offset=0;
    decoding_order=0;
	fields_presented=0;
    temporal_reference=0;
	pulldown_32 = 0;
    temporal_reference = -1;   // Needed to recognise 2nd field of 1st
                               // frame in a field pic sequence
	last_buffered_AU=0;
	max_bits_persec = 0;
	AU_hdr = ZA_SEQUENCE_HEADER;  /* GOP or SEQ Header starting AU? */
	for( int i =0; i<4; ++i )
		num_frames[i] = avg_frames[i] = 0;
    FRAME_CHUNK = 6;
		
}

bool ZAlphaStream::Probe(IBitStream &bs )
{
    return bs.GetBits( 32)  == ZA_SEQUENCE_HEADER;
}

/*********************************
 * Signals if it is permissible/possible to Mux out a sector from the Stream.
 * The universal constraints that muxing should not be complete and that
 * that the reciever buffer should have sufficient it also insists that
 * the muxed data won't hang around in the receiver buffer for more than
 * one second.  This is mainly for the benefit of (S)VCD and DVD applications
 * where long delays mess up random access.
 *******************************/

bool ZAlphaStream::MuxPossible( clockticks currentSCR )
{
    bool a = ElementaryStream::MuxPossible(currentSCR);
    bool b = (RequiredDTS() < (currentSCR + max_STD_buffer_delay));

    //mjpeg_info("Z/A max_STD_buffer_delay %d", max_STD_buffer_delay); 
    //mjpeg_info("Z/A Mux Possible %d, %d ", a, b);
    //mjpeg_info("RequiredDTS %ld",RequiredDTS());
    //mjpeg_info("currentSCR %ld", currentSCR);
    //mjpeg_info("max_STD_buffer_delay %ld", max_STD_buffer_delay);

	return (  a && b );
}

/******************************************************************
	Output_Video
	generates Pack/Sys_Header/Packet information from the
	video stream and writes out the new sector
******************************************************************/

void ZAlphaStream::OutputSector ( )

#if 1
{

	unsigned int max_packet_payload; 	 
	unsigned int actual_payload;
	unsigned int old_au_then_new_payload;
	clockticks  DTS,PTS;
    int autype;

	max_packet_payload = 0;	/* 0 = Fill sector */
  	/* 	
 	   We're now in the last AU of a segment.  So we don't want to go
 	   beyond it's end when filling sectors. Hence we limit packet
 	   payload size to (remaining) AU length.  The same applies when
 	   we wish to ensure sequence headers starting ACCESS-POINT AU's
 	   in (S)VCD's etc are sector-aligned.  

       N.b.runout_PTS is the PTS of the first I picture following the
       run-out is recorded.
	*/
	int nextAU = NextAUType();
	if( ( muxinto.running_out && nextAU == IFRAME && NextRequiredPTS() >= muxinto.runout_PTS) 
        || (muxinto.sector_align_iframeAUs && nextAU == IFRAME  )
		) 
	{
		max_packet_payload = au_unsent;
	}

	/* Figure out the threshold payload size below which we can fit more
	   than one AU into a packet N.b. because fitting more than one in
	   imposses an overhead of additional header fields so there is a
	   dead spot where we *have* to stuff the packet rather than start
	   fitting in an extra AU.  Slightly over-conservative in the case
	   of the last packet...  */

	old_au_then_new_payload = muxinto.PacketPayload( *this,
													 buffers_in_header, 
													 true, true);

	/* CASE: Packet starts with new access unit			*/
	if (new_au_next_sec  )
	{
        autype = AUType();
        //
        // Some types of output format (e.g. DVD) require special
        // control sectors before the sector starting a new GOP
        // N.b. this implies muxinto.sector_align_iframeAUs
        //
        if( gop_control_packet && autype == IFRAME )
        {
            OutputGOPControlSector();
        }

        if(  dtspts_for_all_au  && max_packet_payload == 0 )
            max_packet_payload = au_unsent;

        PTS = RequiredPTS();
        DTS = RequiredDTS();
		actual_payload =
			muxinto.WritePacket ( max_packet_payload,
						*this,
						NewAUBuffers(autype), 
                                  		PTS, DTS,
						NewAUTimestamps(autype) );

	}

	/* CASE: Packet begins with old access unit, no new one	*/
	/*	     can begin in the very same packet					*/

	else if ( au_unsent >= old_au_then_new_payload ||
              (max_packet_payload != 0 && au_unsent >= max_packet_payload) )
	{
		actual_payload = 
			muxinto.WritePacket( au_unsent,
						*this,
						false, 0, 0,
						TIMESTAMPBITS_NO );
	}

	/* CASE: Packet begins with old access unit, a new one	*/
	/*	     could begin in the very same packet			*/
	else /* if ( !new_au_next_sec  && 
			(au_unsent < old_au_then_new_payload)) */
	{
		/* Is there a new access unit ? */
		if( Lookahead() != 0 )
		{
            autype = NextAUType();
			if(  dtspts_for_all_au  && max_packet_payload == 0 )
				max_packet_payload = au_unsent + Lookahead()->length;

			PTS = NextRequiredPTS();
			DTS = NextRequiredDTS();

			actual_payload = 
				muxinto.WritePacket ( max_packet_payload,
						*this,
						NewAUBuffers(autype), 
                                      		PTS, DTS,
						NewAUTimestamps(autype) );
		} 
		else
		{
			actual_payload = muxinto.WritePacket ( au_unsent,
						*this, false, 0, 0,
						TIMESTAMPBITS_NO);
		}
	}
	++nsec;
	buffers_in_header = always_buffers_in_header;
}

#else
{
	unsigned int max_packet_payload; 	 
	unsigned int actual_payload;
	unsigned int prev_au_tail;
	VAunit *vau;
	unsigned int old_au_then_new_payload;
	clockticks  DTS,PTS;
        int autype;

	max_packet_payload = 0;	/* 0 = Fill sector */
  	/* 	
 	   We're now in the last AU of a segment. 
		So we don't want to go beyond it's end when filling
		sectors. Hence we limit packet payload size to (remaining) AU length.
		The same applies when we wish to ensure sequence headers starting
		ACCESS-POINT AU's in (S)VCD's etc are sector-aligned.
	*/
	int nextAU = NextAUType();
	if( (muxinto.running_out && nextAU == IFRAME && 
		 NextRequiredPTS() > muxinto.runout_PTS) ||
		(muxinto.sector_align_iframeAUs && nextAU == IFRAME )
		) 
	{
		max_packet_payload = au_unsent;
	}

	/* Figure out the threshold payload size below which we can fit more
	   than one AU into a packet N.b. because fitting more than one in
	   imposses an overhead of additional header fields so there is a
	   dead spot where we *have* to stuff the packet rather than start
	   fitting in an extra AU.  Slightly over-conservative in the case
	   of the last packet...  */

	old_au_then_new_payload = muxinto.PacketPayload( *this,
					buffers_in_header, true, true);

	/* CASE: Packet starts with new access unit			*/
	if (new_au_next_sec  )
	{
        autype = AUType();
        //
        // Some types of output format (e.g. DVD) require special
        // control sectors before the sector starting a new GOP
        // N.b. this implies muxinto.sector_align_iframeAUs
        //
        if( gop_control_packet && autype == IFRAME )
            OutputGOPControlSector();

        if(  dtspts_for_all_au  && max_packet_payload == 0 )
            max_packet_payload = au_unsent;

        PTS = RequiredPTS();
        DTS = RequiredDTS();
		actual_payload =
			muxinto.WritePacket ( max_packet_payload,
						*this,
						NewAUBuffers(autype), 
                                  		PTS, DTS,
						NewAUTimestamps(autype) );
	}

	/* CASE: Packet begins with old access unit, no new one	*/
	/*	     can begin in the very same packet					*/

	else if ( au_unsent >= old_au_then_new_payload ||
              (max_packet_payload != 0 && au_unsent >= max_packet_payload) )
	{
		actual_payload = 
			muxinto.WritePacket( au_unsent, *this,
						false, 0, 0, TIMESTAMPBITS_NO );
	}

	/* CASE: Packet begins with old access unit, a new one	*/
	/*	     could begin in the very same packet			*/
	else /* if ( !new_au_next_sec  && 
			(au_unsent < old_au_then_new_payload)) */
	{
		/* Is there a new access unit ? */
		if( Lookahead() != 0 )
		{
            autype = NextAUType();
			if(  dtspts_for_all_au  && max_packet_payload == 0 )
				max_packet_payload = au_unsent + Lookahead()->length;

			PTS = NextRequiredPTS();
			DTS = NextRequiredDTS();

			actual_payload = 
				muxinto.WritePacket ( max_packet_payload, *this,
						NewAUBuffers(autype), 
                                     		PTS, DTS, 
						NewAUTimestamps(autype) );
		} 
		else
		{
			actual_payload = muxinto.WritePacket ( 0, *this,
							false, 0, 0,
							TIMESTAMPBITS_NO);
		}

	}
	++nsec;
	buffers_in_header = always_buffers_in_header;
}
#endif

bool ZAlphaStream::RunOutComplete()
{
	return (au_unsent == 0);
// || 
//			( muxinto.running_out &&
//			  au->type == IFRAME && RequiredPTS() >= muxinto.runout_PTS));
}


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
