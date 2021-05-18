
/*
 * CINELERRA
 * Copyright (C) 2008-2021 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PAR
 * You should have recTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcclipboard.h"
#include "bcdisplay.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "bcwindowbase.inc"
#include <string.h>
#include <unistd.h>


// clipboard notes came from 
// https://www.uninformativ.de/blog/postings/2017-04-02/0/POSTING-en.html

BC_Clipboard::BC_Clipboard(const char *display_name) : Thread()
{
	Thread::set_synchronous(1);

	if(display_name) 
		strcpy(this->display_name, display_name);
	else
		this->display_name[0] = 0;

#ifdef SINGLE_THREAD
	in_display = out_display = BC_Display::get_display(display_name);
#else
	in_display = BC_WindowBase::init_display(display_name);
	out_display = BC_WindowBase::init_display(display_name);
#endif

	completion_atom = XInternAtom(out_display, "BC_CLOSE_EVENT", False);
	primary = XA_PRIMARY;
	secondary = XInternAtom(out_display, "CLIPBOARD", False);
    utf8_target = XInternAtom(out_display, "UTF8_STRING", False);
    targets = XInternAtom(out_display, "TARGETS", False);
    string_target = XInternAtom(in_display, "STRING", False);

	in_win = XCreateSimpleWindow(in_display, 
				DefaultRootWindow(in_display), 
				0, 
				0, 
				1, 
				1, 
				0,
				0,
				0);
	out_win = XCreateSimpleWindow(out_display, 
				DefaultRootWindow(out_display), 
				0, 
				0, 
				1, 
				1, 
				0,
				0,
				0);
	data[0] = 0;
	data[1] = 0;
}

BC_Clipboard::~BC_Clipboard()
{
	if(data[0]) delete [] data[0];
	if(data[1]) delete [] data[1];

	XDestroyWindow(in_display, in_win);
	XCloseDisplay(in_display);
	XDestroyWindow(out_display, out_win);
	XCloseDisplay(out_display);
}

int BC_Clipboard::start_clipboard()
{
#ifndef SINGLE_THREAD
	Thread::start();
#endif
	return 0;
}

int BC_Clipboard::stop_clipboard()
{
// Must use a different display handle to send events.
	Display *display = BC_WindowBase::init_display(display_name);
	XEvent event;
	XClientMessageEvent *ptr = (XClientMessageEvent*)&event;

	event.type = ClientMessage;
	ptr->message_type = completion_atom;
	ptr->format = 32;
//printf("BC_Clipboard::stop_clipboard %d\n", __LINE__);
	XSendEvent(display, out_win, 0, 0, &event);
	XFlush(display);
	XCloseDisplay(display);

	Thread::join();
	return 0;
}

void BC_Clipboard::run()
{
#ifndef SINGLE_THREAD
	XEvent event;
	XClientMessageEvent *ptr;
	int done = 0;

	while(!done)
	{
//printf("BC_Clipboard::run 1\n");					
		XNextEvent(out_display, &event);
//printf("BC_Clipboard::run 2 %d\n", event.type);					

#ifdef SINGLE_THREAD
		BC_Display::lock_display("BC_Clipboard::run");
#else
		XLockDisplay(out_display);
#endif
		switch(event.type)
		{
// Termination signal
			case ClientMessage:
				ptr = (XClientMessageEvent*)&event;
				if(ptr->message_type == completion_atom)
				{
					done = 1;
				}
//printf("ClientMessage %x %x %d\n", ptr->message_type, ptr->data.l[0], primary_atom);
				break;


			case SelectionRequest:
			{
				XEvent reply;
                bzero(&reply, sizeof(reply));

				XSelectionRequestEvent *request = (XSelectionRequestEvent*)&event;
				char *data_ptr = (request->selection == primary ? 
                    data[0] : 
                    data[1]);

// printf("BC_Clipboard::run %d selection=%ld property=%ld target=%s primary=%ld secondary=%ld\n", 
// __LINE__, 
// request->selection, 
// request->property, 
// XGetAtomName(out_display, request->target), 
// primary,
// secondary);
// deny requests for wrong target or property
                if(request->target == targets)
                {
// printf("BC_Clipboard::run %d denying request for %s\n", 
// __LINE__, 
// XGetAtomName(out_display, request->target));
                    reply.xselection.type      = SelectionNotify;
                    reply.xselection.requestor = request->requestor;
                    reply.xselection.selection = request->selection;
                    reply.xselection.target = request->target;
                    reply.xselection.property = None;
                    reply.xselection.time = request->time;
                }
                else
                {
        		    XChangeProperty(out_display,
        			    request->requestor,
        			    request->property,
        			    utf8_target,
        			    8,
        			    PropModeReplace,
        			    (unsigned char*)data_ptr,
        			    strlen(data_ptr));

        		    reply.xselection.property  = request->property;
        		    reply.xselection.type      = SelectionNotify;
        		    reply.xselection.display   = request->display;
        		    reply.xselection.requestor = request->requestor;
        		    reply.xselection.selection = request->selection;
        		    reply.xselection.target    = request->target;
        		    reply.xselection.time      = request->time;
                }

				XSendEvent(out_display, request->requestor, True, NoEventMask, &reply);
			    XFlush(out_display);
// printf("BC_Clipboard::run %d requestor=%ld property=%ld text=%s len=%ld\n", 
// __LINE__, 
// request->requestor, 
// request->property,
// data_ptr, 
// strlen(data_ptr));
				break;
			}
			
// another window has copied something.  Clear our own buffer
			case SelectionClear:
			{
				XSelectionClearEvent *request = (XSelectionClearEvent*)&event;
// printf("BC_Clipboard::run %d selection=%p primary=%p secondary=%p\n", 
// __LINE__, 
// request->selection,
// primary,
// secondary);
				if(data[0] && request->selection == primary)
				{
					data[0][0] = 0;
				}
				
				
				if(data[1] && request->selection == secondary)
				{
					data[1][0] = 0;
				}
				break;
			}
		}

#ifdef SINGLE_THREAD
		BC_Display::unlock_display();
#else
		XUnlockDisplay(out_display);
#endif
	}
#endif

}

int BC_Clipboard::to_clipboard(const char *data, int len, int clipboard_num)
{
	if(clipboard_num == BC_PRIMARY_SELECTION)
	{
		XStoreBuffer(out_display, data, len, clipboard_num);
		return 0;
	}

#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_Clipboard::to_clipboard");
#else
	XLockDisplay(out_display);
#endif

// Store in local buffer
	if(this->data[clipboard_num] && length[clipboard_num] != len + 1)
	{
		delete [] this->data[clipboard_num];
		this->data[clipboard_num] = 0;
	}

	if(!this->data[clipboard_num])
	{
		length[clipboard_num] = len;
		this->data[clipboard_num] = new char[len + 1];
		memcpy(this->data[clipboard_num], data, len);
		this->data[clipboard_num][len] = 0;
	}
// printf("BC_Clipboard::to_clipboard %d this=%p clipboard_num=%d len=%ld data=%p\n", 
// __LINE__, 
// this,
// clipboard_num,
// len,
// this->data[clipboard_num]);

	if(clipboard_num == PRIMARY_SELECTION)
	{
		XSetSelectionOwner(out_display, 
			primary, 
			out_win, 
			CurrentTime);
	}
	else
	if(clipboard_num == SECONDARY_SELECTION)
	{
		XSetSelectionOwner(out_display, 
			secondary, 
			out_win, 
			CurrentTime);
	}


	XFlush(out_display);


#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#else
	XUnlockDisplay(out_display);
#endif
	return 0;
}


int BC_Clipboard::from_clipboard(char *data, 
    int maxlen, 
    int *len_return, 
    int clipboard_num)
{

	if(clipboard_num == BC_PRIMARY_SELECTION)
	{
		char *data2;
		int len, i;
		data2 = XFetchBuffer(in_display, &len, clipboard_num);
        
        if(data != 0)
        {
		    for(i = 0; i < len && i < maxlen - 1; i++)
		    {
        	    data[i] = data2[i];
            }

		    data[i] = 0;
        }
        
        if(len_return != 0)
        {
            *len_return = len + 1;
        }

		XFree(data2);
		return 0;
	}



#ifdef SINGLE_THREAD
	BC_Display::lock_display("BC_Clipboard::from_clipboard");
#else
	XLockDisplay(in_display);
#endif

	XEvent event;
    Atom type_return, selection, property;
    int format;
    unsigned long nitems, size, new_size, total;
	char *temp_data = 0;

    property = XInternAtom(in_display, "FOOBAR", False);
    selection = (clipboard_num == PRIMARY_SELECTION) ? primary : secondary; 

	XConvertSelection(in_display, 
		selection, 
		string_target, 
		property,
       	in_win, 
		CurrentTime);

    if(data != 0)
    {
    	data[0] = 0;
    }

	do
	{
		XNextEvent(in_display, &event);
	}while(event.type != SelectionNotify && event.type != None);

	if(event.type != None)
	{
// Get the size
   	    XGetWindowProperty(in_display,
        	in_win,
        	property,
        	0,
        	0,
        	False,
        	AnyPropertyType,
        	&type_return,
        	&format,
        	&nitems,
        	&size,
        	(unsigned char**)&temp_data);
	    if(temp_data) XFree(temp_data);
	    temp_data = 0;

// Get the text
   	    XGetWindowProperty(in_display,
        	in_win,
        	property,
        	0,
        	size,
        	False,
        	AnyPropertyType,
        	&type_return,
        	&format,
        	&nitems,
        	&new_size,
        	(unsigned char**)&temp_data);



// printf("BC_Clipboard::from_clipboard %d data=%p size=%ld temp_data=%s\n", 
// __LINE__, 
// data,
// size,
// temp_data);


        if(data != 0 && temp_data != 0)
        {
            strncpy(data, temp_data, maxlen);
        }


        if(len_return != 0)
        {
            *len_return = size + 1;
        }


	    if(temp_data) XFree(temp_data);
	    temp_data = 0;
	}


#ifdef SINGLE_THREAD
	BC_Display::unlock_display();
#else
	XUnlockDisplay(in_display);
#endif


    return 0;
}



int BC_Clipboard::from_clipboard(char *data, int maxlen, int clipboard_num)
{
//printf("BC_Clipboard::from_clipboard %d clipboard_num=%d\n", __LINE__, clipboard_num);
    return from_clipboard(data, maxlen, 0, clipboard_num);
}

int BC_Clipboard::clipboard_len(int clipboard_num)
{
    int len;
    from_clipboard(0, 0, &len, clipboard_num);
	return len;
}
