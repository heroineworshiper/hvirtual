/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcdialog.h"
#include "bcsignals.h"
#include "condition.h"
#include "mutex.h"





BC_DialogThread::BC_DialogThread()
 : Thread(1, 0, 0)
{
	gui = 0;
    keep_gui = 0;
    async_gui = 0;
    restart = 0;
	startup_lock = new Condition(1, "BC_DialogThread::startup_lock");
	window_lock = new Mutex("BC_DialogThread::window_lock");
}

BC_DialogThread::~BC_DialogThread()
{
	startup_lock->lock("BC_DialogThread::~BC_DialogThread");
	if(gui)
	{
		gui->lock_window();
		gui->set_done(1);
		gui->unlock_window();
	}
	startup_lock->unlock();
	Thread::join();

	delete startup_lock;
	delete window_lock;
}

void BC_DialogThread::lock_window(const char *location)
{
	window_lock->lock(location);
}

void BC_DialogThread::unlock_window()
{
	window_lock->unlock();
}

void BC_DialogThread::set_keep_gui(int value)
{
    keep_gui = value;
}

void BC_DialogThread::set_async_gui()
{
    async_gui = 1;
}

void BC_DialogThread::set_restart()
{
    restart = 1;
}

int BC_DialogThread::is_running()
{
	return Thread::running();
}

void BC_DialogThread::start()
{
	if(Thread::running())
	{
		window_lock->lock("BC_DialogThread::start");
		if(gui)
		{
			gui->lock_window("BC_DialogThread::start");
			gui->raise_window(1);
			gui->unlock_window();
		}
		window_lock->unlock();
		return;
	}

// Don't allow anyone else to create the window
	startup_lock->lock("BC_DialogThread::start");
	Thread::start();

// Wait for startup
	startup_lock->lock("BC_DialogThread::start");
	startup_lock->unlock();
}

void BC_DialogThread::run()
{
// unlock startup_lock once after the window is created
    int pass = 0;
    do
    {
        restart = 0;

        if(pass == 0 && async_gui) startup_lock->unlock();
        if(!gui)
        {
    	    gui = new_gui();
        }
        else
        {
            gui->show_window(1);
        }

	    if(pass == 0 && !async_gui) startup_lock->unlock();
	    int result = gui->run_window();

	    handle_done_event(result);

	    window_lock->lock("BC_DialogThread::run");

        if(!keep_gui)
        {
    	    delete gui;
	        gui = 0;
        }
        else
        {
            gui->hide_window(1);
        }
	    window_lock->unlock();

	    handle_close_event(result);
        pass++;
    } while(restart);
}

void BC_DialogThread::lock_gui(const char *location)
{
	window_lock->lock(location);
}

void BC_DialogThread::unlock_gui()
{
	window_lock->unlock();
}


BC_Window* BC_DialogThread::new_gui()
{
	printf("BC_DialogThread::new_gui called\n");
	return 0;
}

BC_Window* BC_DialogThread::get_gui()
{
	return gui;
}

void BC_DialogThread::handle_done_event(int result)
{
}

void BC_DialogThread::handle_close_event(int result)
{
}

void BC_DialogThread::close_window()
{
	lock_window("BC_DialogThread::close_window");
	if(gui)
	{
		gui->lock_window("BC_DialogThread::close_window");
		gui->set_done(1);
		gui->unlock_window();
	}
	unlock_window();
}




