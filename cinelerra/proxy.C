
/*
 * CINELERRA
 * Copyright (C) 2015 Adam Williams <broadcast at earthling dot net>
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

#include "assets.h"
#include "bcsignals.h"
#include "confirmsave.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "formattools.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "proxy.h"
#include "quicktime.h"
#include "theme.h"

#include <string.h>

#define WIDTH 320
#define HEIGHT 320

static const char* size_text[] = 
{
	"Original size",
	"1/2",
	"1/4",
	"1/8",
	"1/16",
	"1/32"
};

static int sizes = sizeof(size_text) / sizeof(char*);

static int scale_to_text(char *string, int scale)
{
	strcpy(string, size_text[0]);
	for(int i = 0; i < sizes; i++)
	{
		if(scale == (1 << i))
		{
			strcpy(string, size_text[i]);
			break;
		}
	}
}


ProxyMenuItem::ProxyMenuItem(MWindow *mwindow)
 : BC_MenuItem(_("Proxy settings..."))
{
	this->mwindow = mwindow;
}

void ProxyMenuItem::create_objects()
{
	thread = new ProxyThread(mwindow);
}

int ProxyMenuItem::handle_event()
{
	mwindow->gui->unlock_window();
	thread->start();
	mwindow->gui->lock_window("ProxyMenuItem::handle_event");

	return 1;
}




ProxyThread::ProxyThread(MWindow *mwindow)
{
	this->mwindow = mwindow;
	gui = 0;
	asset = new Asset;
}

BC_Window* ProxyThread::new_gui()
{
	asset->format = FILE_MOV;
	strcpy(asset->vcodec, QUICKTIME_JPEG);
	asset->jpeg_quality = 75;
	asset->load_defaults(mwindow->defaults, 
		"PROXY_", 
		1,
		1,
		0,
		0,
		0);



	mwindow->gui->lock_window("ProxyThread::new_gui");
	int x = mwindow->gui->get_abs_cursor_x(0) - WIDTH / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) - HEIGHT / 2;

	gui = new ProxyWindow(mwindow, this, x, y);
	gui->create_objects();
	mwindow->gui->unlock_window();
	return gui;
}

void ProxyThread::handle_close_event(int result)
{
	asset->save_defaults(mwindow->defaults, 
		"PROXY_",
		1,
		1,
		0,
		0,
		0);


	if(!result)
	{
// revert project if new scale is 1
		if(new_scale == 1)
		{
			from_proxy();
		}
		else
		{
			to_proxy();
		}
	}
}

void ProxyThread::from_proxy()
{
	mwindow->undo->update_undo_before(_("proxy"), this);
// resize project
// set EDL proxy size
	mwindow->edl->session->proxy_scale = 1;
// revert assets
	mwindow->undo->update_undo_after(_("proxy"), LOAD_ALL);
}

void ProxyThread::to_proxy()
{
// test for new files
	ArrayList<string*> confirm_paths;
	ArrayList<Asset*> proxy_assets;
	for(Asset *asset = mwindow->edl->assets->first;
		asset;
		asset = asset->next)
	{
		if(asset->video_data)
		{
			string new_path;
			create_path(&new_path, asset, new_scale);

// add to proxy assets
			int got_it = 0;
			for(int i = 0; i < proxy_assets.size(); i++)
			{
				if(!strcmp(proxy_assets.get(i)->path, new_path.c_str()))
				{
					got_it = 1;
					break;
				}
			}

			if(!got_it)
			{
				Asset *proxy_asset = new Asset;
				proxy_asset->copy_from(asset, 0);
				proxy_asset->audio_data = 0;
				proxy_assets.append(proxy_asset);
			}


// test if proxy file exists.
			int exists = 0;
			FILE *fd = fopen(new_path.c_str(), "r");
			if(fd)
			{
				got_it = 1;
				exists = 1;
				fclose(fd);

				FileSystem fs;
// test if proxy file is newer than original.
				if(fs.get_date(new_path.c_str()) < fs.get_date(asset->path))
				{
					got_it = 0;
				}
			}
			else
			{
// proxy doesn't exist
				got_it = 0;
			}

			if(!got_it)
			{
// prompt user to overwrite
				if(exists)
				{
					confirm_paths.append(new string(new_path));
				}
			}
//printf("ProxyThread::handle_close_event %d %s\n", __LINE__, new_path.c_str());
		}
	}

// test for existing files
	int result = 0;
	if(confirm_paths.size())
	{
		result = ConfirmSave::test_files(mwindow, &confirm_paths);
	}

	if(!result)
	{
// create proxy assets


		mwindow->undo->update_undo_before(_("proxy"), this);
// resize project
// set EDL proxy size
		mwindow->edl->session->proxy_scale = new_scale;
// replace assets
		mwindow->undo->update_undo_after(_("proxy"), LOAD_ALL);
	}

	for(int i = 0; i < proxy_assets.size(); i++)
	{
		proxy_assets.get(i)->Garbage::remove_user();
	}
}
		

void ProxyThread::create_path(string *new_path, Asset *asset, int scale)
{
	new_path->assign(asset->path);
	char code[BCTEXTLEN];
	sprintf(code, ".proxy%d", scale);
	int code_len = strlen(code);
	int got_code = 0;

// path is already a proxy
	if(strstr(new_path->c_str(), code)) return;

// insert proxy code
	char *ptr = strrchr((char*)new_path->c_str(), '.');
	if(ptr)
	{
//printf("ProxyThread::create_path %d %d\n", __LINE__, ptr - new_path->c_str());
		new_path->insert(ptr - new_path->c_str(), code);
	}
	else
	{
		new_path->append(code);
	}
}





ProxyWindow::ProxyWindow(MWindow *mwindow, ProxyThread *thread, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Proxy settings"), 
 		x,
		y,
		WIDTH, 
		HEIGHT,
		-1,
		-1,
		0,
		0,
		1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	format_tools = 0;
}

ProxyWindow::~ProxyWindow()
{
	lock_window("ProxyWindow::~ProxyWindow");
	delete format_tools;
	unlock_window();
}


void ProxyWindow::create_objects()
{
	lock_window("ProxyWindow::create_objects");
	
	int margin = mwindow->theme->widget_border;
	int x = margin;
	int y = margin;
	thread->orig_scale = 
		thread->new_scale = 
		mwindow->edl->session->proxy_scale;
	
	BC_Title *text;
	add_subwindow(text = new BC_Title(
		x, 
		y, 
		"What size should the project\n"
		"be scaled to for editing?"));
	y += text->get_h() * 2 + margin;
	
	
	add_subwindow(text = new BC_Title(x, y, "Scale factor:"));
	x += text->get_w() + margin;

	char string[BCTEXTLEN];
	scale_to_text(string, thread->new_scale);

	int popupmenu_w = BC_PopupMenu::calculate_w(get_text_width(MEDIUMFONT, size_text[0]));
	add_subwindow(scale_factor = new ProxyMenu(x, y, popupmenu_w, string, mwindow, this));
	for(int i = 0; i < sizes; i++)
	{
		scale_factor->add_item(new BC_MenuItem(size_text[i]));
	}
	x += scale_factor->get_w() + margin;
	
	ProxyTumbler *tumbler;
	add_subwindow(tumbler = new ProxyTumbler(mwindow, 
		this, 
		x, 
		y));

	x = margin;
	y += tumbler->get_h() + margin;
	ProxyReset *reset;
	add_subwindow(reset = new ProxyReset(x, y, mwindow, this));
	
	y += reset->get_h() * 2 + margin;
	x = margin;
	add_subwindow(text = new BC_Title(x, y, "New project dimensions: "));
	x += text->get_w() + margin;
	sprintf(string, 
		"%dx%d", 
		mwindow->edl->session->output_w / thread->new_scale,
		mwindow->edl->session->output_h / thread->new_scale);
	add_subwindow(new_dimensions = new BC_Title(x, y, string));

	x = margin;
	y += new_dimensions->get_h() * 2 + margin;


	format_tools = new FormatTools(mwindow,
					this, 
					thread->asset);
	format_tools->create_objects(x, 
		y, 
		0, 
		1, 
		0, 
		0, 
		0,
		1,
		0,
		1, // skip the path
		0,
		0);

	
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window(1);
	unlock_window();
}

void ProxyWindow::update()
{
// preview the new size
	char string[BCTEXTLEN];
	int orig_w = mwindow->edl->session->output_w * thread->orig_scale;
	int orig_h = mwindow->edl->session->output_h * thread->orig_scale;
	
	sprintf(string, 
		"%dx%d", 
		orig_w / thread->new_scale,
		orig_h / thread->new_scale);
	new_dimensions->update(string);
	scale_to_text(string, thread->new_scale);
	scale_factor->set_text(string);
}


ProxyReset::ProxyReset(int x, int y, MWindow *mwindow, ProxyWindow *pwindow)
 : BC_GenericButton(x, y, "Reset")
{
	this->mwindow = mwindow;
	this->pwindow = pwindow;
}

int ProxyReset::handle_event()
{
	pwindow->thread->new_scale = pwindow->thread->orig_scale;
	pwindow->update();
	return 1;
}






ProxyMenu::ProxyMenu(int x, int y, int w, char *text, MWindow *mwindow, ProxyWindow *pwindow)
 : BC_PopupMenu(x, y, w, text, 1)
{
	this->mwindow = mwindow;
	this->pwindow = pwindow;
}

int ProxyMenu::handle_event()
{
	for(int i = 0; i < sizes; i++)
	{
		if(!strcmp(get_text(), size_text[i]))
		{
			pwindow->thread->new_scale = (1 << i);
			pwindow->update();
			break;
		}
	}
	return 1;
}




ProxyTumbler::ProxyTumbler(MWindow *mwindow, ProxyWindow *pwindow, int x, int y)
 : BC_Tumbler(x, 
 	y,
	0)
{
	this->mwindow = mwindow;
	this->pwindow = pwindow;
}

int ProxyTumbler::handle_up_event()
{
	if(pwindow->thread->new_scale > 1) 
	{
		pwindow->thread->new_scale /= 2;
		pwindow->update();
	}
}

int ProxyTumbler::handle_down_event()
{
	if(pwindow->thread->new_scale < (1 << (sizes - 1)))
	{
		pwindow->thread->new_scale *= 2;
		pwindow->update();
	}
}







