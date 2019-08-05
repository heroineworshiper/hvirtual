
/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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

#define TOTAL_PARAMS 9

class ReverbWindow;
class ReverbParam;


#include "eqcanvas.inc"
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
class ReverbSize;

class ReverbWindow : public PluginClientWindow
{
public:
	ReverbWindow(Reverb *reverb);
	~ReverbWindow();
	
	void create_objects();
    void update();
	void update_canvas();

	Reverb *reverb;
    
    ReverbParam *params[TOTAL_PARAMS];
    
	ReverbParam *level_init;
	ReverbParam *delay_init;
	ReverbParam *ref_level1;
	ReverbParam *ref_level2;
	ReverbParam *ref_total;
	ReverbParam *ref_length;
	ReverbParam *high;
	ReverbParam *low;
    ReverbParam *q;
    EQCanvas *canvas;
    ReverbSize *size;
};

class ReverbSize : public BC_PopupMenu
{
public:
	ReverbSize(ReverbWindow *window, Reverb *plugin, int x, int y);

	int handle_event();
	void create_objects();         // add initial items
	void update(int size);

	ReverbWindow *window;
	Reverb *plugin;
};





class ReverbFPot : public BC_FPot
{
public:
    ReverbFPot(ReverbParam *param, int x, int y);
    int handle_event();
	ReverbParam *param;
};

class ReverbIPot : public BC_IPot
{
public:
    ReverbIPot(ReverbParam *param, int x, int y);
    int handle_event();
	ReverbParam *param;
};

class ReverbQPot : public BC_QPot
{
public:
    ReverbQPot(ReverbParam *param, int x, int y);
    int handle_event();
	ReverbParam *param;
};

class ReverbText : public BC_TextBox
{
public:
    ReverbText(ReverbParam *param, int x, int y, int w, int value);
    ReverbText(ReverbParam *param, int x, int y, int w, float value);
    int handle_event();
	ReverbParam *param;
};

class ReverbParam
{
public:
    ReverbParam(Reverb *reverb,
        ReverbWindow *gui,
        int x, 
        int x2,
        int x3,
        int y, 
        int text_w,
        int *output_i, 
        float *output_f, // floating point output
        int *output_q, // frequency output
        const char *title,
        float min,
        float max);
    ~ReverbParam();
    
    void initialize();
    void update(int skip_text, int skip_pot);

// 2 possible outputs
    float *output_f;
    ReverbFPot *fpot;
    
    int *output_i;
    ReverbIPot *ipot;
    
    int *output_q;
    ReverbQPot *qpot;
    
    string title;
    ReverbText *text;
    ReverbWindow *gui;
    Reverb *reverb;
    int x;
    int x2;
    int x3;
    int y;
    int text_w;
    float min;
    float max;
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
