
/*
 * CINELERRA
 * Copyright (C) 2017 Adam Williams <broadcast at earthling dot net>
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

#include "affine.h"
#include "bcdisplayinfo.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "spherecam.h"
#include "theme.h"


#include <string.h>



// largely based on equations from http://paulbourke.net/dome/fish2/



REGISTER_PLUGIN(SphereCamMain)




SphereCamConfig::SphereCamConfig()
{
	feather = 0;
	
	for(int i = 0; i < EYES; i++)
	{
		enabled[i] = 1;
		fov[i] = 180;
		radius[i] = 50.0;
		center_y[i] = 50.0;
		rotate_x[i] = 0;
		rotate_y[i] = 0;
		rotate_z[i] = 0;
	}
	
	center_x[0] = 25.0;
	center_x[1] = 75.0;
	draw_guides = 1;
	mode = SphereCamConfig::DO_NOTHING;
}

int SphereCamConfig::equivalent(SphereCamConfig &that)
{
	for(int i = 0; i < EYES; i++)
	{
		if(enabled[i] != that.enabled[i] ||
			!EQUIV(fov[i], that.fov[i]) ||
			!EQUIV(radius[i], that.radius[i]) ||
			!EQUIV(center_x[i], that.center_x[i]) ||
			!EQUIV(center_y[i], that.center_y[i]) ||
			!EQUIV(rotate_x[i], that.rotate_x[i]) ||
			!EQUIV(rotate_y[i], that.rotate_y[i]) ||
			!EQUIV(rotate_z[i], that.rotate_z[i]))
		{
			return 0;
		}
	}



	if(feather != that.feather ||
		mode != that.mode ||
		draw_guides != that.draw_guides)
	{
		return 0;
	}


	return 1;
}

void SphereCamConfig::copy_from(SphereCamConfig &that)
{
	*this = that;
}

void SphereCamConfig::interpolate(SphereCamConfig &prev, 
	SphereCamConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	for(int i = 0; i < EYES; i++)
	{
		enabled[i] = prev.enabled[i];
		fov[i] = prev.fov[i] * prev_scale + next.fov[i] * next_scale;
		radius[i] = prev.radius[i] * prev_scale + next.radius[i] * next_scale;
		center_x[i] = prev.center_x[i] * prev_scale + next.center_x[i] * next_scale;
		center_y[i] = prev.center_y[i] * prev_scale + next.center_y[i] * next_scale;
		rotate_x[i] = prev.rotate_x[i] * prev_scale + next.rotate_x[i] * next_scale;
		rotate_y[i] = prev.rotate_y[i] * prev_scale + next.rotate_y[i] * next_scale;
		rotate_z[i] = prev.rotate_z[i] * prev_scale + next.rotate_z[i] * next_scale;
	}

	feather = prev.feather * prev_scale + next.feather * next_scale;
	draw_guides = prev.draw_guides;
	mode = prev.mode;

	boundaries();
}

void SphereCamConfig::boundaries()
{
	for(int i = 0; i < EYES; i++)
	{
		CLAMP(fov[i], 1.0, 359.0);
		CLAMP(radius[i], 1.0, 100.0);
		CLAMP(center_x[i], 0.0, 100.0);
		CLAMP(center_y[i], 0.0, 100.0);
		CLAMP(rotate_x[i], -180.0, 180.0);
		CLAMP(rotate_y[i], -180.0, 180.0);
		CLAMP(rotate_z[i], -180.0, 180.0);
	}
	
	CLAMP(feather, 0, 50);
}




SphereCamSlider::SphereCamSlider(SphereCamMain *client, 
	SphereCamGUI *gui,
	SphereCamText *text,
	float *output, 
	int x, 
	int y, 
	float min,
	float max)
 : BC_FSlider(x, 
 	y, 
	0, 
	gui->get_w() / 2 - client->get_theme()->widget_border * 3 - 100, 
	gui->get_w() / 2 - client->get_theme()->widget_border * 3 - 100, 
	min, 
	max, 
	*output)
{
	this->gui = gui;
	this->client = client;
	this->output = output;
	this->text = text;
	set_precision(0.01);
}

int SphereCamSlider::handle_event()
{
	float prev_output = *output;
	*output = get_value();
	text->update(*output);


	client->send_configure_change();
	return 1;
}



SphereCamText::SphereCamText(SphereCamMain *client, 
	SphereCamGUI *gui,
	SphereCamSlider *slider,
	float *output, 
	int x, 
	int y)
 : BC_TextBox(x, y, 100, 1, *output)
{
	this->gui = gui;
	this->client = client;
	this->output = output;
	this->slider = slider;
}

int SphereCamText::handle_event()
{
	float prev_output = *output;
	*output = atof(get_text());
	slider->update(*output);


	client->send_configure_change();
	return 1;
}



SphereCamToggle::SphereCamToggle(SphereCamMain *client, 
	int *output, 
	int x, 
	int y,
	const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
	this->client = client;
}

int SphereCamToggle::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}










SphereCamMode::SphereCamMode(SphereCamMain *plugin,  
	SphereCamGUI *gui,
	int x,
	int y)
 : BC_PopupMenu(x,
 	y,
	calculate_w(gui),
	"",
	1)
{
	this->plugin = plugin;
	this->gui = gui;
}

int SphereCamMode::handle_event()
{
	plugin->config.mode = from_text(get_text());
	plugin->send_configure_change();
	return 1;

}

void SphereCamMode::create_objects()
{
	add_item(new BC_MenuItem(to_text(SphereCamConfig::DO_NOTHING)));
	add_item(new BC_MenuItem(to_text(SphereCamConfig::EQUIRECT)));
	update(plugin->config.mode);
}

void SphereCamMode::update(int mode)
{
	char string[BCTEXTLEN];
	sprintf(string, "%s", to_text(mode));
	set_text(string);
}

int SphereCamMode::calculate_w(SphereCamGUI *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(SphereCamConfig::EQUIRECT)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(SphereCamConfig::DO_NOTHING)));
	return result + 50;
}

int SphereCamMode::from_text(char *text)
{
	for(int i = 0; i < 2; i++)
	{
		if(!strcmp(text, to_text(i)))
		{
			return i;
		}
	}

	return SphereCamConfig::EQUIRECT;
}

const char* SphereCamMode::to_text(int mode)
{
	switch(mode)
	{
		case SphereCamConfig::DO_NOTHING:
			return "Do nothing";
			break;
		case SphereCamConfig::EQUIRECT:
			return "Equirectangular";
			break;
	}
	return "Equirectangular";
}
















SphereCamGUI::SphereCamGUI(SphereCamMain *client)
 : PluginClientWindow(client,
	640, 
	650, 
	640, 
	650, 
	0)
{
	this->client = client;
}

SphereCamGUI::~SphereCamGUI()
{
}


void SphereCamGUI::create_objects()
{
	int margin = client->get_theme()->widget_border;
	int margin2 = margin;
	int y = margin;
	int x[EYES];

	x[0] = margin;
	x[1] = get_w() / 2 + margin / 2;

	BC_Title *title;
	SphereCamToggle *toggle;

	for(int i = 0; i < EYES; i++)
	{
		int x3 = x[i];
		y = margin;

		if(i == 0)
		{
			add_tool(title = new BC_Title(x[i], y, _("Left Eye"), LARGEFONT));
		}
		else
		{
			add_tool(title = new BC_Title(x[i], y, _("Right Eye"), LARGEFONT));
		}

		y += title->get_h() + margin2;
		
		
		
		add_tool(enabled[i] = new SphereCamToggle(client, 
			&client->config.enabled[i], 
			x3, 
			y,
			_("Enabled")));
		y += enabled[i]->get_h() + margin2;
		
		add_tool(title = new BC_Title(x3, y, _("FOV:")));
		y += title->get_h() + margin2;
		add_tool(fov_slider[i] = new SphereCamSlider(client, 
			this,
			0,
			&client->config.fov[i], 
			x3, 
			y, 
			1,
			359));
		fov_slider[i]->set_precision(0.1);
		x3 += fov_slider[i]->get_w() + margin;
		add_tool(fov_text[i] = new SphereCamText(client, 
			this,
			fov_slider[i],
			&client->config.fov[i], 
			x3, 
			y));
		fov_slider[i]->text = fov_text[i];
		y += fov_text[i]->get_h() + margin2;

		




		x3 = x[i];
		add_tool(title = new BC_Title(x3, y, _("Radius:")));
		y += title->get_h() + margin2;
		add_tool(radius_slider[i] = new SphereCamSlider(client, 
			this,
			0,
			&client->config.radius[i], 
			x3, 
			y, 
			1,
			100));
		radius_slider[i]->set_precision(0.1);
		x3 += radius_slider[i]->get_w() + margin;
		add_tool(radius_text[i] = new SphereCamText(client, 
			this,
			radius_slider[i],
			&client->config.radius[i], 
			x3, 
			y));
		radius_slider[i]->text = radius_text[i];
		y += radius_text[i]->get_h() + margin2;






		x3 = x[i];
		add_tool(title = new BC_Title(x3, y, _("Center X:")));
		y += title->get_h() + margin2;
		add_tool(centerx_slider[i] = new SphereCamSlider(client, 
			this,
			0,
			&client->config.center_x[i], 
			x3, 
			y, 
			0,
			100));
		centerx_slider[i]->set_precision(0.1);
		x3 += centerx_slider[i]->get_w() + margin;
		add_tool(centerx_text[i] = new SphereCamText(client, 
			this,
			centerx_slider[i],
			&client->config.center_x[i], 
			x3, 
			y));
		centerx_slider[i]->text = centerx_text[i];
		y += centerx_text[i]->get_h() + margin2;







		x3 = x[i];
		add_tool(title = new BC_Title(x3, y, _("Center Y:")));
		y += title->get_h() + margin2;
		add_tool(centery_slider[i] = new SphereCamSlider(client, 
			this,
			0,
			&client->config.center_y[i], 
			x3, 
			y, 
			0,
			100));
		centery_slider[i]->set_precision(0.1);
		x3 += centery_slider[i]->get_w() + margin;
		add_tool(centery_text[i] = new SphereCamText(client, 
			this,
			centery_slider[i],
			&client->config.center_y[i], 
			x3, 
			y));
		centery_slider[i]->text = centery_text[i];
		y += centery_text[i]->get_h() + margin2;








		x3 = x[i];
		add_tool(title = new BC_Title(x3, y, _("Rotate X:")));
		y += title->get_h() + margin2;
		add_tool(rotatex_slider[i] = new SphereCamSlider(client, 
			this,
			0,
			&client->config.rotate_x[i], 
			x3, 
			y, 
			-180,
			180));
		rotatex_slider[i]->set_precision(0.1);
		x3 += rotatex_slider[i]->get_w() + margin;
		add_tool(rotatex_text[i] = new SphereCamText(client, 
			this,
			rotatex_slider[i],
			&client->config.rotate_x[i], 
			x3, 
			y));
		rotatex_slider[i]->text = rotatex_text[i];
		y += rotatex_text[i]->get_h() + margin2;







		x3 = x[i];
		add_tool(title = new BC_Title(x3, y, _("Rotate Y:")));
		y += title->get_h() + margin2;
		add_tool(rotatey_slider[i] = new SphereCamSlider(client, 
			this,
			0,
			&client->config.rotate_y[i], 
			x3, 
			y, 
			-180,
			180));
		rotatey_slider[i]->set_precision(0.1);
		x3 += rotatey_slider[i]->get_w() + margin;
		add_tool(rotatey_text[i] = new SphereCamText(client, 
			this,
			rotatey_slider[i],
			&client->config.rotate_y[i], 
			x3, 
			y));
		rotatey_slider[i]->text = rotatey_text[i];
		y += rotatey_text[i]->get_h() + margin2;







		x3 = x[i];
		add_tool(title = new BC_Title(x3, y, _("Rotate Z:")));
		y += title->get_h() + margin2;
		add_tool(rotatez_slider[i] = new SphereCamSlider(client, 
			this,
			0,
			&client->config.rotate_z[i], 
			x3, 
			y, 
			-180,
			180));
		rotatez_slider[i]->set_precision(0.1);
		x3 += rotatez_slider[i]->get_w() + margin;
		add_tool(rotatez_text[i] = new SphereCamText(client, 
			this,
			rotatez_slider[i],
			&client->config.rotate_z[i], 
			x3, 
			y));
		rotatez_slider[i]->text = rotatez_text[i];
		y += rotatez_text[i]->get_h() + margin2;



	}







	int x3 = x[0];
	add_tool(title = new BC_Title(x3, y, _("Feather:")));
	y += title->get_h() + margin2;
	add_tool(feather_slider = new SphereCamSlider(client, 
		this,
		0,
		&client->config.feather, 
		x3, 
		y, 
		0,
		100));
	feather_slider->set_precision(0.1);
	x3 += feather_slider->get_w() + margin;
	add_tool(feather_text = new SphereCamText(client, 
		this,
		feather_slider,
		&client->config.feather, 
		x3, 
		y));
	feather_slider->text = feather_text;
	y += feather_text->get_h() + margin2;








//printf("SphereCamGUI::create_objects %d %f\n", __LINE__, client->config.distance);


	x3 = x[0];
	add_tool(draw_guides = new SphereCamToggle(client, 
		&client->config.draw_guides, 
		x3, 
		y,
		_("Draw guides")));
	y += draw_guides->get_h() + margin2;

	
	add_tool(title = new BC_Title(x3, y, _("Mode:")));
	add_tool(mode = new SphereCamMode(client, 
		this, 
		x3 + title->get_w() + margin, 
		y));
	mode->create_objects();
	y += mode->get_h() + margin2;



	show_window();
	flush();
}







SphereCamMain::SphereCamMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	affine = 0;
}

SphereCamMain::~SphereCamMain()
{
	delete engine;
	delete affine;
}

NEW_WINDOW_MACRO(SphereCamMain, SphereCamGUI)
LOAD_CONFIGURATION_MACRO(SphereCamMain, SphereCamConfig)
int SphereCamMain::is_realtime() { return 1; }
const char* SphereCamMain::plugin_title() { return N_("Sphere Cam"); }

void SphereCamMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			((SphereCamGUI*)thread->window)->lock_window("SphereCamMain::update_gui");

			for(int i = 0; i < EYES; i++)
			{
				((SphereCamGUI*)thread->window)->enabled[i]->update(config.enabled[i]);

				((SphereCamGUI*)thread->window)->fov_slider[i]->update(config.fov[i]);
				((SphereCamGUI*)thread->window)->fov_text[i]->update(config.fov[i]);

				((SphereCamGUI*)thread->window)->radius_slider[i]->update(config.radius[i]);
				((SphereCamGUI*)thread->window)->radius_text[i]->update(config.radius[i]);

				((SphereCamGUI*)thread->window)->centerx_slider[i]->update(config.center_x[i]);
				((SphereCamGUI*)thread->window)->centerx_text[i]->update(config.center_x[i]);

				((SphereCamGUI*)thread->window)->centery_slider[i]->update(config.center_y[i]);
				((SphereCamGUI*)thread->window)->centery_text[i]->update(config.center_y[i]);

				((SphereCamGUI*)thread->window)->rotatex_slider[i]->update(config.rotate_x[i]);
				((SphereCamGUI*)thread->window)->rotatex_text[i]->update(config.rotate_x[i]);

				((SphereCamGUI*)thread->window)->rotatey_slider[i]->update(config.rotate_y[i]);
				((SphereCamGUI*)thread->window)->rotatey_text[i]->update(config.rotate_y[i]);

				((SphereCamGUI*)thread->window)->rotatez_slider[i]->update(config.rotate_z[i]);
				((SphereCamGUI*)thread->window)->rotatez_text[i]->update(config.rotate_z[i]);
			}

			((SphereCamGUI*)thread->window)->feather_slider->update(config.feather);
			((SphereCamGUI*)thread->window)->feather_text->update(config.feather);

			((SphereCamGUI*)thread->window)->mode->update(config.mode);
			((SphereCamGUI*)thread->window)->draw_guides->update(config.draw_guides);
			((SphereCamGUI*)thread->window)->unlock_window();
		}
	}
}


void SphereCamMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	char string[BCTEXTLEN];



// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("SPHERECAM");
	
	for(int i = 0; i < EYES; i++)
	{
		sprintf(string, "ENABLED_%d", i);
		output.tag.set_property(string, config.enabled[i]);
		sprintf(string, "FOV_%d", i);
		output.tag.set_property(string, config.fov[i]);
		sprintf(string, "RADIUS_%d", i);
		output.tag.set_property(string, config.radius[i]);
		sprintf(string, "CENTER_X_%d", i);
		output.tag.set_property(string, config.center_x[i]);
		sprintf(string, "CENTER_Y_%d", i);
		output.tag.set_property(string, config.center_y[i]);
		sprintf(string, "ROTATE_X_%d", i);
		output.tag.set_property(string, config.rotate_x[i]);
		sprintf(string, "ROTATE_Y_%d", i);
		output.tag.set_property(string, config.rotate_y[i]);
		sprintf(string, "ROTATE_Z_%d", i);
		output.tag.set_property(string, config.rotate_z[i]);
	}
	
	output.tag.set_property("FEATHER", config.feather);
	output.tag.set_property("DRAW_GUIDES", config.draw_guides);
	output.tag.set_property("MODE", config.mode);
	output.append_tag();
	output.terminate_string();

}


void SphereCamMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	char string[BCTEXTLEN];


	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SPHERECAM"))
			{
				for(int i = 0; i < EYES; i++)
				{
					sprintf(string, "ENABLED_%d", i);
					config.enabled[i] = input.tag.get_property(string, config.enabled[i]);
					sprintf(string, "FOV_%d", i);
					config.fov[i] = input.tag.get_property(string, config.fov[i]);
					sprintf(string, "RADIUS_%d", i);
					config.radius[i] = input.tag.get_property(string, config.radius[i]);
					sprintf(string, "CENTER_X_%d", i);
					config.center_x[i] = input.tag.get_property(string, config.center_x[i]);
					sprintf(string, "CENTER_Y_%d", i);
					config.center_y[i] = input.tag.get_property(string, config.center_y[i]);
					sprintf(string, "ROTATE_X_%d", i);
					config.rotate_x[i] = input.tag.get_property(string, config.rotate_x[i]);
					sprintf(string, "ROTATE_Y_%d", i);
					config.rotate_y[i] = input.tag.get_property(string, config.rotate_y[i]);
					sprintf(string, "ROTATE_Z_%d", i);
					config.rotate_z[i] = input.tag.get_property(string, config.rotate_z[i]);
				}

				config.feather = input.tag.get_property("FEATHER", config.feather);
				config.draw_guides = input.tag.get_property("DRAW_GUIDES", config.draw_guides);
				config.mode = input.tag.get_property("MODE", config.mode);
			}
		}
	}
}



int SphereCamMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();
	
	VFrame *input = new_temp(frame->get_w(), frame->get_h(), frame->get_color_model());
	
	read_frame(input, 
		0, 
		start_position, 
		frame_rate,
		0); // use opengl

	calculate_extents();


	if(config.mode == SphereCamConfig::DO_NOTHING)
	{
		get_output()->copy_from(input);
	}
	else
	{
		get_output()->clear_frame();
		if(!engine) engine = new SphereCamEngine(this);
		engine->process_packages();
	}



	if(config.draw_guides)
	{
// input regions
		for(int eye = 0; eye < EYES; eye++)
		{
			if(config.enabled[eye])
			{
// input regions
				get_output()->draw_oval(center_x[eye] - radius[eye], 
					center_y[eye] - radius[eye], 
					center_x[eye] + radius[eye], 
					center_y[eye] + radius[eye]);


// output regions
				get_output()->draw_line(out_x1[eye], 0, out_x1[eye], h);
				get_output()->draw_line(out_x4[eye], 0, out_x4[eye], h);

	// printf("SphereCamMain::process_buffer %d %d %d\n", 
	// __LINE__,
	// out_x1[eye],
	// out_x4[eye]);
				if(feather != 0 && eye == 1)
				{
// crosshatch feather of right eye only, since only the right eye feathers
					int feather_gap = h / 40;
					for(int j = 0; j < h + feather; j += feather_gap)
					{
						draw_feather(out_x1[eye], out_x1[eye] + feather, eye, j);
						draw_feather(out_x4[eye] - feather, out_x4[eye], eye, j);
					}
				}
			}
		}

	}

	return 0;
}

void SphereCamMain::draw_feather(int left, int right, int eye, int y)
{
	int slope = 1;
	if(eye == 1)
	{
		slope = -1;
	}

// wrap
	if(left < 0)
	{
		int left2 = w + left;
		get_output()->draw_line(left2, y, left2 + right - left, y + feather * slope);
	}

	if(right > w)
	{
		int right2 = right - w;
		get_output()->draw_line(0, y, right2, y + feather * slope);

	}

// proper
	get_output()->draw_line(left, y, right, y + feather * slope);

}



void SphereCamMain::calculate_extents()
{
	w = get_output()->get_w();
	h = get_output()->get_h();

	feather = (int)(config.feather * w / 100);
 	feather_x1 = w / 2 - (int)(config.feather * w / 100 / 2);
	feather_x2 = w / 2 + (int)(config.feather * w / 100 / 2);
	
	for(int i = 0; i < EYES; i++)
	{
// innput regions
		center_x[i] = (int)(w * config.center_x[i] / 100);
		center_y[i] = (int)(h * config.center_y[i] / 100);
		radius[i] = (int)(h * config.radius[i] / 100);

// output regions
		int center_x_out = center_x[i] - config.rotate_x[i] * w / 360;
// Assume each lens has to fill 1/2 the width
		out_x1[i] = center_x_out - w / 4 - feather / 2;
		out_x2[i] = center_x_out;
		out_x3[i] = center_x_out;
		out_x4[i] = center_x_out + w / 4 + feather / 2;

// wrap around
		if(out_x1[i] < 0)
		{
			out_x1[i] = w + out_x1[i];
			out_x2[i] = w;
			out_x3[i] = 0;
		}
		
		if(out_x4[i] > w)
		{
			out_x2[i] = w;
			out_x3[i] = 0;
			out_x4[i] -= w;
		}

// printf("SphereCamMain::calculate_extents %d i=%d x=%d y=%d radius=%d\n", 
// __LINE__, 
// i, 
// center_x[i],
// center_y[i],
// radius[i]);
	}
}





SphereCamPackage::SphereCamPackage()
 : LoadPackage() {}





SphereCamUnit::SphereCamUnit(SphereCamEngine *engine, SphereCamMain *plugin)
 : LoadClient(engine)
{
	this->plugin = plugin;
}


SphereCamUnit::~SphereCamUnit()
{
}



// interpolate & accumulate a pixel in the output
#define BLEND_PIXEL(type, components) \
 	if(x_in < 0.0 || x_in >= w - 1 || \
		y_in < 0.0 || y_in >= h - 1) \
	{ \
		out_row += components; \
	} \
	else \
	{ \
		float y1_fraction = y_in - floor(y_in); \
		float y2_fraction = 1.0 - y1_fraction; \
		float x1_fraction = x_in - floor(x_in); \
		float x2_fraction = 1.0 - x1_fraction; \
		type *in_pixel1 = in_rows[(int)y_in] + (int)x_in * components; \
		type *in_pixel2 = in_rows[(int)y_in + 1] + (int)x_in * components; \
		for(int i = 0; i < components; i++) \
		{ \
			float value = in_pixel1[i] * x2_fraction * y2_fraction + \
				in_pixel2[i] * x2_fraction * y1_fraction + \
				in_pixel1[i + components] * x1_fraction * y2_fraction + \
				in_pixel2[i + components] * x1_fraction * y1_fraction; \
			value = *out_row * inv_a + value * a; \
			*out_row++ = (type)value; \
		} \
	}





#define COLORSPACE_SWITCH(function) \
	switch(plugin->get_input()->get_color_model()) \
	{ \
		case BC_RGB888: \
			function(unsigned char, 3, 0x0); \
			break; \
		case BC_RGBA8888: \
			function(unsigned char, 4, 0x0); \
			break; \
		case BC_RGB_FLOAT: \
			function(float, 3, 0.0); \
			break; \
		case BC_RGBA_FLOAT: \
			function(float, 4, 0.0); \
			break; \
		case BC_YUV888: \
			function(unsigned char, 3, 0x80); \
			break; \
		case BC_YUVA8888: \
			function(unsigned char, 4, 0x80); \
			break; \
	}





void SphereCamUnit::process_equirect(SphereCamPackage *pkg)
{
	VFrame *input = plugin->get_temp();
	VFrame *output = plugin->get_output();


// overlay a single eye
#define PROCESS_EQUIRECT(type, components, chroma) \
{ \
	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
 \
	type black[4] = { 0, chroma, chroma, 0 }; \
 \
	for(int y = row1; y < row2; y++) \
	{ \
		type *out_row = out_rows[y]; \
/* polar angle */ \
/* -M_PI/2 to M_PI/2 */ \
		float y_diff = y - center_y; \
		float phi = M_PI / 2 * (y_diff / radius) + rotate_y; \
 \
 		for(int x = 0; x < w; x++) \
		{ \
/* alpha */ \
 			float a = alpha[x]; \
 			float inv_a = 1.0f - a; \
			if(a > 0) \
			{ \
/* polar angle */ \
				float x_diff = x - center_x; \
/* -M_PI/2 to M_PI/2 */ \
				float theta = M_PI / 2 * (x_diff / radius) + rotate_x; \
/* vector in 3D space */ \
				float vect_x = cos(phi) * sin(theta); \
				float vect_y = cos(phi) * cos(theta); \
				float vect_z = sin(phi); \
/* fisheye angle & radius */ \
				float theta2 = atan2(vect_z, vect_x) - rotate_z; \
				float phi2 = atan2(hypot(vect_x, vect_z), vect_y); \
				float r = radius * 2 * phi2 / fov; \
/* pixel in fisheye space */ \
				float x_in = center_x + r * cos(theta2); \
				float y_in = center_y + r * sin(theta2); \
 \
 				BLEND_PIXEL(type, components) \
			} \
			else \
			{ \
				out_row += components; \
			} \
		} \
	} \
}



	int row1 = pkg->row1;
	int row2 = pkg->row2;
	for(int eye = 0; eye < EYES; eye++)
	{
		if(plugin->config.enabled[eye])
		{
			int w = plugin->w;
			int h = plugin->h;
			float fov = plugin->config.fov[eye] * M_PI * 2 / 360;
			float radius = plugin->radius[eye];
			float center_x = plugin->center_x[eye];
			float center_y = plugin->center_y[eye];
			float rotate_x = plugin->config.rotate_x[eye] * M_PI * 2 / 360;
			float rotate_y = plugin->config.rotate_y[eye] * M_PI * 2 / 360;
			float rotate_z = plugin->config.rotate_z[eye] * M_PI * 2 / 360;
			int feather = plugin->feather;
			int out_x1 = plugin->out_x1[eye];
			int out_x2 = plugin->out_x2[eye];
			int out_x3 = plugin->out_x3[eye];
			int out_x4 = plugin->out_x4[eye];

// compute the alpha for every X
			float alpha[w];
			for(int i = 0; i < w; i++)
			{
				alpha[i] = 0;
			}

			for(int i = out_x1; i < out_x2; i++)
			{
				alpha[i] = 1.0f;
			}

			for(int i = out_x3; i < out_x4; i++)
			{
				alpha[i] = 1.0f;
			}
			
			if(eye == 1)
			{
				for(int i = out_x1; i < out_x1 + feather; i++)
				{
					float value = (float)(i - out_x1) / feather;
					if(i >= w)
					{
						alpha[i - w] = value;
					}
					else
					{
						alpha[i] = value;
					}
				}

				for(int i = out_x4 - feather; i < out_x4; i++)
				{
					float value = (float)(out_x4 - i) / feather;
					if(i < 0)
					{
						alpha[w + i] = value;
					}
					else
					{
						alpha[i] = value;
					}
				}
			}
			

	// printf("SphereCamUnit::process_equirect %d rotate_x=%f rotate_y=%f rotate_z=%f\n", 
	// __LINE__, 
	// rotate_x, 
	// rotate_y, 
	// rotate_z);

			COLORSPACE_SWITCH(PROCESS_EQUIRECT)
		}
	}
}



void SphereCamUnit::process_package(LoadPackage *package)
{
	SphereCamPackage *pkg = (SphereCamPackage*)package;

	switch(plugin->config.mode)
	{
		case SphereCamConfig::EQUIRECT:
			process_equirect(pkg);
			break;
	}
}





SphereCamEngine::SphereCamEngine(SphereCamMain *plugin)
 : LoadServer(plugin->PluginClient::smp + 1, plugin->PluginClient::smp + 1)
// : LoadServer(1, 1)
{
	this->plugin = plugin;
}

SphereCamEngine::~SphereCamEngine()
{
}

void SphereCamEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		SphereCamPackage *package = (SphereCamPackage*)LoadServer::get_package(i);
		package->row1 = plugin->get_input()->get_h() * i / LoadServer::get_total_packages();
		package->row2 = plugin->get_input()->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* SphereCamEngine::new_client()
{
	return new SphereCamUnit(this, plugin);
}

LoadPackage* SphereCamEngine::new_package()
{
	return new SphereCamPackage;
}


