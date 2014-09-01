#ifndef LOADDVD_H
#define LOADDVD_H

#include "bcbase.h"
#include "mwindow.h"

class LoadDVDThread : public Thread
{
public:
	LoadDVDThread(MWindow *mwindow);
	~LoadDVDThread();
	void run();
	MWindow *mwindow;
};

class LoadDVDDrive;
class LoadDVDMount;

class LoadDVDDialog : public BC_Window
{
public:
	LoadDVDDialog(MWindow *mwindow);
	~LoadDVDDialog();

	int initialize();
	MWindow *mwindow;
	LoadDVDDrive *drive;
	LoadDVDMount *mnt_point;
};

class LoadDVDDrive : public BC_TextBox
{
public:
	LoadDVDDrive(int x, int y, char *text);
	~LoadDVDDrive();
};

class LoadDVDMount : public BC_TextBox
{
public:
	LoadDVDMount(int x, int y, char *text);
	~LoadDVDMount();
};

class LoadDVDOK : public BC_BigButton
{
public:
	LoadDVDOK(int x, int y);
	~LoadDVDOK();
	int handle_event();
	int keypress_event();
};

class LoadDVDCancel : public BC_BigButton
{
public:
	LoadDVDCancel(int x, int y);
	~LoadDVDCancel();
	int handle_event();
	int keypress_event();
};

#endif
