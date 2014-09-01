#ifndef DELETE_H
#define DELETE_H


#include "deletegui.inc"
#include "guicast.h"
#include "mwindow.inc"


class Delete : public Thread
{
public:
	Delete(MWindow *mwindow);
	~Delete();

	void start();
	void run();

	MWindow *mwindow;
	DeleteGUI *gui;
};



#endif
