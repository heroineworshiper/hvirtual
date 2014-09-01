/* plugreport.c v0.2
 * A program to read all MPR/PCR registers from all devices and report them.
 * Copyright 2002-2004 Dan Dennedy
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* standard system includes */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

/* linux1394 includes */
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>

#include "../src/iec61883.h"

/************ DECLARE PRIVATE INTERFACE BITS OF LIBIEC61883!! **************/

/* standard CSR offsets for plugs */
#define CSR_O_MPR   0x900
#define CSR_O_PCR_0 0x904
#define CSR_O_PCR_1 0x908
#define CSR_O_PCR_2 0x90C
#define CSR_O_PCR_3 0x910

#define CSR_I_MPR   0x980
#define CSR_I_PCR_0 0x984
#define CSR_I_PCR_1 0x988
#define CSR_I_PCR_2 0x98C
#define CSR_I_PCR_3 0x990

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
extern int
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
extern int
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

/************ END PRIVATE INTERFACE **************/

#define FAIL(s) {fprintf(stderr, "libiec61883 error: %s\n", s);}

#define PLUGREPORT_GUID_HI 0x0C
#define PLUGREPORT_GUID_LO 0x10

static octlet_t get_guid(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t       quadlet;
	octlet_t        offset;
	octlet_t    guid = 0;
	
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + PLUGREPORT_GUID_HI;
	raw1394_read(handle, node, offset, sizeof(quadlet_t), &quadlet);
	quadlet = htonl(quadlet);
	guid = quadlet;
	guid <<= 32;
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + PLUGREPORT_GUID_LO;
	raw1394_read(handle, node, offset, sizeof(quadlet_t), &quadlet);
	quadlet = htonl(quadlet);
	guid += quadlet;
	
	return guid;
}


int main(int argc, const char** argv)
{
    raw1394handle_t handle;
	int device;
	struct iec61883_oMPR o_mpr;
	struct iec61883_oPCR o_pcr;
	struct iec61883_iMPR i_mpr;
	struct iec61883_iPCR i_pcr;
    int numcards, port, i;
    struct raw1394_portinfo pinf[16];
        
	if (!(handle = raw1394_new_handle())) {
		perror("raw1394 - couldn't get handle");
		printf("This error usually means that the raw1394 driver is not loaded or that /dev/raw1394 does not exist.\n");
		exit( -1);
	}

	if ((numcards = raw1394_get_port_info(handle, pinf, 16)) < 0) {
		perror("raw1394 - couldn't get card info");
		exit( -1);
	}

	for (port = 0; port < numcards; port++)
	{
		if (raw1394_set_port(handle, port) < 0) {
			perror("raw1394 - couldn't set port");
			exit( -1);
		}
		
		printf( "Host Adapter %d\n==============\n", port);
    
		for (device = 0; device < raw1394_get_nodecount(handle); device++ )
		{
			octlet_t guid = get_guid(handle, 0xffc0 | device);
			
			printf( "\nNode %d GUID 0x%08x%08x\n------------------------------\n",
				device, (quadlet_t) (guid>>32), (quadlet_t) (guid & 0xffffffff));
			
			if (iec61883_get_oMPR(handle, 0xffc0 | device, &o_mpr) <  0)
				FAIL("error reading oMPR")
			else
			{
				printf( "oMPR n_plugs=%d, data_rate=%d, bcast_channel=%d\n", o_mpr.n_plugs, o_mpr.data_rate, o_mpr.bcast_channel);
				for (i = 0; i < o_mpr.n_plugs; i++)
				{
					if (iec61883_get_oPCRX( handle, 0xffc0 | device, &o_pcr, i) < 0)
						FAIL("error reading oPCR")
					else
					{
						printf( "oPCR[%d] online=%d, bcast_connection=%d, n_p2p_connections=%d\n", i,
							o_pcr.online, o_pcr.bcast_connection, o_pcr.n_p2p_connections);
						printf( "\tchannel=%d, data_rate=%d, overhead_id=%d, payload=%d\n", 
							o_pcr.channel, o_pcr.data_rate, o_pcr.overhead_id, o_pcr.payload);
					}
				}
			}
	
			if (iec61883_get_iMPR(handle, 0xffc0 | device, &i_mpr) <  0)
				FAIL("error reading iMPR")
			else
			{
				printf( "iMPR n_plugs=%d, data_rate=%d\n", i_mpr.n_plugs, i_mpr.data_rate);
				for (i = 0; i < i_mpr.n_plugs; i++)
				{
					if (iec61883_get_iPCRX( handle, 0xffc0 | device, &i_pcr, i) < 0)
						FAIL("error reading iPCR")
					else
					{
						printf( "iPCR[%d] online=%d, bcast_connection=%d, n_p2p_connections=%d\n", i,
							i_pcr.online, i_pcr.bcast_connection, i_pcr.n_p2p_connections);
						printf( "\tchannel=%d\n", 
							i_pcr.channel);
					}
				}
			}
		}
		printf("\n");
		
		raw1394_destroy_handle(handle);
		
		if (!(handle = raw1394_new_handle())) {
			perror("raw1394 - couldn't get handle");
			printf("This error usually means that the raw1394 driver is not loaded or that /dev/raw1394 does not exist.\n");
			exit( -1);
		}

		if ((numcards = raw1394_get_port_info(handle, pinf, 16)) < 0) {
			perror("raw1394 - couldn't get card info");
			exit( -1);
		}

	}
	
	return 0;
}
