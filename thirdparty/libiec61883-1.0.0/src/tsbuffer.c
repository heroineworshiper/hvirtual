/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 * This file orginially written in C++ by Dan Maas; ported to C by Dan Dennedy.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>

#include "deque.h"
#include "tsbuffer.h"

// max # of packets to look ahead for PCRs
// reasonable values: 1000 - 10000
#define MAX_PCR_LOOKAHEAD 20000

// # of PCRs to average over when estimating bitrate
// reasonable values: 1-100
#define PCR_SMOOTH_INTERVAL 5

// approximate # of ISO cycles transmission delay
// valid range is 0-10; good values are 5-15
#define SYT_OFFSET 7

// leave this off for now; I don't think it's needed with the new algorithm
#define ENABLE_PCR_DRIFT_CORRECTION 0

// # of seconds between PCR bitrate drift checks
// reasonable values: 1-5
#define PCR_DRIFT_INTERVAL 1

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed long long s64;

/*  8-byte CIP (Common Isochronous Packet) header */
struct CIP_header
{
	u8 b[ 8 ];
};

static void 
fill_mpeg_cip_header( struct CIP_header *cip, u8 src_node_id, u8 counter )
{
	cip->b[ 0 ] = src_node_id;
	cip->b[ 1 ] = 0x06;
	cip->b[ 2 ] = 0xC4;
	cip->b[ 3 ] = counter;

	cip->b[ 4 ] = 0xA0;
	cip->b[ 5 ] = 0x80; // "time-shifted" flag; also try 0x00
	cip->b[ 6 ] = 0x00;
	cip->b[ 7 ] = 0x00;
}

/* MPEG-2 TS timestamp */
typedef u32 sph_t;

static inline u32 
mpeg_ts_offset( sph_t t )
{
	return t & 0xfff;
}

static inline u32 
mpeg_ts_count( sph_t t )
{
	return ( t >> 12 ) & 0x1fff;
}

static inline sph_t 
make_sph( u32 count, u32 offset )
{
	return ( ( count & 0x1fff ) << 12 ) | ( offset & 0xfff );
}


/* MPEG-2 TSP packet (timestamp plus TS packet) */
struct TSP_packet
{
	sph_t sph;
	unsigned char data[ 188 ];
};

// one ISO cycle
struct buf_cycle
{
	struct CIP_header header;
	struct TSP_packet packet[ 3 ]; // up to three TSP packets
};

// 188-byte MPEG-2 TS packet
struct mpeg2_ts
{
	u8 ts_header[ 4 ];
	u8 adapt_len;
	u8 adapt_flags;
	u8 pcr[ 6 ];
	u8 data[ 176 ];
};

static u8
ts_get_pid( struct mpeg2_ts *ts )
{
	return ( ( ts->ts_header[ 1 ] << 8 ) + ts->ts_header[ 2 ] ) & 0x1fff;
}

// returns the PCR clock, in units of 1 / 27MHz
static u64 
ts_get_pcr( struct mpeg2_ts *ts )
{
	u64 pcr;

	pcr = ts->pcr[ 0 ] << 25;
	pcr += ts->pcr[ 1 ] << 17;
	pcr += ts->pcr[ 2 ] << 9;
	pcr += ts->pcr[ 3 ] << 1;
	pcr += ( ts->pcr[ 4 ] & 0x80 ) >> 7;
	pcr *= 300;
	pcr += ( ts->pcr[ 4 ] & 0x1 ) << 8;
	pcr += ts->pcr[ 5 ];

	return pcr;
}

static int
ts_has_pcr( struct mpeg2_ts *ts, int pid )
{
	if ( ( pid == -1 ) ||
	        ( pid == ts_get_pid( ts ) ) )
	{


		if ( ( ts->ts_header[ 3 ] & 0x20 )  // adaptation field present
		        && ( ts->adapt_len > 0 )
		        && ( ts->adapt_flags & 0x10 ) )
		{
			return 1;
		}
	}

	return 0;
}



/** tsbuffer ******************************************************************/

struct tsbuffer
{
	// queue of TS packets waiting to be sent
	iec61883_deque_t ts_queue;
	iec61883_mpeg2_xmit_t read_packet;
	void *callback_data;
	unsigned int dropped;

	// PCR state machine
	u64 last_pcr;  // last PCR seen
	u64 pcr_drift_ref;     // PCR from which to base drift calculations
	u32 pcr_drift_cycles;  // how many cycles have passed since pcr_drift_ref was set
	u32 packets_since_last_pcr;
	u64 delta_pcr_per_packet;

	// packetization state machine
	// num/denom algorithm for determining # of TS packets to send out in each ISO cycle
	u64 tsp_accum;
	u64 tsp_whole;
	u64 tsp_num;
	u64 tsp_denom;

	int selected_pid;

	// ISO continuity counter
	u32 iso_counter;
};

tsbuffer_t
tsbuffer_init (iec61883_mpeg2_xmit_t read_cb, void *callback_data, int pid)
{
	tsbuffer_t this = (tsbuffer_t) calloc (1, sizeof (struct tsbuffer));
	if (this) {
		// initialize members
		this->last_pcr = 0;
		this->pcr_drift_ref = 0;
		this->pcr_drift_cycles = 0;
		this->tsp_accum = 0;
		this->iso_counter = 0;
		this->selected_pid = pid;
		this->ts_queue = iec61883_deque_init();
		this->read_packet = read_cb;
		this->callback_data = callback_data;
		this->dropped = 0;
		
		// skip ahead to the first PCR
		tsbuffer_read_to_next_pcr (this);
		this->last_pcr = ts_get_pcr (iec61883_deque_back (this->ts_queue));
	
		// dump the useless packets that precede the first PCR
		while (iec61883_deque_size (this->ts_queue) > 0)
			free (iec61883_deque_pop_front (this->ts_queue));
	
		tsbuffer_refill (this);
	}
	return this;
}


void 
tsbuffer_set_pid (tsbuffer_t this, int pid)
{
	this->selected_pid = pid;
}

int
tsbuffer_read_ts (tsbuffer_t this)
{
	struct mpeg2_ts* new_ts = calloc (1, sizeof (struct mpeg2_ts));
	unsigned char *ts = (unsigned char*) new_ts;

	if (this->read_packet (ts, 1, this->dropped, this->callback_data) < 0)
		return 0;
	/* Do not necessarily indicate dropped packet on next call; rawiso handler 
	   will set again when needed. */
	this->dropped = 0;
	
	iec61883_deque_push_back (this->ts_queue, new_ts);

	return 1;
}

int
tsbuffer_read_to_next_pcr (tsbuffer_t this)
{
	do {
		if (iec61883_deque_size (this->ts_queue) > MAX_PCR_LOOKAHEAD) {
			fprintf (stderr, "couldn't find a PCR within %d packets; giving up\n", MAX_PCR_LOOKAHEAD);
			fprintf (stderr, "(try reducing PCR_SMOOTH_INTERVAL or increase MAX_PCR_LOOKAHEAD\n");
			return 0;
		}

		if (tsbuffer_read_ts (this) == 0)
			return 0;
		if (this->selected_pid == -1)
			this->selected_pid = ts_get_pid (iec61883_deque_back (this->ts_queue));

	} while (ts_has_pcr (iec61883_deque_back (this->ts_queue), this->selected_pid) == 0);

	return 1;
}

int
tsbuffer_refill (tsbuffer_t this)
{
	unsigned int i;
	u32 n_packets;
	u64 pcr;
	u64 delta_pcr;
	u64 num;
	u64 denom;
	
	for (i = 0; i < PCR_SMOOTH_INTERVAL; i++)
		if (tsbuffer_read_to_next_pcr (this) == 0)
			return 0;

	n_packets = iec61883_deque_size (this->ts_queue);

	pcr = ts_get_pcr (iec61883_deque_back (this->ts_queue));

	if (this->pcr_drift_ref == 0) {
		// set up a PCR drift calculation
		this->pcr_drift_ref = pcr;
		this->pcr_drift_cycles = 0;
	}

	delta_pcr = pcr - this->last_pcr;

	this->delta_pcr_per_packet = delta_pcr / n_packets;

	// Calculate the TSP packetization rate

	/*
	   This is an *exact* calculation! We want to send n_packets packets
	   in the period of time covered by delta_pcr. The packet transmission
	   rate should therefore be:

	            (n_packets / delta_pcr) * (27,000,000 PCRs/sec)

	   But what we really want is the packet transmission rate *per ISO cycle*,
	   not per second. So, divide by 8000 ISO cycles/sec:

	            (n_packets / delta_pcr) * (27,000,000 / sec) / (8000 cycles /sec)

	   And then something magic happens: 27,000,000 / 8,000 = 3,375 exactly!

	   So the *exact* transmission rate is:

	            (n_packets / delta_pcr) * (3,375)        (per ISO cycle)

	   We use a standard numerator/denominator algorithm to achieve this.
	          
	 */

	num = n_packets * 3375;
	denom = delta_pcr;

	this->tsp_whole = num / denom;
	this->tsp_num = num % denom;
	this->tsp_denom = denom;

	/* note: We don't reset tsp_accum to zero. This should improve our accuracy,
	   as long as the transmission rate stays fairly constant. */

#if 0
	// for my SD streams: 1 90/2981 is best
	fprintf( stderr, "PCR (PID %d) after %d packets %llu delta = %llu    TSP wh %llu num %llu den %llu\n",
			 ts_get_pid (iec61883_deque_back (this->ts_queue)), n_packets, pcr, delta_pcr,
			 this->tsp_whole, this->tsp_num, this->tsp_denom );
#endif

	this->last_pcr = pcr;
	this->packets_since_last_pcr = 0;

	return 1;
}

u32 
tsbuffer_send_iso_cycle (tsbuffer_t this, void *data, 
	u32 iso_cycle, u8 src_node_id, unsigned int dropped)
{
	// choose # of tsps by num/denom algorithm
	unsigned int n_tsps = this->tsp_whole;
	unsigned int i;
	struct buf_cycle *cycle = (struct buf_cycle*) data;
		
	this->dropped = dropped;

top:
	if (this->tsp_accum > (this->tsp_denom - this->tsp_num)) {
		n_tsps += 1;
		this->tsp_accum -= (this->tsp_denom - this->tsp_num);
	} else {
		this->tsp_accum += this->tsp_num;
	}

	if (this->pcr_drift_ref != 0) {
		this->pcr_drift_cycles++;
		if (this->pcr_drift_cycles % (PCR_DRIFT_INTERVAL * 8000) == 0) {

			// time to check for PCR drift

			// estimate the PCR that would correspond to *this* packet, if it had a PCR
			u64 cur_pcr = this->last_pcr + ((u64) this->packets_since_last_pcr)
				* this->delta_pcr_per_packet;

			// compare cur_pcr to pcr_drift_ref + (1/27MHz) * seconds_elapsed_since_pcr_drift_ref
			s64 drift = (cur_pcr - this->pcr_drift_ref) - (((s64) 27000000) * 
				(this->pcr_drift_cycles / 8000));

#if 0
			fprintf (stderr, "PCR drift: %lld after %u seconds\n", drift, this->pcr_drift_cycles/8000);
#endif

			if (ENABLE_PCR_DRIFT_CORRECTION) {
				//if(pcr_drift_cycles % (5*PCR_DRIFT_INTERVAL*8000) == 0)
				if ( ( drift > (s64) 27000000 ) | ( drift < (s64) -27000000) ) {
					fprintf (stderr, "applying PCR correction of %lld\n", -drift);

					// apply the drift correction
					//this->last_pcr -= drift;
					this->pcr_drift_ref = 0;
					this->pcr_drift_cycles = 0;
					while (iec61883_deque_size (this->ts_queue) > 0)
						free (iec61883_deque_pop_front (this->ts_queue));
					if (tsbuffer_refill (this) == 0)
						return 0;
					goto top;
				}
			}
		}
	}

	while (n_tsps > iec61883_deque_size (this->ts_queue))
		if (tsbuffer_read_ts (this) == 0)
			return 0;

	// write the CIP header
	fill_mpeg_cip_header (&cycle->header, src_node_id, this->iso_counter);

	for (i = 0; i < n_tsps; i++) {
		u32 cycle_count ;
		
		memcpy ((char*) &cycle->packet[i].data[0], 
			(char*) iec61883_deque_front (this->ts_queue), 
			sizeof (struct mpeg2_ts));

		free (iec61883_deque_pop_front (this->ts_queue));

		// set timestamp to iso_cycle + SYT_OFFSET + 1000 offsets per TSP
		// (since at most 3 TSP per packet; this will ensure monotonically
		// increasing timestamps, spaced out semi-regularly)
		cycle_count = (iso_cycle + SYT_OFFSET) % 8000;

		cycle->packet[i].sph = htonl( make_sph( cycle_count, 1000 * i));

		this->packets_since_last_pcr += 1;
	}

	if (iec61883_deque_size (this->ts_queue) == 0)
		if (tsbuffer_refill (this) == 0)
			return 0;

	// advance continuity counter by 8 per TSP in this cycle
	this->iso_counter += 8 * n_tsps;

	return sizeof (struct CIP_header) + n_tsps * sizeof (struct TSP_packet);
}

void
tsbuffer_close (tsbuffer_t this)
{
	iec61883_deque_close (this->ts_queue);
	free (this);
}
