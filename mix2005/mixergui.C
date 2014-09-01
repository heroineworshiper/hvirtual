#include "audiodriver.h"
#include "clip.h"
#include "keys.h"
#include "mixer.h"
#include "mixergui.h"
#include "mixerpopup.h"
#include "mixertree.h"
#include "theme.h"

#include <string.h>





MixerGUI::MixerGUI(Mixer *mixer, int x, int y, int w, int h)
 : BC_Window(PROGRAM_NAME, 
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
	control_window = 0;
	vscroll = 0;
	hscroll = 0;
}

MixerGUI::~MixerGUI()
{
}

void MixerGUI::create_objects()
{
// build permanent controls
	lock_window("MixerGUI::create_objects");
	mixer->theme->calculate_main_sizes();
	add_subwindow(menu = new MixerMenu(mixer));
	menu->create_objects();
	update_display();
	show_window();
	unlock_window();
}

int MixerGUI::create_controls()
{
	return 0;
}

void MixerGUI::update_display(int lock_it)
{
	if(lock_it) lock_window("MixerGUI::update_display");
	if(control_window) control_window->hide_window(0);

// Delete old controls
	controls.remove_all_objects();

	mixer->theme->draw_main_bg();

// Calculate column widths
	title_column_w = 0;
	control_column_w = 0;
	control_w = 0;
	control_h = 0;
	pot_w = 0;
	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *node = mixer->tree->values[i];
		if(node->show)
		{
			int result = get_text_width(MEDIUMFONT, node->title) + 10;
			if(result > title_column_w) title_column_w = result;
			result = node->channels * 40;
			if(result > control_column_w) control_column_w = result;
		}
	}


// Create subwindows
	if(!control_window)
		add_subwindow(control_window = new BC_SubWindow(
			0,
			0,
			mixer->theme->main_subwindow_w,
			mixer->theme->main_subwindow_h));
	control_window->draw_top_background(this, 
		0, 
		0, 
		control_window->get_w(),
		control_window->get_h());
	control_window->flash();

// Create new controls
	int x = 10;
	int y = 10;

// Create toggle controls
	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *node = mixer->tree->values[i];
		if(node->type == MixerNode::TYPE_TOGGLE && node->show)
		{
			MixerControl *control = new MixerControl(mixer,
				node,
				x,
				y);
			control->create_objects();
			controls.append(control);
			y += control->h;
			control_w = MAX(control_w, control->w);
		}
	}



// Get widest menu item
	menu_w = 0;
	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *node = mixer->tree->values[i];
		if(node->type == MixerNode::TYPE_MENU && node->show)
		{
			for(int j = 0; j < node->menu_items.total; j++)
			{
				char *item = node->menu_items.values[j];
				if(get_text_width(MEDIUMFONT, item) > menu_w)
					menu_w = get_text_width(MEDIUMFONT, item);
			}
		}
	}
	menu_w += 50;

// Create menu controls
	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *node = mixer->tree->values[i];
		if(node->type == MixerNode::TYPE_MENU && node->show)
		{
			MixerControl *control = new MixerControl(mixer,
				node,
				x,
				y);
			control->create_objects();
			controls.append(control);
			y += control->h;
			control_w = MAX(control_w, control->w);
		}
	}


// Create pot controls
	for(int i = 0; i < mixer->tree->total; i++)
	{
		MixerNode *node = mixer->tree->values[i];
		if(node->type == MixerNode::TYPE_POT && node->show)
		{
			MixerControl *control = new MixerControl(mixer,
				node,
				x,
				y);
			control->create_objects();
			controls.append(control);
			y += control->h;
			control_w = MAX(control_w, control->w);
		}
	}

// Create scrollbars
	control_h = y - control_h;
	if(!vscroll)
		add_subwindow(vscroll = new MixerVScroll(mixer,
			mixer->theme->main_vscroll_x, 
			mixer->theme->main_vscroll_y, 
			mixer->theme->main_vscroll_h));
	else
		vscroll->update_length(control_h,
			mixer->y_position,
			mixer->theme->main_subwindow_h,
			0);

	if(!hscroll)
		add_subwindow(hscroll = new MixerHScroll(mixer,
			mixer->theme->main_hscroll_x, 
			mixer->theme->main_hscroll_y, 
			mixer->theme->main_hscroll_w));
	else
		hscroll->update_length(control_w,
			mixer->x_position,
			mixer->theme->main_subwindow_w,
			0);

	control_window->show_window(1);
	if(lock_it) unlock_window();
}

void MixerGUI::update_values()
{
	for(int i = 0; i < controls.total; i++)
	{
		controls.values[i]->read_value();
	}
}






int MixerGUI::close_event()
{
	set_done(0);
	return 1;
}

int MixerGUI::keypress_event()
{
	switch(get_keypress())
	{
		case 'q':
		case ESC:
			set_done(0);
			return 1;
			break;
		case PGUP:
		{
			int new_y = mixer->y_position - control_window->get_h();
			new_y = MAX(0, new_y);
			reposition_controls(mixer->x_position, new_y);
			vscroll->update_value(new_y);
			break;
		}
		case PGDN:
		{
			int new_y = mixer->y_position + control_window->get_h();
			new_y = MIN(new_y, control_h - control_window->get_h());
			reposition_controls(mixer->x_position, new_y);
			vscroll->update_value(new_y);
			break;
		}
	}
	return 0;
}



int MixerGUI::button_press_event()
{
	switch(get_buttonpress())
	{
	 	case 3:
			menu->activate_menu();
			return 1;
		case 4:
		{
			int new_y = mixer->y_position - control_window->get_h() / 10;
			new_y = MAX(0, new_y);
			reposition_controls(mixer->x_position, new_y);
			vscroll->update_value(new_y);
			return 1;
			break;
		}
		case 5:
		{
			int new_y = mixer->y_position + control_window->get_h() / 10;
			new_y = MIN(new_y, control_h - control_window->get_h());
			reposition_controls(mixer->x_position, new_y);
			vscroll->update_value(new_y);
			return 1;
			break;
		}
	}
	return 0;
}

int MixerGUI::translation_event()
{
	mixer->x = get_x();
	mixer->y = get_y();
	mixer->save_defaults();
	return 0;
}

int MixerGUI::resize_event(int w, int h)
{
	mixer->w = w;
	mixer->h = h;

	mixer->theme->calculate_main_sizes();
	mixer->save_defaults();
	mixer->theme->draw_main_bg();

	control_window->reposition_window(0, 
		0, 
		mixer->theme->main_subwindow_w,
		mixer->theme->main_subwindow_h);
	control_window->draw_top_background(this, 
		0, 
		0, 
		mixer->theme->main_subwindow_w,
		mixer->theme->main_subwindow_h);
	control_window->flash();
	vscroll->reposition_window(mixer->theme->main_vscroll_x, 
		mixer->theme->main_vscroll_y, 
		mixer->theme->main_vscroll_h);
	vscroll->update_length(vscroll->get_length(),
		mixer->y_position,
		mixer->theme->main_subwindow_h,
		0);
	hscroll->reposition_window(mixer->theme->main_hscroll_x, 
		mixer->theme->main_hscroll_y, 
		mixer->theme->main_hscroll_w);
	hscroll->update_length(hscroll->get_length(),
		mixer->x_position,
		mixer->theme->main_subwindow_w,
		0);
}

void MixerGUI::reposition_controls(int new_x, int new_y)
{
	int x_diff = mixer->x_position - new_x;
	int y_diff = mixer->y_position - new_y;

	for(int i = 0; i < controls.total; i++)
	{
		MixerControl *control = controls.values[i];
		control->reposition(x_diff, y_diff);
	}
	mixer->x_position = new_x;
	mixer->y_position = new_y;
	flush();
}

int MixerGUI::cursor_motion_event()
{
	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();
	if(get_toggle_drag() && 
		cursor_y >= 0 && 
		cursor_y < control_window->get_h())
	{
		for(int i = 0; i < controls.total; i++)
		{
			MixerControl *control = controls.values[i];

			if(control->node->type == MixerNode::TYPE_TOGGLE &&
//			||
//				control->node->type == MixerNode::TYPE_POT &&
//				control->node->has_mute) &&
				cursor_y >= control->y - mixer->y_position && 
				cursor_y < control->y - mixer->y_position + control->h)
			{
				if(control->total_controls > mixer->gui->drag_channel)
				{
					MixerToggle *toggle = control->toggles[mixer->gui->drag_channel];
					if(toggle->get_value() != get_toggle_value())
					{
//						if(control->node->has_mute)
//							control->node->mute[mixer->gui->drag_channel] = 
//								get_toggle_value();
//						else
							control->node->value[mixer->gui->drag_channel] = 
								get_toggle_value();
						toggle->update(get_toggle_value());
						mixer->audio->write_parameters();
					}
				}
			}
		}
	}
	return 0;
}


MixerVScroll::MixerVScroll(Mixer *mixer,
	int x, 
	int y, 
	int h)
 : BC_ScrollBar(x,
 	y,
	SCROLL_VERT,
	h,
	mixer->gui->control_h,
	mixer->y_position,
	h)
{
	this->mixer = mixer;
}

int MixerVScroll::handle_event()
{
	mixer->gui->reposition_controls(mixer->x_position, get_position());
	return 1;
}






MixerHScroll::MixerHScroll(Mixer *mixer,
	int x, 
	int y, 
	int w)
 : BC_ScrollBar(x,
 	y,
	SCROLL_HORIZ,
	w,
	mixer->gui->control_w,
	mixer->x_position,
	w)
{
	this->mixer = mixer;
}

int MixerHScroll::handle_event()
{
	mixer->gui->reposition_controls(get_position(), mixer->y_position);
	return 1;
}











MixerControl::MixerControl(Mixer *mixer, MixerNode *tree_node, int x, int y)
{
	this->mixer = mixer;
	this->node = tree_node;
	this->x = x;
	this->y = y;
	w = 0;
	h = 0;
	total_controls = 0;
	bzero(toggles, sizeof(void*) * MAX_CHANNELS);
	bzero(pots, sizeof(void*) * MAX_CHANNELS);
	bzero(menu, sizeof(void*) * MAX_CHANNELS);
}

MixerControl::~MixerControl()
{
// Can't access node here since it's normally called after the node is deleted.
	delete title;
	for(int i = 0; i < total_controls; i++)
	{
		if(toggles[i]) delete toggles[i];
		if(pots[i]) delete pots[i];
		if(menu[i]) delete menu[i];
	}
}

void MixerControl::create_objects()
{
	int x1 = x - mixer->x_position;
	int y1 = y - mixer->y_position;
	int x_orig = x1;
	int y_orig = y1;
	mixer->gui->control_window->add_subwindow(
		title = new BC_Title(x1, y1, node->title));
	h = title->get_h();


	x1 += mixer->gui->title_column_w;
	type = node->type;
	total_controls = node->channels;
// Stagger the pots
// 	if(type == MixerNode::TYPE_POT &&
// 		mixer->gui->pot_w &&
// 		mixer->gui->controls.total % 2) x1 += mixer->gui->pot_w / 2;

// Create toggles if bound to pots
// 	if(type == MixerNode::TYPE_POT && node->has_mute)
// 	{
// 		for(int i = 0; i < total_controls; i++)
// 		{
// 			mixer->gui->control_window->add_subwindow(
// 					toggles[i] = new MixerToggle(x1, 
// 						y1,
// 						mixer,
// 						this,
// 						i));
// 			x1 += toggles[i]->get_w() + 10;
// 			h = MAX(h, toggles[i]->get_h());
// 		}
// 	}

	for(int i = 0; i < total_controls; i++)
	{
		switch(type)
		{
			case MixerNode::TYPE_TOGGLE:
				mixer->gui->control_window->add_subwindow(
					toggles[i] = new MixerToggle(x1, 
						y1,
						mixer,
						this,
						i));
				x1 += toggles[i]->get_w() + 10;
				h = MAX(h, toggles[i]->get_h());
				break;
			case MixerNode::TYPE_POT:
				mixer->gui->control_window->add_subwindow(
					pots[i] = new MixerPot(x1,
						y1,
						mixer,
						this,
						i));
				x1 += pots[i]->get_w() + 5;
				mixer->gui->pot_w = pots[i]->get_w();
				h = MAX(h, pots[i]->get_h());
				break;
			case MixerNode::TYPE_MENU:
			{
				int menu_w = 0;
				for(int j = 0; j < node->menu_items.total; j++)
				{
					int current_w = mixer->gui->get_text_width(MEDIUMFONT,
						node->menu_items.values[j]);
					if(current_w > menu_w)
						menu_w = current_w;
				}
				mixer->gui->control_window->add_subwindow(
					menu[i] = new MixerPopupMenu(x1,
						y1,
						mixer->gui->menu_w,
						mixer,
						this,
						i));
				menu[i]->create_objects();
				x1 += menu[i]->get_w() + 10;
				h = MAX(h, menu[i]->get_h());
				break;
			}
		}
	}

	w = x1 + 10 - x_orig;
	h += 5;
}

void MixerControl::read_value()
{
	for(int i = 0; i < total_controls; i++)
	{
		if(toggles[i]) toggles[i]->update(node->value[i]);
		if(pots[i]) pots[i]->update(node->value[i]);
		if(menu[i]) menu[i]->set_text(
			node->menu_items.values[
				node->value[i]]);
	}
}

void MixerControl::update(MixerNode *node, int x, int y)
{
}

void MixerControl::update(MixerNode *node)
{
}

void MixerControl::reposition(int x_diff, int y_diff)
{

	title->reposition(title->get_x() + x_diff, title->get_y() + y_diff);
	for(int i = 0; i < total_controls; i++)
	{
		if(toggles[i]) toggles[i]->reposition_window(
			toggles[i]->get_x() + x_diff,
			toggles[i]->get_y() + y_diff);
		if(pots[i]) pots[i]->reposition_window(
			pots[i]->get_x() + x_diff,
			pots[i]->get_y() + y_diff);
		if(menu[i]) menu[i]->reposition_window(
			menu[i]->get_x() + x_diff,
			menu[i]->get_y() + y_diff);
	}
}

int MixerControl::change_value(int value, 
	int current_channel,
	int do_difference)
{
	for(int i = 0; i < mixer->gui->controls.total; i++)
	{
		MixerControl *control = mixer->gui->controls.values[i];
		MixerNode *node = control->node;
		int control_matches = (control == this);

// Control matches
		if(mixer->lock_elements && !control_matches)
		{
			if(!strcmp(node->title_base, this->node->title_base) &&
				node->type == this->node->type) control_matches = 1;
		}

// Control must be visible
// Change all elements or change current element
		if(control_matches)
		{
			for(int i = 0; 
				i < control->total_controls && i < node->channels; 
				i++)
			{
				if(
// Change any channel on any other control
					(mixer->lock_channels && control != this) ||
// Change only unaffected channels on current control
					(mixer->lock_channels && control == this && i != current_channel) ||
// Change only affected channel on other controls
					(!mixer->lock_channels && control != this && i == current_channel))
				{
					int new_value;
					if(do_difference)
						new_value = node->value[i] + value;
					else
						new_value = value;
					CLAMP(new_value, 
						node->min,
						node->max);
					node->value[i] = new_value;
					if(control->toggles[i])
					{
						control->toggles[i]->update(new_value);
					}
					if(control->pots[i])
					{
						control->pots[i]->update(new_value);
						control->pots[i]->last_value = new_value;
					}
					if(control->menu[i])
					{
						control->menu[i]->set_text(
							node->get_menu_item(new_value));
					}
				}
			}
		}
	}
	return 0;
}












MixerPot::MixerPot(int x, 
			int y, 
			Mixer *mixer, 
			MixerControl *control, 
			int channel)
 : BC_IPot(x, 
 	y, 
	control->node->value[channel], 
	control->node->min, 
	control->node->max)
{
	this->mixer = mixer;
	this->control = control;
	this->channel = channel;
	this->last_value = control->node->value[channel];
}


int MixerPot::handle_event()
{
	control->change_value(get_value() - last_value, channel, 1);
	last_value = control->node->value[channel] = get_value();
	mixer->audio->write_parameters();
	return 1;
}






MixerToggle::MixerToggle(int x, 
	int y, 
	Mixer *mixer, 
	MixerControl *control,
	int channel)
 : BC_CheckBox(x, 
 	y, 
	control->node->value[channel])
{
	this->mixer = mixer;
	this->control = control;
	this->channel = channel;
	set_select_drag(1);
}

int MixerToggle::handle_event()
{
//	if(control->node->has_mute)
// Matched to a pot
//		control->node->mute[channel] = get_value();
//	else
// Standalone toggle
		control->node->value[channel] = get_value();
	mixer->gui->drag_channel = channel;
	mixer->audio->write_parameters();
	return 1;
}






MixerPopupMenu::MixerPopupMenu(int x,
	int y, 
	int w,
	Mixer *mixer, 
	MixerControl *control,
	int channel)
 : BC_PopupMenu(x,
 	y,
	w,
	control->node->get_menu_text(channel),
	1)
{
	this->mixer = mixer;
	this->control = control;
	this->channel = channel;
}

void MixerPopupMenu::create_objects()
{
	for(int i = 0; i < control->node->menu_items.total; i++)
	{
		add_item(new BC_MenuItem(control->node->menu_items.values[i]));
	}
}

int MixerPopupMenu::handle_event()
{
	for(int i = 0; i < control->node->menu_items.total; i++)
	{
		if(!strcmp(get_text(), control->node->menu_items.values[i]))
		{
			control->node->value[channel] = i;
			break;
		}
	}
	control->change_value(control->node->value[channel], channel, 0);
	mixer->audio->write_parameters();
	return 1;
}










