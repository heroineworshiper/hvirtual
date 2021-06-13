/*
 * CINELERRA
 * Copyright (C) 1997-2021 Adam Williams <broadcast at earthling dot net>
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



#ifndef EDGEENGINE_H
#define EDGEENGINE_H


#include "loadbalance.h"
#include "vframe.h"



class EdgeEngine : public LoadServer
{
public:
	EdgeEngine(int total_clients, 
		int total_packages);
	~EdgeEngine();

	void init_packages();
	void process(VFrame *dst, VFrame *src, int amount);

	LoadClient* new_client();
	LoadPackage* new_package();
	VFrame *src, *dst;
    VFrame *tmp;
    int amount;
};


class EdgePackage : public LoadPackage
{
public:
	EdgePackage();
	int y1;
	int y2;
};

class EdgeUnit : public LoadClient
{
public:
	EdgeUnit(EdgeEngine *server);
	~EdgeUnit();
	float edge_detect(float *data, float max, int do_max);
	void process_package(LoadPackage *package);
	EdgeEngine *server;
};



#endif

