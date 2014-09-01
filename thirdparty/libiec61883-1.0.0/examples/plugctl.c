/* plugctl.c
 * A program to get or set any MPR or PCR register value.
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
#include <strings.h>
#include <errno.h>

/* linux1394 includes */
#include <libraw1394/raw1394.h>

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

static void usage()
{
	printf("plugctl: [-p port}] [-n node] <plug>.<attribute>[=<value>]\n"); 
	printf("The equal sign determines whether you want to get or set the value\n"); 
	printf("of a plug's attribute.\n"); 
	printf("<plug> is one of oMPR, iMPR, oPCR[n], or iPCR[n] (case insensitive).\n");
	printf("Please supply a numerical index for [n]!\n");
}

int main(int argc, const char** argv)
{
    raw1394handle_t handle;
	struct iec61883_oMPR o_mpr;
	struct iec61883_oPCR o_pcr;
	struct iec61883_iMPR i_mpr;
	struct iec61883_iPCR i_pcr;
	int result = 0;
	int port = 0;
	int node = 0xffc0;
	int is_got_ompr = 0, is_got_opcr[32], is_got_impr = 0, is_got_ipcr[32];
	int is_set_ompr = 0, is_set_opcr[32], is_set_impr = 0, is_set_ipcr[32];
	int i;
	
	if (argc == 1) {
		usage();
		return 0;
	}
		
	for (i = 0; i < 32; i++) {
		is_got_opcr[i] = is_got_ipcr[i] = 0;
		is_set_opcr[i] = is_set_ipcr[i] = 0;
	}
        
	for (i = 1; i < argc; i++)
	{
		if (strcmp (argv[i], "-p") == 0)
			port = atoi (argv[++i]);
	}

	if (!(handle = raw1394_new_handle_on_port(port))) {
		perror("raw1394 - couldn't get handle");
		printf("This error usually means that the raw1394 driver is not loaded or that /dev/raw1394 does not exist.\n");
		return EFAULT;
	}
	
	for (i = 1; i < argc && result == 0; i++)
	{
		if (strcmp (argv[i], "-n") == 0) {
			node |= atoi (argv[++i]);
		} else if (strncmp (argv[i], "-h", 2) == 0) {
			usage();
			return 0;
			
		} else {
			char *plug = strdup (argv[i]);
			char *attribute = strchr (plug, '.');
			
			if (attribute != NULL) {
				char *valuestr = strchr (attribute, '=');
				*attribute++  = '\0';
				
				if (valuestr != NULL) {
					/* set */
					int value;
					*valuestr++ = '\0';
					value = atoi (valuestr);
					
					if (strcasecmp (plug, "ompr") == 0) {
						if (!is_got_ompr) {
							result = iec61883_get_oMPR (handle, node, &o_mpr);
							if (result < 0)
								continue;
							is_got_ompr = 1;
						}
						is_set_ompr = 1;
						if (strcasecmp (attribute, "data_rate") == 0) {
							o_mpr.data_rate = value;
						} else if (strcasecmp (attribute, "bcast_channel") == 0) {
							o_mpr.bcast_channel = value;
						} else if (strcasecmp (attribute, "non_persist_ext") == 0) {
							o_mpr.non_persist_ext = value;
						} else if (strcasecmp (attribute, "persist_ext") == 0) {
							o_mpr.persist_ext = value;
						} else if (strcasecmp (attribute, "reserved") == 0) {
							o_mpr.reserved = value;
						} else if (strcasecmp (attribute, "n_plugs") == 0) {
							o_mpr.n_plugs = value;
						} else {
							result = EINVAL;
							continue;
						}
						
					} else if (strcasecmp (plug, "impr") == 0) {
						if (!is_got_impr) {
							result = iec61883_get_iMPR (handle, node, &i_mpr);
							if (result < 0)
								continue;
							is_got_impr = 1;
						}
						is_set_impr = 1;
						if (strcasecmp (attribute, "data_rate") == 0) {
							i_mpr.data_rate = value;
						} else if (strcasecmp (attribute, "reserved") == 0) {
							i_mpr.reserved = value;
						} else if (strcasecmp (attribute, "non_persist_ext") == 0) {
							i_mpr.non_persist_ext = value;
						} else if (strcasecmp (attribute, "persist_ext") == 0) {
							i_mpr.persist_ext = value;
						} else if (strcasecmp (attribute, "reserved2") == 0) {
							i_mpr.reserved2 = value;
						} else if (strcasecmp (attribute, "n_plugs") == 0) {
							i_mpr.n_plugs = value;
						} else {
							result = EINVAL;
							continue;
						}
						
					} else if (strncasecmp (plug, "opcr[", 5) == 0) {
						int idx = atoi (plug + 5);
						if (!is_got_opcr[idx]) {
							result = iec61883_get_oPCRX (handle, node, &o_pcr, idx);
							if (result < 0)
								continue;
							is_got_opcr[idx];
						}
						is_set_opcr[idx] = 1;
						if (strcasecmp (attribute, "online") == 0) {
							o_pcr.online = value;
						} else if (strcasecmp (attribute, "bcast_connection") == 0) {
							o_pcr.bcast_connection = value;
						} else if (strcasecmp (attribute, "n_p2p_connections") == 0) {
							o_pcr.n_p2p_connections = value;
						} else if (strcasecmp (attribute, "reserved") == 0) {
							o_pcr.reserved = value;
						} else if (strcasecmp (attribute, "channel") == 0) {
							o_pcr.channel = value;
						} else if (strcasecmp (attribute, "data_rate") == 0) {
							o_pcr.data_rate = value;
						} else if (strcasecmp (attribute, "overhead_id") == 0) {
							o_pcr.overhead_id = value;
						} else if (strcasecmp (attribute, "payload") == 0) {
							o_pcr.payload = value;
						} else {
							result = EINVAL;
							continue;
						}
						
					} else if (strncasecmp (plug, "ipcr[", 5) == 0) {
						int idx = atoi (plug + 5);
						if (!is_got_ipcr[idx]) {
							result = iec61883_get_iPCRX (handle, node, &i_pcr, idx);
							if (result < 0)
								continue;
							is_got_ipcr[idx];
						}
						is_set_ipcr[idx] = 1;
						if (strcasecmp (attribute, "online") == 0) {
							i_pcr.online = value;
						} else if (strcasecmp (attribute, "bcast_connection") == 0) {
							i_pcr.bcast_connection = value;
						} else if (strcasecmp (attribute, "n_p2p_connections") == 0) {
							i_pcr.n_p2p_connections = value;
						} else if (strcasecmp (attribute, "reserved") == 0) {
							i_pcr.reserved = value;
						} else if (strcasecmp (attribute, "channel") == 0) {
							i_pcr.channel = value;
						} else if (strcasecmp (attribute, "reserved2") == 0) {
							i_pcr.reserved2 = value;
						} else {
							result = EINVAL;
							continue;
						}
						
					} else {
						result = EINVAL;
						continue;
					}
				} else {
					/* get */
					if (strcasecmp (plug, "ompr") == 0) {
						result = iec61883_get_oMPR (handle, node, &o_mpr);
						if (result < 0)
							continue;
						if (strcasecmp (attribute, "data_rate") == 0) {
							printf ("%d\n", o_mpr.data_rate);
						} else if (strcasecmp (attribute, "bcast_channel") == 0) {
							printf ("%d\n", o_mpr.bcast_channel);
						} else if (strcasecmp (attribute, "non_persist_ext") == 0) {
							printf ("%d\n", o_mpr.non_persist_ext);
						} else if (strcasecmp (attribute, "persist_ext") == 0) {
							printf ("%d\n", o_mpr.persist_ext);
						} else if (strcasecmp (attribute, "reserved") == 0) {
							printf ("%d\n", o_mpr.reserved);
						} else if (strcasecmp (attribute, "n_plugs") == 0) {
							printf ("%d\n", o_mpr.n_plugs);
						} else {
							result = EINVAL;
							continue;
						}
						
					} else if (strcasecmp (plug, "impr") == 0) {
						result = iec61883_get_iMPR (handle, node, &i_mpr);
						if (result < 0)
							continue;
						if (strcasecmp (attribute, "data_rate") == 0) {
							printf ("%d\n", i_mpr.data_rate);
						} else if (strcasecmp (attribute, "reserved") == 0) {
							printf ("%d\n", i_mpr.reserved);
						} else if (strcasecmp (attribute, "non_persist_ext") == 0) {
							printf ("%d\n", i_mpr.non_persist_ext);
						} else if (strcasecmp (attribute, "persist_ext") == 0) {
							printf ("%d\n", i_mpr.persist_ext);
						} else if (strcasecmp (attribute, "reserved2") == 0) {
							printf ("%d\n", i_mpr.reserved2);
						} else if (strcasecmp (attribute, "n_plugs") == 0) {
							printf ("%d\n", i_mpr.n_plugs);
						} else {
							result = EINVAL;
							continue;
						}
						
					} else if (strncasecmp (plug, "opcr[", 5) == 0) {
						int idx = atoi (plug + 5);
						result = iec61883_get_oPCRX (handle, node, &o_pcr, idx);
						if (result < 0)
							continue;
						if (strcasecmp (attribute, "online") == 0) {
							printf ("%d\n", o_pcr.online);
						} else if (strcasecmp (attribute, "bcast_connection") == 0) {
							printf ("%d\n", o_pcr.bcast_connection);
						} else if (strcasecmp (attribute, "n_p2p_connections") == 0) {
							printf ("%d\n", o_pcr.n_p2p_connections);
						} else if (strcasecmp (attribute, "reserved") == 0) {
							printf ("%d\n", o_pcr.reserved);
						} else if (strcasecmp (attribute, "channel") == 0) {
							printf ("%d\n", o_pcr.channel);
						} else if (strcasecmp (attribute, "data_rate") == 0) {
							printf ("%d\n", o_pcr.data_rate);
						} else if (strcasecmp (attribute, "overhead_id") == 0) {
							printf ("%d\n", o_pcr.overhead_id);
						} else if (strcasecmp (attribute, "payload") == 0) {
							printf ("%d\n", o_pcr.payload);
						} else {
							result = EINVAL;
							continue;
						}
						
					} else if (strncasecmp (plug, "ipcr[", 5) == 0) {
						int idx = atoi (plug + 5);
						result = iec61883_get_iPCRX (handle, node, &i_pcr, idx);
						if (result < 0)
							continue;
						if (strcasecmp (attribute, "online") == 0) {
							printf ("%d\n", i_pcr.online);
						} else if (strcasecmp (attribute, "bcast_connection") == 0) {
							printf ("%d\n", i_pcr.bcast_connection);
						} else if (strcasecmp (attribute, "n_p2p_connections") == 0) {
							printf ("%d\n", i_pcr.n_p2p_connections);
						} else if (strcasecmp (attribute, "reserved") == 0) {
							printf ("%d\n", i_pcr.reserved);
						} else if (strcasecmp (attribute, "channel") == 0) {
							printf ("%d\n", i_pcr.channel);
						} else if (strcasecmp (attribute, "reserved2") == 0) {
							printf ("%d\n", i_pcr.reserved2);
						} else {
							result = EINVAL;
							continue;
						}
						
					} else {
						result = EINVAL;
						continue;
					}
				}
			}
			free (plug);
		}
	}
	if (result == 0 && is_set_ompr)
		result = iec61883_set_oMPR (handle, node, o_mpr);
	if (result == 0 && is_set_impr)
		result = iec61883_set_iMPR (handle, node, i_mpr);
	for (i = 0; result == 0 && i < 32; i++) {
		if (is_set_opcr[i])
			result = iec61883_set_oPCRX (handle, node, o_pcr, i);
		if (is_set_ipcr[i])
			result = iec61883_set_iPCRX (handle, node, i_pcr, i);
	}
	raw1394_destroy_handle(handle);
	
	return result;
}
