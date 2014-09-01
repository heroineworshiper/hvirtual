/*
 * iec61883-private.h - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#ifndef _IEC61883_PRIVATE_H
#define _IEC61883_PRIVATE_H

#include <libraw1394/raw1394.h>
#include <endian.h>
#include "tsbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IEC61883_DEBUG
#define DEBUG(s, args...) fprintf(stderr, "libiec61883 debug: " s "\n", ## args)
#else
#define DEBUG(s, args...)
#endif
#define WARN(s, args...) {fprintf(stderr, "libiec61883 warning: " s "\n", ## args);}
#define FAIL(s, args...) {fprintf(stderr, "libiec61883 error: " s "\n", ## args);return(-1);}

/*
 * The TAG value is present in the isochronous header (first quadlet). It
 * provides a high level label for the format of data carried by the
 * isochronous packet.
 */

#define IEC61883_TAG_WITHOUT_CIP 0 /* CIP header NOT included */
#define IEC61883_TAG_WITH_CIP    1 /* CIP header included. */
#define IEC61883_TAG_RESERVED1   2 /* Reserved */
#define IEC61883_TAG_RESERVED2   3 /* Reserved */

/**
 * Common Isochronous Protocol
 **/

#define IEC61883_FMT_DV 0x00
#define IEC61883_FMT_AMDTP 0x10
#define IEC61883_FMT_MPEG2 0x20

#if __BYTE_ORDER == __BIG_ENDIAN

struct iso_packet_header {
	unsigned int data_length : 16;
	unsigned int tag         : 2;
	unsigned int channel     : 6;
	unsigned int tcode       : 4;
	unsigned int sy          : 4;
};

struct iec61883_packet {
	/* First quadlet */
	unsigned int dbs      : 8;
	unsigned int eoh0     : 2;
	unsigned int sid      : 6;

	unsigned int dbc      : 8;
	unsigned int fn       : 2;
	unsigned int qpc      : 3;
	unsigned int sph      : 1;
	unsigned int reserved : 2;

	/* Second quadlet */
	unsigned int fdf      : 8;
	unsigned int eoh1     : 2;
	unsigned int fmt      : 6;

	unsigned int syt      : 16;

	unsigned char data[0];
};

#elif __BYTE_ORDER == __LITTLE_ENDIAN

struct iso_packet_header {
	unsigned int data_length : 16;
	unsigned int channel     : 6;
	unsigned int tag         : 2;
	unsigned int sy          : 4;
	unsigned int tcode       : 4;
};

struct iec61883_packet {
	/* First quadlet */
	unsigned int sid      : 6;
	unsigned int eoh0     : 2;
	unsigned int dbs      : 8;

	unsigned int reserved : 2;
	unsigned int sph      : 1;
	unsigned int qpc      : 3;
	unsigned int fn       : 2;
	unsigned int dbc      : 8;

	/* Second quadlet */
	unsigned int fmt      : 6;
	unsigned int eoh1     : 2;
	unsigned int fdf      : 8;

	unsigned int syt      : 16;

	unsigned char data[0];
};

#else

#error Unknown bitfield type

#endif

struct iec61883_fraction {
	int integer;
	int numerator;
	int denominator;
};

struct iec61883_cip {
	struct iec61883_fraction cycle_offset;
	struct iec61883_fraction ticks_per_syt_offset;
	struct iec61883_fraction ready_samples;
	struct iec61883_fraction samples_per_cycle;
	int dbc, dbs;
	int cycle_count;
	int cycle_count2;
	int mode;
	int syt_interval;
	int dimension;
	int rate;
	int fdf;
	int format;
};

void
iec61883_cip_init(struct iec61883_cip *cip, int format, int fdf,
		int rate, int dbs, int syt_interval);
void 
iec61883_cip_set_transmission_mode(struct iec61883_cip *ptz, int mode);

int 
iec61883_cip_get_max_packet_size(struct iec61883_cip *ptz);

int
iec61883_cip_fill_header(raw1394handle_t handle, struct iec61883_cip *cip,
		struct iec61883_packet *packet);

		
/**
 * Audio and Music Data Transport Protocol 
 **/

#define IEC61883_FDF_NODATA   0xFF
		
/* AM824 format definitions. */
#define IEC61883_FDF_AM824 0x00
#define IEC61883_FDF_AM824_CONTROLLED 0x04
#define IEC61883_FDF_SFC_MASK 0x03

#define IEC61883_AM824_LABEL              0x40
#define IEC61883_AM824_LABEL_RAW_24BITS   0x40
#define IEC61883_AM824_LABEL_RAW_20BITS   0x41
#define IEC61883_AM824_LABEL_RAW_16BITS   0x42
#define IEC61883_AM824_LABEL_RAW_RESERVED 0x43

#define IEC61883_AM824_VBL_24BITS   0x0
#define IEC61883_AM824_VBL_20BITS   0x1
#define IEC61883_AM824_VBL_16BITS   0x2
#define IEC61883_AM824_VBL_RESERVED 0x3

/* IEC-60958 format definitions. */
#define IEC60958_LABEL   0x0
#define IEC60958_PAC_B   0x3 /* Preamble Code 'B': Start of channel 1, at
			      * the start of a data block. */
#define IEC60958_PAC_RSV 0x2 /* Preamble Code 'RESERVED' */
#define IEC60958_PAC_M   0x1 /* Preamble Code 'M': Start of channel 1 that
			      *	is not at the start of a data block. */
#define IEC60958_PAC_W   0x0 /* Preamble Code 'W': start of channel 2. */
#define IEC60958_DATA_VALID   0 /* When cleared means data is valid. */
#define IEC60958_DATA_INVALID 1 /* When set means data is not suitable for an ADC. */

#if __BYTE_ORDER == __BIG_ENDIAN

struct iec60958_data {
	u_int32_t data      : 24;
	u_int32_t validity  : 1;
	u_int32_t user_data : 1;
	u_int32_t ch_status : 1;
	u_int32_t parity    : 1;
	u_int32_t pac       : 2;
	u_int32_t label     : 2;
};

#elif __BYTE_ORDER == __LITTLE_ENDIAN

struct iec60958_data {
	u_int32_t data      : 24;
	u_int32_t validity  : 1;
	u_int32_t user_data : 1;
	u_int32_t ch_status : 1;
	u_int32_t parity    : 1;
	u_int32_t pac       : 2;
	u_int32_t label     : 2;
};

#else

#error Unknown bitfield type

#endif

struct iec61883_amdtp {
	struct iec61883_cip cip;
	int dimension;
	int rate;
	int iec958_rate_code;
	int sample_format;
	iec61883_amdtp_recv_t put_data;
	iec61883_amdtp_xmit_t get_data;
	void *callback_data;
	int format;
	int syt_interval;
	raw1394handle_t handle;
	int channel;
	unsigned int buffer_packets;
	unsigned int prebuffer_packets;
	unsigned int irq_interval;
	int synch;
	int speed;
	unsigned int total_dropped;
};


/**
 * DV Digital Video
 **/

struct iec61883_dv {
	struct iec61883_cip cip;
	iec61883_dv_recv_t put_data;
	iec61883_dv_xmit_t get_data;
	void *callback_data;
	raw1394handle_t handle;
	int channel;
	unsigned int buffer_packets;
	unsigned int prebuffer_packets;
	unsigned int irq_interval;
	int synch;
	int speed;
	unsigned int total_dropped;
};

struct iec61883_dv_fb {
	iec61883_dv_t dv;
	unsigned char data[480*300];
	int len;
	iec61883_dv_fb_recv_t put_data;
	void *callback_data;
	int ff;
	unsigned int total_incomplete;
};


/**
 * MPEG-2 Transport Stream
 **/

struct iec61883_mpeg2 {
	struct iec61883_cip cip;
	iec61883_mpeg2_recv_t put_data;
	iec61883_mpeg2_xmit_t get_data;
	void *callback_data;
	int syt_interval;
	raw1394handle_t handle;
	int channel;
	struct tsbuffer *tsbuffer;
	unsigned int buffer_packets;
	unsigned int prebuffer_packets;
	unsigned int irq_interval;
	int synch;
	int speed;
	unsigned int total_dropped;
};


/**
 * Plug Control Registers
 **/

/* maximum number of PCRs allowed within the standard 
 * MPR/PCR addresses defined in IEC-61883.
 * This refers to the number of output or input PCRs--
 * not the MPRs and not the combined total.
 */
#define IEC61883_PCR_MAX 31

/* standard CSR offsets for plugs */
#define CSR_O_MPR   0x900
#define CSR_O_PCR_0 0x904

#define CSR_I_MPR   0x980
#define CSR_I_PCR_0 0x984

#if ( __BYTE_ORDER == __BIG_ENDIAN )

struct iec61883_oMPR {
	unsigned int data_rate:2;
	unsigned int bcast_channel:6;
	unsigned int non_persist_ext:8;
	unsigned int persist_ext:8;
	unsigned int reserved:3;
	unsigned int n_plugs:5;
};

struct iec61883_iMPR {
	unsigned int data_rate:2;
	unsigned int reserved:6;
	unsigned int non_persist_ext:8;
	unsigned int persist_ext:8;
	unsigned int reserved2:3;
	unsigned int n_plugs:5;
};

struct iec61883_oPCR {
	unsigned int online:1;
	unsigned int bcast_connection:1;
	unsigned int n_p2p_connections:6;
	unsigned int reserved:2;
	unsigned int channel:6;
	unsigned int data_rate:2;
	unsigned int overhead_id:4;
	unsigned int payload:10;
};

struct iec61883_iPCR {
	unsigned int online:1;
	unsigned int bcast_connection:1;
	unsigned int n_p2p_connections:6;
	unsigned int reserved:2;
	unsigned int channel:6;
	unsigned int reserved2:16;
};

#else

struct iec61883_oMPR {
	unsigned int n_plugs:5;
	unsigned int reserved:3;
	unsigned int persist_ext:8;
	unsigned int non_persist_ext:8;
	unsigned int bcast_channel:6;
	unsigned int data_rate:2;
};

struct iec61883_iMPR {
	unsigned int n_plugs:5;
	unsigned int reserved2:3;
	unsigned int persist_ext:8;
	unsigned int non_persist_ext:8;
	unsigned int reserved:6;
	unsigned int data_rate:2;
};

struct iec61883_oPCR {
	unsigned int payload:10;
	unsigned int overhead_id:4;
	unsigned int data_rate:2;
	unsigned int channel:6;
	unsigned int reserved:2;
	unsigned int n_p2p_connections:6;
	unsigned int bcast_connection:1;
	unsigned int online:1;
};

struct iec61883_iPCR {
	unsigned int reserved2:16;
	unsigned int channel:6;
	unsigned int reserved:2;
	unsigned int n_p2p_connections:6;
	unsigned int bcast_connection:1;
	unsigned int online:1;
};

#endif

/**
 * iec61883_plug_get - Read a node's plug register.
 * @h: A raw1394 handle.
 * @n: The node id of the node to read
 * @a: The CSR offset address (relative to base) of the register to read.
 * @value: A pointer to a quadlet where the plug register's value will be stored.
 * 
 * This function handles bus to host endian conversion. It returns 0 for 
 * suceess or -1 for error (errno available).
 **/
int
iec61883_plug_get(raw1394handle_t h, nodeid_t n, nodeaddr_t a, quadlet_t *value);


/** 
 * iec61883_plug_set - Write a node's plug register.
 * @h: A raw1394 handle.
 * @n: The node id of the node to read
 * @a: The CSR offset address (relative to CSR base) of the register to write.
 * @value: A quadlet containing the new register value.
 *
 * This uses a compare/swap lock operation to safely write the
 * new register value, as required by IEC 61883-1.
 * This function handles host to bus endian conversion. It returns 0 for success
 * or -1 for error (errno available).
 **/
int
iec61883_plug_set(raw1394handle_t h, nodeid_t n, nodeaddr_t a, quadlet_t value);

/**
 * High level plug access macros
 */

#define iec61883_get_oMPR(h,n,v) iec61883_plug_get((h), (n), CSR_O_MPR, (quadlet_t *)(v))
#define iec61883_set_oMPR(h,n,v) iec61883_plug_set((h), (n), CSR_O_MPR, *((quadlet_t *)&(v)))
#define iec61883_get_oPCR0(h,n,v) iec61883_plug_get((h), (n), CSR_O_PCR_0, (quadlet_t *)(v))
#define iec61883_set_oPCR0(h,n,v) iec61883_plug_set((h), (n), CSR_O_PCR_0, *((quadlet_t *)&(v)))
#define iec61883_get_oPCRX(h,n,v,x) iec61883_plug_get((h), (n), CSR_O_PCR_0+(4*(x)), (quadlet_t *)(v))
#define iec61883_set_oPCRX(h,n,v,x) iec61883_plug_set((h), (n), CSR_O_PCR_0+(4*(x)), *((quadlet_t *)&(v)))
#define iec61883_get_iMPR(h,n,v) iec61883_plug_get((h), (n), CSR_I_MPR, (quadlet_t *)(v))
#define iec61883_set_iMPR(h,n,v) iec61883_plug_set((h), (n), CSR_I_MPR, *((quadlet_t *)&(v)))
#define iec61883_get_iPCR0(h,n,v) iec61883_plug_get((h), (n), CSR_I_PCR_0, (quadlet_t *)(v))
#define iec61883_set_iPCR0(h,n,v) iec61883_plug_set((h), (n), CSR_I_PCR_0, *((quadlet_t *)&(v)))
#define iec61883_get_iPCRX(h,n,v,x) iec61883_plug_get((h), (n), CSR_I_PCR_0+(4*(x)), (quadlet_t *)(v))
#define iec61883_set_iPCRX(h,n,v,x) iec61883_plug_set((h), (n), CSR_I_PCR_0+(4*(x)), *((quadlet_t *)&(v)))


#ifdef __cplusplus
}
#endif

#endif /* _IEC61883_PRIVATE_H */
