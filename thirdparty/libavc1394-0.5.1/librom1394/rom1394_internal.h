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

#ifndef ROM1394_INTERNAL_H
#define ROM1394_INTERNAL_H 1

#include <stdint.h>
#include "rom1394.h"

#define QUADINC(x) x+=4
#define WARN(node, s, addr) fprintf(stderr,"rom1394_%u warning: %s: 0x%08x%08x\n", node, s, (int) (addr>>32), (int) addr)
#define QUADREADERR(handle, node, offset, buf) if(cooked1394_read(handle, (nodeid_t) 0xffc0 | node, (nodeaddr_t) offset, (size_t) sizeof(quadlet_t), (quadlet_t *) buf) < 0) WARN(node, "read failed", offset);
#define FAIL(node, s) {fprintf(stderr, "rom1394_%i error: %s\n", node, s);return(-1);}
#define NODECHECK(handle, node) \
	if ( ((int16_t) node < 0) || node >= raw1394_get_nodecount( (raw1394handle_t) handle)) FAIL(node,"invalid node"); 
#ifdef ROM1394_DEBUG
#define DEBUG(node, s, args...) \
printf("rom1394_%i debug: " s "\n", node, ## args)
#else
#define DEBUG(node, s, args...)
#endif

int
read_textual_leaf(raw1394handle_t handle, nodeid_t node, octlet_t offset,
    rom1394_directory *dir);

int
proc_directory (raw1394handle_t handle, nodeid_t node, octlet_t offset,
    rom1394_directory *dir);
    
uint16_t
make_crc (uint32_t *ptr, int length);

int
set_unit_directory(quadlet_t *buffer, rom1394_directory *dir);

int
set_textual_leaf(quadlet_t *buffer, const char *s);

int
add_textual_leaf(quadlet_t *buffer, const char *s);

int
get_leaf_size(quadlet_t *buffer);

int
get_unit_size(quadlet_t *buffer);

#endif
