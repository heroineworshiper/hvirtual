/*
 * CINELERRA
 * Copyright (C) 2011-2024 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "bcdialog.h"
#include "bcsignals.h"
#include "channelpicker.h"
#include "condition.h"
#include "cursors.h"
#include "libdv.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "record.h"
#include "recordconfig.h"
#include "recordgui.h"
#include "recordscopes.h"
#include "recordtransport.h"
#include "recordmonitor.h"
#include "screencapedit.h"
#include "theme.h"
#include "videodevice.inc"
#include "vframe.h"
#include "videodevice.h"


RecordMonitor::RecordMonitor(MWindow *mwindow, Record *record)
 : Thread(1)
{
	this->mwindow = mwindow;
	this->record = record;
	device = 0;
	thread = 0;
	scope_thread = 0;
}


RecordMonitor::~RecordMonitor()
{
	if(thread)
	{
		thread->stop_playback();
		delete thread;
	}
	window->set_done(0);
	Thread::join();
	if(device) 
	{
		device->close_all();
		delete device;
	}
	delete scope_thread;
	delete window;
}

void RecordMonitor::create_objects()
{
	int min_w = 150;
	mwindow->session->rwindow_fullscreen = 0;

	if(!record->default_asset->video_data)
		min_w = MeterPanel::get_meters_width(
			mwindow->theme,
			record->default_asset->channels, 1);



//SET_TRACE
	window = new RecordMonitorGUI(mwindow,
		record, 
		this,
		min_w);
//SET_TRACE
	window->create_objects();
//SET_TRACE

	if(record->default_asset->video_data)
	{
// Configure the output for record monitoring
		VideoOutConfig config;
//SET_TRACE
		device = new VideoDevice;
//SET_TRACE



// Override default device for X11 drivers
		if(MWindow::preferences->playback_config->vconfig->driver ==
			PLAYBACK_X11_XV) config.driver = PLAYBACK_X11_XV;
		config.x11_use_fields = 0;

//SET_TRACE

		device->open_output(&config, 
			record->default_asset->frame_rate, 
			record->default_asset->width, 
			record->default_asset->height,
			window->canvas,
			0);
// printf("RecordMonitor::create_objects %d w=%d h=%d\n", 
// __LINE__, 
// record->default_asset->width, 
// record->default_asset->height);
//SET_TRACE

		scope_thread = new RecordScopeThread(mwindow, this);

		if(mwindow->session->record_scope)
		{
			scope_thread->start();
		}


		thread = new RecordMonitorThread(mwindow, record, this);
//SET_TRACE
		thread->start_playback();
//SET_TRACE
	}
//SET_TRACE

	Thread::start();
}


void RecordMonitor::run()
{
	window->run_window();
	close_threads();
}

int RecordMonitor::close_threads()
{
	if(window->channel_picker) window->channel_picker->close_threads();
    return 0;
}

int RecordMonitor::update(VFrame *vframe)
{
	return thread->write_frame(vframe);
}

void RecordMonitor::update_channel(char *text)
{
	if(window->channel_picker)
		window->channel_picker->channel_text->update(text);
}

int RecordMonitor::get_mbuttons_height()
{
	return RECBUTTON_HEIGHT;
}

// int RecordMonitor::fix_size(int &w, int &h, int width_given, float aspect_ratio)
// {
// 	w = width_given;
// 	h = (int)((float)width_given / aspect_ratio);
//     return 0;
// }

// float RecordMonitor::get_scale(int w)
// {
// 	if(mwindow->edl->get_aspect_ratio() > 
// 		(float)record->frame_w / record->frame_h)
// 	{
// 		return (float)w / 
// 			((float)record->frame_h * 
// 			mwindow->edl->get_aspect_ratio());
// 	}
// 	else
// 	{
// 		return (float)w / record->frame_w;
// 	}
// }

int RecordMonitor::get_canvas_height()
{
	return window->get_h() - get_mbuttons_height();
}

// int RecordMonitor::get_channel_x()
// {
// //	return 240;
// 	return 5;
// }
// 
// int RecordMonitor::get_channel_y()
// {
// 	return 2;
// }










RecordMonitorGUI::RecordMonitorGUI(MWindow *mwindow,
	Record *record, 
	RecordMonitor *thread,
	int min_w)
 : BC_Window(PROGRAM_NAME ": Video in", 
 			mwindow->session->rmonitor_x,
			mwindow->session->rmonitor_y,
 			mwindow->session->rmonitor_w, 
 			mwindow->session->rmonitor_h, 
			min_w, 
			50, 
			1, 
			1,
			1)
{
//printf("%d %d\n", mwindow->session->rmonitor_w, mwindow->theme->rmonitor_meter_x);
	this->mwindow = mwindow;
	this->thread = thread;
	this->record = record;
//	avc = 0;
//	avc1394_transport = 0;
//	avc1394transport_title = 0;
//	avc1394transport_timecode = 0;
//	avc1394transport_thread = 0;
	bitmap = 0;
	channel_picker = 0;
	reverse_interlace = 0;
	meters = 0;
	canvas = 0;
//	cursor_toggle = 0;
//	big_cursor_toggle = 0;
    screencap = 0;
	current_operation = MONITOR_NONE;
}

RecordMonitorGUI::~RecordMonitorGUI()
{
	lock_window("RecordMonitorGUI::~RecordMonitorGUI");
	delete canvas;
// 	if(cursor_toggle)
// 	{
// 		delete cursor_toggle;
// 		delete big_cursor_toggle;
// 	}
    delete screencap;
	if(bitmap) delete bitmap;
	if(channel_picker) delete channel_picker;
//	if(avc1394transport_thread)
//		delete avc1394transport_thread;
//	if(avc)
//	{
//		delete avc;
//	}
//	if(avc1394_transport)
//	{
//		delete avc1394_transport;
//	}
//	if(avc1394transport_title)
//		delete avc1394transport_title;
	unlock_window();
}

void RecordMonitorGUI::create_objects()
{
// y offset for video canvas if we have the transport controls
	lock_window("RecordMonitorGUI::create_objects");
    VideoInConfig *in_config = MWindow::preferences->vconfig_in;
	int driver = in_config->driver;
	int do_channel = (driver == VIDEO4LINUX ||
			driver == CAPTURE_BUZ ||
			driver == VIDEO4LINUX2 
// ||
//			driver == VIDEO4LINUX2MJPG ||
//			driver == VIDEO4LINUX2JPEG ||
//			driver == CAPTURE_JPEG_WEBCAM ||
//			driver == CAPTURE_YUYV_WEBCAM ||
//			driver == CAPTURE_MPEG
        );
	int do_scopes = do_channel || (driver == SCREENCAPTURE);
	int do_interlace = (driver == CAPTURE_BUZ ||
		(driver == VIDEO4LINUX2 &&
            in_config->v4l2_format == CAPTURE_MJPG));
	int background_done = 0;
	int x = mwindow->theme->widget_border;
	int y = mwindow->theme->widget_border;

	mwindow->theme->get_rmonitor_sizes(record->default_asset->audio_data, 
		record->default_asset->video_data,
		do_channel || do_scopes,
		do_interlace,
		0,
		record->default_asset->channels);






	if(record->default_asset->video_data)
	{
// 		if(driver == CAPTURE_FIREWIRE ||
// 			driver == CAPTURE_IEC61883)
// 		{
// 			avc = new AVC1394Control;
// 			if(avc->device > -1)
// 			{
// 				mwindow->theme->get_rmonitor_sizes(record->default_asset->audio_data, 
// 					record->default_asset->video_data,
// 					do_channel,
// 					do_interlace,
// 					1,
// 					record->default_asset->channels);
// 				mwindow->theme->draw_rmonitor_bg(this);
// 				background_done = 1;
// 
// 				avc1394_transport = new AVC1394Transport(mwindow,
// 					avc,
// 					this,
// 					mwindow->theme->rmonitor_tx_x,
// 					mwindow->theme->rmonitor_tx_y);
// 				avc1394_transport->create_objects();
// 
// 				add_subwindow(avc1394transport_timecode =
// 					new BC_Title(avc1394_transport->x_end,
// 						mwindow->theme->rmonitor_tx_y + 10,
// 						_("00:00:00:00"),
// 						MEDIUM_7SEGMENT,
// 						BLACK));
// 
// 				avc1394transport_thread =
// 					new AVC1394TransportThread(avc1394transport_timecode,
// 						avc);
// 
// 				avc1394transport_thread->start();
// 
// 			}
// 		}



		if(!background_done)
		{
			mwindow->theme->draw_rmonitor_bg(this);
			background_done = 1;
		}

		mwindow->theme->rmonitor_canvas_w = MAX(10, mwindow->theme->rmonitor_canvas_w);
		mwindow->theme->rmonitor_canvas_h = MAX(10, mwindow->theme->rmonitor_canvas_h);
		canvas = new RecordMonitorCanvas(mwindow, 
			this,
			record, 
			mwindow->theme->rmonitor_canvas_x, 
			mwindow->theme->rmonitor_canvas_y, 
			mwindow->theme->rmonitor_canvas_w, 
			mwindow->theme->rmonitor_canvas_h);
		canvas->create_objects(0);

		if(do_channel)
		{
			channel_picker = new RecordChannelPicker(mwindow,
				record,
				thread,
				this,
				record->channeldb,
				mwindow->theme->rmonitor_channel_x, 
				mwindow->theme->rmonitor_channel_y);
			channel_picker->create_objects();
			x += channel_picker->get_w() + mwindow->theme->widget_border;
		}

		if(do_interlace)
		{
			add_subwindow(reverse_interlace = new ReverseInterlace(record,
				mwindow->theme->rmonitor_interlace_x, 
				mwindow->theme->rmonitor_interlace_y));
			x += reverse_interlace->get_w() + mwindow->theme->widget_border;
		}

		if(do_scopes)
		{
			add_subwindow(scope_toggle = new ScopeEnable(mwindow, 
				thread, 
				x, 
				y));
			x += scope_toggle->get_w() + mwindow->theme->widget_border;
		}
		
		
		if(driver == SCREENCAPTURE)
		{
// 			add_subwindow(cursor_toggle = new DoCursor(record,
// 				x, 
// 				y));
// 			x += cursor_toggle->get_w() + mwindow->theme->widget_border;
// 			add_subwindow(big_cursor_toggle = new DoBigCursor(record,
// 				x, 
// 				y));
// 			x += big_cursor_toggle->get_w() + mwindow->theme->widget_border;
            add_subwindow(screencap = new ScreenCapEdit(this, x, y));
		}
		

		add_subwindow(monitor_menu = new BC_PopupMenu(0, 
			0, 
			0, 
			"", 
			0));
		monitor_menu->add_item(new RecordMonitorFullsize(mwindow, 
			this));
	}


	if(!background_done)
	{
		mwindow->theme->draw_rmonitor_bg(this);
		background_done = 1;
	}

	if(record->default_asset->audio_data)
	{
		meters = new MeterPanel(mwindow, 
			this,
			mwindow->theme->rmonitor_meter_x,
			mwindow->theme->rmonitor_meter_y,
			record->default_asset->video_data ? -1 : mwindow->theme->rmonitor_meter_w,
			mwindow->theme->rmonitor_meter_h,
			record->default_asset->channels,
			1,
			1,
			0);
		meters->create_objects();
	}
	unlock_window();
}

int RecordMonitorGUI::button_press_event()
{
	if(mwindow->session->rwindow_fullscreen && canvas && canvas->get_canvas())
		return canvas->button_press_event_base(canvas->get_canvas());
		
	if(get_buttonpress() == 2)
	{
		return 0;
	}
	else
// Right button
	if(get_buttonpress() == 3)
	{
		monitor_menu->activate_menu();
		return 1;
	}
	return 0;
}

int RecordMonitorGUI::cursor_leave_event()
{
	if(canvas && canvas->get_canvas())
		return canvas->cursor_leave_event_base(canvas->get_canvas());
	return 0;
}

int RecordMonitorGUI::cursor_enter_event()
{
	if(canvas && canvas->get_canvas())
		return canvas->cursor_enter_event_base(canvas->get_canvas());
	return 0;
}

int RecordMonitorGUI::button_release_event()
{
	if(canvas && canvas->get_canvas())
		return canvas->button_release_event();
	return 0;
}

int RecordMonitorGUI::cursor_motion_event()
{
//SET_TRACE
	if(canvas && canvas->get_canvas())
	{
//SET_TRACE
		canvas->get_canvas()->unhide_cursor();
//SET_TRACE
		return canvas->cursor_motion_event();
	}
	return 0;
}

int RecordMonitorGUI::keypress_event()
{
	int result = 0;

	switch(get_keypress())
	{
		case LEFT:
			if(!ctrl_down()) 
			{ 
				record->record_gui->set_translation(--(record->video_x), record->video_y, record->video_zoom);
			}
			else
			{
				record->video_zoom -= 0.1;
				record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
			}
			result = 1;
			break;
		case RIGHT:
			if(!ctrl_down()) 
			{ 
				record->record_gui->set_translation(++(record->video_x), record->video_y, record->video_zoom);
			}
			else
			{
				record->video_zoom += 0.1;
				record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
			}
			result = 1;
			break;
		case UP:
			if(!ctrl_down()) 
			{ 
				record->record_gui->set_translation(record->video_x, --(record->video_y), record->video_zoom);
			}
			else
			{
				record->video_zoom -= 0.1;
				record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
			}
			result = 1;
			break;
		case DOWN:
			if(!ctrl_down()) 
			{ 
				record->record_gui->set_translation(record->video_x, ++(record->video_y), record->video_zoom);
			}
			else
			{
				record->video_zoom += 0.1;
				record->record_gui->set_translation(record->video_x, record->video_y, record->video_zoom);
			}
			result = 1;
			break;
		case 'w':
			close_event();
			break;

		default:
			if(canvas) result = canvas->keypress_event(this);
//			if(!result && avc1394_transport)
//				result = avc1394_transport->keypress_event(get_keypress());
			break;
	}

	return result;
}


int RecordMonitorGUI::translation_event()
{
//printf("MWindowGUI::translation_event 1 %d %d\n", get_x(), get_y());
	mwindow->session->rmonitor_x = get_x();
	mwindow->session->rmonitor_y = get_y();
	return 0;
}

int RecordMonitorGUI::resize_event(int w, int h)
{
    VideoInConfig *in_config = MWindow::preferences->vconfig_in;
	int driver = in_config->driver;
	int do_channel = (driver == VIDEO4LINUX ||
			driver == CAPTURE_BUZ ||
			driver == VIDEO4LINUX2
// ||
//			driver == VIDEO4LINUX2MJPG ||
//			driver == VIDEO4LINUX2JPEG ||
//			driver == CAPTURE_JPEG_WEBCAM ||
//			driver == CAPTURE_YUYV_WEBCAM ||
//			driver == CAPTURE_MPEG
        );
	int do_scopes = do_channel || driver == SCREENCAPTURE;
	int do_interlace = (driver == CAPTURE_BUZ ||
		(driver == VIDEO4LINUX2 &&
            in_config->v4l2_format == CAPTURE_MJPG));
//	int do_avc = avc1394_transport ? 1 : 0;

	mwindow->session->rmonitor_x = get_x();
	mwindow->session->rmonitor_y = get_y();
	mwindow->session->rmonitor_w = w;
	mwindow->session->rmonitor_h = h;

	mwindow->theme->get_rmonitor_sizes(record->default_asset->audio_data, 
		record->default_asset->video_data,
		do_channel || do_scopes,
		do_interlace,
//		do_avc,
        0,
		record->default_asset->channels);
	mwindow->theme->draw_rmonitor_bg(this);


// 	record_transport->reposition_window(mwindow->theme->rmonitor_tx_x,
// 		mwindow->theme->rmonitor_tx_y);
// 	if(avc1394_transport)
// 	{
// 		avc1394_transport->reposition_window(mwindow->theme->rmonitor_tx_x,
// 			mwindow->theme->rmonitor_tx_y);
// 	}
	
	if(channel_picker) 
	{
		channel_picker->reposition();
	}
	
	if(reverse_interlace) 
	{
		reverse_interlace->reposition_window(reverse_interlace->get_x(),
			reverse_interlace->get_y());
	}

// 	if(cursor_toggle)
// 	{
// 		cursor_toggle->reposition_window(cursor_toggle->get_x(),
// 			cursor_toggle->get_y());
// 		big_cursor_toggle->reposition_window(big_cursor_toggle->get_x(),
// 			big_cursor_toggle->get_y());
// 	}
	
	if(canvas && record->default_asset->video_data)
	{
		canvas->reposition_window(0,
			mwindow->theme->rmonitor_canvas_x, 
			mwindow->theme->rmonitor_canvas_y, 
			mwindow->theme->rmonitor_canvas_w, 
			mwindow->theme->rmonitor_canvas_h);
	}

	if(record->default_asset->audio_data)
	{
		meters->reposition_window(mwindow->theme->rmonitor_meter_x, 
			mwindow->theme->rmonitor_meter_y, 
			record->default_asset->video_data ? -1 : mwindow->theme->rmonitor_meter_w,
			mwindow->theme->rmonitor_meter_h);
	}

	set_title();
	BC_WindowBase::resize_event(w, h);
	flash();
	return 1;
}

int RecordMonitorGUI::set_title()
{
return 0;
	char string[1024];
	int scale;

	scale = (int)(thread->get_scale(thread->record->video_window_w) * 100 + 0.5);

	sprintf(string, PROGRAM_NAME ": Video in %d%%", scale);
	BC_Window::set_title(string);
	return 0;
}

int RecordMonitorGUI::close_event()
{
	thread->record->monitor_video = 0;
	thread->record->monitor_audio = 0;
	thread->record->video_window_open = 0;
	unlock_window();

	record->record_gui->lock_window("RecordMonitorGUI::close_event");
	if(record->record_gui->monitor_video) record->record_gui->monitor_video->update(0);
	if(record->record_gui->monitor_audio) record->record_gui->monitor_audio->update(0);
	record->record_gui->flush();
	record->record_gui->unlock_window();


	lock_window("RecordMonitorGUI::close_event");
	hide_window();
	return 0;
}

// int RecordMonitorGUI::create_bitmap()
// {
// 	if(bitmap && 
// 		(bitmap->get_w() != get_w() || 
// 			bitmap->get_h() != thread->get_canvas_height()))
// 	{
// 		delete bitmap;
// 		bitmap = 0;
// 	}
// 
// 	if(!bitmap && canvas)
// 	{
// //		bitmap = canvas->new_bitmap(get_w(), thread->get_canvas_height());
// 	}
// 	return 0;
// }

// DoCursor::DoCursor(Record *record, int x, int y)
//  : BC_CheckBox(x, y, record->do_cursor, _("Record cursor"))
// {
// 	this->record = record;
// }
// 
// DoCursor::~DoCursor()
// {
// }
// 
// int DoCursor::handle_event()
// {
// 	record->do_cursor = get_value();
// 	return 0;
// }
// 
// 
// DoBigCursor::DoBigCursor(Record *record, int x, int y)
//  : BC_CheckBox(x, y, record->do_big_cursor, _("Big cursor"))
// {
// 	this->record = record;
// }
// 
// DoBigCursor::~DoBigCursor()
// {
// }
// 
// int DoBigCursor::handle_event()
// {
// 	record->do_big_cursor = get_value();
// 	return 0;
// }
// 





ReverseInterlace::ReverseInterlace(Record *record, int x, int y)
 : BC_CheckBox(x, y, record->reverse_interlace, _("Swap fields"))
{
	this->record = record;
}

ReverseInterlace::~ReverseInterlace()
{
}

int ReverseInterlace::handle_event()
{
	record->reverse_interlace = get_value();
	return 0;
}

RecordMonitorCanvas::RecordMonitorCanvas(MWindow *mwindow, 
	RecordMonitorGUI *window, 
	Record *record, 
	int x, 
	int y, 
	int w, 
	int h)
 : Canvas(mwindow,
 	window, 
 	x, 
	y, 
	w, 
	h, 
	record->default_asset->width,
	record->default_asset->height,
	0, // use_scrollbars
	0, // use_cwindow
	1) // use_rwindow
{
	this->window = window;
	this->mwindow = mwindow;
	this->record = record;
// printf("RecordMonitorCanvas::RecordMonitorCanvas 1 %d %d %d %d\n", 
// x, y, w, h);
//printf("RecordMonitorCanvas::RecordMonitorCanvas 2\n");
}

RecordMonitorCanvas::~RecordMonitorCanvas()
{
}

int RecordMonitorCanvas::get_output_w()
{
	return record->default_asset->width;
}

int RecordMonitorCanvas::get_output_h()
{
	return record->default_asset->height;
}


int RecordMonitorCanvas::button_press_event()
{

	if(Canvas::button_press_event()) return 1;
	
	if(MWindow::preferences->vconfig_in->driver == SCREENCAPTURE)
	{
		window->current_operation = MONITOR_TRANSLATE;
		window->translate_x_origin = record->video_x;
		window->translate_y_origin = record->video_y;
		window->cursor_x_origin = get_cursor_x();
		window->cursor_y_origin = get_cursor_y();
	}

	return 0;
}

void RecordMonitorCanvas::zoom_resize_window(float percentage)
{
	int canvas_w, canvas_h;
//     float aspect_ratio = mwindow->edl->get_aspect_ratio();
// // compute auto aspect ratio from the recording size
//     if(mwindow->edl->session->auto_aspect)
//     {
//         aspect_ratio = (float)MWindow::preferences->vconfig_in->w / 
//             MWindow::preferences->vconfig_in->h;
//     }

// always use square pixels.  See also Canvas::get_transfers
    float aspect_ratio = (float)record->default_asset->width / 
        record->default_asset->height;

	calculate_sizes(aspect_ratio, 
		record->default_asset->width, 
		record->default_asset->height, 
		percentage,
		canvas_w,
		canvas_h);
	int new_w, new_h;
	new_w = canvas_w + (window->get_w() - mwindow->theme->rmonitor_canvas_w);
	new_h = canvas_h + (window->get_h() - mwindow->theme->rmonitor_canvas_h);
// printf("RecordMonitorCanvas::zoom_resize_window %d %d %d\n",
// __LINE__,
// canvas_w,
// canvas_h);
	window->resize_window(new_w, new_h);
	window->resize_event(new_w, new_h);
}

int RecordMonitorCanvas::get_fullscreen()
{
	return mwindow->session->rwindow_fullscreen;
}

void RecordMonitorCanvas::set_fullscreen(int value)
{
	mwindow->session->rwindow_fullscreen = value;
}


int RecordMonitorCanvas::button_release_event()
{
	window->current_operation = MONITOR_NONE;
	return 0;
}

int RecordMonitorCanvas::cursor_motion_event()
{
//SET_TRACE
	if(window->current_operation == MONITOR_TRANSLATE)
	{
//SET_TRACE
		record->set_translation(
			get_cursor_x() - window->cursor_x_origin + window->translate_x_origin,
			get_cursor_y() - window->cursor_y_origin + window->translate_y_origin);
//SET_TRACE
	}

	return 0;
}

int RecordMonitorCanvas::cursor_enter_event()
{
	if(MWindow::preferences->vconfig_in->driver == SCREENCAPTURE)
		set_cursor(MOVE_CURSOR);
	return 0;
}

void RecordMonitorCanvas::reset_translation()
{
	record->set_translation(0, 0);
}

void RecordMonitorCanvas::preset_translation(int position)
{
    switch(position)
    {
        case TOP_LEFT:
	        record->set_translation(0, 0);
            break;
        case TOP_RIGHT:
	        record->set_translation(window->get_root_w(1) - record->default_asset->width, 
                0);
            break;
        case BOTTOM_LEFT:
	        record->set_translation(0, 
                window->get_root_h(0) - record->default_asset->height);
            break;
        case BOTTOM_RIGHT:
	        record->set_translation(window->get_root_w(1) - record->default_asset->width, 
                window->get_root_h(0) - record->default_asset->height);
            break;
    }
}

int RecordMonitorCanvas::keypress_event()
{
	int result = 0;
	if(get_canvas())
	{
		switch(get_canvas()->get_keypress())
		{
			case LEFT:
				record->set_translation(--record->video_x, record->video_y);
				break;
			case RIGHT:
				record->set_translation(++record->video_x, record->video_y);
				break;
			case UP:
				record->set_translation(record->video_x, --record->video_y);
				break;
			case DOWN:
				record->set_translation(record->video_x, ++record->video_y);
				break;
		}
	}
	
	return result;
}


RecordMonitorFullsize::RecordMonitorFullsize(MWindow *mwindow, 
	RecordMonitorGUI *window)
 : BC_MenuItem(_("Zoom 100%"))
{
	this->mwindow = mwindow;
	this->window = window;
}
int RecordMonitorFullsize::handle_event()
{
	return 1;
}








// ================================== slippery playback ============================


RecordMonitorThread::RecordMonitorThread(MWindow *mwindow, 
	Record *record, 
	RecordMonitor *record_monitor)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->record_monitor = record_monitor;
	this->record = record;
	reset_parameters();
	output_lock = new Condition(1, "RecordMonitor::output_lock");
	input_lock = new Condition(1, "RecordMonitor::input_lock");
}


void RecordMonitorThread::reset_parameters()
{
	input_frame = 0;
	output_frame = 0;
	shared_data = 0;
	jpeg_engine = 0;
	dv_engine = 0;
	ready = 0;
}


RecordMonitorThread::~RecordMonitorThread()
{
	if(input_frame && !shared_data) delete input_frame;
	delete output_lock;
	delete input_lock;
}

void RecordMonitorThread::init_output_format()
{

//printf("RecordMonitorThread::init_output_format 1\n");
	switch(MWindow::preferences->vconfig_in->driver)
	{
		case SCREENCAPTURE:
			output_colormodel = record->vdevice->get_best_colormodel(record->default_asset);
			break;


		case CAPTURE_BUZ:
//		case VIDEO4LINUX2MJPG:
			jpeg_engine = new RecVideoMJPGThread(record, this, 2);
			jpeg_engine->start_rendering();
			output_colormodel = BC_YUV422P;
			break;

		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			dv_engine = new RecVideoDVThread(record, this);
			dv_engine->start_rendering();
			output_colormodel = BC_YUV422P;
			break;

//		case CAPTURE_JPEG_WEBCAM:
//		case VIDEO4LINUX2JPEG:
//			jpeg_engine = new RecVideoMJPGThread(record, this, 1);
//			jpeg_engine->start_rendering();
//			output_colormodel = BC_YUV422P;
//			break;

//		case CAPTURE_YUYV_WEBCAM:
//			output_colormodel = BC_YUV422;
//			break;

		case VIDEO4LINUX:
//		case VIDEO4LINUX2:
			output_colormodel = record->vdevice->get_best_colormodel(record->default_asset);
//printf("RecordMonitorThread::init_output_format 2 %d\n", output_colormodel);
			break;

        case VIDEO4LINUX2:
            switch(MWindow::preferences->vconfig_in->v4l2_format)
            {
            case CAPTURE_JPEG:
            case CAPTURE_JPEG_NOHEAD:
		    case CAPTURE_MJPG_1FIELD:
			    jpeg_engine = new RecVideoMJPGThread(record, this, 1);
			    jpeg_engine->start_rendering();
			    output_colormodel = BC_YUV422P;
                break;
            case CAPTURE_YUYV:
			    output_colormodel = BC_YUV422;
                break;
		    case CAPTURE_MJPG:
			    jpeg_engine = new RecVideoMJPGThread(record, this, 2);
			    jpeg_engine->start_rendering();
			    output_colormodel = BC_YUV422P;
			    break;
            default:
			    output_colormodel = record->vdevice->get_best_colormodel(record->default_asset);
                break;
            }
	}
}

int RecordMonitorThread::start_playback()
{
	ready = 1;
	done = 0;
	output_lock->lock("RecordMonitorThread::start_playback");
	Thread::start();
	return 0;
}

int RecordMonitorThread::stop_playback()
{
	done = 1;
	output_lock->unlock();
	Thread::join();
//printf("RecordMonitorThread::stop_playback 1\n");

// 	switch(MWindow::preferences->vconfig_in->driver)
// 	{
// 		case CAPTURE_BUZ:
// 		case VIDEO4LINUX2JPEG:
// 		case VIDEO4LINUX2MJPG:
			if(jpeg_engine) 
			{
				jpeg_engine->stop_rendering();
				delete jpeg_engine;
			}
// 			break;
// 
// 		case CAPTURE_FIREWIRE:
// 		case CAPTURE_IEC61883:
			if(dv_engine)
			{
				dv_engine->stop_rendering();
				delete dv_engine;
			}
// 			break;
// 	}
//printf("RecordMonitorThread::stop_playback 4\n");

	return 0;
}

int RecordMonitorThread::write_frame(VFrame *new_frame)
{
	if(ready)
	{
		ready = 0;
		shared_data = (new_frame->get_color_model() != BC_COMPRESSED);


// Need to wait until after Record creates the input device before starting monitor
// because the input device deterimes the output format.
// First time
		if(!output_frame) init_output_format();
		if(!shared_data)
		{
			if(!input_frame) input_frame = new VFrame;
			input_frame->allocate_compressed_data(new_frame->get_compressed_size());
			memcpy(input_frame->get_data(), 
				new_frame->get_data(), 
				new_frame->get_compressed_size());
			input_frame->set_compressed_size(new_frame->get_compressed_size());
			input_frame->set_field2_offset(new_frame->get_field2_offset());
		}
		else
		{
			input_lock->lock("RecordMonitorThread::write_frame");
			input_frame = new_frame;
		}
		output_lock->unlock();
	}
	return 0;
}

int RecordMonitorThread::render_jpeg()
{
//printf("RecordMonitorThread::render_jpeg 1\n");
	jpeg_engine->render_frame(input_frame, input_frame->get_compressed_size());
//printf("RecordMonitorThread::render_jpeg 2\n");
	return 0;
}

int RecordMonitorThread::render_dv()
{
	dv_engine->render_frame(input_frame, input_frame->get_compressed_size());
	return 0;
}

void RecordMonitorThread::render_uncompressed()
{
// printf("RecordMonitorThread::render_uncompressed %d %d %d %d %d\n",
// __LINE__,
// output_frame->get_w(),
// output_frame->get_h(),
// input_frame->get_w(),
// input_frame->get_h());
// printf("RecordMonitorThread::render_uncompressed %d %d %d %d %p %p %p\n", 
// __LINE__, 
// input_frame->get_color_model(),
// output_frame->get_color_model(),
// (int)input_frame->get_data_size(),
// input_frame->get_y(),
// input_frame->get_u(),
// input_frame->get_v());
// output_frame->dump();
// input_frame->dump();
	output_frame->copy_from(input_frame);
}

void RecordMonitorThread::show_output_frame()
{
	record_monitor->device->write_buffer(output_frame, record->edl);
}


void RecordMonitorThread::unlock_input()
{
	if(shared_data) input_lock->unlock();
}

int RecordMonitorThread::render_frame()
{
    if(jpeg_engine) 
        render_jpeg();
    else
    if(dv_engine) 
        render_dv();
    else
        render_uncompressed();
// 	switch(MWindow::preferences->vconfig_in->driver)
// 	{
// 		case CAPTURE_BUZ:
// 		case VIDEO4LINUX2MJPG:
// 		case VIDEO4LINUX2JPEG:
// 		case CAPTURE_JPEG_WEBCAM:
// 			render_jpeg();
// 			break;
// 
// 		case CAPTURE_FIREWIRE:
// 		case CAPTURE_IEC61883:
// 			render_dv();
// 			break;
// 
// 		default:
// 			render_uncompressed();
// 			break;
// 	}

	return 0;
}

void RecordMonitorThread::new_output_frame()
{
	record_monitor->device->new_output_buffer(&output_frame, 
		output_colormodel,
		record->edl);
}

void RecordMonitorThread::run()
{
//printf("RecordMonitorThread::run 1 %d\n", getpid());
	while(!done)
	{
// Wait for next frame
//SET_TRACE
		output_lock->lock("RecordMonitorThread::run");

		if(done)
		{
			unlock_input();
			return;
		}

//PRINT_TRACE
		new_output_frame();

//PRINT_TRACE
		render_frame();

		record_monitor->scope_thread->process(output_frame);

//PRINT_TRACE
		show_output_frame();

//PRINT_TRACE
		unlock_input();
// Get next frame
		ready = 1;
	}
}



RecVideoMJPGThread::RecVideoMJPGThread(Record *record, 
	RecordMonitorThread *thread,
	int fields)
{
	this->record = record;
	this->thread = thread;
	mjpeg = 0;
	this->fields = fields;
}

RecVideoMJPGThread::~RecVideoMJPGThread()
{
}

int RecVideoMJPGThread::start_rendering()
{
	mjpeg = mjpeg_new(record->default_asset->width, 
		record->default_asset->height, 
		fields);
//printf("RecVideoMJPGThread::start_rendering 1 %p\n", mjpeg);
	return 0;
}

int RecVideoMJPGThread::stop_rendering()
{
//printf("RecVideoMJPGThread::stop_rendering 1 %p\n", mjpeg);
	if(mjpeg) mjpeg_delete(mjpeg);
//printf("RecVideoMJPGThread::stop_rendering 2\n");
	return 0;
}

int RecVideoMJPGThread::render_frame(VFrame *frame, long size)
{
// printf("RecVideoMJPGThread::render_frame %d field2=%d\n", 
// __LINE__,
// frame->get_field2_offset());
// printf("RecVideoMJPGThread::render_frame %d %02x%02x%02x%02x\n", 
// __LINE__, 
// frame->get_data()[0], 
// frame->get_data()[1], 
// frame->get_data()[2], 
// frame->get_data()[3]);
// printf("RecVideoMJPGThread::render_frame %d %02x%02x %02x%02x\n", 
// frame->get_field2_offset(), 
// frame->get_data()[0], 
// frame->get_data()[1], 
// frame->get_data()[frame->get_field2_offset()], 
// frame->get_data()[frame->get_field2_offset() + 1]);
//frame->set_field2_offset(0);
//printf("RecVideoMJPGThread::render_frame %d\n", __LINE__);
	mjpeg_decompress(mjpeg, 
		frame->get_data(), 
		frame->get_compressed_size(), 
		frame->get_field2_offset(), 
		thread->output_frame->get_rows(), 
		thread->output_frame->get_y(), 
		thread->output_frame->get_u(), 
		thread->output_frame->get_v(),
		thread->output_frame->get_color_model(),
		record->mwindow->preferences->processors);
//printf("RecVideoMJPGThread::render_frame %d\n", __LINE__);
	return 0;
}




RecVideoDVThread::RecVideoDVThread(Record *record, RecordMonitorThread *thread)
{
	this->record = record;
	this->thread = thread;
	dv = 0;
}

RecVideoDVThread::~RecVideoDVThread()
{
}


int RecVideoDVThread::start_rendering()
{
	dv = dv_new();
	return 0;
}

int RecVideoDVThread::stop_rendering()
{
	if(dv) dv_delete(((dv_t*)dv));
	return 0;
}

int RecVideoDVThread::render_frame(VFrame *frame, long size)
{
	unsigned char *yuv_planes[3];
	yuv_planes[0] = thread->output_frame->get_y();
	yuv_planes[1] = thread->output_frame->get_u();
	yuv_planes[2] = thread->output_frame->get_v();
	dv_read_video(((dv_t*)dv), 
		yuv_planes, 
		frame->get_data(), 
		frame->get_compressed_size(),
		thread->output_frame->get_color_model());

	return 0;
}












