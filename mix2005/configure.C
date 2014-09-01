#include "clip.h"
#include "configure.h"
#include "mixer.h"
#include "mixergui.h"
#include "mixertree.h"
#include "theme.h"


ConfigureThread::ConfigureThread(Mixer *mixer)
 : BC_DialogThread()
{
	this->mixer = mixer;
}

ConfigureThread::~ConfigureThread()
{
}

void ConfigureThread::handle_close_event(int result)
{
	mixer->defaults->update("CONFIGURE_X", x_position);
	mixer->defaults->update("CONFIGURE_Y", y_position);
	mixer->defaults->update("CONFIGURE_W", w);
	mixer->defaults->update("CONFIGURE_H", h);
	mixer->save_defaults();
	mixer->gui->lock_window("ConfigureThread::handle_close_event");
	mixer->gui->update_display();
	mixer->gui->unlock_window();
}

BC_Window* ConfigureThread::new_gui()
{
	w = mixer->defaults->get("CONFIGURE_W", 640);
	h = mixer->defaults->get("CONFIGURE_H", 480);
	int x = mixer->gui->get_abs_cursor_x(1) - w / 2;
	int y = mixer->gui->get_abs_cursor_y(1) - h / 2;
	x_position = mixer->defaults->get("CONFIGURE_X", 0);
	y_position = mixer->defaults->get("CONFIGURE_Y", 0);

	gui = new ConfigureGUI(mixer, this, x, y, w, h);
	gui->create_objects();
	return gui;
}





ConfigureGUI::ConfigureGUI(Mixer *mixer, 
	ConfigureThread *thread, 
	int x, 
	int y, 
	int w, 
	int h)
 : BC_Window(PROGRAM_NAME ": Configure",
 	x,
	y,
	w,
	h,
	20,
	20,
	1,
	0,
	1)
{
	this->mixer = mixer;
	this->thread = thread;
}

#define CREATE_TOGGLE \
	ConfigureCheckBox *checkbox; \
	control_window->add_subwindow(checkbox =  \
		new ConfigureCheckBox(mixer,  \
		x,  \
		y,  \
		node->title, \
		node)); \
	y += 30; \
	if(checkbox->get_w() + 10 > thread->control_w)  \
		thread->control_w = checkbox->get_w() + 10 > thread->control_w; \
	checkboxes.append(checkbox); \
	thread->control_w = MAX(thread->control_w, checkbox->get_w());


void ConfigureGUI::create_objects()
{
	lock_window("ConfigureGUI::create_objects");
	int x = -thread->x_position + 10;
	int y = -thread->y_position + 10;
	thread->control_h = -thread->y_position;
	thread->control_w = 0;
	mixer->theme->calculate_config_sizes();

	add_subwindow(control_window = new BC_SubWindow(0, 
		0, 
		mixer->theme->config_subwindow_w,
		mixer->theme->config_subwindow_h));
	mixer->theme->draw_config_bg();
	control_window->draw_top_background(this,
		0, 
		0, 
		control_window->get_w(), 
		control_window->get_h());


// Create items in the same order as the GUI
	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *node = mixer->tree->values[i];
		if(node->type == MixerNode::TYPE_TOGGLE)
		{
			CREATE_TOGGLE
		}
	}

	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *node = mixer->tree->values[i];
		if(node->type == MixerNode::TYPE_MENU)
		{
			CREATE_TOGGLE
		}
	}

	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *node = mixer->tree->values[i];
		if(node->type == MixerNode::TYPE_POT)
		{
			CREATE_TOGGLE
		}
	}

	thread->control_h = y - thread->control_h;
	add_subwindow(vscroll = new ConfigureVScroll(mixer,
		thread,
		mixer->theme->config_vscroll_x, 
		mixer->theme->config_vscroll_y, 
		mixer->theme->config_vscroll_h));
	add_subwindow(hscroll = new ConfigureHScroll(mixer,
		thread,
		mixer->theme->config_hscroll_x, 
		mixer->theme->config_hscroll_y, 
		mixer->theme->config_hscroll_w));

	show_window();
	flush();
	unlock_window();
}

int ConfigureGUI::resize_event(int w, int h)
{
	thread->w = w;
	thread->h = h;
	mixer->theme->calculate_config_sizes();

	control_window->reposition_window(0,
		0,
		mixer->theme->config_subwindow_w,
		mixer->theme->config_subwindow_h);
	mixer->theme->draw_config_bg();
	control_window->draw_top_background(this,
		0, 
		0, 
		control_window->get_w(), 
		control_window->get_h());

	vscroll->reposition_window(mixer->theme->config_vscroll_x,
		mixer->theme->config_vscroll_y,
		mixer->theme->config_vscroll_h);
	vscroll->update_length(vscroll->get_length(),
		thread->y_position,
		mixer->theme->config_vscroll_h,
		0);
	hscroll->reposition_window(mixer->theme->config_hscroll_x,
		mixer->theme->config_hscroll_y,
		mixer->theme->config_hscroll_w);
	hscroll->update_length(hscroll->get_length(),
		thread->x_position,
		mixer->theme->config_hscroll_w,
		0);
}

int ConfigureGUI::button_press_event()
{
	switch(get_buttonpress())
	{
		case 4:
		{
			int new_y = thread->y_position - control_window->get_h() / 10;
			new_y = MAX(0, new_y);
			reposition_controls(thread->x_position, new_y);
			vscroll->update_value(new_y);
			break;
		}
		case 5:
		{
			int new_y = thread->y_position + control_window->get_h() / 10;
			new_y = MIN(new_y, thread->control_h - control_window->get_h());
			reposition_controls(thread->x_position, new_y);
			vscroll->update_value(new_y);
			break;
		}
	}
	return 0;
}


void ConfigureGUI::reposition_controls(int new_x, int new_y)
{
	int x_diff = thread->x_position - new_x;
	int y_diff = thread->y_position - new_y;

	for(int i = 0; i < checkboxes.total; i++)
	{
		ConfigureCheckBox *checkbox = checkboxes.values[i];
		checkbox->reposition_window(checkbox->get_x() + x_diff, 
			checkbox->get_y() + y_diff);
	}
	thread->x_position = new_x;
	thread->y_position = new_y;
	flush();
}

int ConfigureGUI::cursor_motion_event()
{
	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();
	if(get_toggle_drag() &&
		cursor_y >= 0 &&
		cursor_y < control_window->get_h())
	{
		for(int i = 0; i < checkboxes.total; i++)
		{
			ConfigureCheckBox *checkbox = checkboxes.values[i];
			if(cursor_y >= checkbox->get_y() &&
				cursor_y < checkbox->get_y() + checkbox->get_h())
			{
				int prev_value = checkbox->node->show;
				checkbox->node->show = get_toggle_value();
				checkbox->update(get_toggle_value());

				if(prev_value != checkbox->node->show)
				{
					unlock_window();
					mixer->gui->update_display(1);
					lock_window("ConfigureGUI::cursor_motion_event");
				}
			}
		}
	}
	return 0;
}

	





ConfigureCheckBox::ConfigureCheckBox(Mixer *mixer, 
	int x, 
	int y, 
	char *text,
	MixerNode *node)
 : BC_CheckBox(x, y, node->show, text)
{
	this->mixer = mixer;
	this->node = node;
	set_select_drag(1);
}

int ConfigureCheckBox::handle_event()
{
	node->show = get_value();
	unlock_window();
	mixer->gui->update_display(1);
	lock_window("ConfigureCheckBox::handle_event");
	return 1;
}








ConfigureVScroll::ConfigureVScroll(Mixer *mixer,
	ConfigureThread *thread,
	int x, 
	int y, 
	int h)
 : BC_ScrollBar(x,
 	y,
	SCROLL_VERT,
	h,
	thread->control_h,
	thread->y_position,
	thread->gui->control_window->get_h())
{
	this->thread = thread;
	this->mixer = mixer;
}

int ConfigureVScroll::handle_event()
{
	thread->gui->reposition_controls(mixer->x_position, get_position());
	return 1;
}




ConfigureHScroll::ConfigureHScroll(Mixer *mixer,
	ConfigureThread *thread,
	int x, 
	int y, 
	int w)
 : BC_ScrollBar(x,
 	y,
	SCROLL_HORIZ,
	w,
	thread->control_w,
	thread->x_position,
	thread->gui->control_window->get_w())
{
	this->thread = thread;
	this->mixer = mixer;
}

int ConfigureHScroll::handle_event()
{
	thread->gui->reposition_controls(get_position(), mixer->y_position);
	return 1;
}









