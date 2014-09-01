/*
 * deque.c -- double ended queue
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

#include "deque.h"

#include <stdlib.h>
#include <string.h>

/** Private structure.
*/

struct iec61883_deque
{
	void **list;
	int size;
	int count;
};

/** Create a deque.
*/

iec61883_deque_t iec61883_deque_init( )
{
	iec61883_deque_t this = malloc( sizeof( struct iec61883_deque ) );
	if ( this != NULL )
	{
		this->list = NULL;
		this->size = 0;
		this->count = 0;
	}
	return this;
}

/** Return the number of items in the deque.
*/

int iec61883_deque_size( iec61883_deque_t this )
{
	return this->count;
}

/** Allocate space on the deque.
*/

static int iec61883_deque_allocate( iec61883_deque_t this )
{
	if ( this->count == this->size )
	{
		this->list = realloc( this->list, sizeof( void * ) * ( this->size + 20 ) );
		this->size += 20;
	}
	return this->list == NULL;
}

/** Push an item to the end.
*/

int iec61883_deque_push_back( iec61883_deque_t this, void *item )
{
	int error = iec61883_deque_allocate( this );

	if ( error == 0 )
		this->list[ this->count ++ ] = item;

	return error;
}

/** Pop an item.
*/

void *iec61883_deque_pop_back( iec61883_deque_t this )
{
	return this->count > 0 ? this->list[ -- this->count ] : NULL;
}

/** Queue an item at the start.
*/

int iec61883_deque_push_front( iec61883_deque_t this, void *item )
{
	int error = iec61883_deque_allocate( this );

	if ( error == 0 )
	{
		memmove( &this->list[ 1 ], this->list, ( this->count ++ ) * sizeof( void * ) );
		this->list[ 0 ] = item;
	}

	return error;
}

/** Remove an item from the start.
*/

void *iec61883_deque_pop_front( iec61883_deque_t this )
{
	void *item = NULL;

	if ( this->count > 0 )
	{
		item = this->list[ 0 ];
		memmove( this->list, &this->list[ 1 ], ( -- this->count ) * sizeof( void * ) );
	}

	return item;
}

/** Get the item at back of deque but don't remove.
*/

void *iec61883_deque_back( iec61883_deque_t this )
{
	return this->count > 0 ? this->list[ this->count - 1 ] : NULL;
}

/** Get the item at front of deque but don't remove.
*/

void *iec61883_deque_front( iec61883_deque_t this )
{
	return this->count > 0 ? this->list[ 0 ] : NULL;
}

/** Close the queue.
*/

void iec61883_deque_close( iec61883_deque_t this )
{
	free( this->list );
	free( this );
}
