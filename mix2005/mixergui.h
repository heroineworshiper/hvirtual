#ifndef MIXERGUI_H
#define MIXERGUI_H


#include "arraylist.h"
#include "guicast.h"
#include "mixer.inc"
#include "mixergui.inc"
#include "mixerpopup.inc"


class MixerLock;
class MixerSlider;
class MixerPot;
class MixerToggle;
class MixerPopupMenu;
class MixerVScroll;
class MixerHScroll;

class MixerGUI : public BC_Window
{
public:
	MixerGUI(Mixer *mixer, int x, int y, int w, int h);
	~MixerGUI();

	void create_objects();
	int create_controls();
	int close_event();
	int keypress_event();
	int button_press_event();
	int translation_event();
	int resize_event(int w, int h);
	void update_display(int lock_it = 0);
	void update_values();
	void reposition_controls(int new_x, int new_y);
	int cursor_motion_event();

	int locked();

	int title_column_w;
	int control_column_w;
// Maximum dimensions of control block
	int control_h;
	int control_w;
	int pot_w;
	int menu_w;
// Affected channel if dragging toggle value
	int drag_channel;
	ArrayList<MixerControl*> controls;
	Mixer *mixer;
	MixerMenu *menu;
	BC_SubWindow *control_window;
	MixerVScroll *vscroll;
	MixerHScroll *hscroll;
};

class MixerVScroll : public BC_ScrollBar
{
public:
	MixerVScroll(Mixer *mixer,
		int x, 
		int y, 
		int h);
	int handle_event();
	Mixer *mixer;
};

class MixerHScroll : public BC_ScrollBar
{
public:
	MixerHScroll(Mixer *mixer,
		int x, 
		int y, 
		int w);
	int handle_event();
	Mixer *mixer;
};

class MixerControl
{
public:
	MixerControl(Mixer *mixer, 
		MixerNode *tree_node, 
		int x, 
		int y);
	~MixerControl();
	
	void create_objects();

	void reposition(int x_diff, int y_diff);
// store last_value and update everyone
	void update(MixerNode *node);
// Update values with the new node
	void update(MixerNode *node, int x, int y);
	int change_value(int new_value, 
		int omit_channel,
		int do_difference);
// adjust value of everyone by offset
	int reposition(int x);
// Whether or not the control represents the tree node.
	int equivalent(MixerNode *node);
	void read_value();

// Coords relative to x_position and y_position
	int x, y;
// Sizes derived from creation of control
	int w, h;
	int parameter_number;
	int total_controls;
	int type;
	BC_Title *title;
	Mixer *mixer;
	MixerNode *node;
	MixerPot *pots[MAX_CHANNELS];
	MixerToggle *toggles[MAX_CHANNELS];
	MixerPopupMenu *menu[MAX_CHANNELS];
};


class MixerToggle : public BC_CheckBox
{
public:
	MixerToggle(int x, 
		int y, 
		Mixer *mixer, 
		MixerControl *control,
		int channel);

	int handle_event();
	
	Mixer *mixer;
	MixerControl *control;
	int channel;
};


class MixerPopupMenu : public BC_PopupMenu
{
public:
	MixerPopupMenu(int x,
		int y, 
		int w,
		Mixer *mixer, 
		MixerControl *control,
		int channel);
	void create_objects();
	int handle_event();
	
	Mixer *mixer;
	MixerControl *control;
	int channel;
};

class MixerPot : public BC_IPot
{
public:
	MixerPot(int x, 
			int y, 
			Mixer *mixer, 
			MixerControl *control, 
			int channel);

	int handle_event();
	int last_value;
	int channel;
	Mixer *mixer;
	MixerControl *control;
};



class MixerLock : public BC_CheckBox
{
public:
	MixerLock(int x, int y, Mixer *mixer);

	int handle_event();

	Mixer *mixer;
};


#endif
