#ifndef THEME_H
#define THEME_H

#include "bctheme.h"
#include "mwindowgui.inc"
#include "vframe.inc"

class Theme : public BC_Theme
{
public:
	Theme();

	virtual int draw_mwindow_bg(MWindow *mwindow, MWindowGUI *gui) { return 0; };
	virtual int update_positions(MWindow *mwindow, MWindowGUI *gui) { return 0; };
	virtual void draw_canvas_bg(BC_WindowBase *canvas) {};
	virtual void update_positions_from_canvas(MWindow *mwindow, MWindowGUI *gui) {};
	void build_button(VFrame** &data,
		unsigned char *png_overlay,
		VFrame *up_vframe,
		VFrame *hi_vframe,
		VFrame *dn_vframe);
	void overlay(VFrame *dst, VFrame *src);

	VFrame *icon;
	VFrame **play;
	VFrame **frame_fwd;
	VFrame **frame_bck;
	VFrame **pause;
// Canvas size when not fullscreen
	int canvas_x, canvas_y, canvas_w, canvas_h;     
	int time_x, time_y, time_w;
	int play_x, play_y;
	int frameback_x, frameback_y;
	int framefwd_x, framefwd_y;
	int scroll_x, scroll_y, scroll_w;
	int gui_x, gui_y;
};

class GoldTheme : public Theme
{
public:
	GoldTheme();

	void draw_canvas_bg(BC_WindowBase *canvas);
	int draw_mwindow_bg(MWindow *mwindow, MWindowGUI *gui);
	int update_positions(MWindow *mwindow, MWindowGUI *gui);
	void update_positions_from_canvas(MWindow *mwindow, MWindowGUI *gui);
	VFrame *bar_left, *bar_mid, *bar_right;
	VFrame *heroine_bg;
};

#endif
