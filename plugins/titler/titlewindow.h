
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

#ifndef TITLEWINDOW_H
#define TITLEWINDOW_H

#include "guicast.h"

class TitleThread;
class TitleWindow;
class TitleInterlace;

#include "colorpicker.h"
#include "filexml.h"
#include "mutex.h"
#include "title.h"





class TitleFontTumble;
class TitleSizeTumble;
class TitleItalic;
class TitleBold;
class TitleSize;
class TitleEncoding;
class TitleColorButton;
class TitleDropShadow;
class TitleMotion;
class TitleLoop;
class TitleFade;
class TitleFont;
class TitleText;
class TitleX;
class TitleY;
class TitleLeft;
class TitleCenter;
class TitleRight;class TitleTop;
class TitleMid;
class TitleBottom;
class TitleColorThread;
class TitleSpeed;
class TitleTimecode;
class TitleTimecodeFormat;
class TitleOutline;

class TitleWindow : public PluginClientWindow
{
public:
	TitleWindow(TitleMain *client);
	~TitleWindow();
	
	void create_objects();
	int resize_event(int w, int h);
	void update_color();
	void update_justification();
	void update();
	void previous_font();
	void next_font();

	TitleMain *client;

	BC_Title *font_title;
	TitleFont *font;
	TitleFontTumble *font_tumbler;
	TitleSizeTumble *size_tumbler;
	BC_Title *x_title;
	TitleX *title_x;
	BC_Title *y_title;
	TitleY *title_y;
	BC_Title *dropshadow_title;
	TitleDropShadow *dropshadow;
	BC_Title *outline_title;
	TitleOutline *outline;
	BC_Title *style_title;
	TitleItalic *italic;
	TitleBold *bold;


	int color_x, color_y;
	int outline_color_x, outline_color_y;
	BC_Title *size_title;
	BC_Title *encoding_title;
	TitleSize *size;
	TitleEncoding *encoding;
	TitleColorButton *color_button;
	TitleColorThread *color_thread;
	TitleColorButton *outline_color_button;
	TitleColorThread *outline_color_thread;
	BC_Title *motion_title;
	TitleMotion *motion;
	TitleLoop *loop;
	BC_Title *fadein_title;
	TitleFade *fade_in;
	BC_Title *fadeout_title;
	TitleFade *fade_out;
	BC_Title *text_title;
	TitleText *text;
	BC_Title *justify_title;
	TitleLeft *left;
	TitleCenter *center;
	TitleRight *right;
	TitleTop *top;
	TitleMid *mid;
	TitleBottom *bottom;
	BC_Title *speed_title;
	TitleSpeed *speed;
	TitleTimecode *timecode;
	TitleTimecodeFormat *timecode_format;

// Color preview
	ArrayList<BC_ListBoxItem*> sizes;
	ArrayList<BC_ListBoxItem*> encodings;
	ArrayList<BC_ListBoxItem*> paths;
	ArrayList<BC_ListBoxItem*> fonts;
};


class TitleFontTumble : public BC_Tumbler
{
public:
	TitleFontTumble(TitleMain *client, TitleWindow *window, int x, int y);
	
	int handle_up_event();
	int handle_down_event();
	
	TitleMain *client;
	TitleWindow *window;
};


class TitleSizeTumble : public BC_Tumbler
{
public:
	TitleSizeTumble(TitleMain *client, TitleWindow *window, int x, int y);
	
	int handle_up_event();
	int handle_down_event();
	
	TitleMain *client;
	TitleWindow *window;
};



class TitleItalic : public BC_CheckBox
{
public:
	TitleItalic(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleBold : public BC_CheckBox
{
public:
	TitleBold(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};


class TitleSize : public BC_PopupTextBox
{
public:
	TitleSize(TitleMain *client, TitleWindow *window, int x, int y, char *text);
	~TitleSize();
	int handle_event();
	void update(int size);
	TitleMain *client;
	TitleWindow *window;
};
class TitleEncoding : public BC_PopupTextBox
{
public:
	TitleEncoding(TitleMain *client, TitleWindow *window, int x, int y);
	~TitleEncoding();
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleColorButton : public BC_GenericButton
{
public:
	TitleColorButton(TitleMain *client, 
		TitleWindow *window, 
		int x, 
		int y, 
		int is_outline);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
	int is_outline;
};

class TitleColorThread : public ColorThread
{
public:
	TitleColorThread(TitleMain *client, TitleWindow *window, int is_outline);
	virtual int handle_new_color(int output, int alpha);
	TitleMain *client;
	TitleWindow *window;
	int is_outline;
};


class TitleMotion : public BC_PopupTextBox
{
public:
	TitleMotion(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleLoop : public BC_CheckBox
{
public:
	TitleLoop(TitleMain *client, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleTimecode : public BC_CheckBox
{
public:
	TitleTimecode(TitleMain *client, int x, int y);
	int handle_event();
	TitleMain *client;
};

class TitleTimecodeFormat : public BC_PopupMenu
{
public:
	TitleTimecodeFormat(TitleMain *client, int x, int y, const char *text);
	int handle_event();
	TitleMain *client;
};

class TitleFade : public BC_TextBox
{
public:
	TitleFade(TitleMain *client, TitleWindow *window, double *value, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
	double *value;
};
class TitleFont : public BC_PopupTextBox
{
public:
	TitleFont(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleText : public BC_ScrollTextBox
{
public:
	TitleText(TitleMain *client, 
		TitleWindow *window, 
		int x, 
		int y, 
		int w, 
		int h);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleX : public BC_TumbleTextBox
{
public:
	TitleX(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleY : public BC_TumbleTextBox
{
public:
	TitleY(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleDropShadow : public BC_TumbleTextBox
{
public:
	TitleDropShadow(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleOutline : public BC_TumbleTextBox
{
public:
	TitleOutline(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleSpeed : public BC_TumbleTextBox
{
public:
	TitleSpeed(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
};

class TitleLeft : public BC_Radial
{
public:
	TitleLeft(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleCenter : public BC_Radial
{
public:
	TitleCenter(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleRight : public BC_Radial
{
public:
	TitleRight(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleTop : public BC_Radial
{
public:
	TitleTop(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleMid : public BC_Radial
{
public:
	TitleMid(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleBottom : public BC_Radial
{
public:
	TitleBottom(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

#endif
