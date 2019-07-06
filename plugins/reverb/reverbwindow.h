
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef REVERBWINDOW_H
#define REVERBWINDOW_H

#define TOTAL_LOADS 5

class ReverbThread;
class ReverbWindow;

#include "guicast.h"
#include "mutex.h"
#include "pluginclient.h"
#include "reverb.inc"






class ReverbLevelInit;
class ReverbDelayInit;
class ReverbRefLevel1;
class ReverbRefLevel2;
class ReverbRefTotal;
class ReverbRefLength;
class ReverbHigh;
class ReverbLow;
class ReverbQ;

class ReverbWindow : public PluginClientWindow
{
public:
	ReverbWindow(Reverb *reverb);
	~ReverbWindow();
	
	void create_objects();
	
	Reverb *reverb;
	ReverbLevelInit *level_init;
	ReverbDelayInit *delay_init;
	ReverbRefLevel1 *ref_level1;
	ReverbRefLevel2 *ref_level2;
	ReverbRefTotal *ref_total;
	ReverbRefLength *ref_length;
	ReverbHigh *high;
	ReverbLow *low;
    ReverbQ *q;
};

class ReverbLevelInit : public BC_FPot
{
public:
	ReverbLevelInit(Reverb *reverb, int x, int y);
	~ReverbLevelInit();
	int handle_event();
	Reverb *reverb;
};

class ReverbDelayInit : public BC_IPot
{
public:
	ReverbDelayInit(Reverb *reverb, int x, int y);
	~ReverbDelayInit();
	int handle_event();
	Reverb *reverb;
};

class ReverbRefLevel1 : public BC_FPot
{
public:
	ReverbRefLevel1(Reverb *reverb, int x, int y);
	~ReverbRefLevel1();
	int handle_event();
	Reverb *reverb;
};

class ReverbRefLevel2 : public BC_FPot
{
public:
	ReverbRefLevel2(Reverb *reverb, int x, int y);
	~ReverbRefLevel2();
	int handle_event();
	Reverb *reverb;
};

class ReverbRefTotal : public BC_IPot
{
public:
	ReverbRefTotal(Reverb *reverb, int x, int y);
	~ReverbRefTotal();
	int handle_event();
	Reverb *reverb;
};

class ReverbRefLength : public BC_IPot
{
public:
	ReverbRefLength(Reverb *reverb, int x, int y);
	~ReverbRefLength();
	int handle_event();
	Reverb *reverb;
};

class ReverbHigh : public BC_QPot
{
public:
	ReverbHigh(Reverb *reverb, int x, int y);
	~ReverbHigh();
	int handle_event();
	Reverb *reverb;
};

class ReverbLow : public BC_QPot
{
public:
	ReverbLow(Reverb *reverb, int x, int y);
	~ReverbLow();
	int handle_event();
	Reverb *reverb;
};

class ReverbQ: public BC_QPot
{
public:
	ReverbQ(Reverb *reverb, int x, int y);
	~ReverbQ();
	int handle_event();
	Reverb *reverb;
};




#endif
