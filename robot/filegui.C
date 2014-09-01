#include "filegui.h"
#include "filethread.h"
#include "mwindow.h"
#include "robottheme.h"




#include <string.h>




FileText::FileText(MWindow *mwindow,
	FileGUI *gui,
	int x,
	int y,
	int w,
	char *text)
 : BC_TextBox(x, y, w, 1, text)
{
	this->gui = gui;
	this->mwindow = mwindow;
}
FileText::~FileText()
{
}

int FileText::handle_event()
{
	strcpy(gui->path, get_text());
	gui->handle_event();
	return 1;
}




FileBrowse::FileBrowse(MWindow *mwindow,
	FileGUI *gui,
	int x,
	int y)
 : BC_Button(x, y, mwindow->theme->browse)
{
	this->gui = gui;
	this->mwindow = mwindow;
}
FileBrowse::~FileBrowse()
{
}

int FileBrowse::handle_event()
{
	gui->thread->start_browse();
	return 1;
}




FileGUI::FileGUI(MWindow *mwindow,
	BC_WindowBase *parent_window,
	int x, 
	int y,
	int w,
	char *path,
	int want_directory,
	char *title,
	char *caption,
	int show_hidden)
{
	this->mwindow = mwindow;
	this->parent_window = parent_window;
	strcpy(this->path, path);
	strcpy(this->title, title);
	strcpy(this->caption, caption);
	this->x = x;
	this->y = y;
	this->w = w;
	this->want_directory = want_directory;
	this->show_hidden = show_hidden;
	text = 0;
	browse = 0;
	thread = 0;
}

FileGUI::~FileGUI()
{
	if(thread) delete thread;
}


void FileGUI::reposition(int x, int y, int w)
{
	text->reposition_window(x, y, w - 50);
	browse->reposition_window(x + w - 50, y);
}

void FileGUI::update_textbox()
{
	parent_window->lock_window();
	text->update(path);
	handle_event();
	parent_window->unlock_window();
}

int FileGUI::handle_event()
{
	return 1;
}


void FileGUI::create_objects()
{
	int x1 = x, y1 = y;

	thread = new FileThread(mwindow, this);
	parent_window->add_subwindow(text = new FileText(mwindow, 
		this,
		x1,
		y1,
		w - 50,
		path));
	parent_window->add_subwindow(browse = new FileBrowse(mwindow,
		this,
		x1 + w - 50,
		y1));
}
