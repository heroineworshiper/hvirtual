/*
 * CINELERRA
 * Copyright (C) 2010-2024 Adam Williams <broadcast at earthling dot net>
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

#ifndef BLURWINDOW_H
#define BLURWINDOW_H


class BlurThread;
class BlurWindow;

#include "blur.inc"
#include "filexml.inc"
#include "guicast.h"
#include "mutex.h"
#include "thread.h"


class BlurValue;
class BlurValueText;
class BlurA;
class BlurR;
class BlurG;
class BlurB;
class BlurAKey;

class BlurWindow : public PluginClientWindow
{
public:
	BlurWindow(BlurMain *client);
	~BlurWindow();
	
	void create_objects();

    void sync_values(BlurValue *pot_src);
	BlurMain *client;
//	BlurVertical *vertical;
//	BlurHorizontal *horizontal;
	BlurValue *h;
	BlurValueText *h_text;
	BlurValue *v;
	BlurValueText *v_text;
	BlurA *a;
	BlurR *r;
	BlurG *g;
	BlurB *b;
	BlurAKey *a_key;
    BC_Hash *defaults;
};

class BlurToggle : public BC_CheckBox
{
public:
	BlurToggle(BlurMain *client, 
		int *output, 
		int x, 
		int y,
		const char *text);
	int handle_event();

	BlurMain *client;
	int *output;
};

class BlurAKey : public BC_CheckBox
{
public:
	BlurAKey(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};

class BlurA : public BC_CheckBox
{
public:
	BlurA(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};
class BlurR : public BC_CheckBox
{
public:
	BlurR(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};
class BlurG : public BC_CheckBox
{
public:
	BlurG(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};
class BlurB : public BC_CheckBox
{
public:
	BlurB(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};


class BlurValue : public BC_FPot
{
public:
	BlurValue(BlurMain *client, 
        BlurWindow *gui, 
        int x, 
        int y, 
        float *output);
	int handle_event();
	BlurMain *client;
	BlurWindow *gui;
    BlurValueText *text;
    float *output;
};

class BlurValueText : public BC_TextBox
{
public:
	BlurValueText(BlurMain *client, 
        BlurWindow *gui, 
        int x, 
        int y, 
        int w, 
        float *output);
	int handle_event();
	BlurMain *client;
	BlurWindow *gui;
    BlurValue *pot;
    float *output;
};

class BlurVertical : public BC_CheckBox
{
public:
	BlurVertical(BlurMain *client, BlurWindow *window, int x, int y);
	~BlurVertical();
	int handle_event();

	BlurMain *client;
	BlurWindow *window;
};

class BlurHorizontal : public BC_CheckBox
{
public:
	BlurHorizontal(BlurMain *client, BlurWindow *window, int x, int y);
	~BlurHorizontal();
	int handle_event();

	BlurMain *client;
	BlurWindow *window;
};


#endif
