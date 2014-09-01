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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "avc1394_vcr.h"
#include "avc1394.h"

#define AVC1394_RETRY 2

#define CTLVCR0 AVC1394_CTYPE_CONTROL | AVC1394_SUBUNIT_TYPE_TAPE_RECORDER | AVC1394_SUBUNIT_ID_0
#define STATVCR0 AVC1394_CTYPE_STATUS | AVC1394_SUBUNIT_TYPE_TAPE_RECORDER | AVC1394_SUBUNIT_ID_0
#define CTLTUNER0 AVC1394_CTYPE_CONTROL | AVC1394_SUBUNIT_TYPE_TUNER | AVC1394_SUBUNIT_ID_0
#define STATTUNER0 AVC1394_CTYPE_STATUS | AVC1394_SUBUNIT_TYPE_TUNER | AVC1394_SUBUNIT_ID_0
#define TUNER0 AVC1394_SUBUNIT_TYPE_TUNER | AVC1394_SUBUNIT_ID_0
#define CTLUNIT AVC1394_CTYPE_CONTROL | AVC1394_SUBUNIT_TYPE_UNIT | AVC1394_SUBUNIT_ID_IGNORE
#define STATUNIT AVC1394_CTYPE_STATUS | AVC1394_SUBUNIT_TYPE_UNIT | AVC1394_SUBUNIT_ID_IGNORE

int avc1394_vcr_is_playing(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t response = avc1394_transaction(handle, node, STATVCR0
		| AVC1394_VCR_COMMAND_TRANSPORT_STATE | AVC1394_VCR_OPERAND_TRANSPORT_STATE,
		AVC1394_RETRY);
	if (AVC1394_MASK_OPCODE(response)
		== AVC1394_VCR_RESPONSE_TRANSPORT_STATE_PLAY)
		return AVC1394_GET_OPERAND0(response);
	else
		return 0;
}


int avc1394_vcr_is_recording(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t response = avc1394_transaction(handle, node, STATVCR0
		| AVC1394_VCR_COMMAND_TRANSPORT_STATE | AVC1394_VCR_OPERAND_TRANSPORT_STATE,
		AVC1394_RETRY);
	if (AVC1394_MASK_OPCODE(response)
		== AVC1394_VCR_RESPONSE_TRANSPORT_STATE_RECORD)
		return AVC1394_GET_OPERAND0(response);
	else
		return 0;
}


void avc1394_vcr_play(raw1394handle_t handle, nodeid_t node)
{
	if (avc1394_vcr_is_playing(handle, node) == AVC1394_VCR_OPERAND_PLAY_FORWARD) {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_SLOWEST_FORWARD);
	} else {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_FORWARD);
	}
}


void avc1394_vcr_reverse(raw1394handle_t handle, nodeid_t node)
{
	if (avc1394_vcr_is_playing(handle, node) == AVC1394_VCR_OPERAND_PLAY_REVERSE) {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_SLOWEST_REVERSE);
	} else {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_REVERSE);
	}
}


void avc1394_vcr_trick_play(raw1394handle_t handle, nodeid_t node, int speed)
{
	if (!avc1394_vcr_is_recording(handle, node)) {
	    if (speed == 0) {
		    avc1394_send_command(handle, node, CTLVCR0
			    | AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_FORWARD);
	    } else if (speed > 0) {
	        if (speed > 14) speed = 14;
		    avc1394_send_command(handle, node, CTLVCR0
			    | AVC1394_VCR_COMMAND_PLAY | (AVC1394_VCR_OPERAND_PLAY_NEXT_FRAME + speed));	        
	    } else {
	        if (speed < -14) speed = -14;
		    avc1394_send_command(handle, node, CTLVCR0
			    | AVC1394_VCR_COMMAND_PLAY | (AVC1394_VCR_OPERAND_PLAY_PREVIOUS_FRAME - speed));	        
	    }
	}
}


void avc1394_vcr_stop(raw1394handle_t handle, nodeid_t node)
{
	avc1394_send_command(handle, node, CTLVCR0
		| AVC1394_VCR_COMMAND_WIND | AVC1394_VCR_OPERAND_WIND_STOP);
}


void avc1394_vcr_rewind(raw1394handle_t handle, nodeid_t node)
{
	if (avc1394_vcr_is_playing(handle, node)) {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_FASTEST_REVERSE);
	} else {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_WIND | AVC1394_VCR_OPERAND_WIND_REWIND);
	}
}


void avc1394_vcr_pause(raw1394handle_t handle, nodeid_t node)
{
	int mode;
	
	if ((mode = avc1394_vcr_is_recording(handle, node))) {
		if (mode == AVC1394_VCR_OPERAND_RECORD_PAUSE) {
			avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_RECORD | AVC1394_VCR_OPERAND_RECORD_RECORD);
		} else {
			avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_RECORD | AVC1394_VCR_OPERAND_RECORD_PAUSE);
		}
	} else {
		if (avc1394_vcr_is_playing(handle, node)==AVC1394_VCR_OPERAND_PLAY_FORWARD_PAUSE) {
			avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_FORWARD);
		} else {
			avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_FORWARD_PAUSE);
		}
	}

}


void avc1394_vcr_forward(raw1394handle_t handle, nodeid_t node)
{
	if (avc1394_vcr_is_playing(handle, node)) {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_FASTEST_FORWARD);
	} else {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_WIND | AVC1394_VCR_OPERAND_WIND_FAST_FORWARD);

	}
}


void avc1394_vcr_next(raw1394handle_t handle, nodeid_t node)
{
	if (avc1394_vcr_is_playing(handle, node)) {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_NEXT_FRAME);
	} 
}

void avc1394_vcr_next_index(raw1394handle_t handle, nodeid_t node)
{
    quadlet_t request[2];
	if (avc1394_vcr_is_playing(handle, node)) {
	    request[0] = CTLVCR0 | AVC1394_VCR_COMMAND_FORWARD | 
	        AVC1394_VCR_MEASUREMENT_INDEX;
	    request[1] = 0x01FFFFFF;
		avc1394_send_command_block(handle, node, request, 2);
	} 
}

void avc1394_vcr_previous(raw1394handle_t handle, nodeid_t node)
{
	if (avc1394_vcr_is_playing(handle, node)) {
		avc1394_send_command(handle, node, CTLVCR0
			| AVC1394_VCR_COMMAND_PLAY | AVC1394_VCR_OPERAND_PLAY_PREVIOUS_FRAME);
	} 
}

void avc1394_vcr_previous_index(raw1394handle_t handle, nodeid_t node)
{
    quadlet_t request[2];
	if (avc1394_vcr_is_playing(handle, node)) {
	    request[0] = CTLVCR0 | AVC1394_VCR_COMMAND_BACKWARD | 
	        AVC1394_VCR_MEASUREMENT_INDEX;
	    request[1] = 0x01FFFFFF;
		avc1394_send_command_block(handle, node, request, 2);
	} 
}


void avc1394_vcr_eject(raw1394handle_t handle, nodeid_t node)
{
	avc1394_send_command(handle, node, CTLVCR0
		| AVC1394_VCR_COMMAND_LOAD_MEDIUM | AVC1394_VCR_OPERAND_LOAD_MEDIUM_EJECT);
}


void avc1394_vcr_record(raw1394handle_t handle, nodeid_t node)
{
	avc1394_send_command(handle, node, CTLVCR0
		| AVC1394_VCR_COMMAND_RECORD | AVC1394_VCR_OPERAND_RECORD_RECORD);
}

quadlet_t avc1394_vcr_status(raw1394handle_t handle, nodeid_t node)
{
	return avc1394_transaction(handle, node,
			STATVCR0 | AVC1394_VCR_COMMAND_TRANSPORT_STATE
			| AVC1394_VCR_OPERAND_TRANSPORT_STATE, AVC1394_RETRY);

}

char *avc1394_vcr_decode_status(quadlet_t response)
{
	/*quadlet_t resp0 = AVC1394_MASK_RESPONSE_OPERAND(response, 0);
	quadlet_t resp1 = AVC1394_MASK_RESPONSE_OPERAND(response, 1);*/
	quadlet_t resp2 = AVC1394_MASK_RESPONSE_OPERAND(response, 2);
	quadlet_t resp3 = AVC1394_MASK_RESPONSE_OPERAND(response, 3);
	
	if (response == 0) {
		return "OK";
	} else if (resp2 == AVC1394_VCR_RESPONSE_TRANSPORT_STATE_LOAD_MEDIUM) {
		return("Loading Medium");
	} else if (resp2 == AVC1394_VCR_RESPONSE_TRANSPORT_STATE_RECORD) {
		if (resp3 == AVC1394_VCR_OPERAND_RECORD_PAUSE)
			return("Recording Paused");
		else
			return("Recording");
	} else if (resp2 == AVC1394_VCR_RESPONSE_TRANSPORT_STATE_PLAY) {
		if (resp3 >= AVC1394_VCR_OPERAND_PLAY_FAST_FORWARD_1
				&& resp3 <= AVC1394_VCR_OPERAND_PLAY_FASTEST_FORWARD) {
			return("Playing Fast Forward");
		} else if (resp3 >= AVC1394_VCR_OPERAND_PLAY_FAST_REVERSE_1
					&& resp3 <= AVC1394_VCR_OPERAND_PLAY_FASTEST_REVERSE) {
			return("Playing Reverse");
		} else if (resp3 == AVC1394_VCR_OPERAND_PLAY_FORWARD_PAUSE) {
			return("Playing Paused");
		} else {
			return("Playing");
		}
	} else if (resp2 == AVC1394_VCR_RESPONSE_TRANSPORT_STATE_WIND) {
		if (resp3 == AVC1394_VCR_OPERAND_WIND_HIGH_SPEED_REWIND) {
			return("Winding backward at incredible speed");
		} else if (resp3 == AVC1394_VCR_OPERAND_WIND_STOP) {
			return("Winding stopped");
		} else if (resp3 == AVC1394_VCR_OPERAND_WIND_REWIND) {
			return("Winding reverse");
		} else if (resp3 == AVC1394_VCR_OPERAND_WIND_FAST_FORWARD) {
			return("Winding forward");
		} else {
			return("Winding");
		}
	} else {
		return("Unknown");
	}
}

/* Get the time code on tape in format HH:MM:SS:FF */
char *
avc1394_vcr_get_timecode(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t  request[2];
	quadlet_t *response;
	char      *output = NULL;
		
	request[0] = STATVCR0 | AVC1394_VCR_COMMAND_TIME_CODE | 
		AVC1394_VCR_OPERAND_TIME_CODE_STATUS;
	request[1] = 0xFFFFFFFF;
	response = avc1394_transaction_block( handle, node, request, 2, AVC1394_RETRY);
	if (response == NULL) return NULL;
	
	output = malloc(12);    
	// consumer timecode format
	sprintf(output, "%2.2x:%2.2x:%2.2x:%2.2x",
		response[1] & 0x000000ff,
		(response[1] >> 8) & 0x000000ff,
		(response[1] >> 16) & 0x000000ff,
		(response[1] >> 24) & 0x000000ff);
	
	return output;
}

/* Get the time code on tape in format HH:MM:SS:FF */
int
avc1394_vcr_get_timecode2(raw1394handle_t handle, nodeid_t node, char *output)
{
	quadlet_t  request[2];
	quadlet_t *response;
		
	request[0] = STATVCR0 | AVC1394_VCR_COMMAND_TIME_CODE | 
		AVC1394_VCR_OPERAND_TIME_CODE_STATUS;
	request[1] = 0xFFFFFFFF;
	response = avc1394_transaction_block( handle, node, request, 2, AVC1394_RETRY);
	if (response == NULL) 
		return -1;
	
	// consumer timecode format
	sprintf(output, "%2.2x:%2.2x:%2.2x:%2.2x",
		response[1] & 0x000000ff,
		(response[1] >> 8) & 0x000000ff,
		(response[1] >> 16) & 0x000000ff,
		(response[1] >> 24) & 0x000000ff);
	
	return 0;
}


/* Go to the time code on tape in format HH:MM:SS:FF */
void
avc1394_vcr_seek_timecode(raw1394handle_t handle, nodeid_t node, char *timecode)
{
	quadlet_t  request[2];
	unsigned int hh,mm,ss,ff;
		
	request[0] = CTLVCR0 | AVC1394_VCR_COMMAND_TIME_CODE | 
		AVC1394_VCR_OPERAND_TIME_CODE_CONTROL;
	
	// consumer timecode format
	sscanf(timecode, "%2x:%2x:%2x:%2x", &hh, &mm, &ss, &ff);
	request[1] = 
		((ff & 0x000000ff) << 24) |
		((ss & 0x000000ff) << 16) |
		((mm & 0x000000ff) <<  8) |
		((hh & 0x000000ff) <<  0) ;
	printf("timecode: %08x\n", request[1]);
	
	avc1394_send_command_block( handle, node, request, 2);
}
