#include "keys.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "robotdb.h"
#include "robotprefs.h"
#include "robottheme.h"
#include "searchgui.h"


#include <string.h>



SearchText::SearchText(MWindow *mwindow, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, mwindow->prefs->search_string)
{
	this->mwindow = mwindow;
}

int SearchText::handle_event()
{
	strcpy(mwindow->prefs->search_string, get_text());
	return 1;
}






SearchField::SearchField(MWindow *mwindow, 
	int x, 
	int y, 
	int *output,
	char *text)
 : BC_CheckBox(x,
 	y,
	*output,
	text)
{
	this->mwindow = mwindow;
	this->value = value;
	this->output = output;
}

int SearchField::handle_event()
{
	*output = get_value();
	return 1;
}




SearchCase::SearchCase(MWindow *mwindow, int x, int y)
 : BC_CheckBox(x, y, mwindow->prefs->search_case, "Case sensitive")
{
	this->mwindow = mwindow;
}

int SearchCase::handle_event()
{
	mwindow->prefs->search_case = get_value();
	return 1;
}





SearchBackward::SearchBackward(MWindow *mwindow, int x, int y)
 : BC_CheckBox(x, y, mwindow->prefs->search_backward, "Search backward")
{
	this->mwindow = mwindow;
}

int SearchBackward::handle_event()
{
	mwindow->prefs->search_backward = get_value();
	return 1;
}







SearchOK::SearchOK(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, y, "Find next")
{
	this->mwindow = mwindow;
}

int SearchOK::handle_event()
{
	mwindow->do_search(1);
	return 1;
}








// SearchAgain::SearchAgain(MWindow *mwindow, int x, int y)
//  : BC_GenericButton(x, y, "Search Again")
// {
// 	this->mwindow = mwindow;
// }
// int SearchAgain::handle_event()
// {
// 	mwindow->do_search(1, 1);
// 	return 1;
// }







SearchCancel::SearchCancel(MWindow *mwindow, SearchGUI *gui, int x, int y)
 : BC_GenericButton(x, y, "Close")
{
	this->gui = gui;
	this->mwindow = mwindow;
}
int SearchCancel::handle_event()
{
	gui->set_done(1);
	return 1;
}
int SearchCancel::keypress_event()
{
	switch(get_keypress())
	{
		case ESC:
			gui->set_done(1);
			return 1;
			break;
	}
	return 0;
}








SearchGUI::SearchGUI(MWindow *mwindow, Search *thread)
 : BC_Window(TITLE ": Search Database", 
				mwindow->gui->get_abs_cursor_x(1) - 
					mwindow->theme->search_w / 2,
				mwindow->gui->get_abs_cursor_y(1) - 
					mwindow->theme->search_h / 2,
				mwindow->theme->search_w,
				mwindow->theme->search_h,
				mwindow->theme->search_w,
				mwindow->theme->search_h,
				0,
				0, 
				1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void SearchGUI::create_objects()
{
	int x;
	int y;

	add_subwindow(new BC_Title(mwindow->theme->search_string_x,
		mwindow->theme->search_string_y - 20,
		"Search string:"));
	add_subwindow(new SearchText(mwindow, 
		mwindow->theme->search_string_x, 
		mwindow->theme->search_string_y, 
		mwindow->theme->search_string_w));

	x = mwindow->theme->search_fields_x;
	y = mwindow->theme->search_fields_y;

	add_subwindow(new BC_Title(x, y, "Fields:"));
	y += 20;
	add_subwindow(new SearchField(mwindow, 
		x, 
		y, 
		&mwindow->prefs->search_fields[RobotDB::PATH],
		RobotDB::column_titles[RobotDB::PATH]));
	y += 30;
	add_subwindow(new SearchField(mwindow, 
		x, 
		y, 
		&mwindow->prefs->search_fields[RobotDB::SIZE],
		RobotDB::column_titles[RobotDB::SIZE]));
	y += 30;
	add_subwindow(new SearchField(mwindow, 
		x, 
		y, 
		&mwindow->prefs->search_fields[RobotDB::DATE],
		RobotDB::column_titles[RobotDB::DATE]));
	y += 30;
	add_subwindow(new SearchField(mwindow, 
		x, 
		y, 
		&mwindow->prefs->search_fields[RobotDB::DESC],
		RobotDB::column_titles[RobotDB::DESC]));

	x = mwindow->theme->search_case_x;
	y = mwindow->theme->search_case_y;
	add_subwindow(new SearchCase(mwindow, 
		x,
		y));
	y += 30;
	add_subwindow(new SearchBackward(mwindow, 
		x,
		y));

	add_subwindow(new SearchOK(mwindow, 
		mwindow->theme->search_ok_x,
		mwindow->theme->search_ok_y));
// 	add_subwindow(new SearchAgain(mwindow, 
// 		mwindow->theme->search_again_x,
// 		mwindow->theme->search_again_y));
	add_subwindow(new SearchCancel(mwindow, 
		this,
		mwindow->theme->search_cancel_x,
		mwindow->theme->search_cancel_y));

	show_window();
	flush();
}

int SearchGUI::keypress_event()
{
	switch(get_keypress())
	{
		case 'w':
			set_done(1);
			return 1;
			break;
	}
	return 0;
}

int SearchGUI::close_event()
{
	set_done(1);
	return 1;
}



