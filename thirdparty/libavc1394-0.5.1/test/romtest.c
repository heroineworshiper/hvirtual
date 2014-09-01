/*
 * librom1394 - GNU/Linux IEEE 1394 CSR Config ROM Library
 * romtest - a program to test the library and give info about your devices
 * 
 * Copyright 2001 by Dan Dennedy <dan@dennedy.org>
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

#include <config.h>

#include "../librom1394/rom1394.h"

#include <libraw1394/raw1394.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main (int argc, char *argv[])
{
	raw1394handle_t handle;
	int i, length;
	rom1394_bus_options bus_options;
	octlet_t guid;
	rom1394_directory dir;

#ifdef RAW1394_V_0_8
	handle = raw1394_get_handle();
#else
    handle = raw1394_new_handle();
#endif
    if (!handle) {
        if (!errno) {
            printf("Not Compatable!\n");
        } else {
            printf("\ncouldn't get handle\n");
            printf("Not Loaded!\n");
        }
        exit(1);
    } 


	if ( raw1394_set_port(handle, 0) < 0 ) {
		printf("couldn't set port\n");
		exit(1);
	}

    printf("Librom1394 Test Report\n");
    printf("=================================================\n");

    for (i=0; i < raw1394_get_nodecount(handle); ++i) {
        printf( "\nNode %d: \n", i);
        printf( "-------------------------------------------------\n");
        length = rom1394_get_bus_info_block_length(handle, i);
        printf("bus info block length = %d\n", length);
        printf("bus id = 0x%08x\n", rom1394_get_bus_id(handle, i));
        rom1394_get_bus_options(handle, i, &bus_options);
        printf("bus options:\n");
        printf("    isochronous resource manager capable: %d\n", bus_options.irmc);
        printf("    cycle master capable                : %d\n", bus_options.cmc);
        printf("    isochronous capable                 : %d\n", bus_options.isc);
        printf("    bus manager capable                 : %d\n", bus_options.bmc);
        printf("    cycle master clock accuracy         : %d ppm\n", bus_options.cyc_clk_acc);
        printf("    maximum asynchronous record size    : %d bytes\n", bus_options.max_rec);
        guid = rom1394_get_guid(handle, i);
        printf("GUID: 0x%08x%08x\n", (quadlet_t) (guid>>32), (quadlet_t) (guid & 0xffffffff));
        rom1394_get_directory( handle, i, &dir);
        printf("directory:\n");
        printf("    node capabilities    : 0x%08x\n", dir.node_capabilities);
        printf("    vendor id            : 0x%08x\n", dir.vendor_id);
        printf("    unit spec id         : 0x%08x\n", dir.unit_spec_id);
        printf("    unit software version: 0x%08x\n", dir.unit_sw_version);
        printf("    model id             : 0x%08x\n", dir.model_id);
        printf("    textual leaves       : %s\n", dir.label);

        rom1394_free_directory( &dir);
    }
    return 0;
}
