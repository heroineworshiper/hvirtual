
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

#include "automation.h"
#include "cicolors.h"
#include "clip.h"
#include "condition.h"
#include "cpanel.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "cwindowtool.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "maskauto.h"
#include "maskautos.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "transportque.h"


CWindowTool::CWindowTool(MWindow *mwindow, CWindowGUI *gui)
 : Thread()
{
	this->mwindow = mwindow;
	this->gui = gui;
	tool_gui = 0;
	done = 0;
	current_tool = CWINDOW_NONE;
	set_synchronous(1);
	input_lock = new Condition(0, "CWindowTool::input_lock");
	output_lock = new Condition(1, "CWindowTool::output_lock");
	tool_gui_lock = new Mutex("CWindowTool::tool_gui_lock");
}

CWindowTool::~CWindowTool()
{
	done = 1;
	stop_tool();
	input_lock->unlock();
	Thread::join();
	delete input_lock;
	delete output_lock;
	delete tool_gui_lock;
}

void CWindowTool::start_tool(int operation)
{
	CWindowToolGUI *new_gui = 0;
	int result = 0;

//printf("CWindowTool::start_tool 1\n");
	if(current_tool != operation)
	{
		current_tool = operation;
		switch(operation)
		{
			case CWINDOW_EYEDROP:
				new_gui = new CWindowEyedropGUI(mwindow, this);
				break;
			case CWINDOW_CROP:
				new_gui = new CWindowCropGUI(mwindow, this);
				break;
			case CWINDOW_CAMERA:
				new_gui = new CWindowCameraGUI(mwindow, this);
				break;
			case CWINDOW_PROJECTOR:
				new_gui = new CWindowProjectorGUI(mwindow, this);
				break;
			case CWINDOW_MASK:
				new_gui = new CWindowMaskGUI(mwindow, this);
				break;
			case CWINDOW_RULER:
				new_gui = new CWindowRulerGUI(mwindow, this);
				break;
			default:
				result = 1;
				stop_tool();
				break;
		}

//printf("CWindowTool::start_tool 1\n");


		if(!result)
		{
			stop_tool();
// Wait for previous tool GUI to finish
			output_lock->lock("CWindowTool::start_tool");
			this->tool_gui = new_gui;
			tool_gui->create_objects();
			
			if(mwindow->edl->session->tool_window &&
				mwindow->session->show_cwindow) tool_gui->show_window();
			tool_gui->lock_window("CWindowTool::start_tool 1");
			tool_gui->flush();
			tool_gui->unlock_window();
			
			
// Signal thread to run next tool GUI
			input_lock->unlock();
		}
//printf("CWindowTool::start_tool 1\n");
	}
	else
	if(tool_gui) 
	{
		tool_gui->lock_window("CWindowTool::start_tool");
		tool_gui->update();
		tool_gui->unlock_window();
	}

//printf("CWindowTool::start_tool 2\n");

}


void CWindowTool::stop_tool()
{
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::stop_tool");
		tool_gui->set_done(0);
		tool_gui->unlock_window();
	}
}

void CWindowTool::show_tool()
{
	if(tool_gui && mwindow->edl->session->tool_window)
	{
		tool_gui->lock_window("CWindowTool::show_tool");
		tool_gui->show_window();
		tool_gui->unlock_window();
	}
}

void CWindowTool::hide_tool()
{
	if(tool_gui && mwindow->edl->session->tool_window)
	{
		tool_gui->lock_window("CWindowTool::show_tool");
		tool_gui->hide_window();
		tool_gui->unlock_window();
	}
}


void CWindowTool::run()
{
	while(!done)
	{
		input_lock->lock("CWindowTool::run");
		if(!done)
		{
			tool_gui->run_window();
			tool_gui_lock->lock("CWindowTool::run");
			delete tool_gui;
			tool_gui = 0;
			tool_gui_lock->unlock();
		}
		output_lock->unlock();
	}
}

void CWindowTool::update_show_window()
{
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::update_show_window");

		if(mwindow->edl->session->tool_window) 
		{
			tool_gui->update();
			tool_gui->show_window();
		}
		else
			tool_gui->hide_window();
		tool_gui->flush();

		tool_gui->unlock_window();
	}
}

void CWindowTool::raise_window()
{
	if(tool_gui)
	{
		gui->unlock_window();
		tool_gui->lock_window("CWindowTool::raise_window");
		tool_gui->raise_window();
		tool_gui->unlock_window();
		gui->lock_window("CWindowTool::raise_window");
	}
}

void CWindowTool::update_values()
{
	tool_gui_lock->lock("CWindowTool::update_values");
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::update_values");
		tool_gui->update();
		tool_gui->flush();
		tool_gui->unlock_window();
	}
	tool_gui_lock->unlock();
}







CWindowToolGUI::CWindowToolGUI(MWindow *mwindow, 
	CWindowTool *thread, 
	const char *title,
	int w, 
	int h)
 : BC_Window(title,
 	mwindow->session->ctool_x,
	mwindow->session->ctool_y,
	w,
	h,
	w,
	h,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	current_operation = 0;
}

CWindowToolGUI::~CWindowToolGUI()
{
}

int CWindowToolGUI::close_event()
{
	hide_window();
	flush();
	mwindow->edl->session->tool_window = 0;
	unlock_window();



	thread->gui->lock_window("CWindowToolGUI::close_event");
	thread->gui->composite_panel->set_operation(mwindow->edl->session->cwindow_operation);
	thread->gui->flush();
	thread->gui->unlock_window();

	lock_window("CWindowToolGUI::close_event");
	return 1;
;}

int CWindowToolGUI::keypress_event()
{
	if(get_keypress() == 'w' || get_keypress() == 'W')
		return close_event();
	return 0;
}

int CWindowToolGUI::translation_event()
{
	mwindow->session->ctool_x = get_x();
	mwindow->session->ctool_y = get_y();
	return 0;
}






CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, float value)
 : BC_TumbleTextBox(gui, 
		(float)value,
		(float)-65536,
		(float)65536,
		x, 
		y, 
		100)
{
	this->gui = gui;
}

CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, int value)
 : BC_TumbleTextBox(gui, 
		(int64_t)value,
		(int64_t)-65536,
		(int64_t)65536,
		x, 
		y, 
		100)
{
	this->gui = gui;
}
int CWindowCoord::handle_event()
{
	gui->event_caller = this;
	gui->handle_event();
	return 1;
}


CWindowCropOK::CWindowCropOK(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Do it"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int CWindowCropOK::handle_event()
{
	mwindow->crop_video();
	return 1;
}


int CWindowCropOK::keypress_event()
{
	if(get_keypress() == 0xd) 
	{
		handle_event();
		return 1;
	}
	return 0;
}







CWindowCropGUI::CWindowCropGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
 	thread,
	PROGRAM_NAME ": Crop",
	330,
	100)
{
}


CWindowCropGUI::~CWindowCropGUI()
{
}

void CWindowCropGUI::create_objects()
{
	int x = 10, y = 10;
	BC_TumbleTextBox *textbox;
	BC_Title *title;

	lock_window("CWindowCropGUI::create_objects");
	int column1 = 0;
	int pad = MAX(BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1), 
		BC_Title::calculate_h(this, "X")) + 5;
	add_subwindow(title = new BC_Title(x, y, _("X1:")));
	column1 = MAX(column1, title->get_w());
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("W:")));
	column1 = MAX(column1, title->get_w());
	y += pad;
	add_subwindow(new CWindowCropOK(mwindow, thread->tool_gui, x, y));

	x += column1 + 5;
	y = 10;
	x1 = new CWindowCoord(thread->tool_gui, x, y, mwindow->edl->session->crop_x1);
	x1->create_objects();
	y += pad;
	width = new CWindowCoord(thread->tool_gui, 
		x, 
		y, 
		mwindow->edl->session->crop_x2 - 
			mwindow->edl->session->crop_x1);
	width->create_objects();


	x += x1->get_w() + 10;
	y = 10;
	int column2 = 0;
	add_subwindow(title = new BC_Title(x, y, _("Y1:")));
	column2 = MAX(column2, title->get_w());
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("H:")));
	column2 = MAX(column2, title->get_w());
	y += pad;

	y = 10;
	x += column2 + 5;
	y1 = new CWindowCoord(thread->tool_gui, x, y, mwindow->edl->session->crop_y1);
	y1->create_objects();
	y += pad;
	height = new CWindowCoord(thread->tool_gui, 
		x, 
		y, 
		mwindow->edl->session->crop_y2 - 
			mwindow->edl->session->crop_y1);
	height->create_objects();
	unlock_window();
}

void CWindowCropGUI::handle_event()
{
	int new_x1, new_y1;
	new_x1 = atol(x1->get_text());
	new_y1 = atol(y1->get_text());
	if(new_x1 != mwindow->edl->session->crop_x1)
	{
		mwindow->edl->session->crop_x2 = new_x1 +
			mwindow->edl->session->crop_x2 - 
			mwindow->edl->session->crop_x1;
		mwindow->edl->session->crop_x1 = new_x1;
	}
	if(new_y1 != mwindow->edl->session->crop_y1)
	{
		mwindow->edl->session->crop_y2 = new_y1 +
			mwindow->edl->session->crop_y2 -
			mwindow->edl->session->crop_y1;
		mwindow->edl->session->crop_y1 = atol(y1->get_text());
	}
 	mwindow->edl->session->crop_x2 = atol(width->get_text()) + 
 		mwindow->edl->session->crop_x1;
 	mwindow->edl->session->crop_y2 = atol(height->get_text()) + 
 		mwindow->edl->session->crop_y1;
	update();
	mwindow->cwindow->gui->lock_window("CWindowCropGUI::handle_event");
	mwindow->cwindow->gui->canvas->draw_refresh();
	mwindow->cwindow->gui->unlock_window();
}

void CWindowCropGUI::update()
{
	x1->update((int64_t)mwindow->edl->session->crop_x1);
	y1->update((int64_t)mwindow->edl->session->crop_y1);
	width->update((int64_t)mwindow->edl->session->crop_x2 - 
		mwindow->edl->session->crop_x1);
	height->update((int64_t)mwindow->edl->session->crop_y2 - 
		mwindow->edl->session->crop_y1);
}






CWindowEyedropGUI::CWindowEyedropGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
 	thread,
	PROGRAM_NAME ": Color",
	200,
	300)
{
}

CWindowEyedropGUI::~CWindowEyedropGUI()
{
}

void CWindowEyedropGUI::create_objects()
{
	int margin = mwindow->theme->widget_border;
	int x = margin;
	int y = margin;
	int x2 = 70;
	lock_window("CWindowEyedropGUI::create_objects");
	BC_Title *title1, *title2, *title3, *title4, *title5, *title6, *title7;
	add_subwindow(title7 = new BC_Title(x, y, "Radius:"));
	y += BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1) + margin;
	
	add_subwindow(title1 = new BC_Title(x, y, "Red:"));
	y += title1->get_h() + margin;
	add_subwindow(title2 = new BC_Title(x, y, "Green:"));
	y += title2->get_h() + margin;
	add_subwindow(title3 = new BC_Title(x, y, "Blue:"));
	y += title3->get_h() + margin;

	add_subwindow(title4 = new BC_Title(x, y, "Y:"));
	y += title4->get_h() + margin;
	add_subwindow(title5 = new BC_Title(x, y, "U:"));
	y += title5->get_h() + margin;
	add_subwindow(title6 = new BC_Title(x, y, "V:"));


	radius = new CWindowCoord(this,
		x2,
		title7->get_y(),
		mwindow->edl->session->eyedrop_radius);
	radius->create_objects();
	radius->set_boundaries((int64_t)0, (int64_t)255);


	add_subwindow(red = new BC_Title(x2, title1->get_y(), "0"));
	add_subwindow(green = new BC_Title(x2, title2->get_y(), "0"));
	add_subwindow(blue = new BC_Title(x2, title3->get_y(), "0"));

	add_subwindow(this->y = new BC_Title(x2, title4->get_y(), "0"));
	add_subwindow(this->u = new BC_Title(x2, title5->get_y(), "0"));
	add_subwindow(this->v = new BC_Title(x2, title6->get_y(), "0"));


	y += title6->get_h() + margin;
	add_subwindow(use_max = new CWindowEyedropCheckBox(mwindow, 
		this,
		x2, 
		y));

	y = title6->get_y() + this->v->get_h() + margin;
	add_subwindow(sample = new BC_SubWindow(x, y, 50, 50));
	update();
	unlock_window();
}

void CWindowEyedropGUI::update()
{
	radius->update((int64_t)mwindow->edl->session->eyedrop_radius);

	float y, u, v;
	if(mwindow->edl->local_session->use_max)
	{
		red->update(mwindow->edl->local_session->red_max);
		green->update(mwindow->edl->local_session->green_max);
		blue->update(mwindow->edl->local_session->blue_max);

		YUV::rgb_to_yuv_f(mwindow->edl->local_session->red_max, 
			mwindow->edl->local_session->green_max, 
			mwindow->edl->local_session->blue_max, 
			y, 
			u, 
			v);
	}
	else
	{
		red->update(mwindow->edl->local_session->red);
		green->update(mwindow->edl->local_session->green);
		blue->update(mwindow->edl->local_session->blue);

		YUV::rgb_to_yuv_f(mwindow->edl->local_session->red, 
			mwindow->edl->local_session->green, 
			mwindow->edl->local_session->blue, 
			y, 
			u, 
			v);
	}
	this->y->update(y);
	this->u->update(u);
	this->v->update(v);
	use_max->update(mwindow->edl->local_session->use_max);

	int red = (int)(CLIP(mwindow->edl->local_session->red, 0, 1) * 0xff);
	int green = (int)(CLIP(mwindow->edl->local_session->green, 0, 1) * 0xff);
	int blue = (int)(CLIP(mwindow->edl->local_session->blue, 0, 1) * 0xff);
	sample->set_color((red << 16) | (green << 8) | blue);
	sample->draw_box(0, 0, sample->get_w(), sample->get_h());
	sample->set_color(BLACK);
	sample->draw_rectangle(0, 0, sample->get_w(), sample->get_h());
	sample->flash();
}

void CWindowEyedropGUI::handle_event()
{
	int new_radius = atoi(radius->get_text());
	if(new_radius != mwindow->edl->session->eyedrop_radius)
	{
		CWindowGUI *gui = mwindow->cwindow->gui;
		if(gui->eyedrop_visible)
		{
			gui->lock_window("CWindowEyedropGUI::handle_event");
// hide it
			int rerender;
			gui->canvas->do_eyedrop(rerender, 0, 1);
		}

		mwindow->edl->session->eyedrop_radius = new_radius;

		if(gui->eyedrop_visible)
		{
// draw it
			int rerender;
			gui->canvas->do_eyedrop(rerender, 0, 1);
			gui->unlock_window();
		}
	}
}







CWindowEyedropCheckBox::CWindowEyedropCheckBox(MWindow *mwindow, 
	CWindowEyedropGUI *gui,
	int x, 
	int y)
 : BC_CheckBox(x, 
 	y, 
	mwindow->edl->local_session->use_max, 
	"Use maximum")
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CWindowEyedropCheckBox::handle_event()
{
	mwindow->edl->local_session->use_max = get_value();
	
	gui->update();
}







CWindowCameraGUI::CWindowCameraGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
 	thread,
	PROGRAM_NAME ": Camera",
	170,
	170)
{
}
CWindowCameraGUI::~CWindowCameraGUI()
{
}

void CWindowCameraGUI::create_objects()
{
	int x = 10, y = 10, x1;
	Track *track = mwindow->cwindow->calculate_affected_track();
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	BC_Title *title;
	BC_Button *button;

	lock_window("CWindowCameraGUI::create_objects");
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			&y_auto,
			&z_auto,
			track,
			1,
			0,
			0,
			0);
	}

	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w();
	this->x = new CWindowCoord(this, 
		x, 
		y, 
		x_auto ? x_auto->value : (float)0);
	this->x->create_objects();

	
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w();
	this->y = new CWindowCoord(this, 
		x, 
		y, 
		y_auto ? y_auto->value : (float)0);
	this->y->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Z:")));
	x += title->get_w();
	this->z = new CWindowCoord(this, 
		x, 
		y, 
		z_auto ? z_auto->value : (float)1);
	this->z->create_objects();
	this->z->set_increment(0.01);

	y += 30;
	x1 = 10;
	add_subwindow(button = new CWindowCameraLeft(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraCenter(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraRight(mwindow, this, x1, y));

	y += button->get_h();
	x1 = 10;
	add_subwindow(button = new CWindowCameraTop(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraMiddle(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraBottom(mwindow, this, x1, y));
	unlock_window();
}

void CWindowCameraGUI::update_preview()
{
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);

	mwindow->cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
	mwindow->cwindow->gui->lock_window("CWindowCameraGUI::update_preview");
	mwindow->cwindow->gui->canvas->draw_refresh();
	mwindow->cwindow->gui->unlock_window();
}


void CWindowCameraGUI::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->undo->update_undo_before(_("camera"), this);
		if(event_caller == x)
		{
			x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_CAMERA_X],
				1);
			if(x_auto)
			{
				x_auto->value = atof(x->get_text());
				update_preview();
			}
		}
		else
		if(event_caller == y)
		{
			y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_CAMERA_Y],
				1);
			if(y_auto)
			{
				y_auto->value = atof(y->get_text());
				update_preview();
			}
		}
		else
		if(event_caller == z)
		{
			z_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_CAMERA_Z],
				1);
			if(z_auto)
			{
				float zoom = atof(z->get_text());
				if(zoom > 10) zoom = 10; 
				else
				if(zoom < 0) zoom = 0;
	// Doesn't allow user to enter from scratch
	// 		if(zoom != atof(z->get_text())) 
	// 			z->update(zoom);

				z_auto->value = zoom;
				mwindow->gui->lock_window("CWindowCameraGUI::handle_event");
				mwindow->gui->draw_overlays(1);
				mwindow->gui->unlock_window();
				update_preview();
			}
		}

		mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
	}
}

void CWindowCameraGUI::update()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			&y_auto,
			&z_auto,
			track,
			1,
			0,
			0,
			0);
	}

	if(x_auto)
		x->update(x_auto->value);
	if(y_auto)
		y->update(y_auto->value);
	if(z_auto)
		z->update(z_auto->value);
}

// BezierAuto* CWindowCameraGUI::get_keyframe()
// {
// 	BezierAuto *keyframe = 0;
// 	Track *track = mwindow->cwindow->calculate_affected_track();
// 	if(track)
// 		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(
// 			track->automation->autos[AUTOMATION_CAMERA]);
// 	return keyframe;
// }



CWindowCameraLeft::CWindowCameraLeft(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("left_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}
int CWindowCameraLeft::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			0,
			&z_auto,
			track,
			1,
			1,
			0,
			0);
	}

	if(x_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			x_auto->value = 
				(double)track->track_w / z_auto->value / 2 - 
				(double)w / 2;
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
			gui->update();
			gui->update_preview();
		}
	}

	return 1;
}


CWindowCameraCenter::CWindowCameraCenter(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("center_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}
int CWindowCameraCenter::handle_event()
{
	FloatAuto *x_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_X],
			1);

	if(x_auto)
	{
		mwindow->undo->update_undo_before(_("camera"), 0);
		x_auto->value = 0;
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
	}

	return 1;
}


CWindowCameraRight::CWindowCameraRight(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("right_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}
int CWindowCameraRight::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			0,
			&z_auto,
			track,
			1,
			1,
			0,
			0);
	}

	if(x_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			x_auto->value = -((double)track->track_w / z_auto->value / 2 - 
				(double)w / 2);
			gui->update();
			gui->update_preview();
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
		}
	}

	return 1;
}


CWindowCameraTop::CWindowCameraTop(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("top_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}
int CWindowCameraTop::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(0,
			&y_auto,
			&z_auto,
			track,
			1,
			0,
			1,
			0);
	}

	if(y_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			y_auto->value = (double)track->track_h / z_auto->value / 2 - 
				(double)h / 2;
			gui->update();
			gui->update_preview();
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
		}
	}

	return 1;
}


CWindowCameraMiddle::CWindowCameraMiddle(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("middle_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}
int CWindowCameraMiddle::handle_event()
{
	FloatAuto *y_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_Y], 1);

	if(y_auto)
	{
		mwindow->undo->update_undo_before(_("camera"), 0);
		y_auto->value = 0;
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
	}

	return 1;
}


CWindowCameraBottom::CWindowCameraBottom(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("bottom_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}
int CWindowCameraBottom::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(0,
			&y_auto,
			&z_auto,
			track,
			1,
			0,
			1,
			0);
	}

	if(y_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			y_auto->value = -((double)track->track_h / z_auto->value / 2 - 
				(double)h / 2);
			gui->update();
			gui->update_preview();
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
		}
	}

	return 1;
}

















CWindowProjectorGUI::CWindowProjectorGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
 	thread,
	PROGRAM_NAME ": Projector",
	170,
	170)
{
}
CWindowProjectorGUI::~CWindowProjectorGUI()
{
}
void CWindowProjectorGUI::create_objects()
{
	int x = 10, y = 10, x1;
	Track *track = mwindow->cwindow->calculate_affected_track();
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	BC_Title *title;
	BC_Button *button;

	lock_window("CWindowProjectorGUI::create_objects");
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			&y_auto,
			&z_auto,
			track,
			0,
			0,
			0,
			0);
	}

	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w();
	this->x = new CWindowCoord(this, 
		x, 
		y, 
		x_auto ? x_auto->value : (float)0);
	this->x->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w();
	this->y = new CWindowCoord(this, 
		x, 
		y, 
		y_auto ? y_auto->value : (float)0);
	this->y->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Z:")));
	x += title->get_w();
	this->z = new CWindowCoord(this, 
		x, 
		y, 
		z_auto ? z_auto->value : (float)1);
	this->z->create_objects();
	this->z->set_increment(0.01);

	y += 30;
	x1 = 10;
	add_subwindow(button = new CWindowProjectorLeft(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorCenter(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorRight(mwindow, this, x1, y));

	y += button->get_h();
	x1 = 10;
	add_subwindow(button = new CWindowProjectorTop(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorMiddle(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorBottom(mwindow, this, x1, y));
	unlock_window();
}

void CWindowProjectorGUI::update_preview()
{
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
	mwindow->cwindow->gui->lock_window("CWindowProjectorGUI::update_preview");
	mwindow->cwindow->gui->canvas->draw_refresh();
	mwindow->cwindow->gui->unlock_window();
}

void CWindowProjectorGUI::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		mwindow->undo->update_undo_before(_("projector"), this);
		if(event_caller == x)
		{
			x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_PROJECTOR_X],
				1);
			if(x_auto)
			{
				x_auto->value = atof(x->get_text());
				update_preview();
			}
		}
		else
		if(event_caller == y)
		{
			y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_PROJECTOR_Y],
				1);
			if(y_auto)
			{
				y_auto->value = atof(y->get_text());
				update_preview();
			}
		}
		else
		if(event_caller == z)
		{
			z_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_PROJECTOR_Z],
				1);
			if(z_auto)
			{
				float zoom = atof(z->get_text());
				if(zoom > 10000) zoom = 10000; 
				else 
				if(zoom < 0) zoom = 0;
// 			if (zoom != atof(z->get_text())) 
// 				z->update(zoom);
				z_auto->value = zoom;

				mwindow->gui->lock_window("CWindowProjectorGUI::handle_event");
				mwindow->gui->draw_overlays(1);
				mwindow->gui->unlock_window();

				update_preview();
			}
		}
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}
}

void CWindowProjectorGUI::update()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			&y_auto,
			&z_auto,
			track,
			0,
			0,
			0,
			0);
	}

	if(x_auto)
		x->update(x_auto->value);
	if(y_auto)
		y->update(y_auto->value);
	if(z_auto)
		z->update(z_auto->value);
}

// BezierAuto* CWindowProjectorGUI::get_keyframe()
// {
// 	BezierAuto *keyframe = 0;
// 	Track *track = mwindow->cwindow->calculate_affected_track();
// 	if(track)
// 		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(
// 			track->automation->autos[AUTOMATION_PROJECTOR]);
// 	return keyframe;
// }






































CWindowProjectorLeft::CWindowProjectorLeft(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("left_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}
int CWindowProjectorLeft::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			0,
			&z_auto,
			track,
			0,
			1,
			0,
			0);
	}
	if(x_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		x_auto->value = (double)track->track_w * z_auto->value / 2 - 
			(double)mwindow->edl->session->output_w / 2;
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorCenter::CWindowProjectorCenter(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("center_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}
int CWindowProjectorCenter::handle_event()
{
	FloatAuto *x_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_X],
			1);

	if(x_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		x_auto->value = 0;
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorRight::CWindowProjectorRight(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("right_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}
int CWindowProjectorRight::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			0,
			&z_auto,
			track,
			0,
			1,
			0,
			0);
	}

	if(x_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		x_auto->value = -((double)track->track_w * z_auto->value / 2 - 
			(double)mwindow->edl->session->output_w / 2);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorTop::CWindowProjectorTop(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("top_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}
int CWindowProjectorTop::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(0,
			&y_auto,
			&z_auto,
			track,
			0,
			0,
			1,
			0);
	}

	if(y_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		y_auto->value = (double)track->track_h * z_auto->value / 2 - 
			(double)mwindow->edl->session->output_h / 2;
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorMiddle::CWindowProjectorMiddle(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("middle_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}
int CWindowProjectorMiddle::handle_event()
{
	FloatAuto *y_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_Y], 1);

	if(y_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		y_auto->value = 0;
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorBottom::CWindowProjectorBottom(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("bottom_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}
int CWindowProjectorBottom::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(0,
			&y_auto,
			&z_auto,
			track,
			0,
			0,
			1,
			0);
	}

	if(y_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		y_auto->value = -((double)track->track_h * z_auto->value / 2 - 
			(double)mwindow->edl->session->output_h / 2);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}








CWindowMaskMode::CWindowMaskMode(MWindow *mwindow, 
	CWindowToolGUI *gui, 
	int x, 
	int y,
	const char *text)
 : BC_PopupMenu(x,
 	y,
	200,
	text,
	1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void CWindowMaskMode::create_objects()
{
	add_item(new BC_MenuItem(mode_to_text(MASK_MULTIPLY_ALPHA)));
	add_item(new BC_MenuItem(mode_to_text(MASK_SUBTRACT_ALPHA)));
}

char* CWindowMaskMode::mode_to_text(int mode)
{
	switch(mode)
	{
		case MASK_MULTIPLY_ALPHA:
			return _("Multiply alpha");
			break;
		
		case MASK_SUBTRACT_ALPHA:
			return _("Subtract alpha");
			break;
	}

	return _("Subtract alpha");
}

int CWindowMaskMode::text_to_mode(char *text)
{
	if(!strcasecmp(text, _("Multiply alpha")))
		return MASK_MULTIPLY_ALPHA;
	else
	if(!strcasecmp(text, _("Subtract alpha")))
		return MASK_SUBTRACT_ALPHA;

	return MASK_SUBTRACT_ALPHA;
}

int CWindowMaskMode::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
// Get existing keyframe
	((CWindowMaskGUI*)gui)->get_keyframe(track, 
		autos,
		keyframe, 
		mask,
		point,
		0);
	if(track)
	{
		mwindow->undo->update_undo_before(_("mask mode"), 0);
#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Update parameter
		temp_keyframe.mode = text_to_mode(get_text());
// Commit change to span of keyframes
		autos->update_parameter(&temp_keyframe);
#else
		((MaskAuto*)autos->default_auto)->mode = 
			text_to_mode(get_text());
#endif
		mwindow->undo->update_undo_after(_("mask mode"), LOAD_AUTOMATION);
	}

//printf("CWindowMaskMode::handle_event 1\n");
	gui->update_preview();
	return 1;
}








CWindowMaskDelete::CWindowMaskDelete(MWindow *mwindow, 
	CWindowToolGUI *gui, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CWindowMaskDelete::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

// Get existing keyframe
	((CWindowMaskGUI*)gui)->get_keyframe(track, 
		autos,
		keyframe, 
		mask,
		point,
		0);

	if(track)
	{
		mwindow->undo->update_undo_before(_("mask delete"), 0);

#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Update parameter
		SubMask *submask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
		for(int i = mwindow->cwindow->gui->affected_point;
			i < submask->points.total - 1;
			i++)
		{
			*submask->points.values[i] = *submask->points.values[i + 1];
		}

		if(submask->points.total)
		{
			submask->points.remove_object(
				submask->points.values[submask->points.total - 1]);
		}
// Commit change to span of keyframes
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->update_parameter(&temp_keyframe);
#else
		for(MaskAuto *current = (MaskAuto*)autos->default_auto;
			current; )
		{
			SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);


			
			for(int i = mwindow->cwindow->gui->affected_point;
				i < submask->points.total - 1;
				i++)
			{
				*submask->points.values[i] = *submask->points.values[i + 1];
			}

			if(submask->points.total)
			{
				submask->points.remove_object(
					submask->points.values[submask->points.total - 1]);
			}


			if(current == (MaskAuto*)autos->default_auto)
				current = (MaskAuto*)autos->first;
			else
				current = (MaskAuto*)NEXT;
		}
#endif

		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("mask delete"), LOAD_AUTOMATION);
	}


	return 1;
}

int CWindowMaskDelete::keypress_event()
{
	if(get_keypress() == BACKSPACE ||
		get_keypress() == DELETE) 
		return handle_event();
	return 0;
}


// CWindowMaskCycleNext::CWindowMaskCycleNext(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
//  : BC_GenericButton(x, y, _("Cycle next"))
// {
// 	this->mwindow = mwindow;
// 	this->gui = gui;
// }
// int CWindowMaskCycleNext::handle_event()
// {
// 	MaskAuto *keyframe;
// 	Track *track;
// 	MaskPoint *point;
// 	SubMask *mask;
// 	((CWindowMaskGUI*)gui)->get_keyframe(track, 
// 		keyframe,
// 		mask,  
// 		point,
// 		0);
// 
// 	MaskPoint *temp;
// 
// // Should apply to all keyframes
// 	if(keyframe && mask->points.total)
// 	{
// 		temp = mask->points.values[0];
// 
// 		for(int i = 0; i < mask->points.total - 1; i++)
// 		{
// 			mask->points.values[i] = mask->points.values[i + 1];
// 		}
// 		mask->points.values[mask->points.total - 1] = temp;
// 
// 		mwindow->cwindow->gui->affected_point--;
// 		if(mwindow->cwindow->gui->affected_point < 0)
// 			mwindow->cwindow->gui->affected_point = mask->points.total - 1;
// 
// 		gui->update();
// 		gui->update_preview();
// 	}
// 	
// 	return 1;
// }
// 
// CWindowMaskCyclePrev::CWindowMaskCyclePrev(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
//  : BC_GenericButton(x, y, _("Cycle prev"))
// {
// 	this->mwindow = mwindow;
// 	this->gui = gui;
// }
// int CWindowMaskCyclePrev::handle_event()
// {
// 	MaskAuto *keyframe;
// 	Track *track;
// 	MaskPoint *point;
// 	SubMask *mask;
// 	((CWindowMaskGUI*)gui)->get_keyframe(track, 
// 		keyframe,
// 		mask, 
// 		point,
// 		0);
// 
// // Should apply to all keyframes
// 	MaskPoint *temp;
// 	if(keyframe && mask->points.total)
// 	{
// 		temp = mask->points.values[mask->points.total - 1];
// 
// 		for(int i = mask->points.total - 1; i > 0; i--)
// 		{
// 			mask->points.values[i] = mask->points.values[i - 1];
// 		}
// 		mask->points.values[0] = temp;
// 
// 		mwindow->cwindow->gui->affected_point++;
// 		if(mwindow->cwindow->gui->affected_point >= mask->points.total)
// 			mwindow->cwindow->gui->affected_point = 0;
// 
// 		gui->update();
// 		gui->update_preview();
// 	}
// 	return 1;
// }


CWindowMaskNumber::CWindowMaskNumber(MWindow *mwindow, 
	CWindowToolGUI *gui, 
	int x, 
	int y)
 : BC_TumbleTextBox(gui, 
		(int64_t)mwindow->edl->session->cwindow_mask,
		(int64_t)0,
		(int64_t)SUBMASKS - 1,
		x, 
		y, 
		100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskNumber::~CWindowMaskNumber()
{
}

int CWindowMaskNumber::handle_event()
{
	mwindow->edl->session->cwindow_mask = atol(get_text());
	gui->update();
	gui->update_preview();
	return 1;
}





CWindowMaskFeather::CWindowMaskFeather(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui, 
		(int64_t)0,
		(int64_t)0,
		(int64_t)0xff,
		x, 
		y, 
		100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
CWindowMaskFeather::~CWindowMaskFeather()
{
}
int CWindowMaskFeather::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

	mwindow->undo->update_undo_before(_("mask feather"), this);

// Get existing keyframe
	((CWindowMaskGUI*)gui)->get_keyframe(track, 
		autos,
		keyframe,
		mask, 
		point,
#ifdef USE_KEYFRAME_SPANNING
		0);
#else
		1);
#endif

	if(track)
	{	
#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Update parameter
		temp_keyframe.feather = atof(get_text());
// Commit change to span of keyframes
		autos->update_parameter(&temp_keyframe);
#else
		keyframe->feather = atof(get_text());
#endif

		gui->update_preview();
	}

	mwindow->undo->update_undo_after(_("mask feather"), LOAD_AUTOMATION);
	return 1;
}

CWindowMaskValue::CWindowMaskValue(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_ISlider(x, 
			y,
			0,
			200, 
			200, 
			0, 
			100, 
			0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskValue::~CWindowMaskValue()
{
}

int CWindowMaskValue::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	
	mwindow->undo->update_undo_before(_("mask value"), this);
	((CWindowMaskGUI*)gui)->get_keyframe(track, 
		autos,
		keyframe,
		mask, 
		point,
#ifdef USE_KEYFRAME_SPANNING
		0);
#else
		1);
#endif

	if(track)
	{
#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Update parameter
		temp_keyframe.value = get_value();
// Commit change to span of keyframes
		autos->update_parameter(&temp_keyframe);
#else
		keyframe->value = get_value();
#endif
	}

	gui->update_preview();
	mwindow->undo->update_undo_after(_("mask value"), LOAD_AUTOMATION);
	return 1;
}











CWindowMaskGUI::CWindowMaskGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
 	thread,
	PROGRAM_NAME ": Mask",
	330,
	280)
{
	this->mwindow = mwindow;
	this->thread = thread;
}
CWindowMaskGUI::~CWindowMaskGUI()
{
	lock_window("CWindowMaskGUI::~CWindowMaskGUI");
	delete number;
	delete feather;
	unlock_window();
}

void CWindowMaskGUI::create_objects()
{
	int x = 10, y = 10, margin = mwindow->theme->widget_border;
	MaskAuto *keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->autos[AUTOMATION_MASK], 0);

	lock_window("CWindowMaskGUI::create_objects");
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	add_subwindow(mode = new CWindowMaskMode(mwindow, 
		this, 
		x + title->get_w() + margin, 
		y,
		""));
	mode->create_objects();
	y += mode->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Value:")));
	add_subwindow(value = new CWindowMaskValue(mwindow, this, x + title->get_w() + margin, y));
	y += value->get_h() + margin;
	add_subwindow(delete_point = new CWindowMaskDelete(mwindow, this, x, y));
	y += delete_point->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Mask number:")));
	number = new CWindowMaskNumber(mwindow, 
		this, 
		x + title->get_w() + margin, 
		y);
	number->create_objects();
	y += number->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Feather:")));
	feather = new CWindowMaskFeather(mwindow,
		this,
		x + title->get_w() + margin,
		y);
	feather->create_objects();
	y += feather->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w() + margin;
	this->x = new CWindowCoord(this, 
		x, 
		y, 
		(float)0.0);
	this->x->create_objects();
	x += this->x->get_w() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w() + margin;
	this->y = new CWindowCoord(this, 
		x, 
		y, 
		(float)0.0);
	this->y->create_objects();
	
	x = 10;
	y += this->y->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Press Ctrl to move a point")));
	y += title->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Press Alt to translate the mask")));
	y += title->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Press Shift to edit bezier curve")));

	update();
	unlock_window();
}

void CWindowMaskGUI::get_keyframe(Track* &track, 
	MaskAutos* &autos,
	MaskAuto* &keyframe, 
	SubMask* &mask, 
	MaskPoint* &point,
	int create_it)
{
	autos = 0;
	keyframe = 0;

	track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		autos = (MaskAutos*)track->automation->autos[AUTOMATION_MASK];
		keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(
			autos, 
			create_it);
	}

	if(keyframe)
		mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);
	else
		mask = 0;

	point = 0;
	if(keyframe)
	{
		if(mwindow->cwindow->gui->affected_point < mask->points.total &&
			mwindow->cwindow->gui->affected_point >= 0)
		{
			point = mask->points.values[mwindow->cwindow->gui->affected_point];
		}
	}
}

void CWindowMaskGUI::update()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
//printf("CWindowMaskGUI::update 1\n");
	get_keyframe(track, 
		autos,
		keyframe, 
		mask,
		point,
		0);

	double position = mwindow->edl->local_session->get_selectionstart(1);
	position = mwindow->edl->align_to_frame(position, 0);
	if(track)
	{
		int64_t position_i = track->to_units(position, 0);

		if(point)
		{
			x->update(point->x);
			y->update(point->y);
		}

		if(mask)
		{
			feather->update((int64_t)autos->get_feather(position_i, PLAY_FORWARD));
			value->update((int64_t)autos->get_value(position_i, PLAY_FORWARD));
		}
	}
//printf("CWindowMaskGUI::update 1\n");

	number->update((int64_t)mwindow->edl->session->cwindow_mask);

//printf("CWindowMaskGUI::update 1\n");
	if(track)
	{
#ifdef USE_KEYFRAME_SPANNING
		mode->set_text(
			CWindowMaskMode::mode_to_text(keyframe->mode));
#else
		mode->set_text(
			CWindowMaskMode::mode_to_text(((MaskAuto*)autos->default_auto)->mode));
#endif
	}
//printf("CWindowMaskGUI::update 2\n");
}

void CWindowMaskGUI::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	get_keyframe(track, 
		autos,
		keyframe, 
		mask,
		point,
		0);

	mwindow->undo->update_undo_before(_("mask point"), this);

	if(point)
	{
#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Get affected point in temp keyframe
		mask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
		if(mwindow->cwindow->gui->affected_point < mask->points.total &&
			mwindow->cwindow->gui->affected_point >= 0)
		{
			point = mask->points.values[mwindow->cwindow->gui->affected_point];
		}

		if(point)
		{
			point->x = atof(x->get_text());
			point->y = atof(y->get_text());
// Commit to spanned keyframes
			autos->update_parameter(&temp_keyframe);
		}
#else
		point->x = atof(x->get_text());
		point->y = atof(y->get_text());
#endif
	}

	update_preview();
	mwindow->undo->update_undo_after(_("mask point"), LOAD_AUTOMATION);
}

void CWindowMaskGUI::update_preview()
{
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
	mwindow->cwindow->gui->lock_window("CWindowMaskGUI::update_preview");
	mwindow->cwindow->gui->canvas->draw_refresh();
	mwindow->cwindow->gui->unlock_window();
}








CWindowRulerGUI::CWindowRulerGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
 	thread,
	PROGRAM_NAME ": Ruler",
	320,
	240)
{
}

CWindowRulerGUI::~CWindowRulerGUI()
{
}

void CWindowRulerGUI::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title;

	lock_window("CWindowRulerGUI::create_objects");
	add_subwindow(title = new BC_Title(x, y, "Current:"));
	add_subwindow(current = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 5;
	
	add_subwindow(title = new BC_Title(x, y, "Point 1:"));
	add_subwindow(point1 = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 5;

	add_subwindow(title = new BC_Title(x, y, "Point 2:"));
	add_subwindow(point2 = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 5;

	add_subwindow(title = new BC_Title(x, y, "Distance:"));
	add_subwindow(distance = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 5;
	add_subwindow(title = new BC_Title(x, y, "Angle:"));
	add_subwindow(angle = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 10;
	char string[BCTEXTLEN];
	sprintf(string, _("Press Ctrl to lock ruler to the\nnearest 45%c angle."), 0xb0);
	add_subwindow(title = new BC_Title(x, 
		y, 
		string));
	y += title->get_h() + 10;
	sprintf(string, _("Press Alt to translate the ruler."));
	add_subwindow(title = new BC_Title(x, 
		y, 
		string));
	update();
	unlock_window();
}

void CWindowRulerGUI::update()
{
	double distance_value = 
		sqrt(SQR(mwindow->edl->session->ruler_x2 - mwindow->edl->session->ruler_x1) +
		SQR(mwindow->edl->session->ruler_y2 - mwindow->edl->session->ruler_y1));
	double angle_value = atan((mwindow->edl->session->ruler_y2 - mwindow->edl->session->ruler_y1) /
		(mwindow->edl->session->ruler_x2 - mwindow->edl->session->ruler_x1)) * 
		360 /
		2 / 
		M_PI;

	if(EQUIV(distance_value, 0.0)) 
	{
		angle_value = 0.0;
	}
	else
	if(angle_value < 0)
	{
		angle_value *= -1;
	}

	char string[BCTEXTLEN];
	sprintf(string, "%d, %d", 
		mwindow->session->cwindow_output_x,
		mwindow->session->cwindow_output_y);
	current->update(string);
	sprintf(string, "%.0f, %.0f", 
		mwindow->edl->session->ruler_x1,
		mwindow->edl->session->ruler_y1);
	point1->update(string);
	sprintf(string, "%.0f, %.0f", 
		mwindow->edl->session->ruler_x2,
		mwindow->edl->session->ruler_y2);
	point2->update(string);

	sprintf(string, "%0.01f pixels", distance_value);
	distance->update(string);
	sprintf(string, "%0.02f %c", angle_value, 0xb0);
	angle->update(string);
}

void CWindowRulerGUI::handle_event()
{
}




