/*
 * CINELERRA
 * Copyright (C) 1997-2025 Adam Williams <broadcast at earthling dot net>
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

#include "clip.h"
#include "ctimebar.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "datatype.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "formatpresets.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "preferences.h"
#include "rotateframe.h"
#include "setformat.h"
#include "theme.h"
#include "vframe.h"
#include "vtimebar.h"
#include "vwindow.h"
#include "vwindowgui.h"



SetFormat::SetFormat(MWindow *mwindow)
 : BC_MenuItem(_("EDL settings..."), "Shift+F", 'F')
{
	set_shift(1); 
	this->mwindow = mwindow;
	thread = new SetFormatThread(mwindow);
}

int SetFormat::handle_event()
{
	if(!thread->running())
	{
		thread->start();
	}
	else
	{
// window_lock has to be locked but window can't be locked until after
// it is known to exist, so we neglect window_lock for now
		if(thread->window)
		{
			thread->window_lock->lock("SetFormat::handle_event");
			thread->window->lock_window("SetFormat::handle_event");
			thread->window->raise_window();
			thread->window->unlock_window();
			thread->window_lock->unlock();
		}
	}
	return 1;
}

SetFormatThread::SetFormatThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	window_lock = new Mutex("SetFormatThread::window_lock");
	window = 0;
}

void SetFormatThread::run()
{
	orig_dimension[0] = dimension[0] = mwindow->edl->session->output_w;
	orig_dimension[1] = dimension[1] = mwindow->edl->session->output_h;
	constrain_ratio = 0;
	ratio[0] = ratio[1] = 1;

	new_settings = new EDL;
	new_settings->create_objects();
	new_settings->copy_session(mwindow->edl);

// This locks mwindow, so it must be done outside window_lock
 	int x = mwindow->gui->get_abs_cursor_x(1) - mwindow->theme->setformat_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(1) - mwindow->theme->setformat_h / 2;

	window_lock->lock("SetFormatThread::run 1");
	window = new SetFormatWindow(mwindow, this, x, y);
	window->create_objects();
	window_lock->unlock();

	int result = window->run_window();


	window_lock->lock("SetFormatThread::run 2");
	delete window;
	window = 0;
	window_lock->unlock();


	if(!result)
	{
		apply_changes();
	}

	new_settings->Garbage::remove_user();
}

void SetFormatThread::apply_changes()
{
	double new_samplerate = new_settings->session->sample_rate;
	double old_samplerate = mwindow->edl->session->sample_rate;
	double new_framerate = new_settings->session->frame_rate;
	double old_framerate = mwindow->edl->session->frame_rate;
	int new_channels = new_settings->session->audio_channels;
    int old_channels = mwindow->edl->session->audio_channels;
	CLAMP(new_channels, 1, MAXCHANNELS);


    int redraw_labels = 0;
    for(int i = 0; i < LABEL_COLORS; i++)
    {
        if(mwindow->edl->session->label_text[i].compare(
            new_settings->session->label_text[i]))
        {
            redraw_labels = 1;
            break;
        }
    }
    
    int redraw_meters = (new_channels != old_channels ||
        new_settings->session->min_meter_db != mwindow->edl->session->min_meter_db ||
        new_settings->session->max_meter_db != mwindow->edl->session->max_meter_db);


	mwindow->undo->update_undo_before();

// copy channel positions to the persistent settings
    int need_channel_positions = 0;
    for(int i = 0; i < new_channels; i++)
    {
        if(new_settings->session->achannel_positions[i] !=
            mwindow->edl->session->achannel_positions[i])
        {
            need_channel_positions = 1;
            break;
        }
    }
    
    if(need_channel_positions)
	    memcpy(&mwindow->preferences->channel_positions[MAXCHANNELS * (new_channels - 1)],
		    new_settings->session->achannel_positions,
		    sizeof(int) * MAXCHANNELS);


	mwindow->edl->copy_session(new_settings, 1);
	mwindow->edl->session->output_w = dimension[0];
	mwindow->edl->session->output_h = dimension[1];
	if(need_channel_positions || new_channels != old_channels)
        mwindow->edl->rechannel();
    if((int)old_samplerate != (int)new_samplerate)
        mwindow->edl->resample(old_samplerate, new_samplerate, TRACK_AUDIO);
    if(old_framerate != new_framerate)
        mwindow->edl->resample(old_framerate, new_framerate, TRACK_VIDEO);

    mwindow->save_backup();
	mwindow->undo->update_undo_after(_("set format"), LOAD_ALL);

//printf("SetFormatThread::apply_changes %d %d\n", __LINE__, mwindow->edl->session->output_w);

// Update GUIs
	mwindow->restart_brender();


    if(redraw_labels)
    {
        mwindow->gui->put_event([](void *ptr)
            {
                MWindow::instance->gui->mbuttons->edit_panel->update_label_text();
                MWindow::instance->gui->update_timebar(0);
            },
            0);
        mwindow->cwindow->gui->put_event([](void *ptr)
            {
                MWindow::instance->cwindow->gui->edit_panel->update_label_text();
                MWindow::instance->cwindow->gui->timebar->update_labels();
            },
            0);
	    for(int j = 0; j < mwindow->vwindows.size(); j++)
	    {
		    VWindow *vwindow = mwindow->vwindows.get(j);
		    if(vwindow->is_running())
		    {
                vwindow->gui->put_event([](void *ptr)
                    {
                        VWindow *vwindow = (VWindow*)ptr;
                        vwindow->gui->edit_panel->update_label_text();
                        vwindow->gui->timebar->update_labels();
                    },
                    vwindow);
		    }
	    }
    }

	mwindow->gui->lock_window("SetFormatThread::apply_changes");
    if(redraw_meters) 
        mwindow->gui->set_meter_format(new_settings->session->meter_format,
			new_settings->session->min_meter_db,
			new_settings->session->max_meter_db);
	mwindow->gui->update(1,
		1,
		1,
		1,
		1, 
		1,
		0);
	mwindow->gui->unlock_window();

	mwindow->cwindow->gui->lock_window("SetFormatThread::apply_changes");
	mwindow->cwindow->gui->resize_event(mwindow->cwindow->gui->get_w(), 
		mwindow->cwindow->gui->get_h());
#ifdef USE_METERS
	if(redraw_meters)
    {
        mwindow->cwindow->gui->meters->set_meters(new_channels, 1);
		mwindow->cwindow->gui->meters->change_format(
            new_settings->session->meter_format,
			new_settings->session->min_meter_db,
			new_settings->session->max_meter_db);
    }
#endif
#ifdef USE_SLIDER
	mwindow->cwindow->gui->slider->set_position();
#endif
	mwindow->cwindow->gui->flush();
	mwindow->cwindow->gui->unlock_window();

	for(int i = 0; i < mwindow->vwindows.size(); i++)
	{
		VWindow *vwindow = mwindow->vwindows.get(i);
		vwindow->gui->lock_window("SetFormatThread::apply_changes");
		vwindow->gui->resize_event(vwindow->gui->get_w(), 
			vwindow->gui->get_h());
#ifdef USE_METERS
		if(redraw_meters)
        {
            vwindow->gui->meters->set_meters(new_channels, 1);
			vwindow->gui->meters->change_format(
                new_settings->session->meter_format,
				new_settings->session->min_meter_db,
				new_settings->session->max_meter_db);
        }
#endif
		vwindow->gui->flush();
		vwindow->gui->unlock_window();
	}

// update the meters
    if(redraw_meters)
    {
	    mwindow->lwindow->gui->lock_window("SetFormatThread::apply_changes");
	    mwindow->lwindow->gui->panel->set_meters(new_channels, 1);
		mwindow->lwindow->gui->panel->change_format(
            new_settings->session->meter_format,
			new_settings->session->min_meter_db,
			new_settings->session->max_meter_db);
	    mwindow->lwindow->gui->flush();
	    mwindow->lwindow->gui->unlock_window();
    }

// Warn user
// 	if(((mwindow->edl->session->output_w % 4) ||
// 		(mwindow->edl->session->output_h % 4)) &&
// 		mwindow->edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL)
// 	{
// 		MainError::show_error(
// 			_("This project's dimensions are not multiples of 4 so\n"
// 			"it can't be rendered by OpenGL."));
// 	}


// Flash frame
	mwindow->sync_parameters(CHANGE_ALL);
}

void SetFormatThread::update()
{
	window->sample_rate->update(new_settings->session->sample_rate);
	window->channels->update((int64_t)new_settings->session->audio_channels);
	window->frame_rate->update((float)new_settings->session->frame_rate);

	window->auto_aspect->update(new_settings->session->auto_aspect);

	constrain_ratio = 0;
	dimension[0] = new_settings->session->output_w;
	window->dimension[0]->update((int64_t)dimension[0]);
	dimension[1] = new_settings->session->output_h;
	window->dimension[1]->update((int64_t)dimension[1]);

	ratio[0] = (float)dimension[0] / orig_dimension[0];
	window->ratio[0]->update(ratio[0]);
	ratio[1] = (float)dimension[1] / orig_dimension[1];
	window->ratio[1]->update(ratio[1]);

	window->aspect_w->update(new_settings->session->aspect_w);
	window->aspect_h->update(new_settings->session->aspect_h);

	window->canvas->draw();
}


void SetFormatThread::update_interpolation(int interpolation)
{
	new_settings->session->interpolation_type = interpolation;
	window->nearest_neighbor->update(interpolation == NEAREST_NEIGHBOR);
	window->cubic_cubic->update(interpolation == CUBIC_CUBIC ||
		interpolation == CUBIC_LINEAR ||
		interpolation == LINEAR_LINEAR ||
		interpolation == LANCZOS_LANCZOS);
//	cubic_linear->update(interpolation == CUBIC_LINEAR);
//	linear_linear->update(interpolation == LINEAR_LINEAR);
}

void SetFormatThread::update_window()
{
	int i, result, modified_item, dimension_modified = 0, ratio_modified = 0;

	for(i = 0, result = 0; i < 2 && !result; i++)
	{
		if(dimension[i] < 0)
		{
			dimension[i] *= -1;
			result = 1;
			modified_item = i;
			dimension_modified = 1;
		}
		if(ratio[i] < 0)
		{
			ratio[i] *= -1;
			result = 1;
			modified_item = i;
			ratio_modified = 1;
		}
	}

	if(result)
	{
		if(dimension_modified)
			ratio[modified_item] = (float)dimension[modified_item] / orig_dimension[modified_item];

		if(ratio_modified && !constrain_ratio)
		{
			dimension[modified_item] = (int)(orig_dimension[modified_item] * ratio[modified_item]);
			window->dimension[modified_item]->update((int64_t)dimension[modified_item]);
		}

		for(i = 0; i < 2; i++)
		{
			if(dimension_modified ||
				(i != modified_item && ratio_modified))
			{
				if(constrain_ratio) ratio[i] = ratio[modified_item];
				window->ratio[i]->update(ratio[i]);
			}

			if(ratio_modified ||
				(i != modified_item && dimension_modified))
			{
				if(constrain_ratio) 
				{
					dimension[i] = (int)(orig_dimension[i] * ratio[modified_item]);
					window->dimension[i]->update((int64_t)dimension[i]);
				}
			}
		}
	}

	update_aspect();
}

void SetFormatThread::update_aspect()
{
	if(new_settings->session->auto_aspect)
	{
		char string[BCTEXTLEN];
		MWindow::create_aspect_ratio(new_settings->session->aspect_w, 
			new_settings->session->aspect_h, 
			dimension[0], 
			dimension[1]);
		sprintf(string, "%.02f", new_settings->session->aspect_w);
		window->aspect_w->update(string);
		sprintf(string, "%.02f", new_settings->session->aspect_h);
		window->aspect_h->update(string);
	}
}









SetFormatWindow::SetFormatWindow(MWindow *mwindow, 
	SetFormatThread *thread,
	int x,
	int y)
 : BC_Window(PROGRAM_NAME ": EDL settings",
 	x,
	y,
	mwindow->theme->setformat_w,
	mwindow->theme->setformat_h,
	-1,
	-1,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void SetFormatWindow::create_objects()
{
    Theme *theme = mwindow->theme;
    int margin = theme->widget_border;
    int window_margin = theme->window_border;
	int x = window_margin, y = window_margin;
	BC_Title *title;
	BC_Title *title1;
	BC_Title *title2;
	BC_Title *title3;
	BC_Title *title4;
    char string[BCTEXTLEN];

	lock_window("SetFormatWindow::create_objects");

// AUDIO
	add_subwindow(title = new BC_Title(x, 
		y, 
		_("Audio"), 
		LARGEFONT));
	y += title->get_h() + margin;
    int y1 = y;
	add_subwindow(title1 = new BC_Title(x, y, _("Samplerate:")));
    int text_h = BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1);
    y += text_h + margin;
	add_subwindow(title2 = new BC_Title(x, y, _("Channels:")));
    y += text_h + margin;
	add_subwindow(title3 = new BC_Title(x, 
		y, 
		_("Channel positions:")));
    y += mwindow->theme->get_image("channels_bg")->get_h() + margin;
	add_subwindow(title4 = new BC_Title(x, y, _("Meter range (DB):")));

    int max_w = title1->get_w();
    max_w = MAX(max_w, title2->get_w());
    max_w = MAX(max_w, title3->get_w());
    max_w = MAX(max_w, title4->get_w());
    
    x += max_w + margin;
    y = y1;
    int x1 = x;
	add_subwindow(sample_rate = new SetSampleRateTextBox(thread, 
		x, 
		y));
    x += sample_rate->get_w();
    SampleRatePulldown *menu1;
	add_subwindow(menu1 = new SampleRatePulldown(mwindow, 
		sample_rate, 
		x, 
		y));
    x = x1;
    y += text_h + margin;
	add_subwindow(channels = new SetChannelsTextBox(thread, 
		x, 
		y));
    x += channels->get_w();
	add_subwindow(new BC_ITumbler(channels, 
		1, 
		MAXCHANNELS, 
		x, 
		y));

    x = x1;
	y += text_h + margin;
	add_subwindow(canvas = new SetChannelsCanvas(mwindow, 
		thread, 
		x, 
		y));
	canvas->draw();
    y += canvas->get_h() + margin;
    x = x1;

	add_subwindow(title = new BC_Title(x, y, _("Min:")));
	x += title->get_w() + margin;
	sprintf(string, "%d", thread->new_settings->session->min_meter_db);
	add_subwindow(min_db = new MeterMinDB(thread, string, x, y));

	x += min_db->get_w() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Max:")));
	x += title->get_w() + margin;
	sprintf(string, "%d", thread->new_settings->session->max_meter_db);
	add_subwindow(max_db = new MeterMaxDB(thread, string, x, y));

    x = window_margin;
    y += text_h + margin;
 	add_subwindow(new BC_Bar(x, y, get_w() - margin * 2));
	y += margin;


// VIDEO
	add_subwindow(title = new BC_Title(x, 
		y, 
		_("Video"), 
		LARGEFONT));
	y += title->get_h();
    y1 = y;
	add_subwindow(title1 = new BC_Title(x, y, _("Frame rate:")));
    y += text_h + margin;
	add_subwindow(title2 = new BC_Title(x, y, _("Color model:")));
    y += text_h + margin;
	add_subwindow(title3 = new BC_Title(x, y, _("Canvas size:")));
    y += 2 * (text_h + margin);
	add_subwindow(title4 = new BC_Title(x, y, _("Aspect ratio:")));

    max_w = MAX(title1->get_w(), title2->get_w());
    max_w = MAX(max_w, title3->get_w());
    max_w = MAX(max_w, title4->get_w());

    x += max_w + margin;
    x1 = x;
    y = y1;
	add_subwindow(frame_rate = new SetFrameRateTextBox(thread, 
		x, 
		y));
    x += frame_rate->get_w();
    FrameRatePulldown *menu2;
	add_subwindow(menu2 = new FrameRatePulldown(mwindow, 
		frame_rate, 
		x, 
		y));
    x += menu2->get_w() + margin;

    x = x1;
    y += text_h + margin;
	add_subwindow(color_model = new BC_TextBox(x, 
		y, 
		DP(150), 
		1, 
		""));
	x += color_model->get_w();
	add_subwindow(new ColormodelPulldown(mwindow, 
		color_model, 
		&thread->new_settings->session->color_model,
		x, 
		y));

    x = x1;
    y += text_h + margin;


    y1 = y;
	add_subwindow(title1 = new BC_Title(x, y, _("W:")));
	y += frame_rate->get_h() + margin;
	add_subwindow(title2 = new BC_Title(x, y, _("H:")));
    max_w = MAX(title1->get_w(), title2->get_w());
    x += max_w + margin;
    y = y1;

	add_subwindow(dimension[0] = new ScaleSizeText(x, 
		y, 
		thread, 
		&(thread->dimension[0])));
    y += dimension[0]->get_h() + margin;
	add_subwindow(dimension[1] = new ScaleSizeText(x, 
		y, 
		thread, 
		&(thread->dimension[1])));

// canvas size
	x += dimension[0]->get_w();
    y = y1;
	FrameSizePulldown *pulldown;
	add_subwindow(pulldown = new FrameSizePulldown(mwindow->theme, 
		dimension[0], 
		dimension[1], 
		x, 
		y));
    x += pulldown->get_w();
    FormatSwapExtents *button;
	add_subwindow(button = new FormatSwapExtents(mwindow, 
		thread, 
		this, 
		x,
		y));

    x += button->get_w() + margin;
	add_subwindow(title1 = new BC_Title(x, 
		y, 
		_("W Scale:")));
	y += frame_rate->get_h() + margin;
	add_subwindow(title2 = new BC_Title(x, 
		y, 
		_("H Scale:")));
    max_w = MAX(title1->get_w(), title2->get_w());
    x += max_w + margin;
    y = y1;
	add_subwindow(ratio[0] = new ScaleRatioText(x, 
		y, 
		thread, 
		&(thread->ratio[0])));
    y += text_h + margin;
	add_subwindow(ratio[1] = new ScaleRatioText(x, 
		y, 
		thread, 
		&(thread->ratio[1])));
	y += text_h + margin;

// aspect ratio
	x = x1;
	add_subwindow(aspect_w = new ScaleAspectText(x, 
		y, 
		thread, 
		&(thread->new_settings->session->aspect_w)));
	x += aspect_w->get_w() + margin;
	add_subwindow(title = new BC_Title(x, y, _(":")));
	x += title->get_w() + margin;
	add_subwindow(aspect_h = new ScaleAspectText(x, 
		y, 
		thread, 
		&(thread->new_settings->session->aspect_h)));
	x += aspect_h->get_w();
    AspectPulldown *menu3;
	add_subwindow(menu3 = new AspectPulldown(mwindow, 
		aspect_w, 
		aspect_h, 
		x, 
		y));
	x += menu3->get_w();
	add_subwindow(auto_aspect = new ScaleAspectAuto(x, y, thread));

    y += aspect_w->get_h() + margin;
    x = window_margin;


SET_TRACE
	BC_WindowBase *window;
	add_subwindow(window = new VideoEveryFrame(thread, x, y));
	y += window->get_h() + margin;

    add_subwindow(window = new DisableMutedTracks(thread, x, y));
    y += window->get_h() + margin;

    add_subwindow(window = new OnlyTop(thread, x, y));
    y += window->get_h() + margin;

 	add_subwindow(title = new BC_Title(x, y, _("CPU camera/projector scaling:")));
    x += title->get_w() + margin;
	add_subwindow(nearest_neighbor = new PlaybackNearest(thread, 
		thread->new_settings->session->interpolation_type == NEAREST_NEIGHBOR, 
		x, 
		y));
    x += nearest_neighbor->get_w() + margin;
	add_subwindow(cubic_cubic = new PlaybackBicubicBicubic(thread, 
		thread->new_settings->session->interpolation_type == CUBIC_CUBIC ||
			thread->new_settings->session->interpolation_type == CUBIC_LINEAR ||
			thread->new_settings->session->interpolation_type == LINEAR_LINEAR ||
			thread->new_settings->session->interpolation_type == LANCZOS_LANCZOS, 
		x, 
		y));
	y += cubic_cubic->get_h() + margin;
    x = window_margin;

	add_subwindow(title1 = new BC_Title(x, y, _("Preload buffer for Quicktime:"), MEDIUMFONT));
	sprintf(string, "%d", (int)thread->new_settings->session->playback_preload);
	PlaybackPreload *preload;
	x1 = title1->get_x() + title1->get_w() + margin;

	add_subwindow(preload = new PlaybackPreload(x1, y, thread, string));

	y += preload->get_h() + margin;
	add_subwindow(title1 = new BC_Title(x, y, _("DVD Subtitle to display:")));
	PlaybackSubtitleNumber *subtitle_number;
	x1 = x + title1->get_w() + margin;
	subtitle_number = new PlaybackSubtitleNumber(x1, 
		y, 
		thread);
	subtitle_number->create_objects();

	PlaybackSubtitle *subtitle_toggle;
	x1 += subtitle_number->get_w() + margin;
	add_subwindow(subtitle_toggle = new PlaybackSubtitle(
		x1, 
		y, 
		thread));
	y += subtitle_number->get_h();


	add_subwindow(interpolate_raw = new PlaybackInterpolateRaw(
		x, 
		y,
		thread));
	y += interpolate_raw->get_h() + margin;

// 	add_subwindow(hw_decode = new PlaybackHWDecode(
// 		x, 
// 		y,
// 		pwindow,
// 		this));
// 	y += hw_decode->get_h() + margin;
// 
// 	add_subwindow(ffmpeg_mov = new PlaybackFFmpegMov(
// 		x, 
// 		y,
// 		pwindow,
// 		this));
// 	y += ffmpeg_mov->get_h() + margin;

	add_subwindow(white_balance_raw = new PlaybackWhiteBalanceRaw(
		x, 
		y,
		thread));
	y += white_balance_raw->get_h() + margin;

	add_subwindow(title1 = new BC_Title(x, y, _("Frames per foot:")));
    x += title1->get_w() + margin;

	sprintf(string, "%0.2f", thread->new_settings->session->frames_per_foot);
	TimeFormatFeetSetting *text;
    add_subwindow(text = new TimeFormatFeetSetting(thread, 
		x, 
		y, 
		string));
	y += text_h + margin;
    x = window_margin;


 	add_subwindow(new BC_Bar(x, y, get_w() - margin * 2));
	y += margin;

// LABELS
	add_subwindow(title = new BC_Title(x, 
		y, 
		_("Label text"), 
		LARGEFONT));
	y += title->get_h();
    x1 = x;
    for(int i = 0; i < LABEL_COLORS; i++)
    {
        if((i % 2) == 0) x = x1;
        draw_vframe(theme->label_icon[i], x, y);
        x += theme->label_icon[0]->get_w() + margin;
        LabelText *text;
        
        add_subwindow(text = new LabelText(thread, 
            x, 
            y, 
            i));
        x += text->get_w() + margin;
        if((i % 2) == 1) y += text->get_h() + margin;
    }

	BC_OKTextButton *ok;
	BC_CancelTextButton *cancel;
	add_subwindow(ok = new BC_OKTextButton(this));
	add_subwindow(cancel = new BC_CancelTextButton(this));
	add_subwindow(new SetFormatApply((ok->get_x() + cancel->get_x()) / 2, 
		ok->get_y(), 
		thread));
	flash();
	show_window();
	unlock_window();
}

const char* SetFormatWindow::get_preset_text()
{
	return "";
}














SetFormatPresets::SetFormatPresets(MWindow *mwindow, 
	SetFormatWindow *gui, 
	int x, 
	int y)
 : FormatPresets(mwindow, 0, gui, x, y)
{
	
}

SetFormatPresets::~SetFormatPresets()
{
}

int SetFormatPresets::handle_event()
{
	format_gui->thread->update();
	return 1;
}

EDL* SetFormatPresets::get_edl()
{
	return format_gui->thread->new_settings;
}















SetSampleRateTextBox::SetSampleRateTextBox(SetFormatThread *thread, int x, int y)
 : BC_TextBox(x, y, DP(100), 1, (int64_t)thread->new_settings->session->sample_rate)
{
	this->thread = thread;
}
int SetSampleRateTextBox::handle_event()
{
	thread->new_settings->session->sample_rate = CLIP(atol(get_text()), 1, 1000000);
	return 1;
}

SetChannelsTextBox::SetChannelsTextBox(SetFormatThread *thread, int x, int y)
 : BC_TextBox(x, y, DP(100), 1, thread->new_settings->session->audio_channels)
{
	this->thread = thread;
}
int SetChannelsTextBox::handle_event()
{
	int new_channels = CLIP(atoi(get_text()), 1, MAXCHANNELS);
	
	thread->new_settings->session->audio_channels = new_channels;


	if(new_channels > 0)
	{
		memcpy(thread->new_settings->session->achannel_positions,
			&thread->mwindow->preferences->channel_positions[MAXCHANNELS * (new_channels - 1)],
			sizeof(int) * MAXCHANNELS);
	}


	thread->window->canvas->draw();
	return 1;
}


SetChannelsCanvas::SetChannelsCanvas(MWindow *mwindow, 
	SetFormatThread *thread, 
	int x, 
	int y)
 : BC_SubWindow(x, 
 	y, 
	mwindow->theme->get_image("channels_bg")->get_w(),
	mwindow->theme->get_image("channels_bg")->get_h())
{
	this->thread = thread;
	this->mwindow = mwindow;
	active_channel = -1;
	box_r = mwindow->theme->channel_position_data->get_w() / 2;
	temp_picon = new VFrame(0, 
		-1,
		mwindow->theme->channel_position_data->get_w(),
		mwindow->theme->channel_position_data->get_h(),
		mwindow->theme->channel_position_data->get_color_model(),
		-1);
	rotater = new RotateFrame(mwindow->preferences->processors,
		mwindow->theme->channel_position_data->get_w(),
		mwindow->theme->channel_position_data->get_h());
}
SetChannelsCanvas::~SetChannelsCanvas()
{
	delete temp_picon;
	delete rotater;
}

int SetChannelsCanvas::initialize()
{
	BC_SubWindow::initialize();
    set_background(mwindow->theme->get_image("channels_bg"));
    return 0;
}

int SetChannelsCanvas::draw(int angle)
{
	set_color(RED);
	int real_w = get_w() - box_r * 2;
	int real_h = get_h() - box_r * 2;
	int real_x = box_r;
	int real_y = box_r;

	draw_background(0, 0, get_w(), get_h());
//	draw_vframe(mwindow->theme->channel_bg_data, 0, 0);




	int x, y, w, h;
	char string[BCTEXTLEN];
	set_color(mwindow->theme->channel_position_color);
	for(int i = 0; i < thread->new_settings->session->audio_channels; i++)
	{
//printf("SetChannelsCanvas::draw %d i=%d angle=%d\n",
//__LINE__, i, thread->new_settings->session->achannel_positions[i]);
		get_dimensions(thread->new_settings->session->achannel_positions[i], 
			x, 
			y, 
			w, 
			h);
		double rotate_angle = thread->new_settings->session->achannel_positions[i];
		rotate_angle = -rotate_angle;
		while(rotate_angle < 0) rotate_angle += 360;
		rotater->rotate(temp_picon, 
			mwindow->theme->channel_position_data, 
			rotate_angle, 
			0);

		BC_Pixmap temp_pixmap(this, 
			temp_picon, 
			PIXMAP_ALPHA,
			0);
		draw_pixmap(&temp_pixmap, x, y);
		sprintf(string, "%d", i + 1);
		draw_text(x + 2, y + box_r * 2 - 2, string);
	}

	if(angle > -1)
	{
		sprintf(string, _("%d degrees"), angle);
		draw_text(this->get_w() / 2 - DP(40), this->get_h() / 2, string);
	}

	flash();
	return 0;
}

int SetChannelsCanvas::get_dimensions(int channel_position, 
	int &x, 
	int &y, 
	int &w, 
	int &h)
{
#define MARGIN -DP(10)
	int real_w = this->get_w() - box_r * 2 - MARGIN;
	int real_h = this->get_h() - box_r * 2 - MARGIN;
	float corrected_position = channel_position;
	if(corrected_position < 0) corrected_position += 360;
	Units::polar_to_xy((float)corrected_position, real_w / 2, x, y);
	x += real_w / 2 + MARGIN / 2;
	y += real_h / 2 + MARGIN / 2;
	w = box_r * 2;
	h = box_r * 2;
	return 0;
}

int SetChannelsCanvas::button_press_event()
{
	if(!cursor_inside()) return 0;
// get active channel
	for(int i = 0; 
		i < thread->new_settings->session->audio_channels; 
		i++)
	{
		int x, y, w, h;
		get_dimensions(thread->new_settings->session->achannel_positions[i], 
			x, 
			y, 
			w, 
			h);
		if(get_cursor_x() > x && get_cursor_y() > y && 
			get_cursor_x() < x + w && get_cursor_y() < y + h)
		{
			active_channel = i;
			degree_offset = (int)Units::xy_to_polar(get_cursor_x() - this->get_w() / 2, get_cursor_y() - this->get_h() / 2);
			degree_offset += 90;
			if(degree_offset >= 360) degree_offset -= 360;
			degree_offset -= thread->new_settings->session->achannel_positions[i];
			draw(thread->new_settings->session->achannel_positions[i]);
			return 1;
		}
	}
	return 0;
}

int SetChannelsCanvas::button_release_event()
{
	if(active_channel >= 0)
	{
		active_channel = -1;
		draw(-1);
		return 1;
	}
	return 0;
}

int SetChannelsCanvas::cursor_motion_event()
{
	if(active_channel >= 0)
	{
// get degrees of new channel
		int new_d;
		new_d = (int)Units::xy_to_polar(get_cursor_x() - this->get_w() / 2, get_cursor_y() - this->get_h() / 2);
		new_d += 90;
		new_d -= degree_offset;

		while(new_d >= 360) new_d -= 360;
		while(new_d < 0) new_d += 360;

		if(thread->new_settings->session->achannel_positions[active_channel] != new_d)
		{
			thread->new_settings->session->achannel_positions[active_channel] = new_d;
			int new_channels = thread->new_settings->session->audio_channels;
			memcpy(&thread->mwindow->preferences->channel_positions[MAXCHANNELS * (new_channels - 1)],
				thread->new_settings->session->achannel_positions,
				sizeof(int) * MAXCHANNELS);
			draw(thread->new_settings->session->achannel_positions[active_channel]);
		}
		return 1;
	}
	return 0;
}









SetFrameRateTextBox::SetFrameRateTextBox(SetFormatThread *thread, int x, int y)
 : BC_TextBox(x, y, DP(100), 1, (float)thread->new_settings->session->frame_rate)
{
	this->thread = thread;
}

int SetFrameRateTextBox::handle_event()
{
	thread->new_settings->session->frame_rate = Units::atoframerate(get_text());
	return 1;
}


// 
// SetVChannels::SetVChannels(SetFormatThread *thread, int x, int y)
//  : BC_TextBox(x, y, 100, 1, thread->channels)
// {
// 	this->thread = thread;
// }
// int SetVChannels::handle_event()
// {
// 	thread->channels = atol(get_text());
// 	return 1;
// }




ScaleSizeText::ScaleSizeText(int x, int y, SetFormatThread *thread, int *output)
 : BC_TextBox(x, y, DP(100), 1, *output)
{ 
	this->thread = thread; 
	this->output = output; 
}
ScaleSizeText::~ScaleSizeText()
{
}
int ScaleSizeText::handle_event()
{
	*output = atol(get_text());
//	*output /= 2;
//	*output *= 2;
	if(*output <= 0) *output = 2;
	if(*output > 32768) *output = 32768;
	*output *= -1;
	thread->update_window();
    return 0;
}



ScaleRatioText::ScaleRatioText(int x, 
	int y, 
	SetFormatThread *thread, 
	float *output)
 : BC_TextBox(x, y, DP(100), 1, *output)
{ 
	this->thread = thread; 
	this->output = output; 
}
ScaleRatioText::~ScaleRatioText()
{
}
int ScaleRatioText::handle_event()
{
	*output = atof(get_text());
	//if(*output <= 0) *output = 1;
	if(*output > 10000) *output = 10000;
	if(*output < -10000) *output = -10000;
	*output *= -1;
	thread->update_window();
	return 1;
}



ScaleAspectAuto::ScaleAspectAuto(int x, int y, SetFormatThread *thread)
 : BC_CheckBox(x, 
    y, 
    thread->new_settings->session->auto_aspect, 
    _("Auto"))
{
	this->thread = thread; 
}

ScaleAspectAuto::~ScaleAspectAuto()
{
}

int ScaleAspectAuto::handle_event()
{
	thread->new_settings->session->auto_aspect = get_value();
	thread->update_aspect();
    return 0;
}

ScaleAspectText::ScaleAspectText(int x, int y, SetFormatThread *thread, float *output)
 : BC_TextBox(x, y, DP(80), 1, *output, 1, MEDIUMFONT, 2)
{
	this->output = output;
	this->thread = thread;
}
ScaleAspectText::~ScaleAspectText()
{
}

int ScaleAspectText::handle_event()
{
	*output = atof(get_text());
	return 1;
}






SetFormatApply::SetFormatApply(int x, int y, SetFormatThread *thread)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->thread = thread;
}

int SetFormatApply::handle_event()
{
	thread->apply_changes();
	return 1;
}




FormatSwapExtents::FormatSwapExtents(MWindow *mwindow, 
	SetFormatThread *thread,
	SetFormatWindow *gui, 
	int x, 
	int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("swap_extents"))
{
	this->mwindow = mwindow;
	this->thread = thread;
	this->gui = gui;
	set_tooltip("Swap dimensions");
}

int FormatSwapExtents::handle_event()
{
	int w = thread->dimension[0];
	int h = thread->dimension[1];
	thread->dimension[0] = -h;
	gui->dimension[0]->update((int64_t)h);
	gui->dimension[1]->update((int64_t)w);
	thread->update_window();
	thread->dimension[1] = -w;
	thread->update_window();
	return 1;
}




LabelText::LabelText(SetFormatThread *thread, int x, int y, int color)
 : BC_TextBox(x, y, DP(200), 1, &thread->new_settings->session->label_text[color])
{
    this->thread = thread;
    this->color = color;
}

int LabelText::handle_event()
{
    thread->new_settings->session->label_text[color].assign(get_text());
    return 1;
}






PlaybackNearest::PlaybackNearest(SetFormatThread *thread, int value, int x, int y)
 : BC_Radial(x, y, value, _("Low quality"))
{
	this->thread = thread;
}
int PlaybackNearest::handle_event()
{
	thread->update_interpolation(NEAREST_NEIGHBOR);
	return 1;
}





PlaybackBicubicBicubic::PlaybackBicubicBicubic(SetFormatThread *thread, int value, int x, int y)
 : BC_Radial(x, y, value, _("High quality"))
{
	this->thread = thread;
}
int PlaybackBicubicBicubic::handle_event()
{
	thread->update_interpolation(CUBIC_CUBIC);
	return 1;
}




PlaybackBicubicBilinear::PlaybackBicubicBilinear(SetFormatThread *thread, int value, int x, int y)
 : BC_Radial(x, y, value, _("Bicubic enlarge and bilinear reduce"))
{
	this->thread = thread;
}
int PlaybackBicubicBilinear::handle_event()
{
	thread->update_interpolation(CUBIC_LINEAR);
	return 1;
}


PlaybackBilinearBilinear::PlaybackBilinearBilinear(SetFormatThread *thread, 
	int value, 
	int x, 
	int y)
 : BC_Radial(x, y, value, _("Bilinear enlarge and bilinear reduce"))
{
	this->thread = thread;
}
int PlaybackBilinearBilinear::handle_event()
{
	thread->update_interpolation(LINEAR_LINEAR);
	return 1;
}

PlaybackLanczos::PlaybackLanczos(SetFormatThread *thread, 
	int value, 
	int x, 
	int y)
 : BC_Radial(x, y, value, _("Lanczos"))
{
	this->thread = thread;
}
int PlaybackLanczos::handle_event()
{
	thread->update_interpolation(LANCZOS_LANCZOS);
	return 1;
}


PlaybackPreload::PlaybackPreload(int x, 
	int y, 
	SetFormatThread *thread, 
	char *text)
 : BC_TextBox(x, y, DP(100), 1, text)
{ 
	this->thread = thread; 
}

int PlaybackPreload::handle_event() 
{ 
	thread->new_settings->session->playback_preload = atol(get_text()); 
	return 1;
}


PlaybackInterpolateRaw::PlaybackInterpolateRaw(
	int x, 
	int y, 
	SetFormatThread *thread)
 : BC_CheckBox(x, 
 	y, 
	thread->new_settings->session->interpolate_raw, 
	_("Interpolate CR2 images (restart required)"))
{
	this->thread = thread;
}

int PlaybackInterpolateRaw::handle_event()
{
	thread->new_settings->session->interpolate_raw = get_value();
// 	if(!pwindow->thread->edl->session->interpolate_raw)
// 	{
// 		playback->white_balance_raw->update(0, 0);
// 		playback->white_balance_raw->disable();
// 	}
// 	else
// 	{
// 		playback->white_balance_raw->update(pwindow->thread->edl->session->white_balance_raw, 0);
// 		playback->white_balance_raw->enable();
// 	}
	return 1;
}




PlaybackWhiteBalanceRaw::PlaybackWhiteBalanceRaw(
	int x, 
	int y, 
	SetFormatThread *thread)
 : BC_CheckBox(x, 
 	y, 
//	pwindow->thread->edl->session->interpolate_raw &&
		thread->new_settings->session->white_balance_raw, 
	_("White balance CR2 images"))
{
	this->thread = thread;
//	if(!pwindow->thread->edl->session->interpolate_raw) disable();
}

int PlaybackWhiteBalanceRaw::handle_event()
{
	thread->new_settings->session->white_balance_raw = get_value();
	return 1;
}




// VideoAsynchronous::VideoAsynchronous(PreferencesWindow *pwindow, int x, int y)
//  : BC_CheckBox(x, 
//  	y, 
// 	pwindow->thread->edl->session->video_every_frame &&
// 		pwindow->thread->edl->session->video_asynchronous, 
// 	_("Decode frames asynchronously"))
// {
// 	this->pwindow = pwindow;
// 	if(!pwindow->thread->edl->session->video_every_frame)
// 		disable();
// }
// 
// int VideoAsynchronous::handle_event()
// {
// 	pwindow->thread->edl->session->video_asynchronous = get_value();
// 	return 1;
// }




VideoEveryFrame::VideoEveryFrame(SetFormatThread *thread,
	int x, 
	int y)
 : BC_CheckBox(x, y, thread->new_settings->session->video_every_frame, _("Play every frame"))
{
	this->thread = thread;
}

int VideoEveryFrame::handle_event()
{
	thread->new_settings->session->video_every_frame = get_value();
	return 1;
}

DisableMutedTracks::DisableMutedTracks(SetFormatThread *thread,
	int x, 
	int y)
 : BC_CheckBox(x, y, thread->new_settings->session->disable_muted, _("Disable muted video tracks"))
{
	this->thread = thread;
}

int DisableMutedTracks::handle_event()
{
	thread->new_settings->session->disable_muted = get_value();
	return 1;
}

OnlyTop::OnlyTop(SetFormatThread *thread,
	int x, 
	int y)
 : BC_CheckBox(x, y, thread->new_settings->session->only_top, _("Play only 1 video track"))
{
	this->thread = thread;
}

int OnlyTop::handle_event()
{
	thread->new_settings->session->only_top = get_value();
	return 1;
}







PlaybackSubtitle::PlaybackSubtitle(int x, 
	int y, 
	SetFormatThread *thread)
 : BC_CheckBox(x, 
 	y, 
	thread->new_settings->session->decode_subtitles,
	_("Enable subtitles"))
{
	this->thread = thread;
}

int PlaybackSubtitle::handle_event()
{
	thread->new_settings->session->decode_subtitles = get_value();
	return 1;
}










PlaybackSubtitleNumber::PlaybackSubtitleNumber(int x, 
	int y, 
	SetFormatThread *thread)
 : BC_TumbleTextBox(thread->window,
 	thread->new_settings->session->subtitle_number,
 	0,
	31,
	x, 
 	y, 
	DP(50))
{
	this->thread = thread;
}

int PlaybackSubtitleNumber::handle_event()
{
	thread->new_settings->session->subtitle_number = atoi(get_text());
	return 1;
}



TimeFormatFeetSetting::TimeFormatFeetSetting(SetFormatThread *thread, int x, int y, char *string)
 : BC_TextBox(x, y, DP(90), 1, string)
{
	this->thread = thread;
}
int TimeFormatFeetSetting::handle_event()
{
	thread->new_settings->session->frames_per_foot = atof(get_text());
	if(thread->new_settings->session->frames_per_foot < 1) thread->new_settings->session->frames_per_foot = 1;
	return 0;
}




MeterMinDB::MeterMinDB(SetFormatThread *thread, char *text, int x, int y)
 : BC_TextBox(x, y, DP(50), 1, text)
{
	this->thread = thread;
}
int MeterMinDB::handle_event()
{ 
	thread->new_settings->session->min_meter_db = atol(get_text()); 
	return 0;
}




MeterMaxDB::MeterMaxDB(SetFormatThread *thread, char *text, int x, int y)
 : BC_TextBox(x, y, DP(50), 1, text)
{
	this->thread = thread;
}
int MeterMaxDB::handle_event()
{ 
	thread->new_settings->session->max_meter_db = atol(get_text()); 
	return 0;
}


