#include "filegui.h"
#include "mwindow.h"
#include "options.h"
#include "optionsgui.h"
#include "robotprefs.h"
#include "robottheme.h"

#include <string.h>

OptionsPath::OptionsPath(MWindow *mwindow, 
	Options *thread, 
	int x, 
	int y, 
	int w)
 : FileGUI(mwindow,
		thread->gui,
		x, 
		y,
		w,
		thread->prefs->db_path,
		0,
		TITLE ": Database file",
		"Select the database file",
		1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

int OptionsPath::handle_event()
{
	strcpy(thread->prefs->db_path, path);
	return 1;
}









OptionsHost::OptionsHost(MWindow *mwindow, 
	Options *thread, 
	int x, 
	int y, 
	int w)
 : BC_TextBox(x, 
 	y,
	w, 
	1,
	thread->prefs->hostname)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

int OptionsHost::handle_event()
{
	strcpy(thread->prefs->hostname, get_text());
	return 1;
}







OptionsPort::OptionsPort(MWindow *mwindow, 
	Options *thread, 
	int x, 
	int y, 
	int w)
 : BC_TumbleTextBox(thread->gui, 
	(int)thread->prefs->port,
	(int)1,
	(int)65535,
	x, 
	y, 
	w)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

int OptionsPort::handle_event()
{
	thread->prefs->port = atol(get_text());
	return 1;
}










OptionsGUI::OptionsGUI(MWindow *mwindow, Options *thread)
 : BC_Window(TITLE ": Options", 
				mwindow->theme->mwindow_x + 
					mwindow->theme->mwindow_w / 2 - 
					mwindow->theme->options_w / 2,
				mwindow->theme->mwindow_y +
					mwindow->theme->mwindow_h / 2 - 
					mwindow->theme->options_h / 2,
				mwindow->theme->options_w, 
				mwindow->theme->options_h, 
				10,
				10, 
				0,
				0, 
				1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

OptionsGUI::~OptionsGUI()
{
	delete path;
}


void OptionsGUI::create_objects()
{
	add_subwindow(new BC_Title(mwindow->theme->options_path_x,
		mwindow->theme->options_path_y - 20,
		"Database path:"));
	path = new OptionsPath(mwindow, 
		thread, 
		mwindow->theme->options_path_x, 
		mwindow->theme->options_path_y, 
		mwindow->theme->options_path_w);
	path->create_objects();

	add_subwindow(new BC_Title(mwindow->theme->options_host_x,
		mwindow->theme->options_host_y - 20,
		"Robot hostname:"));
	add_subwindow(new OptionsHost(mwindow, 
		thread, 
		mwindow->theme->options_host_x, 
		mwindow->theme->options_host_y, 
		mwindow->theme->options_host_w));

	add_subwindow(new BC_Title(mwindow->theme->options_port_x,
		mwindow->theme->options_port_y - 20,
		"Robot port:"));
	OptionsPort *port = new OptionsPort(mwindow, 
		thread, 
		mwindow->theme->options_port_x, 
		mwindow->theme->options_port_y, 
		mwindow->theme->options_port_w);
	port->create_objects();

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
}

int OptionsGUI::keypress_event()
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

int OptionsGUI::close_event()
{
	set_done(1);
	return 1;
}


