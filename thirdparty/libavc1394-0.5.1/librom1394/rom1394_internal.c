/*
 * librom1394 - GNU/Linux 1394 CSR Config ROM Library
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

#include "rom1394_internal.h"
#include "rom1394.h"
#include "../common/raw1394util.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include <unistd.h>

/*
 * Read a textual leaf into a malloced ASCII string
 * TODO: This routine should probably care about character sets, Unicode, etc.
 * IN:		phyID:	Physical ID of the node to read from
 *		offset:	Memory offset to read from
 * RETURNS:	pointer to a freshly malloced string that contains the
 *		requested text or NULL if the text could not be read.
 */
int read_textual_leaf(raw1394handle_t handle, nodeid_t node, octlet_t offset,
    rom1394_directory *dir) 
{
	int i, length;
	char *s;
	quadlet_t quadlet;
	quadlet_t language_spec;	// language specifier
	quadlet_t charset_spec;		// character set specifier

	DEBUG(node, "reading textual leaf: 0x%08x%08x\n", (unsigned int) (offset>>32), 
	    (unsigned int) offset&0xFFFFFFFF);

	QUADREADERR(handle, node, offset, &quadlet);
	quadlet = htonl(quadlet);
	length = ((quadlet >> 16) - 2) * 4;
	DEBUG(node, "textual leaf length: %i (0x%08X) %08x %08x\n", length, length, quadlet, htonl (quadlet));

	if (length<=0 || length > 256) {
	    WARN(node, "invalid number of textual leaves", offset);
	    return -1;
	}

	QUADINC(offset);
	QUADREADERR(handle, node, offset, &language_spec);
	language_spec = htonl(language_spec);
	/* assert language specifier=0 */
	if (language_spec != 0) {
		if (!(language_spec & 0x80000000)) 
			WARN(node, "unimplemented language for textual leaf", offset);
	}

	QUADINC(offset);
	QUADREADERR(handle, node, offset, &charset_spec);
	charset_spec = htonl(charset_spec);
	/* assert character set =0 */
	if (charset_spec != 0) {
		if (charset_spec != 0x409) 					// US_ENGLISH (unicode) Microsoft format leaf
			WARN(node, "unimplemented character set for textual leaf", offset);
	}

	if ((s = (char *) malloc(length+1)) == NULL)
		FAIL( node, "out of memory");

	if (!dir->max_textual_leafs) {
		if (!(dir->textual_leafs = (char **) calloc (1, sizeof (char *)))) {
			FAIL( node, "out of memory");
		}
		dir->max_textual_leafs = 1;
	}

	if (dir->nr_textual_leafs == dir->max_textual_leafs) {
		if (!(dir->textual_leafs = (char **) realloc (dir->textual_leafs, 
		    dir->max_textual_leafs * 2 * sizeof (char *)))) {
			FAIL( node, "out of memory");
		}
		dir->max_textual_leafs *= 2;
	}

	for (i=0; i<length; i++) {
		QUADINC(offset);
		QUADREADERR(handle, node, offset, &quadlet);
		quadlet = htonl(quadlet);
		if (charset_spec == 0) {
			s[i] = quadlet>>24;
			if (++i < length) s[i] = (quadlet>>16)&0xFF;
			else break;
			if (++i < length) s[i] = (quadlet>>8)&0xFF;
			else break;
			if (++i < length) s[i] = (quadlet)&0xFF;
			else break;
		} else {
			if (charset_spec == 0x409) {
				s[i] = (quadlet>>24) & 0xFF;
				if (++i < length) s[i] = (quadlet>>8) & 0xFF;
				else break;
			}
		}
	}
	s[i] = '\0';
    DEBUG( node, "textual leaf is: (%s)\n", s);
	dir->textual_leafs[dir->nr_textual_leafs++] = s;
	return 0;
}

int proc_directory (raw1394handle_t handle, nodeid_t node, octlet_t offset,
    rom1394_directory *dir)
{
	int		length, i, key, value;
	quadlet_t 	quadlet;
	octlet_t	subdir, selfdir;
	
	selfdir = offset;
	
	QUADREADERR(handle, node, offset, &quadlet);
	if (cooked1394_read(handle, (nodeid_t) 0xffc0 | node, offset, sizeof(quadlet_t), &quadlet) < 0) {
		return -1;
	} else {
		quadlet = htonl(quadlet);
		length = quadlet>>16;
	
		DEBUG(node, "directory has %d entries\n", length);
		for (i=0; i<length; i++) {
			QUADINC(offset);
			QUADREADERR(handle, node, offset, &quadlet);
			quadlet = htonl(quadlet);
			key = quadlet>>24;
			value = quadlet&0x00FFFFFF;
			DEBUG(node, "key/value: %08x/%08x\n", key, value);
			switch (key) {
				case 0x0C:
					dir->node_capabilities = value;
					break;
				case 0x03:
					dir->vendor_id = value;
					break;
				case 0x12:
					dir->unit_spec_id = value;
					break;
				case 0x13:
					dir->unit_sw_version = value;
					break;
				case 0x17:
					dir->model_id = value;
					break;
				case 0x81: // ASCII textual leaf offset
				case 0x82:
					if (value != 0)
						read_textual_leaf( handle, node, offset + value * 4, dir);
					break;
				case 0xC1: // Descriptor directory
				case 0xC3: // vendor directory
				case 0xC7: // Module directory
				case 0xD1: // Unit directory
				case 0xD4:
				case 0xD8:
					subdir = offset + value*4;
					if (subdir > selfdir) {
						if ( proc_directory( handle, node, subdir, dir) < 0 )
							FAIL(node, "failed to read sub directory" );
					} else {
						FAIL(node, "unit directory with back reference");
					}
					break;
			}
		}
	}
	return 0;
}

uint16_t make_crc (uint32_t *ptr, int length)
{
	int shift;
	uint32_t crc, sum, data;

	crc = 0;
	for (; length > 0; length--) {
		data = ntohl(*ptr++);
		for (shift = 28; shift >= 0; shift -= 4) {
			sum = ((crc >> 12) ^ (data >> shift)) & 0x000f;
			crc = (crc << 4) ^ (sum << 12) ^ (sum << 5) ^ sum;
		}
		crc &= 0xffff;
	}
	return crc;
}

int set_unit_directory(quadlet_t *buffer, rom1394_directory *dir)
{
	int i, length;
	quadlet_t *p = buffer;
	int	key, value;
	quadlet_t quadlet;

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
			case 0x12:
				if (dir->unit_spec_id != -1)
				{
					value = (key << 24) | (dir->unit_spec_id & 0x00FFFFFF);
					*p = htonl(value);
				}
				break;
			case 0x13:
				if (dir->unit_sw_version != -1)
				{
					value = (key << 24) | (dir->unit_sw_version & 0x00FFFFFF);
					*p = htonl(value);
				}
				break;
			case 0x81:
			case 0x82:
				// TODO: unit textual leaves.
				//if (n < dir->nr_textual_leaves)
				//	set_textual_leaf( p + value, dir->textual_leaves[n++]);
				break;
		}
	}
	
	/* compute CRC for unit */
	p = buffer;
	quadlet = (length << 16);
	quadlet |= make_crc(p + 1, length) & 0x0000FFFF;
	*p = htonl(quadlet);

	return 0;
}

/* currently does not extend length of textual leaf */
int
set_textual_leaf(quadlet_t *buffer, const char *s)
{
	int i, length;
	quadlet_t *p = buffer, *q = (quadlet_t*)s;
	quadlet_t quadlet;
	int n_q; /* number of existing quadlets */
	
	quadlet = ntohl(*p);
	n_q = quadlet >> 16;
	
	/* set to simple ascii */
	p++; *p++ = 0; *p++ = 0;
	
	length = (strlen(s) + 3) /4;
	for (i = 0; i < length && i < n_q-2; i++)
		*p++ = q[i];
	
	/* compute CRC for leaf */
	p = buffer;
	quadlet = (n_q << 16);
	quadlet |= make_crc(p + 1, n_q) & 0x0000FFFF;
	*p = htonl(quadlet);
	
	return 0;
}

/* make sure buffer is pointing to the end of the current rom image! */
/* returns the number of new quadlets */
int
add_textual_leaf(quadlet_t *buffer, const char *s)
{
	int i, length;
	quadlet_t *p = buffer, *q = (quadlet_t*) s;
	quadlet_t quadlet;
	int n_q = 3; /* number of quadlets added */

	/* set to simple ascii */
	p++; *p++ = 0; *p++ = 0;
	
	length = (strlen(s) + 3) /4;
	n_q += length;
	for (i = 0; i < length; i++)
		*p++ = q[i];
	
	/* compute CRC for leaf */
	p = buffer;
	quadlet = ((n_q-1) << 16);
	quadlet |= make_crc(p + 1, n_q-1) & 0x0000FFFF;
	*p = htonl(quadlet);

	return n_q;
}

int get_leaf_size(quadlet_t *buffer)
{
	int length;
	quadlet_t quadlet;

	quadlet = ntohl(*buffer);
	length = quadlet >> 16;
	return length + 1;
}

int get_unit_size(quadlet_t *buffer)
{
	int i, length;
	quadlet_t *p = buffer;
	int	key, value;
	quadlet_t quadlet;
	int x = 1;

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
			case 0x81: // textual leaf
				if (value > 0)
					x += get_leaf_size(p + value);
				break;
		}
	}
	
	return x;
}
