/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 * This file written by Dan Dennedy.
 *
 * Bits of raw1394 ARM handling borrowed from 
 * Christian Toegel's <christian.toegel@gmx.at> demos.
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
#include "cooked.h"

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <netinet/in.h>

#include <libraw1394/csr.h>

/*
 * Plug register access functions
 *
 * Please see the convenience macros defined in iec61883.h.
 */

int
iec61883_plug_get(raw1394handle_t h, nodeid_t n, nodeaddr_t a, quadlet_t *value)
{
	quadlet_t temp; /* register value */
	int result;
  
	result = iec61883_cooked_read( h, n, CSR_REGISTER_BASE + a, sizeof(quadlet_t), &temp);
	if (result >= 0)
		*value = ntohl(temp); /* endian conversion */
	return result;
}


int
iec61883_plug_set(raw1394handle_t h, nodeid_t n, nodeaddr_t a, quadlet_t value)
{
	quadlet_t compare, swap, new;
	int result;
	
	/* get the current register value for comparison */
	result = iec61883_plug_get( h, n, a, &compare );
	if (result >= 0)
	{
		/* convert endian */
		compare = htonl(compare);
		swap = htonl(value);
		result = raw1394_lock( h, n, CSR_REGISTER_BASE + a, EXTCODE_COMPARE_SWAP, swap, compare, &new);
		if (new != compare)
			result = -EAGAIN;
	}
	return result;
}


/* 
 * Local host plugs implementation
 *
 * This requires the address range mapping feature of libraw1394 1.0.
 */

/* ARM context identifiers */
static char g_arm_callback_context_out[] = "libiec61883 output context";
static char g_arm_callback_context_in[] = "libiec61883 input context";
static struct raw1394_arm_reqhandle g_arm_reqhandle_out;
static struct raw1394_arm_reqhandle g_arm_reqhandle_in;

/* register space */

/* Please note that this is host global. Each port (host adapter)
   does not get its own register space. Also, this is only intended
   for use by a single application. It appears that when multiple processes
   host these plugs, it still works; as long as the application uses the
   plug access functions above, then it should work. */
static struct output_registers {
	struct iec61883_oMPR mpr;
	struct iec61883_oPCR pcr[IEC61883_PCR_MAX];
} g_data_out;

static struct input_registers {
	struct iec61883_iMPR mpr;
	struct iec61883_iPCR pcr[IEC61883_PCR_MAX];
} g_data_in;


/** Send an async packet in response to a register read.
 *
 *  This function handles host to bus endian conversion.
 *
 * \param handle  A raw1394 handle.
 * \param arm_req A pointer to an arm_request struct from the ARM callback
 *                handler.
 * \param a       The CSR offset address of the register.
 * \param data    The base address of the register space to read.
 * \return        0 for success, -1 on error.
 */
static int
do_arm_read(raw1394handle_t handle, struct raw1394_arm_request *arm_req, 
		nodeaddr_t a, quadlet_t *data)
{
	quadlet_t *response;
	int num=4, offset;
	
	/* allocate response packet */
	response = malloc(num * sizeof(quadlet_t));
	if (!response)
		FAIL("unable to allocate response packet");
	memset(response, 0x00, num * sizeof(quadlet_t));
	
	/* fill data of response */
	response[0] = 
		((arm_req->source_nodeid & 0xFFFF) << 16) +
		((arm_req->tlabel        & 0x3F)   << 10) +
		(6 << 4); /* tcode = 6 */
	response[1] = ((arm_req->destination_nodeid & 0xFFFF) << 16);
		/* rcode = resp_complete implied */
	
	DEBUG ("      destination_offset=%d", 
		arm_req->destination_offset - CSR_REGISTER_BASE - a);
	offset = (arm_req->destination_offset - CSR_REGISTER_BASE - a)/4;
	response[3] = htonl(data[offset]);
	
	DEBUG("      response: 0x%8.8X",response[0]);
	DEBUG("                0x%8.8X",response[1]);
	DEBUG("                0x%8.8X",response[2]);
	DEBUG("                0x%8.8X",response[3]);

	/* send response */
	raw1394_start_async_send(handle, 16 , 16, 0, response, 0);

	free (response);
	return 0;
}


/** Update a local register value, and send a response packet.
 *
 *  This function performs a compare/swap lock operation only.
 *  This function handles host to bus endian conversion.
 *
 * \param handle  A raw1394 handle.
 * \param arm_req A pointer to an arm_request struct from the ARM callback
 *                handler.
 * \param a       The CSR offset address of the register.
 * \param data    The base address of the register space to update.
 * \return        0 for success, -1 on error.
 */
static int
do_arm_lock(raw1394handle_t handle, struct raw1394_arm_request *arm_req,
		nodeaddr_t a, quadlet_t *data)
{
	quadlet_t *response = NULL;
	int num, offset;
	int rcode = RCODE_COMPLETE;
	int requested_length = 4;
			
	if (arm_req->extended_transaction_code == EXTCODE_COMPARE_SWAP)
	{
		quadlet_t arg_q, data_q, old_q, new_q;
		
		/* allocate response packet */
		num = 4 + requested_length;
		response = malloc(num * sizeof(quadlet_t));
		if (!response)
			FAIL("unable to allocate response packet");
		memset(response, 0x00, num * sizeof(quadlet_t));
		
		/* load data */
		offset = (arm_req->destination_offset - CSR_REGISTER_BASE - a)/4;
		response[4] = htonl(data[offset]);
		
		/* compare */
		arg_q  = *(quadlet_t *) (&arm_req->buffer[0]);
		data_q = *(quadlet_t *) (&arm_req->buffer[4]);
		old_q = *(quadlet_t *) (&response[4]);
		new_q = (old_q == arg_q) ? data_q : old_q;

		/* swap */
		data[offset] = ntohl(new_q);
		
	}
	else
	{
		rcode = RCODE_TYPE_ERROR;
		requested_length = 0;
	}

	/* fill data of response */
	response[0] = 
		((arm_req->source_nodeid & 0xFFFF) << 16) +
		((arm_req->tlabel        & 0x3F)   << 10) +
		(0xB << 4); /* tcode = B */
	response[1] = 
		((arm_req->destination_nodeid & 0xFFFF) << 16) +
		((rcode & 0xF) << 12);
	response[3] = 
		((requested_length & 0xFFFF) << 16) +
		(arm_req->extended_transaction_code & 0xFF);

	DEBUG("      response: 0x%8.8X",response[0]);
	DEBUG("                0x%8.8X",response[1]);
	DEBUG("                0x%8.8X",response[2]);
	DEBUG("                0x%8.8X",response[3]);
	DEBUG("                0x%8.8X",response[4]);

	/* send response */
	raw1394_start_async_send(handle, requested_length + 16, 16, 0, response, 0);

	free (response);
	return 0;
}


/* local plug ARM handler */
static int
iec61883_arm_callback (raw1394handle_t handle, 
	struct raw1394_arm_request_response *arm_req_resp,
	unsigned int requested_length,
	void *pcontext, byte_t request_type)
{
	struct raw1394_arm_request  *arm_req  = arm_req_resp->request;
	
	DEBUG( "request type=%d tcode=%d length=%d", request_type, arm_req->tcode, requested_length);
	DEBUG( "context = %s", (char *) pcontext);
	fflush(stdout);
	
	if (pcontext == g_arm_callback_context_out && requested_length == 4)
	{
		if (request_type == RAW1394_ARM_READ && arm_req->tcode == 4)
		{
			do_arm_read( handle, arm_req, CSR_O_MPR, (quadlet_t *) &g_data_out );
		}
		else if (request_type == RAW1394_ARM_LOCK)
		{
			do_arm_lock( handle, arm_req, CSR_O_MPR, (quadlet_t *) &g_data_out );
		}
		else
		{
			/* error response */
		}
	}
	else if (pcontext == g_arm_callback_context_in && requested_length == 4)
	{
		if (request_type == RAW1394_ARM_READ)
		{
			do_arm_read( handle, arm_req, CSR_I_MPR, (quadlet_t *) &g_data_in  );
		}
		else if (request_type == RAW1394_ARM_LOCK)
		{
			do_arm_lock( handle, arm_req, CSR_I_MPR, (quadlet_t *) &g_data_in );
		}
		else
		{
			/* error response */
		}
	}
	else
	{
		/* error response */
	}
	fflush(stdout);
	return 0;
}


int
iec61883_plug_impr_init (raw1394handle_t h, unsigned int data_rate)
{
	/* validate parameters */
	if (data_rate >> 2 != 0)
		errno = -EINVAL;
	
	/* initialize data */
	memset (&g_data_in, 0, sizeof (g_data_in));
	g_data_in.mpr.data_rate = data_rate;

	/* initialize host environment */
	memset (&g_arm_reqhandle_in, 0, sizeof(g_arm_reqhandle_in));
	g_arm_reqhandle_in.arm_callback = (arm_req_callback_t) iec61883_arm_callback;
	g_arm_reqhandle_in.pcontext = g_arm_callback_context_in;

	/* register callback */
	return raw1394_arm_register( h, CSR_REGISTER_BASE + CSR_I_MPR, sizeof(g_data_in),
		(byte_t *) &g_data_in, (unsigned long) &g_arm_reqhandle_in, 
		0, 0, ( RAW1394_ARM_READ | RAW1394_ARM_LOCK ) );
}


void
iec61883_plug_impr_clear (raw1394handle_t h)
{
	g_data_in.mpr.n_plugs = 0;
}


int
iec61883_plug_impr_close (raw1394handle_t h)
{
	g_data_in.mpr.n_plugs = 0;
	return raw1394_arm_unregister(h, CSR_REGISTER_BASE + CSR_I_MPR);
}


int
iec61883_plug_ipcr_add (raw1394handle_t h, unsigned int online)
{
	int i = g_data_in.mpr.n_plugs;
	
	/* validate parameters */
	if (g_arm_reqhandle_in.arm_callback == NULL)
		return -EPERM;
	if (i + 1 > IEC61883_PCR_MAX)
		return -ENOSPC;
	if (online >> 1 != 0)
		return -EINVAL;
	
	/* update data */
	g_data_in.pcr[i].online = online;
	g_data_in.mpr.n_plugs++;
	
	/* return which plug is added */
	return i;
}


int
iec61883_plug_ompr_init (raw1394handle_t h, unsigned int data_rate,
		unsigned int bcast_channel)
{
	/* validate parameters */
	if (data_rate >> 2 != 0)
		errno = -EINVAL;
	if (bcast_channel >> 6 != 0)
		errno = -EINVAL;
	
	/* initialize data */
	memset (&g_data_out, 0, sizeof (g_data_out));
	g_data_out.mpr.data_rate = data_rate;
	g_data_out.mpr.bcast_channel = bcast_channel;

	/* initialize host environment */
	memset (&g_arm_reqhandle_out, 0, sizeof(g_arm_reqhandle_out));
	g_arm_reqhandle_out.arm_callback = (arm_req_callback_t) iec61883_arm_callback;
	g_arm_reqhandle_out.pcontext = g_arm_callback_context_out;

	/* register callback */
	return raw1394_arm_register( h, CSR_REGISTER_BASE + CSR_O_MPR, sizeof(g_data_out),
		(byte_t *) &g_data_out, (unsigned long) &g_arm_reqhandle_out, 
		0, 0, ( RAW1394_ARM_READ | RAW1394_ARM_LOCK ) );
}


void
iec61883_plug_ompr_clear (raw1394handle_t h)
{
	g_data_out.mpr.n_plugs = 0;
}


int
iec61883_plug_ompr_close (raw1394handle_t h)
{
	g_data_out.mpr.n_plugs = 0;
	return raw1394_arm_unregister(h, CSR_REGISTER_BASE + CSR_O_MPR);
}


int
iec61883_plug_opcr_add (raw1394handle_t h, unsigned int online,
		unsigned int overhead_id, unsigned int payload)
{
	int i = g_data_out.mpr.n_plugs;
	
	/* validate parameters */
	if (g_arm_reqhandle_out.arm_callback == NULL)
		return -EPERM;
	if (i + 1 > IEC61883_PCR_MAX)
		return -ENOSPC;
	if (online >> 1 != 0)
		return -EINVAL;
	if (overhead_id >> 4 != 0)
		return -EINVAL;
	if (payload >> 10 != 0)
		return -EINVAL;
	
	/* update plug */
	g_data_out.pcr[i].online = online;
	g_data_out.pcr[i].overhead_id = overhead_id;
	g_data_out.pcr[i].payload = payload;
	
	/* increment the number of plugs available */
	g_data_out.mpr.n_plugs++;
	
	/* return which plug is added */
	return i;
}
