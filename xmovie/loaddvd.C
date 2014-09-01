#include "loaddvd.h"

LoadDVDThread::LoadDVDThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
}
LoadDVDThread::~LoadDVDThread()
{
}
void LoadDVDThread::run()
{
	int result = 0;
	{
		LoadDVDDialog dialog(mwindow);
		dialog.initialize();
		result = dialog.run_window();
	}
}


LoadDVDDialog::LoadDVDDialog(MWindow *mwindow)
 : BC_Window("XMovie: Load DVD", 220, 175, 220, 175)
{
	this->mwindow = mwindow;
}
LoadDVDDialog::~LoadDVDDialog()
{
}
int LoadDVDDialog::initialize()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, "DVD drive:"));
	y += 30;
	add_tool(new LoadDVDDrive(x, y, mwindow->dvd_device));
	y += 30;
	add_tool(new BC_Title(x, y, "Mount point for DVD drive:"));
	y += 30;
	add_tool(new LoadDVDMount(x, y, mwindow->dvd_mount));
	y += 40;
	add_tool(new LoadDVDOK(x, y));
	x += 110;
	add_tool(new LoadDVDCancel(x, y));
}

LoadDVDDrive::LoadDVDDrive(int x, int y, char *text)
 : BC_TextBox(x, y, 200, text)
{
}
LoadDVDDrive::~LoadDVDDrive()
{
}

LoadDVDMount::LoadDVDMount(int x, int y, char *text)
 : BC_TextBox(x, y, 200, text)
{
}
LoadDVDMount::~LoadDVDMount()
{
}



LoadDVDOK::LoadDVDOK(int x, int y)
 : BC_BigButton(x, y, "OK")
{
}
LoadDVDOK::~LoadDVDOK()
{
}
int LoadDVDOK::handle_event()
{
	set_done(0);
}

int LoadDVDOK::keypress_event()
{
	if(get_keypress() == 13) { set_done(0); trap_keypress(); return 1; }
	return 0;
}


LoadDVDCancel::LoadDVDCancel(int x, int y)
 : BC_BigButton(x, y, "Cancel")
{
}
LoadDVDCancel::~LoadDVDCancel()
{
	set_done(1);
}

int LoadDVDCancel::handle_event()
{
	set_done(1);
}

int LoadDVDCancel::keypress_event()
{
	if(get_keypress() == ESC) { set_done(1); trap_keypress(); return 1; }
	return 0;
}
