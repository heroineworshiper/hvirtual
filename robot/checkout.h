#ifndef CHECKOUT_H
#define CHECKOUT_H


#include "checkoutgui.inc"
#include "mwindow.inc"
#include "thread.h"


class CheckOut : public Thread
{
public:
	CheckOut(MWindow *mwindow);
	~CheckOut();

	void start(int delete_next = 0);
	void run();

	CheckOutGUI *gui;
	MWindow *mwindow;
	int delete_next;
};


#endif
