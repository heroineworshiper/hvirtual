/*
 *	MPEG Audio Encoder for DirectShow
 *	CEncoder definition
 *
 *	Copyright (c) 2000 Marie Orlova, Peter Gubanov, Elecard Ltd.
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

#if !defined(AFX_VITECENCODER_H__40DC8A44_B937_11D2_A381_A2FD7C37FA15__INCLUDED_)
#define AFX_VITECENCODER_H__40DC8A44_B937_11D2_A381_A2FD7C37FA15__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "lame.h"

#define	INPUT_BUFF_SIZE		65536*2
#define OUTPUT_BUFF_SIZE	16384
#define	FRAME_SIZE_LAYER3	1152

extern DWORD dwBitRateValue[2][14];


typedef struct {
	DWORD	dwSampleRate;					//SF in Hz
	DWORD	dwBitrate;						//BR in bit per second
	vbr_mode	vmVariable;
	DWORD	dwVariableMin;                  //specify a minimum allowed bitrate
	DWORD	dwVariableMax;                  //specify a maximum allowed bitrate
	DWORD	dwQuality;                      //Encoding quality
	DWORD   dwVBRq;                         // VBR quality setting (0=highest quality, 9=lowest)                         
	long	lLayer;									//Layer: 1 or 2

	DWORD	dwChMode;								//Channel coding mode: see doc
	DWORD	dwForceMS;

	DWORD	bCRCProtect;						//Is CRC protection activated?
	DWORD	bCopyright;							//Is the stream protected by copyright?
	DWORD	bOriginal;                          //Is the stream an original?

	DWORD	dwPES; 

	DWORD	dwEnforceVBRmin;
	DWORD	dwVoiceMode; 
	DWORD	dwKeepAllFreq; 
	DWORD	dwStrictISO; 
	DWORD	dwNoShortBlock; 
	DWORD	dwXingTag; 
	DWORD   dwModeFixed;
	
} MPEG_ENCODER_CONFIG;

////////////////////////////////////////////////////////////////////////////////////////
// CEncoder is a wraper class for VITEC audio encoder SDK
////////////////////////////////////////////////////////////////////////////////////////
class CEncoder : public CCritSec
{
public:

	CEncoder();
	virtual ~CEncoder();

	// Initialize encoder with PCM stream properties
	HRESULT SetInputType(LPWAVEFORMATEX lpwfex);	// returns E_INVALIDARG if not supported
	// GetInputType - returns current input type
	HRESULT GetInputType(WAVEFORMATEX *pwfex)
	{ 
		if(m_bInpuTypeSet)
		{
			memcpy(pwfex, &m_wfex, sizeof(WAVEFORMATEX));
			return S_OK;
		}
		else
			return E_UNEXPECTED;
	}

	// Set MPEG audio parameters
	HRESULT SetOutputType(MPEG_ENCODER_CONFIG &mabsi);		// returns E_INVALIDARG if not supported or
														// not compatible with input type
	// Return current MPEG audio settings
	HRESULT GetOutputType(MPEG_ENCODER_CONFIG* pmabsi)		
	{ 
		if(m_bOutpuTypeSet)
		{
			memcpy(pmabsi, &m_mabsi, sizeof(MPEG_ENCODER_CONFIG));
			return S_OK;
		}
		else
			return E_UNEXPECTED;
	}
	
	// Set if output stream is a PES
	void SetPES(bool bPES)		
	{ 
		m_mabsi.dwPES = bPES;
	}
	// Is output stream a PES
	BOOL IsPES()		
	{ 
		return (BOOL)m_mabsi.dwPES;
	}
	
	// Initialize encoder SDK
	HRESULT Init();
	// Close encoder SDK
	HRESULT Close();

	// Encode media sample data
	HRESULT Encode(LPVOID pSrc, DWORD dwSrcSize, 
					LPVOID pDst, LPDWORD lpdwDstSize, REFERENCE_TIME rt);
	HRESULT Finish(LPVOID pDst, LPDWORD lpdwDstSize);

protected:
	
	HRESULT SetDefaultOutputType(LPWAVEFORMATEX lpwfex); // 

	// Input meida type
	WAVEFORMATEX   m_wfex;
	// Output media type
	MPEG_ENCODER_CONFIG m_mabsi;

	// Compressor private data
	lame_global_flags *pgf;

	// Compressor miscelaneous state
	BOOL	m_bInpuTypeSet;
	BOOL	m_bOutpuTypeSet;

	// Refrence times of media samples
	REFERENCE_TIME		m_rtLast;
	BOOL				m_bLast;

	// PES headers routine
	int			m_nPos;
	int			m_nCounter;
	LPBYTE		m_pPos;

	void		Reset();
	void		CreatePESHdr(LPBYTE ppHdr, LONGLONG dwPTS, int dwPacketSize);
	void		WriteBits(int nBits, int nVal);
};

#endif // !defined(AFX_VITECENCODER_H__40DC8A44_B937_11D2_A381_A2FD7C37FA15__INCLUDED_)
