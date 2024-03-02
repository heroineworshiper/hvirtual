/*
 * CINELERRA
 * Copyright (C) 2024 Adam Williams <broadcast at earthling dot net>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

// create EDLs from various sources

#ifndef EDLFACTORY_H
#define EDLFACTORY_H


#include "asset.inc"
#include "edl.inc"
#include "edlfactory.inc"
#include "recordlabel.inc"


class EDLFactory
{
public:
    EDLFactory();
    
    static EDLFactory instance;
    static int asset_to_edl(EDL *new_edl, 
	    Asset *new_asset, 
	    RecordLabels *labels,
        int conform);

    static int asset_to_edl(EDL *new_edl, 
	    Asset *new_asset, 
	    RecordLabels *labels,
        int conform,
        int auto_aspect);

    static void transition_to_edl(EDL *edl, 
        const char *title,
        int data_type);
};


#endif

