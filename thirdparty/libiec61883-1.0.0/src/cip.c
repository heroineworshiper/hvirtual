/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 * This file written by Kristian Hogsberg.
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

#include "iec61883.h"
#include "iec61883-private.h"

#include <netinet/in.h>


/* Integer fractional math.  When we transmit a 44k1Hz signal we must
 * send 5 41/80 samples per isochronous cycle, as these occur 8000
 * times a second.  Of course, we must send an integral number of
 * samples in a packet, so we use the integer math to alternate
 * between sending 5 and 6 samples per packet.
 */

static void
fraction_init(struct iec61883_fraction *f, int numerator, int denominator)
{
  f->integer = numerator / denominator;
  f->numerator = numerator % denominator;
  f->denominator = denominator;
}

static __inline__ void
fraction_add(struct iec61883_fraction *dst,
	     struct iec61883_fraction *src1, struct iec61883_fraction *src2)
{
  /* assert: src1->denominator == src2->denominator */

  int sum, denom;

  /* We use these two local variables to allow gcc to optimize
   * the division and the modulo into only one division. */

  sum = src1->numerator + src2->numerator;
  denom = src1->denominator;
  dst->integer = src1->integer + src2->integer + sum / denom;
  dst->numerator = sum % denom;
  dst->denominator = denom;
}

static __inline__ void
fraction_sub_int(struct iec61883_fraction *dst, struct iec61883_fraction *src, int integer)
{
  dst->integer = src->integer - integer;
  dst->numerator = src->numerator;
  dst->denominator = src->denominator;
}

static __inline__ int
fraction_floor(struct iec61883_fraction *frac)
{
  return frac->integer;
}

static __inline__ int
fraction_ceil(struct iec61883_fraction *frac)
{
  return frac->integer + (frac->numerator > 0 ? 1 : 0);
}

void
iec61883_cip_init(struct iec61883_cip *ptz, int format, int fdf,
		int rate, int dbs, int syt_interval)
{
  const int transfer_delay = 9000;

  ptz->rate = rate;
  ptz->cycle_count = transfer_delay / 3072;
  ptz->cycle_count2 = 0;
  ptz->format = format;
  ptz->fdf = fdf;
  ptz->mode = IEC61883_MODE_BLOCKING_EMPTY;
  ptz->dbs = dbs;
  ptz->dbc = 0;
  ptz->syt_interval = syt_interval;

  fraction_init(&ptz->samples_per_cycle, ptz->rate, 8000);
  fraction_init(&ptz->ready_samples, 0, 8000);

  /* The ticks_per_syt_offset is initialized to the number of
   * ticks between syt_interval events.  The number of ticks per
   * second is 24.576e6, so the number of ticks between
   * syt_interval events is 24.576e6 * syt_interval / rate.
   */
  fraction_init(&ptz->ticks_per_syt_offset,
		24576000 * ptz->syt_interval, ptz->rate);
  fraction_init(&ptz->cycle_offset,
		(transfer_delay % 3072) * ptz->rate, ptz->rate);
}

void 
iec61883_cip_set_transmission_mode(struct iec61883_cip *ptz, int mode)
{
  ptz->mode = mode;
}

int 
iec61883_cip_get_max_packet_size(struct iec61883_cip *ptz)
{
  int max_nevents;

  if (ptz->mode == IEC61883_MODE_BLOCKING_EMPTY || ptz->mode == IEC61883_MODE_BLOCKING_NODATA)
    max_nevents = ptz->syt_interval;
  else
    max_nevents = fraction_ceil(&ptz->samples_per_cycle);
 
  return max_nevents * ptz->dbs * 4 + 8;
}


int
iec61883_cip_fill_header(raw1394handle_t handle, struct iec61883_cip *ptz,
		struct iec61883_packet *packet)
{
  struct iec61883_fraction next;
  int nevents, nevents_dbc, syt_index, syt;

  fraction_add(&next, &ptz->ready_samples, &ptz->samples_per_cycle);
  if (ptz->mode == IEC61883_MODE_BLOCKING_EMPTY ||
      ptz->mode == IEC61883_MODE_BLOCKING_NODATA) {
    if (fraction_floor(&next) >= ptz->syt_interval)
      nevents = ptz->syt_interval;
    else
      nevents = 0;
  }
  else
    nevents = fraction_floor(&next);

  if (ptz->mode == IEC61883_MODE_BLOCKING_NODATA) {
    /* The DBC is incremented even with NO_DATA packets. */
    nevents_dbc = ptz->syt_interval;
  }
  else {
    nevents_dbc = nevents;
  }

  /* Now that we know how many events to put in the packet, update the
   * fraction ready_samples. */
  fraction_sub_int(&ptz->ready_samples, &next, nevents);

  /* Calculate synchronization timestamp (syt). First we
   * determine syt_index, that is, the index in the packet of
   * the sample for which the timestamp is valid. */
  syt_index = (ptz->syt_interval - ptz->dbc) & (ptz->syt_interval - 1);
  if (syt_index < nevents) {
    syt = ((ptz->cycle_count << 12) | fraction_floor(&ptz->cycle_offset)) & 0xffff;
    fraction_add(&ptz->cycle_offset, &ptz->cycle_offset,
		 &ptz->ticks_per_syt_offset);

    /* This next addition should be modulo 8000 (0x1f40),
     * but we only use the lower 4 bits of cycle_count, so
     * we don't need the modulo. */
    ptz->cycle_count += ptz->cycle_offset.integer / 3072;
    ptz->cycle_offset.integer %= 3072;
  }
  else
    syt = 0xffff;

  packet->eoh0 = 0;

  /* Our node ID can change after a bus reset, so it is best to fetch
   * our node ID for each packet. */
  packet->sid = raw1394_get_local_id( handle ) & 0x3f;

  packet->dbs = ptz->dbs;
  packet->fn = 0;
  packet->qpc = 0;
  packet->sph = 0;
  packet->reserved = 0;
  packet->dbc = ptz->dbc;
  packet->eoh1 = 2;
  packet->fmt = ptz->format;

  if ( nevents == 0 && ptz->mode == IEC61883_MODE_BLOCKING_NODATA ) {
    /* FDF code for packets containing dummy data. */
    packet->fdf = IEC61883_FDF_NODATA;
  }
  else {
    /* FDF code for non-blocking mode and for blocking mode with empty packets. */
    packet->fdf = ptz->fdf;
  }
  
  packet->syt = htons(syt);

  ptz->dbc += nevents_dbc;

  return nevents;
}
