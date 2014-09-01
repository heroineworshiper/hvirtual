/*
 *  zalphastrm_in.cpp:  Members of Z/Alpha stream class related to raw stream
 *               scanning and buffering.
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
#include <math.h>
#include <stdlib.h>

#include "zalphastrm.hpp"
#include "interact.hpp"
#include "multiplexor.hpp"



static void marker_bit (IBitStream &bs, unsigned int what)
{
    if (what != bs.Get1Bit())
    {
        mjpeg_error ("Illegal MPEG stream at offset (bits) %lld: supposed marker bit not found.",bs.bitcount());
        exit (1);
    }
}


void ZAlphaStream::ScanFirstSeqHeader()
{
    if (bs.GetBits( 32)==ZA_SEQUENCE_HEADER)
    {
        mjpeg_debug("Reading Z/Alpha sequence header ... ");
		num_sequence++;
        
        int headerlen = bs.GetBits(16);
        mjpeg_info("Header frame length is %d", headerlen);

        int frame_num = bs.GetBits(8);
        mjpeg_info("Header frame number is %d", frame_num);
		horizontal_size	= bs.GetBits( 12);
		vertical_size	= bs.GetBits( 12);

#if 0 
		aspect_ratio	= bs.GetBits(  4);
		picture_rate 	= bs.GetBits(  4);
		bit_rate		= bs.GetBits( 18);
		marker_bit( bs, 1);
		vbv_buffer_size	= bs.GetBits( 10);
		CSPF		= bs.get1bit();
#else
        // hardcoded here, but should really be copied from the video stream... 
		aspect_ratio	= 2;
		picture_rate	= 3;
		bit_rate		= 3000;
        //bit_rate = 0x3ffff;
		//marker_bit( bs, 1);
		vbv_buffer_size	= 50;
		CSPF		= 0;
#endif
        uint8_t conv[4];

        conv[0] = bs.GetBits(8);
        conv[1] = bs.GetBits(8);
        conv[2] = bs.GetBits(8);
        conv[3] = bs.GetBits(8);

        memcpy(&z_min, conv, 4);

        conv[0] = bs.GetBits(8);
        conv[1] = bs.GetBits(8);
        conv[2] = bs.GetBits(8);
        conv[3] = bs.GetBits(8);

        memcpy(&z_max, conv, 4);

        z_format = bs.GetBits(8);
        printf("Z format byte is set to 0x%x\n", z_format);
        z_depth = bs.GetBits(8);
        alpha_depth = bs.GetBits(8);
    } 
    else
    {
		mjpeg_error ("Invalid MPEG ZAlpha stream header.");
		exit (1);
    }

	if (mpeg_valid_framerate_code(picture_rate))
    {
		frame_rate = Y4M_RATIO_DBL(mpeg_framerate(picture_rate));
	}
    else
    {
		frame_rate = 25.0;
	}

}




void ZAlphaStream::Init ( const int stream_num )
{
	mjpeg_debug( "SETTING video buffer to %d", parms->DecodeBufferSize() );
	MuxStream::Init( ZALPHA_STR_0+stream_num,
					 1,  // Buffer scale
					 parms->DecodeBufferSize()*1024,
					 0,  // Zero stuffing
					 muxinto.buffers_in_video,
					 muxinto.always_buffers_in_video);
    mjpeg_debug( "Scanning for header info: Z/Alpha stream %02x (%s)",
                ZALPHA_STR_0+stream_num,
                bs.StreamName()
                );
	InitAUbuffer();

	SetBufSize( 4*1024*1024 );
	ScanFirstSeqHeader();

	/* Skip to the end of the 1st AU (*2nd* Picture start!)
	*/
	AU_hdr = ZA_SEQUENCE_HEADER;
	AU_pict_data = 1;
	AU_start = 0LL;
    
    OutputSeqhdrInfo();
}

//
// Set the Maximum STD buffer delay for this video stream.
// By default we set 1 second but if we have specified a video
// buffer that can hold more than 1.0 seconds demuxed data we
// set the delay to the time to fill the buffer.
//

void ZAlphaStream::SetMaxStdBufferDelay( unsigned int dmux_rate )
{
    double max_delay = CLOCKS;
    if( static_cast<double>(BufferSize()) / dmux_rate > 1.0 )
        max_delay *= static_cast<double>(BufferSize()) / dmux_rate;

    //
    // To enforce a maximum STD buffer residency the
    // calculation is a bit tricky as when we decide to mux we may
    // (but not always) have some of the *previous* picture left to
    // mux in which case it is the timestamp of the next picture that counts.
    // For simplicity we simply reduce the limit by 1.5 frame intervals
    // and use the timestamp for the current picture.
    //
    if( frame_rate > 10.0 )
        max_STD_buffer_delay = static_cast<clockticks>(max_delay * (frame_rate-1.5)/frame_rate);
    else
        max_STD_buffer_delay = static_cast<clockticks>(10.0 * max_delay / frame_rate);

    mjpeg_error_exit1("dmux_rate is %d max_delay %g", dmux_rate, max_delay);

}

//
// Return whether AU buffer needs refilling.  There are two cases:
// 1. We have less than our look-ahead "FRAME_CHUNK" buffer AU's
// buffered 2. AU's are very small and we could have less than 1
// sector's worth of data buffered.
//

bool ZAlphaStream::AUBufferNeedsRefill()
{
    int retval = 
        !eoscan
        && ( aunits.current()+FRAME_CHUNK > last_buffered_AU
             ||
             bs.BufferedBytes() < muxinto.sector_size 
            );

    //mjpeg_info("Z/A needs %s refill.\n", retval ? "a":"no");
    return retval;
}

//
// Refill the AU unit buffer setting  AU PTS DTS from the scanned
// header information...
//

void ZAlphaStream::FillAUbuffer(unsigned int frames_to_buffer)
{
    int frame_num;
    unsigned int bits_persec;
    int headerlen;

    if( eoscan )
        return;

	last_buffered_AU += frames_to_buffer;
	mjpeg_debug( "Scanning %d Z/A video frames to frame %d", 
				 frames_to_buffer, last_buffered_AU );

    // We set a limit of 2M to seek before we give up.
    // This is intentionally very high because some heavily
    // padded still frames may have a loooong gap before
    // a following sequence end marker.
	while(!bs.eos() 
          && decoding_order < last_buffered_AU 
          && !muxinto.AfterMaxPTS(access_unit.PTS)
          && bs.SeekSync( SYNCWORD_START, 24, 2*1024*1024)
		)
	{
		syncword = (SYNCWORD_START<<8) + bs.GetBits( 8);
        //mjpeg_info( "Traversing ... syncword 0x%x ", syncword); 
		if( AU_pict_data )
		{
			
			/* Handle the header *ending* an AU...
			   If we have the AU picture data an AU and have now
			   reached a header marking the end of an AU fill in the
			   the AU length and append it to the list of AU's and
			   start a new AU.  I.e. sequence and gop headers count as
			   part of the AU of the corresponding picture
			*/
			stream_length = bs.bitcount()-32LL;
			switch (syncword) 
			{
            // note: there are currently _no_ sequence distinctions in Z/Alpha ! 
			case ZA_SEQUENCE_HEADER :
				access_unit.start = AU_start;
				access_unit.length = (int) (stream_length - AU_start)>>3;
				access_unit.end_seq = 0;
                access_unit.type = IFRAME;
				avg_frames[access_unit.type-1]+=access_unit.length;
                access_unit.dorder = decoding_order;
                access_unit.seq_header = 1; //( AU_hdr == SEQUENCE_HEADER);

                
                bits_persec = 
					(unsigned int) ( ((double)(stream_length - prev_offset)) *
									 2*frame_rate / ((double)(2 /*+ fields_presented- group_start_field*/)));
				
                //mjpeg_info("New bits_persec is %d bits/second.", bits_persec);
                
				if( bits_persec > max_bits_persec )
				{
					max_bits_persec = bits_persec;
				}

				prev_offset = stream_length;

                headerlen = bs.GetBits(16);
                frame_num = bs.GetBits(8);
                //mjpeg_info("Header frame number is %d", frame_num);

				mjpeg_debug( "Found Z/Alpha AU %d (real: %d): DTS=%d", access_unit.dorder, frame_num,
							 access_unit.DTS/300);
                
                if ( decoding_order >= old_frames+1000 )
                {
                    mjpeg_debug("Got %d picture headers.", decoding_order);
                    old_frames = decoding_order;
                }

                decoding_order++;
                //mjpeg_info("Decoding order is now %d", decoding_order);

                NextDTSPTS( access_unit.DTS, access_unit.PTS);
				aunits.append( access_unit ); // INSERTING NEW AUNIT ! 

                
                if ((access_unit.type>0) && (access_unit.type<5))
                {
                    num_frames[access_unit.type-1]++; // counts the frames in this Z/Alpha stream
                }                
                
				AU_hdr = syncword;
				AU_start = stream_length;
				AU_pict_data = 1;

                if (decoding_order >= last_buffered_AU)
                {
                    mjpeg_debug("Reached end of buffer ...");
                    break;
                }

				break;

			case SEQUENCE_END: // still using the MPEG sequence end, so that we don't have to use another SYNCWORD
				access_unit.start = AU_start;
				access_unit.length = (int) ((stream_length - AU_start)>>3)+4;
				access_unit.end_seq = 1;
				avg_frames[access_unit.type-1]+=access_unit.length;
                access_unit.dorder = decoding_order;
                access_unit.seq_header = 1; //( AU_hdr == SEQUENCE_HEADER);

                access_unit.type = IFRAME;
				mjpeg_debug( "Adding final AU %d (real: %d): DTS=%d", access_unit.dorder, frame_num,
							 access_unit.DTS/300);
                NextDTSPTS( access_unit.DTS, access_unit.PTS);
				aunits.append( access_unit ); // INSERTING NEW AUNIT ! 


				//access_unit.length = ((stream_length - AU_start)>>3)+4;
				//access_unit.end_seq = 1;
				//aunits.append( access_unit );
				mjpeg_debug( "Z/Alpha: Scanned to end AU %d at %d", access_unit.dorder, stream_length/8 );
				avg_frames[access_unit.type-1]+=access_unit.length;

				/* Do we have a sequence split in the video stream? */
				if( !bs.eos() && 
					bs.GetBits( 32) ==SEQUENCE_HEADER )
				{
					stream_length = bs.bitcount()-32LL;
					AU_start = stream_length;
					syncword  = AU_hdr = SEQUENCE_HEADER;
					AU_pict_data = 0;
					if( !muxinto.split_at_seq_end )
						mjpeg_warn("Sequence end marker found in video stream but single-segment splitting specified!" );
				}
				else
				{
					if( !bs.eos() && muxinto.split_at_seq_end )
						mjpeg_warn("No seq. header starting new sequence after seq. end!");
				}
					
				num_seq_end++;
				break;
			}
		}
	}
	last_buffered_AU = decoding_order;
	num_pictures = decoding_order;	
	eoscan = bs.eos() || muxinto.AfterMaxPTS(access_unit.PTS);
}

void ZAlphaStream::Close()
{
    unsigned int comp_bit_rate	;
    unsigned int peak_bit_rate  ;

    stream_length = (unsigned int)(bs.bitcount() / 8);
    for (int i=0; i<4; i++)
	{
		avg_frames[i] /= num_frames[i] == 0 ? 1 : num_frames[i];
	}

    comp_bit_rate = (unsigned int)
		(
			(((double)stream_length) / ((double)fields_presented)) * 2.0
			* ((double)frame_rate)  + 25.0
			) / 50;
	
	/* Peak bit rate in 50B/sec units... */
	peak_bit_rate = ((max_bits_persec / 8) / 50);
	mjpeg_info ("VIDEO_STATISTICS: %02x", stream_id); 
    mjpeg_info ("Video Stream length: %11llu bytes",stream_length);
    mjpeg_info ("Sequence headers: %8u",num_sequence);
    mjpeg_info ("Sequence ends   : %8u",num_seq_end);
    mjpeg_info ("No. Pictures    : %8u",num_pictures);
    mjpeg_info ("No. I Frames    : %8u avg. size%6u bytes",
			  num_frames[0],avg_frames[0]);
    mjpeg_info("Average bit-rate : %8u bits/sec",comp_bit_rate*400);
    mjpeg_info("Peak bit-rate    : %8u  bits/sec",peak_bit_rate*400);

	
}
	



/*************************************************************************
	OutputSeqHdrInfo
     Display sequence header parameters
*************************************************************************/

void ZAlphaStream::OutputSeqhdrInfo ()
{
	const char *str;
	mjpeg_info("Z/Alpha STREAM: %02x", stream_id);

    mjpeg_info ("Frame width     : %u\n",horizontal_size);
    mjpeg_info ("Frame height    : %u\n",vertical_size);

    mjpeg_info ("Z min    : %f\n", z_min);
    mjpeg_info ("Z max    : %f\n", z_max);

    mjpeg_info ("Z format 0x%x, depth    : %d", z_format, z_depth);
    mjpeg_info ("Alpha depth   : %d", alpha_depth);

	if (mpeg_valid_aspect_code(muxinto.mpeg, aspect_ratio))
		str =  mpeg_aspect_code_definition(muxinto.mpeg,aspect_ratio);
	else
		str = "forbidden";
    mjpeg_info ("Aspect ratio    : %s", str );

    if (picture_rate == 0)
		mjpeg_info( "Picture rate    : forbidden");
    else if (mpeg_valid_framerate_code(picture_rate))
		mjpeg_info( "Picture rate    : %2.3f frames/sec",
					Y4M_RATIO_DBL(mpeg_framerate(picture_rate)) );
    else
		mjpeg_info( "Picture rate    : %x reserved",picture_rate);
				
    if (bit_rate == 0x3ffff)
		{
			bit_rate = 0;
			mjpeg_info( "Bit rate        : variable"); 
		}
    else if (bit_rate == 0)
		mjpeg_info( "Bit rate       : forbidden");
    else
		mjpeg_info( "Bit rate        : %u bits/sec",
					bit_rate*400);


    mjpeg_info("Vbv buffer size : %u bytes",vbv_buffer_size*2048);
    mjpeg_info("CSPF            : %u",CSPF);
}

//
// Compute DTS of current AU in the Z/alpha sequence being
// scanned.  
//

void ZAlphaStream::NextDTSPTS( clockticks &DTS, clockticks &PTS)
{
#if 0  // ZAlpha can't currently handle interlaced or 3:2 pulldown
    if( pict_struct != PIC_FRAME )
    {
		DTS = static_cast<clockticks>
			(fields_presented * (double)(CLOCKS/2) / frame_rate);
        int dts_fields = temporal_reference*2 + group_start_field+1;
        if( temporal_reference == prev_temp_ref )
            dts_fields += 1;
        PTS =
            static_cast<clockticks>(dts_fields* (double)(CLOCKS/2) / frame_rate);
		access_unit.porder = temporal_reference /*+ group_start_pic*/;
        fields_presented += 1;
    }	
    else if( pulldown_32 )
	{
		int frames2field;
		int frames3field;
		DTS = static_cast<clockticks>
			(fields_presented * (double)(CLOCKS/2) / frame_rate);
		if( repeat_first_field )
		{
			frames2field = (temporal_reference+1) / 2;
			frames3field = temporal_reference / 2;
			fields_presented += 3;
		}
		else
		{
			frames2field = (temporal_reference) / 2;
			frames3field = (temporal_reference+1) / 2;
			fields_presented += 2;
		}
		PTS = static_cast<clockticks>
			((frames2field*2 + frames3field*3 + group_start_field+1) * (double)(CLOCKS/2) / frame_rate);
		access_unit.porder = temporal_reference + group_start_pic;
	}
    else
#endif
	{
		DTS = static_cast<clockticks> 
			(decoding_order * (double)CLOCKS / frame_rate);
        PTS=DTS;
		fields_presented += 2;
	}
}





/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */

