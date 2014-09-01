#ifndef SPLASHGUI_H
#define SPLASHGUI_H

#include "guicast.h"
#include "mwindow.inc"
#include "vframe.inc"

class SplashGUI : public BC_Window
{
public:
	SplashGUI(MWindow *mwindow, VFrame *bg, int x, int y);
	~SplashGUI();

	void create_objects();
	void update_progress_current(int current);
	void update_progress_length(int total);

	BC_ProgressBar *progress;
	MWindow *mwindow;
	VFrame *bg;
	int last_position;
	int total;
};


#endif
