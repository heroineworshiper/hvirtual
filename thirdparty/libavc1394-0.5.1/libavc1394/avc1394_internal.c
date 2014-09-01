/*
 * libavc1394 - GNU/Linux IEEE 1394 AV/C Library
 *
 * Originally written by Andreas Micklei <andreas.micklei@ivistar.de>
 * Currently maintained by Dan Dennedy <dan@dennedy.org>
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

#include "avc1394_internal.h"
#include "../common/raw1394util.h"
#include <netinet/in.h>
#include <string.h>

unsigned char g_fcp_response[MAX_RESPONSE_SIZE];
unsigned int g_fcp_response_length;

void htonl_block(quadlet_t *buf, int len)
{
	int i;
	for (i=0; i<len; i++) {
		buf[i] = htonl(buf[i]);
	}
}

void ntohl_block(quadlet_t *buf, int len)
{
	int i;
	for (i=0; i<len; i++) {
		buf[i] = ntohl(buf[i]);
	}
}

/* used for debug output */
char *decode_response(quadlet_t response)
{
	quadlet_t resp = AVC1394_MASK_RESPONSE(response);
	if (resp == AVC1394_RESPONSE_NOT_IMPLEMENTED)
		return "NOT IMPLEMENTED";
	if (resp == AVC1394_RESPONSE_ACCEPTED)
		return "ACCEPTED";
	if (resp == AVC1394_RESPONSE_REJECTED)
		return "REJECTED";
	if (resp == AVC1394_RESPONSE_IN_TRANSITION)
		return "IN TRANSITION";
	if (resp == AVC1394_RESPONSE_IMPLEMENTED)
		return "IMPLEMENTED / STABLE";
	if (resp == AVC1394_RESPONSE_CHANGED)
		return "CHANGED";
	if (resp == AVC1394_RESPONSE_INTERIM)
		return "INTERIM";
	return "UNKNOWN RESPONSE";
}

/* used for debug output */
char *decode_ctype(quadlet_t command)
{
	quadlet_t resp = AVC1394_MASK_CTYPE(command);
	if (resp == AVC1394_CTYPE_CONTROL)
		return "CONTROL";
	if (resp == AVC1394_CTYPE_STATUS)
		return "STATUS";
	if (resp == AVC1394_CTYPE_SPECIFIC_INQUIRY)
		return "SPECIFIC INQUIRY";
	if (resp == AVC1394_CTYPE_NOTIFY)
		return "NOTIFY";
	if (resp == AVC1394_CTYPE_GENERAL_INQUIRY)
		return "GENERAL INQUIRY";
	return "UNKOWN CTYPE";
}

int avc_fcp_handler(raw1394handle_t handle, nodeid_t nodeid, int response,
					size_t length, unsigned char *data)
{
	if ( response && length > 3 )
	{
		if ( *((quadlet_t*)data) != 0 )
			g_fcp_response_length = (length + sizeof(quadlet_t) - 1) / sizeof(quadlet_t);
		else
			g_fcp_response_length = 0;
		memcpy(g_fcp_response, data, length);
	}

	return 0;
}

void init_avc_response_handler(raw1394handle_t handle)
{
	memset(g_fcp_response, 0, MAX_RESPONSE_SIZE);
	raw1394_set_fcp_handler(handle, avc_fcp_handler);
	raw1394_start_fcp_listen(handle);
}

void stop_avc_response_handler(raw1394handle_t handle)
{
	raw1394_stop_fcp_listen(handle);
}
