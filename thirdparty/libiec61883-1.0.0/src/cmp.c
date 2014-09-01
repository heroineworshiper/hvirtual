/*
 * libiec61883 - Linux IEEE 1394 streaming media library.
 * Copyright (C) 2004 Kristian Hogsberg, Dan Dennedy, and Dan Maas.
 * This file written by Dan Dennedy.
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

#include <stdio.h>
#include <libraw1394/csr.h>
#include <netinet/in.h>


int
iec61883_cmp_calc_bandwidth (raw1394handle_t handle, nodeid_t from, int plug,
		int speed)
{
	struct iec61883_oMPR ompr;
	struct iec61883_oPCR opcr;
	int bwu = -1; // failure
		
	if (iec61883_get_oMPR (handle, from, &ompr) < 0) {
		WARN ("%s: Failed to get the oMPR plug for node %d.", __FUNCTION__, 
			(int) from & 0x3f);
	} else if (ompr.n_plugs == 0) {
		WARN ("%s: The transmitting device (%d) does not have any output plugs.", 
			__FUNCTION__, (int) from & 0x3f);
	} else if (plug < ompr.n_plugs && plug < IEC61883_PCR_MAX) {
		if (iec61883_get_oPCRX (handle, from, &opcr, plug) < 0) {
			WARN ("%s: Failed to get the oPCR[%d] plug for node %d.", __FUNCTION__,
				plug, (int) from & 0x3f);
		} else {
			if (speed < 0 || speed > 2)
				speed = opcr.data_rate;
			if (opcr.overhead_id > 0)
				bwu = (opcr.overhead_id * 32) + (opcr.payload + 3) * (1 << (2 - speed)) * 4;
			else
				bwu = 512 + (opcr.payload + 3) * (1 << (2 - speed)) * 4;
		}
	}
	
	return bwu;
}

int
iec61883_cmp_create_p2p (raw1394handle_t handle, 
		nodeid_t output_node, int output_plug,
		nodeid_t input_node, int input_plug,	
		unsigned int channel, unsigned int speed)
{
	struct iec61883_oPCR opcr, save_opcr;
	struct iec61883_iPCR ipcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_oPCRX (handle, output_node, &opcr, output_plug) < 0) {
		WARN ("%s: Failed to get the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	if (iec61883_get_iPCRX (handle, input_node, &ipcr, input_plug) < 0) {
		WARN ("%s: Failed to get the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug, (int) input_node & 0x3f);
		return -1;
	}

	save_opcr = opcr;
	opcr.channel = ipcr.channel = channel;
	opcr.data_rate = speed;
	if (opcr.n_p2p_connections < 63)
		opcr.n_p2p_connections++;
	if (ipcr.n_p2p_connections < 63)
		ipcr.n_p2p_connections++;
	
	if (iec61883_set_oPCRX (handle, output_node, opcr, output_plug) < 0) {
		WARN ("%s: Failed to set the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	if (iec61883_set_iPCRX (handle, input_node, ipcr, input_plug) < 0) {
		WARN ("%s: Failed to set the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug,	(int) input_node & 0x3f);
		// Undo changes on the oPCR
		if (iec61883_set_oPCRX (handle, output_node, save_opcr, output_plug) < 0) {
			WARN ("%s: Failed to undo changes on the oPCR[%d] plug for node %d.",
				__FUNCTION__, output_plug, (int) output_node & 0x3f);
		}
		return -1;
	}
	
	return 0;
}

int
iec61883_cmp_create_p2p_output (raw1394handle_t handle, 
		nodeid_t output_node, int output_plug,
		unsigned int channel, unsigned int speed)
{
	struct iec61883_oPCR opcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_oPCRX (handle, output_node, &opcr, output_plug) < 0) {
		WARN ("%s: Failed to get the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}

	opcr.channel = channel;
	opcr.data_rate = speed;
	if (opcr.n_p2p_connections < 63)
		opcr.n_p2p_connections++;
	
	if (iec61883_set_oPCRX (handle, output_node, opcr, output_plug) < 0) {
		WARN ("%s: Failed to set the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	
	return 0;
}

int
iec61883_cmp_create_p2p_input (raw1394handle_t handle,
		nodeid_t input_node, int input_plug,
		unsigned int channel)
{
	struct iec61883_iPCR ipcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_iPCRX (handle, input_node, &ipcr, input_plug) < 0) {
		WARN ("%s: Failed to get the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug, (int) input_node & 0x3f);
		return -1;
	}

	ipcr.channel = channel;
	if (ipcr.n_p2p_connections < 63)
		ipcr.n_p2p_connections++;
	
	if (iec61883_set_iPCRX (handle, input_node, ipcr, input_plug) < 0) {
		WARN ("%s: Failed to set the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug, (int) input_node & 0x3f);
		return -1;
	}
	
	return 0;
}

int
iec61883_cmp_create_bcast (raw1394handle_t handle, 
		nodeid_t output_node, int output_plug,
		nodeid_t input_node, int input_plug,
		unsigned int channel, unsigned int speed)
{
	struct iec61883_oPCR opcr, save_opcr;
	struct iec61883_iPCR ipcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_oPCRX (handle, output_node, &opcr, output_plug) < 0) {
		WARN ("%s: Failed to get the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	if (iec61883_get_iPCRX (handle, input_node, &ipcr, input_plug) < 0) {
		WARN ("%s: Failed to get the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug, (int) input_node & 0x3f);
		return -1;
	}

	save_opcr = opcr;
	opcr.channel = ipcr.channel = channel;
	opcr.data_rate = speed;
	opcr.bcast_connection = 1;
	ipcr.bcast_connection = 1;
	
	if (iec61883_set_oPCRX (handle, output_node, opcr, output_plug) < 0) {
		WARN ("%s: Failed to set the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	if (iec61883_set_iPCRX (handle, input_node, ipcr, input_plug) < 0) {
		WARN ("%s: Failed to set the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug,	(int) input_node & 0x3f);
		// Undo changes on the oPCR
		if (iec61883_set_oPCRX (handle, output_node, save_opcr, output_plug) < 0) {
			WARN ("%s: Failed to undo changes on the oPCR[%d] plug for node %d.", 
				__FUNCTION__, output_plug, (int) output_node & 0x3f);
		}
		return -1;
	}
	
	return 0;
}

int
iec61883_cmp_create_bcast_output (raw1394handle_t handle, 
		nodeid_t output_node, int output_plug,
		unsigned int channel, unsigned int speed)
{
	struct iec61883_oPCR opcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_oPCRX (handle, output_node, &opcr, output_plug) < 0) {
		WARN ("%s: Failed to get the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}

	opcr.channel = channel;
	opcr.data_rate = speed;
	opcr.bcast_connection  = 1;
	
	if (iec61883_set_oPCRX (handle, output_node, opcr, output_plug) < 0) {
		WARN ("%s: Failed to set the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	
	return 0;
}

int
iec61883_cmp_create_bcast_input (raw1394handle_t handle,
		nodeid_t input_node, int input_plug,
		unsigned int channel)
{
	struct iec61883_iPCR ipcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_iPCRX (handle, input_node, &ipcr, input_plug) < 0) {
		WARN ("%s: Failed to get the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug,	(int) input_node & 0x3f);
		return -1;
	}

	ipcr.channel = channel;
	ipcr.bcast_connection = 1;
	
	if (iec61883_set_iPCRX (handle, input_node, ipcr, input_plug) < 0) {
		WARN ("%s: Failed to set the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug, (int) input_node & 0x3f);
		return -1;
	}
	
	return 0;
}

int
iec61883_cmp_overlay_p2p (raw1394handle_t handle, 
		nodeid_t output_node, int output_plug,
		nodeid_t input_node, int input_plug)
{
	struct iec61883_oPCR opcr, save_opcr;
	struct iec61883_iPCR ipcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_oPCRX (handle, output_node, &opcr, output_plug) < 0) {
		WARN ("%s: Failed to get the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	if (iec61883_get_iPCRX (handle, input_node, &ipcr, input_plug) < 0) {
		WARN ("%s: Failed to get the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug,	(int) input_node & 0x3f);
		return -1;
	}

	if (opcr.bcast_connection == 0) {
		save_opcr = opcr;
		if (opcr.n_p2p_connections < 63)
			opcr.n_p2p_connections++;
	}
	if (ipcr.bcast_connection == 0)
		if (ipcr.n_p2p_connections < 63)
			ipcr.n_p2p_connections++;
	
	if (iec61883_set_oPCRX (handle, output_node, opcr, output_plug) < 0) {
		WARN ("%s: Failed to set the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	if (iec61883_set_iPCRX (handle, input_node, ipcr, input_plug) < 0) {
		WARN ("%s: Failed to set the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug,	(int) input_node & 0x3f);
		// Undo changes on the oPCR
		if (iec61883_set_oPCRX (handle, output_node, save_opcr, output_plug) < 0) {
			WARN ("%s: Failed to undo changes on the oPCR[%d] plug for node %d.", 
				__FUNCTION__, output_plug, (int) output_node & 0x3f);
		}
		return -1;
	}
	
	return 0;
}

int
iec61883_cmp_overlay_p2p_output (raw1394handle_t handle, 
		nodeid_t output_node, int output_plug)
{
	struct iec61883_oPCR opcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_oPCRX (handle, output_node, &opcr, output_plug) < 0) {
		WARN ("%s: Failed to get the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}

	if (opcr.bcast_connection == 0) {
		if (opcr.n_p2p_connections < 63)
			opcr.n_p2p_connections++;
		
		if (iec61883_set_oPCRX (handle, output_node, opcr, output_plug) < 0) {
			WARN ("%s: Failed to set the oPCR[%d] plug for node %d.", __FUNCTION__,
				output_plug, (int) output_node & 0x3f);
			return -1;
		}
	}
	
	return 0;
}

int
iec61883_cmp_overlay_p2p_input (raw1394handle_t handle,
		nodeid_t input_node, int input_plug)
{
	struct iec61883_iPCR ipcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_iPCRX (handle, input_node, &ipcr, input_plug) < 0) {
		WARN ("%s: Failed to get the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug, (int) input_node & 0x3f);
		return -1;
	}

	if (ipcr.bcast_connection == 0) {
		if (ipcr.n_p2p_connections < 63)
			ipcr.n_p2p_connections++;
		
		if (iec61883_set_iPCRX (handle, input_node, ipcr, input_plug) < 0) {
			WARN ("%s: Failed to set the iPCR[%d] plug for node %d.", __FUNCTION__,
				input_plug,	(int) input_node & 0x3f);
			return -1;
		}
	}
	
	return 0;
}

int
iec61883_cmp_overlay_bcast (raw1394handle_t handle, 
		nodeid_t output_node, int output_plug,
		nodeid_t input_node, int input_plug)
{
	struct iec61883_oPCR opcr, save_opcr;
	struct iec61883_iPCR ipcr;
	
	DEBUG ("%s", __FUNCTION__);
	
	if (iec61883_get_oPCRX (handle, output_node, &opcr, output_plug) < 0) {
		WARN ("%s: Failed to get the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	if (iec61883_get_iPCRX (handle, input_node, &ipcr, input_plug) < 0) {
		WARN ("%s: Failed to get the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug, (int) input_node & 0x3f);
		return -1;
	}

	save_opcr = opcr;
	// We still need this function because only one of the plugs might have the
	// bcast_connection set.
	opcr.bcast_connection = 1;
	ipcr.bcast_connection = 1;
	
	if (iec61883_set_oPCRX (handle, output_node, opcr, output_plug) < 0) {
		WARN ("%s: Failed to set the oPCR[%d] plug for node %d.", __FUNCTION__,
			output_plug, (int) output_node & 0x3f);
		return -1;
	}
	if (iec61883_set_iPCRX (handle, input_node, ipcr, input_plug) < 0) {
		WARN ("%s: Failed to set the iPCR[%d] plug for node %d.", __FUNCTION__,
			input_plug,	(int) input_node & 0x3f);
		// Undo changes on the oPCR
		if (iec61883_set_oPCRX (handle, output_node, save_opcr, output_plug) < 0) {
			WARN ("%s: Failed to undo changes on the oPCR[%d] plug for node %d.", 
				__FUNCTION__, output_plug, (int) output_node & 0x3f);
		}
		return -1;
	}
	
	return 0;
}

static int
allocate_channel (raw1394handle_t handle)
{
	int c = -1;
	
	for (c = 0; c < 63; c++)
		if (raw1394_channel_modify (handle, c, RAW1394_MODIFY_ALLOC) == 0)
			break;
	
	DEBUG ("%s: %d", __FUNCTION__, c);
	
	return c;
}

int
iec61883_cmp_connect (raw1394handle_t handle, nodeid_t output, int *oplug, 
	nodeid_t input, int *iplug, int *bandwidth)
{
	struct iec61883_oMPR ompr;
	struct iec61883_iMPR impr;
	struct iec61883_oPCR opcr;
	struct iec61883_iPCR ipcr;
	int oplug_online = -1, iplug_online = -1;
	int channel = -1;
	int skip_bandwidth = (*bandwidth == 0);
	int failure = 0;
	
	DEBUG ("%s", __FUNCTION__);
	
	*bandwidth = 0;
		
	// Check for plugs on output
	if (iec61883_get_oMPR (handle, output, &ompr) < 0)
		ompr.n_plugs = 0;
	
	// Check for plugs on input
	if (iec61883_get_iMPR (handle, input, &impr) < 0)
		impr.n_plugs = 0;
	
	DEBUG ("output node %d #plugs=%d, input node %d #plugs=%d", output & 0x3f,
		ompr.n_plugs, input & 0x3f, impr.n_plugs);
	
	if (ompr.n_plugs > 0 && impr.n_plugs > 0) {
		// establish or overlay point-to-point
		
		// speed to use is lesser of output and input
		unsigned int speed = 
			impr.data_rate < ompr.data_rate ? impr.data_rate : ompr.data_rate;
		
		// determine if output has plug available
		if (*oplug < 0) {
			for (*oplug = 0; *oplug < ompr.n_plugs; (*oplug)++) {
				if (iec61883_get_oPCRX (handle, output, &opcr, *oplug) == 0) {
					// get first online plug
					if (oplug_online == -1 && opcr.online)
						oplug_online = *oplug;
					if (opcr.online && opcr.n_p2p_connections == 0)
						break;
				}
			}
		} else if (iec61883_get_oPCRX (handle, output, &opcr, *oplug) < 0)
			FAIL ("Failed to get plug %d for output node", *oplug);
		
		// determine if input has plug available
		if (*iplug < 0) {
			for (*iplug = 0; *iplug < impr.n_plugs; (*iplug)++) {
				if (iec61883_get_iPCRX (handle, input, &ipcr, *iplug) == 0) {
					// get first online plug
					if (iplug_online == -1 && ipcr.online)
						iplug_online = *iplug;
					if (ipcr.online && ipcr.n_p2p_connections == 0)
						break;
				}
			}
		} else if (iec61883_get_iPCRX (handle, input, &ipcr, *iplug) < 0)
			FAIL ("Failed to get plug %d for input node", *iplug);
		
		if (*oplug < ompr.n_plugs && *iplug < impr.n_plugs) {
			
			if (opcr.bcast_connection == 1) {
				channel = opcr.channel;
				iec61883_cmp_overlay_bcast (handle, output, *oplug, input, *iplug);
			} else {
				// allocate bandwidth
				if (!skip_bandwidth) {
					*bandwidth = iec61883_cmp_calc_bandwidth (handle, output, *oplug, speed);
					if (*bandwidth < 1) {
						WARN ("Failed to calculate bandwidth.");
						failure = 1;
					} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
						WARN ("Failed to allocate bandwidth.");
						failure = 1;
					}
				}
				if (!failure) {
					channel = allocate_channel (handle);
					if (iec61883_cmp_create_p2p (handle, output, *oplug, input, *iplug, 
						channel, speed) < 0) {
						// release channel and bandwidth
						failure = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
						if (!failure)
							raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_FREE);
						channel = -1;
					}
				}
			}

		} else if (*iplug < impr.n_plugs && oplug_online > -1 ) {
			// get the channel from output - can not start another transmission 
			// on an existing channel, but can receive from multiple nodes/plugs
			*oplug = oplug_online;
			if (iec61883_get_oPCRX (handle, output, &opcr, oplug_online) == 0) {
				channel = opcr.channel;
				if (opcr.bcast_connection == 1) {
					iec61883_cmp_overlay_bcast (handle, output, oplug_online, input, *iplug);
				} else {
					if (iec61883_cmp_create_p2p_input (handle, input, *iplug, channel) < 0)
						channel = -1;
					else if (iec61883_cmp_overlay_p2p_output (handle, output, oplug_online) < 0)
						channel = -1;
				}
			}
			
		} else if (oplug_online != -1 && iplug_online > -1) {
			// get channel from output
			*oplug = oplug_online;
			if (iec61883_get_oPCRX (handle, output, &opcr, oplug_online) == 0) {
				channel = opcr.channel;
				if (iec61883_cmp_overlay_p2p (handle, output, oplug_online,
					input, iplug_online) < 0)
					channel = -1;
			}
		} else {
			WARN ("All the plugs on both nodes are offline!");
			*oplug = *iplug = -1;
		}
			
	} else if (ompr.n_plugs > 0) {
		// establish or overlay half point-to-point on output
		*iplug = -1;
		
		// determine if output has plug available
		if (*oplug < 0) {
			for (*oplug = 0; *oplug < ompr.n_plugs; (*oplug)++) {
				if (iec61883_get_oPCRX (handle, output, &opcr, *oplug) == 0) {
					// get first online plug
					if (oplug_online == -1 && opcr.online)
						oplug_online = *oplug;
					if (opcr.online && opcr.n_p2p_connections == 0)
						break;
				}
			}
		} else if (iec61883_get_oPCRX (handle, output, &opcr, *oplug) < 0)
			FAIL ("Failed to get plug %d for output node", *oplug);
		
		if (*oplug < ompr.n_plugs) {
			if (opcr.bcast_connection == 1) {
				channel = opcr.channel;
			} else {
				// establish
				// XXX: the input must provide manual channel selection or we should
				// do a broadcast. Example use case: DV device is output and local
				// node is input, but software allows channel select. Failure use
				// case: local node is output but input device has no channel selection!
				// Both use cases are actually quite common. Should we provide a 
				// parameter to offer a hint in case operator knows something more
				// about the device than firewire interfaces on it suggest?
				
				// allocate bandwidth
				if (!skip_bandwidth) {
					*bandwidth = iec61883_cmp_calc_bandwidth (handle, output, *oplug, ompr.data_rate);
					if (*bandwidth < 1) {
						WARN ("Failed to calculate bandwidth.");
						failure = 1;
					} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
						WARN ("Failed to allocate bandwidth.");
						failure = 1;
					}
				}
				if (!failure) {
					channel = allocate_channel (handle);
					if (iec61883_cmp_create_p2p_output (handle, output, *oplug, 
						channel, ompr.data_rate) == 0) {
						WARN ("Established connection on channel %d.\n"
							  "You may need to manually set the channel on the receiving node.",
							  channel);
					} else {
						// release channel and bandwidth
						failure = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
						if (!failure)
							raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_FREE);
						channel = -1;
					}
				}
			}
			
		} else if (oplug_online > -1) {
			// overlay
			// get channel from output
			*oplug = oplug_online;
			if (iec61883_get_oPCRX (handle, output, &opcr, oplug_online) == 0) {
				channel = opcr.channel;
				if (opcr.bcast_connection != 1)
					if (iec61883_cmp_overlay_p2p_output (handle, output, oplug_online) < 0)
						channel = -1;
			}
			WARN ("Overlayed connection on channel %d.\n"
				  "You may need to manually set the channel on the receiving node.",
				  channel);
		} else {
			WARN ("Transmission node has no plugs online!");
			*oplug = -1;
			// failover to broadcast
			// allocate bandwidth based upon first out plug
			if (!skip_bandwidth) {
				*bandwidth = iec61883_cmp_calc_bandwidth (handle, output, 0, ompr.data_rate);
				if (*bandwidth < 1) {
					WARN ("Failed to calculate bandwidth.");
					failure = 1;
				} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
					WARN ("Failed to allocate bandwidth.");
					failure = 1;
				}
			}
			if (!failure) {
				if (raw1394_channel_modify (handle, ompr.bcast_channel, RAW1394_MODIFY_ALLOC) == 0)
					channel = ompr.bcast_channel;
			}
		}
		
	} else if (impr.n_plugs > 0) {
		// establish or overlay half point-to-point on input
		*oplug = -1;
		
		// determine if input has plug available
		if (*iplug < 0) {
			for (*iplug = 0; *iplug < impr.n_plugs; (*iplug)++) {
				if (iec61883_get_iPCRX (handle, input, &ipcr, *iplug) == 0) {
					// get first online plug
					if (iplug_online == -1 && ipcr.online)
						iplug_online = *iplug;
					if (ipcr.online && ipcr.n_p2p_connections == 0)
						break;
				}
			}
		} else if (iec61883_get_iPCRX (handle, input, &ipcr, *iplug) < 0)
			FAIL ("Failed to get plug %d for input node", *iplug);
		
		if (*iplug < impr.n_plugs) {
			if (ipcr.bcast_connection == 1) {
				channel = ipcr.channel;
			} else {
				// establish
				
				// allocate bandwidth
				// cannot accurately allocate bandwidth with no output plug
				// use an output plug on the input device as a best guess
				if (!skip_bandwidth) {
					*bandwidth = iec61883_cmp_calc_bandwidth (handle, input, *iplug, -1);
					if (*bandwidth < 1) {
						WARN ("Failed to calculate bandwidth.");
						failure = 1;
					} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
						WARN ("Failed to allocate bandwidth.");
						failure = 1;
					}
				}
				if (!failure) {
					channel = allocate_channel (handle);
					if (iec61883_cmp_create_p2p_input (handle, input, *iplug, channel) == 0) {
						WARN ("Established connection on channel %d.\n"
							  "You may need to manually set the channel on the transmitting node.",
							  channel);
					} else {
						// release channel and bandwidth
						failure = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
						if (!failure)
							raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_FREE);
						channel = -1;
					}
				}
			}
			
		} else if (iplug_online > -1) {
			// overlay
			// get channel from input
			*iplug = iplug_online;
			if (iec61883_get_iPCRX (handle, input, &ipcr, iplug_online) == 0) {
				channel = ipcr.channel;
				if (ipcr.bcast_connection != 1)
					if (iec61883_cmp_overlay_p2p_input (handle, input, iplug_online) < 0)
						channel = -1;
			}
			WARN ("Overlayed connection on channel %d.\n"
				  "You may need to manually set the channel on the transmitting node.",
				  channel);
		} else {
			WARN ("Receiving node has no plugs online!");
			// failover to broadcast
			// allocate bandwidth based upon first input plug
			if (!skip_bandwidth) {
				*bandwidth = iec61883_cmp_calc_bandwidth (handle, input, 0, impr.data_rate);
				if (*bandwidth < 1) {
					WARN ("Failed to calculate bandwidth.");
					failure = 1;
				} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
					WARN ("Failed to allocate bandwidth.");
					failure = 1;
				}
			}
			if (!failure) {
				if (raw1394_channel_modify (handle, 63, RAW1394_MODIFY_ALLOC) == 0)
					channel = 63;
			}
		}
			
	} else {
		// no input or output plugs - failover broadcast on channel 63
		// not enough information to calculate bandwidth
		*oplug = *iplug = -1;
		if (raw1394_channel_modify (handle, 63, RAW1394_MODIFY_ALLOC) == 0)
			channel = 63;
		if (channel == 63)
			WARN ("No plugs exist on either node; using default broadcast channel 63.");
	}

	return channel;
}

int
iec61883_cmp_reconnect (raw1394handle_t handle, nodeid_t output, int *oplug,
		nodeid_t input, int *iplug, int *bandwidth, int channel)
{
	struct iec61883_oMPR ompr;
	struct iec61883_iMPR impr;
	struct iec61883_oPCR opcr;
	struct iec61883_iPCR ipcr;
	int oplug_online = -1, iplug_online = -1;
	int skip_bandwidth = (*bandwidth == 0);
	int failure = 0;

	DEBUG ("%s", __FUNCTION__);
	
	*bandwidth = 0;

	// Check for plugs on output
	if (iec61883_get_oMPR (handle, output, &ompr) < 0)
		ompr.n_plugs = 0;

	// Check for plugs on input
	if (iec61883_get_iMPR (handle, input, &impr) < 0)
		impr.n_plugs = 0;

	DEBUG ("output node %d #plugs=%d, input node %d #plugs=%d", output & 0x3f,
	       ompr.n_plugs, input & 0x3f, impr.n_plugs);

	if (ompr.n_plugs > 0 && impr.n_plugs > 0) {
		// establish or overlay point-to-point

		// speed to use is lesser of output and input
		unsigned int speed = impr.data_rate < ompr.data_rate ? impr.data_rate : ompr.data_rate;

		// determine if output has plug available
		if (*oplug < 0) {
			for (*oplug = 0; *oplug < ompr.n_plugs; (*oplug)++) {
				if (iec61883_get_oPCRX (handle, output, &opcr, *oplug) == 0) {
					// get first online plug
					if (oplug_online == -1 && opcr.online)
						oplug_online = *oplug;
					if (opcr.online && opcr.n_p2p_connections == 0)
						break;
				}
			}
		} else if (iec61883_get_oPCRX (handle, output, &opcr, *oplug) < 0)
			FAIL ("Failed to get plug %d for output node", *oplug);

		// determine if input has plug available
		if (*iplug < 0) {
			for (*iplug = 0; *iplug < impr.n_plugs; (*iplug)++) {
				if (iec61883_get_iPCRX (handle, input, &ipcr, *iplug) == 0) {
					// get first online plug
					if (iplug_online == -1 && ipcr.online)
						iplug_online = *iplug;
					if (ipcr.online && ipcr.n_p2p_connections == 0)
						break;
				}
			}
		} else if (iec61883_get_iPCRX (handle, input, &ipcr, *iplug) < 0)
			FAIL ("Failed to get plug %d for input node", *iplug);

		if (*oplug < ompr.n_plugs && *iplug < impr.n_plugs) {

			if (opcr.bcast_connection == 1) {
				channel = opcr.channel;
				iec61883_cmp_overlay_bcast (handle, output, *oplug, input, *iplug);
			} else {
				// allocate bandwidth
				if (!skip_bandwidth) {
					*bandwidth = iec61883_cmp_calc_bandwidth (handle, output, *oplug, speed);
					if (*bandwidth < 1) {
						WARN ("Failed to calculate bandwidth.");
						failure = 1;
					} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
						WARN ("Failed to allocate bandwidth.");
						failure = 1;
					}
				}
				if (!failure) {
					raw1394_channel_modify (handle, channel, RAW1394_MODIFY_ALLOC);
					if (iec61883_cmp_create_p2p (handle, output, *oplug, input, *iplug, channel, speed) < 0) {
						// release channel and bandwidth
						failure = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
						if (!failure)
							raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_FREE);
						channel = -1;
					}
				}
			}

		} else if (*iplug < impr.n_plugs && oplug_online > -1 ) {
			// get the channel from output - can not start another transmission
			// on an existing channel, but can receive from multiple nodes/plugs
			*oplug = oplug_online;
			if (iec61883_get_oPCRX (handle, output, &opcr, oplug_online) == 0) {
				channel = opcr.channel;
				if (opcr.bcast_connection == 1) {
					iec61883_cmp_overlay_bcast (handle, output, oplug_online, input, *iplug);
				} else {
					if (iec61883_cmp_create_p2p_input (handle, input, *iplug, channel) < 0)
						channel = -1;
					else if (iec61883_cmp_overlay_p2p_output (handle, output, oplug_online) < 0)
						channel = -1;
				}
			}

		} else if (oplug_online != -1 && iplug_online > -1) {
			// get channel from output
			*oplug = oplug_online;
			if (iec61883_get_oPCRX (handle, output, &opcr, oplug_online) == 0) {
				channel = opcr.channel;
				if (iec61883_cmp_overlay_p2p (handle, output, oplug_online, input, iplug_online) < 0)
					channel = -1;
			}
		} else {
			WARN ("All the plugs on both nodes are offline!");
			*oplug = *iplug = -1;
		}

	} else if (ompr.n_plugs > 0) {
		// establish or overlay half point-to-point on output
		*iplug = -1;

		// determine if output has plug available
		if (*oplug < 0) {
			for (*oplug = 0; *oplug < ompr.n_plugs; (*oplug)++) {
				if (iec61883_get_oPCRX (handle, output, &opcr, *oplug) == 0) {
					// get first online plug
					if (oplug_online == -1 && opcr.online)
						oplug_online = *oplug;
					if (opcr.online && opcr.n_p2p_connections == 0)
						break;
				}
			}
		} else if (iec61883_get_oPCRX (handle, output, &opcr, *oplug) < 0)
			FAIL ("Failed to get plug %d for output node", *oplug);

		if (*oplug < ompr.n_plugs) {
			if (opcr.bcast_connection == 1) {
				channel = opcr.channel;
			} else {
				// establish
				// XXX: the input must provide manual channel selection or we should
				// do a broadcast. Example use case: DV device is output and local
				// node is input, but software allows channel select. Failure use
				// case: local node is output but input device has no channel selection!
				// Both use cases are actually quite common. Should we provide a
				// parameter to offer a hint in case operator knows something more
				// about the device than firewire interfaces on it suggest?

				// allocate bandwidth
				if (!skip_bandwidth) {
					*bandwidth = iec61883_cmp_calc_bandwidth (handle, output, *oplug, ompr.data_rate);
					if (*bandwidth < 1) {
						WARN ("Failed to calculate bandwidth.");
						failure = 1;
					} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
						WARN ("Failed to allocate bandwidth.");
						failure = 1;
					}
				}
				if (!failure) {
					raw1394_channel_modify (handle, channel, RAW1394_MODIFY_ALLOC);
					if (iec61883_cmp_create_p2p_output (handle, output, *oplug, channel, ompr.data_rate) == 0) {
						WARN ("Established connection on channel %d.\n"
						      "You may need to manually set the channel on the receiving node.",
						      channel);
					} else {
						// release channel and bandwidth
						failure = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
						if (!failure)
							raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_FREE);
						channel = -1;
					}
				}
			}

		} else if (oplug_online > -1) {
			// overlay
			// get channel from output
			*oplug = oplug_online;
			if (iec61883_get_oPCRX (handle, output, &opcr, oplug_online) == 0) {
				channel = opcr.channel;
				if (opcr.bcast_connection != 1)
					if (iec61883_cmp_overlay_p2p_output (handle, output, oplug_online) < 0)
						channel = -1;
			}
			WARN ("Overlayed connection on channel %d.\n"
			      "You may need to manually set the channel on the receiving node.",
			      channel);
		} else {
			WARN ("Transmission node has no plugs online!");
			// failover to broadcast
			// allocate bandwidth based upon first out plug
			*oplug = -1;
			if (!skip_bandwidth) {
				*bandwidth = iec61883_cmp_calc_bandwidth (handle, output, 0, ompr.data_rate);
				if (*bandwidth < 1) {
					WARN ("Failed to calculate bandwidth.");
					failure = 1;
				} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
					WARN ("Failed to allocate bandwidth.");
					failure = 1;
				}
			}
			if (!failure) {
				if (raw1394_channel_modify (handle, ompr.bcast_channel, RAW1394_MODIFY_ALLOC) == 0)
					channel = ompr.bcast_channel;
			}
		}

	} else if (impr.n_plugs > 0) {
		// establish or overlay half point-to-point on input
		*oplug = -1;

		// determine if input has plug available
		if (*iplug < 0) {
			for (*iplug = 0; *iplug < impr.n_plugs; (*iplug)++) {
				if (iec61883_get_iPCRX (handle, input, &ipcr, *iplug) == 0) {
					// get first online plug
					if (iplug_online == -1 && ipcr.online)
						iplug_online = *iplug;
					if (ipcr.online && ipcr.n_p2p_connections == 0)
						break;
				}
			}
		} else if (iec61883_get_iPCRX (handle, input, &ipcr, *iplug) < 0)
			FAIL ("Failed to get plug %d for input node", *iplug);

		if (*iplug < impr.n_plugs) {
			if (ipcr.bcast_connection == 1) {
				channel = ipcr.channel;
			} else {
				// establish

				// allocate bandwidth
				// cannot accurately allocate bandwidth with no output plug
				// use an output plug on the input device as a best guess
				if (!skip_bandwidth) {
					*bandwidth = iec61883_cmp_calc_bandwidth (handle, input, *iplug, -1);
					if (*bandwidth < 1) {
						WARN ("Failed to calculate bandwidth.");
						failure = 1;
					} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
						WARN ("Failed to allocate bandwidth.");
						failure = 1;
					}
				}
				if (!failure) {
					raw1394_channel_modify (handle, channel, RAW1394_MODIFY_ALLOC);
					if (iec61883_cmp_create_p2p_input (handle, input, *iplug, channel) == 0) {
						WARN ("Established connection on channel %d.\n"
						      "You may need to manually set the channel on the transmitting node.",
						      channel);
					} else {
						// release channel and bandwidth
						failure = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
						if (!failure)
							raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_FREE);
						channel = -1;
					}
				}
			}

		} else if (iplug_online > -1) {
			// overlay
			// get channel from input
			*iplug = iplug_online;
			if (iec61883_get_iPCRX (handle, input, &ipcr, iplug_online) == 0) {
				channel = ipcr.channel;
				if (ipcr.bcast_connection != 1)
					if (iec61883_cmp_overlay_p2p_input (handle, input, iplug_online) < 0)
						channel = -1;
			}
			WARN ("Overlayed connection on channel %d.\n"
			      "You may need to manually set the channel on the transmitting node.",
			      channel);
		} else {
			WARN ("Receiving node has no plugs online!");
			// failover to broadcast
			// allocate bandwidth based upon first input plug
			*iplug = -1;
			if (!skip_bandwidth) {
				*bandwidth = iec61883_cmp_calc_bandwidth (handle, input, 0, impr.data_rate);
				if (*bandwidth < 1) {
					WARN ("Failed to calculate bandwidth.");
					failure = 1;
				} else if (raw1394_bandwidth_modify (handle, *bandwidth, RAW1394_MODIFY_ALLOC) < 0) {
					WARN ("Failed to allocate bandwidth.");
					failure = 1;
				}
			}
			if (!failure) {
				if (raw1394_channel_modify (handle, 63, RAW1394_MODIFY_ALLOC) == 0)
					channel = 63;
			}
		}

	} else {
		// no input or output plugs - failover broadcast on channel 63
		// not enough information to calculate bandwidth
		*oplug = *iplug = -1;
		if (raw1394_channel_modify (handle, 63, RAW1394_MODIFY_ALLOC) == 0)
			channel = 63;
		if (channel == 63)
			WARN ("No plugs exist on either node; using default broadcast channel 63.");
	}

	return channel;
}

int
iec61883_cmp_disconnect (raw1394handle_t handle, nodeid_t output, int oplug,
		nodeid_t input, int iplug, unsigned int channel, unsigned int bandwidth)
{
	struct iec61883_oMPR ompr;
	struct iec61883_iMPR impr;
	struct iec61883_oPCR opcr;
	struct iec61883_iPCR ipcr;
	int result = 0;
	
	DEBUG ("%s: oplug %d iplug %d channel %u bw %u", __FUNCTION__,
		oplug, iplug, channel, bandwidth);
	
	// Check for plugs on output
	if (iec61883_get_oMPR (handle, output, &ompr) < 0)
		ompr.n_plugs = 0;
	
	// Check for plugs on input
	if (iec61883_get_iMPR (handle, input, &impr) < 0)
		impr.n_plugs = 0;
	
	if (ompr.n_plugs > 0 && impr.n_plugs > 0) {
		// establish or overlay point-to-point
		
		// determine if output has plug available
		if (oplug < 0) {
			for (oplug = 0; oplug < ompr.n_plugs; oplug++) {
				if (iec61883_get_oPCRX (handle, output, &opcr, oplug) == 0) {
					if (opcr.online && opcr.channel == channel)
						break;
				}
			}
		} else if (iec61883_get_oPCRX (handle, output, &opcr, oplug) < 0)
			FAIL ("Failed to get plug %d for output node", oplug);
		
		// determine if input has plug available
		if (iplug < 0) {
			for (iplug = 0; iplug < impr.n_plugs; iplug++) {
				if (iec61883_get_iPCRX (handle, input, &ipcr, iplug) == 0) {
					if (ipcr.online && ipcr.channel == channel)
						break;
				}
			}
		} else if (iec61883_get_iPCRX (handle, input, &ipcr, iplug) < 0)
			FAIL ("Failed to get plug %d for input node", iplug);
		
		if (oplug != ompr.n_plugs) {
			if (opcr.n_p2p_connections > 0) {
				opcr.n_p2p_connections--;
				if (opcr.n_p2p_connections == 0)
					opcr.channel = ompr.bcast_channel;
				result = iec61883_set_oPCRX (handle, output, opcr, oplug);
				if (result == 0) {
					if (opcr.n_p2p_connections == 0) {
						// release channel and bandwidth
						result = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
						if (result == 0)
							result = raw1394_bandwidth_modify (handle, bandwidth, RAW1394_MODIFY_FREE);
					}
				}
			} else if (opcr.bcast_connection == 1) {
				// XXX: only node which established broadcast connection is
				// really allowed to tear it down. But we want to handle the
				// more common case of repeated use of iec61883_cmp_connect() 
				// and iec61883_cmp_disconnect() in simple scenarios
				// where we want some simple way to release the bandwidth
				// and channel.
				opcr.bcast_connection = 0;
				result = iec61883_set_oPCRX (handle, output, opcr, oplug);
				if (result == 0) {
					// release channel and bandwidth
					result = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
					if (result == 0)
						result = raw1394_bandwidth_modify (handle, bandwidth, RAW1394_MODIFY_FREE);
				}
			}
		}
		if (iplug != impr.n_plugs) {
			if (ipcr.n_p2p_connections > 0) {
				ipcr.n_p2p_connections--;
				// receiver connection count does not affect iso resource management
				result = iec61883_set_iPCRX (handle, input, ipcr, iplug);
			} else if (ipcr.bcast_connection == 1) {
				ipcr.bcast_connection = 0;
				ipcr.channel = ompr.bcast_channel;
				result = iec61883_set_iPCRX (handle, input, ipcr, iplug);
			}
		}
		if (oplug == ompr.n_plugs && iplug == impr.n_plugs)
			result = -1;
			
	} else if (ompr.n_plugs > 0) {
		// establish or overlay half point-to-point on output
		
		// determine if output has plug available
		if (oplug < 0) {
			for (oplug = 0; oplug < ompr.n_plugs; oplug++) {
				if (iec61883_get_oPCRX (handle, output, &opcr, oplug) == 0) {
					if (opcr.online && opcr.channel == channel)
						break;
				}
			}
		} else if (iec61883_get_oPCRX (handle, output, &opcr, oplug) < 0)
			FAIL ("Failed to get plug %d for output node", oplug);
		
		if (oplug != ompr.n_plugs) {
			if (opcr.n_p2p_connections > 0) {
				opcr.n_p2p_connections--;
				if (opcr.n_p2p_connections == 0)
					opcr.channel = ompr.bcast_channel;
				result = iec61883_set_oPCRX (handle, output, opcr, oplug);
				if (result == 0 && opcr.n_p2p_connections == 0) {
					// release channel and bandwidth
					result = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
					if (result == 0)
						result = raw1394_bandwidth_modify (handle, bandwidth, RAW1394_MODIFY_FREE);
				}
			} else if (opcr.bcast_connection == 1) {
				// XXX: only node which established broadcast connection is
				// really allowed to tear it down. But we want to handle the
				// more common case of repeated use of iec61883_cmp_connect() 
				// and iec61883_cmp_disconnect() in simple scenarios
				// where we want some simple way to release the bandwidth
				// and channel.
				opcr.bcast_connection = 0;
				result = iec61883_set_oPCRX (handle, output, opcr, oplug);
				if (result == 0) {
					// release channel and bandwidth
					result = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
					if (result == 0)
						result = raw1394_bandwidth_modify (handle, bandwidth, RAW1394_MODIFY_FREE);
				}
			}
		} else {
			// release channel and bandwidth
			result = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
			if (result == 0)
				result = raw1394_bandwidth_modify (handle, bandwidth, RAW1394_MODIFY_FREE);
		}
		
	} else if (impr.n_plugs > 0) {
		// establish or overlay half point-to-point on input
		
		// determine if input has plug available
		if (iplug < 0) {
			for (iplug = 0; iplug < impr.n_plugs; iplug++) {
				if (iec61883_get_iPCRX (handle, input, &ipcr, iplug) == 0) {
					if (ipcr.online && ipcr.channel == channel)
						break;
				}
			}
		} else if (iec61883_get_iPCRX (handle, input, &ipcr, iplug) < 0)
			FAIL ("Failed to get plug %d for input node", iplug);
		
		if (iplug != impr.n_plugs) {
			if (ipcr.n_p2p_connections > 0) {
				ipcr.n_p2p_connections--;
				if (ipcr.n_p2p_connections == 0)
					ipcr.channel = 63;
				// Normally, changes on receiver connection count does not 
				// affect iso resource management. However, in this special
				// half-way management mode, we need to in order to allow
				// multiple capture sessions.
				result = iec61883_set_iPCRX (handle, input, ipcr, iplug);
				if (result == 0 && ipcr.n_p2p_connections == 0) {
					// release channel and bandwidth
					result = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
					if (result == 0)
						result = raw1394_bandwidth_modify (handle, bandwidth, RAW1394_MODIFY_FREE);
				}
			} else if (ipcr.bcast_connection == 1) {
				// XXX: only node which established broadcast connection is
				// really allowed to tear it down. But we want to handle the
				// more common case of repeated use of iec61883_cmp_connect() 
				// and iec61883_cmp_disconnect() in simple scenarios
				// where we want some simple way to release the bandwidth
				// and channel.
				ipcr.bcast_connection = 0;
				result = iec61883_set_iPCRX (handle, input, ipcr, iplug);
				if (result == 0) {
					// release channel and bandwidth
					result = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
					if (result == 0)
						result = raw1394_bandwidth_modify (handle, bandwidth, RAW1394_MODIFY_FREE);
				}
			}
		} else {
			// release channel and bandwidth
			result = raw1394_channel_modify (handle, channel, RAW1394_MODIFY_FREE);
			if (result == 0)
				result = raw1394_bandwidth_modify (handle, bandwidth, RAW1394_MODIFY_FREE);
		}
			
	} else {
		// no input or output plugs - failover broadcast on channel 63
		// just release channel
		result = raw1394_channel_modify (handle, 63, RAW1394_MODIFY_FREE);
	}
	
	return result;
}

int
iec61883_cmp_normalize_output (raw1394handle_t handle, nodeid_t node)
{
	struct iec61883_oMPR ompr;
	struct iec61883_oPCR opcr;
	int oplug;
	int result = 0;

	DEBUG ("iec61883_cmp_normalize_output: node %d\n", (int) node & 0x3f);

	// Check for plugs on output
	result = iec61883_get_oMPR (handle, node, &ompr);
	if (result < 0)
		return result;
	
	// locate an ouput plug that has a connection
	for (oplug = 0; oplug < ompr.n_plugs; oplug++) {
		if (iec61883_get_oPCRX (handle, node, &opcr, oplug) == 0) {
			if (opcr.online && (opcr.n_p2p_connections > 0 || 
				                opcr.bcast_connection == 1)) {

				// Make sure the plug's channel is allocated with IRM
				quadlet_t buffer;
				nodeaddr_t addr = CSR_REGISTER_BASE;
				unsigned int c = opcr.channel;
				quadlet_t compare, swap = 0;
				quadlet_t new;
				
				if (c > 31 && c < 64) {
					addr += CSR_CHANNELS_AVAILABLE_LO;
					c -= 32;
				} else if (c < 64)
					addr += CSR_CHANNELS_AVAILABLE_HI;
				else
					FAIL ("Invalid channel");
				c = 31 - c;

				result = iec61883_cooked_read (handle, raw1394_get_irm_id (handle), addr, 
					sizeof (quadlet_t), &buffer);
				if (result < 0)
					FAIL ("Failed to get channels available.");
				
				buffer = ntohl (buffer);
				DEBUG ("channels available before: 0x%08x", buffer);

				if ((buffer & (1 << c)) != 0) {
					swap = htonl (buffer & ~(1 << c));
					compare = htonl (buffer);

					result = raw1394_lock (handle, raw1394_get_irm_id (handle), addr,
							   EXTCODE_COMPARE_SWAP, swap, compare, &new);
					if ( (result < 0) || (new != compare) ) {
						FAIL ("Failed to modify channel %d", opcr.channel);
					}
					DEBUG ("channels available after: 0x%08x", ntohl (swap));
				}
			}
		}
	}
		
	return result;
}
