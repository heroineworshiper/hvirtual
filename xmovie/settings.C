#include "asset.h"
#include "file.h"
#include "mwindow.h"
#include "mainmenu.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "settings.h"

#include <string.h>

SettingsThread::SettingsThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	thread_running = 0;
}
SettingsThread::~SettingsThread()
{
}

void SettingsThread::run()
{
	if(thread_running) return;
	thread_running = 1;

	aspect_w = mwindow->aspect_w;
	aspect_h = mwindow->aspect_h;
	letter_w = mwindow->letter_w;
	letter_h = mwindow->letter_h;
	mmx = mwindow->use_mmx;
	square_pixels = mwindow->square_pixels;
	crop_letterbox = mwindow->crop_letterbox;
	prebuffer_size = mwindow->prebuffer_size;
	convert_601 = mwindow->convert_601;
	audio_priority = mwindow->audio_priority;
	use_deblocking = mwindow->use_deblocking;
	mix_strategy = mwindow->mix_strategy;
	vdevice = mwindow->video_device;
	change_lock.lock();
	SettingsWindow window(mwindow, this);
	window.initialize();
	this->window = &window;
	change_lock.unlock();

	int result = window.run_window();
	
	if(!result)
	{
		int want_resize = 0;

		if(mwindow->prebuffer_size != prebuffer_size)
		{
//			mwindow->stop_playback();
			mwindow->error_thread->show_error("You must restart XMovie for the changes\nto take effect.");
		}
		
		if(mwindow->use_mmx != mmx)
		{
			mwindow->use_mmx = mmx;
			if(mwindow->video_file)
			{
				mwindow->video_file->lock_read();
				mwindow->video_file->set_mmx(mmx);
				mwindow->video_file->unlock_read();
			}
			if(mwindow->audio_file)
			{
				mwindow->audio_file->lock_read();
				mwindow->audio_file->set_mmx(mmx);
				mwindow->audio_file->unlock_read();
			}
		}

		if(convert_601 != mwindow->convert_601)
		{
			mwindow->convert_601 = convert_601;
		}

		if(aspect_w != mwindow->aspect_w ||
			aspect_h != mwindow->aspect_h ||
			letter_w != mwindow->letter_w ||
			letter_h != mwindow->letter_h ||
			square_pixels != mwindow->square_pixels ||
			crop_letterbox != mwindow->crop_letterbox ||
			square_pixels != mwindow->square_pixels ||
			crop_letterbox != mwindow->crop_letterbox ||
			audio_priority != mwindow->audio_priority ||
			prebuffer_size != mwindow->prebuffer_size ||
			mix_strategy != mwindow->mix_strategy ||
			vdevice != mwindow->video_device ||
			use_deblocking != mwindow->use_deblocking)
		{
			want_resize = 1;
			if(aspect_w <= 0) aspect_w = 1;
			if(aspect_h <= 0) aspect_h = 1;
			mwindow->aspect_w = aspect_w;
			mwindow->aspect_h = aspect_h;
			mwindow->letter_w = letter_w;
			mwindow->letter_h = letter_h;
			mwindow->square_pixels = square_pixels;
			mwindow->crop_letterbox = crop_letterbox;
			mwindow->audio_priority = audio_priority;
			mwindow->prebuffer_size = prebuffer_size;
			mwindow->mix_strategy = mix_strategy;
			mwindow->video_device = vdevice;
			mwindow->use_deblocking = use_deblocking;

			mwindow->gui->lock_window();
			mwindow->gui->resize_event(mwindow->mwindow_w, mwindow->mwindow_h);
			mwindow->gui->unlock_window();
		}

		mwindow->save_defaults();
	}

// Lock out changes to the framerate
	change_lock.lock();
	this->window = 0;
	change_lock.unlock();
	thread_running = 0;
}

int SettingsThread::update_framerate()
{
	change_lock.lock();
	if(window)
	{
		char string[1024];
		
		if(mwindow->actual_framerate > 0)
			sprintf(string, "%.02f", mwindow->actual_framerate);
		else
			sprintf(string, "--");
		window->lock_window();
		window->frame_rate->update(string);
		window->unlock_window();
	}
	change_lock.unlock();
	return 0;
}



SettingsWindow::SettingsWindow(MWindow *mwindow, SettingsThread *thread)
 : BC_Window("XMovie: Settings", 
 	mwindow->gui->get_abs_cursor_x(1), 
	mwindow->gui->get_abs_cursor_y(1),
	250, 
	550, 
	250, 
	550,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}
SettingsWindow::~SettingsWindow()
{
	aspect_ratios.remove_all_objects();
	mix_strategies.remove_all_objects();
}
	
int SettingsWindow::initialize()
{
	char string[1024];
	int x, y;
	BC_TextBox *textbox_w, *textbox_h;
	
	aspect_ratios.append(new BC_ListBoxItem("4:3"));
	aspect_ratios.append(new BC_ListBoxItem("16:9"));
	aspect_ratios.append(new BC_ListBoxItem("2.20:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.25:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.30:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.35:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.66:1"));

	for(int i = 0; i < TOTAL_VDEVICES; i++)
		vdevices.append(new BC_ListBoxItem(MWindowGUI::vdevice_to_text(i)));

	for(int i = 0; i < TOTAL_STRATEGIES; i++)
		mix_strategies.append(new BC_ListBoxItem(MixStrategy::strategy_to_text(i)));

	x = 10;
	y = 10;
	add_tool(new BC_Title(x, y, "Aspect ratio"));
	y += 20;
	sprintf(string, "%.2f", thread->aspect_w);
	add_tool(textbox_w = new SettingsW(thread, x, y, string));
	x += textbox_w->get_w() + 5;
	add_tool(new BC_Title(x, y, ":"));
	x += 10;
	sprintf(string, "%.2f", thread->aspect_h);
	add_tool(textbox_h = new SettingsH(thread, x, y, string));
	x += textbox_h->get_w() + 5;
	add_tool(new AspectPulldown(this, x, y, textbox_w, textbox_h, &thread->aspect_w, &thread->aspect_h));
	y += 25;
	x = 10;
	add_tool(new SettingsEnableAspect(thread, x, y));
	y += 40;
	add_tool(new BC_Title(x, y, "Letterbox ratio:"));
	y += 20;
	sprintf(string, "%.2f", thread->letter_w);
	add_tool(textbox_w = new SettingsLetterW(thread, x, y, string));
	x += textbox_w->get_w() + 5;
	add_tool(new BC_Title(x, y, ":"));
	x += 10;
	sprintf(string, "%.2f", thread->letter_h);
	add_tool(textbox_h = new SettingsLetterH(thread, x, y, string));
	x += textbox_h->get_w() + 5;
	add_tool(new AspectPulldown(this, x, y, textbox_w, textbox_h, &thread->letter_w, &thread->letter_h));
	y += 25;
	x = 10;
	add_tool(new SettingsEnableLetter(thread, x, y));
	y += 40;
//	add_tool(new SettingsEnable601(thread, x, y));
//	y += 30;
// 	add_tool(new SettingsSMP(thread, x, y));
// 	y += 30;
//	add_tool(new SettingsMMX(thread, x, y));
// 	y += 30;
// 	add_tool(new SettingsDeblock(thread, x, y));
	y += 40;
	add_tool(new BC_Title(x, y, "Audio Priority"));
	y += 20;
	sprintf(string, "%ld", thread->audio_priority);
	add_tool(new SettingsAudioPri(thread, x, y, string));
	y += 30;
	add_tool(new BC_Title(x, y, "Preload size"));
	y += 20;
	sprintf(string, "%ld", thread->prebuffer_size);
	add_tool(new SettingsPrebuff(thread, x, y, string));
	y += 40;
	add_tool(new BC_Title(x, y, "Actual framerate:"));
	sprintf(string, "%.2f", mwindow->actual_framerate);
	add_tool(frame_rate = new BC_Title(x + 130, y, string, MEDIUMFONT, RED));
	y += 35;
	add_tool(new BC_Title(x, y, "Downmixing strategy:"));
	y += 20;
	MixStrategy *mixstrategy = new MixStrategy(this, x, y);
	mixstrategy->create_objects();
	y += 35;
	add_tool(new BC_Title(x, y, "Video device:"));
	y += 20;
	VDevicePopup *vdevice_popup = new VDevicePopup(this, x, y);
	vdevice_popup->create_objects();
	y += 35;

	y = get_h() - 50;
	x = 10;
	add_tool(new BC_OKButton(this));
	x = get_w() - 80;
	add_tool(new BC_CancelButton(this));
	show_window();
	return 0;
}

SettingsW::SettingsW(SettingsThread *thread, int x, int y, char *string)
 : BC_TextBox(x, y, 60, 1, string)
{
	this->thread = thread;
}
SettingsW::~SettingsW()
{
}
int SettingsW::handle_event()
{
	thread->aspect_w = atof(get_text());
	return 1;
}

SettingsH::SettingsH(SettingsThread *thread, int x, int y, char *string)
 : BC_TextBox(x, y, 60, 1, string)
{
	this->thread = thread;
}
SettingsH::~SettingsH()
{
}
int SettingsH::handle_event()
{
	thread->aspect_h = atof(get_text());
	return 1;
}

SettingsLetterW::SettingsLetterW(SettingsThread *thread, int x, int y, char *string)
 : BC_TextBox(x, y, 60, 1, string)
{
	this->thread = thread;
}
SettingsLetterW::~SettingsLetterW()
{
}
int SettingsLetterW::handle_event()
{
	thread->letter_w = atof(get_text());
	return 1;
}

SettingsLetterH::SettingsLetterH(SettingsThread *thread, int x, int y, char *string)
 : BC_TextBox(x, y, 60, 1, string)
{
	this->thread = thread;
}
SettingsLetterH::~SettingsLetterH()
{
}
int SettingsLetterH::handle_event()
{
	thread->letter_h = atof(get_text());
	return 1;
}


SettingsPrebuff::SettingsPrebuff(SettingsThread *thread, int x, int y, char *string)
 : BC_TextBox(x, y, 150, 1, string)
{
	this->thread = thread;
}
SettingsPrebuff::~SettingsPrebuff()
{
}
	
int SettingsPrebuff::handle_event()
{
	if(atol(get_text()) >= 0)
		thread->prebuffer_size = atol(get_text());
	return 1;
}

SettingsAudioPri::SettingsAudioPri(SettingsThread *thread, int x, int y, char *string)
 : BC_TextBox(x, y, 150, 1, string)
{
	this->thread = thread;
}
SettingsAudioPri::~SettingsAudioPri()
{
}
	
int SettingsAudioPri::handle_event()
{
	if(atol(get_text()) >= 0)
		thread->audio_priority = atol(get_text());
	return 1;
}


SettingsEnableAspect::SettingsEnableAspect(SettingsThread *thread, int x, int y)
 : BC_CheckBox(x, y, !thread->square_pixels, "Enable aspect ratio")
{
	this->thread = thread;
	set_tooltip("Force window size to\nconform to aspect ratio.");
}
SettingsEnableAspect::~SettingsEnableAspect()
{
}
int SettingsEnableAspect::handle_event()
{
	thread->square_pixels = !get_value();
	return 1;
}

SettingsEnableLetter::SettingsEnableLetter(SettingsThread *thread, int x, int y)
 : BC_CheckBox(x, y, thread->crop_letterbox, "Crop letterbox")
{
	this->thread = thread;
	set_tooltip("Crop letterbox to conform\nto letterbox ratio.");
}
SettingsEnableLetter::~SettingsEnableLetter()
{
}
int SettingsEnableLetter::handle_event()
{
	thread->crop_letterbox = get_value();
	return 1;
}

SettingsEnable601::SettingsEnable601(SettingsThread *thread, int x, int y)
 : BC_CheckBox(x, y, thread->convert_601, "Convert 601 luminance")
{
	this->thread = thread;
	set_tooltip("Expand luminance\n(required for hardware acceleration)");
}
SettingsEnable601::~SettingsEnable601()
{
}
int SettingsEnable601::handle_event()
{
	thread->convert_601 = get_value();
	return 1;
}


SettingsSMP::SettingsSMP(SettingsThread *thread, int x, int y)
 : BC_CheckBox(x, y, thread->smp, "Enable SMP extensions")
{
	this->thread = thread;
	set_tooltip("Use 2 CPUs to decompress video.");
}
SettingsSMP::~SettingsSMP()
{
}

int SettingsSMP::handle_event()
{
	thread->smp = get_value();
	return 1;
}




SettingsMMX::SettingsMMX(SettingsThread *thread, int x, int y)
 : BC_CheckBox(x, y, thread->mmx, "Enable MPEG-1/2 MMX")
{
	this->thread = thread;
	set_tooltip("Enable MPEG-1/2 MMX.  (Lower quality)");
}
SettingsMMX::~SettingsMMX()
{
}

int SettingsMMX::handle_event()
{
	thread->mmx = get_value();
	return 1;
}





// SettingsDeblock::SettingsDeblock(SettingsThread *thread, int x, int y)
//  : BC_CheckBox(x, y, thread->use_deblocking, "Enable MPEG-4 deblocking")
// {
// 	this->thread = thread;
// 	set_tooltip("Enable MPEG-4 MMX.  (Higher quality)");
// }
// SettingsDeblock::~SettingsDeblock()
// {
// }
// 
// int SettingsDeblock::handle_event()
// {
// 	thread->use_deblocking = get_value();
// 	return 1;
// }
// 






AspectPulldown::AspectPulldown(SettingsWindow *window, 
	int x, 
	int y, 
	BC_TextBox *textbox_w, 
	BC_TextBox *textbox_h, 
	float *output_w, 
	float *output_h)
 : BC_ListBox(x, y, 100, 200, 
		LISTBOX_TEXT, 
		&window->aspect_ratios, 
		0, 
		0, 
		1, 
		0, 
		1)
{
	this->window = window;
	this->textbox_w = textbox_w;
	this->textbox_h = textbox_h;
	this->output_w = output_w;
	this->output_h = output_h;
}

int AspectPulldown::handle_event()
{
	char string1[1024], *letter;
	float w, h;

	strcpy(string1, get_selection(0, 0)->get_text());
	letter = strchr(string1, ':');
	*letter = 0;
	w = atof(string1);
	letter++;
	h = atof(letter);
	textbox_w->update(w);
	textbox_h->update(h);
	*output_w = w;
	*output_h = h;
	return 1;
}

#define DOLBY51_TO_STEREO_TEXT "Dolby 5.1 to stereo"
#define DIRECT_COPY_TEXT "Direct copy"
#define STEREO_TO_DOLBY51_TEXT "Stereo to Dolby 5.1"

MixStrategy::MixStrategy(SettingsWindow *window, int x, int y)
 : BC_PopupTextBox(window, 
		&window->mix_strategies,
		strategy_to_text(window->thread->mix_strategy),
		x, 
		y, 
		200,
		200)
{
	this->window = window;
}
MixStrategy::~MixStrategy()
{
}

int MixStrategy::handle_event()
{
	window->thread->mix_strategy = text_to_strategy(get_text());
	return 1;
}

char* MixStrategy::strategy_to_text(int strategy)
{
	switch(strategy)
	{
		case DOLBY51_TO_STEREO:
			return DOLBY51_TO_STEREO_TEXT;
			break;
		case DIRECT_COPY:
			return DIRECT_COPY_TEXT;
			break;
		case STEREO_TO_DOLBY51:
			return STEREO_TO_DOLBY51_TEXT;
			break;
	}

	return DOLBY51_TO_STEREO_TEXT;
}

int MixStrategy::text_to_strategy(char *text)
{
	for(int i = 0; i < TOTAL_STRATEGIES; i++)
	{
		if(!strcasecmp(strategy_to_text(i), text))
			return i;
	}
	return DOLBY51_TO_STEREO;
}




VDevicePopup::VDevicePopup(SettingsWindow *window, int x, int y)
 : BC_PopupTextBox(window, 
		&window->vdevices,
		MWindowGUI::vdevice_to_text(window->thread->vdevice),
		x, 
		y, 
		200,
		200)
{
	this->window = window;
}
VDevicePopup::~VDevicePopup()
{
}

int VDevicePopup::handle_event()
{
	window->thread->vdevice = MWindowGUI::text_to_vdevice(get_text());
	return 1;
}
