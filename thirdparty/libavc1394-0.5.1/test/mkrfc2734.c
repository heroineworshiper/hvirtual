/*
 * mkrfc2734.c - A utility to set the configROM of a Linux host running eth1394
 *
 * Copyright 2003 Dan Dennedy <dan@dennedy.org>
 *
 * This library is licensed under the GNU Lesser General Public License (LGPL),
 * version 2.1 or later. See the file COPYING.LIB in the distribution for
 * details.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#include "../librom1394/rom1394.h"

const char not_compatible[] = 
"This libraw1394 does not work with your version of Linux. You need a different\n"
"version that matches your kernel (see kernel help text for the raw1394 option to\n"
"find out which is the correct version).\n";

const char not_loaded[] = 
"This probably means that you don't have raw1394 support in the kernel or that\n"
"you haven't loaded the raw1394 module.\n";

int main(int argc, char **argv)
{
	raw1394handle_t handle;
	int retval;
	quadlet_t rom[0x100];
	size_t rom_size;
	unsigned char rom_version;
	rom1394_directory dir;
	char *(leaf[2]);
	
	handle = raw1394_new_handle();
	
	if (!handle) {
		if (!errno) {
				printf(not_compatible);
		} else {
				perror("couldn't get handle");
				printf(not_loaded);
		}
		exit(EXIT_FAILURE);
	}
	
	if (raw1394_set_port(handle, 0) < 0) {
		perror("couldn't set port");
		exit(EXIT_FAILURE);
	}
	
	/* get the current rom image */
	retval=raw1394_get_config_rom(handle, rom, 0x100, &rom_size, &rom_version);
	rom_size = rom1394_get_size(rom);
	printf("get_config_rom returned %d, romsize %d, rom_version %d:",retval,rom_size,rom_version);
	
	/* get the local directory */
	rom1394_get_directory( handle, raw1394_get_local_id(handle) & 0x3f, &dir);
	
	/* free the allocated mem for the textual leaves */
	rom1394_free_directory( &dir);
	
	/* add an RFC 2734 unit directory */
	dir.unit_spec_id    = 0x0000005e;
	dir.unit_sw_version = 0x00000001;
	leaf[0] = "IANA";
    leaf[1] = "IPv4";
	dir.nr_textual_leafs = 2;
	dir.textual_leafs = leaf;
	
	/* manipulate the rom */
	retval = rom1394_add_unit( rom, &dir);
	
	/* get the computed size of the rom image */
	rom_size = rom1394_get_size(rom);
	
	printf("rom1394_add_unit_directory returned %d, romsize %d:",retval,rom_size);
	
	/* convert computed rom size from quadlets to bytes before update */
	rom_size *= sizeof(quadlet_t);
	retval = raw1394_update_config_rom(handle, rom, rom_size, rom_version);
	printf("update_config_rom returned %d\n",retval);
	printf("You need to reload your ieee1394 modules to reset the rom.\n");
    
    raw1394_reset_bus(handle);
	
	exit(EXIT_SUCCESS);
}
