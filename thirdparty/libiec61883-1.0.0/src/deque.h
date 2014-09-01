/*
 * deque.h -- double ended queue
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 * Copied from mlt: http://www.sf.net/projects/mlt
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


#ifndef _IEC61883_DEQUE_H
#define _IEC61883_DEQUE_H

typedef struct iec61883_deque* iec61883_deque_t;

#ifdef __cplusplus
extern "C" {
#endif
	
iec61883_deque_t iec61883_deque_init( );
int iec61883_deque_size( iec61883_deque_t self );
int iec61883_deque_push_back( iec61883_deque_t self, void *item );
void *iec61883_deque_pop_back( iec61883_deque_t self );
int iec61883_deque_push_front( iec61883_deque_t self, void *item );
void *iec61883_deque_pop_front( iec61883_deque_t self );
void *iec61883_deque_back( iec61883_deque_t self );
void *iec61883_deque_front( iec61883_deque_t self );
void iec61883_deque_close( iec61883_deque_t self );

#ifdef __cplusplus
}
#endif

#endif /* _IEC61883_DEQUE_H */
