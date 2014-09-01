#ifndef THEME_H
#define THEME_H


#include "bctheme.h"
#include "mixer.inc"


class Theme : public BC_Theme
{
public:
	Theme(Mixer *mixer);
	void initialize();
	void calculate_main_sizes();
	void calculate_config_sizes();
	void draw_main_bg();
	void draw_config_bg();


	Mixer *mixer;

	int is_bright;
	VFrame *main_bg;
	int main_subwindow_w, main_subwindow_h;
	int main_vscroll_x, main_vscroll_y, main_vscroll_h;
	int main_hscroll_x, main_hscroll_y, main_hscroll_w;

	int config_subwindow_w, config_subwindow_h;
	int config_vscroll_x, config_vscroll_y, config_vscroll_h;
	int config_hscroll_x, config_hscroll_y, config_hscroll_w;
};



#endif
