
/*
 * CINELERRA
 * Copyright (C) 2012-2017 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "language.h"
#include "motion.h"
#include "motionscan.h"
#include "motionwindow.h"










MotionWindow::MotionWindow(MotionMain *plugin)
 : PluginClientWindow(plugin,
 	DP(600), 
	DP(650), 
	DP(600),
	DP(650),
	0)
{
	this->plugin = plugin; 
}

MotionWindow::~MotionWindow()
{
}

void MotionWindow::create_objects()
{
	int x1 = DP(10), x = DP(10), y = DP(10);
	int x2 = DP(310);
	BC_Title *title;



// 	add_subwindow(global = new MotionGlobal(plugin,
// 		this,
// 		x1,
// 		y));

	add_subwindow(rotate = new MotionRotate(plugin,
		this,
		x2,
		y));
	y += DP(50);

	add_subwindow(title = new BC_Title(x1, 
		y, 
		_("Translation search radius:\n(W/H Percent of image)")));
	add_subwindow(global_range_w = new GlobalRange(plugin, 
		x1 + title->get_w() + DP(10), 
		y,
		&plugin->config.global_range_w));
	add_subwindow(global_range_h = new GlobalRange(plugin, 
		x1 + title->get_w() + DP(10) + global_range_w->get_w(), 
		y,
		&plugin->config.global_range_h));

	add_subwindow(title = new BC_Title(x2, 
		y, 
		_("Rotation search radius:\n(Degrees)")));
	add_subwindow(rotation_range = new RotationRange(plugin, 
		x2 + title->get_w() + DP(10), 
		y));

	y += DP(50);
	add_subwindow(title = new BC_Title(x1, 
		y, 
		_("Translation block size:\n(W/H Percent of image)")));
	add_subwindow(global_block_w = new BlockSize(plugin, 
		x1 + title->get_w() + DP(10), 
		y,
		&plugin->config.global_block_w));
	add_subwindow(global_block_h = new BlockSize(plugin, 
		x1 + title->get_w() + DP(10) + global_block_w->get_w(), 
		y,
		&plugin->config.global_block_h));


	y += DP(50);
	add_subwindow(title = new BC_Title(x, y, _("Translation direction:")));
	add_subwindow(track_direction = new TrackDirection(plugin, 
		this, 
		x + title->get_w() + DP(10), 
		y));
	track_direction->create_objects();

	y += DP(40);
	add_subwindow(title = new BC_Title(x, y + 10, _("Block X:")));
	add_subwindow(block_x = new MotionBlockX(plugin, 
		this, 
		x + title->get_w() + DP(10), 
		y));
	add_subwindow(block_x_text = new MotionBlockXText(plugin, 
		this, 
		x + title->get_w() + DP(10) + block_x->get_w() + DP(10), 
		y + 10));

	add_subwindow(title = new BC_Title(x2, y, _("Rotation center:")));
	add_subwindow(rotation_center = new RotationCenter(plugin,
		x2 + title->get_w() + DP(10),
		y));



	int y1 = y;
	y += DP(50);
	add_subwindow(title = new BC_Title(x2, y + DP(10), _("Maximum angle offset:")));
	add_subwindow(rotate_magnitude = new MotionRMagnitude(plugin, 
		x2 + title->get_w() + DP(10), 
		y));

	y += DP(40);
	add_subwindow(title = new BC_Title(x2, y + DP(10), _("Rotation settling speed:")));
	add_subwindow(rotate_return_speed = new MotionRReturnSpeed(plugin,
		x2 + title->get_w() + DP(10), 
		y));



	y = y1;
	y += DP(40);
	add_subwindow(title = new BC_Title(x, y + DP(10), _("Block Y:")));
	add_subwindow(block_y = new MotionBlockY(plugin, 
		this, 
		x + title->get_w() + DP(10), 
		y));
	add_subwindow(block_y_text = new MotionBlockYText(plugin, 
		this, 
		x + title->get_w() + DP(10) + block_y->get_w() + DP(10), 
		y + DP(10)));

	y += DP(50);
	add_subwindow(title = new BC_Title(x, y + DP(10), _("Maximum absolute offset:")));
	add_subwindow(magnitude = new MotionMagnitude(plugin, 
		x + title->get_w() + DP(10), 
		y));

	y += DP(40);
	add_subwindow(title = new BC_Title(x, y + DP(10), _("Motion settling speed:")));
	add_subwindow(return_speed = new MotionReturnSpeed(plugin,
		x + title->get_w() + DP(10), 
		y));



	y += DP(40);
	add_subwindow(vectors = new MotionDrawVectors(plugin,
		this,
		x,
		y));


	y += DP(40);
	add_subwindow(track_single = new TrackSingleFrame(plugin, 
		this,
		x, 
		y));
	add_subwindow(title = new BC_Title(x + track_single->get_w() + DP(20), 
		y, 
		_("Frame number:")));
	add_subwindow(track_frame_number = new TrackFrameNumber(plugin, 
		this,
		x + track_single->get_w() + title->get_w() + DP(20), 
		y));

	y += DP(20);
	add_subwindow(track_previous = new TrackPreviousFrame(plugin, 
		this,
		x, 
		y));

	y += DP(20);
	add_subwindow(previous_same = new PreviousFrameSameBlock(plugin, 
		this,
		x, 
		y));

	y += DP(40);
	y1 = y;
	add_subwindow(title = new BC_Title(x, y, _("Master layer:")));
	add_subwindow(master_layer = new MasterLayer(plugin, 
		this,
		x + title->get_w() + DP(10), 
		y));
	master_layer->create_objects();
	y += DP(30);


	add_subwindow(title = new BC_Title(x, y, _("Action:")));
	add_subwindow(action_type = new ActionType(plugin, 
		this,
		x + title->get_w() + DP(10), 
		y));
	action_type->create_objects();
	y += DP(30);




	add_subwindow(title = new BC_Title(x, y, _("Calculation:")));
	add_subwindow(tracking_type = new TrackingType(&plugin->config.tracking_type,
        plugin, 
		this, 
		x + title->get_w() + DP(10), 
		y));
	tracking_type->create_objects();



	show_window(1);
}

void MotionWindow::update_mode()
{
	global_range_w->update(plugin->config.global_range_w,
	 	MIN_RADIUS,
	 	MAX_RADIUS);
	global_range_h->update(plugin->config.global_range_h,
	 	MIN_RADIUS,
	 	MAX_RADIUS);
	rotation_range->update(plugin->config.rotation_range,
	 	MIN_ROTATION,
	 	MAX_ROTATION);
	vectors->update(plugin->config.draw_vectors);
//	global->update(plugin->config.global);
	rotate->update(plugin->config.rotate);
}













GlobalRange::GlobalRange(MotionMain *plugin, 
	int x, 
	int y,
	int *value)
 : BC_IPot(x, 
		y, 
		(int64_t)*value,
		(int64_t)MIN_RADIUS,
		(int64_t)MAX_RADIUS)
{
	this->plugin = plugin;
	this->value = value;
}


int GlobalRange::handle_event()
{
	*value = (int)get_value();
	plugin->send_configure_change();
	return 1;
}




RotationRange::RotationRange(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.rotation_range,
		(int64_t)MIN_ROTATION,
		(int64_t)MAX_ROTATION)
{
	this->plugin = plugin;
}


int RotationRange::handle_event()
{
	plugin->config.rotation_range = (int)get_value();
	plugin->send_configure_change();
	return 1;
}




RotationCenter::RotationCenter(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.rotation_center,
		(int64_t)-MAX_ROTATION,
		(int64_t)MAX_ROTATION)
{
	this->plugin = plugin;
}


int RotationCenter::handle_event()
{
	plugin->config.rotation_center = (int)get_value();
	plugin->send_configure_change();
	return 1;
}






BlockSize::BlockSize(MotionMain *plugin, 
	int x, 
	int y,
	int *value)
 : BC_IPot(x, 
		y, 
		(int64_t)*value,
		(int64_t)MIN_BLOCK,
		(int64_t)MAX_BLOCK)
{
	this->plugin = plugin;
	this->value = value;
}


int BlockSize::handle_event()
{
	*value = (int)get_value();
	plugin->send_configure_change();
	return 1;
}













// GlobalSearchPositions::GlobalSearchPositions(MotionMain *plugin, 
// 	int x, 
// 	int y,
// 	int w)
//  : BC_PopupMenu(x,
//  	y,
// 	w,
// 	"",
// 	1)
// {
// 	this->plugin = plugin;
// }
// void GlobalSearchPositions::create_objects()
// {
// 	add_item(new BC_MenuItem("16"));
// 	add_item(new BC_MenuItem("32"));
// 	add_item(new BC_MenuItem("64"));
// 	add_item(new BC_MenuItem("128"));
// 	add_item(new BC_MenuItem("256"));
// 	add_item(new BC_MenuItem("512"));
// 	add_item(new BC_MenuItem("1024"));
// 	add_item(new BC_MenuItem("2048"));
// 	add_item(new BC_MenuItem("4096"));
// 	add_item(new BC_MenuItem("8192"));
// 	add_item(new BC_MenuItem("16384"));
// 	add_item(new BC_MenuItem("32768"));
// 	add_item(new BC_MenuItem("65536"));
// 	add_item(new BC_MenuItem("131072"));
// 	char string[BCTEXTLEN];
// 	sprintf(string, "%d", plugin->config.global_positions);
// 	set_text(string);
// }
// 
// int GlobalSearchPositions::handle_event()
// {
// 	plugin->config.global_positions = atoi(get_text());
// 	plugin->send_configure_change();
// 	return 1;
// }
// 
// 
// 
// 
// 
// 
// 
// RotationSearchPositions::RotationSearchPositions(MotionMain *plugin, 
// 	int x, 
// 	int y,
// 	int w)
//  : BC_PopupMenu(x,
//  	y,
// 	w,
// 	"",
// 	1)
// {
// 	this->plugin = plugin;
// }
// void RotationSearchPositions::create_objects()
// {
// 	add_item(new BC_MenuItem("4"));
// 	add_item(new BC_MenuItem("8"));
// 	add_item(new BC_MenuItem("16"));
// 	add_item(new BC_MenuItem("32"));
// 	char string[BCTEXTLEN];
// 	sprintf(string, "%d", plugin->config.rotate_positions);
// 	set_text(string);
// }
// 
// int RotationSearchPositions::handle_event()
// {
// 	plugin->config.rotate_positions = atoi(get_text());
// 	plugin->send_configure_change();
// 	return 1;
// }








MotionMagnitude::MotionMagnitude(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.magnitude,
		(int64_t)0,
		(int64_t)100)
{
	this->plugin = plugin;
}

int MotionMagnitude::handle_event()
{
	plugin->config.magnitude = (int)get_value();
	plugin->send_configure_change();
	return 1;
}


MotionReturnSpeed::MotionReturnSpeed(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.return_speed,
		(int64_t)0,
		(int64_t)100)
{
	this->plugin = plugin;
}

int MotionReturnSpeed::handle_event()
{
	plugin->config.return_speed = (int)get_value();
	plugin->send_configure_change();
	return 1;
}



MotionRMagnitude::MotionRMagnitude(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.rotate_magnitude,
		(int64_t)0,
		(int64_t)90)
{
	this->plugin = plugin;
}

int MotionRMagnitude::handle_event()
{
	plugin->config.rotate_magnitude = (int)get_value();
	plugin->send_configure_change();
	return 1;
}



MotionRReturnSpeed::MotionRReturnSpeed(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, 
		y, 
		(int64_t)plugin->config.rotate_return_speed,
		(int64_t)0,
		(int64_t)100)
{
	this->plugin = plugin;
}

int MotionRReturnSpeed::handle_event()
{
	plugin->config.rotate_return_speed = (int)get_value();
	plugin->send_configure_change();
	return 1;
}





// MotionGlobal::MotionGlobal(MotionMain *plugin, 
// 	MotionWindow *gui,
// 	int x, 
// 	int y)
//  : BC_CheckBox(x, 
//  	y, 
// 	plugin->config.global,
// 	_("Track translation"))
// {
// 	this->plugin = plugin;
// 	this->gui = gui;
// }
// 
// int MotionGlobal::handle_event()
// {
// 	plugin->config.global = get_value();
// 	plugin->send_configure_change();
// 	return 1;
// }

MotionRotate::MotionRotate(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x, 
 	y, 
	plugin->config.rotate,
	_("Track rotation"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int MotionRotate::handle_event()
{
	plugin->config.rotate = get_value();
	plugin->send_configure_change();
	return 1;
}





MotionBlockX::MotionBlockX(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_FPot(x,
 	y,
	plugin->config.block_x,
	(float)0, 
	(float)100)
{
	this->plugin = plugin;
	this->gui = gui;
}

int MotionBlockX::handle_event()
{
	plugin->config.block_x = get_value();
	gui->block_x_text->update((float)plugin->config.block_x);
	plugin->send_configure_change();
	return 1;
}




MotionBlockY::MotionBlockY(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_FPot(x,
 	y,
	(float)plugin->config.block_y,
	(float)0, 
	(float)100)
{
	this->plugin = plugin;
	this->gui = gui;
}

int MotionBlockY::handle_event()
{
	plugin->config.block_y = get_value();
	gui->block_y_text->update((float)plugin->config.block_y);
	plugin->send_configure_change();
	return 1;
}

MotionBlockXText::MotionBlockXText(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_TextBox(x,
 	y,
	DP(100),
	1,
	(float)plugin->config.block_x)
{
	this->plugin = plugin;
	this->gui = gui;
	set_precision(4);
}

int MotionBlockXText::handle_event()
{
	plugin->config.block_x = atof(get_text());
	gui->block_x->update(plugin->config.block_x);
	plugin->send_configure_change();
	return 1;
}




MotionBlockYText::MotionBlockYText(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_TextBox(x,
 	y,
	DP(100),
	1,
	(float)plugin->config.block_y)
{
	this->plugin = plugin;
	this->gui = gui;
	set_precision(4);
}

int MotionBlockYText::handle_event()
{
	plugin->config.block_y = atof(get_text());
	gui->block_y->update(plugin->config.block_y);
	plugin->send_configure_change();
	return 1;
}
















MotionDrawVectors::MotionDrawVectors(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x,
 	y, 
	plugin->config.draw_vectors,
	_("Draw vectors"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int MotionDrawVectors::handle_event()
{
	plugin->config.draw_vectors = get_value();
	plugin->send_configure_change();
	return 1;
}








TrackSingleFrame::TrackSingleFrame(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_Radial(x, 
 	y, 
	plugin->config.tracking_object == MotionScan::TRACK_SINGLE, 
 	_("Track single frame"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int TrackSingleFrame::handle_event()
{
	plugin->config.tracking_object = MotionScan::TRACK_SINGLE;
	gui->track_previous->update(0);
	gui->previous_same->update(0);
	gui->track_frame_number->enable();
	plugin->send_configure_change();
	return 1;
}








TrackFrameNumber::TrackFrameNumber(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_TextBox(x, y, DP(100), 1, plugin->config.track_frame)
{
	this->plugin = plugin;
	this->gui = gui;
	if(plugin->config.tracking_object != MotionScan::TRACK_SINGLE) disable();
}

int TrackFrameNumber::handle_event()
{
	plugin->config.track_frame = atol(get_text());
	plugin->send_configure_change();
	return 1;
}







TrackPreviousFrame::TrackPreviousFrame(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_Radial(x, 
 	y, 
	plugin->config.tracking_object == MotionScan::TRACK_PREVIOUS, 
	_("Track previous frame"))
{
	this->plugin = plugin;
	this->gui = gui;
}
int TrackPreviousFrame::handle_event()
{
	plugin->config.tracking_object = MotionScan::TRACK_PREVIOUS;
	gui->track_single->update(0);
	gui->previous_same->update(0);
	gui->track_frame_number->disable();
	plugin->send_configure_change();
	return 1;
}








PreviousFrameSameBlock::PreviousFrameSameBlock(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_Radial(x, 
 	y, 
	plugin->config.tracking_object == MotionScan::PREVIOUS_SAME_BLOCK, 
	_("Previous frame same block"))
{
	this->plugin = plugin;
	this->gui = gui;
}
int PreviousFrameSameBlock::handle_event()
{
	plugin->config.tracking_object = MotionScan::PREVIOUS_SAME_BLOCK;
	gui->track_single->update(0);
	gui->track_previous->update(0);
	gui->track_frame_number->disable();
	plugin->send_configure_change();
	return 1;
}








MasterLayer::MasterLayer(MotionMain *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x, 
 	y, 
	calculate_w(gui),
	to_text(plugin->config.bottom_is_master))
{
	this->plugin = plugin;
	this->gui = gui;
}

int MasterLayer::handle_event()
{
	plugin->config.bottom_is_master = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void MasterLayer::create_objects()
{
	add_item(new BC_MenuItem(to_text(0)));
	add_item(new BC_MenuItem(to_text(1)));
}

int MasterLayer::from_text(char *text)
{
	if(!strcmp(text, _("Top"))) return 0;
	return 1;
}

char* MasterLayer::to_text(int mode)
{
	return mode ? _("Bottom") : _("Top");
}

int MasterLayer::calculate_w(MotionWindow *gui)
{
	int result = 0;
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(0)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(1)));
	return result + 50;
}








ActionType::ActionType(MotionMain *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x, 
 	y, 
	calculate_w(gui),
	to_text(plugin->config.action_type))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ActionType::handle_event()
{
	plugin->config.action_type = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void ActionType::create_objects()
{
	add_item(new BC_MenuItem(to_text(MotionScan::TRACK)));
	add_item(new BC_MenuItem(to_text(MotionScan::TRACK_PIXEL)));
	add_item(new BC_MenuItem(to_text(MotionScan::STABILIZE)));
	add_item(new BC_MenuItem(to_text(MotionScan::STABILIZE_PIXEL)));
	add_item(new BC_MenuItem(to_text(MotionScan::STABILIZE_SPHERE)));
	add_item(new BC_MenuItem(to_text(MotionScan::NOTHING)));
}

int ActionType::from_text(char *text)
{
	if(!strcmp(text, _("Track Subpixel"))) return MotionScan::TRACK;
	if(!strcmp(text, _("Track Pixel"))) return MotionScan::TRACK_PIXEL;
	if(!strcmp(text, _("Stabilize Subpixel"))) return MotionScan::STABILIZE;
	if(!strcmp(text, _("Stabilize Pixel"))) return MotionScan::STABILIZE_PIXEL;
	if(!strcmp(text, _("Stabilize Sphere"))) return MotionScan::STABILIZE_SPHERE;
	if(!strcmp(text, _("Do Nothing"))) return MotionScan::NOTHING;
    return 0;
}

char* ActionType::to_text(int mode)
{
	switch(mode)
	{
		case MotionScan::TRACK:
			return _("Track Subpixel");
			break;
		case MotionScan::TRACK_PIXEL:
			return _("Track Pixel");
			break;
		case MotionScan::STABILIZE:
			return _("Stabilize Subpixel");
			break;
		case MotionScan::STABILIZE_PIXEL:
			return _("Stabilize Pixel");
			break;
		case MotionScan::STABILIZE_SPHERE:
			return _("Stabilize Sphere");
			break;
		case MotionScan::NOTHING:
			return _("Do Nothing");
			break;
	}
    return 0;
}

int ActionType::calculate_w(MotionWindow *gui)
{
	int result = 0;
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::TRACK)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::TRACK_PIXEL)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::STABILIZE)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::STABILIZE_PIXEL)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::STABILIZE_SPHERE)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(MotionScan::NOTHING)));
	return result + 50;
}






TrackDirection::TrackDirection(MotionMain *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x, 
 	y, 
	calculate_w(gui),
	to_text(plugin->config.horizontal_only, plugin->config.vertical_only))
{
	this->plugin = plugin;
	this->gui = gui;
}

int TrackDirection::handle_event()
{
	from_text(&plugin->config.horizontal_only, &plugin->config.vertical_only, get_text());
	plugin->send_configure_change();
	return 1;
}

void TrackDirection::create_objects()
{
	add_item(new BC_MenuItem(to_text(1, 0)));
	add_item(new BC_MenuItem(to_text(0, 1)));
	add_item(new BC_MenuItem(to_text(0, 0)));
}

void TrackDirection::from_text(int *horizontal_only, int *vertical_only, char *text)
{
	*horizontal_only = 0;
	*vertical_only = 0;
	if(!strcmp(text, to_text(1, 0))) *horizontal_only = 1;
	if(!strcmp(text, to_text(0, 1))) *vertical_only = 1;
}

char* TrackDirection::to_text(int horizontal_only, int vertical_only)
{
	if(horizontal_only) return _("Horizontal only");
	if(vertical_only) return _("Vertical only");
	return _("Both");
}

int TrackDirection::calculate_w(MotionWindow *gui)
{
	int result = 0;
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(1, 0)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(0, 1)));
	result = MAX(result, BC_PopupMenu::calculate_w(gui, to_text(0, 0)));
	return result + 50;
}

