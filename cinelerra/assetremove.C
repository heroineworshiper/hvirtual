
/*
 * CINELERRA
 * Copyright (C) 2008-2017 Adam Williams <broadcast at earthling dot net>
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

#include "assetremove.h"
#include "indexable.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"


AssetRemoveWindow::AssetRemoveWindow(MWindow *mwindow, AssetRemoveThread *thread)
 : BC_Window(PROGRAM_NAME ": Remove assets", 
	mwindow->gui->get_abs_cursor_x(1),
	mwindow->gui->get_abs_cursor_y(1),
	DP(320), 
	DP(400), 
	-1, 
	-1, 
	0,
	0, 
	1)
{
	this->mwindow = mwindow;
    this->thread = thread;
	data = 0;
}

AssetRemoveWindow::~AssetRemoveWindow()
{
	delete data;
}

void AssetRemoveWindow::create_objects()
{
	int x = DP(10), y = DP(10);
	int margin = mwindow->theme->widget_border;

	data = new ArrayList<BC_ListBoxItem*>;


	for(int i = 0; i < thread->assets->size(); i++)
	{
		data->append(new BC_ListBoxItem(
			thread->assets->get(i)->path));
	}

	lock_window("AssetRemoveWindow::create_objects");
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Permanently remove from disk?")));
	y += title->get_h() + margin;
	BC_ListBox *list;
	add_subwindow(list = new BC_ListBox(
		x, 
		y, 
		get_w() - x * 2, 
		get_h() - y - BC_OKButton::calculate_h() - margin * 2,
		LISTBOX_TEXT,
		data));
	y += list->get_h() + margin;
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}




AssetRemoveThread::AssetRemoveThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
    assets = new ArrayList<Indexable*>;
	Thread::set_synchronous(0);
}

void AssetRemoveThread::start(ArrayList<Indexable*> *assets)
{
    if(assets->size() > 0)
    {
        this->assets->remove_all();
        for(int i = 0; i < assets->size(); i++)
        {
            this->assets->append(assets->get(i));
        }
        Thread::start();
    }
}


void AssetRemoveThread::run()
{
	AssetRemoveWindow *window = new AssetRemoveWindow(mwindow, this);
	window->create_objects();
	int result = window->run_window();
	delete window;
	
	if(!result)
	{
		mwindow->remove_assets_from_disk(assets);
	}
}



