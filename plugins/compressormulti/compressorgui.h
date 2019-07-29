#ifndef COMPRESSORGUI_H
#define COMPRESSORGUI_H

#include "compressor.h"
#include "eqcanvas.inc"
#include "guicast.h"

class CompressorWindow;


class CompressorCanvas : public BC_SubWindow
{
public:
	CompressorCanvas(CompressorEffect *plugin, int x, int y, int w, int h);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();


	enum
	{
		NONE,
		DRAG
	};

	int current_point;
	int current_operation;
	CompressorEffect *plugin;
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


class CompressorReaction : public BC_TextBox
{
public:
	CompressorReaction(CompressorEffect *plugin, int x, int y);
	int handle_event();
	int button_press_event();
	CompressorEffect *plugin;
};

class CompressorClear : public BC_GenericButton
{
public:
	CompressorClear(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorX : public BC_TextBox
{
public:
	CompressorX(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorY : public BC_TextBox
{
public:
	CompressorY(CompressorEffect *plugin, int x, int y);
	int handle_event();
	CompressorEffect *plugin;
};

class CompressorTrigger : public BC_TextBox
{
public:
	CompressorTrigger(CompressorEffect *plugin, int x, int y);
	int handle_event();
	int button_press_event();
	CompressorEffect *plugin;
};

class CompressorDecay : public BC_TextBox
{
public:
	CompressorDecay(CompressorEffect *plugin, int x, int y);
	int handle_event();
	int button_press_event();
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
	void draw_scales();
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
    CompressorBand *band[TOTAL_BANDS];


	CompressorQPot *freq;
    CompressorFPot *q;
    CompressorSize *size;
    EQCanvas *eqcanvas;

	CompressorEffect *plugin;
};



#endif

