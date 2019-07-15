
/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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
#include "bchash.h"
#include "bcsignals.h"
#include "eqcanvas.h"
#include "filesystem.h"
#include "language.h"
#include "reverb.h"
#include "reverbwindow.h"
#include "theme.h"

#include <string.h>

#define TEXT_W DP(90)
#define WINDOW_W DP(400)
#define WINDOW_H DP(490)

ReverbWindow::ReverbWindow(Reverb *reverb)
 : PluginClientWindow(reverb, 
	WINDOW_W, 
	WINDOW_H, 
	WINDOW_W, 
	WINDOW_H, 
	0)
{ 
	this->reverb = reverb; 
}

ReverbWindow::~ReverbWindow()
{
    for(int i = 0; i < TOTAL_PARAMS; i++)
    {
        delete params[i];
    }
    delete canvas;
}

void ReverbWindow::create_objects()
{
	int margin = client->get_theme()->widget_border;
	int x = DP(200), y = margin;
    int x1 = x + BC_Pot::calculate_w();
    int x2 = x1 + BC_Pot::calculate_w() + margin;
    int height = BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1) + margin;


    int i = 0;
    params[i] = new ReverbParam(reverb,
        this,
        margin, 
        x,
        x2,
        y, 
        0,  // output_i
        &reverb->config.level_init, // output_f
        0, // output_q
        "Initial signal level:",
        INFINITYGAIN, // min
        0); // max
    params[i]->initialize();
    i++;
    y += height;

    params[i] = new ReverbParam(reverb,
        this,
        margin, 
        x1,
        x2,
        y, 
        &reverb->config.delay_init,  // output_i
        0, // output_f
        0, // output_q
        "ms before reflections:",
        0, // min
        MAX_DELAY_INIT); // max
    params[i]->initialize();
    i++;
    y += height;

    params[i] = new ReverbParam(reverb,
        this,
        margin, 
        x,
        x2,
        y, 
        0,  // output_i
        &reverb->config.ref_level1, // output_f
        0, // output_q
        "First reflection level:",
        INFINITYGAIN, // min
        0); // max
    params[i]->initialize();
    i++;
    y += height;

    params[i] = new ReverbParam(reverb,
        this,
        margin, 
        x1,
        x2,
        y, 
        0,  // output_i
        &reverb->config.ref_level2, // output_f
        0, // output_q
        "Last reflection level:",
        INFINITYGAIN, // min
        0); // max
    params[i]->initialize();
    i++;
    y += height;

    params[i] = new ReverbParam(reverb,
        this,
        margin, 
        x,
        x2,
        y, 
        &reverb->config.ref_total,  // output_i
        0, // output_f
        0, // output_q
        "Number of reflections:",
        MIN_REFLECTIONS, // min
        MAX_REFLECTIONS); // max
    params[i]->initialize();
    i++;
    y += height;

    params[i] = new ReverbParam(reverb,
        this,
        margin, 
        x1,
        x2,
        y, 
        &reverb->config.ref_length,  // output_i
        0, // output_f
        0, // output_q
        "ms of reflections:",
        0, // min
        MAX_REFLENGTH); // max
    params[i]->initialize();
    i++;
    y += height;

    params[i] = new ReverbParam(reverb,
        this,
        margin, 
        x,
        x2,
        y, 
        0,  // output_i
        0, // output_f
        &reverb->config.low, // output_q
        "Low freq of bandpass:",
        0, // min
        0); // max
    params[i]->initialize();
    i++;
    y += height;

    params[i] = new ReverbParam(reverb,
        this,
        margin, 
        x1,
        x2,
        y, 
        0,  // output_i
        0, // output_f
        &reverb->config.high, // output_q
        "High freq of bandpass:",
        0, // min
        0); // max
    params[i]->initialize();
    i++;
    y += height;

    params[i] = new ReverbParam(reverb,
        this,
        margin, 
        x,
        x2,
        y, 
        0,  // output_i
        &reverb->config.q, // output_f
        0, // output_q
        "Steepness of bandpass:",
        0.0, // min
        1.0); // max
    params[i]->initialize();
    params[i]->fpot->set_precision(0.01);
    i++;
    y += BC_Pot::calculate_h() + margin;


    BC_Title *title;
    add_subwindow(title = new BC_Title(margin, y, _("Window:")));
	add_subwindow(size = new ReverbSize(this, 
		reverb, 
		margin + title->get_w() + margin, 
		y));
	size->create_objects();
	size->update(reverb->config.window_size);
    y += size->get_h() + margin;

	int canvas_x = 0;
	int canvas_y = y;
	int canvas_w = get_w() - margin;
	int canvas_h = get_h() - canvas_y - margin;
	canvas = new EQCanvas(this,
        canvas_x, 
		canvas_y, 
		canvas_w, 
		canvas_h,
        INFINITYGAIN,
        0.0);
    canvas->initialize();
    update_canvas();

	show_window();
}

void ReverbWindow::update()
{
    for(int i = 0; i < TOTAL_PARAMS; i++)
    {
        params[i]->update(1, 1);
    }
}


void ReverbWindow::update_canvas()
{
    int niquist = reverb->PluginAClient::project_sample_rate / 2;
    BC_SubWindow *gui = canvas->canvas;
    canvas->update_spectrogram(reverb);

// draw the envelope
    reverb->calculate_envelope();
	gui->set_color(WHITE);
	gui->set_line_width(2);

    int y1;
    int window_size = reverb->config.window_size;
    for(int i = 0; i < gui->get_w(); i++)
    {
        int freq = Freq::tofreq(i * TOTALFREQS / gui->get_w());
        int index = (int64_t)freq * (int64_t)window_size / 2 / niquist;
        if(freq < niquist && index < window_size / 2)
        {
            double mag = reverb->envelope[index];
            int y2 = (int)(DB::todb(mag) * gui->get_h() / INFINITYGAIN);
            
            if(y2 >= gui->get_h())
            {
                y2 = gui->get_h() - 1;
            }
            
            if(i > 0)
            {
                gui->draw_line(i - 1, y1, i, y2);
            }
            y1 = y2;
        }
        else
        if(i > 0)
        {
            int y2 = gui->get_h() - 1;
            gui->draw_line(i - 1, y1, i, y2);
            y1 = y2;
        }
    }

    gui->set_line_width(1);
	gui->flash(1);


//printf("ReverbWindow::update_canvas %d\n", __LINE__);
}








ReverbSize::ReverbSize(ReverbWindow *window, Reverb *plugin, int x, int y)
 : BC_PopupMenu(x, y, DP(100), "4096", 1)
{
	this->plugin = plugin;
	this->window = window;
}

int ReverbSize::handle_event()
{
	plugin->config.window_size = atoi(get_text());
	plugin->send_configure_change();

	window->update_canvas();
	return 1;
}

void ReverbSize::create_objects()
{
	add_item(new BC_MenuItem("2048"));
	add_item(new BC_MenuItem("4096"));
	add_item(new BC_MenuItem("8192"));
	add_item(new BC_MenuItem("16384"));
	add_item(new BC_MenuItem("32768"));
	add_item(new BC_MenuItem("65536"));
	add_item(new BC_MenuItem("131072"));
	add_item(new BC_MenuItem("262144"));
}

void ReverbSize::update(int size)
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", size);
	set_text(string);
}








ReverbParam::ReverbParam(Reverb *reverb,
    ReverbWindow *gui,
    int x, 
    int x2,
    int x3,
    int y, 
    int *output_i, 
    float *output_f, // floating point output
    int *output_q,
    const char *title,
    float min,
    float max)
{
    this->output_i = output_i;
    this->output_f = output_f;
    this->output_q = output_q;
    this->title.assign(title);
    this->reverb = reverb;
    this->gui = gui;
    this->x = x;
    this->x2 = x2;
    this->x3 = x3;
    this->y = y;
    this->min = min;
    this->max = max;
    fpot = 0;
    ipot = 0;
    qpot = 0;
    text = 0;
}

ReverbParam::~ReverbParam()
{
    if(fpot) delete fpot;
    if(ipot) delete ipot;
    if(qpot) delete qpot;
    if(text) text;
}


void ReverbParam::initialize()
{
    BC_Title *title_;
    int y2 = y + 
        (BC_Pot::calculate_h() - 
        BC_Title::calculate_h(gui, _(title.c_str()), MEDIUMFONT)) / 2;
    gui->add_tool(title_ = new BC_Title(x, y2, _(title.c_str())));
    
    if(output_f)
    {
        gui->add_tool(fpot = new ReverbFPot(this, x2, y));
    }
    
    if(output_i)
    {
        gui->add_tool(ipot = new ReverbIPot(this, x2, y));
    }
    
    if(output_q)
    {
        gui->add_tool(qpot = new ReverbQPot(this, x2, y));
    }
    
    int y3 = y + 
        (BC_Pot::calculate_h() - 
        BC_TextBox::calculate_h(gui, MEDIUMFONT, 1, 1)) / 2;
    if(output_i)
    {
        gui->add_tool(text = new ReverbText(this, x3, y3, *output_i));
    }
    if(output_f)
    {
        gui->add_tool(text = new ReverbText(this, x3, y3, *output_f));
    }
    if(output_q)
    {
        gui->add_tool(text = new ReverbText(this, x3, y3, *output_q));
    }
}

void ReverbParam::update(int skip_text, int skip_pot)
{
    if(!skip_text)
    {
        if(output_i)
        {
            text->update((int64_t)*output_i);
        }
        if(output_q)
        {
            text->update((int64_t)*output_q);
        }
        if(output_f)
        {
            text->update((float)*output_f);
        }
    }
    
    if(!skip_pot)
    {
        if(ipot)
        {
            ipot->update((int64_t)*output_i);
        }
        if(qpot)
        {
            ipot->update((int64_t)*output_q);
        }
        if(fpot)
        {
            ipot->update((float)*output_f);
        }
    }
}



ReverbFPot::ReverbFPot(ReverbParam *param, int x, int y) 
 : BC_FPot(x, 
 	y, 
	*param->output_f, 
	param->min, 
	param->max)
{
    this->param = param;
    set_use_caption(0);
}

int ReverbFPot::handle_event()
{
	*param->output_f = get_value();
    param->update(0, 1);
	param->reverb->send_configure_change();
    param->gui->update_canvas();
    return 1;
}


ReverbIPot::ReverbIPot(ReverbParam *param, int x, int y)
 : BC_IPot(x, 
 	y, 
	*param->output_i, 
	(int)param->min, 
	(int)param->max)
{
    this->param = param;
    set_use_caption(0);
}

int ReverbIPot::handle_event()
{
	*param->output_i = get_value();
    param->update(0, 1);
	param->reverb->send_configure_change();
    param->gui->update_canvas();
    return 1;
}


ReverbQPot::ReverbQPot(ReverbParam *param, int x, int y)
 : BC_QPot(x, 
 	y, 
	*param->output_q)
{
    this->param = param;
    set_use_caption(0);
}

int ReverbQPot::handle_event()
{
	*param->output_q = get_value();
    param->update(0, 1);
	param->reverb->send_configure_change();
    param->gui->update_canvas();
    return 1;
}


ReverbText::ReverbText(ReverbParam *param, int x, int y, int value)
 : BC_TextBox(x, y, TEXT_W, 1, (int64_t)value, 1, MEDIUMFONT)
{
    this->param = param;
}

ReverbText::ReverbText(ReverbParam *param, int x, int y, float value)
 : BC_TextBox(x, y, TEXT_W, 1, (float)value, 1, MEDIUMFONT, 2)
{
    this->param = param;
}

int ReverbText::handle_event()
{
    if(param->output_i)
    {
        *param->output_i = atoi(get_text());
    }

    if(param->output_f)
    {
        *param->output_f = atof(get_text());
    }

    if(param->output_q)
    {
        *param->output_q = atoi(get_text());
    }
    
    param->update(1, 0);
    param->reverb->send_configure_change();
    param->gui->update_canvas();
    return 1;
}





