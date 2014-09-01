/*
 * avc1394_simple.c - Linux IEEE-1394 Subsystem AV/C routines
 * These routines are very basic. They can only be used for sending simple
 * commands to AV/C equipment. No control of Input and Output Plugs, etc. is
 * provided.
 * Written 8.12.1999 - 22.5.2000 by Andreas Micklei
 * 14.1.2000: added block operations
 * 6.4.2000: adapted to new fcp handling for libraw1394 0.6
 *           avc1394_transaction() and avc1394_transaction_block() are much cleaner
 *           now thanks to the new fcp handling. get_avc_response() and
 *           get_avc_response_block() are broken at the moment and will
 *           probably bee removed.
 * 22.5.2000: fixed block transactions
 *            added lots of defines and some new convenience functions for
 *            special operations like AV/C descriptor processing
 * 4.10.2000: switched to cooked functions from raw1394util
 * Modifications by Dan Dennedy <dan@dennedy.org>
 * 22.1.2001: remove debug code and delay parameters for inclusion in Kino
 * 07.05.2001: libtool-ized 
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

#include "avc1394.h"
#include "avc1394_internal.h"
#include "../common/raw1394util.h"

/* For select() */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <time.h>

#include <string.h>
#include <netinet/in.h>

#include <stdio.h>	//DEBUG

extern unsigned char g_fcp_response[];
extern unsigned int g_fcp_response_length;

int avc1394_send_command(raw1394handle_t handle, nodeid_t node, quadlet_t command)
{
	quadlet_t cmd = htonl(command);

	return cooked1394_write(handle, 0xffc0 | node, FCP_COMMAND_ADDR, sizeof(quadlet_t), &cmd);
}

int avc1394_send_command_block(raw1394handle_t handle, nodeid_t node,
                           quadlet_t *command, int command_len)
{
	quadlet_t cmd[command_len];
	int i;
	
	for (i=0; i < command_len; i++) {
		cmd[i] = ntohl(command[i]);
	}

#ifdef DEBUG
	fprintf(stderr, "avc1394_send_command_block: ");
	for (i=0; i < command_len; i++)
		fprintf(stderr, " 0x%08X", htonl(command[i]));
	fprintf(stderr, " (%s)\n", decode_ctype(command[0]));
#endif
	return cooked1394_write(handle, 0xffc0 | node, FCP_COMMAND_ADDR,
	                        command_len * sizeof(quadlet_t), cmd);
}


/*
 * Send an AV/C request to a device, wait for the corresponding AV/C
 * response and return that. This version only uses quadlet transactions.
 * IN:		handle:		the libraw1394 handle
 *		node:		the phyisical ID of the node
 *		quadlet: 	the FCP request to send
 *		retry:		retry sending the request this many times
 * RETURNS:	the AV/C response if everything went well, -1 in case of an
 * 		error
 */
quadlet_t avc1394_transaction(raw1394handle_t handle, nodeid_t node,
                          quadlet_t quadlet, int retry)
{
	quadlet_t response = 0;
	struct pollfd raw1394_poll;
	raw1394_poll.fd = raw1394_get_fd(handle);
	raw1394_poll.events = POLLIN;
	
	init_avc_response_handler(handle);
	
	do {
		if (avc1394_send_command(handle, node, quadlet) < 0) {
			struct timespec ts = {0, AVC1394_SLEEP};
			fprintf(stderr,"send oops\n");
			nanosleep(&ts, NULL);
			continue;
		}
	
		if ( poll( &raw1394_poll, 1, AVC1394_POLL_TIMEOUT) > 0 ) {
			if (raw1394_poll.revents & POLLIN) {
				raw1394_loop_iterate(handle);
				response = ntohl(*((quadlet_t *)g_fcp_response));
			}
		}
		if (response != 0) {
			while (AVC1394_MASK_RESPONSE(response) == AVC1394_RESPONSE_INTERIM) {
#ifdef DEBUG
				fprintf(stderr,"INTERIM\n");
#endif
				if ( poll( &raw1394_poll, 1, AVC1394_POLL_TIMEOUT) > 0 ) {
					if (raw1394_poll.revents & POLLIN) {
						raw1394_loop_iterate(handle);
						response = ntohl(*((quadlet_t *)g_fcp_response));
					}
				}
			}
		}
#ifdef DEBUG
		if (response != 0)
			fprintf(stderr, "avc1394_transaction: Got AVC response 0x%0x (%s)\n", response, decode_response(response));
		else
			fprintf(stderr, "avc1394_transaction: no response\n");
#endif
		
		if (response != 0) {
			stop_avc_response_handler(handle);
			return response;
		}

	} while (--retry >= 0);
	
	stop_avc_response_handler(handle);
	
	return (response == 0 ? -1 : response);
}

/*
 * Send an AV/C request to a device, wait for the corresponding AV/C
 * response and return that. This version uses block transactions.
 * IN:		handle:		the libraw1394 handle
 *		node:		the phyisical ID of the node
 *		buf:	 	the FCP request to send
 *		len:		the length of the FCP request
 *		retry:		retry sending the request this many times
 * RETURNS:	the AV/C response if everything went well, NULL in case of an
 * 		error. The response always has the same length as the request.
 */
quadlet_t *avc1394_transaction_block(raw1394handle_t handle, nodeid_t node,
                                 quadlet_t *buf, int len, int retry)
{
	quadlet_t *response = NULL;
	struct pollfd raw1394_poll;
	raw1394_poll.fd = raw1394_get_fd(handle);
	raw1394_poll.events = POLLIN;
	
	init_avc_response_handler(handle);
	
	do {
		if (avc1394_send_command_block(handle, node, buf, len) < 0) {
			struct timespec ts = {0, AVC1394_SLEEP};
			fprintf(stderr,"send oops\n");
			nanosleep(&ts, NULL);
			continue;
		}
	
		if ( poll( &raw1394_poll, 1, AVC1394_POLL_TIMEOUT) > 0 ) {
			if (raw1394_poll.revents & POLLIN) {
				raw1394_loop_iterate(handle);
				response = (quadlet_t *)g_fcp_response;
				ntohl_block(response, g_fcp_response_length);
			}
		}
		if (response != NULL) {
			while (AVC1394_MASK_RESPONSE(response[0]) == AVC1394_RESPONSE_INTERIM) {
#ifdef DEBUG
				fprintf(stderr,"INTERIM\n");
#endif
				if ( poll( &raw1394_poll, 1, AVC1394_POLL_TIMEOUT) > 0 ) {
					if (raw1394_poll.revents & POLLIN) {
						raw1394_loop_iterate(handle);
						response = (quadlet_t *)g_fcp_response;
						ntohl_block(response, g_fcp_response_length);
					}
				}
			}
		}

#ifdef DEBUG
		if (response != NULL) {
			int i;
			fprintf(stderr, "avc1394_transaction_block received response (retry %d): ",
				retry);
			for (i=0; i<len; i++) fprintf(stderr, " 0x%08X", response[i]);
			fprintf(stderr, " (%s)\n", decode_response(response[0]));
		} else {
			fprintf(stderr, "avc1394_transaction_block: no response\n");
		}
#endif
		
		if (response != 0) {
			stop_avc_response_handler(handle);
			return response;
		}
	} while (--retry >= 0);
	
	stop_avc_response_handler(handle);
	return NULL;
}

/*---------------------
 * HIGH-LEVEL-FUNCTIONS
 * --------------------
 */

/*
 * Open an AV/C descriptor
 */
int avc1394_open_descriptor(raw1394handle_t handle, nodeid_t node,
                        quadlet_t ctype, quadlet_t subunit,
                        unsigned char *descriptor_identifier, int len_descriptor_identifier,
                        unsigned char readwrite)
{
	//quadlet_t request[2];
	quadlet_t request[2];
	quadlet_t *response;
	unsigned char subfunction = readwrite?
		AVC1394_OPERAND_DESCRIPTOR_SUBFUNCTION_WRITE_OPEN
		:AVC1394_OPERAND_DESCRIPTOR_SUBFUNCTION_READ_OPEN;
	
#ifdef DEBUG
	{
		int i;
		fprintf(stderr, "Open descriptor: ctype: 0x%08X, subunit:0x%08X,\n     descriptor_identifier:", ctype, subunit);
		for (i=0; i<len_descriptor_identifier; i++)
			fprintf(stderr, " 0x%02X", descriptor_identifier[i]);
		fprintf(stderr,"\n");
	}
#endif

	if (len_descriptor_identifier != 1)
		fprintf(stderr, "Unimplemented.\n");
	/*request[0] = ctype | subunit | AVC1394_COMMAND_OPEN_DESCRIPTOR
		| ((*descriptor_identifier & 0xFF00) >> 16);
	request[1] = ((*descriptor_identifier & 0xFF) << 24) | subfunction;*/

	request[0] = ctype | subunit | AVC1394_COMMAND_OPEN_DESCRIPTOR
				| *descriptor_identifier;
	request[1] = subfunction << 24;
	if (ctype == AVC1394_CTYPE_STATUS)
		request[1] = 0xFF00FFFF;

	response = avc1394_transaction_block(handle, node, request, 2, AVC1394_RETRY);
	if (response == NULL)
		return -1;

#ifdef DEBUG
	fprintf(stderr, "Open descriptor response: 0x%08X.\n", *response);
#endif

	return 0;
}

/*
 * Close an AV/C descriptor
 */
int avc1394_close_descriptor(raw1394handle_t handle, nodeid_t node,
                         quadlet_t ctype, quadlet_t subunit,
                         unsigned char *descriptor_identifier, int len_descriptor_identifier)
{
	quadlet_t request[2];
	quadlet_t *response;
	unsigned char subfunction = AVC1394_OPERAND_DESCRIPTOR_SUBFUNCTION_CLOSE;

#ifdef DEBUG
	{
		int i;
		fprintf(stderr, "Close descriptor: ctype: 0x%08X, subunit:0x%08X,\n      descriptor_identifier:", ctype, subunit);
		for (i=0; i<len_descriptor_identifier; i++)
			fprintf(stderr, " 0x%02X", descriptor_identifier[i]);
		fprintf(stderr,"\n");
	}
#endif
	if (len_descriptor_identifier != 1)
		fprintf(stderr, "Unimplemented.\n");
	/*request[0] = ctype | subunit | AVC1394_COMMAND_OPEN_DESCRIPTOR
		| ((*descriptor_identifier & 0xFF00) >> 16);
	request[1] = ((*descriptor_identifier & 0xFF) << 24) | subfunction;*/

	request[0] = ctype | subunit | AVC1394_COMMAND_OPEN_DESCRIPTOR
	             | *descriptor_identifier;
	request[1] = subfunction << 24;

	response = avc1394_transaction_block(handle, node, request, 2, AVC1394_RETRY);
	if (response == NULL)
		return -1;

#ifdef DEBUG
	fprintf(stderr, "Close descriptor response: 0x%08X.\n", *response);
#endif

	return 0;
}

/*
 * Read an entire AV/C descriptor
 */
unsigned char *avc1394_read_descriptor(raw1394handle_t handle, nodeid_t node,
                                   quadlet_t subunit,
                                   unsigned char *descriptor_identifier, int len_descriptor_identifier)
{
	quadlet_t request[128];
	quadlet_t *response;
	
	if (len_descriptor_identifier != 1)
		fprintf(stderr, "Unimplemented.\n");
	
	memset(request, 0, 128*4);
	request[0] = AVC1394_CTYPE_CONTROL | subunit | AVC1394_COMMAND_READ_DESCRIPTOR
	             | *descriptor_identifier;
	request[1] = 0xFF000000;	/* read entire descriptor */
	request[2] = 0x00000000;	/* beginning from 0x0000 */
	
	response = avc1394_transaction_block(handle, node, request, 3, AVC1394_RETRY);
	if (response == NULL)
		return NULL;
	
	return (unsigned char *) response;
}

/*
 * Get subunit info
 */
#define EXTENSION_CODE 7
int avc1394_subunit_info(raw1394handle_t handle, nodeid_t node, quadlet_t *table)
{
	quadlet_t request[2];
	quadlet_t *response;
	int page;

	for (page=0; page < 8; page++) {
		request[0] = AVC1394_CTYPE_STATUS | AVC1394_SUBUNIT_TYPE_UNIT
		             | AVC1394_SUBUNIT_ID_IGNORE | AVC1394_COMMAND_SUBUNIT_INFO
		             | page << 4 | EXTENSION_CODE;
		request[1] = 0xFFFFFFFF;
		response = avc1394_transaction_block(handle, node, request, 2, AVC1394_RETRY);
		if (response == NULL)
			return -1;
		table[page] = response[1];
	}

#ifdef DEBUG
	{
		fprintf(stderr, "avc_subunit_info:");
		for (page=0; page < 8; page++) fprintf(stderr, " 0x%08X", table[page]);
		fprintf(stderr, "\n");
	}
#endif

	return 0;
}

int avc1394_check_subunit_type(raw1394handle_t handle, nodeid_t node, int subunit_type)
{
	quadlet_t table[8];
	int i, j;
	int entry;
	int id;
	
	if ( avc1394_subunit_info( handle, node, table) < 0) 
		return 0;
	for (i=0; i<8; i++) {
		for (j=3; j>=0; j--) {
			entry = (table[i] >> (j * 8)) & 0xFF;
			if (entry == 0xff) continue;
			id = entry >> 3;
			if (id == AVC1394_GET_SUBUNIT_TYPE(subunit_type))
				return 1;
		}
	}
	return 0;
}

quadlet_t *avc1394_unit_info(raw1394handle_t handle, nodeid_t node)
{

	quadlet_t request[2];
	quadlet_t *response;

	request[0] = AVC1394_CTYPE_STATUS | AVC1394_SUBUNIT_TYPE_UNIT
	             | AVC1394_SUBUNIT_ID_IGNORE | AVC1394_COMMAND_UNIT_INFO | 0xFF;
	request[1] = 0xFFFFFFFF;
	response = avc1394_transaction_block(handle, node, request, 2, AVC1394_RETRY);
	if (response == NULL)
		return NULL;

#ifdef DEBUG
	fprintf(stderr, "avc_unit_info: 0x%08X 0x%08X\n",
			response[0], response[1]);
#endif
	return response;
}

/*************** TARGET *******************************************************/

avc1394_command_handler_t g_command_handler = NULL;

int target_fcp_handler( raw1394handle_t handle, nodeid_t nodeid, int response, 
	size_t length, unsigned char *data )
{
	int result;
	struct avc1394_command_response cmd_resp;
	quadlet_t *r = (quadlet_t *) &cmd_resp;
	
	/* initialize the response from the request */
	memset(&cmd_resp, 0, sizeof(cmd_resp));
	memcpy(r, data, length);

	if ( response != 0 )
	{
		return 0;
	}
	else
	{
#ifdef DEBUG
		{
		int q, z;
		z = (length % 4) ? length + (4 - (length % 4)) : length;
		printf("----> ");
		for (q = 0; q < z / 4; q++)
		    printf("%08x ", r[q]);
		printf("(length %d)\n", length);
		printf("----> type=0x%02x subunit_type=0x%x subunit_id=0x%x opcode=0x%x operand0=0x%x\n",
			cmd_resp.status, cmd_resp.subunit_type,
			cmd_resp.subunit_id, cmd_resp.opcode, cmd_resp.operand[0]);
	    }
#endif
		result = g_command_handler( &cmd_resp );
		
		if (result == 0) 
			cmd_resp.status = AVC1394_RESP_NOT_IMPLEMENTED;
		
#ifdef DEBUG
		{
		int q, z;
		z = (length % 4) ? length + (4 - (length % 4)) : length;
		printf("<---- ");
		for (q = 0; q < z / 4; q++)
		    printf("%08x ", r[q]);
		printf("(length %d)\n", length);
		printf("<---- type=0x%02x subunit_type=0x%x subunit_id=0x%x opcode=0x%x operand0=0x%x\n",
			cmd_resp.status, cmd_resp.subunit_type,
			cmd_resp.subunit_id, cmd_resp.opcode, cmd_resp.operand[0]);
	    }
#endif
		
		return cooked1394_write(handle, 0xffc0 | nodeid, FCP_RESPONSE_ADDR, length, r);
	}
}


int
avc1394_init_target( raw1394handle_t handle, avc1394_command_handler_t cmd_handler)
{
	if (cmd_handler == NULL)
		return -1;
	g_command_handler = cmd_handler;
	if (raw1394_set_fcp_handler( handle, target_fcp_handler ) < 0)
		return -1;
	return raw1394_start_fcp_listen( handle );
}


int
avc1394_close_target( raw1394handle_t handle )
{
	return raw1394_stop_fcp_listen(handle);
}
