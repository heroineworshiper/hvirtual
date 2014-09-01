#include "asset.h"
#include "colormodels.h"
#include "errorbox.h"
#include "file.h"
#include "keys.h"
#include "mwindow.h"
#include "mainmenu.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "theme.h"
#include "transportque.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>

MWindowGUI::MWindowGUI(MWindow *mwindow, int x, int y, int w, int h)
 : BC_Window("XMovie", 
 	x, 
	y, 
	w, 
	h, 
	32, 
	32, 
	1, 
	0, 
	1)
{
	this->mwindow = mwindow;
	reset();
}

MWindowGUI::~MWindowGUI()
{
	delete_output_frame();
	if(canvas) delete canvas;
}

void MWindowGUI::reset()
{

	window_resized = 0;
	primary_surface = 0;
	output_frame = 0;
	canvas = 0;
	dvs_frame = 0;
#ifdef HAVE_DVS
	dvs_output = 0;
#endif
	active_vdevice = -1;
}

void MWindowGUI::create_canvas()
{
	int video_on = 0;
//printf("MWindowGUI::create_canvas 1\n");
	if(canvas) 
	{
		video_on = canvas->video_is_on();
		delete canvas;
		canvas = 0;
	}

	if(mwindow->fullscreen)
	{
		if (mwindow->video_device == VDEVICE_XF86VM)
		{
// Default height and width, if no video is loaded.
			int width = 640, height = 480;

			if (mwindow->asset) 
			{
				width = mwindow->asset->width;
				height = mwindow->asset->height;
			}

			canvas = new MainCanvasFullScreen(mwindow, width, height, 1);
		}
		else 
		{
			canvas = new MainCanvasFullScreen(mwindow, 
				get_root_w(0), 
				get_root_h(0),
			    0);
		}
	}
	else
	{
		add_tool(canvas = new MainCanvasSubWindow(mwindow, 
			mwindow->theme->canvas_x, 
			mwindow->theme->canvas_y, 
			mwindow->theme->canvas_w, 
			mwindow->theme->canvas_h));
	}



// XVideo doesn't draw on MainCanvasSubWindow without a resize event but
// does draw on MainCanvasPopup on the first try.

	if(video_on)
	{
		canvas->start_video();
	}
	else
	{
		if(mwindow->asset && mwindow->asset->video_data)
		{
			double percentage, seconds;
			mwindow->engine->current_position(&percentage, &seconds);
			mwindow->engine->que->send_command(CURRENT_FRAME, mwindow->current_position);
		}
		else
		if(!mwindow->fullscreen)
			mwindow->theme->draw_canvas_bg(canvas);
	}

	flush();
	window_resized = 1;
}

int MWindowGUI::create_objects()
{
	int w = get_w(), h = get_h();

	set_icon(mwindow->theme->icon);
	add_tool(menu = new MainMenu(mwindow, get_w()));
	mwindow->theme->update_positions(mwindow, this);
	menu->create_objects();
	create_canvas();

	mwindow->theme->draw_mwindow_bg(mwindow, this);
	add_tool(time_title = new BC_Title(mwindow->theme->time_x, 
		mwindow->theme->time_y, 
		"", 
		MEDIUM_7SEGMENT, 
		BLACK,
		0,
		mwindow->theme->time_w));
	add_tool(playbutton = new PlayButton(mwindow, 
		mwindow->theme->play_x, 
		mwindow->theme->play_y));
	add_tool(backbutton = new FrameBackButton(mwindow, 
		mwindow->theme->frameback_x, 
		mwindow->theme->frameback_y));
	add_tool(forwardbutton = new FrameForwardButton(mwindow, 
		mwindow->theme->framefwd_x, 
		mwindow->theme->framefwd_y));
	add_tool(scrollbar = new MainScrollbar(mwindow, 
		mwindow->theme->scroll_x, 
		mwindow->theme->scroll_y, 
		mwindow->theme->scroll_w));
	return 0;
}

int MWindowGUI::resize_scrollbar()
{
	scrollbar->update(
		scrollbar->get_w(), 
		scrollbar->get_value(), 
		0, 
		SCROLLBAR_LENGTH);
	return 0;
}


int MWindowGUI::resize_widgets()
{
	mwindow->theme->draw_mwindow_bg(mwindow, this);
	if(!mwindow->fullscreen)
	{
// XVideo bug requires the pixmap to be deleted, recreated and flashed
// before displaying a frame
		canvas->reposition_window(mwindow->theme->canvas_x, 
			mwindow->theme->canvas_y, 
			mwindow->theme->canvas_w, 
			mwindow->theme->canvas_h);
		mwindow->theme->draw_canvas_bg(canvas);
	}
	time_title->reposition_window(mwindow->theme->time_x, mwindow->theme->time_y);
	playbutton->reposition_window(mwindow->theme->play_x, mwindow->theme->play_y);
	backbutton->reposition_window(mwindow->theme->frameback_x, mwindow->theme->frameback_y);
	forwardbutton->reposition_window(mwindow->theme->framefwd_x, mwindow->theme->framefwd_y);
	scrollbar->reposition_window(mwindow->theme->scroll_x, 
		mwindow->theme->scroll_y, 
		mwindow->theme->scroll_w, 
		scrollbar->get_h());
	resize_scrollbar();
	return 0;
}

int MWindowGUI::translation_event()
{
	mwindow->mwindow_x = get_x();
	mwindow->mwindow_y = get_y();
	return 0;
}

int MWindowGUI::resize_event(int w, int h)
{
//printf("MWindowGUI::resize_event 1\n");
	mwindow->mwindow_w = w;
	mwindow->mwindow_h = h;
	window_resized = 1;
	mwindow->theme->update_positions(mwindow, this);
	resize_widgets();

	if(!canvas->video_is_on())
	{
		double percentage, seconds;
		mwindow->engine->current_position(&percentage, &seconds);
		mwindow->engine->que->send_command(CURRENT_FRAME, mwindow->current_position);
	}

	return 0;
}

int MWindowGUI::start_video()
{
	delete_output_frame();
	active_vdevice = mwindow->video_device;
	switch(active_vdevice)
	{
		case VDEVICE_X11:
		case VDEVICE_XF86VM:
			start_x11();
			break;
		case VDEVICE_DVS:
			start_dvs();
			break;
	}
	return 0;
}

int MWindowGUI::stop_video()
{
	switch(active_vdevice)
	{
		case VDEVICE_X11:
		case VDEVICE_XF86VM:
			stop_x11();
			break;
		
		case VDEVICE_DVS:
			stop_dvs();
			break;
	}

	active_vdevice = -1;
	return 0;
}

void MWindowGUI::start_dvs()
{
#ifdef HAVE_DVS
	dvs_frame = new VFrame(0, mwindow->asset->width, mwindow->asset->height, BC_YUV422);
//printf("MWindowGUI::start_dvs 1\n");
	if(!dvs_output) dvs_output = sv_open("");
//printf("MWindowGUI::start_dvs 1 %p\n", dvs_output);
	
  	int res = sv_fifo_init(dvs_output, &pfifo, FALSE, TRUE, TRUE, FALSE, 0);
//printf("MWindowGUI::start_dvs 1\n");
	if(res != SV_OK)
	{
    	printf("sv_fifo_init error : %d %s.\n", res, sv_geterrortext(res));
    	return;
	}
	res = sv_fifo_start(dvs_output, pfifo);
	if(res != SV_OK)
	{
    	printf("sv_fifo_start error : %d %s.\n", res, sv_geterrortext(res));
    	return;
	}
//printf("MWindowGUI::start_dvs 2\n");
#endif
}

void MWindowGUI::start_x11()
{
	canvas->start_video();
}

void MWindowGUI::stop_dvs()
{
#ifdef HAVE_DVS
//printf("MWindowGUI::stop_dvs 1\n");
    sv_fifo_stop(dvs_output, pfifo);
	sv_fifo_reset(dvs_output, pfifo);
//	sv_close(dvs_output);
#endif
}

void MWindowGUI::stop_x11()
{
	canvas->stop_video();
// Convert hardware accelerated to flat bitmap for persistent drawing
	int w, h;
	bitmap_dimensions(w, h);
	if(primary_surface)
	{
// Convert existing output frame to software
		if(primary_surface->hardware_scaling())
		{
			BC_Bitmap *new_surface = canvas->new_bitmap(w, h);
			primary_surface->rewind_ring();
			cmodel_transfer(new_surface->get_row_pointers(), 
				primary_surface->get_row_pointers(),
				new_surface->get_y_plane(),
				new_surface->get_u_plane(),
				new_surface->get_v_plane(),
				primary_surface->get_y_plane(),
				primary_surface->get_u_plane(),
				primary_surface->get_v_plane(),
				0, 
				0, 
				primary_surface->get_w(), 
				primary_surface->get_h(),
				0, 
				0, 
				w, 
				h,
				primary_surface->get_color_model(), 
				new_surface->get_color_model(),
				0,
				primary_surface->get_w(),
				new_surface->get_w());
			delete_output_frame();
			primary_surface = new_surface;
		}
		else
			primary_surface->rewind_ring();

		canvas->draw_bitmap(primary_surface, 
			0, 
			canvas->get_w() / 2 - w / 2,
			canvas->get_h() / 2 - h / 2,
			w,
			h);
		canvas->flash();
		canvas->flush();
	}
}


int MWindowGUI::close_event()
{
	mwindow->quit();
// 	mwindow->gui->unlock_window();
// 	mwindow->engine->que->send_command(STOP_PLAYBACK, 1);
// 	mwindow->exit_cleanly();
// 	mwindow->gui->lock_window("Quit::handle_event");
// 	mwindow->gui->set_done(0);
	return 0;
}

int MWindowGUI::update_position(double percentage, double seconds, int use_scrollbar)
{
	char string[1024];
// printf("MWindowGUI::update_position 1 %f %d\n", 
// percentage, 
// (long)(percentage * SCROLLBAR_LENGTH + 0.5));
	if(use_scrollbar)
		scrollbar->update((long)(percentage * SCROLLBAR_LENGTH + 0.5));

	Units::totext(string, seconds, TIME_HMS2);
	time_title->update(string);
	return 0;
}


// Get dimensions of drawing surface given the canvas dimensions
void MWindowGUI::bitmap_dimensions(int &w, int &h)
{
	float aspect_ratio;
	if(mwindow->crop_letterbox)
		aspect_ratio = mwindow->letter_w /  mwindow->letter_h;
	else
		aspect_ratio = mwindow->get_aspect_ratio();

	if((float)canvas->get_w() / canvas->get_h() > aspect_ratio)
	{
		w = (int)((float)canvas->get_h() * aspect_ratio + 0.5);
		h = canvas->get_h();
	}
	else
	{
		w = canvas->get_w();
		h = (int)((float)canvas->get_w() / aspect_ratio + 0.5);
	}
}

void MWindowGUI::delete_output_frame()
{
//printf("MWindowGUI::delete_output_frame\n");
	if(primary_surface)
	{
		delete primary_surface;
		primary_surface = 0;
	}

	if(output_frame)
	{
		delete output_frame;
		output_frame = 0;
	}
	
	if(dvs_frame)
	{
		delete dvs_frame;
		dvs_frame = 0;
	}
// 
// 	if(canvas)
// 	{
// 		delete canvas;
// 		canvas = 0;
// 	}
}

VFrame* MWindowGUI::get_output_frame()
{
	if(mwindow->video_file)
	{
		switch(active_vdevice)
		{
			case -1:
			case VDEVICE_X11:
			case VDEVICE_XF86VM:
				return get_output_frame_x11();
				break;
			
			case VDEVICE_DVS:
				return get_output_frame_dvs();
				break;
		}
	}
	else
		return 0;
}

VFrame* MWindowGUI::get_output_frame_x11()
{
//printf("MWindowGUI::get_output_frame 1 %d %d\n", primary_surface, window_resized);
	if(primary_surface && window_resized)
	{
		delete_output_frame();
	}

	window_resized = 0;
	if(!primary_surface)
	{
		int in_y1, in_y2;
// X server forced alignment
		int aligned_width = (mwindow->asset->width + 0x4) & 0xfffffffb;

		mwindow->get_cropping(in_y1, in_y2);
// Try hardware acceleration.  Take the highest sampling available.
		if(canvas->video_is_on() &&
			mwindow->video_file->colormodel_supported(BC_YUV422) &&
			canvas->accel_available(BC_YUV422, 0))
		{
			primary_surface = canvas->new_bitmap(aligned_width,
				in_y2 - in_y1,
				BC_YUV422);
		}
		else
		if(canvas->video_is_on() &&
			mwindow->video_file->colormodel_supported(BC_YUV420P) &&
			canvas->accel_available(BC_YUV420P, 0))
		{
			primary_surface = canvas->new_bitmap(aligned_width,
				in_y2 - in_y1,
				BC_YUV420P);
//printf("MWindowGUI::get_output_frame 2 %d %d %p\n", mwindow->asset->width, in_y2 - in_y1, canvas);
		}
		else
		{
//printf("MWindowGUI::get_output_frame 1\n");
			int w, h;
			bitmap_dimensions(w, h);
			primary_surface = canvas->new_bitmap(w, h);
		}
	}

	if(!output_frame)
	{
//printf("MWindowGUI::get_output_frame 2\n");
		output_frame = new VFrame(primary_surface->get_data(),
			primary_surface->get_y_offset(),
			primary_surface->get_u_offset(),
			primary_surface->get_v_offset(),
			primary_surface->get_w(),
			primary_surface->get_h(),
			primary_surface->get_color_model());
	}
	else
// Update ring buffer
	{
		output_frame->set_memory(primary_surface->get_data(),
				primary_surface->get_y_offset(),
				primary_surface->get_u_offset(),
				primary_surface->get_v_offset());
	}
	
//printf("MWindowGUI::get_output_frame %d\n", output_frame->get_color_model());
	return output_frame;
}

VFrame* MWindowGUI::get_output_frame_dvs()
{
	return dvs_frame;
}

void MWindowGUI::write_output()
{
	switch(active_vdevice)
	{
		case -1:
		case VDEVICE_X11:
		case VDEVICE_XF86VM:
			write_x11();
			break;
		case VDEVICE_DVS:
			write_dvs();
			break;
	}
}

void MWindowGUI::write_x11()
{
	int w, h;
	bitmap_dimensions(w, h);

//printf("MWindowGUI::write_output %d %d %d %d\n", w, h, canvas->get_w(), canvas->get_h());
//for(int i = 0; i < 1000; i++) primary_surface->get_data()[i] = 255;
	canvas->draw_bitmap(primary_surface, 
		0, 
		canvas->get_w() / 2 - w / 2,
		canvas->get_h() / 2 - h / 2,
		w,
		h);
	if(!canvas->video_is_on()) canvas->flash();
//else
//sleep(10);
	canvas->flush();
}

void MWindowGUI::decode_sign_change(int w, int h, unsigned char **row_pointers)
{
	int y, x;
	for(y = 0; y < h; y++)
	{
		unsigned char *in_row = row_pointers[y];
		for(x = 0; x < w * 2; )
		{
			in_row[1] -= 128;
			in_row[3] -= 128;
			x += 4;
			in_row += 4;
		}
	}
}

void MWindowGUI::write_dvs()
{
#ifdef HAVE_DVS
	int res = sv_fifo_getbuffer(dvs_output, pfifo, &pbuffer, NULL, SV_FIFO_FLAG_VIDEOONLY);
	if(res != SV_OK)
	{
      	printf("sv_fifo_getbuffer error : %d %s.\n", res, sv_geterrortext(res));
	}

	
	decode_sign_change(mwindow->asset->width, mwindow->asset->height, dvs_frame->get_rows());
	pbuffer->dma.addr = (char*)dvs_frame->get_data();
	pbuffer->dma.size = dvs_frame->get_data_size();

	res = sv_fifo_putbuffer(dvs_output, pfifo, pbuffer, NULL);
	if(res != SV_OK)
	{
      	printf("sv_fifo_putbuffer error : %d %s\n", res, sv_geterrortext(res));
	}
#endif // HAVE_DVS
}

char* MWindowGUI::vdevice_to_text(int number)
{
	switch(number)
	{
		case VDEVICE_X11:    return "X Windows";      break;
		case VDEVICE_DVS:    return "DVS SDStation";  break;
		case VDEVICE_XF86VM: return "XF86 Vid. Mode"; break;
	}
	return "";
}

int MWindowGUI::text_to_vdevice(char *text)
{
	for(int i = 0; i < TOTAL_VDEVICES; i++)
	{
		if(!strcasecmp(vdevice_to_text(i), text)) return i;
	}
	return 0;
}






MainScrollbar::MainScrollbar(MWindow *mwindow, int x, int y, int w)
 : BC_PercentageSlider(x, 
 	y, 
	0, 
	w, 
	w, 
	0, 
	SCROLLBAR_LENGTH, 
	0, 
	0)
{
	this->mwindow = mwindow;
	set_tooltip("Seek to a position");
}

MainScrollbar::~MainScrollbar()
{
}

int MainScrollbar::keypress_event()
{
	return handle_event();
}

int MainScrollbar::handle_event()
{
	int result = 0;
	double percentage, seconds;
	mwindow->engine->current_position(&percentage, &seconds);
	
	if(mwindow->asset)
	{
		if(get_keypress())
		{
			if(get_keypress() == LEFT)
			{
				mwindow->gui->unlock_window();
				mwindow->engine->que->send_command(FRAME_REVERSE, percentage);
//printf("MainScrollbar::handle_event %f\n", percentage);
				mwindow->gui->lock_window();
				result = 1;
			}
			else
			if(get_keypress() == RIGHT)
			{
				mwindow->gui->unlock_window();
//printf("MainScrollbar::handle_event %f\n", percentage);
				mwindow->engine->que->send_command(FRAME_FORWARD, percentage);
				mwindow->gui->lock_window();
				result = 1;
			}
		}
		else
		{
//printf("MainScrollbar::handle_event 1\n");
			mwindow->gui->unlock_window();
//printf("MainScrollbar::handle_event 1\n");
			mwindow->engine->que->send_command(STOP_PLAYBACK, percentage);
//printf("MainScrollbar::handle_event 1\n");
			mwindow->engine->interrupt_playback(0);
//printf("MainScrollbar::handle_event 1\n");
			mwindow->gui->lock_window();
//printf("MainScrollbar::handle_event 1\n");
			mwindow->current_position = (double)get_value() / get_length();
//printf("MainScrollbar::handle_event 1\n");
			mwindow->engine->que->send_command(CURRENT_FRAME, 
				mwindow->current_position);
//printf("MainScrollbar::handle_event 2\n");
			result = 1;
		}
	}
	return result;
}

PlayButton::PlayButton(MWindow *mwindow, int x, int y)
 : BC_Button(x, y, mwindow->theme->play)
{
	set_tooltip("Play movie");
	this->mwindow = mwindow;
	mode = 0;
}

PlayButton::~PlayButton()
{
}

int PlayButton::set_mode(int mode)
{
	this->mode = mode;
	switch(mode)
	{
		case 0:
			update_bitmaps(mwindow->theme->play);
			set_tooltip("Play movie");
			break;
		
		case 1:
			update_bitmaps(mwindow->theme->pause);
			set_tooltip("Pause movie");
			break;
	}
	return 0;
}

int PlayButton::handle_event()
{
	double percentage, seconds;
	mwindow->engine->current_position(&percentage, &seconds);
	int temp_mode = mode;

// Use current playback command for context
	mode = temp_mode;
	if(mwindow->engine->command->command == PLAY_FORWARD)
	{
// Stop playback in progress
		mwindow->gui->unlock_window();
		mwindow->engine->que->send_command(STOP_PLAYBACK, percentage);
		mwindow->engine->interrupt_playback(1);
		mwindow->gui->lock_window();
		set_mode(0);
	}
	else
	{
// Play forward
		mwindow->gui->unlock_window();
		mwindow->engine->que->send_command(STOP_PLAYBACK, mwindow->current_position);
		mwindow->engine->interrupt_playback(1);
		mwindow->gui->lock_window();
		set_mode(1);
//printf("PlayButton::handle_event %f\n", mwindow->current_position);
		mwindow->engine->que->send_command(PLAY_FORWARD, mwindow->current_position);
	}
	return 1;
}

int PlayButton::keypress_event()
{
	if(get_keypress() == ' ') 
	{ 
		handle_event(); 
		return 1; 
	}
	return 0;
}


FrameBackButton::FrameBackButton(MWindow *mwindow, int x, int y)
 : BC_Button(x, y, mwindow->theme->frame_bck)
{
	this->mwindow = mwindow;
	set_tooltip("Backward a frame");
}

FrameBackButton::~FrameBackButton()
{
}
int FrameBackButton::handle_event()
{
	mwindow->gui->unlock_window();
	mwindow->engine->que->send_command(FRAME_REVERSE, 0);
	mwindow->gui->lock_window();
	return 1;
}

int FrameBackButton::keypress_event()
{
	if(get_keypress() == LEFT) 
	{ 
		handle_event(); 
		return 1; 
	}
	return 0;
}

FrameForwardButton::FrameForwardButton(MWindow *mwindow, int x, int y)
 : BC_Button(x, y, mwindow->theme->frame_fwd)
{
	this->mwindow = mwindow;
	set_tooltip("Forward a frame");
}

FrameForwardButton::~FrameForwardButton()
{
}
int FrameForwardButton::handle_event()
{
	mwindow->gui->unlock_window();
	mwindow->engine->que->send_command(FRAME_FORWARD, 0);
	mwindow->gui->lock_window();
	return 1;
}

int FrameForwardButton::keypress_event()
{
	if(get_keypress() == RIGHT) 
	{ 
		handle_event(); 
		return 1; 
	}
	return 0;
}



ErrorThread::ErrorThread(MWindowGUI *gui) : Thread()
{
	this->gui = gui;
}

ErrorThread::~ErrorThread()
{
}

int ErrorThread::show_error(char *text)
{
	strcpy(this->text, text);
	this->x = gui->get_abs_cursor_x(0);
	this->y = gui->get_abs_cursor_y(0);
	set_synchronous(0);
	startup_lock.lock();
	start();
	startup_lock.lock();
	startup_lock.unlock();
	return 0;
}

void ErrorThread::run()
{
	ErrorBox box("XMovie: Error", 
		x < BC_INFINITY ? x : gui->get_abs_cursor_x(1), 
		y < BC_INFINITY ? y : gui->get_abs_cursor_y(1));
	box.create_objects(text);
	startup_lock.unlock();
	box.run_window();
}



MainCanvasSubWindow::MainCanvasSubWindow(MWindow *mwindow, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BC_WindowBase::get_resources()->bg_color)
{
	this->mwindow = mwindow;
}
MainCanvasSubWindow::~MainCanvasSubWindow()
{
}


MainCanvasPopup::MainCanvasPopup(MWindow *mwindow, int x, int y, int w, int h)
 : BC_Popup(mwindow->gui, x, y, w, h, 0)
{
	this->mwindow = mwindow;
}
MainCanvasPopup::~MainCanvasPopup()
{
}

MainCanvasFullScreen::MainCanvasFullScreen(MWindow *mwindow, int w, int h, int vm_scale)
 : BC_FullScreen(mwindow->gui, w, h, 0, vm_scale)
{
   this->mwindow = mwindow;
}
MainCanvasFullScreen::~MainCanvasFullScreen()
{
}




