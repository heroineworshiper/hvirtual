#ifndef SEARCH_H
#define SEARCH_H


#include "searchgui.inc"
#include "mwindow.inc"
#include "thread.h"


class Search : public Thread
{
public:
	Search(MWindow *mwindow);
	~Search();
	
	void start();
	void run();
	
	MWindow *mwindow;
	SearchGUI *gui;
};



#endif

