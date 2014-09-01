#ifndef SETTINGS_H
#define SETTINGS_H

#include "guicast.h"
#include "mwindow.inc"
#include "mutex.h"
#include "thread.h"

class SettingsWindow;

class SettingsThread : public Thread
{
public:
	SettingsThread(MWindow *mwindow);
	~SettingsThread();

	void run();
	int update_framerate();

	MWindow *mwindow;
	float aspect_w, aspect_h;
	float letter_w, letter_h;
	int square_pixels;
	int crop_letterbox;
	int use_deblocking;
	int smp;
	long prebuffer_size;
	int thread_running;
	int convert_601;
	int audio_priority;
	int mix_strategy;
	int mmx;
	int vdevice;
	SettingsWindow *window;
	Mutex change_lock;
};


class SettingsWindow : public BC_Window
{
public:
	SettingsWindow(MWindow *mwindow, SettingsThread *thread);
	~SettingsWindow();
	
	int initialize();
	
	MWindow *mwindow;
	SettingsThread *thread;
	BC_Title *frame_rate;
	ArrayList<BC_ListBoxItem*> aspect_ratios;
	ArrayList<BC_ListBoxItem*> mix_strategies;
	ArrayList<BC_ListBoxItem*> vdevices;
};

class SettingsW : public BC_TextBox
{
public:
	SettingsW(SettingsThread *thread, int x, int y, char *string);
	~SettingsW();
	
	int handle_event();
	
	SettingsThread *thread;
};

class SettingsH : public BC_TextBox
{
public:
	SettingsH(SettingsThread *thread, int x, int y, char *string);
	~SettingsH();
	
	int handle_event();
	
	SettingsThread *thread;
};

class SettingsLetterW : public BC_TextBox
{
public:
	SettingsLetterW(SettingsThread *thread, int x, int y, char *string);
	~SettingsLetterW();
	int handle_event();
	SettingsThread *thread;
};

class SettingsLetterH : public BC_TextBox
{
public:
	SettingsLetterH(SettingsThread *thread, int x, int y, char *string);
	~SettingsLetterH();
	int handle_event();
	SettingsThread *thread;
};

class SettingsEnableAspect : public BC_CheckBox
{
public:
	SettingsEnableAspect(SettingsThread *thread, int x, int y);
	~SettingsEnableAspect();
	int handle_event();
	SettingsThread *thread;
};

class SettingsEnableLetter : public BC_CheckBox
{
public:
	SettingsEnableLetter(SettingsThread *thread, int x, int y);
	~SettingsEnableLetter();
	int handle_event();
	SettingsThread *thread;
};

class SettingsEnable601 : public BC_CheckBox
{
public:
	SettingsEnable601(SettingsThread *thread, int x, int y);
	~SettingsEnable601();
	int handle_event();
	SettingsThread *thread;
};

class SettingsSMP : public BC_CheckBox
{
public:
	SettingsSMP(SettingsThread *thread, int x, int y);
	~SettingsSMP();
	
	int handle_event();
	
	SettingsThread *thread;
};

class SettingsMMX : public BC_CheckBox
{
public:
	SettingsMMX(SettingsThread *thread, int x, int y);
	~SettingsMMX();
	
	int handle_event();
	
	SettingsThread *thread;
};

class SettingsDeblock : public BC_CheckBox
{
public:
	SettingsDeblock(SettingsThread *thread, int x, int y);
	~SettingsDeblock();
	
	int handle_event();
	
	SettingsThread *thread;
};

class SettingsPrebuff : public BC_TextBox
{
public:
	SettingsPrebuff(SettingsThread *thread, int x, int y, char *string);
	~SettingsPrebuff();
	int handle_event();
	SettingsThread *thread;
};

class SettingsAudioPri : public BC_TextBox
{
public:
	SettingsAudioPri(SettingsThread *thread, int x, int y, char *string);
	~SettingsAudioPri();
	int handle_event();
	SettingsThread *thread;
};


class AspectPulldown : public BC_ListBox
{
public:
	AspectPulldown(SettingsWindow *window, 
		int x, 
		int y, 
		BC_TextBox *textbox_w, 
		BC_TextBox *textbox_h,
		float *output_w, 
		float *output_h);

	int handle_event();
private:
	SettingsWindow *window;
	BC_TextBox *textbox_w;
	BC_TextBox *textbox_h;
	float *output_w;
	float *output_h;
};

class MixStrategy : public BC_PopupTextBox
{
public:
	MixStrategy(SettingsWindow *window, int x, int y);
	~MixStrategy();
	
	int handle_event();
	static char* strategy_to_text(int strategy);
	static int text_to_strategy(char *text);

	SettingsWindow *window;
};

class VDevicePopup : public BC_PopupTextBox
{
public:
	
	VDevicePopup(SettingsWindow *window, int x, int y);
	~VDevicePopup();
	int handle_event();
	SettingsWindow *window;
};



#endif
