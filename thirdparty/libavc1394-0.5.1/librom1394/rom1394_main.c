/*
 * librom1394 - GNU/Linux IEEE 1394 CSR Config ROM Library
 *
 * Originally written by Andreas Micklei <andreas.micklei@ivistar.de>
 * Better directory and textual leaf processing provided by Stefan Lucke
 * Libtoolize-d and modifications by Dan Dennedy <dan@dennedy.org>
 * ROM manipulation routines by Dan Dennedy
 * Currently maintained by Dan Dennedy
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

#include "rom1394.h"
#include "rom1394_internal.h"
#include "../common/raw1394util.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#define MAXLINE 80
#define MAX_OFFSETS	256

int rom1394_get_bus_info_block_length(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t 	quadlet;
	octlet_t 	offset;
	int 		length;

	NODECHECK(handle, node);
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_HEADER;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);
	length = quadlet >> 24;
	if (length != 4)
		WARN (node, "wrong bus info block length", offset);
	return length;
}
	
quadlet_t rom1394_get_bus_id(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t 	quadlet;
	octlet_t 	offset;

	NODECHECK(handle, node);
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_BUS_ID;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);

	if (quadlet != 0x31333934)
		WARN (node, "invalid bus id", offset);
    return quadlet;
}

int rom1394_get_bus_options(raw1394handle_t handle, nodeid_t node, rom1394_bus_options* bus_options)
{
	quadlet_t 	quadlet;
	octlet_t 	offset;

	NODECHECK(handle, node);
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_BUS_OPTIONS;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);
	bus_options->irmc = quadlet >> 31;
	bus_options->cmc = (quadlet >> 30) & 1;
	bus_options->isc = (quadlet >> 29) & 1;
	bus_options->bmc = (quadlet >> 28) & 1;
	bus_options->cyc_clk_acc = (quadlet >> 16) & 0xFF;
	bus_options->max_rec = (quadlet >> 12) & 0xF;
	bus_options->max_rec = pow( 2, bus_options->max_rec+1);
	return 0;
}

octlet_t rom1394_get_guid(raw1394handle_t handle, nodeid_t node)
{
	quadlet_t 	quadlet;
	octlet_t 	offset;
	octlet_t    guid = 0;

	NODECHECK(handle, node);
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_GUID_HI;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);
	guid = quadlet;
	guid <<= 32;
	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_GUID_LO;
	QUADREADERR (handle, node, offset, &quadlet);
	quadlet = htonl (quadlet);
	guid += quadlet;

    return guid;
}

int rom1394_get_directory(raw1394handle_t handle, nodeid_t node, rom1394_directory *dir)
{
	octlet_t 	offset;
	int i, j;
	char *p;
	int result = 0;

	NODECHECK(handle, node);
    dir->node_capabilities = 0;
    dir->vendor_id = 0;
    dir->unit_spec_id = 0;
    dir->unit_sw_version = 0;
    dir->model_id = 0;
	dir->max_textual_leafs = dir->nr_textual_leafs = 0;
	dir->label = NULL;
	dir->textual_leafs = NULL;

	offset = CSR_REGISTER_BASE + CSR_CONFIG_ROM + ROM1394_ROOT_DIRECTORY;
	if ( ( result = proc_directory (handle, node, offset, dir) ) != -1 )
	{
		 /* Calculate label */
		if (dir->nr_textual_leafs != 0 && dir->textual_leafs[0]) {
			for (i = 0, j = 0; i < dir->nr_textual_leafs; i++)
				if (dir->textual_leafs[i]) j += (strlen(dir->textual_leafs[i]) + 1);
			if ( (dir->label = (char *) malloc(j)) ) {
				for (i = 0, p = dir->label; i < dir->nr_textual_leafs; i++, p++) {
					if (dir->textual_leafs[i]) {
						strcpy ( p, dir->textual_leafs[i]);
						p += strlen(dir->textual_leafs[i]);
						if (i < dir->nr_textual_leafs-1) p[0] = ' ';
					}
				}
			}
		}
	}
	return result;
}

/* ----------------------------------------------------------------------------
 * Get the type / protocol of a node
 * IN:		rom_info:	pointer to the Rom_info structure of the node
 * RETURNS:	one of the defined node types, i.e. NODE_TYPE_AVC, etc.
 */
rom1394_node_types rom1394_get_node_type(rom1394_directory *dir)
{
	if (dir->unit_spec_id == 0xA02D) {
		if (dir->unit_sw_version == 0x100) {
			return ROM1394_NODE_TYPE_DC;
        } else if (dir->unit_sw_version & 0x010000) {
			return ROM1394_NODE_TYPE_AVC;
		}
	} else if (dir->unit_spec_id == 0x609E && dir->unit_sw_version == 0x10483) {
		return ROM1394_NODE_TYPE_SBP2;
	}
	return ROM1394_NODE_TYPE_UNKNOWN;
}

void rom1394_free_directory(rom1394_directory *dir)
{
    int i;
    for (i = 0; dir->textual_leafs && i < dir->nr_textual_leafs; i++)
        if (dir->textual_leafs[i]) free(dir->textual_leafs[i]);
    if (dir->textual_leafs) free(dir->textual_leafs);
    dir->textual_leafs = NULL;
    dir->max_textual_leafs = dir->nr_textual_leafs = 0;
    if (dir->label) free(dir->label);
}


/****************** UPDATE CONFIG ROM IMAGE *******************************/

/* returns number of quadlets */
int rom1394_get_size(quadlet_t *buffer)
{
	int i, length;
	quadlet_t *p = buffer + ROM1394_ROOT_DIRECTORY/4;
	int	key, value;
	quadlet_t quadlet;
	int x = ROM1394_ROOT_DIRECTORY/4 + 1;

	quadlet = ntohl(*p);
	length = quadlet >> 16;
	x += length;
    
	DEBUG(-1, "directory has %d entries\n", length);
	for (i=0; i < length; i++) {
		p++;
		quadlet = ntohl(*p);
		key = quadlet >> 24;
   		value = quadlet & 0x00FFFFFF;
		DEBUG(-1, "key/value: %08x/%08x\n", key, value);
		
		switch (key) {
   			case 0xD1: // Unit directory
				if (value > 0)
					x += get_unit_size(p + value);
				break;
			case 0x81: // textual leaf
				if (value > 0)
					x += get_leaf_size(p + value);
				break;
		}
	}
	
	return x;
}


int rom1394_add_unit(quadlet_t *buffer, rom1394_directory *dir)
{	
	int i, length;
	quadlet_t *p = buffer + ROM1394_ROOT_DIRECTORY/4;
	int	key, value;
	quadlet_t quadlet;
	int offset;
	int n_q = 5; /* number of additional quadlets to add */
	int len = rom1394_get_size( buffer);
	
	if (dir->nr_textual_leafs > 0)
		n_q++; /* we only support one textual leaf per unit */
	
	/* get root dir length and move p to after root dir */
	quadlet = ntohl(*p);
	length = quadlet >> 16;
	p += length + 1;
	
	/* get the difference between current position and beginning */
	offset = (p - buffer);
	
	/* move the rest down */
	/* size = original length minus offset */
	memmove( p+n_q, p, (len-offset) * sizeof(quadlet_t) );
	len += n_q;
	
	/* reset p to beginning of root */
	p = buffer + ROM1394_ROOT_DIRECTORY/4;
	
	/* adjust offsets in root dir */
	for (i=0; i < length; i++) {
		p++;
		quadlet = ntohl(*p);
		key = quadlet >> 24;
		value = quadlet & 0x00FFFFFF;
		DEBUG(-1, "key/value: %08x/%08x\n", key, value);
		
		switch (key) {
			case 0xD1: // Unit directory
			case 0x81: // textual leaves
			case 0x82:
				value = (key << 24) | ((value + n_q) & 0x00FFFFFF);
				*p = htonl(value);
				break;
		}
	}
	
	/* add unit directory entry to root */
	p++;
	value = (0xD1 << 24) | 1;
	*p++ = htonl(value);
	
	/* make new unit directory */
	p++;
	value = (0x12 << 24) | (dir->unit_spec_id & 0x00FFFFFF);
	*p++ = htonl(value);
	value = (0x13 << 24) | (dir->unit_sw_version & 0x00FFFFFF);
	*p++ = htonl(value);
	value = (0x17 << 24) | (dir->model_id & 0x00FFFFFF);
	*p++ = htonl(value);
    
    /* TODO: process multiple leafs */
	for (i = 0; i < 1 /* dir->nr_textual_leafs */; i++)
	{
		value = (0x81 << 24) | (((buffer+len)-p) & 0x00FFFFFF);
		*p++ = htonl(value);
		len += add_textual_leaf( buffer + len, dir->textual_leafs[i]);
	}
	
	/* compute CRC for unit directory */
	p = buffer + offset + 1;
	quadlet = ((n_q-2) << 16);
	quadlet |= make_crc(p + 1, n_q-2) & 0x0000FFFF;
	*p = htonl(quadlet);
	
	/* increment root directory length */
	length++;
	
	/* compute CRC for root */
	p = buffer + ROM1394_ROOT_DIRECTORY/4;
	quadlet = (length << 16);
	quadlet |= make_crc(p + 1, length) & 0x0000FFFF;
	*p = htonl(quadlet);

	return 0;
}


int rom1394_set_directory(quadlet_t *buffer, rom1394_directory *dir)
{
	int i, length;
	quadlet_t *p = buffer + ROM1394_ROOT_DIRECTORY/4;
	int	key, value;
	quadlet_t quadlet;
	int n = 0; /* iterator thru textual leaves */

	quadlet = ntohl(*p);
	length = quadlet >> 16;
    
	DEBUG(-1, "directory has %d entries\n", length);
	for (i=0; i < length; i++) {
		p++;
		quadlet = ntohl(*p);
		key = quadlet >> 24;
   		value = quadlet & 0x00FFFFFF;
		DEBUG(-1, "key/value: %08x/%08x\n", key, value);
		
		switch (key) {
			case 0x03:
				if (dir->vendor_id != -1)
				{
					value = (key << 24) | (dir->vendor_id & 0x00FFFFFF);
					*p = htonl(value);
				}
				break;
			case 0x17:
				if (dir->model_id != -1)
				{
					value = (key << 24) | (dir->model_id & 0x00FFFFFF);
					*p = htonl(value);
				}
				break;
			case 0x0C:
				if (dir->node_capabilities != -1)
				{
					value = (key << 24) | (dir->node_capabilities & 0x00FFFFFF);
					*p = htonl(value);
				}
				break;
   			case 0xD1: // Unit directory
				/* update existing unit directory only */
				set_unit_directory( p + value, dir);
			case 0x81:
			case 0x82:
				if (n < dir->nr_textual_leafs)
					set_textual_leaf( p + value, dir->textual_leafs[n++]);
				break;
		}
	}
	
	/* compute CRC for root */
	p = buffer + ROM1394_ROOT_DIRECTORY/4;
	quadlet = (length << 16);
	quadlet |= make_crc(p + 1, length) & 0x0000FFFF;
	*p = htonl(quadlet);

	return 0;
}
