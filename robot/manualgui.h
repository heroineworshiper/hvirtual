#ifndef MANUALGUI_H
#define MANUALGUI_H

#include "manualgui.inc"
#include "guicast.h"
#include "mwindow.inc"


class ManualGUI : public BC_Window
{
public:
	ManualGUI(MWindow *mwindow);
	~ManualGUI();
	
	void create_objects();
	int keypress_event();
	int close_event();

	MWindow *mwindow;
	BC_OKButton *ok;
};


#endif
