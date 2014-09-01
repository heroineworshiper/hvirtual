/*
* avc_vcr.c - An example of an AV/C Tape Recorder target implementation
*
* Copyright Dan Dennedy <dan@dennedy.org>
* 
* Inspired by virtual_vcr from Bonin Franck <boninf@free.fr>
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#include "../libavc1394/avc1394.h"

const char not_compatible[] = "\n"
	"This libraw1394 does not work with your version of Linux. You need a different\n"
	"version that matches your kernel (see kernel help text for the raw1394 option to\n"
	"find out which is the correct version).\n";

const char not_loaded[] = "\n"
	"This probably means that you don't have raw1394 support in the kernel or that\n"
	"you haven't loaded the raw1394 module.\n";


unsigned char g_signal_mode = 0x05; // SD 525-60, TODO: get from media
unsigned char g_transport_mode = AVC1394_VCR_CMD_WIND;
unsigned char g_transport_state = AVC1394_VCR_OPERAND_WIND_STOP;
int g_done = 0;

/**** subunit handlers ****/
int subunit_control( avc1394_cmd_rsp *cr )
{
	switch ( cr->opcode )
	{
	case AVC1394_VCR_CMD_PLAY:
		switch ( cr->operand[0] )
		{
		case AVC1394_VCR_OPERAND_PLAY_FORWARD:
		case AVC1394_VCR_OPERAND_PLAY_SLOWEST_FORWARD:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_FORWARD_6:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_FORWARD_5:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_FORWARD_4:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_FORWARD_3:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_FORWARD_2:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_FORWARD_1:
		case AVC1394_VCR_OPERAND_PLAY_X1_FORWARD:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_PLAY );
			g_transport_state = AVC1394_VCR_OPERAND_PLAY_FORWARD;
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("PLAY FORWARD\n");
			break;
		
		case AVC1394_VCR_OPERAND_PLAY_FASTEST_FORWARD:
		case AVC1394_VCR_OPERAND_PLAY_FAST_FORWARD_1:
		case AVC1394_VCR_OPERAND_PLAY_FAST_FORWARD_2:
		case AVC1394_VCR_OPERAND_PLAY_FAST_FORWARD_3:
		case AVC1394_VCR_OPERAND_PLAY_FAST_FORWARD_4:
		case AVC1394_VCR_OPERAND_PLAY_FAST_FORWARD_5:
		case AVC1394_VCR_OPERAND_PLAY_FAST_FORWARD_6:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_PLAY );
			g_transport_state = AVC1394_VCR_OPERAND_PLAY_FASTEST_FORWARD;
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("PLAY FASTEST FORWARD\n");
			break;
		
		case AVC1394_VCR_OPERAND_PLAY_REVERSE_PAUSE:
		case AVC1394_VCR_OPERAND_PLAY_FORWARD_PAUSE:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_PLAY );
			g_transport_state = cr->operand[0];
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("PAUSE PLAY\n");
			break;
		
		case AVC1394_VCR_OPERAND_PLAY_REVERSE:
		case AVC1394_VCR_OPERAND_PLAY_SLOWEST_REVERSE:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_REVERSE_6:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_REVERSE_5:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_REVERSE_4:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_REVERSE_3:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_REVERSE_2:
		case AVC1394_VCR_OPERAND_PLAY_SLOW_REVERSE_1:
		case AVC1394_VCR_OPERAND_PLAY_X1_REVERSE:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_PLAY );
			g_transport_state = AVC1394_VCR_OPERAND_PLAY_REVERSE;
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("PLAY REVERSE\n");
			break;

		case AVC1394_VCR_OPERAND_PLAY_FASTEST_REVERSE:
		case AVC1394_VCR_OPERAND_PLAY_FAST_REVERSE_1:
		case AVC1394_VCR_OPERAND_PLAY_FAST_REVERSE_2:
		case AVC1394_VCR_OPERAND_PLAY_FAST_REVERSE_3:
		case AVC1394_VCR_OPERAND_PLAY_FAST_REVERSE_4:
		case AVC1394_VCR_OPERAND_PLAY_FAST_REVERSE_5:
		case AVC1394_VCR_OPERAND_PLAY_FAST_REVERSE_6:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_PLAY );
			g_transport_state = AVC1394_VCR_OPERAND_PLAY_FASTEST_REVERSE;
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("PLAY FASTEST REVERSE\n");
			break;
		
		case AVC1394_VCR_OPERAND_PLAY_NEXT_FRAME:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_PLAY );
			g_transport_state = cr->operand[0];
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("PLAY NEXT FRAME\n");
			break;
		
		case AVC1394_VCR_OPERAND_PLAY_PREVIOUS_FRAME:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_PLAY );
			g_transport_state = cr->operand[0];
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("PLAY PREVIOUS FRAME\n");
			break;
		
		default:
			fprintf( stderr, "play mode 0x%02x non supported\n", cr->operand[0] );
			return 0;
		}
		break;
	case AVC1394_VCR_CMD_RECORD:
		switch ( cr->operand[0] )
		{
		case AVC1394_VCR_OPERAND_RECORD_RECORD:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_RECORD );
			g_transport_state = cr->operand[0];
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("RECORD\n");
			break;
		
		case AVC1394_VCR_OPERAND_RECORD_PAUSE:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_RECORD );
			g_transport_state = cr->operand[0];
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("PAUSE RECORD\n");
			break;
		
		default:
			fprintf( stderr, "record mode 0x%02x non supported\n", cr->operand[0] );
			return 0;
		}
		break;
	case AVC1394_VCR_CMD_WIND:
		switch ( cr->operand[0] )
		{
		case AVC1394_VCR_OPERAND_WIND_STOP:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_WIND );
			g_transport_state = cr->operand[0];
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("STOP\n");
			break;
		
		case AVC1394_VCR_OPERAND_WIND_REWIND:
		case AVC1394_VCR_OPERAND_WIND_HIGH_SPEED_REWIND:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_WIND );
			g_transport_state = AVC1394_VCR_OPERAND_WIND_REWIND;
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("REWIND\n");
			break;
		
		case AVC1394_VCR_OPERAND_WIND_FAST_FORWARD:
			g_transport_mode = AVC1394_GET_OPCODE( AVC1394_VCR_RESPONSE_TRANSPORT_STATE_WIND );
			g_transport_state = cr->operand[0];
			cr->status = AVC1394_RESP_ACCEPTED;
			printf("FAST FORWARD\n");
			break;
		
		default:
			fprintf( stderr, "wind mode 0x%02x non supported\n", cr->operand[0] );
			return 0;
		}
		break;
	default:
		fprintf( stderr, "subunit control command 0x%02x non supported\n", cr->opcode );
		return 0;
	}
	return 1;
}


int subunit_status( avc1394_cmd_rsp *cr )
{
	switch ( cr->opcode )
	{
	case AVC1394_VCR_CMD_OUTPUT_SIGNAL_MODE:
		cr->status = AVC1394_RESP_STABLE;
		cr->operand[0] = g_signal_mode;
		break;
	case AVC1394_VCR_CMD_INPUT_SIGNAL_MODE:
		cr->status = AVC1394_RESP_STABLE;
		cr->operand[0] = g_signal_mode;
		break;
	case AVC1394_VCR_CMD_TRANSPORT_STATE:
		cr->status = AVC1394_RESP_STABLE;
		cr->opcode = g_transport_mode;
		cr->operand[0] = g_transport_state;
		break;
	case AVC1394_VCR_CMD_TIME_CODE:
		cr->status = AVC1394_RESP_STABLE;
		cr->operand[0] = AVC1394_VCR_OPERAND_RECORDING_TIME_STATUS;
		// TODO: extract timecode from media or use time elapsed since start of app
		cr->operand[1] = 1; //frames
		cr->operand[2] = 2; //seconds
		cr->operand[3] = 3; //minutes
		cr->operand[4] = 4; //hours
		break;
	case AVC1394_VCR_CMD_MEDIUM_INFO:
		cr->status = AVC1394_RESP_STABLE;
		cr->operand[0] = AVC1394_VCR_OPERAND_MEDIUM_INFO_DVCR_STD;
		cr->operand[1] = AVC1394_VCR_OPERAND_MEDIUM_INFO_SVHS_OK;
		break;
	default:
		fprintf( stderr, "subunit status command 0x%02x not supported\n", cr->opcode );
		return 0;
	}
	return 1;
}


int subunit_inquiry( avc1394_cmd_rsp *cr )
{
	switch ( cr->opcode )
	{
	case AVC1394_VCR_CMD_PLAY:
	case AVC1394_VCR_CMD_RECORD:
	case AVC1394_VCR_CMD_WIND:
	case AVC1394_VCR_CMD_OUTPUT_SIGNAL_MODE:
	case AVC1394_VCR_CMD_INPUT_SIGNAL_MODE:
	case AVC1394_VCR_CMD_TRANSPORT_STATE:
	case AVC1394_VCR_CMD_TIME_CODE:
	case AVC1394_VCR_CMD_MEDIUM_INFO:
		cr->status = AVC1394_RESP_IMPLEMENTED;
		return 1;
	default:
		fprintf( stderr, "subunit inquiry command 0x%02x not supported\n", cr->opcode );
		return 0;
	}
	return 1;
}


/**** Unit handlers ****/
int unit_control( avc1394_cmd_rsp *cr )
{
	switch ( cr->opcode )
	{
	default:
		fprintf( stderr, "unit control command 0x%02x not supported\n", cr->opcode );
		return 0;
	}
	return 1;
}


int unit_status( avc1394_cmd_rsp *cr )
{
	cr->operand[1] = 0xff;
	cr->operand[2] = 0xff;
	cr->operand[3] = 0xff;
	cr->operand[4] = 0xff;
	switch ( cr->opcode )
	{
	case AVC1394_CMD_UNIT_INFO:
		cr->status = AVC1394_RESP_STABLE;
		cr->operand[0] = AVC1394_OPERAND_UNIT_INFO_EXTENSION_CODE;
		cr->operand[1] = AVC1394_SUBUNIT_TAPE_RECORDER;
		break;
	case AVC1394_CMD_SUBUNIT_INFO:
	{
		int page = ( cr->operand[0] >> 4 ) & 7;
		if ( page == 0 )
		{
			cr->status = AVC1394_RESP_STABLE;
			cr->operand[0] = (page << 4) | AVC1394_OPERAND_UNIT_INFO_EXTENSION_CODE;
			cr->operand[1] = AVC1394_SUBUNIT_TAPE_RECORDER << 3;
		}
		else
		{
			fprintf( stderr, "invalid page %d for subunit\n", page );
			return 0;
		}
		break;
	}
	default:
		fprintf( stderr, "unit status command 0x%02x not supported\n", cr->opcode );
		return 0;
	}
	return 1;
}


int unit_inquiry( avc1394_cmd_rsp *cr )
{
	switch ( cr->opcode )
	{
	case AVC1394_CMD_SUBUNIT_INFO:
	case AVC1394_CMD_UNIT_INFO:
		cr->status = AVC1394_RESP_IMPLEMENTED;
	default:
		fprintf( stderr, "unit inquiry command 0x%02x not supported\n", cr->opcode );
		return 0;
	}
	return 1;
}


/**** primary avc1394 target callback ****/
int command_handler( avc1394_cmd_rsp *cr )
{
	switch ( cr->subunit_type )
	{
	case AVC1394_SUBUNIT_TAPE_RECORDER:
		if ( cr->subunit_id != 0 )
		{
			fprintf( stderr, "subunit id 0x%02x not supported\n", cr->subunit_id );
			return 0;
		}
		switch ( cr->status )
		{
		case AVC1394_CTYP_CONTROL:
			return subunit_control( cr );
			break;
		case AVC1394_CTYP_STATUS:
			return subunit_status( cr );
			break;
		case AVC1394_CTYP_GENERAL_INQUIRY:
			return subunit_inquiry( cr );
			break;
		}
		break;
	case AVC1394_SUBUNIT_UNIT:
		switch ( cr->status )
		{
		case AVC1394_CTYP_CONTROL:
			return unit_control( cr );
			break;
		case AVC1394_CTYP_STATUS:
			return unit_status( cr );
			break;
		case AVC1394_CTYP_GENERAL_INQUIRY:
			return unit_inquiry( cr );
			break;
		}
		break;
	default:
		fprintf( stderr, "subunit type 0x%02x not supported\n", cr->subunit_type );
		return 0;
	}
	return 1;
}

int main( int argc, char **argv )
{
	raw1394handle_t handle;

	handle = raw1394_new_handle();

	if ( !handle )
	{
		if ( !errno )
		{
			printf( not_compatible );
		}
		else
		{
			perror( "couldn't get handle" );
			printf( not_loaded );
		}
		exit( EXIT_FAILURE );
	}

	if ( raw1394_set_port( handle, 0 ) < 0 )
	{
		perror( "couldn't set port" );
		exit( EXIT_FAILURE );
	}

	avc1394_init_target( handle, command_handler );
	
	printf( "Starting AV/C target; press Ctrl+C to quit...\n" );
	while ( !g_done )
	{
		g_done = raw1394_loop_iterate( handle );
	}
	
	avc1394_close_target( handle );
	
	exit( EXIT_SUCCESS );
}
