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



#ifndef COMPRESSORGUI_H
#define COMPRESSORGUI_H

#include "bchash.inc"
#include "compressor.h"
#include "compressortools.h"
#include "eqcanvas.inc"
#include "guicast.h"

class CompressorWindow;


class CompressorCanvas : public CompressorCanvasBase
{
public:
	CompressorCanvas(CompressorEffect *plugin, 
        CompressorWindow *window,
        int x, 
        int y, 
        int w, 
        int h);
    void update_window();
};

class CompressorBand : public BC_Radial
{
public:
    CompressorBand(CompressorWindow *window, 
        CompressorEffect *plugin, 
        int x, 
        int y,
        int number,
        char *text);
    int handle_event();
    
    CompressorWindow *window;
    CompressorEffect *plugin;
// 0 - (TOTAL_BANDS-1)
    int number;
};


class CompressorReaction : public BC_TumbleTextBox
{
public:
	CompressorReaction(CompressorEffect *plugin, 
        CompressorWindow *window, 
        int x, 
        int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorX : public BC_TumbleTextBox
{
public:
	CompressorX(CompressorEffect *plugin, CompressorWindow *window, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorY : public BC_TumbleTextBox
{
public:
	CompressorY(CompressorEffect *plugin, CompressorWindow *window, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorTrigger : public BC_TumbleTextBox
{
public:
	CompressorTrigger(CompressorEffect *plugin, CompressorWindow *window, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorDecay : public BC_TumbleTextBox
{
public:
	CompressorDecay(CompressorEffect *plugin, CompressorWindow *window, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};


class CompressorClear : public BC_GenericButton
{
public:
	CompressorClear(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorSmooth : public BC_CheckBox
{
public:
	CompressorSmooth(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorSolo : public BC_CheckBox
{
public:
	CompressorSolo(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorBypass : public BC_CheckBox
{
public:
	CompressorBypass(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorInput : public BC_PopupMenu
{
public:
	CompressorInput(CompressorEffect *plugin, int x, int y);
	void create_objects();
	int handle_event();
	static const char* value_to_text(int value);
	static int text_to_value(char *text);
	CompressorEffect *plugin;
};


class CompressorFPot : public BC_FPot
{
public:
    CompressorFPot(CompressorWindow *gui, 
        CompressorEffect *plugin, 
        int x, 
        int y, 
        double *output,
        double min,
        double max);
    int handle_event();
    CompressorWindow *gui;
    CompressorEffect *plugin;
    double *output;
};

class CompressorQPot : public BC_QPot
{
public:
    CompressorQPot(CompressorWindow *gui, 
        CompressorEffect *plugin, 
        int x, 
        int y, 
        int *output);
    int handle_event();
    CompressorWindow *gui;
    CompressorEffect *plugin;
    int *output;
};


class CompressorSize : public BC_PopupMenu
{
public:
	CompressorSize(CompressorWindow *gui, 
        CompressorEffect *plugin, 
        int x, 
        int y);

	int handle_event();
	void create_objects();         // add initial items
	void update(int size);
    CompressorWindow *gui;
    CompressorEffect *plugin;    
};


class CompressorWindow : public PluginClientWindow
{
public:
	CompressorWindow(CompressorEffect *plugin);
    ~CompressorWindow();
	void create_objects();
	void update();
// draw the dynamic range canvas
	void update_canvas();
// draw the bandpass canvas
    void update_eqcanvas();
	int resize_event(int w, int h);	
	
	CompressorCanvas *canvas;
	CompressorReaction *reaction;
	CompressorClear *clear;
	CompressorX *x_text;
	CompressorY *y_text;
	CompressorTrigger *trigger;
	CompressorDecay *decay;
	CompressorSmooth *smooth;
	CompressorSolo *solo;
    CompressorBypass *bypass;
	CompressorInput *input;
    BC_Meter *in;
    BC_Meter *gain_change;
    CompressorBand *band[TOTAL_BANDS];


	CompressorQPot *freq1;
	CompressorQPot *freq2;
    CompressorFPot *q;
    CompressorSize *size;
    EQCanvas *eqcanvas;

	CompressorEffect *plugin;
    BC_Hash *defaults;
};



#endif

