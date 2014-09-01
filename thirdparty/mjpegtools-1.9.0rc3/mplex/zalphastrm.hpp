
/*
 *  zalphastrm.hpp:  Z/Alpha video elementary input stream
 *
 *  Copyright (C) 2002 Gernot Ziegler <gz@lysator.liu.se>
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

#ifndef __ZALPHASTRM_H__
#define __ZALPHASTRM_H__

#include "videostrm.hpp"

class ZAlphaStream : public VideoStream
{
public:
	//ZAlphaStream(IBitStream &ibs, OutputStream &into);
	ZAlphaStream(IBitStream &ibs, VideoParams *parms, 
                Multiplexor &into);
	void Init( const int stream_num );
    static bool Probe(IBitStream &bs );

	void Close();

	virtual bool MuxPossible(clockticks currentSCR);
    void SetMaxStdBufferDelay( unsigned int demux_rate );
	void OutputSector();
protected:
	void OutputSeqhdrInfo();
    void FillAUbuffer(unsigned int frames_to_buffer);
	virtual void NextDTSPTS( );
	virtual void ScanFirstSeqHeader();
    bool RunOutComplete();

private:
    float z_min; 
    float z_max;	
    int z_format;
    int alpha_depth, z_depth;
}; 	



#endif // __INPUTSTRM_H__


/* 
 * Local variables:
 *  c-file-style: "stroustrup"
 *  tab-width: 4
 *  indent-tabs-mode: nil
 * End:
 */
