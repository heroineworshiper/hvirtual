#include "manualgui.h"
#include "mwindow.h"
#include "robottheme.h"









ManualGUI::ManualGUI(MWindow *mwindow)
 : BC_Window(TITLE ": Manual Override", 
	mwindow->theme->mwindow_x + 
		mwindow->theme->mwindow_w / 2 - 
		mwindow->theme->manual_w / 2,
	mwindow->theme->mwindow_y +
		mwindow->theme->mwindow_h / 2 - 
		mwindow->theme->manual_h / 2,
	mwindow->theme->manual_w, 
	mwindow->theme->manual_h, 
	10,
	10, 
	1,
	0, 
	1)
{
	this->mwindow = mwindow;
}

ManualGUI::~ManualGUI()
{
}

void ManualGUI::create_objects()
{
	add_subwindow(ok = new BC_OKButton(this));
	show_window();
}

int ManualGUI::keypress_event()
{
	switch(get_keypress())
	{
		case 'w':
			set_done(1);
			return 1;
			break;
	}
	return 0;
}

int ManualGUI::close_event()
{
	set_done(1);
	return 1;
}

